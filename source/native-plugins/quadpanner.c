// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNative.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * FrontLeft, FrontRight, RearLeft, and RearRight speakers: room corners or car doors-like located.
 * Here is each of L-R, F-R controls attenuates two channels but not one.
 *
 * If L-R is +0.1 and F-R is -0.2 then:
 *  FrontLeft channel is *= (0.9 * 1.0),
 *  FrontRight           *= (1.0 * 1.0),
 *  RearLeft             *= (0.9 * 0.8),
 *  and RearRight        *= (1.0 * 0.8).
 */

// --------------------------------------------------------------------------------------------------------------------

typedef struct {
    float a0, b1, z1;
} Filter;

static inline
void set_filter_sample_rate(Filter* const filter, const float sampleRate)
{
    const float frequency = 30.0f / sampleRate;

    filter->b1 = expf(-2.0f * (float)M_PI * frequency);
    filter->a0 = 1.0f - filter->b1;
    filter->z1 = 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------

typedef enum {
    PARAM_LEFT_RIGHT = 0,
    PARAM_FRONT_REAR,
    PARAM_COUNT
} QuadPannerParams;

typedef struct {
    Filter lowpass[PARAM_COUNT * 2];
    float params[PARAM_COUNT];
} QuadPannerHandle;

// --------------------------------------------------------------------------------------------------------------------

static NativePluginHandle quadpanner_instantiate(const NativeHostDescriptor* host)
{
    QuadPannerHandle* const handle = calloc(1, sizeof(QuadPannerHandle));

    if (handle == NULL)
        return NULL;

    const float sampleRate = host->get_sample_rate(host->handle);

    for (unsigned i = 0; i < PARAM_COUNT * 2; ++i)
        set_filter_sample_rate(&handle->lowpass[i], sampleRate);

    return handle;
}

#define handlePtr ((QuadPannerHandle*)handle)

static void quadpanner_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t quadpanner_get_parameter_count(NativePluginHandle handle)
{
    return PARAM_COUNT;

    // unused
    (void)handle;
}

static const NativeParameter* quadpanner_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > PARAM_COUNT)
        return NULL;

    static NativeParameter param;

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMATABLE;
    param.unit = "%";
    param.ranges.def = 0.0f;
    param.ranges.min = -100.0f;
    param.ranges.max = 100.0f;
    param.ranges.step = 1;
    param.ranges.stepSmall = 1;
    param.ranges.stepLarge = 10;

    switch (index)
    {
    case PARAM_LEFT_RIGHT:
        param.name = "Left/Right";
        break;
    case PARAM_FRONT_REAR:
        param.name = "Front/Rear";
        break;
    }

    return &param;

    // unused
    (void)handle;
}

static float quadpanner_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    return handlePtr->params[index];
}

static void quadpanner_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    handlePtr->params[index] = value;
}

static inline
void handle_audio_buffers(const float* inBuffer,
                          float* outBuffer,
                          Filter* const filter,
                          const float gain,
                          const uint32_t frames)
{
    const float a0 = filter->a0;
    const float b1 = filter->b1;
    float z1 = filter->z1;

    for (uint32_t i = 0; i < frames; ++i) {
        z1 = gain * a0 + z1 * b1;
        *outBuffer++ = *inBuffer++ * z1;
    }

    filter->z1 = z1;
}

// FIXME for v3.0, use const for the input buffer
static void quadpanner_process(NativePluginHandle handle,
                               float** inBuffer, float** outBuffer, uint32_t frames,
                               const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    float v1, v2, v3, v4, tmp; // FrontLeft, FrontRight, RearLeft, and RearRight

    // left/right
    if ((tmp = handlePtr->params[PARAM_LEFT_RIGHT]) < 0.f)
    {
        v1 = v3 = 1.f;
        v2 = v4 = 1.f - tmp * -0.01f;
    }
    else
    {
        v1 = v3 = 1.f - tmp * 0.01f;
        v2 = v4 = 1.f;
    }

    // front/rear
    if ((tmp = handlePtr->params[PARAM_FRONT_REAR] * 0.01f) < 0.f)
    {
        v3 *= 1.f - -tmp;
        v4 *= 1.f - -tmp;
    }
    else
    {
        v1 *= 1.f - tmp;
        v2 *= 1.f - tmp;
    }

    handle_audio_buffers(inBuffer[0], outBuffer[0], &handlePtr->lowpass[0], v1, frames);
    handle_audio_buffers(inBuffer[1], outBuffer[1], &handlePtr->lowpass[1], v2, frames);
    handle_audio_buffers(inBuffer[2], outBuffer[2], &handlePtr->lowpass[2], v3, frames);
    handle_audio_buffers(inBuffer[3], outBuffer[3], &handlePtr->lowpass[3], v4, frames);

    return;

    // unused
    (void)midiEvents;
    (void)midiEventCount;
}

static intptr_t quadpanner_dispatcher(NativePluginHandle handle, NativePluginDispatcherOpcode opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    switch (opcode)
    {
        case NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED:
            for (unsigned i = 0; i < PARAM_COUNT * 2; ++i)
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

static const char* quadpanner_get_buffer_port_name(NativePluginHandle handle, uint32_t index, bool isOutput)
{
    static const char* const kInNames[4] = {
        "In-FrontLeft",
        "In-FrontRight",
        "In-RearLeft",
        "In-RearRight",
    };
    static const char* const kOutNames[4] = {
        "Out-FrontLeft",
        "Out-FrontRight",
        "Out-RearLeft",
        "Out-RearRight",
    };

    return isOutput ? kOutNames[index] : kInNames[index];

    // unused
    (void)handle;
}

// --------------------------------------------------------------------------------------------------------------------

static const NativePluginDescriptor quadpannerDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 4,
    .audioOuts = 4,
    .cvIns     = 0,
    .cvOuts    = 0,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = PARAM_COUNT,
    .paramOuts = 0,
    .name      = "Quad-Panner",
    .label     = "quadpanner",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = quadpanner_instantiate,
    .cleanup     = quadpanner_cleanup,

    .get_parameter_count = quadpanner_get_parameter_count,
    .get_parameter_info  = quadpanner_get_parameter_info,
    .get_parameter_value = quadpanner_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = quadpanner_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = quadpanner_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = quadpanner_dispatcher,

    .render_inline_display = NULL,

    .get_buffer_port_name = quadpanner_get_buffer_port_name,
    .get_buffer_port_range = NULL,
};

// --------------------------------------------------------------------------------------------------------------------

void carla_register_native_plugin_quadpanner(void);

void carla_register_native_plugin_quadpanner(void)
{
    carla_register_native_plugin(&quadpannerDesc);
}

// --------------------------------------------------------------------------------------------------------------------
