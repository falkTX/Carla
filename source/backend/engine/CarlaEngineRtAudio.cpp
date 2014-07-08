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
#include "CarlaStringList.hpp"

#include "RtLinkedList.hpp"

#include "jackbridge/JackBridge.hpp"

#include "rtaudio/RtAudio.h"
#include "rtmidi/RtMidi.h"

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -------------------------------------------------------------------------------------------------------------------
// Global static data

static CharStringListPtr         gDeviceNames;
static std::vector<RtAudio::Api> gRtAudioApis;

// -------------------------------------------------------------------------------------------------------------------

static void initRtAudioAPIsIfNeeded()
{
    static bool needsInit = true;

    if (! needsInit)
        return;

    needsInit = false;

    // get APIs in a local var, and pass wanted ones into gRtAudioApis

    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);

    for (std::vector<RtAudio::Api>::iterator it = apis.begin(), end = apis.end(); it != end; ++it)
    {
        const RtAudio::Api& api(*it);

        if (api == RtAudio::LINUX_ALSA)
            continue;
        if (api == RtAudio::MACOSX_CORE)
            continue;
        if (api == RtAudio::WINDOWS_ASIO)
            continue;
        if (api == RtAudio::WINDOWS_DS)
            continue;
        if (api == RtAudio::UNIX_JACK && ! jackbridge_is_ok())
            continue;

        gRtAudioApis.push_back(api);
    }
}

static const char* getRtAudioApiName(const RtAudio::Api api) noexcept
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

