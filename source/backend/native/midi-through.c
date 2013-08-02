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

typedef struct _MidiThroughHandle {
    HostDescriptor* host;
} MidiThroughHandle;

static PluginHandle midiThrough_instantiate(HostDescriptor* host)
{
    MidiThroughHandle* const handle = (MidiThroughHandle*)malloc(sizeof(MidiThroughHandle));

    if (handle == NULL)
        return NULL;

    handle->host = host;
    return handle;
}

#define handlePtr ((MidiThroughHandle*)handle)

static void midiThrough_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static void midiThrough_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
{
    HostDescriptor* const host = handlePtr->host;

    for (uint32_t i=0; i < midiEventCount; ++i)
        host->write_midi_event(host->handle, &midiEvents[i]);

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const PluginDescriptor midiThroughDesc = {
    .category  = PLUGIN_CATEGORY_UTILITY,
    .hints     = PLUGIN_IS_RTSAFE,
    .supports  = PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 1,
    .parameterIns  = 0,
    .parameterOuts = 0,
    .name      = "MIDI Through",
    .label     = "midiThrough",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midiThrough_instantiate,
    .cleanup     = midiThrough_cleanup,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,
    .get_parameter_text  = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = NULL,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midiThrough_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiThrough()
{
    carla_register_native_plugin(&midiThroughDesc);
}

// -----------------------------------------------------------------------
