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

#include "CarlaStringList.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static const char* allocate_port_name(const char* const prefixOrFullName, const uint num = UINT32_MAX)
{
    static CarlaStringList portList;

    char portName[STR_MAX];
    carla_zeroChars(portName, STR_MAX);

    if (num == UINT32_MAX)
        std::strncpy(portName, prefixOrFullName, STR_MAX-1);
    else
        std::snprintf(portName, STR_MAX-1, "%s%u", prefixOrFullName, num+1);

    if (const char* const storedPortName = portList.containsAndReturnString(portName))
        return storedPortName;

    CARLA_SAFE_ASSERT_RETURN(portList.append(portName), nullptr);

    return portList.getLast();
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
const char** jack_get_ports(jack_client_t* const client,
                            const char* const port_name,
                            const char* const port_type,
                            const unsigned long flags)
{
    carla_stdout("%s(%p, %s, %s, 0x%lx)", __FUNCTION__, client, port_name, port_type, flags);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackServerState& jserver(jclient->server);

    const uint numIns  = static_cast<uint>(jclient->audioIns.count() +
                                           jclient->midiIns.count() +
                                           jserver.numAudioIns +
                                           jserver.numMidiIns);
    const uint numOuts = static_cast<uint>(jclient->audioOuts.count() +
                                           jclient->midiOuts.count() +
                                           jserver.numAudioOuts +
                                           jserver.numMidiOuts);

    const bool wantsAudio = port_type == nullptr || port_type[0] == '\0' || std::strstr(port_type, "audio") != nullptr;
    const bool wantsMIDI  = port_type == nullptr || port_type[0] == '\0' || std::strstr(port_type, "midi") != nullptr;

    if (flags == 0x0 || (flags & (JackPortIsInput|JackPortIsOutput)) == (JackPortIsInput|JackPortIsOutput))
    {
        if (const char** const ret = (const char**)calloc(numIns+numOuts+1, sizeof(const char*)))
        {
            uint i=0;

            if (wantsAudio)
            {
                for (uint j=0; j<jserver.numAudioIns; ++i, ++j)
                    ret[i] = allocate_port_name("system:capture_", j);
                for (uint j=0; j<jserver.numAudioOuts; ++i, ++j)
                    ret[i] = allocate_port_name("system:playback_", j);
            }
            if (wantsMIDI)
            {
                for (uint j=0; j<jserver.numMidiIns; ++i, ++j)
                    ret[i] = allocate_port_name("system:midi_capture_", j);
                for (uint j=0; j<jserver.numMidiOuts; ++i, ++j)
                    ret[i] = allocate_port_name("system:midi_playback_", j);
            }

            if ((flags & (JackPortIsPhysical|JackPortIsTerminal)) == 0x0)
            {
                if (wantsAudio)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->audioIns.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }

                    for (LinkedList<JackPortState*>::Itenerator it = jclient->audioOuts.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }

                if (wantsMIDI)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->midiIns.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }

                    for (LinkedList<JackPortState*>::Itenerator it = jclient->midiOuts.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }
            }

            ret[i] = nullptr;

            return ret;
        }

        return nullptr;
    }

    if (flags & JackPortIsInput)
    {
        if (const char** const ret = (const char**)calloc(numIns+1, sizeof(const char*)))
        {
            uint i=0;
            if (wantsAudio)
            {
                for (uint j=0; j<jserver.numAudioOuts; ++i, ++j)
                    ret[i] = allocate_port_name("system:playback_", j);
            }
            if (wantsMIDI)
            {
                for (uint j=0; j<jserver.numMidiOuts; ++i, ++j)
                    ret[i] = allocate_port_name("system:midi_playback_", j);
            }

            if ((flags & (JackPortIsPhysical|JackPortIsTerminal)) == 0x0)
            {
                if (wantsAudio)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->audioIns.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }
                if (wantsMIDI)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->midiIns.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }
            }

            ret[i] = nullptr;

            return ret;
        }

        return nullptr;
    }

    if (flags & JackPortIsOutput)
    {
        if (const char** const ret = (const char**)calloc(numOuts+1, sizeof(const char*)))
        {
            uint i=0;

            if (wantsAudio)
            {
                for (uint j=0; j<jserver.numAudioIns; ++i, ++j)
                    ret[i] = allocate_port_name("system:capture_", j);
            }
            if (wantsMIDI)
            {
                for (uint j=0; j<jserver.numMidiIns; ++i, ++j)
                    ret[i] = allocate_port_name("system:midi_capture_", j);
            }

            if ((flags & (JackPortIsPhysical|JackPortIsTerminal)) == 0x0)
            {
                if (wantsAudio)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->audioOuts.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }
                if (wantsMIDI)
                {
                    for (LinkedList<JackPortState*>::Itenerator it = jclient->midiOuts.begin2(); it.valid(); it.next())
                    {
                        JackPortState* const jport = it.getValue(nullptr);
                        CARLA_SAFE_ASSERT_CONTINUE(jport != nullptr);
                        ret[i++] = allocate_port_name(jport->fullname);
                    }
                }
            }

            ret[i] = nullptr;

            return ret;
        }

        return nullptr;
    }

    return nullptr;
}

