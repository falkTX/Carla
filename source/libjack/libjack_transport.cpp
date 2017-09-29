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
int jack_engine_takeover_timebase(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);

    return ENOSYS;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_release_timebase(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);

    return 0;
}

CARLA_EXPORT
int jack_set_sync_callback(jack_client_t* client, JackSyncCallback callback, void* arg)
{
    carla_stdout("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->syncCb = callback;
    jclient->syncCbPtr = arg;
    return 0;
}

CARLA_EXPORT
int jack_set_sync_timeout(jack_client_t* client, jack_time_t timeout)
{
    carla_stdout("%s(%p, " P_UINT64 ")", __FUNCTION__, client, timeout);

    return 0;
}

CARLA_EXPORT
int jack_set_timebase_callback(jack_client_t* client, int conditional, JackTimebaseCallback callback, void* arg)
{
    carla_stdout("%s(%p, %i, %p, %p)", __FUNCTION__, client, conditional, callback, arg);

    return EBUSY;
}

CARLA_EXPORT
int jack_transport_locate(jack_client_t* client, jack_nframes_t frame)
{
    carla_stdout("%s(%p, %u)", __FUNCTION__, client, frame);

    return ENOSYS;
}

CARLA_EXPORT
jack_transport_state_t jack_transport_query(const jack_client_t* client, jack_position_t* pos)
{
    if (const JackClientState* const jclient = (JackClientState*)client)
    {
        const JackServerState& jserver(jclient->server);

        if (pos != nullptr)
            std::memcpy(pos, &jserver.position, sizeof(jack_position_t));

        return jserver.playing ? JackTransportRolling : JackTransportStopped;
    }

    if (pos != nullptr)
        std::memset(pos, 0, sizeof(jack_position_t));

    return JackTransportStopped;
}

CARLA_EXPORT
jack_nframes_t jack_get_current_transport_frame(const jack_client_t* client)
{
    if (const JackClientState* const jclient = (JackClientState*)client)
        return jclient->server.position.frame;

    return 0;
}

CARLA_EXPORT
int jack_transport_reposition(jack_client_t* client, const jack_position_t* pos)
{
    carla_stdout("%s(%p, %p)", __FUNCTION__, client, pos);

    return ENOSYS;
}

CARLA_EXPORT
void jack_transport_start(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);
}

CARLA_EXPORT
void jack_transport_stop(jack_client_t* client)
{
    carla_stdout("%s(%p)", __FUNCTION__, client);
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
void jack_get_transport_info(jack_client_t* client, void* tinfo)
{
    carla_stdout("%s(%p, %p)", __FUNCTION__, client, tinfo);
}

CARLA_EXPORT
void jack_set_transport_info(jack_client_t* client, void* tinfo)
{
    carla_stdout("%s(%p, %p)", __FUNCTION__, client, tinfo);
}

// --------------------------------------------------------------------------------------------------------------------
