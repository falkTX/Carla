// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNativePlugin.h"
#include "CarlaHostImpl.hpp"

#include "CarlaEngine.hpp"

#include "water/files/File.h"

// --------------------------------------------------------------------------------------------------------------------
// Expose info functions as needed

#ifndef CARLA_HOST_PLUGIN_BUILD
# include "utils/Information.cpp"
#endif

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CarlaHostHandle carla_create_native_plugin_host_handle(const NativePluginDescriptor* desc, NativePluginHandle handle)
{
    CarlaEngine* const engine = carla_get_native_plugin_engine(desc, handle);
    CARLA_SAFE_ASSERT_RETURN(engine, nullptr);

    CarlaHostHandleImpl* hosthandle;

    try {
        hosthandle = new CarlaHostHandleImpl();
    } CARLA_SAFE_EXCEPTION_RETURN("carla_create_native_plugin_host_handle()", nullptr);

    hosthandle->engine = engine;
    hosthandle->isPlugin = true;
    return hosthandle;
}

void carla_host_handle_free(CarlaHostHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(handle->isPlugin,);

    delete (CarlaHostHandleImpl*)handle;
}

// --------------------------------------------------------------------------------------------------------------------

CarlaEngine* carla_get_native_plugin_engine(const NativePluginDescriptor* desc, NativePluginHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(desc != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    return (CarlaEngine*)static_cast<uintptr_t>(desc->dispatcher(handle,
                                                                 NATIVE_PLUGIN_OPCODE_GET_INTERNAL_HANDLE,
                                                                 0, 0, nullptr, 0.0f));
}

// --------------------------------------------------------------------------------------------------------------------

// testing purposes only
#if 0
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

    CarlaEngine* const engine = carla_get_native_plugin_engine(rack, handle);
    CARLA_SAFE_ASSERT_RETURN(engine != nullptr, 1);

    carla_stdout("Got Engine %p, %s, %i, %f",
                 engine, engine->getName(), engine->getBufferSize(), engine->getSampleRate());

    rack->cleanup(handle);
    return 0;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
