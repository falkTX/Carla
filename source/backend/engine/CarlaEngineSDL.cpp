// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaEngineGraph.hpp"
#include "CarlaEngineInit.hpp"
#include "CarlaEngineInternal.hpp"
#include "CarlaStringList.hpp"
#include "CarlaBackendUtils.hpp"

#include <SDL.h>

#ifndef HAVE_SDL2
typedef Uint32 SDL_AudioDeviceID;
#endif

#ifndef SDL_HINT_AUDIO_DEVICE_APP_NAME
# define SDL_HINT_AUDIO_DEVICE_APP_NAME "SDL_AUDIO_DEVICE_APP_NAME"
#endif

#ifndef SDL_HINT_AUDIO_DEVICE_STREAM_NAME
# define SDL_HINT_AUDIO_DEVICE_STREAM_NAME "SDL_AUDIO_DEVICE_STREAM_NAME"
#endif

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Global static data

static CarlaStringList gDeviceNames;

// -------------------------------------------------------------------------------------------------------------------

static void initAudioDevicesIfNeeded()
{
    static bool needsInit = true;

    if (! needsInit)
        return;

    needsInit = false;

#ifdef HAVE_SDL2
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    const int numDevices = SDL_GetNumAudioDevices(0);

    for (int i=0; i<numDevices; ++i)
        gDeviceNames.append(SDL_GetAudioDeviceName(i, 0));
#else
    SDL_Init(SDL_INIT_AUDIO);
#endif
}

// -------------------------------------------------------------------------------------------------------------------
// RtAudio Engine

class CarlaEngineSDL : public CarlaEngine
{
public:
    CarlaEngineSDL()
        : CarlaEngine(),
          fDeviceId(0),
          fDeviceName(),
          fAudioOutCount(0),
          fAudioIntBufOut(nullptr)
    {
        carla_debug("CarlaEngineSDL::CarlaEngineSDL()");

        // just to make sure
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
    }

    ~CarlaEngineSDL() override
    {
        CARLA_SAFE_ASSERT(fAudioOutCount == 0);
        carla_debug("CarlaEngineSDL::~CarlaEngineSDL()");
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDeviceId == 0, false);
        CARLA_SAFE_ASSERT_RETURN(fAudioOutCount == 0, false);
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineSDL::init(\"%s\")", clientName);

        if (pData->options.processMode != ENGINE_PROCESS_MODE_CONTINUOUS_RACK && pData->options.processMode != ENGINE_PROCESS_MODE_PATCHBAY)
        {
            setLastError("Invalid process mode");
            return false;
        }

