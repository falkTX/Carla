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
} MidiSplitHandle;

// -----------------------------------------------------------------------

static PluginHandle midiSplit_instantiate(const PluginHostDescriptor* host)
{
    MidiSplitHandle* const handle = (MidiSplitHandle*)malloc(sizeof(MidiSplitHandle));

    if (handle == NULL)
        return NULL;

    handle->host     = host;
    handle->map_midi = host->map_value(host->handle, EVENT_TYPE_MIDI);
    return handle;
}

#define handlePtr ((MidiSplitHandle*)handle)

static void midiSplit_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static void midiSplit_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount)
{
    const PluginHostDescriptor* const host = handlePtr->host;
    const MappedValue map_midi = handlePtr->map_midi;

    MidiEvent tmpEvent;
    tmpEvent.e.type = map_midi;

    for (uint32_t i=0; i < eventCount; ++i)
    {
        if (events[i].type != map_midi)
            continue;

        const MidiEvent* const midiEvent = (const MidiEvent*)&events[i];

        const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
        const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

        if (channel >= MAX_MIDI_CHANNELS)
            continue;

        tmpEvent.e.frame = midiEvent->e.frame;
        tmpEvent.port    = channel;
        tmpEvent.data[0] = status;
        tmpEvent.data[1] = midiEvent->data[1];
        tmpEvent.data[2] = midiEvent->data[2];
        tmpEvent.data[3] = midiEvent->data[3];
        tmpEvent.size    = midiEvent->size;

        host->write_event(host->handle, (const Event*)&tmpEvent);
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const PluginDescriptor midiSplitDesc = {
    .api        = CARLA_NATIVE_API_VERSION,
    .categories = PLUGIN_CATEGORY_UTILITY ":midi",
    .features   = "rtsafe",
    .supports   = PLUGIN_SUPPORTS_EVERYTHING,
    .metadata   = NULL,
    .audioIns   = 0,
    .audioOuts  = 0,
    .midiIns    = 1,
    .midiOuts   = 16,
    .paramIns   = 0,
    .paramOuts  = 0,
    .author     = "falkTX",
    .name       = "MIDI Split",
    .label      = "midiSplit",
    .copyright  = "GNU GPL v2+",

    .instantiate = midiSplit_instantiate,
    .cleanup     = midiSplit_cleanup,

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
    .process    = midiSplit_process,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiSplit()
{
    carla_register_native_plugin(&midiSplitDesc);
}

// -----------------------------------------------------------------------
