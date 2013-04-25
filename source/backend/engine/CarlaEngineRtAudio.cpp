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
    static bool initiated = false;

    if (! initiated)
    {
        initiated = true;

        RtAudio::getCompiledApi(gRtAudioApis);
    }
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
          fAudioBufRackIn{nullptr},
          fAudioBufRackOut{nullptr},
          fAudioCountIn(0),
          fAudioCountOut(0),
          fAudioIsInterleaved(false),
          fAudioIsReady(false),
          fDummyMidiIn(getMatchedAudioMidiAPi(api), "Carla-Probe-In"),
          fDummyMidiOut(getMatchedAudioMidiAPi(api), "Carla-Probe-Out"),
          fLastConnectionId(0)
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

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

#if 0
        fUsedPortNames.clear();
#endif
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
                    deviceSet = true;
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

        //fMidiIn.cancelCallback();
        //disconnectMidiPort(true, false);
        //disconnectMidiPort(false, false);

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

        //fMidiInEvents.clear();
        //fMidiOutEvents.clear();

        return true;
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
            setLastError("Invalid connection");
            return false;
        }
        if (portA >= 0 && portB >= 0)
        {
            setLastError("Invalid connection");
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
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_MIDI_IN*1000 + 999);
#if 0
            disconnectMidiPort(true, true);

            for (unsigned int i=0, count=fMidiIn.getPortCount(); i < count; ++i)
            {
                const char* const portName(fMidiIn.getPortName(i).c_str());

                if (getPatchbayPortId(portName) == targetPort)
                {
                    fUsedMidiPortIn = portName;
                    fMidiIn.openPort(i, "midi-in");
                    makeConnection = true;
                    break;
                }
            }
#endif
            break;

        case PATCHBAY_PORT_MIDI_OUT:
            CARLA_ASSERT(targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000);
            CARLA_ASSERT(targetPort <= PATCHBAY_GROUP_MIDI_OUT*1000 + 999);
#if 0
            disconnectMidiPort(false, true);

            for (unsigned int i=0, count=fMidiOut.getPortCount(); i < count; ++i)
            {
                const char* const portName(fMidiOut.getPortName(i).c_str());

                if (getPatchbayPortId(portName) == targetPort)
                {
                    fUsedMidiPortOut = portName;
                    fMidiOut.openPort(i, "midi-out");
                    makeConnection = true;
                    break;
                }
            }
