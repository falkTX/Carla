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

#ifndef __JACKBRIDGE_H__
#define __JACKBRIDGE_H__

#if defined(_WIN32) && ! defined(__WINE__)
# include <stdint.h>
# include <windows.h>
# define BRIDGE_EXPORT __declspec (dllexport)
#else
# define BRIDGE_EXPORT __attribute__ ((visibility("default")))
#endif

#ifdef __WINE__
# define GNU_WIN32 // fix jack_native_thread_t
#endif

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>

#ifdef JACKBRIDGE_EXPORT

#ifdef __cplusplus
extern "C" {
#endif

BRIDGE_EXPORT jack_client_t* jackbridge_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...);
BRIDGE_EXPORT int jackbridge_client_close(jack_client_t* client);
BRIDGE_EXPORT int jackbridge_client_name_size();
BRIDGE_EXPORT char* jackbridge_get_client_name(jack_client_t* client);
BRIDGE_EXPORT int jackbridge_port_name_size();
BRIDGE_EXPORT int jackbridge_recompute_total_latencies(jack_client_t* client);
BRIDGE_EXPORT void jackbridge_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range);
BRIDGE_EXPORT void jackbridge_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range);
BRIDGE_EXPORT int jackbridge_activate(jack_client_t* client);
BRIDGE_EXPORT int jackbridge_deactivate(jack_client_t* client);
BRIDGE_EXPORT void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg);
BRIDGE_EXPORT int jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg);
BRIDGE_EXPORT int jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg);
BRIDGE_EXPORT int jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg);
BRIDGE_EXPORT int jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg);
BRIDGE_EXPORT int jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg);
BRIDGE_EXPORT jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client);
BRIDGE_EXPORT jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client);
BRIDGE_EXPORT jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);
BRIDGE_EXPORT int jackbridge_port_unregister(jack_client_t* client, jack_port_t* port);
BRIDGE_EXPORT void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes);

BRIDGE_EXPORT uint32_t jackbridge_midi_get_event_count(void* port_buffer);
BRIDGE_EXPORT int jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index);
BRIDGE_EXPORT void jackbridge_midi_clear_buffer(void* port_buffer);
BRIDGE_EXPORT jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size);
BRIDGE_EXPORT int jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, size_t data_size);

BRIDGE_EXPORT int jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame);
BRIDGE_EXPORT jack_transport_state_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos);
BRIDGE_EXPORT void jackbridge_transport_start(jack_client_t* client);
BRIDGE_EXPORT void jackbridge_transport_stop(jack_client_t* client);

#ifdef __cplusplus
}
#endif

#else // JACKBRIDGE_EXPORT

#define jackbridge_client_open jack_client_open
#define jackbridge_client_close jack_client_close
#define jackbridge_client_name_size jack_client_name_size
#define jackbridge_get_client_name jack_get_client_name
#define jackbridge_port_name_size jack_port_name_size
#define jackbridge_recompute_total_latencies jack_recompute_total_latencies
#define jackbridge_port_get_latency_range jack_port_get_latency_range
#define jackbridge_port_set_latency_range jack_port_set_latency_range
#define jackbridge_activate jack_activate
#define jackbridge_deactivate jack_deactivate
#define jackbridge_on_shutdown jack_on_shutdown
#define jackbridge_set_latency_callback jack_set_latency_callback
#define jackbridge_set_process_callback jack_set_process_callback
#define jackbridge_set_freewheel_callback jack_set_freewheel_callback
#define jackbridge_set_buffer_size_callback jack_set_buffer_size_callback
#define jackbridge_set_sample_rate_callback jack_set_sample_rate_callback
#define jackbridge_get_sample_rate jack_get_sample_rate
#define jackbridge_get_buffer_size jack_get_buffer_size
#define jackbridge_port_register jack_port_register
#define jackbridge_port_unregister jack_port_unregister
#define jackbridge_port_get_buffer jack_port_get_buffer

#define jackbridge_midi_get_event_count jack_midi_get_event_count
#define jackbridge_midi_event_get jack_midi_event_get
#define jackbridge_midi_clear_buffer jack_midi_clear_buffer
#define jackbridge_midi_event_reserve jack_midi_event_reserve
#define jackbridge_midi_event_write jack_midi_event_write

#define jackbridge_transport_locate jack_transport_locate
#define jackbridge_transport_query jack_transport_query
#define jackbridge_transport_start jack_transport_start
#define jackbridge_transport_stop jack_transport_stop

#endif // JACKBRIDGE_EXPORT

#endif // __JACKBRIDGE_H__
