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

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_set_freewheel(jack_client_t* client, int freewheel)
{
    carla_debug("%s(%p, %i)", __FUNCTION__, client, freewheel);
    return ENOSYS;

    // unused
    (void)client;
    (void)freewheel;
}

CARLA_PLUGIN_EXPORT
int jack_set_buffer_size(jack_client_t* client, jack_nframes_t nframes)
{
    carla_debug("%s(%p, %u)", __FUNCTION__, client, nframes);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return (jclient->server.bufferSize == nframes) ? 0 : 1;
}

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_get_sample_rate(jack_client_t* client)
{
    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return static_cast<jack_nframes_t>(jclient->server.sampleRate);
}

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_get_buffer_size(jack_client_t* client)
{
    carla_debug("%s(%p)", __FUNCTION__, client);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return jclient->server.bufferSize;
}

CARLA_PLUGIN_EXPORT
float jack_cpu_load(jack_client_t*)
{
    // TODO
    return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------
