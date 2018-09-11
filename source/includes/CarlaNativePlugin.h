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

#ifndef CARLA_NATIVE_PLUGIN_H_INCLUDED
#define CARLA_NATIVE_PLUGIN_H_INCLUDED

#include "CarlaBackend.h"
#include "CarlaNative.h"
#include "CarlaEngine.hpp"

/*!
 * Get the folder where this carla library resides.
 */
CARLA_EXPORT const NativePluginDescriptor* carla_get_native_rack_plugin();

/*!
 * Get the folder where this carla library resides.
 */
CARLA_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_plugin();

#ifdef __cplusplus
CARLA_BACKEND_USE_NAMESPACE

static inline
CarlaEngine* carla_plugin_get_engine(const NativePluginDescriptor* desc, NativePluginHandle handle)
{
    CARLA_SAFE_ASSERT_RETURN(desc != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    const intptr_t enginePtr = desc->dispatcher(handle, NATIVE_PLUGIN_OPCODE_GET_INTERNAL_HANDLE, 0, 0, nullptr, 0.0f);
    CARLA_SAFE_ASSERT_RETURN(enginePtr != 0, nullptr);

    return (CarlaEngine*)static_cast<uintptr_t>(enginePtr);
}
#endif

#endif /* CARLA_NATIVE_PLUGIN_H_INCLUDED */
