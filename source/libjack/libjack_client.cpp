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

static CarlaJackClient gClient;
static int             gClientRefCount = 0;

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_client_t* jack_client_open(const char* client_name, jack_options_t /*options*/, jack_status_t* status, ...)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    if (! gClient.initIfNeeded(client_name))
    {
        if (status != nullptr)
            *status = JackServerError;
        return nullptr;
    }

    ++gClientRefCount;
    return (jack_client_t*)&gClient;
}

CARLA_EXPORT
jack_client_t* jack_client_new(const char* client_name)
{
    return jack_client_open(client_name, JackNullOption, nullptr);
}

CARLA_EXPORT
int jack_client_close(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackClientState& jstate(jclient->fState);

    if (jstate.activated)
        jclient->deactivate();

    --gClientRefCount;
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_client_name_size(void)
{
    return STR_MAX;
}

CARLA_EXPORT
char* jack_get_client_name(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackClientState& jstate(jclient->fState);

    return jstate.name;
}

CARLA_EXPORT
char* jack_get_uuid_for_client_name(jack_client_t*, const char*)
{
    return nullptr;
}

CARLA_EXPORT
char* jack_get_client_name_by_uuid (jack_client_t*, const char*)
{
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_internal_client_new(const char*, const char*, const char*)
{
    return 1;
}

CARLA_EXPORT
void jack_internal_client_close(const char*)
{
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_activate(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(! jstate.activated, 1);

#if 0
    // needed for pulseaudio
    static bool skipFirstActivate = true;
    if (skipFirstActivate) {
        skipFirstActivate = false;
        return 0;
    }
#endif

    jclient->activate();
    return 0;
}

CARLA_EXPORT
int jack_deactivate(jack_client_t* /*client*/)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    //CarlaJackClient* const jclient = (CarlaJackClient*)client;
    //CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    //JackClientState& jstate(jclient->fState);
    //CARLA_SAFE_ASSERT_RETURN(jstate.activated, 1);

    //jclient->deactivate();
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_get_client_pid(const char*)
{
    return 0;
}

CARLA_EXPORT
pthread_t jack_client_thread_id(jack_client_t* client)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    CarlaJackClient* const jclient = (CarlaJackClient*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackClientState& jstate(jclient->fState);
    CARLA_SAFE_ASSERT_RETURN(jstate.activated, 0);

    return (pthread_t)jclient->getThreadId();
}

// --------------------------------------------------------------------------------------------------------------------
