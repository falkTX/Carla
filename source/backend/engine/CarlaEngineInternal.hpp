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
#include "RtList.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------

const unsigned short INTERNAL_EVENT_COUNT = 512;
const uint32_t       PATCHBAY_BUFFER_SIZE = 128;

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

#ifdef CARLA_PROPER_CPP11_SUPPORT
    EnginePluginData()
        : plugin(nullptr),
          insPeak{0.0f},
          outsPeak{0.0f} {}
#else
    EnginePluginData()
        : plugin(nullptr)
    {
        insPeak[0] = insPeak[1] = nullptr;
        outsPeak[0] = outsPeak[1] = nullptr;
    }
#endif
};

// -------------------------------------------------------------------------------------------------------------------

struct CarlaEngineProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    CallbackFunc callback;
    void*        callbackPtr;
    CarlaString  lastError;

    bool aboutToClose;            // don't re-activate thread if true
    unsigned int curPluginCount;  // number of plugins loaded (0...max)
    unsigned int maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)
    unsigned int nextPluginId;    // invalid if == maxPluginNumber

    EnginePluginData* plugins;

    struct InternalEventBuffer {
        EngineEvent* in;
        EngineEvent* out;

        InternalEventBuffer()
            : in(nullptr),
              out(nullptr) {}
    } bufEvents;

    struct NextAction {
        EnginePostAction opcode;
        unsigned int     pluginId;
        unsigned int     value;
        CarlaMutex       mutex;

        NextAction()
            : opcode(kEnginePostActionNull),
              pluginId(0),
              value(0) {}

        ~NextAction()
        {
            CARLA_ASSERT(opcode == kEnginePostActionNull);
        }

        void ready()
        {
            mutex.lock();
            mutex.unlock();
        }
    } nextAction;

    struct Time {
        bool playing;
        uint32_t frame;

        Time()
            : playing(false),
              frame(0) {}
    } time;

    CarlaEngineProtectedData(CarlaEngine* const engine)
        : osc(engine),
          thread(engine),
          oscData(nullptr),
          callback(nullptr),
          callbackPtr(nullptr),
          aboutToClose(false),
          curPluginCount(0),
          maxPluginNumber(0),
          nextPluginId(0),
          plugins(nullptr) {}

#ifdef CARLA_PROPER_CPP11_SUPPORT
    CarlaEngineProtectedData() = delete;
    CarlaEngineProtectedData(CarlaEngineProtectedData&) = delete;
    CarlaEngineProtectedData(const CarlaEngineProtectedData&) = delete;
#endif

    void doPluginRemove()
    {
        CARLA_ASSERT(curPluginCount > 0);
        CARLA_ASSERT(nextAction.pluginId < curPluginCount);
        --curPluginCount;

        // move all plugins 1 spot backwards
        for (unsigned int i=nextAction.pluginId; i < curPluginCount; ++i)
        {
            CarlaPlugin* const plugin(plugins[i+1].plugin);

            CARLA_ASSERT(plugin != nullptr);

            if (plugin == nullptr)
                break;

            plugin->setId(i);

            plugins[i].plugin      = plugin;
            plugins[i].insPeak[0]  = 0.0f;
            plugins[i].insPeak[1]  = 0.0f;
            plugins[i].outsPeak[0] = 0.0f;
            plugins[i].outsPeak[1] = 0.0f;
        }

        const unsigned int id(curPluginCount);

        // reset now last plugin
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

    void doNextPluginAction(const bool unlock)
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

    class ScopedPluginAction
    {
    public:
        ScopedPluginAction(CarlaEngineProtectedData* const data, const EnginePostAction action, const unsigned int pluginId, const unsigned int value, const bool lockWait)
            : fData(data)
        {
            fData->nextAction.mutex.lock();

            CARLA_ASSERT(fData->nextAction.opcode == kEnginePostActionNull);

            fData->nextAction.opcode   = action;
            fData->nextAction.pluginId = pluginId;
            fData->nextAction.value    = value;

            if (lockWait)
            {
                // block wait for unlock on proccessing side
                carla_stdout("ScopedPluginAction(%i) - blocking START", pluginId);
                fData->nextAction.mutex.lock();
                carla_stdout("ScopedPluginAction(%i) - blocking DONE", pluginId);
            }
            else
            {
                fData->doNextPluginAction(false);
            }
        }

        ~ScopedPluginAction()
        {
            fData->nextAction.mutex.unlock();
        }

    private:
        CarlaEngineProtectedData* const fData;
    };
};

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_ENGINE_INTERNAL_HPP_INCLUDED
