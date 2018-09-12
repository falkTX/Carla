/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.hpp"
#include "CarlaNativePlugin.h"

#include "CarlaString.hpp"
#include "water/files/File.h"

// --------------------------------------------------------------------------------------------------------------------

static uint32_t get_buffer_size(NativeHostHandle)
{
    return 128;
}

static double get_sample_rate(NativeHostHandle)
{
    return 44100.0;
}

static bool is_offline(NativeHostHandle)
{
    return false;
}

int main()
{
    const char* const filename = carla_get_library_filename();
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', 1);

    const char* const folder = carla_get_library_folder();
    CARLA_SAFE_ASSERT_RETURN(folder != nullptr && folder[0] != '\0', 1);

    const NativePluginDescriptor* const rack = carla_get_native_rack_plugin();
    CARLA_SAFE_ASSERT_RETURN(rack != nullptr, 1);

    const NativePluginDescriptor* const patchbay = carla_get_native_patchbay_plugin();
    CARLA_SAFE_ASSERT_RETURN(patchbay != nullptr, 1);

    const NativeHostDescriptor host = {
        nullptr,
        "", // resourceDir
        "Carla Plugin UI",
        0,

        get_buffer_size,
        get_sample_rate,
        is_offline,

        nullptr, // get_time_info
        nullptr, // write_midi_event

        nullptr, // ui_parameter_changed
        nullptr, // ui_midi_program_changed
        nullptr, // ui_custom_data_changed
        nullptr, // ui_closed

        nullptr, // ui_open_file
        nullptr, // ui_save_file

        nullptr, // dispatcher
    };

    const NativePluginHandle handle = rack->instantiate(&host);
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, 1);

    CarlaBackend::CarlaEngine* const engine = carla_plugin_get_engine(rack, handle);
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, 1);

    carla_stdout("Got Engine %p, %s, %i, %f",
                 engine, engine->getName(), engine->getBufferSize(), engine->getSampleRate());

    rack->cleanup(handle);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
