/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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
    const NativeHostDescriptor* host;
} MidiSplitHandle;

// -----------------------------------------------------------------------

static NativePluginHandle midisplit_instantiate(const NativeHostDescriptor* host)
{
    MidiSplitHandle* const handle = (MidiSplitHandle*)malloc(sizeof(MidiSplitHandle));

    if (handle == NULL)
        return NULL;

    handle->host = host;
    return handle;
}

#define handlePtr ((MidiSplitHandle*)handle)

static void midisplit_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static void midisplit_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host = handlePtr->host;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status  = (uint8_t)MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
        const uint8_t channel = (uint8_t)MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

        if (channel >= MAX_MIDI_CHANNELS)
            continue;

        tmpEvent.port    = channel;
        tmpEvent.time    = midiEvent->time;
        tmpEvent.data[0] = status;
        tmpEvent.data[1] = midiEvent->data[1];
        tmpEvent.data[2] = midiEvent->data[2];
        tmpEvent.data[3] = midiEvent->data[3];
        tmpEvent.size    = midiEvent->size;

        host->write_midi_event(host->handle, &tmpEvent);
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const NativePluginDescriptor midisplitDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = MAX_MIDI_CHANNELS,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "MIDI Split",
    .label     = "midisplit",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midisplit_instantiate,
    .cleanup     = midisplit_cleanup,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,

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
    .process    = midisplit_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midisplit(void);

void carla_register_native_plugin_midisplit(void)
{
    carla_register_native_plugin(&midisplitDesc);
}

// -----------------------------------------------------------------------
