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
int jack_set_thread_init_callback(jack_client_t* client, JackThreadInitCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    return 0;
}

CARLA_EXPORT
void jack_on_shutdown(jack_client_t* client, JackShutdownCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr,);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->shutdownCb = callback;
    jclient->shutdownCbPtr = arg;
}

CARLA_EXPORT
void jack_on_info_shutdown(jack_client_t* client, JackInfoShutdownCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr,);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->infoShutdownCb = callback;
    jclient->infoShutdownCbPtr = arg;
}

CARLA_EXPORT
int jack_set_process_callback(jack_client_t* client, JackProcessCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->processCb = callback;
    jclient->processCbPtr = arg;
    return 0;
}

CARLA_EXPORT
int jack_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->freewheelCb = callback;
    jclient->freewheelCbPtr = arg;
    return 0;
}

CARLA_EXPORT
int jack_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->bufferSizeCb = callback;
    jclient->bufferSizeCbPtr = arg;
    return 0;
}

CARLA_EXPORT
int jack_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback callback, void* arg)
{
    carla_debug("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    jclient->sampleRateCb = callback;
    jclient->sampleRateCbPtr = arg;
    return 0;
}

CARLA_EXPORT
int jack_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

CARLA_EXPORT
int jack_set_port_registration_callback(jack_client_t* client, JackPortRegistrationCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

CARLA_EXPORT
int jack_set_port_connect_callback(jack_client_t* client, JackPortConnectCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

CARLA_EXPORT
int jack_set_port_rename_callback(jack_client_t* client, JackPortRenameCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

CARLA_EXPORT
int jack_set_graph_order_callback(jack_client_t* client, JackGraphOrderCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

CARLA_EXPORT
int jack_set_xrun_callback(jack_client_t* client, JackXRunCallback callback, void* arg)
{
    carla_stderr2("%s(%p, %p, %p)", __FUNCTION__, client, callback, arg);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
