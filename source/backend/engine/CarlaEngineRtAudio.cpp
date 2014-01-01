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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMIDI.h"
#include "RtList.hpp"

#include "rtaudio/RtAudio.h"
#include "rtmidi/RtMidi.h"

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
          fAudioCountIn(0),
          fAudioCountOut(0),
          fAudioIsInterleaved(false),
          fAudioIsReady(false),
          fLastTime(0),
          fDummyMidiIn(getMatchedAudioMidiAPi(api), "Carla"),
          fDummyMidiOut(getMatchedAudioMidiAPi(api), "Carla"),
          fLastConnectionId(0)
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

        fAudioBufRackIn[0]  = fAudioBufRackIn[1]  = nullptr;
        fAudioBufRackOut[0] = fAudioBufRackOut[1] = nullptr;

        // just to make sure
        pData->options.forceStereo   = true;
        pData->options.processMode   = ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
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
        CARLA_ASSERT(clientName != nullptr && clientName[0] != '\0');
        carla_debug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        RtAudio::StreamParameters iParams, oParams;
        bool deviceSet = false;

        const unsigned int devCount(fAudio.getDeviceCount());

        if (devCount == 0)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        if (pData->options.audioDevice != nullptr)
        {
            for (unsigned int i=0; i < devCount; ++i)
            {
                RtAudio::DeviceInfo devInfo(fAudio.getDeviceInfo(i));

                if (devInfo.probed && devInfo.outputChannels > 0 && devInfo.name == (const char*)pData->options.audioDevice)
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

        pData->bufferSize = pData->options.audioBufferSize;

        try {
            fAudio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, pData->options.audioSampleRate, &pData->bufferSize, carla_rtaudio_process_callback, this, &rtOptions);
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
        pData->sampleRate = fAudio.getStreamSampleRate();

        CARLA_ASSERT(fAudioCountOut > 0);

        if (fAudioCountIn > 0)
        {
            fAudioBufIn  = new float*[fAudioCountIn];

            for (uint i=0; i < fAudioCountIn; ++i)
                fAudioBufIn[i] = new float[pData->bufferSize];
        }

        if (fAudioCountOut > 0)
        {
            fAudioBufOut = new float*[fAudioCountOut];

            for (uint i=0; i < fAudioCountOut; ++i)
                fAudioBufOut[i] = new float[pData->bufferSize];
        }

        fAudioBufRackIn[0]  = new float[pData->bufferSize];
        fAudioBufRackIn[1]  = new float[pData->bufferSize];
        fAudioBufRackOut[0] = new float[pData->bufferSize];
        fAudioBufRackOut[1] = new float[pData->bufferSize];

        fLastTime     = 0;
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

        for (List<MidiPort>::Itenerator it = fMidiIns.begin(); it.valid(); it.next())
        {
            MidiPort& port(it.getValue());
            RtMidiIn* const midiInPort((RtMidiIn*)port.rtmidi);

            midiInPort->cancelCallback();
            delete midiInPort;
        }

        for (List<MidiPort>::Itenerator it = fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiPort& port(it.getValue());
            RtMidiOut* const midiOutPort((RtMidiOut*)port.rtmidi);

            delete midiOutPort;
        }

        fMidiIns.clear();
        fMidiOuts.clear();

        fMidiInEvents.clear();
        //fMidiOutEvents.clear();

        return (! hasError);
    }

    bool isRunning() const noexcept override
    {
        return fAudio.isStreamRunning();
    }

    bool isOffline() const noexcept override
    {
        return false;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeRtAudio;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        const RtAudio::Api api(fAudio.getCurrentApi());

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

        return nullptr;
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

        callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, portA, portB, 0.0f, nullptr);

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

        for (List<ConnectionToId>::Itenerator it=fUsedConnections.begin(); it.valid(); it.next())
        {
            const ConnectionToId& connection(it.getConstValue());

            if (connection.id == connectionId)
            {
                const int targetPort((connection.portOut >= 0) ? connection.portOut : connection.portIn);
                const int carlaPort((targetPort == connection.portOut) ? connection.portIn : connection.portOut);

                if (targetPort >= PATCHBAY_GROUP_MIDI_OUT*1000)
                {
                    const int portId(targetPort-PATCHBAY_GROUP_MIDI_OUT*1000);

                    for (List<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
                    {
                        MidiPort& midiPort(it.getValue());

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

                    for (List<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
                    {
                        MidiPort& midiPort(it.getValue());

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

                callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED, connection.id, connection.portOut, connection.portIn, 0.0f, nullptr);

                fUsedConnections.remove(it);
                break;
            }
        }

        return true;
    }

    bool patchbayRefresh() override
    {
        CARLA_SAFE_ASSERT_RETURN(fAudioIsReady, false);

        char strBuf[STR_MAX+1];

        fLastConnectionId = 0;
        fUsedMidiIns.clear();
        fUsedMidiOuts.clear();
        fUsedConnections.clear();

        // Main
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, PATCHBAY_GROUP_CARLA, 0, 0, 0.0f, getName());

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_CARLA, PATCHBAY_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
        }

        // Audio In
        {
            if (fConnectName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", (const char*)fConnectName);
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, PATCHBAY_GROUP_AUDIO_IN, 0, 0, 0.0f, strBuf);

            for (unsigned int i=0; i < fAudioCountIn; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_AUDIO_IN, PATCHBAY_GROUP_AUDIO_IN*1000 + i, PATCHBAY_PORT_TYPE_AUDIO, 0.0f, strBuf);
            }
        }

        // Audio Out
        {
            if (fConnectName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", (const char*)fConnectName);
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, PATCHBAY_GROUP_AUDIO_OUT, 0, 0, 0.0f, strBuf);

            for (unsigned int i=0; i < fAudioCountOut; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_AUDIO_OUT, PATCHBAY_GROUP_AUDIO_OUT*1000 + i, PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, strBuf);
            }
        }

        // MIDI In
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, PATCHBAY_GROUP_MIDI_IN, 0, 0, 0.0f, "Readable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiIn.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = PATCHBAY_GROUP_MIDI_IN*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiIn.getPortName(i).c_str(), STR_MAX);
                fUsedMidiIns.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, PATCHBAY_GROUP_MIDI_IN, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);
            }
        }

