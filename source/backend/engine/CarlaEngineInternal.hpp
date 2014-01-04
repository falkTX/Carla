/*
 * Carla Engine
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaPlugin.hpp"

#include "CarlaMutex.hpp"
#include "List.hpp"

#ifdef HAVE_JUCE
# include "juce_audio_basics.h"
using juce::FloatVectorOperations;
#endif

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

static inline
const char* EngineType2Str(const EngineType type)
{
    switch (type)
    {
    case kEngineTypeNull:
        return "kEngineTypeNull";
    case kEngineTypeJack:
        return "kEngineTypeJack";
    case kEngineTypeJuce:
        return "kEngineTypeJuce";
    case kEngineTypeRtAudio:
        return "kEngineTypeRtAudio";
    case kEngineTypePlugin:
        return "kEngineTypePlugin";
    case kEngineTypeBridge:
        return "kEngineTypeBridge";
    }

    carla_stderr("CarlaBackend::EngineType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EnginePortType2Str(const EnginePortType type)
{
    switch (type)
    {
    case kEnginePortTypeNull:
        return "kEnginePortTypeNull";
    case kEnginePortTypeAudio:
        return "kEnginePortTypeAudio";
    case kEnginePortTypeCV:
        return "kEnginePortTypeCV";
    case kEnginePortTypeEvent:
        return "kEnginePortTypeEvent";
    }

    carla_stderr("CarlaBackend::EnginePortType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EngineEventType2Str(const EngineEventType type)
{
    switch (type)
    {
    case kEngineEventTypeNull:
        return "kEngineEventTypeNull";
    case kEngineEventTypeControl:
        return "kEngineEventTypeControl";
    case kEngineEventTypeMidi:
        return "kEngineEventTypeMidi";
    }

    carla_stderr("CarlaBackend::EngineEventType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* EngineControlEventType2Str(const EngineControlEventType type)
{
    switch (type)
    {
    case kEngineControlEventTypeNull:
        return "kEngineNullEvent";
    case kEngineControlEventTypeParameter:
        return "kEngineControlEventTypeParameter";
    case kEngineControlEventTypeMidiBank:
        return "kEngineControlEventTypeMidiBank";
    case kEngineControlEventTypeMidiProgram:
        return "kEngineControlEventTypeMidiProgram";
    case kEngineControlEventTypeAllSoundOff:
        return "kEngineControlEventTypeAllSoundOff";
    case kEngineControlEventTypeAllNotesOff:
        return "kEngineControlEventTypeAllNotesOff";
    }

    carla_stderr("CarlaBackend::EngineControlEventType2Str(%i) - invalid type", type);
    return nullptr;
}

// -----------------------------------------------------------------------

const unsigned short kEngineMaxInternalEventCount = 512;

enum EnginePostAction {
    kEnginePostActionNull,
    kEnginePostActionZeroCount,
    kEnginePostActionRemovePlugin,
    kEnginePostActionSwitchPlugins
};

struct EnginePluginData {
    CarlaPlugin* plugin;
    float insPeak[2];
    float outsPeak[2];

    void clear()
    {
        plugin = nullptr;
        insPeak[0]  = insPeak[1]  = 0.0f;
        outsPeak[0] = outsPeak[1] = 0.0f;
    }
};

// -----------------------------------------------------------------------

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

struct EngineRackBuffers {
    float* in[2];
    float* out[2];

    // connections stuff
    List<uint> connectedIns[2];
    List<uint> connectedOuts[2];
    CarlaMutex connectLock;

    int lastConnectionId;
    List<ConnectionToId> usedConnections;

    EngineRackBuffers(const uint32_t bufferSize)
        : lastConnectionId(0)
    {
        resize(bufferSize);
    }

    ~EngineRackBuffers()
    {
        clear();
    }

    void clear()
    {
        lastConnectionId = 0;

        if (in[0] != nullptr)
        {
            delete[] in[0];
            in[0] = nullptr;
        }

        if (in[1] != nullptr)
        {
            delete[] in[1];
            in[1] = nullptr;
        }

        if (out[0] != nullptr)
        {
            delete[] out[0];
            out[0] = nullptr;
        }

        if (out[1] != nullptr)
        {
            delete[] out[1];
            out[1] = nullptr;
        }

        connectedIns[0].clear();
        connectedIns[1].clear();
        connectedOuts[0].clear();
        connectedOuts[1].clear();
        usedConnections.clear();
    }

    void resize(const uint32_t bufferSize)
    {
        if (bufferSize > 0)
        {
            in[0]  = new float[bufferSize];
            in[1]  = new float[bufferSize];
            out[0] = new float[bufferSize];
            out[1] = new float[bufferSize];
        }
        else
        {
            in[0]  = nullptr;
            in[1]  = nullptr;
            out[0] = nullptr;
            out[1] = nullptr;
        }
    }
};

struct EnginePatchbayBuffers {
    EnginePatchbayBuffers(const uint32_t bufferSize)
    {
        resize(bufferSize);
    }

    ~EnginePatchbayBuffers()
    {
        clear();
    }

    void clear()
    {
    }

    void resize(const uint32_t /*bufferSize*/)
    {
    }
};

