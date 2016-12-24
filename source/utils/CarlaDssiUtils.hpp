/*
 * Carla DSSI utils
 * Copyright (C) 2013-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_DSSI_UTILS_HPP_INCLUDED
#define CARLA_DSSI_UTILS_HPP_INCLUDED

#include "CarlaLadspaUtils.hpp"
#include "dssi/dssi.h"

// --------------------------------------------------------------------------------------------------------------------
// Find UI binary for a plugin (returned value must be deleted)

const char* find_dssi_ui(const char* const filename, const char* const label) noexcept;

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_DSSI_UTILS_HPP_INCLUDED
