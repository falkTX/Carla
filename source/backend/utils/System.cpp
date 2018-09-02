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

#include "CarlaUtils.h"

#include "CarlaThread.hpp"

// -------------------------------------------------------------------------------------------------------------------

void carla_fflush(bool err)
{
    std::fflush(err ? stderr : stdout);
}

void carla_fputs(bool err, const char* string)
{
    std::fputs(string, err ? stderr : stdout);
}

void carla_set_process_name(const char* name)
{
    carla_debug("carla_set_process_name(\"%s\")", name);

    CarlaThread::setCurrentThreadName(name);
}

// -------------------------------------------------------------------------------------------------------------------
