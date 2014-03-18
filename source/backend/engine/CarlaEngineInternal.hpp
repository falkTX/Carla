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

#include "CarlaMathUtils.hpp"
#include "CarlaMutex.hpp"
#include "LinkedList.hpp"

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return false; }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (cond) pass(); else { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return nullptr; }

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Maximum pre-allocated events for rack and bridge modes

const unsigned short kMaxEngineEventInternalCount = 512;

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// Rack Graph stuff

enum RackGraphGroupIds {
    RACK_GRAPH_GROUP_CARLA     = 0,
    RACK_GRAPH_GROUP_AUDIO_IN  = 1,
    RACK_GRAPH_GROUP_AUDIO_OUT = 2,
    RACK_GRAPH_GROUP_MIDI_IN   = 3,
    RACK_GRAPH_GROUP_MIDI_OUT  = 4,
    RACK_GRAPH_GROUP_MAX       = 5
};

enum RackGraphCarlaPortIds {
    RACK_GRAPH_CARLA_PORT_NULL       = 0,
    RACK_GRAPH_CARLA_PORT_AUDIO_IN1  = 1,
    RACK_GRAPH_CARLA_PORT_AUDIO_IN2  = 2,
    RACK_GRAPH_CARLA_PORT_AUDIO_OUT1 = 3,
    RACK_GRAPH_CARLA_PORT_AUDIO_OUT2 = 4,
    RACK_GRAPH_CARLA_PORT_MIDI_IN    = 5,
    RACK_GRAPH_CARLA_PORT_MIDI_OUT   = 6,
    RACK_GRAPH_CARLA_PORT_MAX        = 7
};

struct PortNameToId {
    int group, port;
    char name[STR_MAX+1];
};

struct ConnectionToId {
    uint id;
    int groupA, portA;
    int groupB, portB;
};

// -----------------------------------------------------------------------
// RackGraph

struct RackGraph {
    uint lastConnectionId;

    CarlaCriticalSection connectLock;

    LinkedList<int> connectedIn1;
    LinkedList<int> connectedIn2;
    LinkedList<int> connectedOut1;
    LinkedList<int> connectedOut2;
    LinkedList<ConnectionToId> usedConnections;

    RackGraph() noexcept
        : lastConnectionId(0) {}

    ~RackGraph() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        lastConnectionId = 0;

        connectedIn1.clear();
        connectedIn2.clear();
        connectedOut1.clear();
        connectedOut2.clear();

        usedConnections.clear();
    }

    bool connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;
    void refresh(CarlaEngine* const engine, const LinkedList<PortNameToId>& midiIns, const LinkedList<PortNameToId>& midiOuts) noexcept;

    const char* const* getConnections() const;
};

// -----------------------------------------------------------------------
// PatchbayGraph

struct PatchbayGraph {
    PatchbayGraph() noexcept {}

    ~PatchbayGraph()
    {
        clear();
    }

    void clear() noexcept
    {
    }

    bool connect(CarlaEngine* const engine, const int groupA, const int portA, const int groupB, const int portB) noexcept;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;
    void refresh(CarlaEngine* const engine, const LinkedList<PortNameToId>& midiIns, const LinkedList<PortNameToId>& midiOuts) noexcept;

    const char* const* getConnections() const;
};
#endif

// -----------------------------------------------------------------------
// InternalAudio

struct EngineInternalAudio {
    bool isReady;

    uint inCount;
    uint outCount;
    float** inBuf;
    float** outBuf;

    EngineInternalAudio() noexcept
        : isReady(false),
          inCount(0),
          outCount(0),
          inBuf(nullptr),
          outBuf(nullptr) {}

    ~EngineInternalAudio() noexcept
    {
        CARLA_SAFE_ASSERT(! isReady);
        CARLA_SAFE_ASSERT(inCount == 0);
        CARLA_SAFE_ASSERT(outCount == 0);
        CARLA_SAFE_ASSERT(inBuf == nullptr);
        CARLA_SAFE_ASSERT(outBuf == nullptr);
    }

    void clearBuffers() noexcept
    {
        for (uint32_t i=0; i < inCount; ++i)
        {
            if (inBuf[i] != nullptr)
            {
                delete[] inBuf[i];
                inBuf[i] = nullptr;
            }
        }
        for (uint32_t i=0; i < outCount; ++i)
        {
            if (outBuf[i] != nullptr)
            {
                delete[] outBuf[i];
                outBuf[i] = nullptr;
            }
        }
    }

    void clear() noexcept
    {
        isReady = false;

        clearBuffers();

        inCount  = 0;
        outCount = 0;

        if (inBuf != nullptr)
        {
            delete[] inBuf;
            inBuf = nullptr;
        }

        if (outBuf != nullptr)
        {
            delete[] outBuf;
            outBuf = nullptr;
        }
    }

