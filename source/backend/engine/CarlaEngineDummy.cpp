/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineGraph.hpp"
#include "CarlaEngineInit.hpp"
#include "CarlaEngineInternal.hpp"

#include <ctime>
#include <sys/time.h>

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------
// Dummy Engine

class CarlaEngineDummy : public CarlaEngine,
                         public CarlaThread
{
public:
    CarlaEngineDummy()
        : CarlaEngine(),
          CarlaThread("CarlaEngineDummy"),
          fRunning(false)
    {
        carla_debug("CarlaEngineDummy::CarlaEngineDummy()");

        // just to make sure
        pData->options.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
    }

    ~CarlaEngineDummy() override
    {
        carla_debug("CarlaEngineDummy::~CarlaEngineDummy()");
    }

    // -------------------------------------

    bool init(const char* const clientName) override
    {
        CARLA_SAFE_ASSERT_RETURN(clientName != nullptr && clientName[0] != '\0', false);
        carla_debug("CarlaEngineDummy::init(\"%s\")", clientName);

        if (pData->options.processMode != ENGINE_PROCESS_MODE_CONTINUOUS_RACK)
        {
            setLastError("Invalid process mode");
            return false;
        }

        fRunning = true;

        if (! pData->init(clientName))
        {
            close();
            setLastError("Failed to init internal data");
            return false;
        }

        pData->bufferSize = pData->options.audioBufferSize;
        pData->sampleRate = pData->options.audioSampleRate;
        pData->initTime(pData->options.transportExtra);

        pData->graph.create(2, 2, 0, 0);

        if (! startThread(true))
        {
            close();
            setLastError("Failed to start dummy audio thread");
            return false;
        }

        patchbayRefresh(true, false, false);

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
        carla_debug("CarlaEngineDummy::close()");

        fRunning = false;
        stopThread(-1);
        CarlaEngine::close();

        pData->graph.destroy();
        return true;
    }

    bool isRunning() const noexcept override
    {
        return fRunning;
    }

    bool isOffline() const noexcept override
    {
        return false;
    }

    EngineType getType() const noexcept override
    {
        return kEngineTypeDummy;
    }

    const char* getCurrentDriverName() const noexcept override
    {
        return "Dummy";
    }

    // -------------------------------------------------------------------
    // Patchbay

    bool patchbayRefresh(const bool sendHost, const bool sendOSC, const bool) override
    {
        CARLA_SAFE_ASSERT_RETURN(pData->graph.isReady(), false);

        RackGraph* const graph = pData->graph.getRackGraph();
        CARLA_SAFE_ASSERT_RETURN(graph != nullptr, false);

        ExternalGraph& extGraph(graph->extGraph);

        // ---------------------------------------------------------------
        // clear last ports

        extGraph.clear();

        // ---------------------------------------------------------------
        // fill in new ones

        {
            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioIn, 1, "capture_1", "");

            extGraph.audioPorts.ins.append(portNameToId);
        }

        {
            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioIn, 2, "capture_2", "");

            extGraph.audioPorts.ins.append(portNameToId);
        }

        {
            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioOut, 1, "playback_1", "");

            extGraph.audioPorts.outs.append(portNameToId);
        }

        {
            PortNameToId portNameToId;
            portNameToId.setData(kExternalGraphGroupAudioOut, 2, "playback_2", "");

            extGraph.audioPorts.outs.append(portNameToId);
        }

        // ---------------------------------------------------------------
        // now refresh

        if (sendHost || sendOSC)
            graph->refresh(sendHost, sendOSC, false, "Dummy");

        return true;
    }

    // -------------------------------------------------------------------

protected:
    static int64_t getTimeInMicroseconds() noexcept
    {
    #if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
        struct timeval tv;
        gettimeofday(&tv, nullptr);

        return (tv.tv_sec * 1000000) + tv.tv_usec;
    #else
        struct timespec ts;
    # ifdef CLOCK_MONOTONIC_RAW
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    # else
        clock_gettime(CLOCK_MONOTONIC, &ts);
    # endif

        return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
    #endif
    }

    void run() override
    {
        const uint32_t bufferSize = pData->bufferSize;
        const int64_t cycleTime = static_cast<int64_t>(
            static_cast<double>(bufferSize) / pData->sampleRate * 1000000 + 0.5);

        carla_stdout("CarlaEngineDummy audio thread started, cycle time: " P_INT64 "ms", cycleTime / 1000);

        float* audioIns[2] = {
            (float*)std::malloc(sizeof(float)*bufferSize),
            (float*)std::malloc(sizeof(float)*bufferSize),
        };
        CARLA_SAFE_ASSERT_RETURN(audioIns[0] != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(audioIns[1] != nullptr,);

        float* audioOuts[2] = {
            (float*)std::malloc(sizeof(float)*bufferSize),
            (float*)std::malloc(sizeof(float)*bufferSize),
        };
        CARLA_SAFE_ASSERT_RETURN(audioOuts[0] != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(audioOuts[1] != nullptr,);

        carla_zeroFloats(audioIns[0], bufferSize);
        carla_zeroFloats(audioIns[1], bufferSize);
        carla_zeroStructs(pData->events.in,  kMaxEngineEventInternalCount);

        int64_t oldTime, newTime;

        while (! shouldThreadExit())
        {
            oldTime = getTimeInMicroseconds();

            const PendingRtEventsRunner prt(this, bufferSize, true);

            carla_zeroFloats(audioOuts[0], bufferSize);
            carla_zeroFloats(audioOuts[1], bufferSize);
            carla_zeroStructs(pData->events.out, kMaxEngineEventInternalCount);

            pData->graph.process(pData, audioIns, audioOuts, bufferSize);

            newTime = getTimeInMicroseconds();
            CARLA_SAFE_ASSERT_CONTINUE(newTime >= oldTime);

            const int64_t remainingTime = cycleTime - (newTime - oldTime);

            if (remainingTime <= 0)
            {
                ++pData->xruns;
                carla_stdout("XRUN! remaining time: " P_INT64 ", old: " P_INT64 ", new: " P_INT64 ")",
                             remainingTime, oldTime, newTime);
            }
            else if (remainingTime >= 1000)
            {
                CARLA_SAFE_ASSERT_CONTINUE(remainingTime < 1000000); // 1 sec
                carla_msleep(static_cast<uint>(remainingTime / 1000));
            }
        }

        std::free(audioIns[0]);
        std::free(audioIns[1]);
        std::free(audioOuts[0]);
        std::free(audioOuts[1]);

        carla_stdout("CarlaEngineDummy audio thread finished with %u Xruns", pData->xruns);
    }

    // -------------------------------------------------------------------

private:
    bool fRunning;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaEngineDummy)
};

// -----------------------------------------

namespace EngineInit {

CarlaEngine* newDummy()
{
    carla_debug("EngineInit::newDummy()");
    return new CarlaEngineDummy();
}

}

// -----------------------------------------

CARLA_BACKEND_END_NAMESPACE
