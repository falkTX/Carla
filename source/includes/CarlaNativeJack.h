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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Macros */

#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define JACK_DEFAULT_MIDI_TYPE  "8 bit raw midi"

/* ------------------------------------------------------------------------------------------------------------
 * Basic types */

typedef float    jack_default_audio_sample_t;
typedef uint8_t  jack_midi_data_t;
typedef uint32_t jack_nframes_t;

typedef void (*jack_shutdown_callback)(void* ptr);
typedef int  (*jack_process_callback)(jack_nframes_t nframes, void* ptr);

/*
 * Helper struct for midi events.
 */
typedef struct {
    uint32_t count;
    NativeMidiEvent* events;
} NativeMidiEventsBuffer;

/* ------------------------------------------------------------------------------------------------------------
 * Enums */

enum JackPortFlags {
    JackPortIsInput    = 0x01,
    JackPortIsOutput   = 0x02,
    JackPortIsPhysical = 0x04,
    JackPortCanMonitor = 0x08,
    JackPortIsTerminal = 0x10
};

enum JackOptions {
    JackNullOption    = 0x00,
    JackNoStartServer = 0x01,
    JackUseExactName  = 0x02,
    JackServerName    = 0x04,
    JackLoadName      = 0x08,
    JackLoadInit      = 0x10,
    JackSessionID     = 0x20
};

enum JackStatus {
    JackFailure       = 0x0001,
    JackInvalidOption = 0x0002,
    JackNameNotUnique = 0x0004,
    JackServerStarted = 0x0008,
    JackServerFailed  = 0x0010,
    JackServerError   = 0x0020,
    JackNoSuchClient  = 0x0040,
    JackLoadFailure   = 0x0080,
    JackInitFailure   = 0x0100,
    JackShmFailure    = 0x0200,
    JackVersionError  = 0x0400,
    JackBackendError  = 0x0800,
    JackClientZombie  = 0x1000
};

typedef enum JackOptions jack_options_t;
typedef enum JackStatus  jack_status_t;

/* ------------------------------------------------------------------------------------------------------------
 * Structs */

typedef struct {
    bool  registered;
    bool  isAudio;
    uint  flags;

    union {
        void* audio;
        NativeMidiEventsBuffer midi;
    } buffer;
} jack_port_t;

typedef struct {
    // current state
    bool           active;
    jack_nframes_t bufferSize;
    jack_nframes_t sampleRate;

    // callbacks
    jack_process_callback processCallback;
    void*                 processPtr;

    // ports
    jack_port_t portsAudioIn [16];
    jack_port_t portsAudioOut[16];
    jack_port_t portsMidiIn  [16];
    jack_port_t portsMidiOut [16];
} jack_client_t;

typedef struct {
  uint32_t time;
  uint32_t size;
  uint8_t* buffer;
} jack_midi_event_t;

/* ------------------------------------------------------------------------------------------------------------
 * Client functions */

/*
 * NOTE: This function purposefully returns NULL, as we *do not* want global variables.
 *       The client pointer is passed into the plugin code directly.
 */
static inline
jack_client_t* jack_client_open(const char* clientname, jack_options_t options, jack_status_t* status, ...)
{
    if (status != NULL)
        *status = JackFailure;

    return NULL;

    // unused
    (void)clientname;
    (void)options;
}

static inline
int jack_client_close(jack_client_t* client)
{
    // keep bufsize and srate
    const jack_nframes_t bufferSize = client->bufferSize;
    const jack_nframes_t sampleRate = client->sampleRate;
    memset(client, 0, sizeof(jack_client_t));
    client->bufferSize = bufferSize;
    client->sampleRate = sampleRate;
    return 0;
}

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
jack_port_t* jack_port_register(jack_client_t* client, const char* name, const char* type, ulong flags, ulong buffersize)
{
    const bool isAudio = strcmp(type, JACK_DEFAULT_AUDIO_TYPE) == 0;
    const bool isMIDI  = strcmp(type, JACK_DEFAULT_MIDI_TYPE ) == 0;

    if (! (isAudio || isMIDI))
        return NULL;

    jack_port_t* ports;

    if (isAudio)
        ports = (flags & JackPortIsInput) ? client->portsAudioIn : client->portsAudioOut;
    else
        ports = (flags & JackPortIsInput) ? client->portsMidiIn : client->portsMidiOut;

    for (int i=0; i<16; ++i)
    {
        jack_port_t* const port = &ports[i];

        if (port->registered)
            continue;

        memset(port, 0, sizeof(jack_port_t));

        port->registered = true;
        port->isAudio    = isAudio;
        port->flags      = flags;

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
    jack_port_t* const ports = (port->flags & JackPortIsInput) ? client->portsAudioIn : client->portsAudioOut;

    for (int i=0; i<8; ++i)
    {
        if (&ports[i] == port)
        {
            memset(port, 0, sizeof(jack_port_t));
            return 0;
        }
    }

    return 1;
}

static inline
void* jack_port_get_buffer(jack_port_t* port, jack_nframes_t nframes)
{
    return port->isAudio ? port->buffer.audio : &port->buffer.midi;

    // unused
    (void)nframes;
}

/* ------------------------------------------------------------------------------------------------------------
 * MIDI functions */

static inline
uint32_t jack_midi_get_event_count(void* port_buffer)
{
    NativeMidiEventsBuffer* const midi_buffer = (NativeMidiEventsBuffer*)port_buffer;

    return midi_buffer->count;
}

static inline
int jack_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index)
{
    NativeMidiEventsBuffer* const midi_buffer = (NativeMidiEventsBuffer*)port_buffer;

    if (midi_buffer->count == 0)
        return ENODATA;
    if (event_index >= midi_buffer->count)
        return 1; // FIXME

    NativeMidiEvent* const midiEvent = &midi_buffer->events[event_index];

    event->time   = midiEvent->time;
    event->size   = midiEvent->size;
    event->buffer = midiEvent->data;

    return 0;
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
