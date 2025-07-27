// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNative.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHANNELS 2

// -----------------------------------------------------------------------

typedef struct {
    float a0, b1, z1;
} Filter;

static inline
void set_filter_sample_rate(Filter* const filter, const float sampleRate)
{
    static const float M_PIf = (float)M_PI;
    const float frequency = 30.0f / sampleRate;

    filter->b1 = expf(-2.0f * M_PIf * frequency);
    filter->a0 = 1.0f - filter->b1;
    filter->z1 = 0.0f;
}

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
    Filter lowpass[MAX_CHANNELS];
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

    const float sampleRate = (float)host->get_sample_rate(host->handle);

    for (unsigned i = 0; i < MAX_CHANNELS; ++i)
        set_filter_sample_rate(&handle->lowpass[i], sampleRate);

    return handle;
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
    if (index > (uint32_t)(handlePtr->isMono ? PARAM_COUNT_MONO : PARAM_COUNT_STEREO))
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;
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

static inline
void handle_audio_buffers(const float* inBuffer, float* outBuffer, Filter* const filter, const float gain, const uint32_t frames)
{
    const float a0 = filter->a0;
    const float b1 = filter->b1;
    float z1 = filter->z1;

    for (uint32_t i=0; i < frames; ++i) {
        z1 = gain * a0 + z1 * b1;
        *outBuffer++ = *inBuffer++ * z1;
    }

    filter->z1 = z1;
}

// FIXME for v3.0, use const for the input buffer
static void audiogain_process(NativePluginHandle handle,
                              float** inBuffer, float** outBuffer, uint32_t frames,
                              const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const float gain      = handlePtr->gain;
    const bool applyLeft  = handlePtr->applyLeft;
    const bool applyRight = handlePtr->applyRight;
    const bool isMono     = handlePtr->isMono;

    handle_audio_buffers(inBuffer[0], outBuffer[0], &handlePtr->lowpass[0], (isMono || applyLeft) ? gain : 1.0f, frames);

    if (! isMono)
        handle_audio_buffers(inBuffer[1], outBuffer[1], &handlePtr->lowpass[1], applyRight ? gain : 1.0f, frames);

    return;

    // unused
    (void)midiEvents;
    (void)midiEventCount;
}

static intptr_t audiogain_dispatcher(NativePluginHandle handle, NativePluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    switch (opcode)
    {
    case NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
        for (unsigned i = 0; i < MAX_CHANNELS; ++i)
            set_filter_sample_rate(&handlePtr->lowpass[i], opt);
        break;
    default:
        break;
    }

    return 0;

    // unused
    (void)index;
    (void)value;
    (void)ptr;
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

    .dispatcher = audiogain_dispatcher,

    .render_inline_display = NULL
};

static const NativePluginDescriptor audiogainStereoDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 2,
    .audioOuts = 2,
    .cvIns     = 0,
    .cvOuts    = 0,
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

    .dispatcher = audiogain_dispatcher
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_audiogain(void);

void carla_register_native_plugin_audiogain(void)
{
    carla_register_native_plugin(&audiogainMonoDesc);
    carla_register_native_plugin(&audiogainStereoDesc);
}

// -----------------------------------------------------------------------