static RtMidi::Api getMatchedAudioMidiAPI(const RtAudio::Api rtApi) noexcept
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
          fAudioInCount(0),
          fAudioOutCount(0),
          fLastEventTime(0),
          fMidiOutVector(3)
    {
        carla_debug("CarlaEngineRtAudio::CarlaEngineRtAudio(%i)", api);

        // just to make sure
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
    }

    ~CarlaEngineRtAudio() override
    {
        CARLA_SAFE_ASSERT(fAudioInCount == 0);
        CARLA_SAFE_ASSERT(fAudioOutCount == 0);
        CARLA_SAFE_ASSERT(fLastEventTime == 0);
        carla_debug("CarlaEngineRtAudio::~CarlaEngineRtAudio()");
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fAudioInCount == 0, false);
        CARLA_SAFE_ASSERT_RETURN(fAudioOutCount == 0, false);
        CARLA_SAFE_ASSERT_RETURN(fLastEventTime == 0, false);
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::init(\"%s\")", clientName);

        if (pData->options.processMode != ENGINE_PROCESS_MODE_CONTINUOUS_RACK && pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
        {
            setLastError("Invalid process mode");
            return false;
        }

        const uint devCount(fAudio.getDeviceCount());

        if (devCount == 0)
        {
            setLastError("No audio devices available for this driver");
            return false;
        }

        RtAudio::StreamParameters iParams, oParams;
        bool deviceSet = false;

        if (pData->options.audioDevice != nullptr && pData->options.audioDevice[0] != '\0')
        {
            for (uint i=0; i < devCount; ++i)
            {
                RtAudio::DeviceInfo devInfo(fAudio.getDeviceInfo(i));

                if (devInfo.probed && devInfo.outputChannels > 0 && devInfo.name == pData->options.audioDevice)
                {
                    deviceSet   = true;
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

            carla_stdout("No device set, using %i inputs and %i outputs", iParams.nChannels, oParams.nChannels);
        }

        if (oParams.nChannels == 0)
        {
            setLastError("Current audio setup has no outputs, cannot continue");
            return false;
        }

        RtAudio::StreamOptions rtOptions;
        rtOptions.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE | RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED;
        rtOptions.streamName = clientName;
        rtOptions.priority = 85;

        if (fAudio.getCurrentApi() == RtAudio::LINUX_ALSA && ! deviceSet)
            rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;

        uint bufferFrames = pData->options.audioBufferSize;

        try {
            fAudio.openStream(&oParams, (iParams.nChannels > 0) ? &iParams : nullptr, RTAUDIO_FLOAT32, pData->options.audioSampleRate, &bufferFrames, carla_rtaudio_process_callback, this, &rtOptions);
        }
        catch (const RtError& e) {
            setLastError(e.what());
            return false;
        }

        pData->bufferSize = bufferFrames;
        pData->sampleRate = fAudio.getStreamSampleRate();

        fAudioInCount  = iParams.nChannels;
        fAudioOutCount = oParams.nChannels;
        fLastEventTime = 0;

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            pData->audio.inCount  = 2;
            pData->audio.outCount = 2;
        }
        else
        {
            pData->audio.inCount  = 0;
            pData->audio.outCount = 0;
        }

        pData->audio.create(pData->bufferSize);

        try {
            fAudio.startStream();
        }
        catch (const RtError& e)
        {
            close();
            setLastError(e.what());
            return false;
        }

        CarlaEngine::init(clientName);
        pData->audio.isReady = true;

        patchbayRefresh();

        return true;
    }

    bool close() override
    {
        CARLA_SAFE_ASSERT(fAudioOutCount != 0);
        carla_debug("CarlaEngineRtAudio::close()");

        pData->audio.isReady = false;

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

        for (LinkedList<MidiInPort>::Itenerator it = fMidiIns.begin(); it.valid(); it.next())
        {
            MidiInPort& inPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            inPort.port->cancelCallback();
            inPort.port->closePort();
            delete inPort.port;
        }

        fMidiIns.clear();
        fMidiInEvents.clear();

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it = fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiOutPort& outPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

            outPort.port->closePort();
            delete outPort.port;
        }

        fMidiOuts.clear();
        fMidiOutMutex.unlock();

        fAudioInCount  = 0;
        fAudioOutCount = 0;
        fLastEventTime = 0;
        fDeviceName.clear();

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
        CARLA_SAFE_ASSERT_RETURN(pData->audio.isReady, false);

        if (pData->graph.isRack)
            patchbayRefreshRack();
        else
            patchbayRefreshPatchbay();

        return true;
    }

    void patchbayRefreshRack()
    {
        RackGraph* const rack(pData->graph.rack);

        rack->connections.clear();

        char strBuf[STR_MAX+1];
        strBuf[STR_MAX] = '\0';

        // Main
        {
            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_CARLA, PATCHBAY_ICON_CARLA, -1, 0.0f, getName());

            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN1,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN2,  PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, "audio-in2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT1, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out1");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT2, PATCHBAY_PORT_TYPE_AUDIO,                        0.0f, "audio-out2");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_IN,    PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT,  0.0f, "midi-in");
            callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_OUT,   PATCHBAY_PORT_TYPE_MIDI,                         0.0f, "midi-out");
        }

        // Audio In
        {
            if (fDeviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Capture (%s)", fDeviceName.buffer());
            else
                std::strncpy(strBuf, "Capture", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_AUDIO_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (uint i=0; i < fAudioInCount; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_AUDIO_IN, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO, 0.0f, strBuf);
            }
        }

        // Audio Out
        {
            if (fDeviceName.isNotEmpty())
                std::snprintf(strBuf, STR_MAX, "Playback (%s)", fDeviceName.buffer());
            else
                std::strncpy(strBuf, "Playback", STR_MAX);

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_AUDIO_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, strBuf);

            for (uint i=0; i < fAudioOutCount; ++i)
            {
                std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);
                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, RACK_GRAPH_GROUP_AUDIO_OUT, static_cast<int>(i), PATCHBAY_PORT_TYPE_AUDIO|PATCHBAY_PORT_IS_INPUT, 0.0f, strBuf);
            }
        }

        // MIDI In
        {
            RtMidiIn midiIn(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), "carla-discovery");

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_MIDI_IN, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Readable MIDI ports");

            for (uint i=0, count = midiIn.getPortCount(); i < count; ++i)
            {
                std::string portName(midiIn.getPortName(i));

                std::snprintf(strBuf, STR_MAX, "Readable MIDI ports:%s", portName.c_str());

                PortNameToId portNameToId;
                portNameToId.setData(RACK_GRAPH_GROUP_MIDI_IN, i, portName.c_str(), strBuf);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, portNameToId.group, static_cast<int>(portNameToId.port), PATCHBAY_PORT_TYPE_MIDI, 0.0f, portNameToId.name);

                rack->midi.ins.append(portNameToId);
            }
        }

        // MIDI Out
        {
            RtMidiOut midiOut(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), "carla-discovery");

            callback(ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED, RACK_GRAPH_GROUP_MIDI_OUT, PATCHBAY_ICON_HARDWARE, -1, 0.0f, "Writable MIDI ports");

            for (uint i=0, count = midiOut.getPortCount(); i < count; ++i)
            {
                std::string portName(midiOut.getPortName(i));

                std::snprintf(strBuf, STR_MAX, "Writable MIDI ports:%s", portName.c_str());

                PortNameToId portNameToId;
                portNameToId.setData(RACK_GRAPH_GROUP_MIDI_OUT, i, portName.c_str(), strBuf);

                callback(ENGINE_CALLBACK_PATCHBAY_PORT_ADDED, portNameToId.group, static_cast<int>(portNameToId.port), PATCHBAY_PORT_TYPE_MIDI|PATCHBAY_PORT_IS_INPUT, 0.0f, portNameToId.name);

                rack->midi.outs.append(portNameToId);
            }
        }

        // Connections
        rack->audio.mutex.lock();

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedIn1.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioInCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_AUDIO_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN1);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedIn2.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioInCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_AUDIO_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_IN2);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedOut1.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioOutCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT1, RACK_GRAPH_GROUP_AUDIO_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        for (LinkedList<uint>::Itenerator it = rack->audio.connectedOut2.begin(); it.valid(); it.next())
        {
            const uint& portId(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(portId < fAudioOutCount);

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_AUDIO_OUT2, RACK_GRAPH_GROUP_AUDIO_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        rack->audio.mutex.unlock();

        for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            const MidiInPort& inPort(it.getValue());

            const uint portId(rack->midi.getPortId(true, inPort.name));
            CARLA_SAFE_ASSERT_CONTINUE(portId < rack->midi.ins.count());

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_MIDI_IN, portId, RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_IN);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            const MidiOutPort& outPort(it.getValue());

            const uint portId(rack->midi.getPortId(false, outPort.name));
            CARLA_SAFE_ASSERT_CONTINUE(portId < rack->midi.outs.count());

            ConnectionToId connectionToId;
            connectionToId.setData(++(rack->connections.lastId), RACK_GRAPH_GROUP_CARLA, RACK_GRAPH_CARLA_PORT_MIDI_OUT, RACK_GRAPH_GROUP_MIDI_OUT, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            callback(ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED, connectionToId.id, 0, 0, 0.0f, strBuf);

            rack->connections.list.append(connectionToId);
        }

        fMidiOutMutex.unlock();
    }

    void patchbayRefreshPatchbay() noexcept
    {
    }

    // -------------------------------------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer, uint nframes, double streamTime, RtAudioStreamStatus status)
    {
        // get buffers from RtAudio
        const float* const insPtr  = (const float*)inputBuffer;
              float* const outsPtr =       (float*)outputBuffer;

        // assert rtaudio buffers
        CARLA_SAFE_ASSERT_RETURN(outputBuffer      != nullptr, runPendingRtEvents());
        CARLA_SAFE_ASSERT_RETURN(pData->bufferSize == nframes, runPendingRtEvents());

        if (! pData->audio.isReady)
            return runPendingRtEvents();

        // initialize rtaudio input
        const float* inBuf[fAudioInCount];

        for (uint i=0; i < fAudioInCount; ++i)
            inBuf[i] = insPtr+(nframes*i);

        // initialize rtaudio output
        float* outBuf[fAudioOutCount];

        for (uint i=0; i < fAudioOutCount; ++i)
            outBuf[i] = outsPtr+(nframes*i);

        FLOAT_CLEAR(outsPtr, nframes*fAudioOutCount);

        // initialize events
        carla_zeroStruct<EngineEvent>(pData->events.in,  kMaxEngineEventInternalCount);
        carla_zeroStruct<EngineEvent>(pData->events.out, kMaxEngineEventInternalCount);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            for (LinkedList<RtMidiEvent>::Itenerator it = fMidiInEvents.data.begin(); it.valid(); it.next())
            {
                const RtMidiEvent& midiEvent(it.getValue());
                EngineEvent&       engineEvent(pData->events.in[engineEventIndex++]);

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

            fMidiInEvents.data.clear();
            fMidiInEvents.mutex.unlock();
        }

        if (pData->graph.isRack)
        {
            pData->processRackFull(inBuf, fAudioInCount, outBuf, fAudioOutCount, nframes, false);
        }
        else
        {
        }

        fMidiOutMutex.lock();

        if (fMidiOuts.count() > 0)
        {
            uint8_t        size    = 0;
            uint8_t        data[3] = { 0, 0, 0 };
            const uint8_t* dataPtr = data;

            for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
            {
                const EngineEvent& engineEvent(pData->events.out[i]);

                if (engineEvent.type == kEngineEventTypeNull)
                    break;

                else if (engineEvent.type == kEngineEventTypeControl)
                {
                    const EngineControlEvent& ctrlEvent(engineEvent.ctrl);
                    ctrlEvent.convertToMidiData(engineEvent.channel, size, data);
                    dataPtr = data;
                }
                else if (engineEvent.type == kEngineEventTypeMidi)
                {
                    const EngineMidiEvent& midiEvent(engineEvent.midi);

                    size = midiEvent.size;

                    if (size > EngineMidiEvent::kDataSize && midiEvent.dataExt != nullptr)
                        dataPtr = midiEvent.dataExt;
                    else
                        dataPtr = midiEvent.data;
                }
                else
                {
                    continue;
                }

                if (size > 0)
                {
                    fMidiOutVector.assign(dataPtr, dataPtr + size);

                    for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
                    {
                        MidiOutPort& outPort(it.getValue());
                        CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

                        outPort.port->sendMessage(&fMidiOutVector);
                    }
                }
            }
        }

        fMidiOutMutex.unlock();

        runPendingRtEvents();
        return;

        // unused
        (void)streamTime;
        (void)status;
    }

    void handleMidiCallback(double timeStamp, std::vector<uchar>* const message)
    {
        if (! pData->audio.isReady)
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

    bool connectRackMidiInPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::connectRackMidiInPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        CarlaString newRtMidiPortName;
        newRtMidiPortName += getName();
        newRtMidiPortName += ":";
        newRtMidiPortName += portName;

        RtMidiIn* const rtMidiIn(new RtMidiIn(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), newRtMidiPortName.buffer(), 512));
        rtMidiIn->ignoreTypes();
        rtMidiIn->setCallback(carla_rtmidi_callback, this);

        bool found = false;
        uint rtMidiPortIndex;

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

        try {
            rtMidiIn->openPort(rtMidiPortIndex, portName);
        }
        catch(...) {
            delete rtMidiIn;
            return false;
        };

        MidiInPort midiPort;
        midiPort.port = rtMidiIn;

        std::strncpy(midiPort.name, portName, STR_MAX);
        midiPort.name[STR_MAX] = '\0';

        fMidiIns.append(midiPort);
        return true;
    }

    bool connectRackMidiOutPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::connectRackMidiOutPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        CarlaString newRtMidiPortName;
        newRtMidiPortName += getName();
        newRtMidiPortName += ":";
        newRtMidiPortName += portName;

        RtMidiOut* const rtMidiOut(new RtMidiOut(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), newRtMidiPortName.buffer()));

        bool found = false;
        uint rtMidiPortIndex;

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

        try {
            rtMidiOut->openPort(rtMidiPortIndex, portName);
        }
        catch(...) {
            delete rtMidiOut;
            return false;
        };

        MidiOutPort midiPort;
        midiPort.port = rtMidiOut;

        std::strncpy(midiPort.name, portName, STR_MAX);
        midiPort.name[STR_MAX] = '\0';

        const CarlaMutexLocker cml(fMidiOutMutex);

        fMidiOuts.append(midiPort);
        return true;
    }

    bool disconnectRackMidiInPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::disconnectRackMidiInPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.ins.count() > 0, false);

        for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin(); it.valid(); it.next())
        {
            MidiInPort& inPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            if (std::strcmp(inPort.name, portName) != 0)
                continue;

            inPort.port->cancelCallback();
            inPort.port->closePort();
            delete inPort.port;

            fMidiIns.remove(it);
            return true;
        }

        return false;
    }

    bool disconnectRackMidiOutPort(const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(portName != nullptr && portName[0] != '\0', false);
        carla_debug("CarlaEngineRtAudio::disconnectRackMidiOutPort(\"%s\")", portName);

        RackGraph* const rack(pData->graph.rack);
        CARLA_SAFE_ASSERT_RETURN(rack->midi.outs.count() > 0, false);

        const CarlaMutexLocker cml(fMidiOutMutex);

        for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin(); it.valid(); it.next())
        {
            MidiOutPort& outPort(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

            if (std::strcmp(outPort.name, portName) != 0)
                continue;

            outPort.port->closePort();
            delete outPort.port;

            fMidiOuts.remove(it);
            return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    RtAudio fAudio;

    // useful info
    uint fAudioInCount;
    uint fAudioOutCount;
    uint64_t fLastEventTime;

    // current device name
    CarlaString fDeviceName;

    struct MidiInPort {
        RtMidiIn* port;
        char name[STR_MAX+1];
    };

    struct MidiOutPort {
        RtMidiOut* port;
        char name[STR_MAX+1];
    };

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
            dataPending.spliceAppendTo(data);
        }
    };

    LinkedList<MidiInPort> fMidiIns;
    RtMidiEvents           fMidiInEvents;

    LinkedList<MidiOutPort> fMidiOuts;
    CarlaMutex              fMidiOutMutex;
    std::vector<uint8_t>    fMidiOutVector;

    #define handlePtr ((CarlaEngineRtAudio*)userData)

    static int carla_rtaudio_process_callback(void* outputBuffer, void* inputBuffer, uint nframes, double streamTime, RtAudioStreamStatus status, void* userData)
    {
        handlePtr->handleAudioProcessCallback(outputBuffer, inputBuffer, nframes, streamTime, status);
        return 0;
    }

    static void carla_rtmidi_callback(double timeStamp, std::vector<uchar>* message, void* userData)
    {
        handlePtr->handleMidiCallback(timeStamp, message);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudio)
};

