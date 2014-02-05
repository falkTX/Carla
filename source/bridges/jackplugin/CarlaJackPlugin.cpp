/*
 * Carla Jack Plugin
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngine.hpp"
#include "CarlaHost.h"
#include "CarlaUtils.hpp"

#include "jackbridge/JackBridge.hpp"

//#include "CarlaEngine.hpp"
//#include "CarlaPlugin.hpp"
//#include "CarlaBackendUtils.hpp"
//#include "CarlaBridgeUtils.hpp"
//#include "CarlaMIDI.h"

// -------------------------------------------------------------------------------------------------------------------

struct _jack_client {
    JackShutdownCallback shutdown_cb;
    void*                shutdown_ptr;

    JackProcessCallback process_cb;
    void*               process_ptr;

    _jack_client()
    {
        clear();
    }

    void clear()
    {
        shutdown_cb  = nullptr;
        shutdown_ptr = nullptr;
        process_cb   = nullptr;
        process_ptr  = nullptr;
    }
};

static jack_client_t gJackClient;

// -------------------------------------------------------------------------------------------------------------------

struct _jack_port {
    bool used;
    char name[128+1];

    _jack_port()
        : used(false) {}
};

// system ports
static jack_port_t gPortSystemIn1;  // 0
static jack_port_t gPortSystemIn2;  // 1
static jack_port_t gPortSystemOut1; // 2
static jack_port_t gPortSystemOut2; // 3

// client ports
static jack_port_t gPortAudioIn1;  // 4
static jack_port_t gPortAudioIn2;  // 5
static jack_port_t gPortAudioOut1; // 6
static jack_port_t gPortAudioOut2; // 7
static jack_port_t gPortMidiIn;    // 8
static jack_port_t gPortMidiOut;   // 9

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT jack_client_t* jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...);
CARLA_EXPORT int jack_client_close(jack_client_t* client);

CARLA_EXPORT int   jack_client_name_size();
CARLA_EXPORT char* jack_get_client_name(jack_client_t* client);

CARLA_EXPORT int jack_activate(jack_client_t* client);
CARLA_EXPORT int jack_deactivate(jack_client_t* client);
CARLA_EXPORT int jack_is_realtime(jack_client_t* client);

CARLA_EXPORT jack_nframes_t jack_get_sample_rate(jack_client_t* client);
CARLA_EXPORT jack_nframes_t jack_get_buffer_size(jack_client_t* client);

CARLA_EXPORT jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);

CARLA_EXPORT const char* jack_port_name(const jack_port_t* port);

CARLA_EXPORT const char** jack_get_ports(jack_client_t*, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags);
CARLA_EXPORT jack_port_t* jack_port_by_name(jack_client_t* client, const char* port_name);
CARLA_EXPORT jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id);

CARLA_EXPORT int jack_connect(jack_client_t* client, const char* source_port, const char* destination_port);
CARLA_EXPORT int jack_disconnect(jack_client_t* client, const char* source_port, const char* destination_port);

CARLA_EXPORT void jack_on_shutdown(jack_client_t* client, JackShutdownCallback function, void* arg);
CARLA_EXPORT int  jack_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg);

CARLA_EXPORT void jack_set_info_function(void (*func)(const char*));
CARLA_EXPORT void jack_set_error_function(void (*func)(const char*));
CARLA_EXPORT void jack_free(void* ptr);

// -------------------------------------------------------------------------------------------------------------------

typedef void (*jack_error_callback)(const char* msg);
typedef void (*jack_info_callback)(const char* msg);

jack_error_callback sErrorCallback = nullptr;
jack_info_callback  sInfoCallback  = nullptr;

// -------------------------------------------------------------------------------------------------------------------

jack_client_t* jack_client_open(const char* client_name, jack_options_t /*options*/, jack_status_t* status, ...)
{
    if (carla_is_engine_running())
    {
        if (status != nullptr)
            *status = JackServerStarted;
        return nullptr;
    }

    if (! carla_engine_init("JACK", client_name))
    {
        if (status != nullptr)
            *status = JackServerFailed;
        return nullptr;
    }

    if (! gPortSystemIn1.used)
    {
        gPortSystemIn1.used = true;
        gPortSystemIn2.used = true;
        gPortSystemOut1.used = true;
        gPortSystemOut2.used = true;
        std::strcpy(gPortSystemIn1.name, "system:capture_1");
        std::strcpy(gPortSystemIn2.name, "system:capture_2");
        std::strcpy(gPortSystemOut1.name, "system:playback_1");
        std::strcpy(gPortSystemOut2.name, "system:playback_2");
    }

    return &gJackClient;
}