// -----------------------------------------------------------------------

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
    struct InternalAudio {
        bool isReady;
        bool usePatchbay;

        uint inCount;
        uint outCount;

        union {
            EngineRackBuffers*     rack;
            EnginePatchbayBuffers* patchbay;
        };

        InternalAudio() noexcept
            : isReady(false),
              usePatchbay(false),
              inCount(0),
              outCount(0)
        {
            rack = nullptr;
        }

        ~InternalAudio()
        {
            CARLA_ASSERT(! isReady);
            CARLA_ASSERT(rack == nullptr);
        }

        void initPatchbay()
        {
            if (usePatchbay)
            {
                CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);

                rack->lastConnectionId = 0;
                rack->usedConnections.clear();
            }
        }

        void clear()
        {
            isReady  = false;
            inCount  = 0;
            outCount = 0;

            if (usePatchbay)
            {
                CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
                delete patchbay;
                patchbay = nullptr;
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);
                delete rack;
                rack = nullptr;
            }
        }

        void create(const uint32_t bufferSize)
        {
            if (usePatchbay)
            {
                CARLA_SAFE_ASSERT_RETURN(patchbay == nullptr,);
                patchbay = new EnginePatchbayBuffers(bufferSize);
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(rack == nullptr,);
                rack = new EngineRackBuffers(bufferSize);
            }

            isReady = true;
        }

        void resize(const uint32_t bufferSize)
        {
            if (usePatchbay)
            {
                CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr,);
                patchbay->resize(bufferSize);
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN(rack != nullptr,);
                rack->resize(bufferSize);
            }
        }
    } bufAudio;
