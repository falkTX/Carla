/*
 * Carla Plugin Host
 * Copyright (C) 2011-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngineOsc.hpp"
#include "CarlaEngineThread.hpp"
#include "CarlaEngineUtils.hpp"

#include "hylia/hylia.h"

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

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// InternalGraph

struct RackGraph;
class PatchbayGraph;

class EngineInternalGraph
{
public:
    EngineInternalGraph(CarlaEngine* const engine) noexcept;
    ~EngineInternalGraph() noexcept;

    void create(const uint32_t inputs, const uint32_t outputs);
    void destroy() noexcept;

    void setBufferSize(const uint32_t bufferSize);
    void setSampleRate(const double sampleRate);
    void setOffline(const bool offline);

    bool isReady() const noexcept;

    RackGraph*     getRackGraph() const noexcept;
    PatchbayGraph* getPatchbayGraph() const noexcept;

    void process(CarlaEngine::ProtectedData* const data, const float* const* const inBuf, float* const* const outBuf, const uint32_t frames);

    // special direct process with connections already handled, used in JACK and Plugin
    void processRack(CarlaEngine::ProtectedData* const data, const float* inBuf[2], float* outBuf[2], const uint32_t frames);

    // used for internal patchbay mode
    void addPlugin(CarlaPlugin* const plugin);
    void replacePlugin(CarlaPlugin* const oldPlugin, CarlaPlugin* const newPlugin);
    void renamePlugin(CarlaPlugin* const plugin, const char* const newName);
    void removePlugin(CarlaPlugin* const plugin);
    void removeAllPlugins();

    bool isUsingExternal() const noexcept;
    void setUsingExternal(const bool usingExternal) noexcept;

private:
    bool fIsRack;
    bool fIsReady;

    union {
        RackGraph*     fRack;
        PatchbayGraph* fPatchbay;
    };

    CarlaEngine* const kEngine;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalGraph)
};
#endif

// -----------------------------------------------------------------------
// InternalTime

class EngineInternalTime {
public:
    EngineInternalTime(EngineTimeInfo& timeInfo, const EngineTransportMode& transportMode) noexcept;

    void init(const uint32_t bufferSize, double sampleRate);
    void updateAudioValues(const uint32_t bufferSize, const double sampleRate);

    void enableLink(const bool enable);
    void setBPM(const double bpm);
    void setNeedsReset() noexcept;
    void pause() noexcept;
    void relocate(const uint64_t frame) noexcept;

private:
    double beatsPerBar;
    double beatsPerMinute;
    double bufferSize;
    double sampleRate;
    double tick;
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
    void preProcess(const uint32_t numFrames);
    void fillEngineTimeInfo(const uint32_t newFrames) noexcept;

    friend class CarlaEngineJack;
    void fillJackTimeInfo(jack_position_t* const pos, const uint32_t newFrames) noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalTime)
};

// -----------------------------------------------------------------------
// EngineNextAction

enum EnginePostAction {
    kEnginePostActionNull = 0,
    kEnginePostActionZeroCount,    // set curPluginCount to 0
#ifndef BUILD_BRIDGE
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
    CarlaPlugin* plugin;
    float insPeak[2];
    float outsPeak[2];
};

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

struct CarlaEngine::ProtectedData {
    CarlaEngineThread thread;

#ifdef HAVE_LIBLO
    CarlaEngineOsc osc;
# ifdef BUILD_BRIDGE
    CarlaOscData* oscData;
# else
    const CarlaOscData* oscData;
# endif
#endif

    EngineCallbackFunc callback;
    void*              callbackPtr;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

#ifndef BUILD_BRIDGE
    // special hack for linuxsampler
    bool firstLinuxSamplerInstance;
    bool loadingProject;
#endif

    uint     hints;
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

#ifdef BUILD_BRIDGE
    EnginePluginData plugins[1];
#else
    EnginePluginData* plugins;
#endif

    EngineInternalEvents events;
#ifndef BUILD_BRIDGE
    EngineInternalGraph  graph;
#endif
    EngineInternalTime   time;
    EngineNextAction     nextAction;

    // -------------------------------------------------------------------

    ProtectedData(CarlaEngine* const engine) noexcept;
    ~ProtectedData() noexcept;

    // -------------------------------------------------------------------

    bool init(const char* const clientName);
    void close();

    void initTime(const char* const features);

    // -------------------------------------------------------------------

    void doPluginRemove(const uint pluginId) noexcept;
    void doPluginsSwitch(const uint idA, const uint idB) noexcept;
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
    PendingRtEventsRunner(CarlaEngine* const engine, const uint32_t numFrames) noexcept;
    ~PendingRtEventsRunner() noexcept;

private:
    CarlaEngine::ProtectedData* const pData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(PendingRtEventsRunner)
};

// -----------------------------------------------------------------------

class ScopedActionLock
{
public:
    ScopedActionLock(CarlaEngine* const engine, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept;
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
    ScopedThreadStopper(CarlaEngine* const engine) noexcept;
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
