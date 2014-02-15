/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaMathUtils.hpp"

#include "RtLinkedList.hpp"

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
    {
        RtAudio::getCompiledApi(gRtAudioApis);

#if 0//def HAVE_JUCE
        // prefer juce to handle some APIs
        std::vector<RtAudio::Api>::iterator it = std::find(gRtAudioApis.begin(), gRtAudioApis.end(), RtAudio::LINUX_ALSA);
        if (it != gRtAudioApis.end()) gRtAudioApis.erase(it);

        it = std::find(gRtAudioApis.begin(), gRtAudioApis.end(), RtAudio::MACOSX_CORE);
        if (it != gRtAudioApis.end()) gRtAudioApis.erase(it);

        it = std::find(gRtAudioApis.begin(), gRtAudioApis.end(), RtAudio::WINDOWS_ASIO);
        if (it != gRtAudioApis.end()) gRtAudioApis.erase(it);

        it = std::find(gRtAudioApis.begin(), gRtAudioApis.end(), RtAudio::WINDOWS_DS);
        if (it != gRtAudioApis.end()) gRtAudioApis.erase(it);
#endif

        // in case rtaudio has no apis, add dummy one so that size() != 0
        if (gRtAudioApis.size() == 0)
            gRtAudioApis.push_back(RtAudio::RTAUDIO_DUMMY);
    }
}

static const char* getRtAudioApiName(const RtAudio::Api api)
{
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

    carla_stderr("CarlaBackend::getRtAudioApiName(%i) - invalid API", api);
    return nullptr;
}

static RtMidi::Api getMatchedAudioMidiAPi(const RtAudio::Api rtApi)
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
// Cleanup

