/*
 * Carla Host Plugin test
 * Copyright (C) 2020-2024 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNativePlugin.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t get_buffer_size(NativeHostHandle h)
{
    return 128;

    /* unused */
    (void)h;
}

static double get_sample_rate(NativeHostHandle h)
{
    return 44100.0;

    /* unused */
    (void)h;
}

static bool is_offline(NativeHostHandle h)
{
    return false;

    /* unused */
    (void)h;
}

static intptr_t dispatcher(NativeHostHandle h, NativeHostDispatcherOpcode c, int32_t i, intptr_t v, void* p, float o)
{
    return 0;

    /* unused */
    (void)h;
    (void)c;
    (void)i;
    (void)v;
    (void)p;
    (void)o;
}

static const NativeHostDescriptor host_descriptor = {
    .resourceDir = "",
    .uiName = "",
    .get_buffer_size = get_buffer_size,
    .get_sample_rate = get_sample_rate,
    .is_offline = is_offline,
    .dispatcher = dispatcher
};

int main(void)
{
    const char* const lib_filename = carla_get_library_filename();
    assert(lib_filename != NULL && lib_filename[0] != '\0');

    const char* const lib_folder = carla_get_library_folder();
    assert(lib_folder != NULL && lib_folder[0] != '\0');

    const NativePluginDescriptor* const rack = carla_get_native_rack_plugin();
    assert(rack != NULL);

    const NativePluginDescriptor* const patchbay = carla_get_native_patchbay_plugin();
    assert(patchbay != NULL);

    const NativePluginHandle rack_handle = rack->instantiate(&host_descriptor);
    assert(rack_handle != NULL);

    const NativePluginHandle patchbay_handle = patchbay->instantiate(&host_descriptor);
    assert(patchbay_handle != NULL);

    const CarlaHostHandle rack_host_handle = carla_create_native_plugin_host_handle(rack, rack_handle);
    assert(rack_host_handle);

    const CarlaHostHandle patchbay_host_handle = carla_create_native_plugin_host_handle(patchbay, patchbay_handle);
    assert(patchbay_host_handle);

    carla_set_engine_option(rack_host_handle, ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2, lib_folder);
    carla_set_engine_option(patchbay_host_handle, ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2, lib_folder);

    uint32_t plugins_count = 0;
    const NativePluginDescriptor* const plugin_descriptors = carla_get_native_plugins_data(&plugins_count);
    assert(plugins_count != 0);
    assert(plugin_descriptors != NULL);

#if 1
    for (uint32_t i=0; i<plugins_count; ++i)
    {
        const NativePluginDescriptor* const plugin_descriptor = &plugin_descriptors[i];
        assert(plugin_descriptor->label != NULL);

        printf("Loading plugin #%u '%s'\n", i+1, plugin_descriptor->label);

        if ((plugin_descriptor->hints & NATIVE_PLUGIN_USES_CONTROL_VOLTAGE) == 0x0) {
            assert(carla_add_plugin(rack_host_handle, BINARY_NATIVE, PLUGIN_INTERNAL, "", "", plugin_descriptor->label, 0, NULL, 0x0));
        }

        if (plugin_descriptor->midiIns <= 1 && plugin_descriptor->midiOuts <= 1) {
            assert(carla_add_plugin(patchbay_host_handle, BINARY_NATIVE, PLUGIN_INTERNAL, "", "", plugin_descriptor->label, 0, NULL, 0x0));
        }
    }
#endif

    plugins_count = carla_get_cached_plugin_count(PLUGIN_LV2, lib_folder);
    assert(plugins_count != 0);

    for (uint32_t i=0; i<plugins_count; ++i)
    {
        const CarlaCachedPluginInfo* const plugin_info = carla_get_cached_plugin_info(PLUGIN_LV2, i);
        assert(plugin_info != NULL);

        printf("Loading plugin #%u '%s'\n", i+1, plugin_info->label);

        const char* plugin_uri = strchr(plugin_info->label, '/');
        assert(plugin_uri != NULL && plugin_uri[0] != '\0' && plugin_uri[1] != '\0');
        ++plugin_uri;

        if (plugin_info->cvIns + plugin_info->cvOuts == 0) {
            assert(carla_add_plugin(rack_host_handle, BINARY_NATIVE, PLUGIN_LV2, "", "", plugin_uri, 0, NULL, 0x0));
        }

        if (plugin_info->midiIns <= 1 && plugin_info->midiOuts <= 1) {
            assert(carla_add_plugin(patchbay_host_handle, BINARY_NATIVE, PLUGIN_LV2, "", "", plugin_uri, 0, NULL, 0x0));
        }
    }

#if 1
    assert(carla_add_plugin(rack_host_handle, BINARY_NATIVE, PLUGIN_VST2, "../../bin/CarlaRack.so", "", "", 0, NULL, 0x0));
    assert(carla_add_plugin(rack_host_handle, BINARY_NATIVE, PLUGIN_VST2, "../../bin/CarlaPatchbay.so", "", "", 0, NULL, 0x0));

    assert(carla_add_plugin(patchbay_host_handle, BINARY_NATIVE, PLUGIN_VST2, "../../bin/CarlaRack.so", "", "", 0, NULL, 0x0));
    assert(carla_add_plugin(patchbay_host_handle, BINARY_NATIVE, PLUGIN_VST2, "../../bin/CarlaPatchbay.so", "", "", 0, NULL, 0x0));

    plugins_count = carla_get_current_plugin_count(rack_host_handle);

    for (uint32_t i=0; i<plugins_count; ++i)
    {
        carla_get_plugin_info(rack_host_handle, i);
        carla_get_audio_port_count_info(rack_host_handle, i);
        carla_get_midi_port_count_info(rack_host_handle, i);
        carla_get_parameter_count_info(rack_host_handle, i);
    }

    plugins_count = carla_get_current_plugin_count(patchbay_host_handle);

    for (uint32_t i=0; i<plugins_count; ++i)
    {
        carla_get_plugin_info(patchbay_host_handle, i);
        carla_get_audio_port_count_info(patchbay_host_handle, i);
        carla_get_midi_port_count_info(patchbay_host_handle, i);
        carla_get_parameter_count_info(patchbay_host_handle, i);
    }
#endif

#if 0
    carla_set_engine_about_to_close(rack_host_handle);
    carla_remove_all_plugins(rack_host_handle);

    carla_set_engine_about_to_close(patchbay_host_handle);
    carla_remove_all_plugins(patchbay_host_handle);
#endif

    rack->cleanup(rack_handle);
    rack->cleanup(patchbay_handle);

    carla_host_handle_free(rack_host_handle);
    carla_host_handle_free(patchbay_host_handle);
    return 0;
}
