/*
 * Carla Native Plugins
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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
    // all versions
    PARAM_GAIN = 0,
    PARAM_COUNT_MONO,

    // stereo version
    PARAM_APPLY_LEFT = PARAM_COUNT_MONO,
    PARAM_APPLY_RIGHT,
    PARAM_COUNT_STEREO
} AudioGainParams;

typedef struct {
    float gain;
    bool isMono;
    bool applyLeft;
    bool applyRight;
} AudioGainHandle;

// -----------------------------------------------------------------------

static NativePluginHandle audiogain_instantiate(const NativeHostDescriptor* host, const bool isMono)
{
    AudioGainHandle* const handle = (AudioGainHandle*)malloc(sizeof(AudioGainHandle));

    if (handle == NULL)
        return NULL;

    handle->gain = 1.0f;
    handle->isMono = isMono;
    handle->applyLeft = true;
    handle->applyRight = true;
    return handle;

    // unused
    (void)host;
}

static NativePluginHandle audiogain_instantiate_mono(const NativeHostDescriptor* host)
{
    return audiogain_instantiate(host, true);
}

static NativePluginHandle audiogain_instantiate_stereo(const NativeHostDescriptor* host)
{
    return audiogain_instantiate(host, false);
}

#define handlePtr ((AudioGainHandle*)handle)

static void audiogain_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t audiogain_get_parameter_count(NativePluginHandle handle)
{
    return handlePtr->isMono ? PARAM_COUNT_MONO : PARAM_COUNT_STEREO;
}

static const NativeParameter* audiogain_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > (handlePtr->isMono ? PARAM_COUNT_MONO : PARAM_COUNT_STEREO))
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
        param.ranges.min = 0.0f;
        param.ranges.max = 4.0f;
        param.ranges.step = PARAMETER_RANGES_DEFAULT_STEP;
        param.ranges.stepSmall = PARAMETER_RANGES_DEFAULT_STEP_SMALL;
        param.ranges.stepLarge = PARAMETER_RANGES_DEFAULT_STEP_LARGE;
        break;
    case PARAM_APPLY_LEFT:
        param.name   = "Apply Left";
        param.hints |= NATIVE_PARAMETER_IS_BOOLEAN;
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        break;
    case PARAM_APPLY_RIGHT:
        param.name   = "Apply Right";
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
}

static float audiogain_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    switch (index)
    {
    case PARAM_GAIN:
        return handlePtr->gain;
    case PARAM_APPLY_LEFT:
        return handlePtr->applyLeft ? 1.0f : 0.0f;
    case PARAM_APPLY_RIGHT:
        return handlePtr->applyRight ? 1.0f : 0.0f;
    default:
        return 0.0f;
    }
}

static void audiogain_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    switch (index)
    {
    case PARAM_GAIN:
        handlePtr->gain = value;
        break;
    case PARAM_APPLY_LEFT:
        handlePtr->applyLeft = (value >= 0.5f);
        break;
    case PARAM_APPLY_RIGHT:
        handlePtr->applyRight = (value >= 0.5f);
        break;
    }
}

static void handle_audio_buffers(const float* inBuffer, float* outBuffer, const float gain, const uint32_t frames)
{
    /*
    if (gain == 0.0f)
    {
        memset(outBuffer, 0, sizeof(float)*frames);
    }
    else if (gain == 1.0f)
    {
        if (outBuffer != inBuffer)
            memcpy(outBuffer, inBuffer, sizeof(float)*frames);
    }
    else
    */
    {
        for (uint32_t i=0; i < frames; ++i)
            *outBuffer++ = *inBuffer++ * gain;
    }
}

static void audiogain_process(NativePluginHandle handle,
                             const float** inBuffer, float** outBuffer, uint32_t frames,
                             const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const float gain      = handlePtr->gain;
    const bool applyLeft  = handlePtr->applyLeft;
    const bool applyRight = handlePtr->applyRight;
    const bool isMono     = handlePtr->isMono;

    handle_audio_buffers(inBuffer[0], outBuffer[0], (isMono || applyLeft) ? gain : 1.0f, frames);

    if (! isMono)
        handle_audio_buffers(inBuffer[1], outBuffer[1], applyRight ? gain : 1.0f, frames);

    return;

    // unused
    (void)midiEvents;
    (void)midiEventCount;
}

// -----------------------------------------------------------------------

static const NativePluginDescriptor audiogainMonoDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 1,
    .audioOuts = 1,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT_MONO,
    .paramOuts = 0,
    .name      = "Audio Gain (Mono)",
    .label     = "audiogain",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = audiogain_instantiate_mono,
    .cleanup     = audiogain_cleanup,

    .get_parameter_count = audiogain_get_parameter_count,
    .get_parameter_info  = audiogain_get_parameter_info,
    .get_parameter_value = audiogain_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = audiogain_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = audiogain_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL,

    .render_inline_display = NULL
};

static const NativePluginDescriptor audiogainStereoDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 2,
    .audioOuts = 2,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT_STEREO,
    .paramOuts = 0,
    .name      = "Audio Gain (Stereo)",
    .label     = "audiogain_s",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = audiogain_instantiate_stereo,
    .cleanup     = audiogain_cleanup,

    .get_parameter_count = audiogain_get_parameter_count,
    .get_parameter_info  = audiogain_get_parameter_info,
    .get_parameter_value = audiogain_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = audiogain_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = audiogain_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL,

    .render_inline_display = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_audiogain(void);

void carla_register_native_plugin_audiogain(void)
{
    carla_register_native_plugin(&audiogainMonoDesc);
    carla_register_native_plugin(&audiogainStereoDesc);
}

// -----------------------------------------------------------------------