static struct RtAudioCleanup {
    RtAudioCleanup() {}
    ~RtAudioCleanup()
    {
        if (gRetNames != nullptr)
        {
            delete[] gRetNames;
            gRetNames = nullptr;
        }

        gRtAudioApis.clear();
    }
} sRtAudioCleanup;

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
          fIsAudioInterleaved(false),
          fLastEventTime(0),
          fDummyMidiIn(getMatchedAudioMidiAPi(api), "Carla"),
          fDummyMidiOut(getMatchedAudioMidiAPi(api), "Carla")
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

        // just to make sure
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
    }

    ~CarlaEngineRtAudio() override
    {
        CARLA_ASSERT(fAudioBufIn == nullptr);
        CARLA_ASSERT(fAudioBufOut == nullptr);
        carla_debug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");

        fUsedMidiIns.clear();
        fUsedMidiOuts.clear();
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fAudioBufIn == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fAudioBufOut == nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        if (pData->options.processMode != ENGINE_PROCESS_MODE_CONTINUOUS_RACK && pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
        {
            setLastError("Invalid process mode");
            return false;
        }

        pData->bufAudio.usePatchbay = (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY);

        RtAudio::StreamParameters iParams, oParams;
        bool deviceSet = false;

        const unsigned int devCount(fAudio.getDeviceCount());

        if (devCount == 0)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        if (pData->options.audioDevice != nullptr && pData->options.audioDevice[0] != '\0')
        {
            for (unsigned int i=0; i < devCount; ++i)
            {
                RtAudio::DeviceInfo devInfo(fAudio.getDeviceInfo(i));

                if (devInfo.probed && devInfo.outputChannels > 0 && devInfo.name == pData->options.audioDevice)
                {
                    deviceSet    = true;
                    fDeviceName = devInfo.name.c_str();
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
            iParams.nChannels = fAudio.getDeviceInfo(iParams.deviceId).inputChannels;
            oParams.nChannels = fAudio.getDeviceInfo(oParams.deviceId).outputChannels;
        }

        RtAudio::StreamOptions rtOptions;
        rtOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME;
        rtOptions.streamName = clientName;
        rtOptions.priority = 85;

        if (fAudio.getCurrentApi() != RtAudio::LINUX_PULSE)
        {
            rtOptions.flags |= RTAUDIO_NONINTERLEAVED;
            fIsAudioInterleaved = false;

            if (fAudio.getCurrentApi() == RtAudio::LINUX_ALSA && ! deviceSet)
                rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
        }
        else
            fIsAudioInterleaved = true;

        fLastEventTime = 0;

        unsigned int bufferFrames = pData->options.audioBufferSize;

        try {
            fAudio.openStream(&oParams, &iParams, RTAUDIO_FLOAT32, pData->options.audioSampleRate, &bufferFrames, carla_rtaudio_process_callback, this, &rtOptions);
        }
        catch (const RtError& e)
        {
            setLastError(e.what());
            return false;
        }

        pData->bufferSize = bufferFrames;
        pData->sampleRate = fAudio.getStreamSampleRate();

        pData->bufAudio.inCount  = iParams.nChannels;
        pData->bufAudio.outCount = oParams.nChannels;

        CARLA_ASSERT(pData->bufAudio.outCount > 0);

        if (pData->bufAudio.inCount > 0)
        {
            fAudioBufIn = new float*[pData->bufAudio.inCount];

            for (uint i=0; i < pData->bufAudio.inCount; ++i)
                fAudioBufIn[i] = new float[pData->bufferSize];
        }

        if (pData->bufAudio.outCount > 0)
        {
            fAudioBufOut = new float*[pData->bufAudio.outCount];

            for (uint i=0; i < pData->bufAudio.outCount; ++i)
                fAudioBufOut[i] = new float[pData->bufferSize];
        }

        pData->bufAudio.create(pData->bufferSize);

        try {
            fAudio.startStream();
        }
        catch (const RtError& e)
        {
            setLastError(e.what());
            fAudio.closeStream();
            return false;
        }

        CarlaEngine::init(clientName);
        patchbayRefresh();

        return true;
    }

    bool close() override
    {
        CARLA_ASSERT(fAudioBufOut != nullptr);
        carla_debug("CarlaEngineRtAudio::close()");

        pData->bufAudio.isReady = false;

        bool hasError = !CarlaEngine::close();

        if (fAudio.isStreamOpen())
        {
            if (fAudio.isStreamRunning())
            {
                try {
                    fAudio.stopStream();
                }
                catch (const RtError& e)
                {
                    if (! hasError)
                    {
                        setLastError(e.what());
                        hasError = true;
                    }
                }
            }

            fAudio.closeStream();
        }

        if (fAudioBufIn != nullptr)
        {
            for (uint i=0; i < pData->bufAudio.inCount; ++i)
                delete[] fAudioBufIn[i];

            delete[] fAudioBufIn;
            fAudioBufIn = nullptr;
        }

        if (fAudioBufOut != nullptr)
        {
            for (uint i=0; i < pData->bufAudio.outCount; ++i)
                delete[] fAudioBufOut[i];

            delete[] fAudioBufOut;
            fAudioBufOut = nullptr;
        }

        pData->bufAudio.clear();

        fDeviceName.clear();

        for (LinkedList<MidiPort>::Itenerator it = fMidiIns.begin(); it.valid(); it.next())
        {
            MidiPort& port(it.getValue());
            RtMidiIn* const midiInPort((RtMidiIn*)port.rtmidi);

            midiInPort->cancelCallback();
            delete midiInPort;
        }

        for (LinkedList<MidiPort>::Itenerator it = fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiPort& port(it.getValue());
            RtMidiOut* const midiOutPort((RtMidiOut*)port.rtmidi);

            delete midiOutPort;
        }

        fMidiIns.clear();
        fMidiOuts.clear();

        fMidiInEvents.clear();
        //fMidiOutEvents.clear();

        return !hasError;
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
        return CarlaBackend::getRtAudioApiName(fAudio.getCurrentApi());
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayRefresh() override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->bufAudio.isReady, false);

        fUsedMidiIns.clear();
        fUsedMidiOuts.clear();

        pData->bufAudio.initPatchbay();

        if (pData->bufAudio.usePatchbay)
        {
            // not implemented yet
            return false;
        }

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        EngineRackBuffers* const rack(pData->bufAudio.rack);

        // Main
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_CARLA, PATCHBAY_ICON_CARLA, -1, 0.0f, getName());

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_CARLA, RACK_PATCHBAY_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
        }

        // Audio In
        {
            if (fDeviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", (const char*)fDeviceName);
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (uint i=0; i < pData->bufAudio.inCount; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_IN, int(RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + i), int(PATCHBAY_PORT_TYPE_AUDIO), 0.0f, strBuf);
            }
        }

        // Audio Out
        {
            if (fDeviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", (const char*)fDeviceName);
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (uint i=0; i < pData->bufAudio.outCount; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_AUDIO_OUT, int(RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + i), int(PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT), 0.0f, strBuf);
            }
        }

        // MIDI In
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

            for (uint i=0, count=fDummyMidiIn.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = int(RACK_PATCHBAY_GROUP_MIDI_IN*1000 + i);
                std::strncpy(portNameToId.name, fDummyMidiIn.getPortName(i).c_str(), STR_MAX);
                portNameToId.name[STR_MAX] = '\0';
                fUsedMidiIns.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_PATCHBAY_GROUP_MIDI_IN, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);
            }
        }

