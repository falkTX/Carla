/*
 * Carla Plugin Host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"

#ifdef STATIC_PLUGIN_TARGET
# undef CARLA_PLUGIN_EXPORT
# define CARLA_PLUGIN_EXPORT CARLA_API_EXPORT
#endif

#include "CarlaNative.h"
#include "CarlaHost.h"
#include "CarlaUtils.h"

/*!
 * Get the native plugin descriptor for the carla-rack plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_rack_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay16 plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay16_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay32 plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay32_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay64 plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay64_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay-cv plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_cv_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay-cv8 plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_cv8_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay-cv32 plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_cv32_plugin(void);

/*!
 * Get the native plugin descriptor for the carla-patchbay OBS plugin.
 */
CARLA_API_EXPORT const NativePluginDescriptor* carla_get_native_patchbay_obs_plugin(void);

/*!
 * Create a CarlaHostHandle suitable for CarlaHost API calls.
 * Returned value must be freed by the caller when no longer needed.
 */
CARLA_API_EXPORT CarlaHostHandle carla_create_native_plugin_host_handle(const NativePluginDescriptor* desc,
                                                                        NativePluginHandle handle);

/*!
 * Free memory created during carla_create_native_plugin_host_handle.
 */
CARLA_API_EXPORT void carla_host_handle_free(CarlaHostHandle handle);

#ifdef __cplusplus
/*!
 * Get the internal CarlaEngine instance.
 * @deprecated Please use carla_create_native_plugin_host_handle instead
 */
CARLA_API_EXPORT CARLA_BACKEND_NAMESPACE::CarlaEngine* carla_get_native_plugin_engine(const NativePluginDescriptor* desc,
                                                                                      NativePluginHandle handle);
#endif

#endif /* CARLA_NATIVE_PLUGIN_H_INCLUDED */
