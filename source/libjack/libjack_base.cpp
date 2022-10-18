/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2022 Filipe Coelho <falktx@falktx.com>
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

// ---------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_carla_interposed_action(uint, uint, void*)
{
    static bool printWarning = true;

    if (printWarning)
    {
        printWarning = false;
        carla_stderr2("Non-exported jack_carla_interposed_action called, this should not happen!!");
        carla_stderr("Printing some info:");
       #ifdef CARLA_OS_MAC
        carla_stderr("\tDYLD_LIBRARY_PATH:         '%s'", std::getenv("DYLD_LIBRARY_PATH"));
        carla_stderr("\tDYLD_INSERT_LIBRARIES:     '%s'", std::getenv("DYLD_INSERT_LIBRARIES"));
        carla_stderr("\tDYLD_FORCE_FLAT_NAMESPACE: '%s'", std::getenv("DYLD_FORCE_FLAT_NAMESPACE"));
       #else
        carla_stderr("\tLD_LIBRARY_PATH: '%s'", std::getenv("LD_LIBRARY_PATH"));
        carla_stderr("\tLD_PRELOAD:      '%s'", std::getenv("LD_PRELOAD"));
       #endif
        std::fflush(stderr);
    }

    // ::kill(::getpid(), SIGKILL);
    return 1337;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
void jack_get_version(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr)
{
    carla_debug("%s(%p, %p, %p, %p)", __FUNCTION__, major_ptr, minor_ptr, micro_ptr, proto_ptr);

    *major_ptr = 1;
    *minor_ptr = 9;
    *micro_ptr = 12;
    *proto_ptr = 9; // FIXME
}

CARLA_PLUGIN_EXPORT
const char* jack_get_version_string(void)
{
    carla_debug("%s()", __FUNCTION__);

    static const char* const kVersionStr = "1.9.12";
    return kVersionStr;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_is_realtime(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);
    return 1;

    // unused
    (void)client;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
void jack_free(void* ptr)
{
    carla_debug("%s(%p)", __FUNCTION__, ptr);

    std::free(ptr);
}

// --------------------------------------------------------------------------------------------------------------------
