// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaEngineGraph.hpp"
#include "CarlaEngineInit.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaStringList.hpp"

#include "RtLinkedList.hpp"

#include "jackbridge/JackBridge.hpp"

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wconversion"
# pragma clang diagnostic ignored "-Wdeprecated-copy"
# pragma clang diagnostic ignored "-Weffc++"
# pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "rtaudio/RtAudio.h"
#include "rtmidi/RtMidi.h"

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

CARLA_BACKEND_START_NAMESPACE

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

    for (std::vector<RtAudio::Api>::const_iterator it = apis.begin(), end=apis.end(); it != end; ++it)
    {
        const RtAudio::Api& api(*it);

        if (api == RtAudio::UNIX_JACK)
        {
#if defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
            if ( ! jackbridge_is_ok())
                continue;
#else
            /* NOTE
             * RtMidi doesn't have a native MIDI backend for these OSes,
             * Using RtAudio JACK funcitonality is only useful when we need to access the native MIDI APIs.
             * (JACK audio + ALSA MIDI, or JACK audio + CoreMidi, or JACK audio + Windows MIDI)
             * Because RtMidi has no native MIDI support outside of win/mac/linux, we skip these RtAudio APIs.
             * Those OSes can use Carla's JACK support directly, which is much better than RtAudio classes.
             */
            continue;
#endif
        }

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
    case RtAudio::LINUX_OSS:
        return "OSS";
    case RtAudio::UNIX_PULSE:
        return "PulseAudio";
    case RtAudio::UNIX_JACK:
#if defined(CARLA_OS_LINUX) && defined(__LINUX_ALSA__)
        return "JACK with ALSA-MIDI";
#elif defined(CARLA_OS_MAC)
        return "JACK with CoreMidi";
#elif defined(CARLA_OS_WIN)
        return "JACK with WinMM";
#else
        return "JACK (RtAudio)";
#endif
    case RtAudio::MACOSX_CORE:
        return "CoreAudio";
    case RtAudio::WINDOWS_ASIO:
        return "ASIO";
    case RtAudio::WINDOWS_DS:
        return "DirectSound";
    case RtAudio::WINDOWS_WASAPI:
        return "WASAPI";
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
        return RtMidi::LINUX_ALSA;

    case RtAudio::UNIX_PULSE:
    case RtAudio::UNIX_JACK:
#if defined(CARLA_OS_LINUX) && defined(__LINUX_ALSA__)
        return RtMidi::LINUX_ALSA;
#elif defined(CARLA_OS_MAC)
        return RtMidi::MACOSX_CORE;
#elif defined(CARLA_OS_WIN)
        return RtMidi::WINDOWS_MM;
#else
        return RtMidi::RTMIDI_DUMMY;
#endif

    case RtAudio::MACOSX_CORE:
        return RtMidi::MACOSX_CORE;

    case RtAudio::WINDOWS_ASIO:
    case RtAudio::WINDOWS_DS:
    case RtAudio::WINDOWS_WASAPI:
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
          fAudioInterleaved(false),
          fAudioInCount(0),
          fAudioOutCount(0),
          fLastEventTime(0),
          fDeviceName(),
          fAudioIntBufIn(nullptr),
          fAudioIntBufOut(nullptr),
          fMidiIns(),
          fMidiInEvents(),
          fMidiOuts(),
          fMidiOutMutex(),
          fMidiOutVector(EngineMidiEvent::kDataSize)
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

        const bool isDummy(fAudio.getCurrentApi() == RtAudio::RtAudio::RTAUDIO_DUMMY);
        bool deviceSet = false;
        RtAudio::StreamParameters iParams, oParams;

        if (isDummy)
        {
            if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                setLastError("Cannot use dummy driver in Rack mode");
                return false;
            }

            fDeviceName = "Dummy";
        }
        else
        {
            const uint devCount(fAudio.getDeviceCount());

            if (devCount == 0)
            {
                setLastError("No audio devices available for this driver");
                return false;
            }

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

            if (oParams.nChannels == 0 && pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            {
                setLastError("Current audio setup has no outputs, cannot continue");
                return false;
            }

            iParams.nChannels = carla_fixedValue(0U, 128U, iParams.nChannels);
            oParams.nChannels = carla_fixedValue(0U, 128U, oParams.nChannels);
            fAudioInterleaved = fAudio.getCurrentApi() == RtAudio::UNIX_PULSE;
        }

        RtAudio::StreamOptions rtOptions;
        rtOptions.flags = RTAUDIO_SCHEDULE_REALTIME;
       #ifndef CARLA_OS_MAC
        rtOptions.flags |= RTAUDIO_MINIMIZE_LATENCY;
       #endif
        rtOptions.numberOfBuffers = pData->options.audioTripleBuffer ? 3 : 2;
        rtOptions.streamName = clientName;
        rtOptions.priority = 85;

        if (fAudio.getCurrentApi() == RtAudio::LINUX_ALSA && ! deviceSet)
            rtOptions.flags |= RTAUDIO_ALSA_USE_DEFAULT;
        if (! fAudioInterleaved)
            rtOptions.flags |= RTAUDIO_NONINTERLEAVED;

        uint bufferFrames = pData->options.audioBufferSize;

        try {
            fAudio.openStream(oParams.nChannels > 0 ? &oParams : nullptr,
                              iParams.nChannels > 0 ? &iParams : nullptr,
                              RTAUDIO_FLOAT32, pData->options.audioSampleRate, &bufferFrames,
                              carla_rtaudio_process_callback, this, &rtOptions,
                              carla_rtaudio_buffer_size_callback);
        }
        catch (const RtAudioError& e) {
            setLastError(e.what());
            return false;
        }

        if (! pData->init(clientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        pData->bufferSize = bufferFrames;
        pData->sampleRate = isDummy ? 44100.0 : fAudio.getStreamSampleRate();
        pData->initTime(pData->options.transportExtra);

        fAudioInCount  = iParams.nChannels;
        fAudioOutCount = oParams.nChannels;
        fLastEventTime = 0;

        if (fAudioInCount > 0)
            fAudioIntBufIn = new float[fAudioInCount*bufferFrames];

        if (fAudioOutCount > 0)
            fAudioIntBufOut = new float[fAudioOutCount*bufferFrames];

        pData->graph.create(fAudioInCount, fAudioOutCount, 0, 0);

        try {
            fAudio.startStream();
        }
        catch (const RtAudioError& e)
        {
            close();
            setLastError(e.what());
            return false;
        }

        patchbayRefresh(true, false, false);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            refreshExternalGraphPorts<PatchbayGraph>(pData->graph.getPatchbayGraph(), false, false);

        callback(true, true,
                 ENGINE_CALLBACK_ENGINE_STARTED,
                 0,
                 pData->options.processMode,
                 pData->options.transportMode,
                 static_cast<int>(pData->bufferSize),
                 static_cast<float>(pData->sampleRate),
                 getCurrentDriverName());
        return true;
    }

    bool close() override
    {
        carla_debug("CarlaEngineRtAudio::close()");

        bool hasError = false;

        // stop stream first
        if (fAudio.isStreamOpen() && fAudio.isStreamRunning())
        {
            try {
                fAudio.stopStream();
            }
            catch (const RtAudioError& e)
            {
                setLastError(e.what());
                hasError = true;
            }
        }

        // clear engine data
        CarlaEngine::close();

        pData->graph.destroy();

        for (LinkedList<MidiInPort>::Itenerator it = fMidiIns.begin2(); it.valid(); it.next())
        {
            static MidiInPort fallback = { nullptr, { '\0' } };

            MidiInPort& inPort(it.getValue(fallback));
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            inPort.port->cancelCallback();
            inPort.port->closePort();
            delete inPort.port;
        }

        fMidiIns.clear();
        fMidiInEvents.clear();

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it = fMidiOuts.begin2(); it.valid(); it.next())
        {
            static MidiOutPort fallback = { nullptr, { '\0' } };

            MidiOutPort& outPort(it.getValue(fallback));
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

        if (fAudioIntBufIn != nullptr)
        {
            delete[] fAudioIntBufIn;
            fAudioIntBufIn = nullptr;
        }

        if (fAudioIntBufOut != nullptr)
        {
            delete[] fAudioIntBufOut;
            fAudioIntBufOut = nullptr;
        }

        // close stream
        if (fAudio.isStreamOpen())
            fAudio.closeStream();

        return !hasError;
    }

    bool hasIdleOnMainThread() const noexcept override
    {
        return true;
    }

    bool isRunning() const noexcept override
    {
        return fAudio.isStreamOpen();
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
        return CARLA_BACKEND_NAMESPACE::getRtAudioApiName(fAudio.getCurrentApi());
    }

    // -------------------------------------------------------------------
    // Patchbay

    template<class Graph>
    bool refreshExternalGraphPorts(Graph* const graph, const bool sendHost, const bool sendOSC)
    {
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        char strBuf[STR_MAX+1U];
        strBuf[STR_MAX] = '\0';

        ExternalGraph& extGraph(graph->extGraph);

        // ---------------------------------------------------------------
        // clear last ports

        extGraph.clear();

        // ---------------------------------------------------------------
        // fill in new ones

        // Audio In
        for (uint i=0; i < fAudioInCount; ++i)
        {
            std::snprintf(strBuf, STR_MAX, "capture_%i", i+1);

            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioIn, i+1, strBuf, "");

            extGraph.audioPorts.ins.append(portNameToId);
        }

        // Audio Out
        for (uint i=0; i < fAudioOutCount; ++i)
        {
            std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);

            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioOut, i+1, strBuf, "");

            extGraph.audioPorts.outs.append(portNameToId);
        }

        // MIDI In
        try
        {
            RtMidiIn midiIn(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), "carla-discovery-in");

            for (uint i=0, count = midiIn.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.setData(kExternalGraphGroupMidiIn, i+1, midiIn.getPortName(i).c_str(), "");

                extGraph.midiPorts.ins.append(portNameToId);
            }
        } CARLA_SAFE_EXCEPTION("RtMidiIn discovery");

        // MIDI Out
        try
        {
            RtMidiOut midiOut(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), "carla-discovery-out");

            for (uint i=0, count = midiOut.getPortCount(); i < count; ++i)
            {
                PortNameToId portNameToId;
                portNameToId.setData(kExternalGraphGroupMidiOut, i+1, midiOut.getPortName(i).c_str(), "");

                extGraph.midiPorts.outs.append(portNameToId);
            }
        } CARLA_SAFE_EXCEPTION("RtMidiOut discovery");

        // ---------------------------------------------------------------
        // now refresh

        if (sendHost || sendOSC)
            graph->refresh(sendHost, sendOSC, true, fDeviceName.buffer());

        // ---------------------------------------------------------------
        // add midi connections

        for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin2(); it.valid(); it.next())
        {
            static const MidiInPort fallback = { nullptr, { '\0' } };

            const MidiInPort& inPort(it.getValue(fallback));
            CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

            const uint portId = extGraph.midiPorts.getPortIdFromName(true, inPort.name);
            CARLA_SAFE_ASSERT_UINT_CONTINUE(portId < extGraph.midiPorts.ins.count(), portId);

            ConnectionToId connectionToId;
            connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupMidiIn, portId, kExternalGraphGroupCarla, kExternalGraphCarlaPortMidiIn);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            extGraph.connections.list.append(connectionToId);

            callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                      connectionToId.id,
                      0, 0, 0, 0.0f,
                      strBuf);
        }

        fMidiOutMutex.lock();

        for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin2(); it.valid(); it.next())
        {
            static const MidiOutPort fallback = { nullptr, { '\0' } };

            const MidiOutPort& outPort(it.getValue(fallback));
            CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

            const uint portId = extGraph.midiPorts.getPortIdFromName(false, outPort.name);
            CARLA_SAFE_ASSERT_UINT_CONTINUE(portId < extGraph.midiPorts.outs.count(), portId);

            ConnectionToId connectionToId;
            connectionToId.setData(++(extGraph.connections.lastId), kExternalGraphGroupCarla, kExternalGraphCarlaPortMidiOut, kExternalGraphGroupMidiOut, portId);

            std::snprintf(strBuf, STR_MAX, "%i:%i:%i:%i", connectionToId.groupA, connectionToId.portA, connectionToId.groupB, connectionToId.portB);

            extGraph.connections.list.append(connectionToId);

            callback(sendHost, sendOSC,
                      ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED,
                      connectionToId.id,
                      0, 0, 0, 0.0f,
                      strBuf);
        }

        fMidiOutMutex.unlock();

        return true;
    }

    bool patchbayRefresh(const bool sendHost, const bool sendOSC, const bool external) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);

        if (pData->options.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
            return refreshExternalGraphPorts<RackGraph>(pData->graph.getRackGraph(), sendHost, sendOSC);

        if (sendHost)
            pData->graph.setUsingExternalHost(external);
        if (sendOSC)
            pData->graph.setUsingExternalOSC(external);

        if (external)
            return refreshExternalGraphPorts<PatchbayGraph>(pData->graph.getPatchbayGraph(), sendHost, sendOSC);

        return CarlaEngine::patchbayRefresh(sendHost, sendOSC, false);
    }

    // -------------------------------------------------------------------

