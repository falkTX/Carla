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
jack_nframes_t jack_frames_since_cycle_start(const jack_client_t* client)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    // TODO
    return 0;
}

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_frame_time(const jack_client_t* client)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    // FIXME this is wrong
    return static_cast<jack_nframes_t>(jclient->server.position.usecs);
}

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_last_frame_time(const jack_client_t* client)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return static_cast<jack_nframes_t>(jclient->server.monotonic_frame);
}

CARLA_PLUGIN_EXPORT
int jack_get_cycle_times(const jack_client_t *client,
                         jack_nframes_t *current_frames,
                         jack_time_t    *current_usecs,
                         jack_time_t    *next_usecs,
                         float          *period_usecs)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    /* current_frames: the frame time counter at the start of the current cycle, same as jack_last_frame_time().
     * current_usecs:  the microseconds time at the start of the current cycle.
     * next_usecs:     the microseconds time of the start of the next next cycle as computed by the DLL.
     * period_usecs:   the current best estimate of the period time in microseconds.
     */
    const double _period_usecs = static_cast<double>(jclient->server.bufferSize) / jclient->server.sampleRate;

    *current_frames = jclient->server.monotonic_frame;
    *current_usecs = jclient->server.position.usecs;
    *next_usecs = jclient->server.position.usecs + static_cast<jack_time_t>(_period_usecs + 0.5);
    *period_usecs = static_cast<float>(_period_usecs);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
jack_time_t jack_frames_to_time(const jack_client_t* client, jack_nframes_t frames)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return static_cast<jack_time_t>(static_cast<double>(frames) / jclient->server.sampleRate * 1000000.0);
}

CARLA_PLUGIN_EXPORT
jack_nframes_t jack_time_to_frames(const jack_client_t* client, jack_time_t time)
{
    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    return static_cast<jack_nframes_t>(static_cast<double>(time) / 1000000.0 * jclient->server.sampleRate);
}

CARLA_PLUGIN_EXPORT
jack_time_t jack_get_time(void)
{
    timespec t;
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
#else
    clock_gettime(CLOCK_MONOTONIC, &t);
#endif
    return static_cast<jack_time_t>(t.tv_sec * 1000000 + t.tv_nsec / 1000);
}

// --------------------------------------------------------------------------------------------------------------------
