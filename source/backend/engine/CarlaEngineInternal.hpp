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

#ifndef CARLA_ENGINE_INTERNAL_HPP_INCLUDED
#define CARLA_ENGINE_INTERNAL_HPP_INCLUDED

#include "CarlaEngineOsc.hpp"
#include "CarlaEngineThread.hpp"
#include "CarlaEngineUtils.hpp"

#include "juce_core.h"
using juce::Atomic;
using juce::WaitableEvent;

// FIXME only use CARLA_PREVENT_HEAP_ALLOCATION for structs
//       maybe separate macro

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return false;   }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return nullptr; }

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
struct PatchbayGraph;

class EngineInternalGraph
{
public:
    EngineInternalGraph() noexcept;
    ~EngineInternalGraph() noexcept;

    void create(const bool isRack, const double sampleRate, const uint32_t bufferSize, const uint32_t inputs, const uint32_t outputs);
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

private:
    bool fIsRack;
    bool fIsReady;

    union {
        RackGraph*     fRack;
        PatchbayGraph* fPatchbay;
    };

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalGraph)
};
#endif

// -----------------------------------------------------------------------
// InternalTime

struct EngineInternalTime {
    bool playing;
    uint64_t frame;

    EngineInternalTime() noexcept;

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
    Atomic<EnginePostAction> opcode;
    uint pluginId;
    uint value;
    WaitableEvent waitEvent;

    EngineNextAction() noexcept;
    ~EngineNextAction() noexcept;
    void ready() const noexcept;

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
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

#ifdef BUILD_BRIDGE
    CarlaOscData* oscData;
#else
    const CarlaOscData* oscData;
#endif

    EngineCallbackFunc callback;
    void*              callbackPtr;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

    uint     hints;
    uint32_t bufferSize;
    double   sampleRate;

    bool aboutToClose;    // don't re-activate thread if true
    int  isIdling;        // don't allow any operations while idling
    uint curPluginCount;  // number of plugins loaded (0...max)
    uint maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    uint nextPluginId;    // invalid if == maxPluginNumber

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

    // -------------------------------------------------------------------

    void doPluginRemove() noexcept;
    void doPluginsSwitch() noexcept;
    void doNextPluginAction(const bool unlock) noexcept;

    // -------------------------------------------------------------------

    //friend class ScopedActionLock;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    ProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(ProtectedData)
#endif
};

// -----------------------------------------------------------------------

class ScopedActionLock
{
public:
    ScopedActionLock(CarlaEngine::ProtectedData* const data, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept;
    ~ScopedActionLock() noexcept;

private:
    CarlaEngine::ProtectedData* const fData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ScopedActionLock)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
