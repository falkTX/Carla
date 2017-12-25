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

#include "CarlaStringList.hpp"

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

const char* allocate_port_name(const char* const prefix, const uint32_t num)
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

CARLA_EXPORT
const char** jack_get_ports(jack_client_t* client, const char* a, const char* b, unsigned long flags)
{
    carla_stdout("%s(%p, %s, %s, 0x%lx) WIP", __FUNCTION__, client, a, b, flags);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, nullptr);

    const JackServerState& jserver(jclient->server);

    const uint32_t numIns  = jserver.numAudioIns  + jserver.numMidiIns;
    const uint32_t numOuts = jserver.numAudioOuts + jserver.numMidiOuts;

    if (flags == 0 || (flags & (JackPortIsInput|JackPortIsOutput)) == (JackPortIsInput|JackPortIsOutput))
    {
        if (const char** const ret = (const char**)calloc(numIns+numOuts, sizeof(const char*)))
        {
            uint32_t i=0;
            for (uint32_t j=0; j<jserver.numAudioIns; ++i, ++j)
                ret[i] = allocate_port_name("system:capture_", j);
            for (uint32_t j=0; j<jserver.numAudioOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:playback_", j);
            for (uint32_t j=0; j<jserver.numMidiIns; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_capture_", j);
            for (uint32_t j=0; j<jserver.numMidiOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_playback_", j);

            return ret;
        }
    }

    if (flags & JackPortIsInput)
    {
        if (const char** const ret = (const char**)calloc(numIns, sizeof(const char*)))
        {
            uint32_t i=0;
            for (uint32_t j=0; j<jserver.numAudioOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:playback_", j);
            for (uint32_t j=0; j<jserver.numMidiOuts; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_playback_", j);

            return ret;
        }
    }

    if (flags & JackPortIsOutput)
    {
        if (const char** const ret = (const char**)calloc(numOuts, sizeof(const char*)))
        {
            uint32_t i=0;
            for (uint32_t j=0; j<jserver.numAudioIns; ++i, ++j)
                ret[i] = allocate_port_name("system:capture_", j);
            for (uint32_t j=0; j<jserver.numMidiIns; ++i, ++j)
                ret[i] = allocate_port_name("system:midi_capture_", j);

            return ret;
        }
    }

    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_name(jack_client_t* client, const char* name)
{
    carla_stdout("%s(%p, %s) WIP", __FUNCTION__, client, name);

    JackClientState* const jclient = (JackClientState*)client;
    CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

    const JackServerState& jserver(jclient->server);
    const int commonFlags = JackPortIsPhysical|JackPortIsTerminal;

    static JackPortState retPort(
        /* name        */ nullptr,
        /* fullname    */ nullptr,
        /* index       */ 0,
        /* flags       */ 0x0,
        /* isMidi      */ false,
        /* isSystem    */ true,
        /* isConnected */ false
    );

    if (std::strncmp(name, "system:", 7) == 0)
    {
        std::free(retPort.fullname);
        retPort.fullname = strdup(name);

        name += 7;

        std::free(retPort.name);
        retPort.name = strdup(name);

        /**/ if (std::strncmp(name, "capture_", 8) == 0)
        {
            name += 8;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioIns, nullptr);

            retPort.index  = index;
            retPort.flags  = commonFlags|JackPortIsOutput;
            retPort.isMidi = false;
            retPort.isConnected = jserver.numAudioIns > index;
        }
        else if (std::strncmp(name, "playback_", 9) == 0)
        {
            name += 9;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioOuts, nullptr);

            retPort.index  = (jserver.numAudioIns) + index;
            retPort.flags  = commonFlags|JackPortIsInput;
            retPort.isMidi = false;
            retPort.isConnected = jserver.numAudioOuts > index;
        }
        else if (std::strncmp(name, "midi_capture_", 13) == 0)
        {
            name += 13;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioIns, nullptr);

            retPort.index  = index;
            retPort.flags  = commonFlags|JackPortIsOutput;
            retPort.isMidi = true;
            retPort.isConnected = jserver.numMidiIns > index;
        }
        else if (std::strncmp(name, "midi_playback_", 14) == 0)
        {
            name += 14;

            const int index = std::atoi(name)-1;
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < jserver.numAudioOuts, nullptr);

            retPort.index  = (jserver.numAudioIns) + index;
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
