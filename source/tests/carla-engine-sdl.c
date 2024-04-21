/*
 * Carla Host Plugin SDL test
 * Copyright (C) 2022-2024 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaHost.h"
#include "CarlaUtils.h"

#include <emscripten.h>

#include <stdio.h>
#include <string.h>

static void engine_idle_loop(void* const arg)
{
    const CarlaHostHandle handle = arg;
    carla_engine_idle(handle);

    /*
    static int counter = 0;
    if (++counter == 100)
    {
        counter = 0;
        const uint64_t frame = carla_get_current_transport_frame(handle);
        printf("engine_idle_loop | play frame %llu | audio peaks %f %f\n",
               frame,
               carla_get_output_peak_value(handle, 0, true),
               carla_get_output_peak_value(handle, 0, false));
    }
    */
}

int main(void)
{
    printf("carla_get_complete_license_text: %s\n", carla_get_complete_license_text());
//     printf("carla_get_supported_file_extensions: %s\n", carla_get_supported_file_extensions());
//     printf("carla_get_supported_features: %s\n", carla_get_supported_features());
    printf("carla_get_library_filename: %s\n", carla_get_library_filename());
    printf("carla_get_library_folder: %s\n", carla_get_library_folder());

    /*
    const uint32_t plugins_count = carla_get_cached_plugin_count(PLUGIN_INTERNAL, NULL);
    printf("carla_get_cached_plugin_count: %u\n", plugins_count);

    for (uint32_t i=0; i<plugins_count; ++i)
    {
        const CarlaCachedPluginInfo* const plugin_info = carla_get_cached_plugin_info(PLUGIN_INTERNAL, i);
        printf("Loading carla_get_cached_plugin_info #%u '%s'\n", i+1, plugin_info->label);
    }
    */

    const CarlaHostHandle handle = carla_standalone_host_init();
    printf("carla_standalone_host_init: %p\n", handle);

    carla_set_engine_option(handle, ENGINE_OPTION_PROCESS_MODE, ENGINE_PROCESS_MODE_CONTINUOUS_RACK, NULL);

    const bool engine_init = carla_engine_init(handle, "SDL", "Engine Test");
    printf("carla_engine_init: %d\n", engine_init);

    if (!engine_init)
        return 1;

    const bool plugin_added = carla_add_plugin(handle, BINARY_NATIVE, PLUGIN_INTERNAL, NULL, "Music", "audiofile", 0, NULL, 0x0);
    printf("carla_add_plugin: %d\n", plugin_added);

    if (!plugin_added)
        goto close;

    const bool plugin_added2 = carla_add_plugin(handle, BINARY_NATIVE, PLUGIN_INTERNAL, NULL, "Gain", "audiogain_s", 0, NULL, 0x0);
    printf("carla_add_plugin2: %d\n", plugin_added2);

    if (!plugin_added2)
        goto close;

    const uint32_t current_plugins_count = carla_get_current_plugin_count(handle);
    printf("carla_get_current_plugin_count: %u\n", current_plugins_count);

    if (current_plugins_count != 2)
        goto close;

    carla_set_parameter_value(handle, 1, 0, 1.0f); // gain

    carla_patchbay_connect(handle, false, 1, 3, 3, 1);
    carla_patchbay_connect(handle, false, 1, 4, 3, 2);

    carla_set_custom_data(handle, 0, CUSTOM_DATA_TYPE_PATH, "file", "/foolme.mp3");
    carla_transport_play(handle);

    emscripten_set_main_loop_arg(engine_idle_loop, handle, 0, false);

    return 0;

close:
    printf("carla_engine_close: %d\n", carla_engine_close(handle));
    return 1;
}
