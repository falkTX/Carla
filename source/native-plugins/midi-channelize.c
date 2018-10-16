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
    PARAM_CHANNEL = 0,
    PARAM_COUNT
} MidiChannelizeParams;

typedef struct {
    const NativeHostDescriptor* host;
    int channel;
} MidiChannelizeHandle;

// -----------------------------------------------------------------------

static NativePluginHandle midichannelize_instantiate(const NativeHostDescriptor* host)
{
    MidiChannelizeHandle* const handle = (MidiChannelizeHandle*)malloc(sizeof(MidiChannelizeHandle));

    if (handle == NULL)
        return NULL;

    handle->host    = host;
    handle->channel = 1;
    return handle;
}

#define handlePtr ((MidiChannelizeHandle*)handle)

static void midichannelize_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midichannelize_get_parameter_count(MidiChannelizeHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* midichannelize_get_parameter_info(MidiChannelizeHandle handle, uint32_t index)
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
    case PARAM_CHANNEL:
        param.name = "Channel";
        param.ranges.def = 1.0f,
        param.ranges.min = 1.0f,
        param.ranges.max = 16.0f,
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float midichannelize_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_CHANNEL:
        return (float)handlePtr->channel;
    default:
        return 0.0f;
    }
}

static void midichannelize_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_CHANNEL:
        handlePtr->channel = (int)value;
        break;
    }
}

static void midichannelize_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host = handlePtr->host;
    const int channel = handlePtr->channel;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status = (uint8_t)MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (MIDI_IS_CHANNEL_MESSAGE(status))
        {
            tmpEvent.port = midiEvent->port;
            tmpEvent.time = midiEvent->time;

            tmpEvent.data[0] = status | (channel - 1);
            tmpEvent.data[1] = midiEvent->data[1];
            tmpEvent.data[2] = midiEvent->data[2];
            tmpEvent.data[3] = midiEvent->data[3];
            tmpEvent.size = midiEvent->size;

            host->write_midi_event(host->handle, &tmpEvent);
        }
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const NativePluginDescriptor midichannelizeDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 1,
    .paramIns  = 1,
    .paramOuts = 0,
    .name      = "MIDI Channelize",
    .label     = "midichannelize",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midichannelize_instantiate,
    .cleanup     = midichannelize_cleanup,

    .get_parameter_count = midichannelize_get_parameter_count,
    .get_parameter_info  = midichannelize_get_parameter_info,
    .get_parameter_value = midichannelize_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = midichannelize_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midichannelize_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midichannelize(void);

void carla_register_native_plugin_midichannelize(void)
{
    carla_register_native_plugin(&midichannelizeDesc);
}

// -----------------------------------------------------------------------
