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
const char** jack_get_ports(jack_client_t*, const char* a, const char* b, unsigned long flags)
{
    carla_stdout("CarlaJackClient :: %s | %s %s %li", __FUNCTION__, a, b, flags);

    static const char* capture_1  = "system:capture_1";
    static const char* capture_2  = "system:capture_2";
    static const char* playback_1 = "system:playback_1";
    static const char* playback_2 = "system:playback_2";

    if (flags == 0 || (flags & (JackPortIsInput|JackPortIsOutput)) == (JackPortIsInput|JackPortIsOutput))
    {
        if (const char** const ret = (const char**)calloc(5, sizeof(const char*)))
        {
            ret[0] = capture_1;
            ret[1] = capture_2;
            ret[2] = playback_1;
            ret[3] = playback_2;
            ret[4] = nullptr;
            return ret;
        }
    }

    if (flags & JackPortIsInput)
    {
        if (const char** const ret = (const char**)calloc(3, sizeof(const char*)))
        {
            ret[0] = playback_1;
            ret[1] = playback_2;
            ret[2] = nullptr;
            return ret;
        }
    }

    if (flags & JackPortIsOutput)
    {
        if (const char** const ret = (const char**)calloc(3, sizeof(const char*)))
        {
            ret[0] = capture_1;
            ret[1] = capture_2;
            ret[2] = nullptr;
            return ret;
        }
    }

    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_name(jack_client_t* /*client*/, const char* name)
{
    carla_stdout("CarlaJackClient :: %s | %s", __FUNCTION__, name);

//     CarlaJackClient* const jclient = (CarlaJackClient*)client;
//     CARLA_SAFE_ASSERT_RETURN(jclient != nullptr, 0);

//     const JackClientState& jstate(jclient->fState);
    //CARLA_SAFE_ASSERT_RETURN(jstate.activated, 0);

    static const JackPortState capturePorts[] = {
        JackPortState("system", "capture_1", 0, JackPortIsOutput|JackPortIsPhysical|JackPortIsTerminal, true),
        JackPortState("system", "capture_2", 1, JackPortIsOutput|JackPortIsPhysical|JackPortIsTerminal, true),
    };
    static const JackPortState playbackPorts[] = {
        JackPortState("system", "playback_1", 3, JackPortIsInput|JackPortIsPhysical|JackPortIsTerminal, true),
        JackPortState("system", "playback_2", 4, JackPortIsInput|JackPortIsPhysical|JackPortIsTerminal, true),
    };

    if (std::strncmp(name, "system:", 7) == 0)
    {
        name += 7;

        /**/ if (std::strncmp(name, "capture_", 8) == 0)
        {
            name += 8;

            const int index = std::atoi(name);
            CARLA_SAFE_ASSERT_RETURN(index >= 0 && index < 2, nullptr);

            return (jack_port_t*)&capturePorts[index];
        }
        else if (std::strncmp(name, "playback_", 9) == 0)
        {
            name += 9;

            const int index = std::atoi(name);
            CARLA_SAFE_ASSERT_RETURN(index >= 0, nullptr);

            return (jack_port_t*)&playbackPorts[index];
        }
        else
        {
            carla_stderr2("Invalid port short name: '%s'", name);
            return nullptr;
        }
    }

    carla_stderr2("Invalid port name: '%s'", name);
    return nullptr;
}

CARLA_EXPORT
jack_port_t* jack_port_by_id(jack_client_t*, jack_port_id_t)
{
    carla_stdout("CarlaJackClient :: %s", __FUNCTION__);

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
