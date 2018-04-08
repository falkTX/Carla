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

// -----------------------------------------------------------------------

typedef enum {
    PARAM_OCTAVES = 0,
    PARAM_SEMITONES,
    PARAM_COUNT
} MidiTransposeParams;

typedef struct {
    const NativeHostDescriptor* host;
    int octaves;
    int semitones;
} MidiTransposeHandle;

// -----------------------------------------------------------------------

static NativePluginHandle miditranspose_instantiate(const NativeHostDescriptor* host)
{
    MidiTransposeHandle* const handle = (MidiTransposeHandle*)malloc(sizeof(MidiTransposeHandle));

    if (handle == NULL)
        return NULL;

    handle->host    = host;
    handle->octaves = 0;
    handle->semitones = 0;
    return handle;
}

#define handlePtr ((MidiTransposeHandle*)handle)

static void miditranspose_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t miditranspose_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* miditranspose_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE|NATIVE_PARAMETER_IS_INTEGER;
    param.unit  = NULL;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    switch (index)
    {
    case PARAM_OCTAVES:
        param.name = "Octaves";
        param.ranges.def = 0.0f,
        param.ranges.min = -8.0f,
        param.ranges.max = 8.0f,
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 4.0f;
        break;
    case PARAM_SEMITONES:
        param.name = "Semitones";
        param.ranges.def = 0.0f,
        param.ranges.min = -12.0f,
        param.ranges.max = 12.0f,
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 4.0f;
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float miditranspose_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_OCTAVES:
        return (float)handlePtr->octaves;
    case PARAM_SEMITONES:
        return (float)handlePtr->semitones;
    default:
        return 0.0f;
    }
}

static void miditranspose_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_OCTAVES:
        handlePtr->octaves = (int)value;
        break;
    case PARAM_SEMITONES:
        handlePtr->semitones = (int)value;
        break;
    }
}

static void miditranspose_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host = handlePtr->host;
    const int octaves = handlePtr->octaves;
    const int semitones = handlePtr->semitones;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status = (uint8_t)MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON)
        {
            int oldnote = midiEvent->data[1];
            int newnote = oldnote + octaves*12 + semitones;

            if (newnote < 0 || newnote >= MAX_MIDI_NOTE)
                continue;

            tmpEvent.port    = midiEvent->port;
            tmpEvent.time    = midiEvent->time;
            tmpEvent.data[0] = midiEvent->data[0];
            tmpEvent.data[1] = (uint8_t)newnote;
            tmpEvent.data[2] = midiEvent->data[2];
            tmpEvent.data[3] = midiEvent->data[3];
            tmpEvent.size    = midiEvent->size;

            host->write_midi_event(host->handle, &tmpEvent);
        }
        else
            host->write_midi_event(host->handle, midiEvent);
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const NativePluginDescriptor miditransposeDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 1,
    .paramIns  = 2,
    .paramOuts = 0,
    .name      = "MIDI Transpose",
    .label     = "miditranspose",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = miditranspose_instantiate,
    .cleanup     = miditranspose_cleanup,

    .get_parameter_count = miditranspose_get_parameter_count,
    .get_parameter_info  = miditranspose_get_parameter_info,
    .get_parameter_value = miditranspose_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = miditranspose_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = miditranspose_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_miditranspose(void);

void carla_register_native_plugin_miditranspose(void)
{
    carla_register_native_plugin(&miditransposeDesc);
}

// -----------------------------------------------------------------------
