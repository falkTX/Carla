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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_ENGINE_INTERNAL_HPP_INCLUDED
#define CARLA_ENGINE_INTERNAL_HPP_INCLUDED

#include "CarlaEngineThread.hpp"
#include "CarlaEngineUtils.hpp"
#include "CarlaPlugin.hpp"
#include "LinkedList.hpp"

#ifndef BUILD_BRIDGE
# include "CarlaEngineOsc.hpp"
# include "hylia/hylia.h"
#endif

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
# include "water/processors/AudioProcessorGraph.h"
# include "water/containers/Array.h"
# include "water/memory/Atomic.h"
#endif

#include <vector>

// FIXME only use CARLA_PREVENT_HEAP_ALLOCATION for structs
// maybe separate macro

typedef struct _jack_position jack_position_t;
struct carla_sem_t;

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return false;   }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return nullptr; }

#define CARLA_SAFE_EXCEPTION_RETURN_ERR(excptMsg, errMsg)  catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); setLastError(errMsg); return false;   }
#define CARLA_SAFE_EXCEPTION_RETURN_ERRN(excptMsg, errMsg) catch(...) { carla_safe_exception(excptMsg, __FILE__, __LINE__); setLastError(errMsg); return nullptr; }

// -----------------------------------------------------------------------
// InternalEvents

struct EngineInternalEvents {
    EngineEvent* in;
    EngineEvent* out;

    EngineInternalEvents() noexcept;
    ~EngineInternalEvents() noexcept;
    void clear() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalEvents)
};

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
// -----------------------------------------------------------------------
// InternalGraph

struct RackGraph;
class PatchbayGraph;

class EngineInternalGraph
{
public:
    EngineInternalGraph(CarlaEngine* engine) noexcept;
    ~EngineInternalGraph() noexcept;

    void create(uint32_t audioIns, uint32_t audioOuts, uint32_t cvIns, uint32_t cvOuts);
    void destroy() noexcept;

    void setBufferSize(uint32_t bufferSize);
    void setSampleRate(double sampleRate);
    void setOffline(bool offline);

    bool isRack() const noexcept
    {
        return fIsRack;
    }

    bool isReady() const noexcept
    {
        return fIsReady;
    }

    uint32_t getNumAudioOuts() const noexcept
    {
        return fNumAudioOuts;
    }

    RackGraph*     getRackGraph() const noexcept;
    PatchbayGraph* getPatchbayGraph() const noexcept;
    PatchbayGraph* getPatchbayGraphOrNull() const noexcept;

    void process(CarlaEngine::ProtectedData* data, const float* const* inBuf, float* const* outBuf, uint32_t frames);

    // special direct process with connections already handled, used in JACK and Plugin
    void processRack(CarlaEngine::ProtectedData* data, const float* inBuf[2], float* outBuf[2], uint32_t frames);

    // used for internal patchbay mode
    void addPlugin(CarlaPluginPtr plugin);
    void replacePlugin(CarlaPluginPtr oldPlugin, CarlaPluginPtr newPlugin);
    void renamePlugin(CarlaPluginPtr plugin, const char* newName);
    void switchPlugins(CarlaPluginPtr pluginA, CarlaPluginPtr pluginB);
    void removePlugin(CarlaPluginPtr plugin);
    void removeAllPlugins();

    bool isUsingExternalHost() const noexcept;
    bool isUsingExternalOSC() const noexcept;
    void setUsingExternalHost(bool usingExternal) noexcept;
    void setUsingExternalOSC(bool usingExternal) noexcept;

private:
    bool fIsRack;
    uint32_t fNumAudioOuts;
    volatile bool fIsReady;

    union {
        RackGraph*     fRack;
        PatchbayGraph* fPatchbay;
    };

    CarlaEngine* const kEngine;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalGraph)
};
#endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

// -----------------------------------------------------------------------
// InternalTime

class EngineInternalTime {
public:
    EngineInternalTime(EngineTimeInfo& timeInfo, const EngineTransportMode& transportMode) noexcept;

    void init(uint32_t bufferSize, double sampleRate);
    void updateAudioValues(uint32_t bufferSize, double sampleRate);

    void enableLink(bool enable);
    void setBPM(double bpm);
    void setNeedsReset() noexcept;
    void pause() noexcept;
    void relocate(uint64_t frame) noexcept;

private:
    double beatsPerBar;
    double beatsPerMinute;
    double bufferSize;
    double sampleRate;
    bool needsReset;

    uint64_t nextFrame;

#ifndef BUILD_BRIDGE
    struct Hylia {
        bool enabled;
        hylia_t* instance;
        hylia_time_info_t timeInfo;

