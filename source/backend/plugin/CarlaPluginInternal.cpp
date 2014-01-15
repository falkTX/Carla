/*
 * Carla Plugin
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

#include "CarlaPluginInternal.hpp"

#include "CarlaLibCounter.hpp"

#include <QtCore/QSettings>

CARLA_BACKEND_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

// -----------------------------------------------------------------------
// Buffer functions

void CarlaPluginProtectedData::clearBuffers()
{
    if (latencyBuffers != nullptr)
    {
        CARLA_ASSERT(audioIn.count > 0);

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

            delete[] latencyBuffers[i];
            latencyBuffers[i] = nullptr;
        }

        delete[] latencyBuffers;
        latencyBuffers = nullptr;
        latency = 0;
    }
    else
    {
        CARLA_ASSERT(latency == 0);
    }

    audioIn.clear();
    audioOut.clear();
    param.clear();
    event.clear();
}

void CarlaPluginProtectedData::recreateLatencyBuffers()
{
    if (latencyBuffers != nullptr)
    {
        CARLA_ASSERT(audioIn.count > 0);

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            CARLA_SAFE_ASSERT_CONTINUE(latencyBuffers[i] != nullptr);

            delete[] latencyBuffers[i];
            latencyBuffers[i] = nullptr;
        }

        delete[] latencyBuffers;
        latencyBuffers = nullptr;
    }

    if (audioIn.count > 0 && latency > 0)
    {
        latencyBuffers = new float*[audioIn.count];

        for (uint32_t i=0; i < audioIn.count; ++i)
        {
            latencyBuffers[i] = new float[latency];
            FLOAT_CLEAR(latencyBuffers[i], latency);
        }
    }
}

// -----------------------------------------------------------------------
// Post-poned events

void CarlaPluginProtectedData::postponeRtEvent(const PluginPostRtEventType type, const int32_t value1, const int32_t value2, const float value3)
{
    CARLA_SAFE_ASSERT_RETURN(type != kPluginPostRtEventNull,);

    PluginPostRtEvent event = { type, value1, value2, value3 };

    postRtEvents.appendRT(event);
}

// -----------------------------------------------------------------------
// Library functions

static LibCounter sLibCounter;

const char* CarlaPluginProtectedData::libError(const char* const filename)
{
    return lib_error(filename);
}

bool CarlaPluginProtectedData::libOpen(const char* const filename)
{
    lib = sLibCounter.open(filename);
    return (lib != nullptr);
}

bool CarlaPluginProtectedData::libClose()
{
    const bool ret = sLibCounter.close(lib);
    lib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::libSymbol(const char* const symbol)
{
    return lib_symbol(lib, symbol);
}

bool CarlaPluginProtectedData::uiLibOpen(const char* const filename)
{
    uiLib = sLibCounter.open(filename);
    return (uiLib != nullptr);
}

bool CarlaPluginProtectedData::uiLibClose()
{
    const bool ret = sLibCounter.close(uiLib);
    uiLib = nullptr;
    return ret;
}

void* CarlaPluginProtectedData::uiLibSymbol(const char* const symbol)
{
    return lib_symbol(uiLib, symbol);
}

// -----------------------------------------------------------------------
// Settings functions

void CarlaPluginProtectedData::saveSetting(const unsigned int option, const bool yesNo)
{
    CARLA_SAFE_ASSERT_RETURN(identifier != nullptr && identifier[0] != '\0',);

    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup(identifier);

    switch (option)
    {
    case PLUGIN_OPTION_FIXED_BUFFERS:
        settings.setValue("FixedBuffers", yesNo);
        break;
    case PLUGIN_OPTION_FORCE_STEREO:
        settings.setValue("ForceStereo", yesNo);
        break;
    case PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
        settings.setValue("MapProgramChanges", yesNo);
        break;
    case PLUGIN_OPTION_USE_CHUNKS:
        settings.setValue("UseChunks", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CONTROL_CHANGES:
        settings.setValue("SendControlChanges", yesNo);
        break;
    case PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
        settings.setValue("SendChannelPressure", yesNo);
        break;
    case PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
        settings.setValue("SendNoteAftertouch", yesNo);
        break;
    case PLUGIN_OPTION_SEND_PITCHBEND:
        settings.setValue("SendPitchbend", yesNo);
        break;
    case PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
        settings.setValue("SendAllSoundOff", yesNo);
        break;
    default:
        break;
    }

    settings.endGroup();
}

unsigned int CarlaPluginProtectedData::loadSettings(const unsigned int options, const unsigned int availOptions)
{
    CARLA_SAFE_ASSERT_RETURN(identifier != nullptr && identifier[0] != '\0', 0x0);

    QSettings settings("falkTX", "CarlaPluginSettings");
    settings.beginGroup(identifier);

    unsigned int newOptions = 0x0;

    #define CHECK_AND_SET_OPTION(STR, BIT)                              \
    if ((availOptions & BIT) != 0 || BIT == PLUGIN_OPTION_FORCE_STEREO) \
    {                                                                   \
        if (settings.contains(STR))                                     \
        {                                                               \
            if (settings.value(STR, (options & BIT) != 0).toBool())     \
                newOptions |= BIT;                                      \
        }                                                               \
        else if (options & BIT)                                         \
            newOptions |= BIT;                                          \
    }

    CHECK_AND_SET_OPTION("FixedBuffers", PLUGIN_OPTION_FIXED_BUFFERS);
    CHECK_AND_SET_OPTION("ForceStereo", PLUGIN_OPTION_FORCE_STEREO);
    CHECK_AND_SET_OPTION("MapProgramChanges", PLUGIN_OPTION_MAP_PROGRAM_CHANGES);
    CHECK_AND_SET_OPTION("UseChunks", PLUGIN_OPTION_USE_CHUNKS);
    CHECK_AND_SET_OPTION("SendControlChanges", PLUGIN_OPTION_SEND_CONTROL_CHANGES);
    CHECK_AND_SET_OPTION("SendChannelPressure", PLUGIN_OPTION_SEND_CHANNEL_PRESSURE);
    CHECK_AND_SET_OPTION("SendNoteAftertouch", PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH);
    CHECK_AND_SET_OPTION("SendPitchbend", PLUGIN_OPTION_SEND_PITCHBEND);
    CHECK_AND_SET_OPTION("SendAllSoundOff", PLUGIN_OPTION_SEND_ALL_SOUND_OFF);

    #undef CHECK_AND_SET_OPTION

    settings.endGroup();

    return newOptions;
}

CARLA_BACKEND_END_NAMESPACE
