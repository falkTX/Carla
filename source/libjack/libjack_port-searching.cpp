/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2020 Filipe Coelho <falktx@falktx.com>
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

static const char* allocate_port_name(const char* const prefix, const uint num)
{
    static CarlaStringList portList;

    char portName[STR_MAX];
    carla_zeroChars(portName, STR_MAX);
    std::snprintf(portName, STR_MAX-1, "%s%u", prefix, num+1);

    if (const char* const storedPortName = portList.containsAndReturnString(portName))
        return storedPortName;

    CARLA_SAFE_ASSERT_RETURN(portList.append(portName), nullptr);

    return portList.getLast();
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
const char** jack_get_ports(jack_client_t* client, const char* a, const char* b, unsigned long flags)
{
    carla_stdout("%s(%p, %s, %s, 0x%lx) WIP", __FUNCTION__, client, a, b, flags);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackServerState& jserver(jclient->server);

    const uint numIns  = static_cast<uint>(jserver.numAudioIns  + jserver.numMidiIns);
    const uint numOuts = static_cast<uint>(jserver.numAudioOuts + jserver.numMidiOuts);

    if (flags == 0 || (flags & (JackPortIsInput|JackPortIsOutput)) == (JackPortIsInput|JackPortIsOutput))
    {
        if (const char** const ret = (const char**)calloc(numIns+numOuts+1, sizeof(const char*)))
        {
            uint i=0;
            for (uint j=0; j<jserver.numAudioIns; ++i, ++j)
                ret[i] = allocate_port_name("system:capture_", j);
            for (uint j=0; j<jserver.numAudioOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:playback_", j);
            for (uint j=0; j<jserver.numMidiIns; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_capture_", j);
            for (uint j=0; j<jserver.numMidiOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_playback_", j);

            ret[i] = nullptr;

            return ret;
        }
    }

    if (flags & JackPortIsInput)
    {
        if (const char** const ret = (const char**)calloc(numIns+1, sizeof(const char*)))
        {
            uint i=0;
            for (uint j=0; j<jserver.numAudioOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:playback_", j);
            for (uint j=0; j<jserver.numMidiOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_playback_", j);

            ret[i] = nullptr;

            return ret;
        }
    }

    if (flags & JackPortIsOutput)
    {
        if (const char** const ret = (const char**)calloc(numOuts+1, sizeof(const char*)))
        {
            uint i=0;
            for (uint j=0; j<jserver.numAudioIns; ++i, ++j)
                ret[i] = allocate_port_name("system:capture_", j);
            for (uint j=0; j<jserver.numMidiIns; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_capture_", j);

            ret[i] = nullptr;

            return ret;
        }
    }

    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_name(jack_client_t* client, const char* name)
{
    carla_debug("%s(%p, %s)", __FUNCTION__, client, name);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    if (std::strncmp(name, "system:", 7) == 0)
    {
        static JackPortState retPort(
            /* name        */ nullptr,
            /* fullname    */ nullptr,
            /* index       */ 0,
            /* flags       */ 0x0,
            /* isMidi      */ false,
            /* isSystem    */ true,
            /* isConnected */ false
        );
        static CarlaString rname, rfullname;

        const JackServerState& jserver(jclient->server);
        const int commonFlags = JackPortIsPhysical|JackPortIsTerminal;

        rfullname = name;

        name += 7;

        rname = name;

        retPort.name = rname.buffer();
        retPort.fullname = rfullname.buffer();

        /**/ if (std::strncmp(name, "capture_", 8) == 0)
        {
            name += 8;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioIns, nullptr);

            retPort.index  = static_cast<uint>(index);
            retPort.flags  = commonFlags|JackPortIsOutput;
            retPort.isMidi = false;
            retPort.isConnected = jserver.numAudioIns > index;
        }
        else if (std::strncmp(name, "playback_", 9) == 0)
        {
            name += 9;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioOuts, nullptr);

            retPort.index  = static_cast<uint>(jserver.numAudioIns + index);
            retPort.flags  = commonFlags|JackPortIsInput;
            retPort.isMidi = false;
            retPort.isConnected = jserver.numAudioOuts > index;
        }
        else if (std::strncmp(name, "midi_capture_", 13) == 0)
        {
            name += 13;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numMidiIns, nullptr);

            retPort.index  = static_cast<uint>(index);
            retPort.flags  = commonFlags|JackPortIsOutput;
            retPort.isMidi = true;
            retPort.isConnected = jserver.numMidiIns > index;
        }
        else if (std::strncmp(name, "midi_playback_", 14) == 0)
        {
            name += 14;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numMidiOuts, nullptr);

            retPort.index  = static_cast<uint>(jserver.numMidiIns + index);
            retPort.flags  = commonFlags|JackPortIsInput;
            retPort.isMidi = true;
            retPort.isConnected = jserver.numMidiOuts > index;
        }
        else
        {
            carla_stderr2("jack_port_by_name: invalid port short name '%s'", name);
            return nullptr;
        }

        return (jack_port_t*)&retPort;
    }
    else
    {
        if (JackPortState* const port = jclient->portNameMapping[name])
            return (jack_port_t*)port;
    }

    carla_stderr2("jack_port_by_name: invalid port name '%s'", name);
    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    carla_stderr2("%s(%p, %u)", __FUNCTION__, client, port_id);
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
