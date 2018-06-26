/*
 * Carla Native Plugins
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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
#include <stdio.h>

// -----------------------------------------------------------------------

typedef struct {
    const NativeHostDescriptor* host;
} MidiSplitHandle;

// -----------------------------------------------------------------------

static NativePluginHandle midijoin_instantiate(const NativeHostDescriptor* host)
{
    MidiSplitHandle* const handle = (MidiSplitHandle*)malloc(sizeof(MidiSplitHandle));

    if (handle == NULL)
        return NULL;

    handle->host = host;
    return handle;
}

#define handlePtr ((MidiSplitHandle*)handle)

static void midijoin_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static void midijoin_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host = handlePtr->host;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        if (midiEvent->port >= MAX_MIDI_CHANNELS)
        {
            printf("Assertion error: midiEvent->port:%u >= MAX_MIDI_CHANNELS\n", midiEvent->port);
            continue;
        }

        const uint8_t statusByte = midiEvent->data[0];

        if (MIDI_IS_CHANNEL_MESSAGE(statusByte))
            tmpEvent.data[0] = (uint8_t)((statusByte & MIDI_STATUS_BIT) | (midiEvent->port & MIDI_CHANNEL_BIT));
        else
            tmpEvent.data[0] = statusByte;

        tmpEvent.port    = 0;
        tmpEvent.time    = midiEvent->time;
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

static const NativePluginDescriptor midijoinDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = MAX_MIDI_CHANNELS,
    .midiOuts  = 1,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "MIDI Join",
    .label     = "midijoin",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midijoin_instantiate,
    .cleanup     = midijoin_cleanup,

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
    .process    = midijoin_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midijoin(void);

void carla_register_native_plugin_midijoin(void)
{
    carla_register_native_plugin(&midijoinDesc);
}

// -----------------------------------------------------------------------