CARLA_PLUGIN_EXPORT
jack_port_t* jack_port_by_name(jack_client_t* client, const char* name)
{
    carla_debug("%s(%p, %s)", __FUNCTION__, client, name);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    if (std::strncmp(name, "system:", 7) == 0)
    {
        static std::map<uint, JackPortState*> systemPortIdMapping;

        const JackServerState& jserver(jclient->server);
        const int commonFlags = JackPortIsPhysical|JackPortIsTerminal;

        uint rindex, gid;
        int flags;
        bool isMidi, isConnected;

        const char* const fullname = name;
        const char* const portname = name + 7;
        name += 7;

        /**/ if (std::strncmp(name, "capture_", 8) == 0)
        {
            name += 8;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioIns, nullptr);

            rindex = static_cast<uint>(index);
            flags  = commonFlags|JackPortIsOutput;
            gid    = JackPortState::kPortIdOffsetAudioIn + rindex;
            isMidi = false;
            isConnected = jserver.numAudioIns > rindex;
        }
        else if (std::strncmp(name, "playback_", 9) == 0)
        {
            name += 9;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioOuts, nullptr);

            rindex = static_cast<uint>(jserver.numAudioIns + index);
            flags  = commonFlags|JackPortIsInput;
            gid    = JackPortState::kPortIdOffsetAudioOut + rindex;
            isMidi = false;
            isConnected = jserver.numAudioOuts > rindex;
        }
        else if (std::strncmp(name, "midi_capture_", 13) == 0)
        {
            name += 13;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numMidiIns, nullptr);

            rindex = static_cast<uint>(index);
            flags  = commonFlags|JackPortIsOutput;
            gid    = JackPortState::kPortIdOffsetMidiIn + rindex;
            isMidi = true;
            isConnected = jserver.numMidiIns > rindex;
        }
        else if (std::strncmp(name, "midi_playback_", 14) == 0)
        {
            name += 14;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numMidiOuts, nullptr);

            rindex = static_cast<uint>(jserver.numMidiIns + index);
            flags  = commonFlags|JackPortIsInput;
            gid    = JackPortState::kPortIdOffsetMidiOut + rindex;
            isMidi = true;
            isConnected = jserver.numMidiOuts > rindex;
        }
        else
        {
            carla_stderr2("jack_port_by_name: invalid port short name '%s'", name);
            return nullptr;
        }

        if (JackPortState* const port = systemPortIdMapping[gid])
            return (jack_port_t*)port;

        JackPortState* const port = new JackPortState(fullname,
                                                      portname,
                                                      rindex, flags, gid,
                                                      isMidi, isConnected);
        systemPortIdMapping[gid] = port;

        return (jack_port_t*)port;
    }
    else
    {
        if (JackPortState* const port = jclient->portNameMapping[name])
            return (jack_port_t*)port;
    }

    carla_stderr2("jack_port_by_name: invalid port name '%s'", name);
    return nullptr;
}