        Hylia();
        ~Hylia();
        CARLA_DECLARE_NON_COPY_STRUCT(Hylia)
    } hylia;
#endif

    EngineTimeInfo& timeInfo;
    const EngineTransportMode& transportMode;

    friend class PendingRtEventsRunner;
    void preProcess(uint32_t numFrames);
    void fillEngineTimeInfo(uint32_t newFrames) noexcept;

    friend class CarlaEngineJack;
    void fillJackTimeInfo(jack_position_t* pos, uint32_t newFrames) noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalTime)
};

// -----------------------------------------------------------------------
// EngineNextAction

enum EnginePostAction {
    kEnginePostActionNull = 0,
    kEnginePostActionZeroCount,    // set curPluginCount to 0
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    kEnginePostActionRemovePlugin, // remove a plugin
    kEnginePostActionSwitchPlugins // switch between 2 plugins
#endif
};

struct EngineNextAction {
    EnginePostAction opcode;
    uint pluginId;
    uint value;

    CarlaMutex mutex;

    bool needsPost;
    volatile bool postDone;
    carla_sem_t* sem;

    EngineNextAction() noexcept;
    ~EngineNextAction() noexcept;
    void clearAndReset() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineNextAction)
};

// -----------------------------------------------------------------------
// EnginePluginData

struct EnginePluginData {
    CarlaPluginPtr plugin;
    float peaks[4];

    EnginePluginData()
        : plugin(nullptr),
#ifdef CARLA_PROPER_CPP11_SUPPORT
          peaks{0.0f, 0.0f, 0.0f, 0.0f} {}
#else
          peaks()
    {
        carla_zeroStruct(peaks);
    }
#endif
};

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

struct CarlaEngine::ProtectedData {
    CarlaEngineThread thread;

#if defined(HAVE_LIBLO) && !defined(BUILD_BRIDGE)
    CarlaEngineOsc osc;
#endif

    EngineCallbackFunc callback;
    void*              callbackPtr;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

    bool actionCanceled;

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    bool loadingProject;
    bool ignoreClientPrefix; // backwards compat only
    CarlaString currentProjectFilename;
    CarlaString currentProjectFolder;
#endif

    uint32_t bufferSize;
    double   sampleRate;

    bool aboutToClose;    // don't re-activate thread if true
    int  isIdling;        // don't allow any operations while idling
    uint curPluginCount;  // number of plugins loaded (0...max)
    uint maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    uint nextPluginId;    // invalid if == maxPluginNumber

    CarlaMutex     envMutex;
    CarlaString    lastError;
    CarlaString    name;
    EngineOptions  options;
    EngineTimeInfo timeInfo;

#ifdef BUILD_BRIDGE_ALTERNATIVE_ARCH
    EnginePluginData plugins[1];
#else
    EnginePluginData* plugins;
    uint32_t xruns;
    float dspLoad;
#endif
    float peaks[4];
    std::vector<CarlaPluginPtr> pluginsToDelete;

    EngineInternalEvents events;
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    EngineInternalGraph  graph;
#endif
    EngineInternalTime   time;
    EngineNextAction     nextAction;

    // -------------------------------------------------------------------

    ProtectedData(CarlaEngine* engine);
    ~ProtectedData();

    // -------------------------------------------------------------------

    bool init(const char* clientName);
    void close();

    void initTime(const char* features);

    // -------------------------------------------------------------------

    void deletePluginsAsNeeded();

    // -------------------------------------------------------------------

    void doPluginRemove(uint pluginId) noexcept;
    void doPluginsSwitch(uint idA, uint idB) noexcept;
    void doNextPluginAction() noexcept;

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

// -----------------------------------------------------------------------

class PendingRtEventsRunner
{
public:
    PendingRtEventsRunner(CarlaEngine* engine,
                          uint32_t numFrames,
                          bool calcDSPLoad = false) noexcept;
    ~PendingRtEventsRunner() noexcept;

private:
    CarlaEngine::ProtectedData* const pData;
    int64_t prevTime;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(PendingRtEventsRunner)
};

// -----------------------------------------------------------------------

class ScopedActionLock
{
public:
    ScopedActionLock(CarlaEngine* engine, EnginePostAction action, uint pluginId, uint value) noexcept;
    ~ScopedActionLock() noexcept;

private:
    CarlaEngine::ProtectedData* const pData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ScopedActionLock)
};

// -----------------------------------------------------------------------

class ScopedThreadStopper
{
public:
    ScopedThreadStopper(CarlaEngine* engine) noexcept;
    ~ScopedThreadStopper() noexcept;

private:
    CarlaEngine* const engine;
    CarlaEngine::ProtectedData* const pData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ScopedThreadStopper)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
