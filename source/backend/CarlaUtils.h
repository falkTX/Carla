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
 * Get the juce version used in the current Carla build.
 */
CARLA_EXPORT void carla_set_process_name(const char* name);

/*!
 * Get the current carla library filename.
 */
CARLA_EXPORT const char* carla_get_library_filename();

/*!
 * Get the folder where the current use carla library resides.
 */
CARLA_EXPORT const char* carla_get_library_folder();

/** @} */

#endif /* CARLA_UTILS_H_INCLUDED */
