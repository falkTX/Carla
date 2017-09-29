/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2017 Filipe Coelho <falktx@falktx.com>
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

#include "libjack.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_get_version(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr)
{
    *major_ptr = 1;
    *minor_ptr = 9;
    *micro_ptr = 12;
    *proto_ptr = 1;
}

CARLA_EXPORT
const char* jack_get_version_string(void)
{
    static const char* const kVersionStr = "1.9.12";
    return kVersionStr;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_is_realtime(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);

    return 1;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_free(void* ptr)
{
    carla_stdout("%s(%p)", __FUNCTION__, ptr);

    free(ptr);
}

// --------------------------------------------------------------------------------------------------------------------