        SDL_AudioSpec requested, received;
        carla_zeroStruct(requested);
#ifdef HAVE_SDL2
        requested.format = AUDIO_F32SYS;
#else
        requested.format = AUDIO_S16SYS;
#endif
        requested.channels = 2;
        requested.freq = static_cast<int>(pData->options.audioSampleRate);
        requested.samples = static_cast<Uint16>(pData->options.audioBufferSize);
        requested.callback = carla_sdl_process_callback;
        requested.userdata = this;

#ifdef HAVE_SDL2
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, clientName);
        // SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, );
        SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "2");

        const char* const deviceName = pData->options.audioDevice != nullptr && pData->options.audioDevice[0] != '\0'
                                     ? pData->options.audioDevice
                                     : nullptr;

        int flags = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE;
        if (pData->options.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
            flags |= SDL_AUDIO_ALLOW_CHANNELS_CHANGE;

        fDeviceId = SDL_OpenAudioDevice(deviceName, 0, &requested, &received, flags);
#else
        fDeviceId = SDL_OpenAudio(&requested, &received) == 0 ? 1 : 0;
#endif

        if (fDeviceId == 0)
        {
            setLastError(SDL_GetError());
            return false;
        }

        if (received.channels == 0)
        {
#ifdef HAVE_SDL2
            SDL_CloseAudioDevice(fDeviceId);
#else
            SDL_CloseAudio();
#endif
            fDeviceId = 0;
            setLastError("No output channels available");
            return false;
        }

        if (! pData->init(clientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        pData->bufferSize = received.samples;
        pData->sampleRate = received.freq;
        pData->initTime(pData->options.transportExtra);

        fAudioOutCount = received.channels;
        fAudioIntBufOut = new float*[fAudioOutCount];
        for (uint i=0; i<fAudioOutCount; ++i)
            fAudioIntBufOut[i] = new float[received.samples];

        pData->graph.create(0, fAudioOutCount, 0, 0);

#ifdef HAVE_SDL2
        SDL_PauseAudioDevice(fDeviceId, 0);
#else
        SDL_PauseAudio(0);
#endif
        carla_stdout("open fAudioOutCount %d %d %d | %d vs %d",
                     fAudioOutCount, received.samples, received.freq,
                     received.format, requested.format);

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
        carla_debug("CarlaEngineSDL::close()");

        // close device
        if (fDeviceId != 0)
        {
            // SDL_PauseAudioDevice(fDeviceId, 1);
#ifdef HAVE_SDL2
            SDL_CloseAudioDevice(fDeviceId);
#else
            SDL_CloseAudio();
#endif
            fDeviceId = 0;
        }

        // clear engine data
        CarlaEngine::close();

        pData->graph.destroy();

        // cleanup
        if (fAudioIntBufOut != nullptr)
        {
            for (uint i=0; i<fAudioOutCount; ++i)
                delete[] fAudioIntBufOut[i];
            delete[] fAudioIntBufOut;
            fAudioIntBufOut = nullptr;
        }

        fAudioOutCount = 0;
        fDeviceName.clear();

        return false;
    }

    bool hasIdleOnMainThread() const noexcept override
    {
        return true;
    }

    bool isRunning() const noexcept override
    {
        return fDeviceId != 0 /*&& SDL_GetAudioDeviceStatus(fDeviceId) == SDL_AUDIO_PLAYING*/;
    }

    bool isOffline() const noexcept override
    {
        return false;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeSDL;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "SDL";
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

        // Audio Out
        for (uint i=0; i < fAudioOutCount; ++i)
        {
            std::snprintf(strBuf, STR_MAX, "playback_%i", i+1);

            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioOut, i+1, strBuf, "");

            extGraph.audioPorts.outs.append(portNameToId);
        }

        // ---------------------------------------------------------------
        // now refresh

        if (sendHost || sendOSC)
            graph->refresh(sendHost, sendOSC, true, fDeviceName.buffer());

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
    void handleAudioProcessCallback(uchar* const stream, const int len)
    {
        // safety checks
        CARLA_SAFE_ASSERT_RETURN(stream != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(len > 0,);

#ifdef HAVE_SDL2
        // direct float type
        float* const fstream = (float*)stream;
        const uint ulen = static_cast<uint>(static_cast<uint>(len) / sizeof(float) / fAudioOutCount);
#else
        // signed 16bit int
        int16_t* const istream = (int16_t*)stream;
        const uint ulen = static_cast<uint>(static_cast<uint>(len) / sizeof(int16_t) / fAudioOutCount);
#endif

        const PendingRtEventsRunner prt(this, ulen, true);

        // init our deinterleaved audio buffers
        for (uint i=0, count=fAudioOutCount; i<count; ++i)
            carla_zeroFloats(fAudioIntBufOut[i], ulen);

        // initialize events
        carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);
        carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);

        pData->graph.process(pData, nullptr, fAudioIntBufOut, ulen);

        // interleave audio back
        for (uint i=0; i < fAudioOutCount; ++i)
        {
            for (uint j=0; j < ulen; ++j)
            {
#ifdef HAVE_SDL2
                // direct float type
                fstream[j * fAudioOutCount + i] = fAudioIntBufOut[i][j];
#else
                // signed 16bit int
                istream[j * fAudioOutCount + i] = lrintf(carla_fixedValue(-1.0f, 1.0f, fAudioIntBufOut[i][j]) * 32767.0f);
#endif
            }
        }
    }

    // -------------------------------------------------------------------

    bool connectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
        carla_debug("CarlaEngineSDL::connectExternalGraphPort(%u, %u, \"%s\")", connectionType, portId, portName);

        switch (connectionType)
        {
        case kExternalGraphConnectionAudioIn1:
        case kExternalGraphConnectionAudioIn2:
        case kExternalGraphConnectionAudioOut1:
        case kExternalGraphConnectionAudioOut2:
            return CarlaEngine::connectExternalGraphPort(connectionType, portId, portName);

        case kExternalGraphConnectionMidiInput:
        case kExternalGraphConnectionMidiOutput:
            break;
        }

        return false;
    }

    bool disconnectExternalGraphPort(const uint connectionType, const uint portId, const char* const portName) override
    {
        CARLA_SAFE_ASSERT_RETURN(connectionType != 0 || (portName != nullptr && portName[0] != '\0'), false);
        carla_debug("CarlaEngineSDL::disconnectExternalGraphPort(%u, %u, \"%s\")", connectionType, portId, portName);

        switch (connectionType)
        {
        case kExternalGraphConnectionAudioIn1:
        case kExternalGraphConnectionAudioIn2:
        case kExternalGraphConnectionAudioOut1:
        case kExternalGraphConnectionAudioOut2:
            return CarlaEngine::disconnectExternalGraphPort(connectionType, portId, portName);

        case kExternalGraphConnectionMidiInput:
        case kExternalGraphConnectionMidiOutput:
            break;
        }

        return false;
    }

    // -------------------------------------------------------------------

private:
    SDL_AudioDeviceID fDeviceId;

    // current device name
    String fDeviceName;

    // deinterleaved buffers
    uint fAudioOutCount;
    float** fAudioIntBufOut;

    #define handlePtr ((CarlaEngineSDL*)userData)

    static void carla_sdl_process_callback(void* userData, uchar* stream, int len)
    {
        handlePtr->handleAudioProcessCallback(stream, len);
    }

    #undef handlePtr

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineSDL)
};

// -----------------------------------------

namespace EngineInit {

CarlaEngine* newSDL()
{
    initAudioDevicesIfNeeded();
    return new CarlaEngineSDL();
}

const char* const* getSDLDeviceNames()
{
    initAudioDevicesIfNeeded();

    if (gDeviceNames.count() == 0)
    {
        static const char* deviceNames[] = { "Default", nullptr };
        return deviceNames;
    }

    static const CharStringListPtr deviceNames = gDeviceNames.toCharStringListPtr();
    return deviceNames;
}

}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
