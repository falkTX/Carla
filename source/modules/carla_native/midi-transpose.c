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
    int octaves;
} MidiTransposeHandle;

// -----------------------------------------------------------------------

static PluginHandle midiTranspose_instantiate(const PluginHostDescriptor* host)
{
    MidiTransposeHandle* const handle = (MidiTransposeHandle*)malloc(sizeof(MidiTransposeHandle));

    if (handle == NULL)
        return NULL;

    handle->host     = host;
    handle->map_midi = host->map_value(host->handle, EVENT_TYPE_MIDI);
    handle->octaves  = 0;
    return handle;
}

#define handlePtr ((MidiTransposeHandle*)handle)

static void midiTranspose_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midiTranspose_get_parameter_count(PluginHandle handle)
{
    return 1;

    // unused
    (void)handle;
}

const Parameter* midiTranspose_get_parameter_info(PluginHandle handle, uint32_t index)
{
    if (index != 0)
        return NULL;

    static Parameter param;

    param.hints = PARAMETER_IS_ENABLED ":" PARAMETER_IS_AUTOMABLE ":" PARAMETER_IS_INTEGER;
    param.name  = "Octaves";
    param.unit  = NULL;
    param.ranges.def  = 0.0f;
    param.ranges.min  = -8.0f;
    param.ranges.max  = 8.0f;
    param.ranges.step = 1.0f;
    param.ranges.stepSmall = 1.0f;
    param.ranges.stepLarge = 1.0f;
    param.scalePointCount  = 0;
    param.scalePoints      = NULL;

    return &param;

    // unused
    (void)handle;
}

static float midiTranspose_get_parameter_value(PluginHandle handle, uint32_t index)
{
    if (index != 0)
        return 0.0f;

    return (float)handlePtr->octaves;
}

static void midiTranspose_set_parameter_value(PluginHandle handle, uint32_t index, float value)
{
    if (index != 0)
        return;

    handlePtr->octaves = (int)value;
}

static void midiTranspose_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount)
{
    const PluginHostDescriptor* const host = handlePtr->host;
    const MappedValue map_midi = handlePtr->map_midi;
    const int octaves = handlePtr->octaves;

    MidiEvent tmpEvent;
    tmpEvent.e.type = map_midi;

    for (uint32_t i=0; i < eventCount; ++i)
    {
        if (events[i].type != map_midi)
            continue;

        const MidiEvent* const midiEvent = (const MidiEvent*)&events[i];

        const uint8_t status = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON)
        {
            int oldnote = midiEvent->data[1];
            int newnote = oldnote + octaves*12;

            if (newnote < 0 || newnote >= MAX_MIDI_NOTE)
                continue;

            tmpEvent.e.frame = midiEvent->e.frame;
            tmpEvent.port    = midiEvent->port;
            tmpEvent.data[0] = midiEvent->data[0];
            tmpEvent.data[1] = newnote;
            tmpEvent.data[2] = midiEvent->data[2];
            tmpEvent.data[3] = midiEvent->data[3];
            tmpEvent.size    = midiEvent->size;

            host->write_event(host->handle, (const Event*)&tmpEvent);
        }
        else
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

static const PluginDescriptor midiTransposeDesc = {
    .api        = CARLA_NATIVE_API_VERSION,
    .categories = PLUGIN_CATEGORY_UTILITY ":midi",
    .features   = "rtsafe",
    .supports   = PLUGIN_SUPPORTS_EVERYTHING,
    .metadata   = NULL,
    .audioIns   = 0,
    .audioOuts  = 0,
    .midiIns    = 1,
    .midiOuts   = 1,
    .paramIns   = 1,
    .paramOuts  = 0,
    .author     = "falkTX",
    .name       = "MIDI Transpose",
    .label      = "midiTranspose",
    .copyright  = "GNU GPL v2+",
    .version    = 0x1000,

    .instantiate = midiTranspose_instantiate,
    .cleanup     = midiTranspose_cleanup,

    .get_parameter_count = midiTranspose_get_parameter_count,
    .get_parameter_info  = midiTranspose_get_parameter_info,
    .get_parameter_value = midiTranspose_get_parameter_value,
    .get_parameter_text  = NULL,
    .set_parameter_value = midiTranspose_set_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,
    .set_midi_program       = NULL,

    .idle = NULL,

    .get_state = NULL,
    .set_state = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midiTranspose_process,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiTranspose()
{
    carla_register_native_plugin(&midiTransposeDesc);
}

// -----------------------------------------------------------------------
