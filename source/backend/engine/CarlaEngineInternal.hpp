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
#include "CarlaPatchbayUtils.hpp"
#include "CarlaMutex.hpp"

// -----------------------------------------------------------------------
// Engine helper macro, sets lastError and returns false/NULL

#define CARLA_SAFE_ASSERT_RETURN_ERR(cond, err)  if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return false;   }
#define CARLA_SAFE_ASSERT_RETURN_ERRN(cond, err) if (! (cond)) { carla_safe_assert(#cond, __FILE__, __LINE__); setLastError(err); return nullptr; }

// -----------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Maximum pre-allocated events for rack and bridge modes

const ushort kMaxEngineEventInternalCount = 512;

// -----------------------------------------------------------------------
// Patchbay stuff

struct GroupPort {
    uint group, port;
};

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

// -----------------------------------------------------------------------
// RackGraph

struct RackGraph {
    PatchbayConnectionList connections;

    struct Audio {
        CarlaMutex mutex;
        LinkedList<uint> connectedIn1;
        LinkedList<uint> connectedIn2;
        LinkedList<uint> connectedOut1;
        LinkedList<uint> connectedOut2;
    } audio;

    struct MIDI {
        LinkedList<PortNameToId> ins;
        LinkedList<PortNameToId> outs;

        const char* getName(const bool isInput, const uint index) const noexcept;
        uint getPortId(const bool isInput, const char portName[]) const noexcept;
    } midi;

    RackGraph() noexcept {}

    ~RackGraph() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
        connections.clear();

        audio.mutex.lock();
        audio.connectedIn1.clear();
        audio.connectedIn2.clear();
        audio.connectedOut1.clear();
        audio.connectedOut2.clear();
        audio.mutex.unlock();

        midi.ins.clear();
        midi.outs.clear();
    }

    bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;

    const char* const* getConnections() const;

    bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const;
};

// -----------------------------------------------------------------------
// PatchbayGraph

struct PatchbayGraph {
    PatchbayGraph() noexcept {}

    ~PatchbayGraph() noexcept
    {
        clear();
    }

    void clear() noexcept
    {
    }

    bool connect(CarlaEngine* const engine, const uint groupA, const uint portA, const uint groupB, const uint portB) noexcept;
    bool disconnect(CarlaEngine* const engine, const uint connectionId) noexcept;

    const char* const* getConnections() const;

    bool getPortIdFromFullName(const char* const fullPortName, uint& groupId, uint& portId) const;
};
#endif

// -----------------------------------------------------------------------
// InternalAudio

struct EngineInternalAudio {
    bool isReady;

    // always 2x2 in rack mode
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

    void clear() noexcept
    {
        if (in != nullptr)
        {
            delete[] in;
            in = nullptr;
        }

        if (out != nullptr)
        {
            delete[] out;
            out = nullptr;
        }
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
        if (isRack)
        {
            CARLA_SAFE_ASSERT_RETURN(rack == nullptr,);
            rack = new RackGraph();
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(patchbay == nullptr,);
            patchbay = new PatchbayGraph();
        }
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
    uint pluginId;
    uint value;
    CarlaMutex mutex;

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

struct CarlaEngine::ProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    EngineCallbackFunc callback;
    void*              callbackPtr;

    FileCallbackFunc fileCallback;
    void*            fileCallbackPtr;

    uint     hints;
    uint32_t bufferSize;
    double   sampleRate;

    bool aboutToClose;    // don't re-activate thread if true
    uint curPluginCount;  // number of plugins loaded (0...max)
    uint maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    uint nextPluginId;    // invalid if == maxPluginNumber

    CarlaString    lastError;
    CarlaString    name;
    EngineOptions  options;
    EngineTimeInfo timeInfo;

    EnginePluginData* plugins;

#ifndef BUILD_BRIDGE
    EngineInternalAudio  audio;
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

    void doPluginRemove() noexcept;
    void doPluginsSwitch() noexcept;
    void doNextPluginAction(const bool unlock) noexcept;

#ifndef BUILD_BRIDGE
    // -------------------------------------------------------------------

    // the base, where plugins run
    void processRack(const float* inBufReal[2], float* outBuf[2], const uint32_t nframes, const bool isOffline);

    // extended, will call processRack() in the middle
    void processRackFull(const float* const* const inBuf, const uint32_t inCount, float* const* const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline);
#endif

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
