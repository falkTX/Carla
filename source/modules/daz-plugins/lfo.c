/*
 * Carla Internal Plugins
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

#include "daz/daz-plugin.h"

#include "CarlaDefines.h"
#include "CarlaMIDI.h"

#include <math.h>
#include <stdlib.h>

// -----------------------------------------------------------------------
// Implemented by Carla

extern void carla_register_daz_plugin(const PluginDescriptor* desc);

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
    // host struct
    const PluginHostDescriptor* host;

    // params
    int    mode;
    double speed;
    float  multiplier;
    float  baseStart;
    float  value;

    // extra
    bool firstRun;
    mapped_value_t evTypeParam;
    mapped_value_t evTypeTime;
} LfoHandle;

// -----------------------------------------------------------------------

static const char* lfo_metadata[] = {
    "api",       "1", // FIXME: should be a macro
    "features",  PLUGIN_FEATURE_BUFFER_SIZE_CHANGES PLUGIN_FEATURE_SAMPLE_RATE_CHANGES PLUGIN_FEATURE_TIME PLUGIN_FEATURE_WRITE_EVENT DAZ_TERMINATOR,
    "audioIns",  "0",
    "audioOuts", "0",
    "midiIns",   "0",
    "midiOuts",  "0",
    "paramIns",  "4",
    "paramOuts", "1",
    "author",    "falkTX",
    "name",      "LFO",
    "label",     "lfo",
    "copyright", "GNU GPL v2+",
    "version",   "1.0.0",
    NULL
};

// -----------------------------------------------------------------------

static PluginHandle lfo_instantiate(const PluginHostDescriptor* host)
{
    LfoHandle* const handle = (LfoHandle*)malloc(sizeof(LfoHandle));

    if (handle == NULL)
        return NULL;

    handle->host        = host;
    handle->mode        = 1;
    handle->speed       = 1.0f;
    handle->multiplier  = 1.0f;
    handle->baseStart   = 0.0f;
    handle->value       = 0.0f;
    handle->firstRun    = true;
    handle->evTypeParam = host->map_value(host->handle, EVENT_TYPE_PARAMETER);
    handle->evTypeTime  = host->map_value(host->handle, EVENT_TYPE_TIME);
    return handle;
}

// -----------------------------------------------------------------------

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
    if (index > PARAM_COUNT)
        return NULL;

    static Parameter param;
    static ParameterScalePoint paramModes[5];

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
        param.symbol = "mode";
        param.unit   = NULL;
        param.hints  = PARAMETER_IS_ENABLED PARAMETER_IS_INTEGER PARAMETER_USES_SCALEPOINTS;
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
        param.name   = "Speed";
        param.symbol = "speed";
        param.unit   = "(coef)";
        param.hints  = PARAMETER_IS_ENABLED;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.01f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.25f;
        param.ranges.stepSmall = 0.1f;
        param.ranges.stepLarge = 0.5f;
        break;
    case PARAM_MULTIPLIER:
        param.name   = "Multiplier";
        param.symbol = "multi";
        param.unit   = "(coef)";
        param.hints  = PARAMETER_IS_ENABLED;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.01f;
        param.ranges.max = 2.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_BASE_START:
        param.name   = "Start value";
        param.symbol = "start";
        param.unit   = NULL;
        param.hints  = PARAMETER_IS_ENABLED;
        param.ranges.def = 0.0f;
        param.ranges.min = -1.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 0.01f;
        param.ranges.stepSmall = 0.0001f;
        param.ranges.stepLarge = 0.1f;
        break;
    case PARAM_LFO_OUT:
        param.name   = "LFO Out";
        param.symbol = "out";
        param.unit   = NULL;
        param.hints  = PARAMETER_IS_ENABLED PARAMETER_IS_OUTPUT;
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
    LfoHandle* const lfohandle = handlePtr;

    switch (index)
    {
    case PARAM_MODE:
        return (float)lfohandle->mode;
    case PARAM_SPEED:
        return (float)lfohandle->speed;
    case PARAM_MULTIPLIER:
        return lfohandle->multiplier;
    case PARAM_BASE_START:
        return lfohandle->baseStart;
    case PARAM_LFO_OUT:
        return lfohandle->value;
    default:
        return 0.0f;
    }
}

static void lfo_parameter_changed(LfoHandle* const lfohandle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_MODE:
        lfohandle->mode = (int)value;
        break;
    case PARAM_SPEED:
        lfohandle->speed = value;
        break;
    case PARAM_MULTIPLIER:
        lfohandle->multiplier = value;
        break;
    case PARAM_BASE_START:
        lfohandle->baseStart = value;
        break;
    }
}

static float lfo_calculate_value(LfoHandle* const lfohandle, const uint64_t frame, const double bpm, const double sampleRate)
{
    double value;
    double speedRate  = lfohandle->speed/(bpm/60.0/sampleRate);
    uint   speedRatei = (uint)speedRate;

    switch (lfohandle->mode)
    {
    case 1: // Triangle
        value = fabs(1.0-(double)(frame % speedRatei)/(speedRate/2.0));
        break;
    case 2: // Sawtooth
        value = (double)(frame % speedRatei)/speedRate;
        break;
    case 3: // Sawtooth (inverted)
        value = 1.0 - (double)(frame % speedRatei)/speedRate;
        break;
    case 4: // Sine -- TODO!
        value = 0.0;
        break;
    case 5: // Square
        value = (frame % speedRatei <= speedRatei/2) ? 1.0 : 0.0;
        break;
    }

    value *= lfohandle->multiplier;
    value += lfohandle->baseStart;

    if (value <= 0.0)
    {
        lfohandle->value = 0.0f;
        return 0.0f;
    }
    else if (value >= 1.0)
    {
        lfohandle->value = 1.0f;
        return 1.0f;
    }
    else
    {
        lfohandle->value = (float)value;
        return lfohandle->value;
    }
}

static void lfo_activate(PluginHandle handle)
{
    handlePtr->firstRun = true;
}

static void lfo_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount)
{
    LfoHandle* const lfohandle = handlePtr;

    const PluginHostDescriptor* const host     = lfohandle->host;
    const TimeInfo*             const timeInfo = host->get_time_info(host->handle);

    uint64_t frame = timeInfo->frame;
    double   bpm   = timeInfo->bbt.valid ? timeInfo->bbt.beatsPerMinute : 120.0;

    ParameterEvent event;

    event.e.type  = lfohandle->evTypeParam;
    event.e.frame = 0;
    event.index   = PARAM_LFO_OUT;

    if (lfohandle->firstRun)
    {
        lfohandle->firstRun = false;

        event.value = lfo_calculate_value(lfohandle, frame, bpm, host->sample_rate);
        host->write_event(host->handle, (const Event*)&event);
    }

    for (uint32_t i=0; i < eventCount; ++i)
    {
        const mapped_value_t valType = events[i].type;

        if (valType == lfohandle->evTypeParam)
        {
            const ParameterEvent* const paramEvent = (const ParameterEvent*)&events[i];
            lfo_parameter_changed(lfohandle, paramEvent->index, paramEvent->value);

            event.e.frame = paramEvent->e.frame;
            event.value   = lfo_calculate_value(lfohandle, frame, bpm, host->sample_rate);
            host->write_event(host->handle, (const Event*)&event);
        }
        else if (valType == lfohandle->evTypeTime)
        {
            const TimeInfoEvent* const timeEvent = (const TimeInfoEvent*)&events[i];
            frame = timeEvent->frame;
            bpm   = timeEvent->bbt.valid ? timeEvent->bbt.beatsPerMinute : 120.0;
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

static const PluginDescriptor lfoDesc = {
    .metadata = lfo_metadata,

    .instantiate = lfo_instantiate,
    .cleanup     = lfo_cleanup,

    .get_parameter_count = lfo_get_parameter_count,
    .get_parameter_info  = lfo_get_parameter_info,
    .get_parameter_value = lfo_get_parameter_value,
    .get_parameter_text  = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .idle = NULL,
    .non_rt_event = NULL,

    .get_state = NULL,
    .set_state = NULL,

    .activate   = lfo_activate,
    .deactivate = NULL,
    .process    = lfo_process,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_daz_plugin_lfo()
{
    carla_register_daz_plugin(&lfoDesc);
}

// -----------------------------------------------------------------------
