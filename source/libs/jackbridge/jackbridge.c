/*
 * JackBridge
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "jackbridge.h"

// -----------------------------------------------------------------------------

jack_client_t* jackbridge_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_client_open(client_name, options, status);
#else
    return NULL;
#endif
}

int jackbridge_client_close(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_client_close(client);
#else
    return 0;
#endif
}

int jackbridge_client_name_size()
{
#ifndef JACKBRIDGE_DUMMY
    return jack_client_name_size();
#else
    return 0;
#endif
}

char* jackbridge_get_client_name(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_get_client_name(client);
#else
    return NULL;
#endif
}

int jackbridge_port_name_size()
{
#ifndef JACKBRIDGE_DUMMY
    return jack_port_name_size();
#else
    return 0;
#endif
}

int jackbridge_recompute_total_latencies(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_recompute_total_latencies(client);
#else
    return 0;
#endif
}

void jackbridge_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
#ifndef JACKBRIDGE_DUMMY
    jack_port_get_latency_range(port, mode, range);
#endif
}

void jackbridge_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
#ifndef JACKBRIDGE_DUMMY
    jack_port_set_latency_range(port, mode, range);
#endif
}

int jackbridge_activate(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_activate(client);
#else
    return 0;
#endif
}

int jackbridge_deactivate(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_deactivate(client);
#else
    return 0;
#endif
}

void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    jack_on_shutdown(client, shutdown_callback, arg);
#endif
}

int jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_set_latency_callback(client, latency_callback, arg);
#else
    return 0;
#endif
}

int jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_set_process_callback(client, process_callback, arg);
#else
    return 0;
#endif
}

int jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_set_freewheel_callback(client, freewheel_callback, arg);
#else
    return 0;
#endif
}

int jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_set_buffer_size_callback(client, bufsize_callback, arg);
#else
    return 0;
#endif
}

int jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_set_sample_rate_callback(client, srate_callback, arg);
#else
    return 0;
#endif
}

jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_get_sample_rate(client);
#else
    return 0;
#endif
}

jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_get_buffer_size(client);
#else
    return 0;
#endif
}

jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_port_register(client, port_name, port_type, flags, buffer_size);
#else
    return NULL;
#endif
}

int jackbridge_port_unregister(jack_client_t* client, jack_port_t* port)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_port_unregister(client, port);
#else
    return 0;
#endif
}

void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_port_get_buffer(port, nframes);
#else
    return NULL;
#endif
}

// -----------------------------------------------------------------------------

uint32_t jackbridge_midi_get_event_count(void* port_buffer)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_midi_get_event_count(port_buffer);
#else
    return 0;
#endif
}

int jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_midi_event_get(event, port_buffer, event_index);
#else
    return 0;
#endif
}

void jackbridge_midi_clear_buffer(void* port_buffer)
{
#ifndef JACKBRIDGE_DUMMY
    jack_midi_clear_buffer(port_buffer);
#endif
}

jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_midi_event_reserve(port_buffer, time, data_size);
#else
    return NULL;
#endif
}

int jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, size_t data_size)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_midi_event_write(port_buffer, time, data, data_size);
#else
    return 0;
#endif
}

// -----------------------------------------------------------------------------

jack_transport_state_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos)
{
#ifndef JACKBRIDGE_DUMMY
    return jack_transport_query(client, pos);
#else
    return JackTransportStopped;
#endif
}