// -----------------------------------------

CarlaEngine* CarlaEngine::newRtAudio(const AudioApi api)
{
    initRtAudioAPIsIfNeeded();

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

uint CarlaEngine::getRtAudioApiCount()
{
    initRtAudioAPIsIfNeeded();

    return static_cast<uint>(gRtAudioApis.size());
}

const char* CarlaEngine::getRtAudioApiName(const uint index)
{
    initRtAudioAPIsIfNeeded();

    CARLA_SAFE_ASSERT_RETURN(index < gRtAudioApis.size(), nullptr);

    return CarlaBackend::getRtAudioApiName(gRtAudioApis[index]);
}

const char* const* CarlaEngine::getRtAudioApiDeviceNames(const uint index)
{
    initRtAudioAPIsIfNeeded();

    if (index >= gRtAudioApis.size())
        return nullptr;

    const RtAudio::Api& api(gRtAudioApis[index]);

    RtAudio rtAudio(api);

    const uint devCount(rtAudio.getDeviceCount());

    if (devCount == 0)
        return nullptr;

    CarlaStringList devNames;

    for (uint i=0; i < devCount; ++i)
    {
        RtAudio::DeviceInfo devInfo(rtAudio.getDeviceInfo(i));

        if (devInfo.probed && devInfo.outputChannels > 0 /*&& (devInfo.nativeFormats & RTAUDIO_FLOAT32) != 0*/)
            devNames.append(devInfo.name.c_str());
    }

    gDeviceNames = devNames.toCharStringListPtr();

    return gDeviceNames;
}

const EngineDriverDeviceInfo* CarlaEngine::getRtAudioDeviceInfo(const uint index, const char* const deviceName)
{
    initRtAudioAPIsIfNeeded();

    if (index >= gRtAudioApis.size())
        return nullptr;

    const RtAudio::Api& api(gRtAudioApis[index]);

    RtAudio rtAudio(api);

    const uint devCount(rtAudio.getDeviceCount());

    if (devCount == 0)
        return nullptr;

    uint i;
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
    static uint32_t dummyBufferSizes[]    = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
    static double   dummySampleRates[]    = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0, 0.0 };

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
