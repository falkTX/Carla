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

typedef void *(*JackThreadCallback)(void* arg);

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_nframes_t jack_thread_wait(jack_client_t*, int)
{
    return 0;
}

CARLA_EXPORT
jack_nframes_t jack_cycle_wait(jack_client_t*)
{
    return 0;
}

CARLA_EXPORT
void jack_cycle_signal(jack_client_t*, int)
{
}

CARLA_EXPORT
int jack_set_process_thread(jack_client_t*, JackThreadCallback, void*)
{
    return ENOSYS;
}

// --------------------------------------------------------------------------------------------------------------------