#endif

    struct InternalEvents {
        EngineEvent* in;
        EngineEvent* out;

        InternalEvents() noexcept
            : in(nullptr),
              out(nullptr) {}

        ~InternalEvents()
        {
            CARLA_ASSERT(in == nullptr);
            CARLA_ASSERT(out == nullptr);
        }
    } bufEvents;

    struct InternalTime {
        bool playing;
        uint64_t frame;

        InternalTime() noexcept
            : playing(false),
              frame(0) {}
    } time;

    struct NextAction {
        EnginePostAction opcode;
        unsigned int pluginId;
        unsigned int value;
        CarlaMutex   mutex;

        NextAction() noexcept
            : opcode(kEnginePostActionNull),
              pluginId(0),
              value(0) {}

        ~NextAction()
        {
            CARLA_ASSERT(opcode == kEnginePostActionNull);
        }

        void ready() noexcept
        {
            mutex.lock();
            mutex.unlock();
        }
    } nextAction;

    CarlaEngineProtectedData(CarlaEngine* const engine)
        : osc(engine),
          thread(engine),
          oscData(nullptr),
          callback(nullptr),
          callbackPtr(nullptr),
          hints(0x0),
          bufferSize(0),
          sampleRate(0.0),
          aboutToClose(false),
          curPluginCount(0),
          maxPluginNumber(0),
          nextPluginId(0),
          plugins(nullptr) {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaEngineProtectedData() = delete;
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaEngineProtectedData)
#endif

    ~CarlaEngineProtectedData()
    {
        CARLA_ASSERT(curPluginCount == 0);
        CARLA_ASSERT(maxPluginNumber == 0);
        CARLA_ASSERT(nextPluginId == 0);
        CARLA_ASSERT(plugins == nullptr);
    }

    void doPluginRemove()
    {
        CARLA_ASSERT(curPluginCount > 0);
        CARLA_ASSERT(nextAction.pluginId < curPluginCount);
        --curPluginCount;

        // move all plugins 1 spot backwards
        for (unsigned int i=nextAction.pluginId; i < curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(plugins[i+1].plugin);

            CARLA_SAFE_ASSERT_BREAK(plugin != nullptr);

            plugin->setId(i);

            plugins[i].plugin      = plugin;
            plugins[i].insPeak[0]  = 0.0f;
            plugins[i].insPeak[1]  = 0.0f;
            plugins[i].outsPeak[0] = 0.0f;
            plugins[i].outsPeak[1] = 0.0f;
        }

        const unsigned int id(curPluginCount);

        // reset last plugin (now removed)
        plugins[id].plugin      = nullptr;
        plugins[id].insPeak[0]  = 0.0f;
        plugins[id].insPeak[1]  = 0.0f;
        plugins[id].outsPeak[0] = 0.0f;
        plugins[id].outsPeak[1] = 0.0f;
    }

    void doPluginsSwitch()
    {
        CARLA_ASSERT(curPluginCount >= 2);

        const unsigned int idA(nextAction.pluginId);
        const unsigned int idB(nextAction.value);

        CARLA_ASSERT(idA < curPluginCount);
        CARLA_ASSERT(idB < curPluginCount);
        CARLA_ASSERT(plugins[idA].plugin != nullptr);
        CARLA_ASSERT(plugins[idB].plugin != nullptr);

#if 0
        std::swap(plugins[idA].plugin, plugins[idB].plugin);
#else
        CarlaPlugin* const tmp(plugins[idA].plugin);

        plugins[idA].plugin = plugins[idB].plugin;
        plugins[idB].plugin = tmp;
#endif
    }

    void doNextPluginAction(const bool unlock) noexcept
    {
        switch (nextAction.opcode)
        {
        case kEnginePostActionNull:
            break;
        case kEnginePostActionZeroCount:
            curPluginCount = 0;
            break;
        case kEnginePostActionRemovePlugin:
            doPluginRemove();
            break;
        case kEnginePostActionSwitchPlugins:
            doPluginsSwitch();
            break;
        }

        nextAction.opcode   = kEnginePostActionNull;
        nextAction.pluginId = 0;
        nextAction.value    = 0;

        if (unlock)
            nextAction.mutex.unlock();
    }

#ifndef BUILD_BRIDGE
    // the base, where plugins run
    void processRack(float* inBufReal[2], float* outBuf[2], const uint32_t nframes, const bool isOffline);

    // extended, will call processRack() in the middle
    void processRackFull(float** const inBuf, const uint32_t inCount, float** const outBuf, const uint32_t outCount, const uint32_t nframes, const bool isOffline);
#endif

    class ScopedActionLock
    {
    public:
        ScopedActionLock(CarlaEngineProtectedData* const data, const EnginePostAction action, const unsigned int pluginId, const unsigned int value, const bool lockWait)
            : fData(data)
        {
            fData->nextAction.mutex.lock();

            CARLA_ASSERT(fData->nextAction.opcode == kEnginePostActionNull);

            fData->nextAction.opcode   = action;
            fData->nextAction.pluginId = pluginId;
            fData->nextAction.value    = value;

            if (lockWait)
            {
                // block wait for unlock on processing side
                carla_stdout("ScopedPluginAction(%i) - blocking START", pluginId);
                fData->nextAction.mutex.lock();
                carla_stdout("ScopedPluginAction(%i) - blocking DONE", pluginId);
            }
            else
            {
                fData->doNextPluginAction(false);
            }
        }

        ~ScopedActionLock() noexcept
        {
            fData->nextAction.mutex.unlock();
        }

    private:
        CarlaEngineProtectedData* const fData;
    };
};

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
