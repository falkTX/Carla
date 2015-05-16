/*
 * Carla Print Tests
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.hpp"

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

// -----------------------------------------------------------------------

int main()
{
    carla_debug("DEBUG");
    carla_stdout("STDOUT");
    carla_stderr("STDERR");
    carla_stderr2("STDERR2");

    carla_stdout("char      %c",  (char)'0');
    carla_stdout("int       %i",  (int)0);
    carla_stdout("uint      %u",  (uint)0);
    carla_stdout("long      %li", (long)0);
    carla_stdout("ulong     %lu", (ulong)0);
    carla_stdout("float     %.0f", 0.0f);
    carla_stdout("double    %g",   0.0);
    carla_stdout("int64     " P_INT64,   (int64_t)0);
    carla_stdout("uint64    " P_UINT64,  (uint64_t)0);
    carla_stdout("intptr    " P_INTPTR,  (intptr_t)0);
    carla_stdout("uintptr   " P_UINTPTR, (uintptr_t)0);
    carla_stdout("size      " P_SIZE,    (size_t)0);
    carla_stdout("std::size " P_SIZE,    (std::size_t)0);
    return 0;
}

// -----------------------------------------------------------------------
