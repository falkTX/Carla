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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_ENGINE_INTERNAL_HPP__
#define __CARLA_ENGINE_INTERNAL_HPP__

#include "CarlaEngineOsc.hpp"
#include "CarlaEngineThread.hpp"
#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"
#include "RtList.hpp"

class QMainWindow;

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

const uint32_t       PATCHBAY_BUFFER_SIZE = 128;
const unsigned short PATCHBAY_EVENT_COUNT = 512;
const unsigned short RACK_EVENT_COUNT     = 512;

enum EnginePostAction {
    EnginePostActionNull,
    EnginePostActionRemovePlugin,
    EnginePostActionSwitchPlugins
};

struct EnginePluginData {
    CarlaPlugin* plugin;
    float insPeak[CarlaEngine::MAX_PEAKS];
    float outsPeak[CarlaEngine::MAX_PEAKS];

    EnginePluginData()
        : plugin(nullptr),
          insPeak{0.0f},
          outsPeak{0.0f} {}
};

// -------------------------------------------------------------------------------------------------------------------

struct CarlaEngineProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    CallbackFunc callback;
    void*        callbackPtr;

    CarlaString  lastError;
    QMainWindow* hostWindow;

    bool aboutToClose;            // don't re-activate thread if true
    unsigned int curPluginCount;  // number of plugins loaded (0...max)
    unsigned int maxPluginNumber; // number of plugins allowed (0, 16, 99 or 255)

    struct NextAction {
        EnginePostAction opcode;
        unsigned int     pluginId;
        unsigned int     value;
        CarlaMutex       mutex;

        NextAction()
            : opcode(EnginePostActionNull),
              pluginId(0),
              value(0) {}

        ~NextAction()
        {
            CARLA_ASSERT(opcode == EnginePostActionNull);
        }

        void ready()
        {
            mutex.lock();
            mutex.unlock();
        }
    } nextAction;

#ifndef BUILD_BRIDGE
    struct Rack {
        EngineEvent* in;
        EngineEvent* out;

        Rack()
            : in(nullptr),
              out(nullptr) {}
    } rack;
#endif

    struct Time {
        bool playing;
        uint32_t frame;

        Time()
            : playing(false),
              frame(0) {}
    } time;

    EnginePluginData* plugins;

    CarlaEngineProtectedData(CarlaEngine* const engine)
        : osc(engine),
          thread(engine),
          oscData(nullptr),
          callback(nullptr),
          callbackPtr(nullptr),
          hostWindow(nullptr),
          aboutToClose(false),
          curPluginCount(0),
          maxPluginNumber(0),
          plugins(nullptr) {}

    CarlaEngineProtectedData() = delete;
    CarlaEngineProtectedData(CarlaEngineProtectedData&) = delete;
    CarlaEngineProtectedData(const CarlaEngineProtectedData&) = delete;

#ifndef BUILD_BRIDGE
    static void registerEnginePlugin(CarlaEngine* const engine, const unsigned int id, CarlaPlugin* const plugin)
    {
        CARLA_ASSERT(id == engine->kData->curPluginCount);

        if (id == engine->kData->curPluginCount)
            engine->kData->plugins[id].plugin = plugin;
    }
#endif
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_ENGINE_INTERNAL_HPP__