protected:
    void handleAudioProcessCallback(void* outputBuffer, void* inputBuffer,
                                    uint nframes, double streamTime, RtAudioStreamStatus status)
    {
        const PendingRtEventsRunner prt(this, nframes, true);

        if (status & RTAUDIO_INPUT_OVERFLOW)
            ++pData->xruns;
        if (status & RTAUDIO_OUTPUT_UNDERFLOW)
            ++pData->xruns;

        // get buffers from RtAudio
        const float* const insPtr  = (const float*)inputBuffer;
        /* */ float* const outsPtr =       (float*)outputBuffer;

        // assert rtaudio buffers
        CARLA_SAFE_ASSERT_RETURN(outputBuffer != nullptr,);

        // set rtaudio buffers as non-interleaved
        const float* inBuf[fAudioInCount];
        /* */ float* outBuf[fAudioOutCount];

        if (fAudioInterleaved)
        {
            // FIXME - this looks completely wrong!
            float* inBuf2[fAudioInCount];

            for (uint i=0, count=fAudioInCount; i<count; ++i)
            {
                inBuf [i] = fAudioIntBufIn + (nframes*i);
                inBuf2[i] = fAudioIntBufIn + (nframes*i);
            }
            for (uint i=0, count=fAudioOutCount; i<count; ++i)
                outBuf[i] = fAudioIntBufOut + (nframes*i);

            // init input
            for (uint i=0; i<nframes; ++i)
                for (uint j=0; j<fAudioInCount; ++j)
                    inBuf2[j][i] = insPtr[i*fAudioInCount+j];

            // clear output
            carla_zeroFloats(fAudioIntBufOut, fAudioOutCount*nframes);
        }
        else
        {
            for (uint i=0; i < fAudioInCount; ++i)
                inBuf[i] = insPtr+(nframes*i);
            for (uint i=0; i < fAudioOutCount; ++i)
                outBuf[i] = outsPtr+(nframes*i);

            // clear output
            carla_zeroFloats(outsPtr, nframes*fAudioOutCount);
        }

        // initialize events
        carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);
        carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);

        if (fMidiInEvents.mutex.tryLock())
        {
            uint32_t engineEventIndex = 0;
            fMidiInEvents.splice();

            for (LinkedList<RtMidiEvent>::Itenerator it = fMidiInEvents.data.begin2(); it.valid(); it.next())
            {
                static const RtMidiEvent fallback = { 0, 0, { 0 } };

                const RtMidiEvent& midiEvent(it.getValue(fallback));
                CARLA_SAFE_ASSERT_CONTINUE(midiEvent.size > 0);

                EngineEvent& engineEvent(pData->events.in[engineEventIndex++]);

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

                engineEvent.fillFromMidiData(midiEvent.size, midiEvent.data, 0);

                if (engineEventIndex >= kMaxEngineEventInternalCount)
                    break;
            }

            fMidiInEvents.data.clear();
            fMidiInEvents.mutex.unlock();
        }

        pData->graph.process(pData, inBuf, outBuf, nframes);

        fMidiOutMutex.lock();

        if (fMidiOuts.count() > 0)
        {
            uint8_t size     = 0;
            uint8_t mdata[3] = { 0, 0, 0 };
            uint8_t mdataTmp[EngineMidiEvent::kDataSize];
            const uint8_t* mdataPtr;

            for (ushort i=0; i < kMaxEngineEventInternalCount; ++i)
            {
                const EngineEvent& engineEvent(pData->events.out[i]);

                /**/ if (engineEvent.type == kEngineEventTypeNull)
                {
                    break;
                }
                else if (engineEvent.type == kEngineEventTypeControl)
                {
                    const EngineControlEvent& ctrlEvent(engineEvent.ctrl);

                    size = ctrlEvent.convertToMidiData(engineEvent.channel, mdata);
                    mdataPtr = mdata;
                }
                else if (engineEvent.type == kEngineEventTypeMidi)
                {
                    const EngineMidiEvent& midiEvent(engineEvent.midi);

                    size = midiEvent.size;
                    CARLA_SAFE_ASSERT_CONTINUE(size > 0);

                    if (size > EngineMidiEvent::kDataSize)
                    {
                        CARLA_SAFE_ASSERT_CONTINUE(midiEvent.dataExt != nullptr);
                        mdataPtr = midiEvent.dataExt;
                    }
                    else
                    {
                        // set first byte
                        mdataTmp[0] = static_cast<uint8_t>(midiEvent.data[0] | (engineEvent.channel & MIDI_CHANNEL_BIT));

                        // copy rest
                        carla_copy<uint8_t>(mdataTmp+1, midiEvent.data+1, size-1U);

                        // done
                        mdataPtr = mdataTmp;
                    }
                }
                else
                {
                    continue;
                }

                if (size > 0)
                {
                    fMidiOutVector.assign(mdataPtr, mdataPtr + size);

                    for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin2(); it.valid(); it.next())
                    {
                        static MidiOutPort fallback = { nullptr, { '\0' } };

                        MidiOutPort& outPort(it.getValue(fallback));
                        CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

                        outPort.port->sendMessage(&fMidiOutVector);
                    }
                }
            }
        }

        fMidiOutMutex.unlock();

        if (fAudioInterleaved)
        {
            for (uint i=0; i < nframes; ++i)
                for (uint j=0; j<fAudioOutCount; ++j)
                    outsPtr[i*fAudioOutCount+j] = outBuf[j][i];
        }

        return; // unused
        (void)streamTime;
    }

    void handleBufferSizeCallback(const uint newBufferSize)
    {
        carla_stdout("bufferSize callback %u %u", pData->bufferSize, newBufferSize);
        if (pData->bufferSize == newBufferSize)
            return;

        if (fAudioInCount > 0)
        {
            delete[] fAudioIntBufIn;
            fAudioIntBufIn = new float[fAudioInCount*newBufferSize];
        }

        if (fAudioOutCount > 0)
        {
            delete[] fAudioIntBufOut;
            fAudioIntBufOut = new float[fAudioOutCount*newBufferSize];
        }

        pData->bufferSize = newBufferSize;
        bufferSizeChanged(newBufferSize);
    }

    void handleMidiCallback(double timeStamp, std::vector<uchar>* const message)
    {
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

    bool connectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
        carla_debug("CarlaEngineRtAudio::connectExternalGraphPort(%u, %u, \"%s\")", connectionType, portId, portName);

        switch (connectionType)
        {
        case kExternalGraphConnectionAudioIn1:
        case kExternalGraphConnectionAudioIn2:
        case kExternalGraphConnectionAudioOut1:
        case kExternalGraphConnectionAudioOut2:
            return CarlaEngine::connectExternalGraphPort(connectionType, portId, portName);

        case kExternalGraphConnectionMidiInput: {
            String newRtMidiPortName;
            newRtMidiPortName += getName();
            newRtMidiPortName += ":";
            newRtMidiPortName += portName;

            RtMidiIn* rtMidiIn;

            try {
                rtMidiIn = new RtMidiIn(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), newRtMidiPortName.buffer(), 512);
            } CARLA_SAFE_EXCEPTION_RETURN("new RtMidiIn", false);

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
        }   break;

        case kExternalGraphConnectionMidiOutput: {
            String newRtMidiPortName;
            newRtMidiPortName += getName();
            newRtMidiPortName += ":";
            newRtMidiPortName += portName;

            RtMidiOut* rtMidiOut;

            try {
                rtMidiOut = new RtMidiOut(getMatchedAudioMidiAPI(fAudio.getCurrentApi()), newRtMidiPortName.buffer());
            } CARLA_SAFE_EXCEPTION_RETURN("new RtMidiOut", false);

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
        }   break;
        }

        return false;
    }

    bool disconnectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
        carla_debug("CarlaEngineRtAudio::disconnectExternalGraphPort(%u, %u, \"%s\")", connectionType, portId, portName);

        switch (connectionType)
        {
        case kExternalGraphConnectionAudioIn1:
        case kExternalGraphConnectionAudioIn2:
        case kExternalGraphConnectionAudioOut1:
        case kExternalGraphConnectionAudioOut2:
            return CarlaEngine::disconnectExternalGraphPort(connectionType, portId, portName);

        case kExternalGraphConnectionMidiInput:
            for (LinkedList<MidiInPort>::Itenerator it=fMidiIns.begin2(); it.valid(); it.next())
            {
                static MidiInPort fallback = { nullptr, { '\0' } };

                MidiInPort& inPort(it.getValue(fallback));
                CARLA_SAFE_ASSERT_CONTINUE(inPort.port != nullptr);

                if (std::strncmp(inPort.name, portName, STR_MAX) != 0)
                    continue;

                inPort.port->cancelCallback();
                inPort.port->closePort();
                delete inPort.port;

                fMidiIns.remove(it);
                return true;
            }
            break;

        case kExternalGraphConnectionMidiOutput: {
            const CarlaMutexLocker cml(fMidiOutMutex);

            for (LinkedList<MidiOutPort>::Itenerator it=fMidiOuts.begin2(); it.valid(); it.next())
            {
                static MidiOutPort fallback = { nullptr, { '\0' } };

                MidiOutPort& outPort(it.getValue(fallback));
                CARLA_SAFE_ASSERT_CONTINUE(outPort.port != nullptr);

                if (std::strncmp(outPort.name, portName, STR_MAX) != 0)
                    continue;

                outPort.port->closePort();
                delete outPort.port;

                fMidiOuts.remove(it);
                return true;
            }
        }   break;
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    RtAudio fAudio;

    // useful info
    bool fAudioInterleaved;
    uint fAudioInCount;
    uint fAudioOutCount;
    uint64_t fLastEventTime;

    // current device name
    String fDeviceName;

    // temp buffer for interleaved audio
    float* fAudioIntBufIn;
    float* fAudioIntBufOut;

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
            : mutex(),
              dataPool("RtMidiEvents", 512, 512),
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
            if (dataPending.count() > 0)
                dataPending.moveTo(data, true /* append */);
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

    static bool carla_rtaudio_buffer_size_callback(unsigned int bufferSize, void* userData)
    {
        handlePtr->handleBufferSizeCallback(bufferSize);
        return true;
    }

    static void carla_rtmidi_callback(double timeStamp, std::vector<uchar>* message, void* userData)
    {
        handlePtr->handleMidiCallback(timeStamp, message);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineRtAudio)
};

