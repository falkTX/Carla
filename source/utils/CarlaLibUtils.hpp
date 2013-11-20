/*
 * Carla library utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LIB_UTILS_HPP_INCLUDED
#define CARLA_LIB_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------
// library related calls

/*
 * Open 'filename' library (must not be null).
 * May return null, in which case "lib_error" has the error.
 */
void* lib_open(const char* const filename);

/*
 * Close a previously opened library (must not be null).
 * If false is returned,"lib_error" has the error.
 */
bool lib_close(void* const lib);

/*
 * Get a library symbol (must not be null).
 * May return null if the symbol is not found.
 */
void* lib_symbol(void* const lib, const char* const symbol);

/*
 * Return the last operation error ('filename' must not be null).
 */
const char* lib_error(const char* const filename);

// -----------------------------------------------------------------------

#endif // CARLA_LIB_UTILS_HPP_INCLUDED
