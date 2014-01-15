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

#include "CarlaEngine.hpp"
#include "CarlaEngineOsc.hpp"
#include "CarlaEngineThread.hpp"

#include "CarlaMutex.hpp"
#include "LinkedList.hpp"

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return false; }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return nullptr; }

// -----------------------------------------------------------------------
// Float operations

#ifdef HAVE_JUCE
# define FLOAT_ADD(bufDst, bufSrc, frames)  FloatVectorOperations::add(bufDst, bufSrc, frames)
# define FLOAT_COPY(bufDst, bufSrc, frames) FloatVectorOperations::copy(bufDst, bufSrc, frames)
# define FLOAT_CLEAR(buf, frames)           FloatVectorOperations::clear(buf, frames)
#else
# define FLOAT_ADD(bufDst, bufSrc, frames)  carla_addFloat(bufDst, bufSrc, frames)
# define FLOAT_COPY(bufDst, bufSrc, frames) carla_copyFloat(bufDst, bufSrc, frames)
# define FLOAT_CLEAR(buf, frames)           carla_zeroFloat(buf, frames)
#endif

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Maximum pre-allocated events for rack and bridge modes

const unsigned short kMaxEngineEventInternalCount = 512;

// -----------------------------------------------------------------------
// Rack Patchbay stuff

enum RackPatchbayGroupIds {
    RACK_PATCHBAY_GROUP_CARLA     = -1,
    RACK_PATCHBAY_GROUP_AUDIO_IN  = 0,
    RACK_PATCHBAY_GROUP_AUDIO_OUT = 1,
    RACK_PATCHBAY_GROUP_MIDI_IN   = 2,
    RACK_PATCHBAY_GROUP_MIDI_OUT  = 3,
    RACK_PATCHBAY_GROUP_MAX       = 4
};

enum RackPatchbayPortIds {
    RACK_PATCHBAY_PORT_AUDIO_IN1  = -1,
    RACK_PATCHBAY_PORT_AUDIO_IN2  = -2,
    RACK_PATCHBAY_PORT_AUDIO_OUT1 = -3,
    RACK_PATCHBAY_PORT_AUDIO_OUT2 = -4,
    RACK_PATCHBAY_PORT_MIDI_IN    = -5,
    RACK_PATCHBAY_PORT_MIDI_OUT   = -6,
    RACK_PATCHBAY_PORT_MAX        = -7
};

struct PortNameToId {
    int portId;
    char name[STR_MAX+1];
};

struct ConnectionToId {
    int id;
    int portOut;
    int portIn;
};

// -----------------------------------------------------------------------
// EngineRackBuffers

struct EngineRackBuffers {
    float* in[2];
    float* out[2];

    // connections stuff
    LinkedList<uint> connectedIns[2];
    LinkedList<uint> connectedOuts[2];
    CarlaMutex connectLock;

    int lastConnectionId;
    LinkedList<ConnectionToId> usedConnections;

    EngineRackBuffers(const uint32_t bufferSize);
    ~EngineRackBuffers();
    void clear();
    void resize(const uint32_t bufferSize);

    CARLA_DECLARE_NON_COPY_STRUCT(EngineRackBuffers)
};

// -----------------------------------------------------------------------
// EnginePatchbayBuffers

struct EnginePatchbayBuffers {
    // TODO
    EnginePatchbayBuffers(const uint32_t bufferSize);
    ~EnginePatchbayBuffers();
    void clear();
    void resize(const uint32_t bufferSize);

    CARLA_DECLARE_NON_COPY_STRUCT(EnginePatchbayBuffers)
};

// -----------------------------------------------------------------------
// InternalAudio

struct EngineInternalAudio {
    bool isReady;
    bool usePatchbay;

    uint inCount;
    uint outCount;

    union {
        EngineRackBuffers*     rack;
        EnginePatchbayBuffers* patchbay;
    };

    EngineInternalAudio() noexcept;
    ~EngineInternalAudio() noexcept;
    void initPatchbay() noexcept;
    void clear();
    void create(const uint32_t bufferSize);
    void resize(const uint32_t bufferSize);

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalAudio)
};

// -----------------------------------------------------------------------
// InternalEvents

struct EngineInternalEvents {
    EngineEvent* in;
    EngineEvent* out;

    EngineInternalEvents() noexcept;
    ~EngineInternalEvents() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalEvents)
};

// -----------------------------------------------------------------------
// InternalTime

struct EngineInternalTime {
    bool playing;
    uint64_t frame;

    EngineInternalTime() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalTime)
};

// -----------------------------------------------------------------------
// NextAction

enum EnginePostAction {
    kEnginePostActionNull,
    kEnginePostActionZeroCount,
    kEnginePostActionRemovePlugin,
    kEnginePostActionSwitchPlugins
};

struct EngineNextAction {
    EnginePostAction opcode;
    unsigned int pluginId;
    unsigned int value;
    CarlaMutex   mutex;

    EngineNextAction() noexcept;
    ~EngineNextAction() noexcept;
    void ready() noexcept;

    CARLA_DECLARE_NON_COPY_STRUCT(EngineNextAction)
};

// -----------------------------------------------------------------------
// EnginePluginData

struct EnginePluginData {
    CarlaPlugin* plugin;
    float insPeak[2];
    float outsPeak[2];

    void clear() noexcept;
};

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

struct CarlaEngineProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    EngineCallbackFunc callback;
    void*              callbackPtr;

    unsigned int hints;
    uint32_t     bufferSize;
    double       sampleRate;

    bool         aboutToClose;    // don't re-activate thread if true
    unsigned int curPluginCount;  // number of plugins loaded (0...max)
    unsigned int maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    unsigned int nextPluginId;    // invalid if == maxPluginNumber

    CarlaString    lastError;
    CarlaString    name;
    EngineOptions  options;
    EngineTimeInfo timeInfo;

    EnginePluginData* plugins;

#ifndef BUILD_BRIDGE
    EngineInternalAudio  bufAudio;
#endif
    EngineInternalEvents bufEvents;
    EngineInternalTime   time;
    EngineNextAction     nextAction;

    // -------------------------------------------------------------------

    CarlaEngineProtectedData(CarlaEngine* const engine);
    ~CarlaEngineProtectedData() noexcept;

    // -------------------------------------------------------------------

    void doPluginRemove() noexcept;
    void doPluginsSwitch() noexcept;
    void doNextPluginAction(const bool unlock) noexcept;

    // -------------------------------------------------------------------

#ifndef BUILD_BRIDGE
    // the base, where plugins run
    void processRack(float* inBufReal[2], float* outBuf[2], const uint32_t nframes, const bool isOffline);

    // extended, will call processRack() in the middle
    void processRackFull(float** const inBuf, const uint32_t inCount, float** const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline);
#endif

    // -------------------------------------------------------------------

    class ScopedActionLock
    {
    public:
        ScopedActionLock(CarlaEngineProtectedData* const data, const EnginePostAction action, const unsigned int pluginId, const unsigned int value, const bool lockWait) noexcept;
        ~ScopedActionLock() noexcept;

    private:
        CarlaEngineProtectedData* const fData;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedActionLock)
    };

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaEngineProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaEngineProtectedData)
#endif
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
