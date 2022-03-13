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

static uint32_t gPortId = JackPortState::kPortIdOffsetUser;

CARLA_PLUGIN_EXPORT
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
            const uint gid = ++gPortId;
            JackPortState* const port = new JackPortState(jclient->name, port_name,
                                                          static_cast<uint>(index),
                                                          static_cast<int>(flags),
                                                          gid,
                                                          false, false, index < jserver.numAudioIns);

            {
                const CarlaMutexLocker cms(jclient->mutex);
                jclient->audioIns.append(port);
            }

            jclient->portIdMapping[gid] = port;
            jclient->portNameMapping[port->fullname] = port;
            return (jack_port_t*)port;
        }

        if (flags & JackPortIsOutput)
        {
            const std::size_t index = jclient->audioOuts.count();
            const uint gid = ++gPortId;
            JackPortState* const port = new JackPortState(jclient->name, port_name,
                                                          static_cast<uint>(index),
                                                          static_cast<int>(flags),
                                                          gid,
                                                          false, false, index < jserver.numAudioOuts);

            {
                const CarlaMutexLocker cms(jclient->mutex);
                jclient->audioOuts.append(port);
            }

            jclient->portIdMapping[gid] = port;
            jclient->portNameMapping[port->fullname] = port;
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
            const uint gid = ++gPortId;
            JackPortState* const port = new JackPortState(jclient->name, port_name,
                                                          static_cast<uint>(index),
                                                          static_cast<int>(flags),
                                                          gid,
                                                          true, false, index < jserver.numMidiIns);

            {
                const CarlaMutexLocker cms(jclient->mutex);
                jclient->midiIns.append(port);
            }

            jclient->portIdMapping[gid] = port;
            jclient->portNameMapping[port->fullname] = port;
            return (jack_port_t*)port;
        }

        if (flags & JackPortIsOutput)
        {
            const std::size_t index = jclient->midiOuts.count();
            const uint gid = ++gPortId;
            JackPortState* const port = new JackPortState(jclient->name, port_name,
                                                          static_cast<uint>(index),
                                                          static_cast<int>(flags),
                                                          gid,
                                                          true, false, index < jserver.numMidiOuts);

            {
                const CarlaMutexLocker cms(jclient->mutex);
                jclient->midiOuts.append(port);
            }

            jclient->portIdMapping[gid] = port;
            jclient->portNameMapping[port->fullname] = port;
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

CARLA_PLUGIN_EXPORT
int jack_port_unregister(jack_client_t* client, jack_port_t* port)
{
    carla_debug("%s(%p, %p)", __FUNCTION__, client, port);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, EINVAL);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, EINVAL);
    CARLA_SAFE_ASSERT_RETURN(! jport->isSystem, EINVAL);

    {
        const CarlaMutexLocker cms(jclient->mutex);

        if (jport->isMidi)
        {
            if (jport->flags & JackPortIsInput)
            {
                CARLA_SAFE_ASSERT_RETURN(jclient->midiIns.removeOne(jport), ENOENT);
                jclient->portIdMapping.erase(jport->gid);
                jclient->portNameMapping.erase(jport->fullname);
                return 0;
            }

            if (jport->flags & JackPortIsOutput)
            {
                CARLA_SAFE_ASSERT_RETURN(jclient->midiOuts.removeOne(jport), ENOENT);
                jclient->portIdMapping.erase(jport->gid);
                jclient->portNameMapping.erase(jport->fullname);
                return 0;
            }
        }
        else
        {
            if (jport->flags & JackPortIsInput)
            {
                CARLA_SAFE_ASSERT_RETURN(jclient->audioIns.removeOne(jport), ENOENT);
                jclient->portIdMapping.erase(jport->gid);
                jclient->portNameMapping.erase(jport->fullname);
                return 0;
            }

            if (jport->flags & JackPortIsOutput)
            {
                CARLA_SAFE_ASSERT_RETURN(jclient->audioOuts.removeOne(jport), ENOENT);
                jclient->portIdMapping.erase(jport->gid);
                jclient->portNameMapping.erase(jport->fullname);
                return 0;
            }
        }
    }

    carla_stderr2("jack_port_register: invalid port '%s'", jport->name);
    return EINVAL;
}

CARLA_PLUGIN_EXPORT
void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t)
{
    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->buffer;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
jack_uuid_t jack_port_uuid(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->uuid;
}

CARLA_PLUGIN_EXPORT
const char* jack_port_name(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->fullname;
}

CARLA_PLUGIN_EXPORT
const char* jack_port_short_name(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    return jport->name;
}

CARLA_PLUGIN_EXPORT
int jack_port_flags(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->flags;
}

CARLA_PLUGIN_EXPORT
const char* jack_port_type(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);

    static const char* const kAudioType = JACK_DEFAULT_AUDIO_TYPE;
    static const char* const kMidiType  = JACK_DEFAULT_MIDI_TYPE;

    return jport->isMidi ? kMidiType : kAudioType;
}

