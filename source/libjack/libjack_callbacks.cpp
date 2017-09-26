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
int jack_set_thread_init_callback(jack_client_t*, JackThreadInitCallback, void*)
{
    return 0;
}

CARLA_EXPORT
void jack_on_shutdown(jack_client_t* client, JackShutdownCallback callback, void* arg)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr,);

    JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated,);

    jstate.shutdown = callback;
    jstate.shutdownPtr = arg;
}

// void jack_on_info_shutdown (jack_client_t *client,
//                             JackInfoShutdownCallback shutdown_callback, void *arg) JACK_WEAK_EXPORT;

CARLA_EXPORT
int jack_set_process_callback(jack_client_t* client, JackProcessCallback callback, void* arg)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

    jstate.process = callback;
    jstate.processPtr = arg;

    return 0;
}

// int jack_set_freewheel_callback (jack_client_t *client,
//                                  JackFreewheelCallback freewheel_callback,
//                                  void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_buffer_size_callback (jack_client_t *client,
//                                    JackBufferSizeCallback bufsize_callback,
//                                    void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_sample_rate_callback (jack_client_t *client,
//                                    JackSampleRateCallback srate_callback,
//                                    void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_client_registration_callback (jack_client_t *client,
//                                             JackClientRegistrationCallback
//                                             registration_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;


// int jack_set_port_registration_callback (jack_client_t *client,
//                                           JackPortRegistrationCallback
//                                           registration_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_port_connect_callback (jack_client_t *client,
//                                     JackPortConnectCallback
//                                     connect_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_port_rename_callback (jack_client_t *client,
//                                    JackPortRenameCallback
//                                    rename_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

// int jack_set_graph_order_callback (jack_client_t *client,
//                                    JackGraphOrderCallback graph_callback,
//                                    void *) JACK_OPTIONAL_WEAK_EXPORT;

CARLA_EXPORT
int jack_set_xrun_callback(jack_client_t*, JackXRunCallback, void*)
{
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