#endif
            break;
        }

        if (! makeConnection)
        {
            setLastError("Invalid connection");
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

        for (auto it=fUsedConnections.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connection(*it);

            if (connection.id == connectionId)
            {
                const int targetPort((connection.portOut >= 0) ? connection.portOut : connection.portIn);
                const int carlaPort((targetPort == connection.portOut) ? connection.portIn : connection.portOut);

                if (targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000)
                {
                    //fMidiOut.closePort();
                    //fUsedMidiPortOut.clear();
                }
                else if (targetPort >= PATCHBAY_GROUP_MIDI_IN*1000)
                {
                    //fMidiIn.closePort();
                    //fUsedMidiPortIn.clear();
                }
                else if (targetPort >= PATCHBAY_GROUP_AUDIO_OUT*1000)
                {
                    CARLA_ASSERT(carlaPort == PATCHBAY_PORT_AUDIO_OUT1 || carlaPort == PATCHBAY_PORT_AUDIO_OUT2);

                    fConnectAudioLock.lock();

                    if (carlaPort == PATCHBAY_PORT_AUDIO_OUT1)
                        fConnectedAudioOuts[0].removeAll(targetPort-PATCHBAY_GROUP_AUDIO_OUT*1000);
                    else
                        fConnectedAudioOuts[1].removeAll(targetPort-PATCHBAY_GROUP_AUDIO_OUT*1000);

                    fConnectAudioLock.unlock();
                }
                else if (targetPort >= PATCHBAY_GROUP_AUDIO_IN*1000)
                {
                    CARLA_ASSERT(carlaPort == PATCHBAY_PORT_AUDIO_IN1 || carlaPort == PATCHBAY_PORT_AUDIO_IN2);

                    fConnectAudioLock.lock();

                    if (carlaPort == PATCHBAY_PORT_AUDIO_IN1)
                        fConnectedAudioIns[0].removeAll(targetPort-PATCHBAY_GROUP_AUDIO_IN*1000);
                    else
                        fConnectedAudioIns[1].removeAll(targetPort-PATCHBAY_GROUP_AUDIO_IN*1000);

                    fConnectAudioLock.unlock();
                }
                else
                {
                    CARLA_ASSERT(false);
                }

                callback(CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, connection.id, 0, 0.0f, nullptr);

                fUsedConnections.remove(it);
            }
        }

        return false;
    }

    void patchbayRefresh() override
    {
        CARLA_ASSERT(fAudioIsReady);

        if (! fAudioIsReady)
            return;

        char strBuf[STR_MAX];

        fLastConnectionId = 0;
        fUsedConnections.clear();

        // Main
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_CARLA, 0, 0.0f, getName());
        {
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN1,  PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT,  "audio-in1");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN2,  PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT,  "audio-in2");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT1, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, "audio-out1");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT2, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, "audio-out2");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_IN,    PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_INPUT,   "midi-in");
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_OUT,   PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_OUTPUT,  "midi-out");
        }

        // Audio In
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_AUDIO_IN, 0, 0.0f, "Capture");

        for (unsigned int i=0; i < fAudioCountIn; ++i)
        {
            std::sprintf(strBuf, "capture_%i", i+1);
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_AUDIO_IN, PATCHBAY_GROUP_AUDIO_IN*1000 + i, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_OUTPUT, strBuf);
        }

        // Audio Out
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_AUDIO_OUT, 0, 0.0f, "Playback");

        for (unsigned int i=0; i < fAudioCountOut; ++i)
        {
            std::sprintf(strBuf, "playback_%i", i+1);
            callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_AUDIO_OUT, PATCHBAY_GROUP_AUDIO_OUT*1000 + i, PATCHBAY_PORT_IS_AUDIO|PATCHBAY_PORT_IS_INPUT, strBuf);
        }

        // MIDI In
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_IN, 0, 0.0f, "Readable MIDI ports");
        {
            const unsigned int portCount(fDummyMidiIn.getPortCount());

            for (unsigned int i=0; i < portCount; ++i)
            {
                //PortNameToId portNameToId;
                //portNameToId.portId = PATCHBAY_GROUP_MIDI_IN*1000 + i;
                //portNameToId.name   = fMidiIn.getPortName(i).c_str();
                //fUsedPortNames.append(portNameToId);

                const char* const portName(fDummyMidiIn.getPortName(i).c_str());//portNameToId.name.toUtf8().constData());
                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_MIDI_IN, PATCHBAY_GROUP_MIDI_IN*1000 + i/*portNameToId.portId*/, PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_OUTPUT, portName);
            }
        }

        // MIDI Out
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, 0, 0.0f, "Writable MIDI ports");

        // Connections
        fConnectAudioLock.lock();

        for (auto it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
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

        for (auto it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
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

        for (auto it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
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

        for (auto it = fConnectedAudioOuts[1].begin(); it.valid(); it.next())
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

#if 0
        fUsedPortNames.clear();

        // MIDI In
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_IN, 0, 0.0f, "Readable MIDI ports");


        // MIDI Out
        callback(CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, 0, 0.0f, "Writable MIDI ports");
        {
            const unsigned int portCount = fMidiOut.getPortCount();

            for (unsigned int i=0; i < portCount; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = PATCHBAY_GROUP_MIDI_OUT*1000 + i;
                portNameToId.name   = fMidiOut.getPortName(i).c_str();
                fUsedPortNames.append(portNameToId);

                const char* const portName(portNameToId.name.toUtf8().constData());
                callback(CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, portNameToId.portId, PATCHBAY_PORT_IS_MIDI|PATCHBAY_PORT_IS_INPUT, portName);
            }
        }

        // Connections
        if (! fUsedMidiPortIn.isEmpty())
        {
            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = getPatchbayPortId(fUsedMidiPortIn);
            connectionToId.portIn  = PATCHBAY_PORT_MIDI_IN;

            fUsedConnections.append(connectionToId);
            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);
            fLastConnectionId++;
        }
        if (! fUsedMidiPortOut.isEmpty())
        {
            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_MIDI_OUT;
            connectionToId.portIn  = getPatchbayPortId(fUsedMidiPortOut);

            fUsedConnections.append(connectionToId);
            callback(CALLBACK_PATCHBAY_CONNECTION_ADDED, 0, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, nullptr);
            fLastConnectionId++;
        }
#endif
    }

    // -------------------------------------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        // get buffers from RtAudio
        float* insPtr  = (float*)inputBuffer;
        float* outsPtr = (float*)outputBuffer;

        // assert buffers
        CARLA_ASSERT(nframes == fBufferSize);
        CARLA_ASSERT(outsPtr != nullptr);

        if (kData->curPluginCount == 0 || ! (fAudioIsReady && fConnectAudioLock.tryLock()))
        {
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
        carla_zeroMem(kData->rack.in, sizeof(EngineEvent)*RACK_EVENT_COUNT);

#if 0
        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            while (! fMidiInEvents.data.isEmpty())
            {
                const RtMidiEvent& midiEvent = fMidiInEvents.data.getFirst(true);

                EngineEvent* const engineEvent = &kData->rack.in[engineEventIndex++];
                engineEvent->clear();

                const uint8_t midiStatus  = MIDI_GET_STATUS_FROM_DATA(midiEvent.data);
                const uint8_t midiChannel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent.data);

                engineEvent->channel = midiChannel;

                if (midiEvent.time < fTimeInfo.frame)
                    engineEvent->time = 0;
                else if (midiEvent.time >= fTimeInfo.frame + nframes)
                {
                    engineEvent->time = fTimeInfo.frame + nframes-1;
                    carla_stderr("MIDI Event in the future!, %i vs %i", engineEvent->time, fTimeInfo.frame);
                }
                else
                    engineEvent->time = midiEvent.time - fTimeInfo.frame;

                //carla_stdout("Got midi, time %f vs %i", midiEvent.time, engineEvent->time);

                if (MIDI_IS_STATUS_CONTROL_CHANGE(midiStatus))
                {
                    const uint8_t midiControl = midiEvent.data[1];
                    engineEvent->type         = kEngineEventTypeControl;

                    if (MIDI_IS_CONTROL_BANK_SELECT(midiControl))
                    {
                        const uint8_t midiBank  = midiEvent.data[2];

                        engineEvent->ctrl.type  = kEngineControlEventTypeMidiBank;
                        engineEvent->ctrl.param = midiBank;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_SOUND_OFF)
                    {
                        engineEvent->ctrl.type  = kEngineControlEventTypeAllSoundOff;
                        engineEvent->ctrl.param = 0;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else if (midiControl == MIDI_CONTROL_ALL_NOTES_OFF)
                    {
                        engineEvent->ctrl.type  = kEngineControlEventTypeAllNotesOff;
                        engineEvent->ctrl.param = 0;
                        engineEvent->ctrl.value = 0.0;
                    }
                    else
                    {
                        const uint8_t midiValue = midiEvent.data[2];

                        engineEvent->ctrl.type  = kEngineControlEventTypeParameter;
                        engineEvent->ctrl.param = midiControl;
                        engineEvent->ctrl.value = double(midiValue)/127.0;
                    }
                }
                else if (MIDI_IS_STATUS_PROGRAM_CHANGE(midiStatus))
                {
                    const uint8_t midiProgram = midiEvent.data[1];
                    engineEvent->type         = kEngineEventTypeControl;

                    engineEvent->ctrl.type  = kEngineControlEventTypeMidiProgram;
                    engineEvent->ctrl.param = midiProgram;
                    engineEvent->ctrl.value = 0.0;
                }
                else
                {
                    engineEvent->type = kEngineEventTypeMidi;

                    engineEvent->midi.data[0] = midiStatus;
                    engineEvent->midi.data[1] = midiEvent.data[1];
                    engineEvent->midi.data[2] = midiEvent.data[2];
                    engineEvent->midi.size    = midiEvent.size;
                }

                if (engineEventIndex >= RACK_EVENT_COUNT)
                    break;
            }

            fMidiInEvents.mutex.unlock();
        }
