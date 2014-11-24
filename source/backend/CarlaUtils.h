/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_UTILS_H_INCLUDED
#define CARLA_UTILS_H_INCLUDED

#include "CarlaDefines.h"

/*!
 * @defgroup CarlaUtilsAPI Carla Utils API
 *
 * The Carla Utils API.
 *
 * This API allows to call advanced features from Python.
 * @{
 */

/*!
 * TODO.
 */
typedef void* CarlaPipeClientHandle;

/*!
 * TODO.
 */
typedef void (*CarlaPipeCallbackFunc)(void* ptr, const char* msg);

/*!
 * Get the current carla library filename.
 */
CARLA_EXPORT const char* carla_get_library_filename();

/*!
 * Get the folder where the current use carla library resides.
 */
CARLA_EXPORT const char* carla_get_library_folder();

/*!
 * TODO.
 */
CARLA_EXPORT void carla_set_locale_C();

/*!
 * Get the juce version used in the current Carla build.
 */
CARLA_EXPORT void carla_set_process_name(const char* name);

/*!
 * TODO.
 */
CARLA_EXPORT CarlaPipeClientHandle carla_pipe_client_new(const char* argv[], CarlaPipeCallbackFunc callbackFunc, void* callbackPtr);

/*!
 * TODO.
 */
CARLA_EXPORT void carla_pipe_client_idle(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT bool carla_pipe_client_is_running(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT void carla_pipe_client_lock(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT void carla_pipe_client_unlock(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT const char* carla_pipe_client_readlineblock(CarlaPipeClientHandle handle, uint timeout);

/*!
 * TODO.
 */
CARLA_EXPORT bool carla_pipe_client_write_msg(CarlaPipeClientHandle handle, const char* msg);

/*!
 * TODO.
 */
CARLA_EXPORT bool carla_pipe_client_write_and_fix_msg(CarlaPipeClientHandle handle, const char* msg);

/*!
 * TODO.
 */
CARLA_EXPORT bool carla_pipe_client_flush(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT bool carla_pipe_client_flush_and_unlock(CarlaPipeClientHandle handle);

/*!
 * TODO.
 */
CARLA_EXPORT void carla_pipe_client_destroy(CarlaPipeClientHandle handle);

/** @} */

#endif /* CARLA_UTILS_H_INCLUDED */
