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

// need to include this first
#include "libjack.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

// int jack_set_latency_callback (jack_client_t *client,
//                                JackLatencyCallback latency_callback,
//                                void *) JACK_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

//void jack_port_set_latency (jack_port_t *port, jack_nframes_t) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_port_get_latency_range(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t* range)
{
    range->min = range->max = 0;
}

//void jack_port_set_latency_range (jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range) JACK_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

//int jack_recompute_total_latencies (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

CARLA_EXPORT
jack_nframes_t jack_port_get_latency(jack_port_t*)
{
    return 0;
}

//jack_nframes_t jack_port_get_total_latency (jack_client_t *client,
//                                            jack_port_t *port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

//int jack_recompute_total_latency (jack_client_t*, jack_port_t* port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

// --------------------------------------------------------------------------------------------------------------------
