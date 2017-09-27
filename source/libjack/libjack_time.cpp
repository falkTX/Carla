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

//jack_nframes_t jack_frames_since_cycle_start (const jack_client_t *) JACK_OPTIONAL_WEAK_EXPORT;

CARLA_EXPORT
jack_nframes_t jack_frame_time(const jack_client_t* client)
{
    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return jclient->server.position.usecs;
}

// jack_nframes_t jack_last_frame_time (const jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_get_cycle_times(const jack_client_t *client,
//                         jack_nframes_t *current_frames,
//                         jack_time_t    *current_usecs,
//                         jack_time_t    *next_usecs,
//                         float          *period_usecs) JACK_OPTIONAL_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

// jack_time_t jack_frames_to_time(const jack_client_t *client, jack_nframes_t) JACK_OPTIONAL_WEAK_EXPORT;
// jack_nframes_t jack_time_to_frames(const jack_client_t *client, jack_time_t) JACK_OPTIONAL_WEAK_EXPORT;
// jack_time_t jack_get_time(void) JACK_OPTIONAL_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------