#if 0 // midi-out not implemented yet
        // MIDI Out
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, 0, 0.0f, "Writable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiOut.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = PATCHBAY_GROUP_MIDI_OUT*1000 + i;
                std::strncpy(portNameToId.name, fDummyMidiOut.getPortName(i).c_str(), STR_MAX);
                fUsedMidiOuts.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, 0, PATCHBAY_GROUP_MIDI_OUT, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, portNameToId.name);
            }
        }
#endif

        // Connections
        fConnectAudioLock.lock();

        for (List<uint>::Itenerator it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getConstValue());
            CARLA_ASSERT(port < fAudioCountIn);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = PATCHBAY_PORT_AUDIO_IN1;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (List<uint>::Itenerator it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getConstValue());
            CARLA_ASSERT(port < fAudioCountIn);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = PATCHBAY_PORT_AUDIO_IN2;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (List<uint>::Itenerator it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
        {
            const uint& port(it.getConstValue());
            CARLA_ASSERT(port < fAudioCountOut);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_AUDIO_OUT1;
            connectionToId.portIn  = PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (List<uint>::Itenerator it = fConnectedAudioOuts[1].begin(); it.valid(); it.next())
        {
            const uint& port(it.getConstValue());
            CARLA_ASSERT(port < fAudioCountOut);

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_AUDIO_OUT2;
            connectionToId.portIn  = PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        fConnectAudioLock.unlock();

        for (List<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getConstValue());

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_GROUP_MIDI_IN*1000 + midiPort.portId;
            connectionToId.portIn  = PATCHBAY_PORT_MIDI_IN;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        for (List<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getConstValue());

            ConnectionToId connectionToId;
            connectionToId.id      = fLastConnectionId;
            connectionToId.portOut = PATCHBAY_PORT_MIDI_OUT;
            connectionToId.portIn  = PATCHBAY_GROUP_MIDI_OUT*1000 + midiPort.portId;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, fLastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            fUsedConnections.append(connectionToId);
            fLastConnectionId++;
        }

        return true;
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
        CARLA_ASSERT_INT2(nframes == pData->bufferSize, nframes, pData->bufferSize);
        CARLA_ASSERT(outsPtr != nullptr);

        if (pData->curPluginCount == 0 || fAudioCountOut == 0 || ! fAudioIsReady)
        {
            if (fAudioCountOut > 0 && fAudioIsReady)
                FLOAT_CLEAR(outsPtr, nframes*fAudioCountOut);

            return runPendingRtEvents();
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
                FLOAT_COPY(fAudioBufIn[i], insPtr+(nframes*i), nframes);
        }

        // initialize audio output
        for (unsigned int i=0; i < fAudioCountOut; ++i)
            FLOAT_CLEAR(fAudioBufOut[i], nframes);

        FLOAT_CLEAR(fAudioBufRackOut[0], nframes);
        FLOAT_CLEAR(fAudioBufRackOut[1], nframes);

        // initialize input events
        carla_zeroMem(pData->bufEvents.in, sizeof(EngineEvent)*kEngineMaxInternalEventCount);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            while (! fMidiInEvents.data.isEmpty())
            {
                RtMidiEvent& midiEvent(fMidiInEvents.data.getFirst(true));
                EngineEvent& engineEvent(pData->bufEvents.in[engineEventIndex++]);

                if (midiEvent.time < pData->timeInfo.frame)
                {
                    engineEvent.time = 0;
                }
                else if (midiEvent.time >= pData->timeInfo.frame + nframes)
                {
                    carla_stderr("MIDI Event in the future!, %i vs %i", engineEvent.time, pData->timeInfo.frame);
                    engineEvent.time = static_cast<uint32_t>(pData->timeInfo.frame) + nframes - 1;
                }
                else
                    engineEvent.time = static_cast<uint32_t>(midiEvent.time - pData->timeInfo.frame);

                engineEvent.fillFromMidiData(midiEvent.size, midiEvent.data);

                if (engineEventIndex >= kEngineMaxInternalEventCount)
                    break;
            }

            fMidiInEvents.mutex.unlock();
        }

        fConnectAudioLock.lock();

        // connect input buffers
        if (fConnectedAudioIns[0].count() == 0)
        {
            FLOAT_CLEAR(fAudioBufRackIn[0], nframes);
        }
        else
        {
            bool first = true;

            for (List<uint>::Itenerator it = fConnectedAudioIns[0].begin(); it.valid(); it.next())
            {
                const uint& port(it.getConstValue());
                CARLA_ASSERT(port < fAudioCountIn);

                if (first)
                {
                    FLOAT_COPY(fAudioBufRackIn[0], fAudioBufIn[port], nframes);
                    first = false;
                }
                else
                {
                    FLOAT_ADD(fAudioBufRackIn[0], fAudioBufIn[port], nframes);
                }
            }

            if (first)
                FLOAT_CLEAR(fAudioBufRackIn[0], nframes);
        }

        if (fConnectedAudioIns[1].count() == 0)
        {
            FLOAT_CLEAR(fAudioBufRackIn[1], nframes);
        }
        else
        {
            bool first = true;

            for (List<uint>::Itenerator it = fConnectedAudioIns[1].begin(); it.valid(); it.next())
            {
                const uint& port(it.getConstValue());
                CARLA_ASSERT(port < fAudioCountIn);

                if (first)
                {
                    FLOAT_COPY(fAudioBufRackIn[1], fAudioBufIn[port], nframes);
                    first = false;
                }
                else
                {
                    FLOAT_ADD(fAudioBufRackIn[1], fAudioBufIn[port], nframes);
                }
            }

            if (first)
                FLOAT_CLEAR(fAudioBufRackIn[1], nframes);
        }

        // process
        processRack(fAudioBufRackIn, fAudioBufRackOut, nframes);

        // connect output buffers
        if (fConnectedAudioOuts[0].count() != 0)
        {
            for (List<uint>::Itenerator it = fConnectedAudioOuts[0].begin(); it.valid(); it.next())
            {
                const uint& port(it.getConstValue());
                CARLA_ASSERT(port < fAudioCountOut);

                FLOAT_ADD(fAudioBufOut[port], fAudioBufRackOut[0], nframes);
            }
        }

        if (fConnectedAudioOuts[1].count() != 0)
        {
            for (List<uint>::Itenerator it = fConnectedAudioOuts[1].begin(); it.valid(); it.next())
            {
                const uint& port(it.getConstValue());
                CARLA_ASSERT(port < fAudioCountOut);

                FLOAT_ADD(fAudioBufOut[port], fAudioBufRackOut[1], nframes);
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
                FLOAT_COPY(outsPtr+(nframes*i), fAudioBufOut[i], nframes);
        }

        // output events
        {
            // TODO
            //fMidiOutEvents...
        }

        runPendingRtEvents();
        return;

        // unused
        (void)streamTime;
        (void)status;
    }

    void handleMidiCallback(double timeStamp, std::vector<unsigned char>* const message)
    {
        if (! fAudioIsReady)
            return;

        const size_t messageSize(message->size());

        if (messageSize == 0 || messageSize > EngineMidiEvent::kDataSize)
            return;

        timeStamp /= 2;

        if (timeStamp > 0.95)
            timeStamp = 0.95;
        else if (timeStamp < 0.0)
            timeStamp = 0.0;

        RtMidiEvent midiEvent;
        midiEvent.time = pData->timeInfo.frame + uint64_t(timeStamp * (double)pData->bufferSize);

        if (midiEvent.time < fLastTime)
            midiEvent.time = fLastTime;
        else
            fLastTime = midiEvent.time;

        midiEvent.size = static_cast<uint8_t>(messageSize);

        size_t i=0;
        for (; i < messageSize; ++i)
            midiEvent.data[i] = message->at(i);
        for (; i < EngineMidiEvent::kDataSize; ++i)
            midiEvent.data[i] = 0;

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

    uint64_t fLastTime;

    List<uint> fConnectedAudioIns[2];
    List<uint> fConnectedAudioOuts[2];
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
    List<PortNameToId> fUsedMidiIns;
    List<PortNameToId> fUsedMidiOuts;
    List<ConnectionToId> fUsedConnections;

    struct MidiPort {
        RtMidi* rtmidi;
        int portId;
    };

    List<MidiPort> fMidiIns;
    List<MidiPort> fMidiOuts;

    struct RtMidiEvent {
        uint64_t time; // needs to compare to internal time
        uint8_t  size;
        uint8_t  data[EngineMidiEvent::kDataSize];
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
            dataPending.spliceAppend(data);
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

CarlaEngine* CarlaEngine::newRtAudio(AudioApi api)
{
    RtAudio::Api rtApi(RtAudio::UNSPECIFIED);

    switch (api)
    {
    case AUDIO_API_NULL:
        rtApi = RtAudio::RTAUDIO_DUMMY;
        break;
    case AUDIO_API_JACK:
        rtApi = RtAudio::UNIX_JACK;
        break;
    case AUDIO_API_ALSA:
        rtApi = RtAudio::LINUX_ALSA;
        break;
    case AUDIO_API_OSS:
        rtApi = RtAudio::LINUX_OSS;
        break;
    case AUDIO_API_PULSE:
        rtApi = RtAudio::LINUX_PULSE;
        break;
    case AUDIO_API_CORE:
        rtApi = RtAudio::MACOSX_CORE;
        break;
    case AUDIO_API_ASIO:
        rtApi = RtAudio::WINDOWS_ASIO;
        break;
    case AUDIO_API_DS:
        rtApi = RtAudio::WINDOWS_DS;
        break;
    }

    return new CarlaEngineRtAudio(rtApi);
}

unsigned int CarlaEngine::getRtAudioApiCount()
{
    initRtApis();

    return static_cast<unsigned int>(gRtAudioApis.size());
}

const char* CarlaEngine::getRtAudioApiName(const unsigned int index)
{
    initRtApis();

    if (index >= gRtAudioApis.size())
        return nullptr;

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

    return nullptr;
}

const char* const* CarlaEngine::getRtAudioApiDeviceNames(const unsigned int index)
{
    initRtApis();

    if (index >= gRtAudioApis.size())
        return nullptr;

    const RtAudio::Api& api(gRtAudioApis[index]);

    RtAudio rtAudio(api);

    const unsigned int devCount(rtAudio.getDeviceCount());

    if (devCount == 0)
        return nullptr;

    List<const char*> devNames;

    for (unsigned int i=0; i < devCount; ++i)
    {
        RtAudio::DeviceInfo devInfo(rtAudio.getDeviceInfo(i));

        if (devInfo.probed && devInfo.outputChannels > 0 /*&& (devInfo.nativeFormats & RTAUDIO_FLOAT32) != 0*/)
            devNames.append(carla_strdup(devInfo.name.c_str()));
    }

    const size_t realDevCount(devNames.count());

    if (gRetNames != nullptr)
    {
        for (int i=0; gRetNames[i] != nullptr; ++i)
            delete[] gRetNames[i];
        delete[] gRetNames;
    }

    gRetNames = new const char*[realDevCount+1];

    for (size_t i=0; i < realDevCount; ++i)
        gRetNames[i] = devNames.getAt(i);

    gRetNames[realDevCount] = nullptr;
    devNames.clear();

    return gRetNames;
}

const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const unsigned int index, const char* const deviceName)
{
    initRtApis();

    if (index >= gRtAudioApis.size())
        return nullptr;

    const RtAudio::Api& api(gRtAudioApis[index]);

    RtAudio rtAudio(api);

    const unsigned int devCount(rtAudio.getDeviceCount());

    if (devCount == 0)
        return nullptr;

    unsigned int i;
    RtAudio::DeviceInfo rtAudioDevInfo;

    for (i=0; i < devCount; ++i)
    {
        rtAudioDevInfo = rtAudio.getDeviceInfo(i);

        if (rtAudioDevInfo.name == deviceName)
            break;
    }

    if (i == devCount)
        return nullptr;

    static EngineDriverDeviceInfo devInfo = { 0x0, nullptr, nullptr };
    static uint32_t dummyBufferSizes[11]  = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
    static double   dummySampleRates[14]  = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0, 0.0 };

    // reset
    devInfo.hints = 0x0;
    devInfo.bufferSizes = dummyBufferSizes;

    // cleanup
    if (devInfo.sampleRates != nullptr && devInfo.sampleRates != dummySampleRates)
    {
        delete[] devInfo.sampleRates;
        devInfo.sampleRates = nullptr;
    }

    if (size_t sampleRatesCount = rtAudioDevInfo.sampleRates.size())
    {
        double* const sampleRates(new double[sampleRatesCount+1]);

        for (size_t i=0; i < sampleRatesCount; ++i)
            sampleRates[i] = rtAudioDevInfo.sampleRates[i];
        sampleRates[sampleRatesCount] = 0.0;

        devInfo.sampleRates = sampleRates;
    }
    else
    {
        devInfo.sampleRates = dummySampleRates;
    }

    return &devInfo;
}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
