/*
 * Carla Engine
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __CARLA_ENGINE_INTERNAL_HPP__
#define __CARLA_ENGINE_INTERNAL_HPP__

#include "carla_engine.hpp"
#include "carla_engine_osc.hpp"
#include "carla_engine_thread.hpp"

#include "carla_plugin.hpp"

#ifndef BUILD_BRIDGE
# include <QtCore/QProcessEnvironment>
#endif

#ifdef CARLA_ENGINE_RTAUDIO
# if defined(Q_OS_MAC) && ! defined(__MACOSX_CORE__)
#  define __MACOSX_CORE__
# endif
# if defined(Q_OS_WIN) && ! (defined(__WINDOWS_ASIO__) || defined(__WINDOWS_DS__))
#  define __WINDOWS_ASIO__
#  define __WINDOWS_DS__
# endif
#endif

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

static inline
const char* CarlaEngineType2Str(const CarlaEngineType type)
{
    switch (type)
    {
    case CarlaEngineTypeNull:
        return "CarlaEngineTypeNull";
    case CarlaEngineTypeJack:
        return "CarlaEngineTypeJack";
    case CarlaEngineTypeRtAudio:
        return "CarlaEngineTypeRtAudio";
    case CarlaEngineTypePlugin:
        return "CarlaEngineTypePlugin";
    }

    qWarning("CarlaBackend::CarlaEngineType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* CarlaEnginePortType2Str(const CarlaEnginePortType type)
{
    switch (type)
    {
    case CarlaEnginePortTypeNull:
        return "CarlaEnginePortTypeNull";
    case CarlaEnginePortTypeAudio:
        return "CarlaEnginePortTypeAudio";
    case CarlaEnginePortTypeControl:
        return "CarlaEnginePortTypeControl";
    case CarlaEnginePortTypeMIDI:
        return "CarlaEnginePortTypeMIDI";
    }

    qWarning("CarlaBackend::CarlaEnginePortType2Str(%i) - invalid type", type);
    return nullptr;
}

static inline
const char* CarlaEngineControlEventType2Str(const CarlaEngineControlEventType type)
{
    switch (type)
    {
    case CarlaEngineNullEvent:
        return "CarlaEngineNullEvent";
    case CarlaEngineParameterChangeEvent:
        return "CarlaEngineParameterChangeEvent";
    case CarlaEngineMidiBankChangeEvent:
        return "CarlaEngineMidiBankChangeEvent";
    case CarlaEngineMidiProgramChangeEvent:
        return "CarlaEngineMidiProgramChangeEvent";
    case CarlaEngineAllSoundOffEvent:
        return "CarlaEngineAllSoundOffEvent";
    case CarlaEngineAllNotesOffEvent:
        return "CarlaEngineAllNotesOffEvent";
    }

    qWarning("CarlaBackend::CarlaEngineControlEventType2Str(%i) - invalid type", type);
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

/*!
 * Maximum number of peaks per plugin.\n
 * \note There are both input and output peaks.
 */
/*static*/ const unsigned short MAX_PEAKS = 2;

const uint32_t       PATCHBAY_BUFFER_SIZE = 128;
const unsigned short PATCHBAY_EVENT_COUNT = 256;

struct CarlaEnginePrivateData {
    CarlaEngineOsc    osc;
    CarlaEngineThread thread;

    const CarlaOscData* oscData;

    CallbackFunc callback;
    void*        callbackPtr;
    CarlaString  lastError;

#ifndef BUILD_BRIDGE
    QProcessEnvironment procEnv;
#endif

    QMutex procLock;
    QMutex midiLock;

    CarlaPlugin* carlaPlugins[MAX_PLUGINS];
    const char*  uniqueNames[MAX_PLUGINS];

    double insPeak[MAX_PLUGINS * MAX_PEAKS];
    double outsPeak[MAX_PLUGINS * MAX_PEAKS];

    bool aboutToClose;
    unsigned short maxPluginNumber;

    CarlaEnginePrivateData(CarlaEngine* const engine)
        : osc(engine),
          thread(engine),
          oscData(nullptr),
          callback(nullptr),
          callbackPtr(nullptr),
          carlaPlugins{nullptr},
          uniqueNames{nullptr},
          insPeak{0.0},
          outsPeak{0.0},
          aboutToClose(false),
          maxPluginNumber(0) {}
};

CARLA_BACKEND_END_NAMESPACE

#endif // __CARLA_ENGINE_INTERNAL_HPP__
