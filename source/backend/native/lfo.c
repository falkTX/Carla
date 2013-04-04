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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaNative.h"
#include "CarlaMIDI.h"

#include <math.h>
#include <stdlib.h>

typedef enum _LfoParams {
    PARAM_MODE       = 0,
    PARAM_SPEED      = 1,
    PARAM_MULTIPLIER = 2,
    PARAM_BASE_START = 3,
    PARAM_LFO_OUT    = 4,
    PARAM_COUNT
} LfoParams;

typedef struct _LfoHandle {
    HostDescriptor* host;
    int   mode;
    float speed;
    float multiplier;
    float baseStart;
    float value;
} LfoHandle;

static PluginHandle lfo_instantiate(const PluginDescriptor* _this_, HostDescriptor* host)
{
    LfoHandle* const handle = (LfoHandle*)malloc(sizeof(LfoHandle));

    if (handle != NULL)
    {
        handle->host       = host;
        handle->mode       = 1;
        handle->speed      = 1.0f;
        handle->multiplier = 1.0f;
        handle->baseStart  = 0.0f;
        handle->value      = 0.0f;
        return handle;
    }

    return NULL;

    // unused
    (void)_this_;
}

#define handlePtr ((LfoHandle*)handle)

static void lfo_cleanup(PluginHandle handle)
{
    free(handlePtr);
}

static uint32_t lfo_get_parameter_count(PluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

const Parameter* lfo_get_parameter_info(PluginHandle handle, uint32_t index)
{
    static Parameter param;
    static ParameterScalePoint paramModes[3];

    param.hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    paramModes[0].label = "Type 1";
    paramModes[1].label = "Type 2";
    paramModes[2].label = "Type 3";
    paramModes[0].value = 1.0f;
    paramModes[1].value = 2.0f;
    paramModes[2].value = 3.0f;

    switch (index)
    {
    case PARAM_MODE:
        param.name   = "Mode";
        param.unit   = NULL;
        param.hints |= PARAMETER_IS_INTEGER|PARAMETER_USES_SCALEPOINTS;
        param.ranges.def = 1.0f;
        param.ranges.min = 1.0f;
        param.ranges.max = 3.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount = 3;
        param.scalePoints = paramModes;
        break;
    case PARAM_SPEED:
        param.name = "Speed";
        param.unit = "(coef)";
        param.ranges.def = 1.0f;
        param.ranges.min = 0.1f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.25f;
        param.ranges.stepSmall = 0.1f;
        param.ranges.stepLarge = 0.5f;
        break;
    case PARAM_MULTIPLIER:
        param.name = "Multiplier";
        param.unit = "(coef)";
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_BASE_START:
        param.name = "Start value";
        param.unit = NULL;
        param.ranges.def = -1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_LFO_OUT:
        param.name   = "LFO Out";
        param.unit   = NULL;
        param.hints |= PARAMETER_IS_OUTPUT;
        param.ranges.def = 0.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float lfo_get_parameter_value(PluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_MODE:
        return (float)handlePtr->mode;
    case PARAM_SPEED:
        return handlePtr->speed;
    case PARAM_MULTIPLIER:
        return handlePtr->multiplier;
    case PARAM_BASE_START:
        return handlePtr->baseStart;
    case PARAM_LFO_OUT:
        return handlePtr->value;
    default:
        return 0.0f;
    }
}

static void lfo_set_parameter_value(PluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_MODE:
        handlePtr->mode = (int)value;
        break;
    case PARAM_SPEED:
        handlePtr->speed = value;
        break;
    case PARAM_MULTIPLIER:
        handlePtr->multiplier = value;
        break;
    case PARAM_BASE_START:
        handlePtr->baseStart = value;
        break;
    case PARAM_LFO_OUT:
        handlePtr->value = value;
        break;
    }
}

static void lfo_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
{
    HostDescriptor* const host     = handlePtr->host;
    const TimeInfo* const timeInfo = host->get_time_info(host->handle);

    if (! timeInfo->playing)
       return;

    const float bpm = timeInfo->bbt.valid ? timeInfo->bbt.beatsPerMinute : 120.0;
    const float SR  = host->get_sample_rate(host->handle);

    const float rate = handlePtr->speed/(bpm/60.0f/SR);
    const int  rateI = rate;

    float value = 0.0f;

    switch (handlePtr->mode)
    {
    case 1:
        /* wave: /\/\ */
        value = fabs(1.0f-(float)(timeInfo->frame % rateI)/(rate/2.0f));
        break;
    case 2:
        /* wave: /|/|/ */
        value = (float)(timeInfo->frame % rateI)/rate;
        break;
    case 3:
        // wave: square
        if (timeInfo->frame % rateI <= rateI/2)
            value = 1.0f;
        else
            value = 0.0f;
        break;
    }

    value *= handlePtr->multiplier;
    value += handlePtr->baseStart;

    if (value < 0.0f)
        handlePtr->value = 0.0f;
    else if (value > 1.0f)
        handlePtr->value = 1.0f;
    else
        handlePtr->value = value;

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
    (void)midiEventCount;
    (void)midiEvents;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const PluginDescriptor lfoDesc = {
    .category  = PLUGIN_CATEGORY_NONE,
    .hints     = PLUGIN_IS_RTSAFE,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 0,
    .midiOuts  = 0,
    .parameterIns  = PARAM_COUNT-1,
    .parameterOuts = 1,
    .name      = "LFO",
    .label     = "lfo",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = lfo_instantiate,
    .cleanup     = lfo_cleanup,

    .get_parameter_count = lfo_get_parameter_count,
    .get_parameter_info  = lfo_get_parameter_info,
    .get_parameter_value = lfo_get_parameter_value,
    .get_parameter_text  = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = lfo_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = lfo_process,

    .get_state = NULL,
    .set_state = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_lfo()
{
    carla_register_native_plugin(&lfoDesc);
}

// -----------------------------------------------------------------------
