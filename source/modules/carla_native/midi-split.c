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

// -----------------------------------------------------------------------

static PluginHandle midiSplit_instantiate(const HostDescriptor* host)
{
    // use HostDescriptor as PluginHandle
    return (PluginHandle)host;
}

static void midiSplit_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    const HostDescriptor* const host = (const HostDescriptor*)handle;
    MidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const MidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status  = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);
        const uint8_t channel = MIDI_GET_CHANNEL_FROM_DATA(midiEvent->data);

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

// -----------------------------------------------------------------------

static const PluginDescriptor midiSplitDesc = {
    .category  = PLUGIN_CATEGORY_UTILITY,
    .hints     = PLUGIN_IS_RTSAFE,
    .supports  = PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 16,
    .parameterIns  = 0,
    .parameterOuts = 0,
    .name      = "MIDI Split",
    .label     = "midiSplit",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midiSplit_instantiate,
    .cleanup     = NULL,

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
    .process    = midiSplit_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiSplit()
{
    carla_register_native_plugin(&midiSplitDesc);
}

// -----------------------------------------------------------------------
