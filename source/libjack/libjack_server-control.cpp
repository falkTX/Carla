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

CARLA_EXPORT
int jack_set_freewheel(jack_client_t*, int)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
int jack_set_buffer_size(jack_client_t*, jack_nframes_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
jack_nframes_t jack_get_sample_rate(jack_client_t* client)
{
    carla_debug("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);

    return jstate.sampleRate;
}

CARLA_EXPORT
jack_nframes_t jack_get_buffer_size(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);

    return jstate.bufferSize;
}

CARLA_EXPORT
int jack_engine_takeover_timebase(jack_client_t*)
{
    return ENOSYS;
}

CARLA_EXPORT
float jack_cpu_load(jack_client_t*)
{
    return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------
