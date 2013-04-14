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

#ifndef __JACKBRIDGE_HPP__
#define __JACKBRIDGE_HPP__

#include "CarlaDefines.hpp"

#ifndef JACKBRIDGE_EXPORT
# undef CARLA_EXPORT
# define CARLA_EXPORT
#endif

#ifdef __WINE__
# define GNU_WIN32 // fix jack threads, always use pthread
#endif

#include <cstdint>
#include <sys/time.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>

CARLA_EXPORT const char*    jackbridge_get_version_string();
CARLA_EXPORT jack_client_t* jackbridge_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...);

CARLA_EXPORT int   jackbridge_client_close(jack_client_t* client);
CARLA_EXPORT int   jackbridge_client_name_size();
CARLA_EXPORT char* jackbridge_get_client_name(jack_client_t* client);

CARLA_EXPORT int  jackbridge_activate(jack_client_t* client);
CARLA_EXPORT int  jackbridge_deactivate(jack_client_t* client);
CARLA_EXPORT void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_port_registration_callback (jack_client_t* client, JackPortRegistrationCallback registration_callback, void *arg);
CARLA_EXPORT int  jackbridge_set_port_connect_callback (jack_client_t* client, JackPortConnectCallback connect_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_port_rename_callback (jack_client_t* client, JackPortRenameCallback rename_callback, void* arg);
CARLA_EXPORT int  jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg);

CARLA_EXPORT jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client);
CARLA_EXPORT jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client);
CARLA_EXPORT jack_port_t*   jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size);

CARLA_EXPORT int   jackbridge_port_unregister(jack_client_t* client, jack_port_t* port);
CARLA_EXPORT void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes);

//           port_register_ptr(nullptr),
//           port_unregister_ptr(nullptr),
//           port_get_buffer_ptr(nullptr),
//           port_name_ptr(nullptr),
//           port_short_name_ptr(nullptr),
//           port_flags_ptr(nullptr),
//           port_type_ptr(nullptr),
//           port_set_name_ptr(nullptr),
//           connect_ptr(nullptr),
//           disconnect_ptr(nullptr),

CARLA_EXPORT int  jackbridge_port_name_size();
CARLA_EXPORT void jackbridge_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range);
CARLA_EXPORT void jackbridge_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range);
CARLA_EXPORT int  jackbridge_recompute_total_latencies(jack_client_t* client);

CARLA_EXPORT uint32_t jackbridge_midi_get_event_count(void* port_buffer);
CARLA_EXPORT int      jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index);
CARLA_EXPORT void     jackbridge_midi_clear_buffer(void* port_buffer);
CARLA_EXPORT int      jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, size_t data_size);
CARLA_EXPORT jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size);

CARLA_EXPORT int  jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame);
CARLA_EXPORT void jackbridge_transport_start(jack_client_t* client);
CARLA_EXPORT void jackbridge_transport_stop(jack_client_t* client);
CARLA_EXPORT jack_transport_state_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos);

// CARLA_EXPORT void jackbridge_clock_gettime_rt(struct timespec*);
// CARLA_EXPORT int  jackbridge_sem_post(void*);
// CARLA_EXPORT int  jackbridge_sem_timedwait(void*, struct timespec*);

#endif // __JACKBRIDGE_HPP__
