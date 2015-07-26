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

#include <string.h>

// -----------------------------------------------------------------------

static NativePluginHandle bypass_instantiate(const NativeHostDescriptor* host)
{
    // dummy, return non-NULL
    return (NativePluginHandle)0x1;

    // unused
    (void)host;
}

static void bypass_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    memcpy(outBuffer[0], inBuffer[0], sizeof(float)*frames);
    return;

    // unused
    (void)handle;
    (void)midiEvents;
    (void)midiEventCount;
}

// -----------------------------------------------------------------------

static const NativePluginDescriptor bypassDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_NONE,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_NOTHING,
    .audioIns  = 1,
    .audioOuts = 1,
    .midiIns   = 0,
    .midiOuts  = 0,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "Bypass",
    .label     = "bypass",
    .maker     = "falkTX",
    .copyright = "GNU GPL v2+",

    .instantiate = bypass_instantiate,
    .cleanup     = NULL,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = NULL,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = bypass_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_bypass(void);

void carla_register_native_plugin_bypass(void)
{
    carla_register_native_plugin(&bypassDesc);
}

// -----------------------------------------------------------------------