#endif

        // connect input buffers
        if (fConnectedAudioIns[0].count() == 0)
        {
            carla_zeroFloat(fAudioBufRackIn[0], nframes);
        }
        else
        {
            bool first = true;

            for (auto it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
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

            for (auto it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
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
            for (auto it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
            {
                const uint& port(*it);
                CARLA_ASSERT(port < fAudioCountOut);

                carla_addFloat(fAudioBufOut[port], fAudioBufRackOut[0], nframes);
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
            //fMidiOut.sendMessage();
        }

        proccessPendingEvents();

        return;

        // unused
        (void)streamTime;
        (void)status;
    }

#if 0
    void handleMidiCallback(double timeStamp, std::vector<unsigned char>* const message)
    {
        const size_t messageSize = message->size();
        static uint32_t lastTime = 0;

        if (messageSize == 0 || messageSize > 4)
            return;

        timeStamp /= 2;

        if (timeStamp > 0.95)
            timeStamp = 0.95;

        RtMidiEvent midiEvent;
        midiEvent.time = fTimeInfo.frame + (timeStamp*(double)fBufferSize);
        carla_stdout("Put midi, frame:%09i/%09i, stamp:%g", fTimeInfo.frame, midiEvent.time, timeStamp);

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

    // -------------------------------------

    void disconnectMidiPort(const bool isInput, const bool doPatchbay)
    {
        carla_debug("CarlaEngineRtAudio::disconnectMidiPort(%s, %s)", bool2str(isInput), bool2str(doPatchbay));

        if (isInput)
        {
            if (! fUsedMidiPortIn.isEmpty())
            {
                fUsedMidiPortIn.clear();
                fMidiIn.closePort();
            }
        }
        else
        {
            if (! fUsedMidiPortOut.isEmpty())
            {
                fUsedMidiPortOut.clear();
                fMidiIn.closePort();
            }
        }

        if (! doPatchbay)
            return;

        for (int i=0, count=fUsedConnections.count(); i < count; ++i)
        {
            const ConnectionToId& connection(fUsedConnections.at(i));

            const int targetPort  = (connection.portOut >= 0) ? connection.portOut : connection.portIn;

            if (targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000)
            {
                if (isInput)
                    continue;

                callback(CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, connection.id, 0, 0.0f, nullptr);
                fUsedConnections.takeAt(i);
                break;
            }
            else if (targetPort >= PATCHBAY_GROUP_MIDI_IN*1000)
            {
                if (! isInput)
                    continue;

                callback(CALLBACK_PATCHBAY_CONNECTION_REMOVED, 0, connection.id, 0, 0.0f, nullptr);
                fUsedConnections.takeAt(i);
                break;
            }
        }
    }

    int getPatchbayPortId(const QString& name)
    {
        carla_debug("CarlaEngineRtAudio::getPatchbayPortId(\"%s\")", name.toUtf8().constData());

        for (int i=0, count=fUsedPortNames.count(); i < count; ++i)
        {
            carla_debug("CarlaEngineRtAudio::getPatchbayPortId(\"%s\") VS \"%s\"", name.toUtf8().constData(), fUsedPortNames[i].name.toUtf8().constData());

            if (fUsedPortNames[i].name == name)
                return fUsedPortNames[i].portId;
        }

        return PATCHBAY_PORT_MAX;
    }
#endif

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

    int fLastConnectionId;
    NonRtList<ConnectionToId> fUsedConnections;

#if 0
    NonRtList<RtMidiIn*>  fMidiIns;
    NonRtList<RtMidiOut*> fMidiOuts;

    struct PortNameToId {
        int portId;
        QString name;
    };

    QList<PortNameToId>   fUsedPortNames;
    QString fUsedMidiPortIn;
    QString fUsedMidiPortOut;

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
              data(&dataPool),
              dataPending(&dataPool) {}

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
    RtMidiEvents fMidiOutEvents;
#endif

    #define handlePtr ((CarlaEngineRtAudio*)userData)

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        handlePtr->handleAudioProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }

#if 0
    static void carla_rtmidi_callback(double timeStamp, std::vector<unsigned char>* message, void* userData)
    {
        handlePtr->handleMidiCallback(timeStamp, message);
    }
#endif

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
