/*
 * Carla shared memory utils
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_MEM_UTILS_HPP_INCLUDED
#define CARLA_MEM_UTILS_HPP_INCLUDED

#include "CarlaUtils.hpp"

#if !defined(CARLA_OS_WASM) && !defined(CARLA_OS_WIN)
# include <sys/mman.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

static inline
bool carla_mlock(void* const ptr, const size_t size)
{
   #if defined(CARLA_OS_WASM)
    // unsupported
    return false;
    (void)ptr; (void)size;
   #elif defined(CARLA_OS_WIN)
    return ::VirtualLock(ptr, size) != FALSE;
   #else
    return ::mlock(ptr, size) == 0;
   #endif
}

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_MEM_UTILS_HPP_INCLUDED