    void create(const uint32_t bufferSize)
    {
        CARLA_SAFE_ASSERT(! isReady);
        CARLA_SAFE_ASSERT(inBuf == nullptr);
        CARLA_SAFE_ASSERT(outBuf == nullptr);

        if (inCount > 0)
        {
            inBuf = new float*[inCount];

            for (uint32_t i=0; i < inCount; ++i)
                inBuf[i] = nullptr;
        }

        if (outCount > 0)
        {
            outBuf = new float*[outCount];

            for (uint32_t i=0; i < outCount; ++i)
                outBuf[i] = nullptr;
        }

        resize(bufferSize, false);

        isReady = true;
    }

    void resize(const uint32_t bufferSize, const bool doClear = true)
    {
        if (doClear)
            clearBuffers();

        CARLA_SAFE_ASSERT_RETURN(bufferSize != 0,);

        for (uint32_t i=0; i < inCount; ++i)
        {
            inBuf[i] = new float[bufferSize];
            FLOAT_CLEAR(inBuf[i], bufferSize);
        }

        for (uint32_t i=0; i < outCount; ++i)
        {
            outBuf[i] = new float[bufferSize];
            FLOAT_CLEAR(outBuf[i], bufferSize);
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalAudio)
};

// -----------------------------------------------------------------------
// InternalEvents

struct EngineInternalEvents {
    EngineEvent* in;
    EngineEvent* out;

    EngineInternalEvents() noexcept
        : in(nullptr),
          out(nullptr) {}

    ~EngineInternalEvents() noexcept
    {
        CARLA_SAFE_ASSERT(in == nullptr);
        CARLA_SAFE_ASSERT(out == nullptr);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalEvents)
};

#ifndef BUILD_BRIDGE
// -----------------------------------------------------------------------
// InternalGraph

struct EngineInternalGraph {
    bool isRack;

    union {
        RackGraph* rack;
        PatchbayGraph* patchbay;
    };

    EngineInternalGraph() noexcept
        : isRack(true)
    {
        rack = nullptr;
    }

    ~EngineInternalGraph() noexcept
    {
        CARLA_SAFE_ASSERT(rack == nullptr);
    }

    void create()
    {
        CARLA_SAFE_ASSERT(rack == nullptr);

        if (isRack)
            rack = new RackGraph();
        else
            patchbay = new PatchbayGraph();
    }

    void clear()
    {
        if (isRack)
        {
            CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);
            delete rack;
            rack = nullptr;
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
            delete patchbay;
            patchbay = nullptr;
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(EngineInternalGraph)
};
#endif

// -----------------------------------------------------------------------
// InternalTime

struct EngineInternalTime {
    bool playing;
    uint64_t frame;

    EngineInternalTime() noexcept
        : playing(false),
          frame(0) {}

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

    EngineNextAction() noexcept
        : opcode(kEnginePostActionNull),
          pluginId(0),
          value(0) {}

    ~EngineNextAction() noexcept
    {
        CARLA_SAFE_ASSERT(opcode == kEnginePostActionNull);
    }

    void ready() const noexcept
    {
        mutex.lock();
        mutex.unlock();
    }

    CARLA_DECLARE_NON_COPY_STRUCT(EngineNextAction)
};

// -----------------------------------------------------------------------
// EnginePluginData

struct EnginePluginData {
    CarlaPlugin* plugin;
    float insPeak[2];
    float outsPeak[2];

    void clear() noexcept
    {
        plugin = nullptr;
        insPeak[0] = insPeak[1] = 0.0f;
        outsPeak[0] = outsPeak[1] = 0.0f;
    }
};

// -----------------------------------------------------------------------
// CarlaEngineProtectedData

struct CarlaEngineProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    EngineCallbackFunc callback;
    void*              callbackPtr;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

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

//#ifndef BUILD_BRIDGE
    EngineInternalAudio  audio;
//#endif
    EngineInternalEvents events;
    EngineInternalGraph  graph;
    EngineInternalTime   time;
    EngineNextAction     nextAction;

    // -------------------------------------------------------------------

    CarlaEngineProtectedData(CarlaEngine* const engine) noexcept;
    ~CarlaEngineProtectedData() noexcept;

    // -------------------------------------------------------------------

    void doPluginRemove() noexcept;
    void doPluginsSwitch() noexcept;
    void doNextPluginAction(const bool unlock) noexcept;

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------

    // the base, where plugins run
    void processRack(float* inBufReal[2], float* outBuf[2], const uint32_t nframes, const bool isOffline);

    // extended, will call processRack() in the middle
    void processRackFull(float** const inBuf, const uint32_t inCount, float** const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline);
#endif

    // -------------------------------------------------------------------

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaEngineProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaEngineProtectedData)
#endif
};

// -----------------------------------------------------------------------

class ScopedActionLock
{
public:
    ScopedActionLock(CarlaEngineProtectedData* const data, const EnginePostAction action, const uint pluginId, const uint value, const bool lockWait) noexcept;
    ~ScopedActionLock() noexcept;

private:
    CarlaEngineProtectedData* const fData;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(ScopedActionLock)
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
