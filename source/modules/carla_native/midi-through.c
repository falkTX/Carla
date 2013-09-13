/*
 * Carla Native Plugins
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

#include "CarlaNative.h"
#include "CarlaMIDI.h"

#include <stdlib.h>

// -----------------------------------------------------------------------

typedef struct {
    const PluginHostDescriptor* host;
    MappedValue map_midi;
} MidiThroughHandle;

// -----------------------------------------------------------------------

static PluginHandle midiThrough_instantiate(const PluginHostDescriptor* host)
{
    MidiThroughHandle* const handle = (MidiThroughHandle*)malloc(sizeof(MidiThroughHandle));

    if (handle == NULL)
        return NULL;

    handle->host     = host;
    handle->map_midi = host->map_value(host->handle, EVENT_TYPE_MIDI);
    return handle;
}

#define handlePtr ((MidiThroughHandle*)handle)

static void midiThrough_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static void midiThrough_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount)
{
    const PluginHostDescriptor* const host = handlePtr->host;
    const MappedValue map_midi = handlePtr->map_midi;

    for (uint32_t i=0; i < eventCount; ++i)
    {
        if (events[i].type == map_midi)
            host->write_event(host->handle, &events[i]);
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const PluginDescriptor midiThroughDesc = {
    .api        = CARLA_NATIVE_API_VERSION,
    .categories = PLUGIN_CATEGORY_UTILITY ":midi",
    .features   = "rtsafe",
    .supports   = PLUGIN_SUPPORTS_EVERYTHING,
    .metadata   = NULL,
    .audioIns   = 0,
    .audioOuts  = 0,
    .midiIns    = 1,
    .midiOuts   = 1,
    .paramIns   = 0,
    .paramOuts  = 0,
    .author     = "falkTX",
    .name       = "MIDI Through",
    .label      = "midiThrough",
    .copyright  = "GNU GPL v2+",
    .version    = 0x1000,

    .instantiate = midiThrough_instantiate,
    .cleanup     = midiThrough_cleanup,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,
    .get_parameter_text  = NULL,
    .set_parameter_value = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,
    .set_midi_program       = NULL,

    .idle = NULL,

    .get_state = NULL,
    .set_state = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midiThrough_process,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiThrough()
{
    carla_register_native_plugin(&midiThroughDesc);
}

// -----------------------------------------------------------------------
