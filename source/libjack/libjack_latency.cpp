/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2018 Filipe Coelho <falktx@falktx.com>
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

int jack_set_latency_callback(jack_client_t* client, JackLatencyCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
    carla_debug("%s(%p, %u, %p)", __FUNCTION__, port, mode, range);

    range->min = range->max = 0;
    return;

    // unused
    (void)port;
    (void)mode;
}

CARLA_EXPORT
void jack_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
    carla_stderr2("%s(%p, %u, %p)", __FUNCTION__, port, mode, range);
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_recompute_total_latencies(jack_client_t* client)
{
    carla_stderr2("%s(%p)", __FUNCTION__, client);
    return 0;
}

CARLA_EXPORT
jack_nframes_t jack_port_get_latency(jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    if (jport->isMidi || ! jport->isSystem)
        return 0;

    // TODO
    const uint32_t bufferSize = 128;
    const uint32_t latencyMultiplier = 3;

    if (jport->flags & JackPortIsInput)
        return bufferSize*latencyMultiplier;
    if (jport->flags & JackPortIsOutput)
        return bufferSize;

    return 0;
}

CARLA_EXPORT
jack_nframes_t jack_port_get_total_latency(jack_client_t* client, jack_port_t* port)
{
    carla_debug("%s(%p, %p)", __FUNCTION__, client, port);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    if (jport->isMidi)
        return 0;

    // TODO
    const uint32_t bufferSize = jclient->server.bufferSize;
    const uint32_t latencyMultiplier = 3;

    if (jport->isSystem)
    {
        if (jport->flags & JackPortIsInput)
            return bufferSize*latencyMultiplier;
        if (jport->flags & JackPortIsOutput)
            return bufferSize;
    }
    else
    {
        if (jport->flags & JackPortIsInput)
            return bufferSize;
        if (jport->flags & JackPortIsOutput)
            return bufferSize*latencyMultiplier;
    }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// deprecated calls

CARLA_EXPORT
void jack_port_set_latency(jack_port_t* port, jack_nframes_t nframes)
{
    carla_stderr2("%s(%p, %u)", __FUNCTION__, port, nframes);
}

CARLA_EXPORT
int jack_recompute_total_latency(jack_client_t* client, jack_port_t* port)
{
    carla_stderr2("%s(%p, %p)", __FUNCTION__, client, port);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
