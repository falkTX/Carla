/*
 * Carla Native Plugins
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_native.h"

#include <string.h> // for memcpy

static PluginHandle bypass_instantiate(const PluginDescriptor* _this_, HostDescriptor* host)
{
    // dummy, return non-NULL
    return (PluginHandle)1;

    // unused
    (void)_this_;
    (void)host;
}

static void bypass_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, uint32_t midiEventCount, const MidiEvent* midiEvents)
{
    float* in  = inBuffer[0];
    float* out = outBuffer[0];

    memcpy(out, in, sizeof(float)*frames);
    return;

    // unused
    (void)handle;
    (void)midiEventCount;
    (void)midiEvents;
}

// -----------------------------------------------------------------------

static const PluginDescriptor bypassDesc = {
    .category  = PLUGIN_CATEGORY_NONE,
    .hints     = 0x0,
    .audioIns  = 1,
    .audioOuts = 1,
    .midiIns   = 0,
    .midiOuts  = 0,
    .parameterIns  = 0,
    .parameterOuts = 0,
    .name       = "ByPass",
    .label      = "bypass",
    .maker      = "falkTX",
    .copyright  = "GNU GPL v2+",

    .instantiate = bypass_instantiate,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,
    .get_parameter_text  = NULL,

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
    .cleanup    = NULL,
    .process    = bypass_process
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_bypass()
{
    carla_register_native_plugin(&bypassDesc);
}

// -----------------------------------------------------------------------
