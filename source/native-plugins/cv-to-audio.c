/*
 * Carla Native Plugins
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------

typedef enum {
    // all versions
    PARAM_LIMITER = 0,
    PARAM_COUNT
} CvToAudioParams;

typedef struct {
    bool limiterOn;
} CvToAudioHandle;

// -----------------------------------------------------------------------

static NativePluginHandle cv2audio_instantiate(const NativeHostDescriptor* host)
{
    CvToAudioHandle* const handle = (CvToAudioHandle*)malloc(sizeof(CvToAudioHandle));

    if (handle == NULL)
        return NULL;

    handle->limiterOn = true;

    return handle;

    // unused
    (void)host;
}

#define handlePtr ((CvToAudioHandle*)handle)

static void cv2audio_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t cv2audio_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* cv2audio_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;
    param.unit  = NULL;
    param.scalePointCount = 0;
    param.scalePoints     = NULL;

    switch (index)
    {
    case PARAM_LIMITER:
        param.name = "Briwall Limiter";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
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

static float cv2audio_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_LIMITER:
        return handlePtr->limiterOn ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

static void cv2audio_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_LIMITER:
        handlePtr->limiterOn = (value >= 0.5f);
        break;
    }
}

static const NativePortRange* cv2audio_get_buffer_port_range(NativePluginHandle handle, uint32_t index, bool isOutput)
{
    if (isOutput)
        return NULL;

    static NativePortRange npr;

    switch (index)
    {
    case 0:
        npr.minimum = -1.0f;
        npr.maximum = 1.0f;
        return &npr;
    default:
        return NULL;
    }

    // unused
    (void)handle;
}

static const char* cv2audio_get_buffer_port_name(NativePluginHandle handle, uint32_t index, bool isOutput)
{
    if (index != 0)
        return NULL;

    return isOutput ? "Audio Output" : "CV Input";

    // unused
    (void)handle;
}

// FIXME for v3.0, use const for the input buffer
static void cv2audio_process(NativePluginHandle handle,
                             float** inBuffer, float** outBuffer, uint32_t frames,
                             const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const float* const inBuf  = inBuffer[0];
    /**/  float* const outBuf = outBuffer[0];

    if (handlePtr->limiterOn)
    {
        for (uint32_t i=0; i<frames; ++i)
          outBuf[i] = fmaxf(fminf(inBuf[i], 1.0f), -1.0f);
    }
    else
    {
        if (outBuf != inBuf)
            memcpy(outBuf, inBuf, sizeof(float)*frames);
    }

    return;

    // unused
    (void)midiEvents;
    (void)midiEventCount;
}

// -----------------------------------------------------------------------

static const NativePluginDescriptor cv2audioDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE|NATIVE_PLUGIN_USES_CONTROL_VOLTAGE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 0,
    .audioOuts = 1,
    .cvIns     = 1,
    .cvOuts    = 0,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT,
    .paramOuts = 0,
    .name      = "CV to Audio",
    .label     = "cv2audio",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = cv2audio_instantiate,
    .cleanup     = cv2audio_cleanup,

    .get_parameter_count = cv2audio_get_parameter_count,
    .get_parameter_info  = cv2audio_get_parameter_info,
    .get_parameter_value = cv2audio_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = cv2audio_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .get_buffer_port_name = cv2audio_get_buffer_port_name,
    .get_buffer_port_range = cv2audio_get_buffer_port_range,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = cv2audio_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_cv2audio(void);

void carla_register_native_plugin_cv2audio(void)
{
    carla_register_native_plugin(&cv2audioDesc);
}

// -----------------------------------------------------------------------
