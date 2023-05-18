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

#include "CarlaUtils.h"

#ifndef CARLA_OS_WASM
# include "CarlaThread.hpp"
#else
# include "CarlaUtils.hpp"
#endif

// -------------------------------------------------------------------------------------------------------------------

void carla_fflush(const bool err)
{
    std::fflush(err ? stderr : stdout);
}

void carla_fputs(const bool err, const char* const string)
{
    std::fputs(string, err ? stderr : stdout);
}

void carla_set_process_name(const char* const name)
{
    carla_debug("carla_set_process_name(\"%s\")", name);

#ifndef CARLA_OS_WASM
    CarlaThread::setCurrentThreadName(name);
#else
    // unused
    (void)name;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
