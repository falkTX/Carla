/*
 * Carla Native Plugin API (JACK Compatibility header)
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_NATIVE_JACK_H_INCLUDED
#define CARLA_NATIVE_JACK_H_INCLUDED

#include "CarlaNative.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Macros */

#define JACK_DEFAULT_AUDIO_TYPE "audio"
#define JACK_DEFAULT_MIDI_TYPE  "midi"

/* ------------------------------------------------------------------------------------------------------------
 * Basic types */

typedef float    jack_default_audio_sample_t;
typedef uint32_t jack_nframes_t;

typedef void (*jack_shutdown_callback)(void* ptr);
typedef int  (*jack_process_callback)(jack_nframes_t nframes, void* ptr);

/* ------------------------------------------------------------------------------------------------------------
 * Enums */

typedef enum {
    JackPortIsInput  = 1 << 0,
    JackPortIsOutput = 1 << 1
} jack___t;

typedef enum {
    JackNoStartServer = 0,
    JackServerName
} jack_options_t;

typedef enum {
    JackNoError = 0x0
} jack_status_t;

/* ------------------------------------------------------------------------------------------------------------
 * Structs */

typedef struct {
    bool  isInput;
    void* buffer;
} jack_port_t;

typedef struct {
    // current state
    bool           active;
    const char*    clientName;
    jack_nframes_t bufferSize;
    jack_nframes_t sampleRate;

    // callbacks
    jack_process_callback processCallback;
    void*                 processPtr;

    // ports
    jack_port_t* portsAudioIn[8];
    jack_port_t* portsAudioOut[8];
} jack_client_t;

/* ------------------------------------------------------------------------------------------------------------
 * Client functions, defined in the plugin code */

jack_client_t* jack_client_open(const char* clientname, jack_options_t options, jack_status_t* status, ...);

int jack_client_close(jack_client_t* client);

/* ------------------------------------------------------------------------------------------------------------
 * Callback functions */

static inline
int jack_on_shutdown(jack_client_t* client, jack_shutdown_callback callback, void* ptr)
{
    return 0;

    // unused
    (void)client;
    (void)callback;
    (void)ptr;
}

static inline
int jack_set_process_callback(jack_client_t* client, jack_process_callback callback, void* ptr)
{
    client->processCallback = callback;
    client->processPtr      = ptr;
    return 0;
}

/* ------------------------------------------------------------------------------------------------------------
 * Port functions */

static inline
jack_port_t* jack_port_register(jack_client_t* client, const char* name, const char* type, jack___t hints, jack_nframes_t buffersize)
{
    jack_port_t** const ports = (hints & JackPortIsInput) ? client->portsAudioIn : client->portsAudioOut;

    for (int i=0; i<8; ++i)
    {
        if (ports[i] != NULL)
            continue;

        jack_port_t* const port = (jack_port_t*)calloc(1, sizeof(jack_port_t));

        if (port == NULL)
            return NULL;

        port->isInput = (hints & JackPortIsInput);

        ports[i] = port;

        return port;
    }

    return NULL;

    // unused
    (void)name;
    (void)type;
    (void)buffersize;
}

static inline
int jack_port_unregister(jack_client_t* client, jack_port_t* port)
{
    jack_port_t** const ports = port->isInput ? client->portsAudioIn : client->portsAudioOut;

    for (int i=0; i<8; ++i)
    {
        if (ports[i] == port)
        {
            ports[i] = nullptr;
            return 0;
        }
    }

    return 1;
}

static inline
void* jack_port_get_buffer(const jack_port_t* port, jack_nframes_t nframes)
{
    return port->buffer;

    // unused
    (void)nframes;
}

/* ------------------------------------------------------------------------------------------------------------
 * [De/]Activate */

static inline
int jack_activate(jack_client_t* client)
{
    client->active = true;
    return 0;
}

static inline
int jack_deactivate(jack_client_t* client)
{
    client->active = false;
    return 0;
}

/* ------------------------------------------------------------------------------------------------------------
 * Get data functions */

static inline
const char* jack_get_client_name(const jack_client_t* client)
{
    return client->clientName;
}

static inline
jack_nframes_t jack_get_buffer_size(const jack_client_t* client)
{
    return client->bufferSize;
}

static inline
jack_nframes_t jack_get_sample_rate(const jack_client_t* client)
{
    return client->sampleRate;
}

/* ------------------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CARLA_NATIVE_JACK_H_INCLUDED */
