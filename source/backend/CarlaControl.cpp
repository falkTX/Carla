/*
 * Carla Control
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaString.hpp"

// -------------------------------------------------------------------------------------------------------------------
// Single, control engine

struct CarlaBackendControl {
    CarlaString lastError;

    CarlaBackendControl() noexcept
        : lastError() {}

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_STRUCT(CarlaBackendControl)
};

CarlaBackendControl gControl;

// -------------------------------------------------------------------------------------------------------------------
// API

#define CARLA_COMMON_NEED_CHECKSTRINGPTR
#include "CarlaHostCommon.cpp"

// -------------------------------------------------------------------------------------------------------------------

uint carla_get_engine_driver_count()
{
    return 0;
}

const char* carla_get_engine_driver_name(uint)
{
    return nullptr;
}

const char* const* carla_get_engine_driver_device_names(uint)
{
    return nullptr;
}

const EngineDriverDeviceInfo* carla_get_engine_driver_device_info(uint, const char*)
{
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CarlaEngine* carla_get_engine()
{
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_engine_init(const char*, const char*)
{
    return false;
}

bool carla_engine_close()
{
    return false;
}

void carla_engine_idle()
{
}

bool carla_is_engine_running()
{
    return false;
}

void carla_set_engine_about_to_close()
{
}

void carla_set_engine_callback(EngineCallbackFunc, void*)
{
}

void carla_set_engine_option(EngineOption, int, const char*)
{
}

void carla_set_file_callback(FileCallbackFunc, void*)
{
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_load_file(const char*)
{
    return false;
}

bool carla_load_project(const char*)
{
    return false;
}

bool carla_save_project(const char*)
{
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

bool carla_patchbay_connect(uint, uint, uint, uint)
{
    return false;
}

bool carla_patchbay_disconnect(uint)
{
    return false;
}

bool carla_patchbay_refresh(bool)
{
    return false;
}

// -------------------------------------------------------------------------------------------------------------------

void carla_transport_play()
{
}

void carla_transport_pause()
{
}

void carla_transport_relocate(uint64_t)
{
}

uint64_t carla_get_current_transport_frame()
{
    return 0;
}

const CarlaTransportInfo* carla_get_transport_info()
{
    static CarlaTransportInfo retInfo;

    // reset
    retInfo.playing = false;
    retInfo.frame   = 0;
    retInfo.bar     = 0;
    retInfo.beat    = 0;
    retInfo.tick    = 0;
    retInfo.bpm     = 0.0;

    return &retInfo;
}

// -------------------------------------------------------------------------------------------------------------------

uint32_t carla_get_current_plugin_count()
{
    return 0;
}

uint32_t carla_get_max_plugin_number()
{
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
