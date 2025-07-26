// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_ENGINE_INTERNAL_HPP_INCLUDED
#define CARLA_ENGINE_INTERNAL_HPP_INCLUDED

#include "CarlaEngineRunner.hpp"
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

    CARLA_DECLARE_NON_COPYABLE(EngineInternalEvents)
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

    void create(uint32_t audioIns, uint32_t audioOuts,
                uint32_t cvIns, uint32_t cvOuts,
                bool withMidiIn = true, bool withMidiOut = true);
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
    void removeAllPlugins(bool aboutToClose);

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
    CARLA_DECLARE_NON_COPYABLE(EngineInternalGraph)
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
        CARLA_DECLARE_NON_COPYABLE(Hylia)
    } hylia;
#endif

    EngineTimeInfo& timeInfo;
    const EngineTransportMode& transportMode;

    friend class PendingRtEventsRunner;
    void preProcess(uint32_t numFrames);
    void fillEngineTimeInfo(uint32_t newFrames) noexcept;

    friend class CarlaEngineJack;
    void fillJackTimeInfo(jack_position_t* pos, uint32_t newFrames) noexcept;

    CARLA_DECLARE_NON_COPYABLE(EngineInternalTime)
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

static inline
const char* EnginePostAction2Str(const EnginePostAction action)
{
    switch (action)
    {
    case kEnginePostActionNull:
        return "kEnginePostActionNull";
    case kEnginePostActionZeroCount:
        return "kEnginePostActionZeroCount";
#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    case kEnginePostActionRemovePlugin:
        return "kEnginePostActionRemovePlugin";
    case kEnginePostActionSwitchPlugins:
        return "kEnginePostActionSwitchPlugins";
#endif
    }

    carla_stderr("CarlaBackend::EnginePostAction2Str(%i) - invalid action", action);
    return nullptr;
}

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

    CARLA_DECLARE_NON_COPYABLE(EngineNextAction)
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
    CarlaEngineRunner runner;

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
    String currentProjectFilename;
    String currentProjectFolder;
#endif

    uint32_t bufferSize;
    double   sampleRate;

    bool aboutToClose;    // don't re-activate runner if true
    int  isIdling;        // don't allow any operations while idling
    uint curPluginCount;  // number of plugins loaded (0...max)
    uint maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    uint nextPluginId;    // invalid if == maxPluginNumber

    CarlaMutex envMutex;
    String lastError;
    String name;
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

    CarlaMutex pluginsToDeleteMutex;
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
    CARLA_DECLARE_NON_COPYABLE(ProtectedData)
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
    CARLA_DECLARE_NON_COPYABLE(PendingRtEventsRunner)
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
    CARLA_DECLARE_NON_COPYABLE(ScopedActionLock)
};

// -----------------------------------------------------------------------

class ScopedRunnerStopper
{
public:
    ScopedRunnerStopper(CarlaEngine* engine) noexcept;
    ~ScopedRunnerStopper() noexcept;

private:
    CarlaEngine* const engine;
    CarlaEngine::ProtectedData* const pData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(ScopedRunnerStopper)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
