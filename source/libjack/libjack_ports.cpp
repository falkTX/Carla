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
jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type,
                                unsigned long flags, unsigned long buffer_size)
{
    carla_debug("%s(%p, %s, %s, 0x%lx, %lu)", __FUNCTION__, client, port_name, port_type, flags, buffer_size);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    CARLA_SAFE_ASSERT_RETURN(port_name != nullptr && port_name[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(port_type != nullptr && port_type[0] != '\0', nullptr);

    const JackServerState& jserver(jclient->server);

    if (std::strcmp(port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            const std::size_t index = jclient->audioIns.count();
            JackPortState* const port = new JackPortState(jclient->name, port_name, index, flags,
                                                          false, false, index < jserver.numAudioIns);

            const CarlaMutexLocker cms(jclient->mutex);

            jclient->audioIns.append(port);
            return (jack_port_t*)port;
        }

        if (flags & JackPortIsOutput)
        {
            const std::size_t index = jclient->audioOuts.count();
            JackPortState* const port = new JackPortState(jclient->name, port_name, index, flags,
                                                          false, false, index < jserver.numAudioOuts);

            const CarlaMutexLocker cms(jclient->mutex);

            jclient->audioOuts.append(port);
            return (jack_port_t*)port;
        }

        carla_stderr2("jack_port_register: invalid port flags '%x'", flags);
        return nullptr;
    }

    if (std::strcmp(port_type, JACK_DEFAULT_MIDI_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            const std::size_t index = jclient->midiIns.count();
            JackPortState* const port = new JackPortState(jclient->name, port_name, index, flags,
                                                          true, false, index < jserver.numMidiIns);

            const CarlaMutexLocker cms(jclient->mutex);

            jclient->midiIns.append(port);
            return (jack_port_t*)port;
        }

        if (flags & JackPortIsOutput)
        {
            const std::size_t index = jclient->midiOuts.count();
            JackPortState* const port = new JackPortState(jclient->name, port_name, index, flags,
                                                          true, false, index < jserver.numMidiOuts);

            const CarlaMutexLocker cms(jclient->mutex);

            jclient->midiOuts.append(port);
            return (jack_port_t*)port;
        }

        carla_stderr2("jack_port_register: invalid port flags '%x'", flags);
        return nullptr;
    }

    carla_stderr2("jack_port_register: invalid port type '%s'", port_type);
    return nullptr;

    // unused
    (void)buffer_size;
}

CARLA_EXPORT
int jack_port_unregister(jack_client_t* client, jack_port_t* port)
{
    carla_debug("%s(%p, %p)", __FUNCTION__, client, port);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 1);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 1);
    CARLA_SAFE_ASSERT_RETURN(! jport->isSystem, 1);

    const CarlaMutexLocker cms(jclient->mutex);

    if (jport->isMidi)
    {
        if (jport->flags & JackPortIsInput)
        {
            CARLA_SAFE_ASSERT_RETURN(jclient->midiIns.removeOne(jport), 1);
            return 0;
        }

        if (jport->flags & JackPortIsOutput)
        {
            CARLA_SAFE_ASSERT_RETURN(jclient->midiOuts.removeOne(jport), 1);
            return 0;
        }
    }
    else
    {
        if (jport->flags & JackPortIsInput)
        {
            CARLA_SAFE_ASSERT_RETURN(jclient->audioIns.removeOne(jport), 1);
            return 0;
        }

        if (jport->flags & JackPortIsOutput)
        {
            CARLA_SAFE_ASSERT_RETURN(jclient->audioOuts.removeOne(jport), 1);
            return 0;
        }
    }

    carla_stderr2("jack_port_register: invalid port '%s'", jport->name);
    return 1;
}

CARLA_EXPORT
void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t)
{
    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->buffer;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
jack_uuid_t jack_port_uuid(const jack_port_t* port)
{
    carla_stderr2("%s(%p)", __FUNCTION__, port);
    return 0;
}

CARLA_EXPORT
const char* jack_port_name(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->fullname;
}

CARLA_EXPORT
const char* jack_port_short_name(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->name;
}

CARLA_EXPORT
int jack_port_flags(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->flags;
}

CARLA_EXPORT
const char* jack_port_type(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    static const char* const kAudioType = JACK_DEFAULT_AUDIO_TYPE;
    static const char* const kMidiType  = JACK_DEFAULT_MIDI_TYPE;

    return jport->isMidi ? kMidiType : kAudioType;
}

CARLA_EXPORT
uint32_t jack_port_type_id(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isMidi ? 1 : 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_port_is_mine(const jack_client_t*, const jack_port_t* port)
{
    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isSystem ? 0 : 1;
}

CARLA_EXPORT
int jack_port_connected(const jack_port_t* port)
{
    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isConnected ? 1 : 0;
}

CARLA_EXPORT
int jack_port_connected_to(const jack_port_t* port, const char* port_name)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, port_name);
    return 1;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
const char** jack_port_get_connections (const jack_port_t* port)
{
    carla_stderr2("%s(%p)", __FUNCTION__, port);
    return nullptr;
}

CARLA_EXPORT
const char** jack_port_get_all_connections(const jack_client_t* client, const jack_port_t* port)
{
    carla_stderr2("%s(%p, %p)", __FUNCTION__, client, port);
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
    carla_stderr2("%s(%p, %p)", __FUNCTION__, src, dst);
    return ENOSYS;
}

CARLA_EXPORT
int jack_port_untie(jack_port_t* port)
{
    carla_stderr2("%s(%p)", __FUNCTION__, port);
    return ENOSYS;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_port_set_name(jack_port_t *port, const char *port_name)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, port_name);
    return ENOSYS;
}

CARLA_EXPORT
int jack_port_rename(jack_client_t* client, jack_port_t *port, const char *port_name)
{
    carla_stderr2("%s(%p, %p, %s)", __FUNCTION__, client, port, port_name);
    return ENOSYS;
}

CARLA_EXPORT
int jack_port_set_alias(jack_port_t* port, const char* alias)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, alias);
    return ENOSYS;
}

CARLA_EXPORT
int jack_port_unset_alias(jack_port_t* port, const char* alias)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, alias);
    return ENOSYS;
}

CARLA_EXPORT
int jack_port_get_aliases(const jack_port_t*, char* aliases[2])
{
    aliases[0] = aliases[1] = nullptr;
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

//int jack_port_request_monitor (jack_port_t *port, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

//int jack_port_request_monitor_by_name (jack_client_t *client,
//                                       const char *port_name, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

//int jack_port_ensure_monitor (jack_port_t *port, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

//int jack_port_monitoring_input (jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_connect(jack_client_t* client, const char* a, const char* b)
{
    carla_stderr2("%s(%p, %s. %s)", __FUNCTION__, client, a, b);
    return 0;
}

CARLA_EXPORT
int jack_disconnect(jack_client_t* client, const char* a, const char* b)
{
    carla_stderr2("%s(%p, %s. %s)", __FUNCTION__, client, a, b);
    return 0;
}

CARLA_EXPORT
int jack_port_disconnect(jack_client_t* client, jack_port_t* port)
{
    carla_stderr2("%s(%p, %p)", __FUNCTION__, client, port);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_port_name_size(void)
{
    return STR_MAX;
}

// int jack_port_type_size(void) JACK_OPTIONAL_WEAK_EXPORT;

//size_t jack_port_type_get_buffer_size (jack_client_t *client, const char *port_type) JACK_WEAK_EXPORT;

// --------------------------------------------------------------------------------------------------------------------