int jack_client_close(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    if (! carla_is_engine_running())
        return -1;

    carla_engine_close();
    gJackClient.clear();
    gPortAudioIn1.used = 0;
    gPortAudioIn2.used = 0;
    gPortAudioOut1.used = 0;
    gPortAudioOut2.used = 0;
    gPortMidiIn.used = 0;
    gPortMidiOut.used = 0;
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_client_name_size()
{
    return 32+1; // same as JACK1
}

char* jack_get_client_name(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (const CarlaEngine* const engine = carla_get_engine())
        return const_cast<char*>(engine->getName());

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_activate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

int jack_deactivate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

int jack_is_realtime(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);

    return 1;
}

// -------------------------------------------------------------------------------------------------------------------

jack_nframes_t jack_get_sample_rate(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);

    return static_cast<uint32_t>(carla_get_sample_rate());
}

jack_nframes_t jack_get_buffer_size(jack_client_t* client)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, 0);

    return carla_get_buffer_size();
}

// -------------------------------------------------------------------------------------------------------------------

jack_port_t* jack_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (std::strcmp(port_type, JACK_DEFAULT_AUDIO_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            if (gPortAudioIn1.used && gPortAudioIn2.used)
                return nullptr;

            if (! gPortAudioIn1.used)
            {
                std::strncpy(gPortAudioIn1.name, port_name, 128);
                return &gPortAudioIn1;
            }
            else
            {
                std::strncpy(gPortAudioIn2.name, port_name, 128);
                return &gPortAudioIn2;
            }
        }
        else
        {
            if (gPortAudioOut1.used && gPortAudioOut2.used)
                return nullptr;

            if (! gPortAudioOut1.used)
            {
                std::strncpy(gPortAudioOut1.name, port_name, 128);
                return &gPortAudioOut1;
            }
            else
            {
                std::strncpy(gPortAudioOut2.name, port_name, 128);
                return &gPortAudioOut2;
            }
        }
    }

    if (std::strcmp(port_type, JACK_DEFAULT_MIDI_TYPE) == 0)
    {
        if (flags & JackPortIsInput)
        {
            if (gPortMidiIn.used)
                return nullptr;
            std::strncpy(gPortMidiIn.name, port_name, 128);
            return &gPortMidiIn;
        }
        else
        {
            if (gPortMidiOut.used)
                return nullptr;
            std::strncpy(gPortMidiOut.name, port_name, 128);
            return &gPortMidiOut;
        }
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

const char* jack_port_name(const jack_port_t* port)
{
    return port->name;
}

// -------------------------------------------------------------------------------------------------------------------

const char** jack_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);

    if (port_name_pattern != nullptr)
    {
        if (std::strstr("system:playback_", port_name_pattern) == nullptr)
            return nullptr;
        if (std::strstr("system:capture_", port_name_pattern) == nullptr)
            return nullptr;
    }
    if (type_name_pattern != nullptr)
    {
        if (std::strstr(JACK_DEFAULT_AUDIO_TYPE, type_name_pattern) == nullptr)
            return nullptr;
    }

    uint numPorts = 0;

    if (flags == 0)
    {
        numPorts = 4;
    }
    else
    {
        if (flags & JackPortIsInput)
            numPorts += 2;
        if (flags & JackPortIsOutput)
            numPorts += 2;
    }

    if (numPorts == 0)
        return nullptr;

    const char** const ports = new const char*[numPorts+1];
    uint i = 0;

    if (flags == 0 || (flags & JackPortIsInput) != 0)
    {
        ports[i++] = gPortSystemIn1.name;
        ports[i++] = gPortSystemIn1.name;
    }
    if (flags == 0 || (flags & JackPortIsOutput) != 0)
    {
        ports[i++] = gPortSystemOut1.name;
        ports[i++] = gPortSystemOut1.name;
    }
    ports[i++] = nullptr;

    return ports;
}

