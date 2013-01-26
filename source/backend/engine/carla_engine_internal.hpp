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

#include "carla_engine.hpp"
#include "carla_engine_osc.hpp"
#include "carla_engine_thread.hpp"

#include "carla_plugin.hpp"

//#include "rt_list.hpp"

#ifndef BUILD_BRIDGE
# include <QtCore/QProcessEnvironment>
#endif

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
    }

    qWarning("CarlaBackend::EngineType2Str(%i) - invalid type", type);
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

    qWarning("CarlaBackend::EnginePortType2Str(%i) - invalid type", type);
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

    qWarning("CarlaBackend::EngineControlEventType2Str(%i) - invalid type", type);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

/*!
 * Maximum number of peaks per plugin.\n
 * \note There are both input and output peaks.
 */
/*static*/ const unsigned short MAX_PEAKS = 2;

const uint32_t       PATCHBAY_BUFFER_SIZE = 128;
const unsigned short PATCHBAY_EVENT_COUNT = 512;
const unsigned short RACK_EVENT_COUNT     = 1024;

#if 0
enum EnginePostEventType {
    EnginePostEventNull,
    EnginePostEventDebug,
    EnginePostEventAddPlugin,   // id, ptr
    EnginePostEventRemovePlugin // id
};

struct EnginePostEvent {
    EnginePostEventType type;
    int32_t value1;
    void*   valuePtr;

    EnginePostEvent()
        : type(EnginePostEventNull),
          value1(-1),
          valuePtr(nullptr) {}
};
#endif

struct EnginePluginData {
    CarlaPlugin* plugin;
    double insPeak[MAX_PEAKS];
    double outsPeak[MAX_PEAKS];

    EnginePluginData()
        : plugin(nullptr),
          insPeak{0},
          outsPeak{0} {}
};

struct CarlaEngineProtectedData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    CallbackFunc callback;
    void*        callbackPtr;

    CarlaString  lastError;

#ifndef BUILD_BRIDGE
    QProcessEnvironment procEnv;
#endif

    bool aboutToClose;            // don't re-activate thread if true
    unsigned int curPluginCount;  // number of plugins loaded (0...max)
    unsigned int maxPluginNumber; // number of plugins allowed (0, 16, 99 or 999)

    EnginePluginData* plugins;

    CarlaEngineProtectedData(CarlaEngine* const engine)
        : osc(engine),
          thread(engine),
          oscData(nullptr),
          callback(nullptr),
          callbackPtr(nullptr),
          aboutToClose(false),
          curPluginCount(0),
          maxPluginNumber(0),
          plugins(nullptr) {}

    CarlaEngineProtectedData() = delete;

    CARLA_LEAK_DETECTOR(CarlaEngineProtectedData)
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_ENGINE_INTERNAL_HPP__