CARLA_PLUGIN_EXPORT
jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    carla_debug("%s(%p, %u)", __FUNCTION__, client, port_id);

    CARLA_SAFE_ASSERT_UINT_RETURN(port_id >= JackPortState::kPortIdOffsetUser, port_id, nullptr);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    if (JackPortState* const port = jclient->portIdMapping[port_id])
        return (jack_port_t*)port;

    carla_stderr2("jack_port_by_id: invalid port id %u", port_id);
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT
const char** jack_port_get_connections(const jack_port_t* port)
{
    carla_stderr2("%s(%p)", __FUNCTION__, port);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(! jport->isSystem, nullptr);

    if (! jport->isConnected)
        return nullptr;

    return nullptr;
}

CARLA_PLUGIN_EXPORT
const char** jack_port_get_all_connections(const jack_client_t* client, const jack_port_t* port)
{
    carla_stdout("%s(%p, %p) WIP", __FUNCTION__, client, port);

    const JackClientState* const jclient = (const JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackPortState* const jport = (const JackPortState*)port;
    CARLA_SAFE_ASSERT_RETURN(jport != nullptr, nullptr);
    CARLA_SAFE_ASSERT_UINT_RETURN(jport->gid >= JackPortState::kPortIdOffsetAudioIn, jport->gid, nullptr);

    if (! jport->isConnected)
        return nullptr;

    if (jport->isSystem)
    {
        const JackPortState* connectedPort;

        /**/ if (jport->gid >= JackPortState::kPortIdOffsetMidiOut)
            connectedPort = jclient->midiOuts.getAt(jport->gid - JackPortState::kPortIdOffsetMidiOut, nullptr);
        else if (jport->gid >= JackPortState::kPortIdOffsetAudioOut)
            connectedPort = jclient->audioOuts.getAt(jport->gid - JackPortState::kPortIdOffsetAudioOut, nullptr);
        else if (jport->gid >= JackPortState::kPortIdOffsetMidiIn)
            connectedPort = jclient->midiIns.getAt(jport->gid - JackPortState::kPortIdOffsetMidiIn, nullptr);
        else
            connectedPort = jclient->audioIns.getAt(jport->gid - JackPortState::kPortIdOffsetAudioIn, nullptr);

        if (connectedPort == nullptr)
        {
            carla_debug("port %s has no connections?", jport->fullname);
            return nullptr;
        }

        if (const char** const ret = static_cast<const char**>(malloc(sizeof(const char*)*2)))
        {
            carla_debug("port %s is connected to %s", jport->fullname, connectedPort->fullname);
            ret[0] = connectedPort->fullname;
            ret[1] = nullptr;
            return ret;
        }
    }
    else
    {
        const JackServerState& jserver(jclient->server);
        const char* connectedPortName = nullptr;

        if (jport->isMidi)
        {
            if (jport->flags & JackPortIsOutput)
            {
                if (jport->index < jserver.numMidiOuts)
                     connectedPortName = allocate_port_name("system:midi_playback_", jport->index);
            }
            else
            {
                if (jport->index < jserver.numMidiIns)
                     connectedPortName = allocate_port_name("system:midi_capture_", jport->index);
            }
        }
        else
        {
            if (jport->flags & JackPortIsOutput)
            {
                if (jport->index < jserver.numAudioOuts)
                    connectedPortName = allocate_port_name("system:playback_", jport->index);
            }
            else
            {
                if (jport->index < jserver.numAudioIns)
                    connectedPortName = allocate_port_name("system:capture_", jport->index);
            }
        }

        if (connectedPortName != nullptr)
        {
            if (const char** const ret = static_cast<const char**>(malloc(sizeof(const char*)*2)))
            {
                carla_debug("port %s is connected to %s", jport->fullname, connectedPortName);
                ret[0] = connectedPortName;
                ret[1] = nullptr;
                return ret;
            }
        }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
