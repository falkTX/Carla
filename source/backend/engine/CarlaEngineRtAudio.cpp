/*
 * Carla RtAudio Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifdef WANT_RTAUDIO

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include "RtAudio.h"
#include "RtMidi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------

static const char** gRetNames = nullptr;
static std::vector<RtAudio::Api> gRtAudioApis;

static void initRtApis()
{
    if (gRtAudioApis.size() == 0)
        RtAudio::getCompiledApi(gRtAudioApis);
}

RtMidi::Api getMatchedAudioMidiAPi(const RtAudio::Api rtApi)
{
    switch (rtApi)
    {
    case RtAudio::UNSPECIFIED:
        return RtMidi::UNSPECIFIED;

    case RtAudio::LINUX_ALSA:
    case RtAudio::LINUX_OSS:
    case RtAudio::LINUX_PULSE:
        return RtMidi::LINUX_ALSA;

    case RtAudio::UNIX_JACK:
#if defined(CARLA_OS_WIN)
        return RtMidi::WINDOWS_MM;
#elif defined(CARLA_OS_MAC)
        return RtMidi::MACOSX_CORE;
#elif defined(CARLA_OS_LINUX)
        return RtMidi::LINUX_ALSA;
#else
        return RtMidi::UNIX_JACK;
#endif

    case RtAudio::MACOSX_CORE:
        return RtMidi::MACOSX_CORE;

    case RtAudio::WINDOWS_ASIO:
    case RtAudio::WINDOWS_DS:
        return RtMidi::WINDOWS_MM;

    case RtAudio::RTAUDIO_DUMMY:
        return RtMidi::RTMIDI_DUMMY;
    }

    return RtMidi::UNSPECIFIED;
}

// -------------------------------------------------------------------------------------------------------------------
// RtAudio Engine

class CarlaEngineRtAudio : public CarlaEngine
{
public:
    CarlaEngineRtAudio(const RtAudio::Api api)
        : CarlaEngine(),
          fAudio(api),
          fAudioBufIn(nullptr),
          fAudioBufOut(nullptr),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          fAudioBufRackIn{nullptr},
          fAudioBufRackOut{nullptr},
#endif
          fAudioCountIn(0),
          fAudioCountOut(0),
          fAudioIsInterleaved(false),
          fAudioIsReady(false),
          fDummyMidiIn(getMatchedAudioMidiAPi(api), "Carla"),
          fDummyMidiOut(getMatchedAudioMidiAPi(api), "Carla"),
          fLastConnectionId(0)
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

#ifndef CARLA_PROPER_CPP11_SUPPORT
        fAudioBufRackIn[0] = fAudioBufRackIn[1] = nullptr;
        fAudioBufRackOut[0] = fAudioBufRackOut[1] = nullptr;
#endif

        // just to make sure
        fOptions.forceStereo = true;
        fOptions.processMode = PROCESS_MODE_CONTINUOUS_RACK;
    }

    ~CarlaEngineRtAudio() override
    {
        CARLA_ASSERT(fAudioBufIn == nullptr);
        CARLA_ASSERT(fAudioBufOut == nullptr);
        CARLA_ASSERT(fAudioCountIn == 0);
        CARLA_ASSERT(fAudioCountOut == 0);
        CARLA_ASSERT(! fAudioIsReady);
        carla_debug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");

        fUsedMidiIns.clear();
        fUsedMidiOuts.clear();
        fUsedConnections.clear();

        if (gRetNames != nullptr)
        {
            delete[] gRetNames;
            gRetNames = nullptr;
        }
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_ASSERT(fAudioBufIn == nullptr);
        CARLA_ASSERT(fAudioBufOut == nullptr);
        CARLA_ASSERT(fAudioCountIn == 0);
        CARLA_ASSERT(fAudioCountOut == 0);
        CARLA_ASSERT(! fAudioIsReady);
        CARLA_ASSERT(clientName != nullptr);
        carla_debug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        RtAudio::StreamParameters iParams, oParams;
        bool deviceSet = false;

        const unsigned int devCount(fAudio.getDeviceCount());

        if (devCount == 0)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        if (fOptions.rtaudioDevice.isNotEmpty())
        {
            for (unsigned int i=0; i < devCount; ++i)
            {
                RtAudio::DeviceInfo devInfo(fAudio.getDeviceInfo(i));

                if (devInfo.probed && devInfo.outputChannels > 0 && devInfo.name == (const char*)fOptions.rtaudioDevice)
                {
                    deviceSet    = true;
                    fConnectName = devInfo.name.c_str();
                    iParams.deviceId  = i;
                    oParams.deviceId  = i;
                    iParams.nChannels = devInfo.inputChannels;
                    oParams.nChannels = devInfo.outputChannels;
                    break;
                }
            }
        }

        if (! deviceSet)
        {
            iParams.deviceId  = fAudio.getDefaultInputDevice();
            oParams.deviceId  = fAudio.getDefaultOutputDevice();
            iParams.nChannels = 2;
            oParams.nChannels = 2;
        }

        RtAudio::StreamOptions rtOptions;
        rtOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME;
        rtOptions.streamName = clientName;
        rtOptions.priority = 85;

        if (fAudio.getCurrentApi() != RtAudio::LINUX_PULSE)
        {
            rtOptions.flags |= RTAUDIO_NONINTERLEAVED;
            fAudioIsInterleaved = false;

            if (fAudio.getCurrentApi() == RtAudio::LINUX_ALSA && ! deviceSet)
                rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
        }
        else
            fAudioIsInterleaved = true;

        fBufferSize = fOptions.rtaudioBufferSize;

        try {
            fAudio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, fOptions.rtaudioSampleRate, &fBufferSize, carla_rtaudio_process_callback, this, &rtOptions);
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        try {
            fAudio.startStream();
        }
        catch (RtError& e)
        {
            setLastError(e.what());
            fAudio.closeStream();
            return false;
        }

        fAudioCountIn  = iParams.nChannels;
        fAudioCountOut = oParams.nChannels;
        fSampleRate    = fAudio.getStreamSampleRate();

        CARLA_ASSERT(fAudioCountOut > 0);

        if (fAudioCountIn > 0)
        {
            fAudioBufIn  = new float*[fAudioCountIn];

            for (uint i=0; i < fAudioCountIn; ++i)
                fAudioBufIn[i] = new float[fBufferSize];
        }

        if (fAudioCountOut > 0)
        {
            fAudioBufOut = new float*[fAudioCountOut];

            for (uint i=0; i < fAudioCountOut; ++i)
                fAudioBufOut[i] = new float[fBufferSize];
        }

        fAudioBufRackIn[0]  = new float[fBufferSize];
        fAudioBufRackIn[1]  = new float[fBufferSize];
        fAudioBufRackOut[0] = new float[fBufferSize];
        fAudioBufRackOut[1] = new float[fBufferSize];

        fAudioIsReady = true;

        CarlaEngine::init(clientName);
        patchbayRefresh();

        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineRtAudio::close()");
        CARLA_ASSERT(fAudioBufOut != nullptr);
        CARLA_ASSERT(fAudioCountOut > 0);
        CARLA_ASSERT(fAudioIsReady);

        fAudioIsReady = false;

        bool hasError = !CarlaEngine::close();

        if (fAudio.isStreamRunning())
        {
            try {
                fAudio.stopStream();
            }
            catch (RtError& e)
            {
                if (! hasError)
                {
                    setLastError(e.what());
                    hasError = true;
                }
            }
        }

        if (fAudio.isStreamOpen())
        {
            try {
                fAudio.closeStream();
            }
            catch (RtError& e)
            {
                if (! hasError)
                {
                    setLastError(e.what());
                    hasError = true;
                }
            }
        }

        if (fAudioBufIn != nullptr)
        {
            CARLA_ASSERT(fAudioCountIn > 0);

            for (uint i=0; i < fAudioCountIn; ++i)
                delete[] fAudioBufIn[i];

            delete[] fAudioBufIn;
            fAudioBufIn = nullptr;
        }

        if (fAudioBufOut != nullptr)
        {
            CARLA_ASSERT(fAudioCountOut > 0);

            for (uint i=0; i < fAudioCountOut; ++i)
                delete[] fAudioBufOut[i];

            delete[] fAudioBufOut;
            fAudioBufOut = nullptr;
        }

        delete[] fAudioBufRackIn[0];
        delete[] fAudioBufRackIn[1];
        delete[] fAudioBufRackOut[0];
        delete[] fAudioBufRackOut[1];

        fAudioCountIn  = 0;
        fAudioCountOut = 0;

        fConnectedAudioIns[0].clear();
        fConnectedAudioIns[1].clear();
        fConnectedAudioOuts[0].clear();
        fConnectedAudioOuts[1].clear();

        fConnectName.clear();

        for (NonRtList<MidiPort>::Itenerator it = fMidiIns.begin(); it.valid(); it.next())
        {
            MidiPort& port(*it);
            RtMidiIn* const midiInPort((RtMidiIn*)port.rtmidi);

            midiInPort->cancelCallback();
            delete midiInPort;
        }

        for (NonRtList<MidiPort>::Itenerator it = fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiPort& port(*it);
            RtMidiOut* const midiOutPort((RtMidiOut*)port.rtmidi);

            delete midiOutPort;
        }

        fMidiIns.clear();
        fMidiOuts.clear();

        fMidiInEvents.clear();
        //fMidiOutEvents.clear();

        return (! hasError);
    }

    bool isRunning() const override
    {
        return fAudio.isStreamRunning();
    }

    bool isOffline() const override
    {
        return false;
    }

    EngineType type() const override
    {
        return kEngineTypeRtAudio;
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayConnect(int portA, int portB) override
    {
        CARLA_ASSERT(fAudioIsReady);
        CARLA_ASSERT(portA > PATCHBAY_PORT_MAX);
        CARLA_ASSERT(portB > PATCHBAY_PORT_MAX);
        carla_debug("CarlaEngineRtAudio::patchbayConnect(%i, %i)", portA, portB);

        if (! fAudioIsReady)
        {
            setLastError("Engine not ready");
            return false;
        }
        if (portA < PATCHBAY_PORT_MAX)
        {
            setLastError("Invalid output port");
            return false;
        }
        if (portB < PATCHBAY_PORT_MAX)
        {
            setLastError("Invalid input port");
            return false;
        }

        // only allow connections between Carla and other ports
        if (portA < 0 && portB < 0)
        {
            setLastError("Invalid connection (1)");
            return false;
        }
        if (portA >= 0 && portB >= 0)
        {
            setLastError("Invalid connection (2)");
            return false;
        }

        const int carlaPort  = (portA < 0) ? portA : portB;
        const int targetPort = (carlaPort == portA) ? portB : portA;
        bool makeConnection  = false;

        switch (carlaPort)
        {
        case PATCHBAY_PORT_AUDIO_IN1:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_AUDIO_IN*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_AUDIO_IN*1000+999);
            fConnectAudioLock.lock();
            fConnectedAudioIns[0].append(targetPort - PATCHBAY_GROUP_AUDIO_IN*1000);
            fConnectAudioLock.unlock();
            makeConnection = true;
            break;

        case PATCHBAY_PORT_AUDIO_IN2:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_AUDIO_IN*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_AUDIO_IN*1000+999);
            fConnectAudioLock.lock();
            fConnectedAudioIns[1].append(targetPort - PATCHBAY_GROUP_AUDIO_IN*1000);
            fConnectAudioLock.unlock();
            makeConnection = true;
            break;

        case PATCHBAY_PORT_AUDIO_OUT1:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_AUDIO_OUT*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_AUDIO_OUT*1000+999);
            fConnectAudioLock.lock();
            fConnectedAudioOuts[0].append(targetPort - PATCHBAY_GROUP_AUDIO_OUT*1000);
            fConnectAudioLock.unlock();
            makeConnection = true;
            break;

        case PATCHBAY_PORT_AUDIO_OUT2:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_AUDIO_OUT*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_AUDIO_OUT*1000+999);
            fConnectAudioLock.lock();
            fConnectedAudioOuts[1].append(targetPort - PATCHBAY_GROUP_AUDIO_OUT*1000);
            fConnectAudioLock.unlock();
            makeConnection = true;
            break;

        case PATCHBAY_PORT_MIDI_IN:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_MIDI_IN*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_MIDI_IN*1000+999);
            makeConnection = connectMidiInPort(targetPort - PATCHBAY_GROUP_MIDI_IN*1000);
            break;

        case PATCHBAY_PORT_MIDI_OUT:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_MIDI_OUT*1000+999);
            makeConnection = connectMidiOutPort(targetPort - PATCHBAY_GROUP_MIDI_OUT*1000);
            break;
        }

        if (! makeConnection)
        {
            setLastError("Invalid connection (3)");
            return false;
        }

        ConnectionToId connectionToId;
        connectionToId.id      = fLastConnectionId;
        connectionToId.portOut = portA;
        connectionToId.portIn  = portB;

        callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, portA, portB, nullptr);

        fUsedConnections.append(connectionToId);
        fLastConnectionId++;

        return true;
    }

    bool patchbayDisconnect(int connectionId) override
    {
        CARLA_ASSERT(fAudioIsReady);
        CARLA_ASSERT(fUsedConnections.count() > 0);
        carla_debug("CarlaEngineRtAudio::patchbayDisconnect(%i)", connectionId);

        if (! fAudioIsReady)
        {
            setLastError("Engine not ready");
            return false;
        }
        if (fUsedConnections.count() == 0)
        {
            setLastError("No connections available");
            return false;
        }

        for (NonRtList<ConnectionToId>::Itenerator it=fUsedConnections.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connection(*it);

            if (connection.id == connectionId)
            {
                const int targetPort((connection.portOut >= 0) ? connection.portOut : connection.portIn);
                const int carlaPort((targetPort == connection.portOut) ? connection.portIn : connection.portOut);

                if (targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000)
                {
                    const int portId(targetPort-PATCHBAY_GROUP_MIDI_OUT*1000);

                    for (NonRtList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
                    {
                        MidiPort& midiPort(*it);

                        if (midiPort.portId == portId)
                        {
                            RtMidiOut* const midiOutPort((RtMidiOut*)midiPort.rtmidi);

                            delete midiOutPort;

                            fMidiOuts.remove(it);
                            break;
                        }
                    }
                }
                else if (targetPort >= PATCHBAY_GROUP_MIDI_IN*1000)
                {
                    const int portId(targetPort-PATCHBAY_GROUP_MIDI_IN*1000);

                    for (NonRtList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
                    {
                        MidiPort& midiPort(*it);

                        if (midiPort.portId == portId)
                        {
                            RtMidiIn* const midiInPort((RtMidiIn*)midiPort.rtmidi);

                            midiInPort->cancelCallback();
                            delete midiInPort;

                            fMidiIns.remove(it);
                            break;
                        }
                    }
                }
                else if (targetPort >= PATCHBAY_GROUP_AUDIO_OUT*1000)
                {
                    CARLA_ASSERT(carlaPort == PATCHBAY_PORT_AUDIO_OUT1 || carlaPort == PATCHBAY_PORT_AUDIO_OUT2);

                    const int portId(targetPort-PATCHBAY_GROUP_AUDIO_OUT*1000);

                    fConnectAudioLock.lock();

                    if (carlaPort == PATCHBAY_PORT_AUDIO_OUT1)
                        fConnectedAudioOuts[0].removeAll(portId);
                    else
                        fConnectedAudioOuts[1].removeAll(portId);

                    fConnectAudioLock.unlock();
                }
                else if (targetPort >= PATCHBAY_GROUP_AUDIO_IN*1000)
                {
                    CARLA_ASSERT(carlaPort == PATCHBAY_PORT_AUDIO_IN1 || carlaPort == PATCHBAY_PORT_AUDIO_IN2);

                    const int portId(targetPort-PATCHBAY_GROUP_AUDIO_IN*1000);

                    fConnectAudioLock.lock();

                    if (carlaPort == PATCHBAY_PORT_AUDIO_IN1)
                        fConnectedAudioIns[0].removeAll(portId);
                    else
                        fConnectedAudioIns[1].removeAll(portId);

                    fConnectAudioLock.unlock();
                }
                else
                {
                    CARLA_ASSERT(false);
                }

                callback(CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, connection.id, 0, 0.0f, nullptr);

                fUsedConnections.remove(it);
                break;
            }
        }

        return true;
    }

    void patchbayRefresh() override
    {
        CARLA_ASSERT(fAudioIsReady);

        if (! fAudioIsReady)
            return;

        char strBuf[STR_MAX+1];

        fLastConnectionId = 0;
        fUsedMidiIns.clear();
        fUsedMidiOuts.clear();
        fUsedConnections.clear();

        // Main
        {
            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_CARLA, 0, 0.0f, getName());

            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN1,  PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT,  "audio-in1");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN2,  PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT,  "audio-in2");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT1, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, "audio-out1");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT2, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, "audio-out2");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_IN,    PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_INPUT,   "midi-in");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_OUT,   PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_OUTPUT,  "midi-out");
        }

        // Audio In
        {
            if (fConnectName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", (const char*)fConnectName);
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_AUDIO_IN, 0, 0.0f, strBuf);

            for (unsigned int i=0; i < fAudioCountIn; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);
                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_AUDIO_IN, PATCHBAY_GROUP_AUDIO_IN*1000 + i, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, strBuf);
            }
        }

        // Audio Out
        {
            if (fConnectName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", (const char*)fConnectName);
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_AUDIO_OUT, 0, 0.0f, strBuf);

            for (unsigned int i=0; i < fAudioCountOut; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);
                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_AUDIO_OUT, PATCHBAY_GROUP_AUDIO_OUT*1000 + i, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT, strBuf);
            }
        }

        // MIDI In
        {
            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_IN, 0, 0.0f, "Readable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiIn.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = PATCHBAY_GROUP_MIDI_IN*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiIn.getPortName(i).c_str(), STR_MAX);
                fUsedMidiIns.append(portNameToId);

                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_MIDI_IN, portNameToId.portId, PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_OUTPUT, portNameToId.name);
            }
        }

#if 0 // midi-out not implemented yet
        // MIDI Out
        {
            callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, 0, 0.0f, "Writable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiOut.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = PATCHBAY_GROUP_MIDI_OUT*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiOut.getPortName(i).c_str(), STR_MAX);
                fUsedMidiOuts.append(portNameToId);

                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, portNameToId.portId, PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_INPUT, portNameToId.name);
            }
        }
#endif

        // Connections
        fConnectAudioLock.lock();

        for (NonRtList<uint>::Itenerator it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
        {
            const uint& port(*it);
            CARLA_ASSERT(port < fAudioCountIn);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = PATCHBAY_PORT_AUDIO_IN1;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (NonRtList<uint>::Itenerator it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
        {
            const uint& port(*it);
            CARLA_ASSERT(port < fAudioCountIn);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = PATCHBAY_PORT_AUDIO_IN2;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (NonRtList<uint>::Itenerator it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
        {
            const uint& port(*it);
            CARLA_ASSERT(port < fAudioCountOut);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_AUDIO_OUT1;
            connectionToId.portIn  = PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (NonRtList<uint>::Itenerator it = fConnectedAudioOuts[1].begin(); it.valid(); it.next())
        {
            const uint& port(*it);
            CARLA_ASSERT(port < fAudioCountOut);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_AUDIO_OUT2;
            connectionToId.portIn  = PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        fConnectAudioLock.unlock();

        for (NonRtList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(*it);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_MIDI_IN*1000 + midiPort.portId;
            connectionToId.portIn  = PATCHBAY_PORT_MIDI_IN;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (NonRtList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(*it);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_MIDI_OUT;
            connectionToId.portIn  = PATCHBAY_GROUP_MIDI_OUT*1000 + midiPort.portId;

            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }
    }

    // -------------------------------------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        // get buffers from RtAudio
        float* insPtr  = (float*)inputBuffer;
        float* outsPtr = (float*)outputBuffer;

        // assert buffers
        CARLA_ASSERT(nframes != 0);
        CARLA_ASSERT_INT2(nframes == fBufferSize, nframes, fBufferSize);
        CARLA_ASSERT(outsPtr != nullptr);

        if (kData->curPluginCount == 0 || fAudioCountOut == 0 || ! fAudioIsReady)
        {
            if (fAudioCountOut > 0 && fAudioIsReady)
                carla_zeroFloat(outsPtr, nframes*fAudioCountOut);

            return proccessPendingEvents();
        }

        // initialize audio input
        if (fAudioIsInterleaved)
        {
            for (unsigned int i=0, j=0; i < nframes*fAudioCountIn; ++i)
            {
                fAudioBufIn[i/fAudioCountIn][j] = insPtr[i];

                if ((i+1) % fAudioCountIn == 0)
                    j += 1;
            }
        }
        else
        {
            for (unsigned int i=0; i < fAudioCountIn; ++i)
                carla_copyFloat(fAudioBufIn[i], insPtr+(nframes*i), nframes);
        }

        // initialize audio output
        for (unsigned int i=0; i < fAudioCountOut; ++i)
            carla_zeroFloat(fAudioBufOut[i], nframes);

        carla_zeroFloat(fAudioBufRackOut[0], nframes);
        carla_zeroFloat(fAudioBufRackOut[1], nframes);

        // initialize input events
        carla_zeroMem(kData->bufEvents.in, sizeof(EngineEvent)*INTERNAL_EVENT_COUNT);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            while (! fMidiInEvents.data.isEmpty())
            {
                const RtMidiEvent& midiEvent(fMidiInEvents.data.getFirst(true));

                EngineEvent& engineEvent(kData->bufEvents.in[engineEventIndex++]);
                engineEvent.clear();

                const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent.data);

                engineEvent.channel = midiChannel;

                if (midiEvent.time < fTimeInfo.frame)
                {
                    engineEvent.time = 0;
                }
                else if (midiEvent.time >= fTimeInfo.frame + nframes)
                {
                    engineEvent.time = fTimeInfo.frame + nframes-1;
                    carla_stderr("MIDI Event in the future!, %i vs %i", engineEvent.time, fTimeInfo.frame);
                }
                else
                    engineEvent.time = midiEvent.time - fTimeInfo.frame;

                if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                {
                    const uint8_t midiControl = midiEvent.data[1];
                    engineEvent.type          = kEngineEventTypeControl;

                    if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                    {
                        const uint8_t midiBank  = midiEvent.data[2];

                        engineEvent.ctrl.type  = kEngineControlEventTypeMidiBank;
                        engineEvent.ctrl.param = midiBank;
                        engineEvent.ctrl.value = 0.0f;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                    {
                        engineEvent.ctrl.type  = kEngineControlEventTypeAllSoundOff;
                        engineEvent.ctrl.param = 0;
                        engineEvent.ctrl.value = 0.0f;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                    {
                        engineEvent.ctrl.type  = kEngineControlEventTypeAllNotesOff;
                        engineEvent.ctrl.param = 0;
                        engineEvent.ctrl.value = 0.0f;
                    }
                    else
                    {
                        const uint8_t midiValue = midiEvent.data[2];

                        engineEvent.ctrl.type  = kEngineControlEventTypeParameter;
                        engineEvent.ctrl.param = midiControl;
                        engineEvent.ctrl.value = float(midiValue)/127.0f;
                    }
                }
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                {
                    const uint8_t midiProgram = midiEvent.data[1];
                    engineEvent.type          = kEngineEventTypeControl;

                    engineEvent.ctrl.type  = kEngineControlEventTypeMidiProgram;
                    engineEvent.ctrl.param = midiProgram;
                    engineEvent.ctrl.value = 0.0f;
                }
                else
                {
                    engineEvent.type = kEngineEventTypeMidi;

                    engineEvent.midi.data[0] = midiStatus;
                    engineEvent.midi.data[1] = midiEvent.data[1];
                    engineEvent.midi.data[2] = midiEvent.data[2];
                    engineEvent.midi.data[3] = midiEvent.data[3];
                    engineEvent.midi.size    = midiEvent.size;
                }

                if (engineEventIndex >= INTERNAL_EVENT_COUNT)
                    break;
            }

            fMidiInEvents.mutex.unlock();
        }

        fConnectAudioLock.lock();

        // connect input buffers
        if (fConnectedAudioIns[0].count() == 0)
        {
            carla_zeroFloat(fAudioBufRackIn[0], nframes);
        }
        else
        {
            bool first = true;

            for (NonRtList<uint>::Itenerator it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
            {
                const uint& port(*it);
                CARLA_ASSERT(port < fAudioCountIn);

                if (first)
                {
                    carla_copyFloat(fAudioBufRackIn[0], fAudioBufIn[port], nframes);
                    first = false;
                }
                else
                    carla_addFloat(fAudioBufRackIn[0], fAudioBufIn[port], nframes);
            }

            if (first)
                carla_zeroFloat(fAudioBufRackIn[0], nframes);
        }

        if (fConnectedAudioIns[1].count() == 0)
        {
            carla_zeroFloat(fAudioBufRackIn[1], nframes);
        }
        else
        {
            bool first = true;

            for (NonRtList<uint>::Itenerator it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
            {
                const uint& port(*it);
                CARLA_ASSERT(port < fAudioCountIn);

                if (first)
                {
                    carla_copyFloat(fAudioBufRackIn[1], fAudioBufIn[port], nframes);
                    first = false;
                }
                else
                    carla_addFloat(fAudioBufRackIn[1], fAudioBufIn[port], nframes);
            }

            if (first)
                carla_zeroFloat(fAudioBufRackIn[1], nframes);
        }

        // process
        processRack(fAudioBufRackIn, fAudioBufRackOut, nframes);

        // connect output buffers
        if (fConnectedAudioOuts[0].count() != 0)
        {
            for (NonRtList<uint>::Itenerator it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
            {
                const uint& port(*it);
                CARLA_ASSERT(port < fAudioCountOut);

                carla_addFloat(fAudioBufOut[port], fAudioBufRackOut[0], nframes);
            }
        }

        if (fConnectedAudioOuts[1].count() != 0)
        {
            for (NonRtList<uint>::Itenerator it = fConnectedAudioOuts[1].begin(); it.valid(); it.next())
            {
                const uint& port(*it);
                CARLA_ASSERT(port < fAudioCountOut);

                carla_addFloat(fAudioBufOut[port], fAudioBufRackOut[1], nframes);
            }
        }

        fConnectAudioLock.unlock();

        // output audio
        if (fAudioIsInterleaved)
        {
            for (unsigned int i=0, j=0; i < nframes*fAudioCountOut; ++i)
            {
                outsPtr[i] = fAudioBufOut[i/fAudioCountOut][j];

                if ((i+1) % fAudioCountOut == 0)
                    j += 1;
            }
        }
        else
        {
            for (unsigned int i=0; i < fAudioCountOut; ++i)
                carla_copyFloat(outsPtr+(nframes*i), fAudioBufOut[i], nframes);
        }

        // output events
        {
            // TODO
            //fMidiOutEvents...
        }

        proccessPendingEvents();
        return;

        // unused
        (void)streamTime;
        (void)status;
    }

    void handleMidiCallback(double timeStamp, std::vector<unsigned char>* const message)
    {
        if (! fAudioIsReady)
            return;

        const size_t messageSize = message->size();
        static uint32_t lastTime = 0;

        if (messageSize == 0 || messageSize > 4)
            return;

        timeStamp /= 2;

        if (timeStamp > 0.95)
            timeStamp = 0.95;
        else if (timeStamp < 0.0)
            timeStamp = 0.0;

        RtMidiEvent midiEvent;
        midiEvent.time = fTimeInfo.frame + (timeStamp*(double)fBufferSize);

        if (midiEvent.time < lastTime)
            midiEvent.time = lastTime;
        else
            lastTime = midiEvent.time;

        if (messageSize == 1)
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = 0;
            midiEvent.data[2] = 0;
            midiEvent.data[3] = 0;
            midiEvent.size    = 1;
        }
        else if (messageSize == 2)
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = message->at(1);
            midiEvent.data[2] = 0;
            midiEvent.data[3] = 0;
            midiEvent.size    = 2;
        }
        else if (messageSize == 3)
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = message->at(1);
            midiEvent.data[2] = message->at(2);
            midiEvent.data[3] = 0;
            midiEvent.size    = 3;
        }
        else
        {
            midiEvent.data[0] = message->at(0);
            midiEvent.data[1] = message->at(1);
            midiEvent.data[2] = message->at(2);
            midiEvent.data[3] = message->at(3);
            midiEvent.size    = 4;
        }

        fMidiInEvents.append(midiEvent);
    }

    bool connectMidiInPort(const int portId)
    {
        CARLA_ASSERT(fUsedMidiIns.count() > 0);
        CARLA_ASSERT(portId >= 0);
        CARLA_ASSERT(static_cast<size_t>(portId) < fUsedMidiIns.count());
        carla_debug("CarlaEngineRtAudio::connectMidiInPort(%i)", portId);

        if (portId < 0 || static_cast<size_t>(portId) >= fUsedMidiIns.count())
            return false;

        const char* const portName(fUsedMidiIns.getAt(portId).name);

        char newPortName[STR_MAX+1];
        std::snprintf(newPortName, STR_MAX, "%s:in-%i", (const char*)getName(), portId+1);

        int rtMidiPortIndex = -1;

        RtMidiIn* const rtMidiIn(new RtMidiIn(getMatchedAudioMidiAPi(fAudio.getCurrentApi()), newPortName, 512));
        rtMidiIn->ignoreTypes();
        rtMidiIn->setCallback(carla_rtmidi_callback, this);

        for (unsigned int i=0, count=rtMidiIn->getPortCount(); i < count; ++i)
        {
            if (rtMidiIn->getPortName(i) == portName)
            {
                rtMidiPortIndex = i;
                break;
            }
        }

        if (rtMidiPortIndex == -1)
        {
            delete rtMidiIn;
            return false;
        }

        rtMidiIn->openPort(rtMidiPortIndex, newPortName+(std::strlen(getName())+1));

        MidiPort midiPort;
        midiPort.portId = portId;
        midiPort.rtmidi = rtMidiIn;

        fMidiIns.append(midiPort);

        return true;
    }

    bool connectMidiOutPort(const int portId)
    {
        CARLA_ASSERT(fUsedMidiOuts.count() > 0);
        CARLA_ASSERT(portId >= 0);
        CARLA_ASSERT(static_cast<size_t>(portId) < fUsedMidiOuts.count());
        carla_debug("CarlaEngineRtAudio::connectMidiOutPort(%i)", portId);

        if (portId < 0 || static_cast<size_t>(portId) >= fUsedMidiOuts.count())
            return false;

        const char* const portName(fUsedMidiOuts.getAt(portId).name);

        char newPortName[STR_MAX+1];
        std::snprintf(newPortName, STR_MAX, "%s:out-%i", (const char*)getName(), portId+1);

        int rtMidiPortIndex = -1;

        RtMidiOut* const rtMidiOut(new RtMidiOut(getMatchedAudioMidiAPi(fAudio.getCurrentApi()), newPortName));

        for (unsigned int i=0, count=rtMidiOut->getPortCount(); i < count; ++i)
        {
            if (rtMidiOut->getPortName(i) == portName)
            {
                rtMidiPortIndex = i;
                break;
            }
        }

        if (rtMidiPortIndex == -1)
        {
            delete rtMidiOut;
            return false;
        }

        rtMidiOut->openPort(rtMidiPortIndex, newPortName+(std::strlen(getName())+1));

        MidiPort midiPort;
        midiPort.portId = portId;
        midiPort.rtmidi = rtMidiOut;

        fMidiOuts.append(midiPort);

        return true;
    }

    // -------------------------------------

private:
    RtAudio fAudio;
    float** fAudioBufIn;
    float** fAudioBufOut;
    float*  fAudioBufRackIn[2];
    float*  fAudioBufRackOut[2];

    uint fAudioCountIn;
    uint fAudioCountOut;
    bool fAudioIsInterleaved;
    bool fAudioIsReady;

    NonRtList<uint> fConnectedAudioIns[2];
    NonRtList<uint> fConnectedAudioOuts[2];
    CarlaMutex      fConnectAudioLock;
    CarlaString     fConnectName;

    RtMidiIn  fDummyMidiIn;
    RtMidiOut fDummyMidiOut;

    enum PatchbayGroupIds {
        PATCHBAY_GROUP_CARLA     = -1,
        PATCHBAY_GROUP_AUDIO_IN  = 0,
        PATCHBAY_GROUP_AUDIO_OUT = 1,
        PATCHBAY_GROUP_MIDI_IN   = 2,
        PATCHBAY_GROUP_MIDI_OUT  = 3,
        PATCHBAY_GROUP_MAX       = 4
    };

    enum PatchbayPortIds {
        PATCHBAY_PORT_AUDIO_IN1  = -1,
        PATCHBAY_PORT_AUDIO_IN2  = -2,
        PATCHBAY_PORT_AUDIO_OUT1 = -3,
        PATCHBAY_PORT_AUDIO_OUT2 = -4,
        PATCHBAY_PORT_MIDI_IN    = -5,
        PATCHBAY_PORT_MIDI_OUT   = -6,
        PATCHBAY_PORT_MAX        = -7
    };

    struct ConnectionToId {
        int id;
        int portOut;
        int portIn;
    };

    struct PortNameToId {
        int portId;
        char name[STR_MAX+1];
    };

    int fLastConnectionId;
    NonRtList<PortNameToId> fUsedMidiIns;
    NonRtList<PortNameToId> fUsedMidiOuts;
    NonRtList<ConnectionToId> fUsedConnections;

    struct MidiPort {
        RtMidi* rtmidi;
        int portId;
    };

    NonRtList<MidiPort> fMidiIns;
    NonRtList<MidiPort> fMidiOuts;

    struct RtMidiEvent {
        uint32_t time;
        unsigned char data[4];
        unsigned char size;
    };

    struct RtMidiEvents {
        CarlaMutex mutex;
        RtList<RtMidiEvent>::Pool dataPool;
        RtList<RtMidiEvent> data;
        RtList<RtMidiEvent> dataPending;

        RtMidiEvents()
            : dataPool(512, 512),
              data(dataPool),
              dataPending(dataPool) {}

        ~RtMidiEvents()
        {
            clear();
        }

        void append(const RtMidiEvent& event)
        {
            mutex.lock();
            dataPending.append(event);
            mutex.unlock();
        }

        void clear()
        {
            mutex.lock();
            data.clear();
            dataPending.clear();
            mutex.unlock();
        }

        void splice()
        {
            dataPending.spliceAppend(data, true);
        }
    };

    RtMidiEvents fMidiInEvents;
    //RtMidiEvents fMidiOutEvents;

    #define handlePtr ((CarlaEngineRtAudio*)userData)

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        handlePtr->handleAudioProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }

    static void carla_rtmidi_callback(double timeStamp, std::vector<unsigned char>* message, void* userData)
    {
        handlePtr->handleMidiCallback(timeStamp, message);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudio)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newRtAudio(RtAudioApi api)
{
    RtAudio::Api rtApi(RtAudio::UNSPECIFIED);

    switch (api)
    {
    case RTAUDIO_DUMMY:
        rtApi = RtAudio::RTAUDIO_DUMMY;
        break;
    case RTAUDIO_LINUX_ALSA:
        rtApi = RtAudio::LINUX_ALSA;
        break;
    case RTAUDIO_LINUX_PULSE:
        rtApi = RtAudio::LINUX_PULSE;
        break;
    case RTAUDIO_LINUX_OSS:
        rtApi = RtAudio::LINUX_OSS;
        break;
    case RTAUDIO_UNIX_JACK:
        rtApi = RtAudio::UNIX_JACK;
        break;
    case RTAUDIO_MACOSX_CORE:
        rtApi = RtAudio::MACOSX_CORE;
        break;
    case RTAUDIO_WINDOWS_ASIO:
        rtApi = RtAudio::WINDOWS_ASIO;
        break;
    case RTAUDIO_WINDOWS_DS:
        rtApi = RtAudio::WINDOWS_DS;
        break;
    }

    return new CarlaEngineRtAudio(rtApi);
}

size_t CarlaEngine::getRtAudioApiCount()
{
    initRtApis();

    return gRtAudioApis.size();
}

const char* CarlaEngine::getRtAudioApiName(const unsigned int index)
{
    initRtApis();

    if (index < gRtAudioApis.size())
    {
        const RtAudio::Api& api(gRtAudioApis[index]);

        switch (api)
        {
        case RtAudio::UNSPECIFIED:
            return "Unspecified";
        case RtAudio::LINUX_ALSA:
            return "ALSA";
        case RtAudio::LINUX_PULSE:
            return "PulseAudio";
        case RtAudio::LINUX_OSS:
            return "OSS";
        case RtAudio::UNIX_JACK:
#if defined(CARLA_OS_WIN)
        return "JACK with WinMM";
#elif defined(CARLA_OS_MAC)
        return "JACK with CoreMidi";
#elif defined(CARLA_OS_LINUX)
        return "JACK with ALSA-MIDI";
#else
        return "JACK (RtAudio)";
#endif
        case RtAudio::MACOSX_CORE:
            return "CoreAudio";
        case RtAudio::WINDOWS_ASIO:
            return "ASIO";
        case RtAudio::WINDOWS_DS:
            return "DirectSound";
        case RtAudio::RTAUDIO_DUMMY:
            return "Dummy";
        }
    }

    return nullptr;
}

const char** CarlaEngine::getRtAudioApiDeviceNames(const unsigned int index)
{
    initRtApis();

    if (index < gRtAudioApis.size())
    {
        const RtAudio::Api& api(gRtAudioApis[index]);

        RtAudio rtAudio(api);

        if (gRetNames != nullptr)
        {
            int i=0;
            while (gRetNames[i] != nullptr)
                delete[] gRetNames[i++];
            delete[] gRetNames;
            gRetNames = nullptr;
        }

        const unsigned int devCount(rtAudio.getDeviceCount());

        if (devCount > 0)
        {
            NonRtList<const char*> devNames;

            for (unsigned int i=0; i < devCount; ++i)
            {
                RtAudio::DeviceInfo devInfo(rtAudio.getDeviceInfo(i));

                if (devInfo.probed && devInfo.outputChannels > 0 /*&& (devInfo.nativeFormats & RTAUDIO_FLOAT32) != 0*/)
                    devNames.append(carla_strdup(devInfo.name.c_str()));
            }

            const unsigned int realDevCount(devNames.count());

            gRetNames = new const char*[realDevCount+1];

            for (unsigned int i=0; i < realDevCount; ++i)
                gRetNames[i] = devNames.getAt(i);

            gRetNames[realDevCount] = nullptr;
            devNames.clear();

            return gRetNames;
        }
    }

    return nullptr;
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_RTAUDIO
