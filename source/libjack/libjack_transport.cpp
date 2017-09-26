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

// int  jack_release_timebase (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

// int  jack_set_sync_callback (jack_client_t *client,
//                              JackSyncCallback sync_callback,
//                              void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int  jack_set_sync_timeout (jack_client_t *client,
//                             jack_time_t timeout) JACK_OPTIONAL_WEAK_EXPORT;

// int  jack_set_timebase_callback (jack_client_t *client,
//                                  int conditional,
//                                  JackTimebaseCallback timebase_callback,
//                                  void *arg) JACK_OPTIONAL_WEAK_EXPORT;

CARLA_EXPORT
int jack_transport_locate(jack_client_t*, jack_nframes_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return 1;
}

CARLA_EXPORT
jack_transport_state_t jack_transport_query(const jack_client_t* client, jack_position_t* pos)
{
    carla_debug("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, JackTransportStopped);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, JackTransportStopped);

    if (pos != nullptr)
        std::memcpy(pos, &jstate.position, sizeof(jack_position_t));

    return jstate.playing ? JackTransportRolling : JackTransportStopped;
}

// jack_nframes_t jack_get_current_transport_frame (const jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

CARLA_EXPORT
int jack_transport_reposition(jack_client_t*, const jack_position_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return EINVAL;
}

CARLA_EXPORT
void jack_transport_start(jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

}

CARLA_EXPORT
void jack_transport_stop (jack_client_t*)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

}

// void jack_get_transport_info (jack_client_t *client,
//                               jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_EXPORT;

// void jack_set_transport_info (jack_client_t *client,
//                               jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------
