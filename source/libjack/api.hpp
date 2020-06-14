/*
 * Carla JACK API for external applications
 * Copyright (C) 2016-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_LIBJACK_API_H_INCLUDED
#define CARLA_LIBJACK_API_H_INCLUDED

#include "CarlaLibJackHints.h"
#include "../jackbridge/JackBridge.hpp"

#include <pthread.h>

extern "C" {

typedef uint32_t jack_port_type_id_t;
typedef void*(*JackThreadCallback)(void* arg);

// --------------------------------------------------------------------------------------------------------------------
// jack.h

CARLA_API void jack_get_version(int*, int*, int*, int*);
CARLA_API const char* jack_get_version_string(void);
CARLA_API jack_client_t* jack_client_open(const char*, jack_options_t, jack_status_t*, ...);
CARLA_API jack_client_t* jack_client_new(const char*);
CARLA_API int jack_client_close(jack_client_t*);
CARLA_API int jack_client_name_size(void);
CARLA_API char* jack_get_client_name(jack_client_t*);
CARLA_API char* jack_get_uuid_for_client_name(jack_client_t*, const char*);
CARLA_API char* jack_get_client_name_by_uuid(jack_client_t*, const char*);
CARLA_API int jack_internal_client_new(const char*, const char*, const char*);
CARLA_API void jack_internal_client_close(const char*);
CARLA_API int jack_activate(jack_client_t*);
CARLA_API int jack_deactivate(jack_client_t*);
CARLA_API int jack_get_client_pid(const char*);
CARLA_API pthread_t jack_client_thread_id(jack_client_t*);
CARLA_API int jack_is_realtime(jack_client_t*);
CARLA_API jack_nframes_t jack_thread_wait(jack_client_t*, int);
CARLA_API jack_nframes_t jack_cycle_wait(jack_client_t*);
CARLA_API void jack_cycle_signal(jack_client_t*, int);
CARLA_API int jack_set_process_thread(jack_client_t*, JackThreadCallback, void*);
CARLA_API int jack_set_thread_init_callback(jack_client_t*, JackThreadInitCallback, void*);
CARLA_API void jack_on_shutdown(jack_client_t*, JackShutdownCallback, void*);
CARLA_API void jack_on_info_shutdown(jack_client_t*, JackInfoShutdownCallback, void*);
CARLA_API int jack_set_process_callback(jack_client_t*, JackProcessCallback, void*);
CARLA_API int jack_set_freewheel_callback(jack_client_t*, JackFreewheelCallback, void*);
CARLA_API int jack_set_buffer_size_callback(jack_client_t*, JackBufferSizeCallback, void*);
CARLA_API int jack_set_sample_rate_callback(jack_client_t*, JackSampleRateCallback, void*);
CARLA_API int jack_set_client_registration_callback(jack_client_t*, JackClientRegistrationCallback, void*);
CARLA_API int jack_set_port_registration_callback(jack_client_t*, JackPortRegistrationCallback, void*);
CARLA_API int jack_set_port_connect_callback(jack_client_t*, JackPortConnectCallback, void*);
CARLA_API int jack_set_port_rename_callback(jack_client_t*, JackPortRenameCallback, void*);
CARLA_API int jack_set_graph_order_callback(jack_client_t*, JackGraphOrderCallback, void*);
CARLA_API int jack_set_xrun_callback(jack_client_t*, JackXRunCallback, void*);
CARLA_API int jack_set_latency_callback(jack_client_t*, JackLatencyCallback, void*);
CARLA_API int jack_set_freewheel(jack_client_t*, int);
CARLA_API int jack_set_buffer_size(jack_client_t*, jack_nframes_t);
CARLA_API jack_nframes_t jack_get_sample_rate(jack_client_t*);
CARLA_API jack_nframes_t jack_get_buffer_size(jack_client_t*);
CARLA_API int jack_engine_takeover_timebase(jack_client_t*);
CARLA_API float jack_cpu_load(jack_client_t*);
CARLA_API jack_port_t* jack_port_register(jack_client_t*, const char*, const char*, unsigned long, unsigned long);
CARLA_API int jack_port_unregister(jack_client_t*, jack_port_t*);
CARLA_API void* jack_port_get_buffer(jack_port_t*, jack_nframes_t);
CARLA_API jack_uuid_t jack_port_uuid(const jack_port_t*);
CARLA_API const char* jack_port_name(const jack_port_t*);
CARLA_API const char* jack_port_short_name(const jack_port_t*);
CARLA_API int jack_port_flags(const jack_port_t*);
CARLA_API const char* jack_port_type(const jack_port_t*);
CARLA_API jack_port_type_id_t jack_port_type_id(const jack_port_t*);
CARLA_API int jack_port_is_mine(const jack_client_t*, const jack_port_t*);
CARLA_API int jack_port_connected(const jack_port_t*);
CARLA_API int jack_port_connected_to(const jack_port_t*, const char*);
CARLA_API const char** jack_port_get_connections(const jack_port_t*);
CARLA_API const char** jack_port_get_all_connections(const jack_client_t*, const jack_port_t*);
CARLA_API int jack_port_tie(jack_port_t*, jack_port_t*);
CARLA_API int jack_port_untie(jack_port_t*);
CARLA_API int jack_port_set_name(jack_port_t*, const char*);
CARLA_API int jack_port_rename(jack_client_t*, jack_port_t*, const char*);
CARLA_API int jack_port_set_alias(jack_port_t*, const char*);
CARLA_API int jack_port_unset_alias(jack_port_t*, const char*);
CARLA_API int jack_port_get_aliases(const jack_port_t*, char* const aliases[2]);
CARLA_API int jack_port_request_monitor(jack_port_t*, int);
CARLA_API int jack_port_request_monitor_by_name(jack_client_t*, const char*, int);
CARLA_API int jack_port_ensure_monitor(jack_port_t*, int);
CARLA_API int jack_port_monitoring_input(jack_port_t*);
CARLA_API int jack_connect(jack_client_t*, const char*, const char*);
CARLA_API int jack_disconnect(jack_client_t*, const char*, const char*);
CARLA_API int jack_port_disconnect(jack_client_t*, jack_port_t*);
CARLA_API int jack_port_name_size(void);
CARLA_API int jack_port_type_size(void);
CARLA_API size_t jack_port_type_get_buffer_size(jack_client_t*, const char*);
CARLA_API void jack_port_set_latency(jack_port_t*, jack_nframes_t);
CARLA_API void jack_port_get_latency_range(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t*);
CARLA_API void jack_port_set_latency_range(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t*);
CARLA_API int jack_recompute_total_latencies(jack_client_t*);
CARLA_API jack_nframes_t jack_port_get_latency(jack_port_t*);
CARLA_API jack_nframes_t jack_port_get_total_latency(jack_client_t*, jack_port_t*);
CARLA_API int jack_recompute_total_latency(jack_client_t*, jack_port_t*);
CARLA_API const char** jack_get_ports(jack_client_t*, const char*, const char*, unsigned long);
CARLA_API jack_port_t* jack_port_by_name(jack_client_t*, const char*);
CARLA_API jack_port_t* jack_port_by_id(jack_client_t*, jack_port_id_t);
CARLA_API jack_nframes_t jack_frames_since_cycle_start(const jack_client_t*);
CARLA_API jack_nframes_t jack_frame_time(const jack_client_t*);
CARLA_API jack_nframes_t jack_last_frame_time(const jack_client_t*);
CARLA_API int jack_get_cycle_times(const jack_client_t*, jack_nframes_t*, jack_time_t*, jack_time_t*, float*);
CARLA_API jack_time_t jack_frames_to_time(const jack_client_t*, jack_nframes_t);
CARLA_API jack_nframes_t jack_time_to_frames(const jack_client_t*, jack_time_t);
CARLA_API jack_time_t jack_get_time(void);
CARLA_API void jack_free(void*);

CARLA_API extern void(*jack_error_callback)(const char*msg);
CARLA_API void jack_set_error_function(void (*func)(const char*));

CARLA_API extern void (*jack_info_callback)(const char*msg);
CARLA_API void jack_set_info_function(void (*func)(const char*));

// --------------------------------------------------------------------------------------------------------------------
// medadata.h

CARLA_API int jack_set_property(jack_client_t*, jack_uuid_t, const char*, const char*, const char*);
CARLA_API int jack_get_property(jack_uuid_t, const char*, char**, char**);
CARLA_API void jack_free_description(jack_description_t*, int);
CARLA_API int jack_get_properties(jack_uuid_t, jack_description_t*);
CARLA_API int jack_get_all_properties(jack_description_t**);
CARLA_API int jack_remove_property(jack_client_t*, jack_uuid_t, const char*);
CARLA_API int jack_remove_properties(jack_client_t*, jack_uuid_t);
CARLA_API int jack_remove_all_properties(jack_client_t*);
CARLA_API int jack_set_property_change_callback(jack_client_t*, JackPropertyChangeCallback, void*);

// extern const char* JACK_METADATA_PRETTY_NAME;
// extern const char* JACK_METADATA_HARDWARE;
// extern const char* JACK_METADATA_CONNECTED;
// extern const char* JACK_METADATA_PORT_GROUP;
// extern const char* JACK_METADATA_ICON_SMALL;
// extern const char* JACK_METADATA_ICON_LARGE;

// --------------------------------------------------------------------------------------------------------------------
// midiport.h

CARLA_API uint32_t jack_midi_get_event_count(void*);
CARLA_API int jack_midi_event_get(jack_midi_event_t*, void*, uint32_t);
CARLA_API void jack_midi_clear_buffer(void*);
CARLA_API void jack_midi_reset_buffer(void*);
CARLA_API size_t jack_midi_max_event_size(void*);
CARLA_API jack_midi_data_t* jack_midi_event_reserve(void*, jack_nframes_t, size_t);
CARLA_API int jack_midi_event_write(void*, jack_nframes_t, const jack_midi_data_t*, size_t);
CARLA_API uint32_t jack_midi_get_lost_event_count(void*);

// --------------------------------------------------------------------------------------------------------------------
// thread.h

CARLA_API int jack_client_real_time_priority(jack_client_t*);
CARLA_API int jack_client_max_real_time_priority(jack_client_t*);
CARLA_API int jack_acquire_real_time_scheduling(pthread_t, int );
CARLA_API int jack_client_create_thread(jack_client_t*, pthread_t*, int, int, void*(*start_routine)(void*), void*);
CARLA_API int jack_drop_real_time_scheduling(pthread_t);
CARLA_API int jack_client_stop_thread(jack_client_t*, pthread_t);
CARLA_API int jack_client_kill_thread(jack_client_t*, pthread_t);

typedef int (*jack_thread_creator_t)(pthread_t*, const pthread_attr_t*, void* (*function)(void*), void* arg);
CARLA_API void jack_set_thread_creator(jack_thread_creator_t);

// --------------------------------------------------------------------------------------------------------------------
// session.h

CARLA_API int jack_set_session_callback(jack_client_t*, JackSessionCallback, void*);
CARLA_API int jack_session_reply(jack_client_t*, jack_session_event_t*);
CARLA_API void jack_session_event_free(jack_session_event_t*);
CARLA_API char* jack_client_get_uuid(jack_client_t*);
CARLA_API jack_session_command_t* jack_session_notify( jack_client_t*, const char*, jack_session_event_type_t, const char*);
CARLA_API void jack_session_commands_free(jack_session_command_t*);
CARLA_API int jack_reserve_client_name(jack_client_t*, const char*, const char*);
CARLA_API int jack_client_has_session_callback(jack_client_t*, const char*);

// --------------------------------------------------------------------------------------------------------------------
// statistics.h

CARLA_API float jack_get_max_delayed_usecs(jack_client_t*);
CARLA_API float jack_get_xrun_delayed_usecs(jack_client_t*);
CARLA_API void jack_reset_max_delayed_usecs(jack_client_t*);

// --------------------------------------------------------------------------------------------------------------------
// transport.h

CARLA_API int jack_release_timebase(jack_client_t*);
CARLA_API int jack_set_sync_callback(jack_client_t*, JackSyncCallback, void*);
CARLA_API int jack_set_sync_timeout(jack_client_t*, jack_time_t);
CARLA_API int jack_set_timebase_callback(jack_client_t*, int, JackTimebaseCallback, void*);
CARLA_API int jack_transport_locate(jack_client_t*, jack_nframes_t);
CARLA_API jack_transport_state_t jack_transport_query(const jack_client_t*, jack_position_t*);
CARLA_API jack_nframes_t jack_get_current_transport_frame(const jack_client_t*);
CARLA_API int jack_transport_reposition(jack_client_t*, const jack_position_t*);
CARLA_API void jack_transport_start(jack_client_t*);
CARLA_API void jack_transport_stop(jack_client_t*);
CARLA_API void jack_get_transport_info(jack_client_t*, void*);
CARLA_API void jack_set_transport_info(jack_client_t*, void*);

// --------------------------------------------------------------------------------------------------------------------
// uuid.h

#define JACK_UUID_SIZE 36
#define JACK_UUID_STRING_SIZE (JACK_UUID_SIZE+1) /* includes trailing null */
#define JACK_UUID_EMPTY_INITIALIZER 0

CARLA_API jack_uuid_t jack_client_uuid_generate(void);
CARLA_API jack_uuid_t jack_port_uuid_generate(uint32_t port_id);
CARLA_API uint32_t jack_uuid_to_index(jack_uuid_t);

CARLA_API int jack_uuid_compare(jack_uuid_t, jack_uuid_t);
CARLA_API void jack_uuid_copy(jack_uuid_t* dst, jack_uuid_t src);
CARLA_API void jack_uuid_clear(jack_uuid_t*);
CARLA_API int jack_uuid_parse(const char*buf, jack_uuid_t*);
CARLA_API void jack_uuid_unparse(jack_uuid_t, char buf[JACK_UUID_STRING_SIZE]);
CARLA_API int jack_uuid_empty(jack_uuid_t);

// --------------------------------------------------------------------------------------------------------------------

}

#endif