// -----------------------------------------

namespace EngineInit {

CarlaEngine* newRtAudio(const AudioApi api)
{
    initRtAudioAPIsIfNeeded();

    RtAudio::Api rtApi = RtAudio::UNSPECIFIED;

    switch (api)
    {
    case AUDIO_API_NULL:
        rtApi = RtAudio::RTAUDIO_DUMMY;
        break;
    case AUDIO_API_JACK:
        rtApi = RtAudio::UNIX_JACK;
        break;
    case AUDIO_API_OSS:
        rtApi = RtAudio::LINUX_OSS;
        break;
    case AUDIO_API_ALSA:
        rtApi = RtAudio::LINUX_ALSA;
        break;
    case AUDIO_API_PULSEAUDIO:
        rtApi = RtAudio::UNIX_PULSE;
        break;
    case AUDIO_API_COREAUDIO:
        rtApi = RtAudio::MACOSX_CORE;
        break;
    case AUDIO_API_ASIO:
        rtApi = RtAudio::WINDOWS_ASIO;
        break;
    case AUDIO_API_DIRECTSOUND:
        rtApi = RtAudio::WINDOWS_DS;
        break;
    case AUDIO_API_WASAPI:
        rtApi = RtAudio::WINDOWS_WASAPI;
        break;
    }

    return new CarlaEngineRtAudio(rtApi);
}

uint getRtAudioApiCount()
{
    initRtAudioAPIsIfNeeded();

    return static_cast<uint>(gRtAudioApis.size());
}

const char* getRtAudioApiName(const uint index)
{
    initRtAudioAPIsIfNeeded();

    CARLA_SAFE_ASSERT_RETURN(index < gRtAudioApis.size(), nullptr);

    return CARLA_BACKEND_NAMESPACE::getRtAudioApiName(gRtAudioApis[index]);
}

const char* const* getRtAudioApiDeviceNames(const uint index)
{
    initRtAudioAPIsIfNeeded();

    if (index >= gRtAudioApis.size())
        return nullptr;

    const RtAudio::Api& api(gRtAudioApis[index]);
    CarlaStringList devNames;

    try {
        RtAudio rtAudio(api);

        const uint devCount(rtAudio.getDeviceCount());

        if (devCount == 0)
            return nullptr;

        for (uint i=0; i < devCount; ++i)
        {
            RtAudio::DeviceInfo devInfo(rtAudio.getDeviceInfo(i));

            if (devInfo.probed && devInfo.outputChannels > 0 /*&& (devInfo.nativeFormats & RTAUDIO_FLOAT32) != 0*/)
                devNames.append(devInfo.name.c_str());
        }

    } CARLA_SAFE_EXCEPTION_RETURN("RtAudio device names", nullptr);

    gDeviceNames = devNames.toCharStringListPtr();

    return gDeviceNames;
}

const EngineDriverDeviceInfo* getRtAudioDeviceInfo(const uint index, const char* const deviceName)
{
    initRtAudioAPIsIfNeeded();

    if (index >= gRtAudioApis.size())
        return nullptr;

    static EngineDriverDeviceInfo devInfo = { 0x0, nullptr, nullptr };
    static uint32_t dummyBufferSizes[]    = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 0 };
    static double   dummySampleRates[]    = { 22050.0, 32000.0, 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0, 0.0 };

