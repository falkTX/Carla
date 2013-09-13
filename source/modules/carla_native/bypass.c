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

#include <string.h>

// -----------------------------------------------------------------------

static PluginHandle bypass_instantiate(const PluginHostDescriptor* host)
{
    // dummy, return non-NULL
    return (PluginHandle)host;
}

static void bypass_process(PluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const Event* events, uint32_t eventCount)
{
    memcpy(outBuffer[0], inBuffer[0], sizeof(float)*frames);
    return;

    // unused
    (void)handle;
    (void)events;
    (void)eventCount;
}

// -----------------------------------------------------------------------

static const PluginDescriptor bypassDesc = {
    .api        = CARLA_NATIVE_API_VERSION,
    .categories = NULL,
    .features   = "rtsafe",
    .supports   = NULL,
    .metadata   = NULL,
    .audioIns   = 1,
    .audioOuts  = 1,
    .midiIns    = 0,
    .midiOuts   = 0,
    .paramIns   = 0,
    .paramOuts  = 0,
    .author     = "falkTX",
    .name       = "ByPass",
    .label      = "bypass",
    .copyright  = "GNU GPL v2+",
    .version    = 0x1000,

    .instantiate = bypass_instantiate,
    .cleanup     = NULL,

    .get_parameter_count = NULL,
    .get_parameter_info  = NULL,
    .get_parameter_value = NULL,
    .get_parameter_text  = NULL,
    .set_parameter_value = NULL,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,
    .set_midi_program       = NULL,

    .idle = NULL,

    .get_state = NULL,
    .set_state = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = bypass_process,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_bypass()
{
    carla_register_native_plugin(&bypassDesc);
}

// -----------------------------------------------------------------------
