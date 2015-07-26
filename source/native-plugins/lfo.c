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

#include "CarlaDefines.h"
#include "CarlaMIDI.h"

#include <math.h>
#include <stdlib.h>

// -----------------------------------------------------------------------

typedef enum {
    PARAM_MODE = 0,
    PARAM_SPEED,
    PARAM_MULTIPLIER,
    PARAM_BASE_START,
    PARAM_LFO_OUT,
    PARAM_COUNT
} LfoParams;

typedef struct {
    const  NativeHostDescriptor* host;
    int    mode;
    double speed;
    float  multiplier;
    float  baseStart;
    float  value;
} LfoHandle;

// -----------------------------------------------------------------------

static NativePluginHandle lfo_instantiate(const NativeHostDescriptor* host)
{
    LfoHandle* const handle = (LfoHandle*)malloc(sizeof(LfoHandle));

    if (handle == NULL)
        return NULL;

    handle->host       = host;
    handle->mode       = 1;
    handle->speed      = 1.0f;
    handle->multiplier = 1.0f;
    handle->baseStart  = 0.0f;
    handle->value      = 0.0f;
    return handle;
}

#define handlePtr ((LfoHandle*)handle)

static void lfo_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t lfo_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* lfo_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;
    static NativeParameterScalePoint paramModes[5];

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    paramModes[0].label = "Triangle";
    paramModes[1].label = "Sawtooth";
    paramModes[2].label = "Sawtooth (inverted)";
    paramModes[3].label = "Sine (TODO)";
    paramModes[4].label = "Square";

    paramModes[0].value = 1.0f;
    paramModes[1].value = 2.0f;
    paramModes[2].value = 3.0f;
    paramModes[3].value = 4.0f;
    paramModes[4].value = 5.0f;

    switch (index)
    {
    case PARAM_MODE:
        param.name   = "Mode";
        param.unit   = NULL;
        param.hints |= NATIVE_PARAMETER_IS_INTEGER|NATIVE_PARAMETER_USES_SCALEPOINTS;
        param.ranges.def = 1.0f;
        param.ranges.min = 1.0f;
        param.ranges.max = 5.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount = 5;
        param.scalePoints = paramModes;
        break;
    case PARAM_SPEED:
        param.name = "Speed";
        param.unit = "(coef)";
        param.ranges.def = 1.0f;
        param.ranges.min = 0.01f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.25f;
        param.ranges.stepSmall = 0.1f;
        param.ranges.stepLarge = 0.5f;
        break;
    case PARAM_MULTIPLIER:
        param.name = "Multiplier";
        param.unit = "(coef)";
        param.ranges.def = 1.0f;
        param.ranges.min = 0.01f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_BASE_START:
        param.name = "Start value";
        param.unit = NULL;
        param.ranges.def = 0.0f;
        param.ranges.min = -1.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_LFO_OUT:
        param.name   = "LFO Out";
        param.unit   = NULL;
        param.hints |= NATIVE_PARAMETER_IS_OUTPUT;
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

static float lfo_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_MODE:
        return (float)handlePtr->mode;
    case PARAM_SPEED:
        return (float)handlePtr->speed;
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

static void lfo_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
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

static void lfo_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host     = handlePtr->host;
    const NativeTimeInfo*       const timeInfo = host->get_time_info(host->handle);

    if (! timeInfo->playing)
       return;

    const double bpm        = timeInfo->bbt.valid ? timeInfo->bbt.beatsPerMinute : 120.0;
    const double sampleRate = host->get_sample_rate(host->handle);

    const double speedRate  = handlePtr->speed/(bpm/60.0/sampleRate);
    const uint   speedRatei = (uint)speedRate;

    double value = 0.0;

    switch (handlePtr->mode)
    {
    case 1: // Triangle
        value = fabs(1.0-(double)(timeInfo->frame % speedRatei)/(speedRate/2.0));
        break;
    case 2: // Sawtooth
        value = (double)(timeInfo->frame % speedRatei)/speedRate;
        break;
    case 3: // Sawtooth (inverted)
        value = 1.0 - (double)(timeInfo->frame % speedRatei)/speedRate;
        break;
    case 4: // Sine -- TODO!
        value = 0.0;
        break;
    case 5: // Square
        value = (timeInfo->frame % speedRatei <= speedRatei/2) ? 1.0 : 0.0;
        break;
    }

    value *= handlePtr->multiplier;
    value += handlePtr->baseStart;

    if (value <= 0.0)
        handlePtr->value = 0.0f;
    else if (value >= 1.0)
        handlePtr->value = 1.0f;
    else
        handlePtr->value = (float)value;

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
    (void)midiEvents;
    (void)midiEventCount;
}

#undef handlePtr

// -----------------------------------------------------------------------

static const NativePluginDescriptor lfoDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT-1,
    .paramOuts = 1,
    .name      = "LFO",
    .label     = "lfo",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = lfo_instantiate,
    .cleanup     = lfo_cleanup,

    .get_parameter_count = lfo_get_parameter_count,
    .get_parameter_info  = lfo_get_parameter_info,
    .get_parameter_value = lfo_get_parameter_value,

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
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_lfo(void);

void carla_register_native_plugin_lfo(void)
{
    carla_register_native_plugin(&lfoDesc);
}

// -----------------------------------------------------------------------
