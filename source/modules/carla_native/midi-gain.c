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
    const HostDescriptor* host;
    float gain;
    bool applyNotes;
    bool applyAftertouch;
    bool applyCC;
} MidiGainHandle;

// -----------------------------------------------------------------------

static PluginHandle midiGain_instantiate(const HostDescriptor* host)
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

static void midiGain_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midiGain_get_parameter_count(PluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

const Parameter* midiGain_get_parameter_info(PluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static Parameter param;

    param.hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;
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
        param.hints |= PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_APPLY_AFTERTOUCH:
        param.name   = "Apply Aftertouch";
        param.hints |= PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_APPLY_CC:
        param.name   = "Apply CC";
        param.hints |= PARAMETER_IS_BOOLEAN;
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

static float midiGain_get_parameter_value(PluginHandle handle, uint32_t index)
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

static void midiGain_set_parameter_value(PluginHandle handle, uint32_t index, float value)
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

static void midiGain_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount)
{
    const HostDescriptor* const host = handlePtr->host;
    const float gain           = handlePtr->gain;
    const bool applyNotes      = handlePtr->applyNotes;
    const bool applyAftertouch = handlePtr->applyAftertouch;
    const bool applyCC         = handlePtr->applyCC;
    MidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const MidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status = MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (midiEvent->size == 3 && ((applyNotes      && (status == MIDI_STATUS_NOTE_OFF || status == MIDI_STATUS_NOTE_ON)) ||
                                     (applyAftertouch &&  status == MIDI_STATUS_POLYPHONIC_AFTERTOUCH) ||
                                     (applyCC         &&  status == MIDI_STATUS_CONTROL_CHANGE)))
        {
            memcpy(&tmpEvent, midiEvent, sizeof(MidiEvent));

            float value = (float)midiEvent->data[2] * gain;

            if (value <= 0.0f)
                tmpEvent.data[2] = 0;
            else if (value >= 1.27f)
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

static const PluginDescriptor midiGainDesc = {
    .category  = PLUGIN_CATEGORY_UTILITY,
    .hints     = PLUGIN_IS_RTSAFE,
    .supports  = PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = 1,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "MIDI Gain",
    .label     = "midiGain",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = midiGain_instantiate,
    .cleanup     = midiGain_cleanup,

    .get_parameter_count = midiGain_get_parameter_count,
    .get_parameter_info  = midiGain_get_parameter_info,
    .get_parameter_value = midiGain_get_parameter_value,
    .get_parameter_text  = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = midiGain_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midiGain_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiGain()
{
    carla_register_native_plugin(&midiGainDesc);
}

// -----------------------------------------------------------------------