jack_port_t* jack_port_by_name(jack_client_t* client, const char* port_name)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);
    CARLA_SAFE_ASSERT_RETURN(gPortSystemIn1.used, nullptr);

    if (std::strcmp(port_name, gPortSystemIn1.name) == 0)
        return &gPortSystemIn1;
    if (std::strcmp(port_name, gPortSystemIn2.name) == 0)
        return &gPortSystemIn2;
    if (std::strcmp(port_name, gPortSystemOut1.name) == 0)
        return &gPortSystemOut1;
    if (std::strcmp(port_name, gPortSystemOut2.name) == 0)
        return &gPortSystemOut2;

    if (gPortAudioIn1.used && std::strcmp(port_name, gPortAudioIn1.name) == 0)
        return &gPortAudioIn1;
    if (gPortAudioIn2.used && std::strcmp(port_name, gPortAudioIn2.name) == 0)
        return &gPortAudioIn2;
    if (gPortAudioOut1.used && std::strcmp(port_name, gPortAudioOut1.name) == 0)
        return &gPortAudioOut1;
    if (gPortAudioOut2.used && std::strcmp(port_name, gPortAudioOut2.name) == 0)
        return &gPortAudioOut2;
    if (gPortMidiIn.used && std::strcmp(port_name, gPortMidiIn.name) == 0)
        return &gPortMidiIn;
    if (gPortMidiOut.used && std::strcmp(port_name, gPortMidiOut.name) == 0)
        return &gPortMidiOut;

    return nullptr;
}

jack_port_t* jack_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, nullptr);
    CARLA_SAFE_ASSERT_RETURN(gPortSystemIn1.used, nullptr);

    switch (port_id)
    {
    case 0:
        return &gPortSystemIn1;
    case 1:
        return &gPortSystemIn2;
    case 2:
        return &gPortSystemOut1;
    case 3:
        return &gPortSystemOut2;
    case 4:
        if (gPortAudioIn1.used)
            return &gPortAudioIn1;
        break;
    case 5:
        if (gPortAudioIn2.used)
            return &gPortAudioIn2;
        break;
    case 6:
        if (gPortAudioOut1.used)
            return &gPortAudioOut1;
        break;
    case 7:
        if (gPortAudioOut2.used)
            return &gPortAudioOut2;
        break;
    case 8:
        if (gPortMidiIn.used)
            return &gPortMidiIn;
        break;
    case 9:
        if (gPortMidiOut.used)
            return &gPortMidiOut;
        break;
    }

    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

int jack_connect(jack_client_t* client, const char*, const char*)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

int jack_disconnect(jack_client_t* client, const char*, const char*)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

void jack_on_shutdown(jack_client_t* client, JackShutdownCallback function, void* arg)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient,);

    gJackClient.shutdown_cb = function;
    gJackClient.shutdown_ptr = arg;
}

int jack_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
    CARLA_SAFE_ASSERT_RETURN(client == &gJackClient, -1);

    gJackClient.process_cb = process_callback;
    gJackClient.process_ptr = arg;
    return 0;
}

// -------------------------------------------------------------------------------------------------------------------

void jack_set_error_function(void (*func)(const char*))
{
    sErrorCallback = func;
}

void jack_set_info_function(void (*func)(const char*))
{
    sInfoCallback = func;
}

void jack_free(void* ptr)
{
    delete[] (char**)ptr;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_register_all_plugins();
void carla_register_all_plugins() {}

// -------------------------------------------------------------------------------------------------------------------