CARLA_PLUGIN_EXPORT
uint32_t jack_port_type_id(const jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isMidi ? 1 : 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_port_is_mine(const jack_client_t*, const jack_port_t* port)
{
    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isSystem ? 0 : 1;
}

CARLA_PLUGIN_EXPORT
int jack_port_connected(const jack_port_t* port)
{
    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    return jport->isConnected ? 1 : 0;
}

CARLA_PLUGIN_EXPORT
int jack_port_connected_to(const jack_port_t* port, const char* port_name)
{
    carla_stderr2("%s(%p, %s) WIP", __FUNCTION__, port, port_name);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, 0);

    if (! jport->isConnected)
        return 0;

    // TODO
    return 1;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_port_tie(jack_port_t* src, jack_port_t* dst)
{
    carla_debug("%s(%p, %p)", __FUNCTION__, src, dst);
    return ENOSYS;

    // unused
    (void)src;
    (void)dst;
}

CARLA_PLUGIN_EXPORT
int jack_port_untie(jack_port_t* port)
{
    carla_debug("%s(%p)", __FUNCTION__, port);
    return ENOSYS;

    // unused
    (void)port;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_port_set_name(jack_port_t *port, const char *port_name)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, port_name);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_rename(jack_client_t* client, jack_port_t *port, const char *port_name)
{
    carla_stderr2("%s(%p, %p, %s) WIP", __FUNCTION__, client, port, port_name);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, EINVAL);

    CARLA_SAFE_ASSERT_RETURN(port_name != nullptr && port_name[0] != '\0', EINVAL);

    JackPortState* const jport = (JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, EINVAL);
    CARLA_SAFE_ASSERT_RETURN(! jport->isSystem, EINVAL);

    // TODO: verify uniqueness

    char* const fullname = (char*)std::malloc(STR_MAX);
    CARLA_SAFE_ASSERT_RETURN(fullname != nullptr, ENOMEM);

    std::snprintf(fullname, STR_MAX, "%s:%s", jclient->name, port_name);
    fullname[STR_MAX-1] = '\0';

    jclient->portNameMapping.erase(jport->fullname);
    jclient->portNameMapping[fullname] = jport;

    std::free(jport->name);
    std::free(jport->fullname);

    jport->name = strdup(port_name);
    jport->fullname = fullname;

    // TODO: port rename callback

    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_port_set_alias(jack_port_t* port, const char* alias)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, alias);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_unset_alias(jack_port_t* port, const char* alias)
{
    carla_stderr2("%s(%p, %s)", __FUNCTION__, port, alias);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_get_aliases(const jack_port_t*, char* const aliases[2])
{
    *aliases[0] = '\0';
    *aliases[1] = '\0';
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_port_request_monitor(jack_port_t* port, int onoff)
{
    carla_stderr2("%s(%p, %i)", __FUNCTION__, port, onoff);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_request_monitor_by_name(jack_client_t* client, const char* port_name, int onoff)
{
    carla_stderr2("%s(%p, %s, %i)", __FUNCTION__, client, port_name, onoff);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_ensure_monitor(jack_port_t* port, int onoff)
{
    carla_stderr2("%s(%p, %i)", __FUNCTION__, port, onoff);
    return ENOSYS;
}

CARLA_PLUGIN_EXPORT
int jack_port_monitoring_input(jack_port_t* port)
{
    carla_stderr2("%s(%p)", __FUNCTION__, port);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_connect(jack_client_t* client, const char* a, const char* b)
{
    carla_stderr2("%s(%p, %s. %s)", __FUNCTION__, client, a, b);
    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_disconnect(jack_client_t* client, const char* a, const char* b)
{
    carla_stderr2("%s(%p, %s. %s)", __FUNCTION__, client, a, b);
    return 0;
}

CARLA_PLUGIN_EXPORT
int jack_port_disconnect(jack_client_t* client, jack_port_t* port)
{
    carla_stderr2("%s(%p, %p)", __FUNCTION__, client, port);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
int jack_port_name_size(void)
{
    return STR_MAX;
}

CARLA_PLUGIN_EXPORT
int jack_port_type_size(void)
{
    return STR_MAX;
}

CARLA_PLUGIN_EXPORT
size_t jack_port_type_get_buffer_size(jack_client_t* client, const char* port_type)
{
    carla_debug("%s(%p, %s)", __FUNCTION__, client, port_type);
    CARLA_SAFE_ASSERT_RETURN(port_type != nullptr && port_type[0] != '\0', 0);

    if (std::strcmp(port_type, JACK_DEFAULT_MIDI_TYPE) == 0)
        return JackMidiPortBufferBase::kMaxEventSize;

    return 0;

    // unused
    (void)client;
}

// --------------------------------------------------------------------------------------------------------------------
