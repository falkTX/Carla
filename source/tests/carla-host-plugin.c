/*
 * Carla Host Plugin test
 * Copyright (C) 2020 Filipe Coelho <falktx@falktx.com>
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
    const char* const standalone_filename = carla_standalone_get_library_filename();
    assert(standalone_filename != NULL && standalone_filename[0] != '\0');

    const char* const utils_filename = carla_utils_get_library_filename();
    assert(utils_filename != NULL && utils_filename[0] != '\0');

    const char* const standalone_folder = carla_standalone_get_library_folder();
    assert(standalone_folder != NULL && standalone_folder[0] != '\0');

    const char* const utils_folder = carla_utils_get_library_folder();
    assert(utils_folder != NULL && utils_folder[0] != '\0');

    carla_juce_init();

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

    uint32_t plugins_count = 0;
    const NativePluginDescriptor* const plugin_descriptors = carla_get_native_plugins_data(&plugins_count);
    assert(plugins_count != 0);
    assert(plugin_descriptors != NULL);

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

    rack->cleanup(patchbay_handle);
    rack->cleanup(rack_handle);

    carla_host_handle_free(patchbay_host_handle);
    carla_host_handle_free(rack_host_handle);

    carla_juce_cleanup();
    return 0;
}
