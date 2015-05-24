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
#include <string.h>

// -----------------------------------------------------------------------

typedef enum {
    PARAM_GAIN = 0,
    PARAM_APPLY_NOTES,
    PARAM_APPLY_AFTERTOUCH,
    PARAM_APPLY_CC,
    PARAM_COUNT
} MidiGainParams;

typedef struct {
    const NativeHostDescriptor* host;
    float gain;
    bool applyNotes;
    bool applyAftertouch;
    bool applyCC;
} MidiGainHandle;

// -----------------------------------------------------------------------

static NativePluginHandle midigain_instantiate(const NativeHostDescriptor* host)
{
    MidiGainHandle* const handle = (MidiGainHandle*)malloc(sizeof(MidiGainHandle));

    if (handle == NULL)
        return NULL;

    handle->host = host;
    handle->gain = 1.0f;
    handle->applyNotes = true;
    handle->applyAftertouch = true;
    handle->applyCC = false;
    return handle;
}

#define handlePtr ((MidiGainHandle*)handle)

static void midigain_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midigain_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* midigain_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE;
    param.unit  = NULL;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    switch (index)
    {
    case PARAM_GAIN:
        param.name = "Gain";
        param.ranges.def = 1.0f;
        param.ranges.min = 0.001f;
        param.ranges.max = 4.0f;
        param.ranges.step = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        break;
    case PARAM_APPLY_NOTES:
        param.name   = "Apply Notes";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_APPLY_AFTERTOUCH:
        param.name   = "Apply Aftertouch";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_APPLY_CC:
        param.name   = "Apply CC";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 0.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float midigain_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_GAIN:
        return handlePtr->gain;
    case PARAM_APPLY_NOTES:
        return handlePtr->applyNotes ? 1.0f : 0.0f;
    case PARAM_APPLY_AFTERTOUCH:
        return handlePtr->applyAftertouch ? 1.0f : 0.0f;
    case PARAM_APPLY_CC:
        return handlePtr->applyCC ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

static void midigain_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_GAIN:
        handlePtr->gain = value;
        break;
    case PARAM_APPLY_NOTES:
        handlePtr->applyNotes = (value >= 0.5f);
        break;
    case PARAM_APPLY_AFTERTOUCH:
        handlePtr->applyAftertouch = (value >= 0.5f);
        break;
    case PARAM_APPLY_CC:
        handlePtr->applyCC = (value >= 0.5f);
        break;
    }
}

static void midigain_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host = handlePtr->host;
    const float gain           = handlePtr->gain;
    const bool applyNotes      = handlePtr->applyNotes;
    const bool applyAftertouch = handlePtr->applyAftertouch;
    const bool applyCC         = handlePtr->applyCC;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status = (uint8_t)MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (midiEvent->size == 3 && ((applyNotes      && (status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON)) ||
                                     (applyAftertouch &&  status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH) ||
                                     (applyCC         &&  status == MIDI_STATUS_CONTROL_CHANGE)))
        {
            memcpy(&tmpEvent, midiEvent, sizeof(NativeMidiEvent));

            float value = (float)midiEvent->data[2] * gain;

            if (value <= 0.0f)
                tmpEvent.data[2] = 0;
            else if (value >= 127.0f)
                tmpEvent.data[2] = 127;
            else
                tmpEvent.data[2] = (uint8_t)value;

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

// -----------------------------------------------------------------------

static const NativePluginDescriptor midigainDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 1,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "MIDI Gain",
    .label     = "midigain",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midigain_instantiate,
    .cleanup     = midigain_cleanup,

    .get_parameter_count = midigain_get_parameter_count,
    .get_parameter_info  = midigain_get_parameter_info,
    .get_parameter_value = midigain_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = midigain_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midigain_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midigain(void);

void carla_register_native_plugin_midigain(void)
{
    carla_register_native_plugin(&midigainDesc);
}

// -----------------------------------------------------------------------