#if 0 // midi-out not implemented yet
        // MIDI Out
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_PATCHBAY_GROUP_MIDI_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

            for (unsigned int i=0, count=fDummyMidiOut.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.portId = int(RACK_PATCHBAY_GROUP_MIDI_OUT*1000 + i);
                std::strncpy(portNameToId.name, fDummyMidiOut.getPortName(i).c_str(), STR_MAX);
                portNameToId.name[STR_MAX] = '\0';
                fUsedMidiOuts.append(portNameToId);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, 0, RACK_PATCHBAY_GROUP_MIDI_OUT, portNameToId.portId, PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, portNameToId.name);
            }
        }
#endif

        // Connections
        rack->connectLock.enter();

        for (LinkedList<int>::Itenerator it = rack->connectedIn1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->bufAudio.inCount));

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_AUDIO_IN1;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<int>::Itenerator it = rack->connectedIn2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->bufAudio.inCount));

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_AUDIO_IN*1000 + port;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_AUDIO_IN2;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<int>::Itenerator it = rack->connectedOut1.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->bufAudio.outCount));

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_AUDIO_OUT1;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<int>::Itenerator it = rack->connectedOut2.begin(); it.valid(); it.next())
        {
            const int& port(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(port >= 0 && port < static_cast<int>(pData->bufAudio.outCount));

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_AUDIO_OUT2;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_AUDIO_OUT*1000 + port;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        pData->bufAudio.rack->connectLock.leave();

        for (LinkedList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getValue());

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_GROUP_MIDI_IN*1000 + midiPort.portId;
            connectionToId.portIn  = RACK_PATCHBAY_PORT_MIDI_IN;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        for (LinkedList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiPort& midiPort(it.getValue());

            ConnectionToId connectionToId;
            connectionToId.id      = rack->lastConnectionId;
            connectionToId.portOut = RACK_PATCHBAY_PORT_MIDI_OUT;
            connectionToId.portIn  = RACK_PATCHBAY_GROUP_MIDI_OUT*1000 + midiPort.portId;

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, rack->lastConnectionId, connectionToId.portOut, connectionToId.portIn, 0.0f, nullptr);

            rack->usedConnections.append(connectionToId);
            rack->lastConnectionId++;
        }

        return true;
    }

    // -------------------------------------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer, unsigned int nframes, double streamTime, RtAudioStreamStatus status)
    {
        // get buffers from RtAudio
        float* const insPtr  = (float*)inputBuffer;
        float* const outsPtr = (float*)outputBuffer;

        // assert rtaudio buffers
        CARLA_SAFE_ASSERT_RETURN(outputBuffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(nframes == pData->bufferSize,);

        if (pData->bufAudio.outCount == 0 || ! pData->bufAudio.isReady)
            return runPendingRtEvents();

        // initialize rtaudio input
        if (fIsAudioInterleaved)
        {
            for (unsigned int i=0, j=0, count=nframes*pData->bufAudio.inCount; i < count; ++i)
            {
                fAudioBufIn[i/pData->bufAudio.inCount][j] = insPtr[i];

                if ((i+1) % pData->bufAudio.inCount == 0)
                    j += 1;
            }
        }
        else
        {
            for (unsigned int i=0; i < pData->bufAudio.inCount; ++i)
                FLOAT_COPY(fAudioBufIn[i], insPtr+(nframes*i), nframes);
        }

        // initialize rtaudio output
        for (unsigned int i=0; i < pData->bufAudio.outCount; ++i)
            FLOAT_CLEAR(fAudioBufOut[i], nframes);

        // initialize input events
        carla_zeroStruct<EngineEvent>(pData->bufEvents.in, kMaxEngineEventInternalCount);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            while (! fMidiInEvents.data.isEmpty())
            {
                const RtMidiEvent& midiEvent(fMidiInEvents.data.getFirst(true));
                EngineEvent&       engineEvent(pData->bufEvents.in[engineEventIndex++]);

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

                if (engineEventIndex >= kMaxEngineEventInternalCount)
                    break;
            }

            fMidiInEvents.mutex.unlock();
        }

        if (pData->bufAudio.usePatchbay)
        {
        }
        else
        {
            pData->processRackFull(fAudioBufIn, pData->bufAudio.inCount, fAudioBufOut, pData->bufAudio.outCount, nframes, false);
        }

        // output audio
        if (fIsAudioInterleaved)
        {
            for (unsigned int i=0, j=0; i < nframes*pData->bufAudio.outCount; ++i)
            {
                outsPtr[i] = fAudioBufOut[i/pData->bufAudio.outCount][j];

                if ((i+1) % pData->bufAudio.outCount == 0)
                    j += 1;
            }
        }
        else
        {
            for (unsigned int i=0; i < pData->bufAudio.outCount; ++i)
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
        if (! pData->bufAudio.isReady)
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

        if (midiEvent.time < fLastEventTime)
            midiEvent.time = fLastEventTime;
        else
            fLastEventTime = midiEvent.time;

        midiEvent.size = static_cast<uint8_t>(messageSize);

        size_t i=0;
        for (; i < messageSize; ++i)
            midiEvent.data[i] = message->at(i);
        for (; i < EngineMidiEvent::kDataSize; ++i)
            midiEvent.data[i] = 0;

        fMidiInEvents.append(midiEvent);
    }

    // -------------------------------------------------------------------

    bool connectRackMidiInPort(const int portId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsedMidiIns.count() > 0, false);
        CARLA_SAFE_ASSERT_RETURN(portId >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(portId) < fUsedMidiIns.count(), false);
        carla_debug("CarlaEngineRtAudio::connectRackMidiInPort(%i)", portId);

        const char* const portName(fUsedMidiIns.getAt(static_cast<size_t>(portId)).name);

        char newPortName[STR_MAX+1];
        std::snprintf(newPortName, STR_MAX, "%s:in-%i", (const char*)getName(), portId+1);

        bool found = false;
        uint rtMidiPortIndex;

        RtMidiIn* const rtMidiIn(new RtMidiIn(getMatchedAudioMidiAPi(fAudio.getCurrentApi()), newPortName, 512));
        rtMidiIn->ignoreTypes();
        rtMidiIn->setCallback(carla_rtmidi_callback, this);

        for (uint i=0, count=rtMidiIn->getPortCount(); i < count; ++i)
        {
            if (rtMidiIn->getPortName(i) == portName)
            {
                found = true;
                rtMidiPortIndex = i;
                break;
            }
        }

        if (! found)
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

    bool connectRackMidiOutPort(const int portId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsedMidiOuts.count() > 0, false);
        CARLA_SAFE_ASSERT_RETURN(portId >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(portId) < fUsedMidiOuts.count(), false);
        carla_debug("CarlaEngineRtAudio::connectRackMidiOutPort(%i)", portId);

        const char* const portName(fUsedMidiOuts.getAt(static_cast<size_t>(portId)).name);

        char newPortName[STR_MAX+1];
        std::snprintf(newPortName, STR_MAX, "%s:out-%i", (const char*)getName(), portId+1);

        bool found = false;
        uint rtMidiPortIndex;

        RtMidiOut* const rtMidiOut(new RtMidiOut(getMatchedAudioMidiAPi(fAudio.getCurrentApi()), newPortName));

        for (uint i=0, count=rtMidiOut->getPortCount(); i < count; ++i)
        {
            if (rtMidiOut->getPortName(i) == portName)
            {
                found = true;
                rtMidiPortIndex = i;
                break;
            }
        }

        if (! found)
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

    bool disconnectRackMidiInPort(const int portId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsedMidiIns.count() > 0, false);
        CARLA_SAFE_ASSERT_RETURN(portId >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(portId) < fUsedMidiIns.count(), false);
        carla_debug("CarlaEngineRtAudio::connectRackMidiInPort(%i)", portId);

        for (LinkedList<MidiPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiPort& midiPort(it.getValue());

            if (midiPort.portId == portId)
            {
                RtMidiOut* const midiOutPort((RtMidiOut*)midiPort.rtmidi);

                delete midiOutPort;

                fMidiOuts.remove(it);
                return true;
            }
        }

        return false;
    }

    bool disconnectRackMidiOutPort(const int portId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fUsedMidiOuts.count() > 0, false);
        CARLA_SAFE_ASSERT_RETURN(portId >= 0, false);
        CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(portId) < fUsedMidiOuts.count(), false);
        carla_debug("CarlaEngineRtAudio::disconnectRackMidiOutPort(%i)", portId);

        for (LinkedList<MidiPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            MidiPort& midiPort(it.getValue());

            if (midiPort.portId == portId)
            {
                RtMidiIn* const midiInPort((RtMidiIn*)midiPort.rtmidi);

                midiInPort->cancelCallback();
                delete midiInPort;

                fMidiIns.remove(it);
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    RtAudio fAudio;

    // audio copy to split and de-interleave rtaudio buffers
    float** fAudioBufIn;
    float** fAudioBufOut;

    // useful info
    bool     fIsAudioInterleaved;
    uint64_t fLastEventTime;

    // current device name
    CarlaString fDeviceName;

    // dummy rtmidi to scan available ports
    RtMidiIn  fDummyMidiIn;
    RtMidiOut fDummyMidiOut;

    LinkedList<PortNameToId> fUsedMidiIns;
    LinkedList<PortNameToId> fUsedMidiOuts;

    struct MidiPort {
        RtMidi* rtmidi;
        int portId;
    };

    LinkedList<MidiPort> fMidiIns;
    LinkedList<MidiPort> fMidiOuts;

    struct RtMidiEvent {
        uint64_t time; // needs to compare to internal time
        uint8_t  size;
        uint8_t  data[EngineMidiEvent::kDataSize];
    };

    struct RtMidiEvents {
        CarlaMutex mutex;
        RtLinkedList<RtMidiEvent>::Pool dataPool;
        RtLinkedList<RtMidiEvent> data;
        RtLinkedList<RtMidiEvent> dataPending;

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
    initRtApis();

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

    return CarlaBackend::getRtAudioApiName(gRtAudioApis[index]);
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

    LinkedList<const char*> devNames;

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

        for (size_t j=0; j < sampleRatesCount; ++j)
            sampleRates[j] = rtAudioDevInfo.sampleRates[j];
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