    // reset
    devInfo.hints = 0x0;

    // cleanup
    if (devInfo.bufferSizes != nullptr && devInfo.bufferSizes != dummyBufferSizes)
    {
        delete[] devInfo.bufferSizes;
        devInfo.bufferSizes = nullptr;
    }
    if (devInfo.sampleRates != nullptr && devInfo.sampleRates != dummySampleRates)
    {
        delete[] devInfo.sampleRates;
        devInfo.sampleRates = nullptr;
    }

    const RtAudio::Api& api(gRtAudioApis[index]);

    if (api == RtAudio::UNIX_JACK)
    {
        devInfo.bufferSizes = nullptr;
        devInfo.sampleRates = nullptr;
        return &devInfo;
    }

    RtAudio::DeviceInfo rtAudioDevInfo;

    try {
        RtAudio rtAudio(api);

        const uint devCount(rtAudio.getDeviceCount());

        if (devCount == 0)
            return nullptr;

        uint i;
        for (i=0; i < devCount; ++i)
        {
            rtAudioDevInfo = rtAudio.getDeviceInfo(i);

            if (rtAudioDevInfo.name == deviceName)
                break;
        }

        if (i == devCount)
            rtAudioDevInfo = rtAudio.getDeviceInfo(rtAudio.getDefaultOutputDevice());

    } CARLA_SAFE_EXCEPTION_RETURN("RtAudio device discovery", nullptr);

    // a few APIs can do triple buffer
    switch (api)
    {
    case RtAudio::LINUX_ALSA:
    case RtAudio::LINUX_OSS:
    case RtAudio::WINDOWS_DS:
        devInfo.hints |= ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER;
        break;
    default:
        break;
    }

    // always use default buffer sizes
    devInfo.bufferSizes = dummyBufferSizes;

    // valid sample rates
    if (const size_t sampleRatesCount = rtAudioDevInfo.sampleRates.size())
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

}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
