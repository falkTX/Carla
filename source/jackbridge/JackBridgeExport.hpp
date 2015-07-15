/*
 * JackBridge (Part 3, Export)
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "JackBridge.hpp"

extern "C" {

// -----------------------------------------------------------------------------

typedef void        (JACKBRIDGE_API *jackbridgesym_get_version)(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr);
typedef const char* (JACKBRIDGE_API *jackbridgesym_get_version_string)(void);
typedef jack_client_t* (JACKBRIDGE_API *jackbridgesym_client_open)(const char* client_name, uint32_t options, jack_status_t* status);
typedef bool           (JACKBRIDGE_API *jackbridgesym_client_close)(jack_client_t* client);
typedef int   (JACKBRIDGE_API *jackbridgesym_client_name_size)(void);
typedef char* (JACKBRIDGE_API *jackbridgesym_get_client_name)(jack_client_t* client);
typedef char* (JACKBRIDGE_API *jackbridgesym_get_uuid_for_client_name)(jack_client_t* client, const char* name);
typedef char* (JACKBRIDGE_API *jackbridgesym_get_client_name_by_uuid)(jack_client_t* client, const char* uuid);
typedef bool (JACKBRIDGE_API *jackbridgesym_activate)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_deactivate)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_is_realtime)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_thread_init_callback)(jack_client_t* client, JackThreadInitCallback thread_init_callback, void* arg);
typedef void (JACKBRIDGE_API *jackbridgesym_on_shutdown)(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg);
typedef void (JACKBRIDGE_API *jackbridgesym_on_info_shutdown)(jack_client_t* client, JackInfoShutdownCallback shutdown_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_process_callback)(jack_client_t* client, JackProcessCallback process_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_freewheel_callback)(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_buffer_size_callback)(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_sample_rate_callback)(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_client_registration_callback)(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_port_registration_callback)(jack_client_t* client, JackPortRegistrationCallback registration_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_port_rename_callback)(jack_client_t* client, JackPortRenameCallback rename_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_port_connect_callback)(jack_client_t* client, JackPortConnectCallback connect_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_graph_order_callback)(jack_client_t* client, JackGraphOrderCallback graph_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_xrun_callback)(jack_client_t* client, JackXRunCallback xrun_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_latency_callback)(jack_client_t* client, JackLatencyCallback latency_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_freewheel)(jack_client_t* client, bool onoff);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_buffer_size)(jack_client_t* client, jack_nframes_t nframes);
typedef jack_nframes_t (JACKBRIDGE_API *jackbridgesym_get_sample_rate)(jack_client_t* client);
typedef jack_nframes_t (JACKBRIDGE_API *jackbridgesym_get_buffer_size)(jack_client_t* client);
typedef float          (JACKBRIDGE_API *jackbridgesym_cpu_load)(jack_client_t* client);
typedef jack_port_t* (JACKBRIDGE_API *jackbridgesym_port_register)(jack_client_t* client, const char* port_name, const char* port_type, uint64_t flags, uint64_t buffer_size);
typedef bool         (JACKBRIDGE_API *jackbridgesym_port_unregister)(jack_client_t* client, jack_port_t* port);
typedef void*        (JACKBRIDGE_API *jackbridgesym_port_get_buffer)(jack_port_t* port, jack_nframes_t nframes);
typedef const char*  (JACKBRIDGE_API *jackbridgesym_port_name)(const jack_port_t* port);
typedef jack_uuid_t  (JACKBRIDGE_API *jackbridgesym_port_uuid)(const jack_port_t* port);
typedef const char*  (JACKBRIDGE_API *jackbridgesym_port_short_name)(const jack_port_t* port);
typedef int          (JACKBRIDGE_API *jackbridgesym_port_flags)(const jack_port_t* port);
typedef const char*  (JACKBRIDGE_API *jackbridgesym_port_type)(const jack_port_t* port);
typedef bool         (JACKBRIDGE_API *jackbridgesym_port_is_mine)(const jack_client_t* client, const jack_port_t* port);
typedef int          (JACKBRIDGE_API *jackbridgesym_port_connected)(const jack_port_t* port);
typedef bool         (JACKBRIDGE_API *jackbridgesym_port_connected_to)(const jack_port_t* port, const char* port_name);
typedef const char** (JACKBRIDGE_API *jackbridgesym_port_get_connections)(const jack_port_t* port);
typedef const char** (JACKBRIDGE_API *jackbridgesym_port_get_all_connections)(const jack_client_t* client, const jack_port_t* port);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_set_name)(jack_port_t* port, const char* port_name);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_set_alias)(jack_port_t* port, const char* alias);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_unset_alias)(jack_port_t* port, const char* alias);
typedef int  (JACKBRIDGE_API *jackbridgesym_port_get_aliases)(const jack_port_t* port, char* const aliases[2]);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_request_monitor)(jack_port_t* port, bool onoff);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_request_monitor_by_name)(jack_client_t* client, const char* port_name, bool onoff);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_ensure_monitor)(jack_port_t* port, bool onoff);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_monitoring_input)(jack_port_t* port);
typedef bool (JACKBRIDGE_API *jackbridgesym_connect)(jack_client_t* client, const char* source_port, const char* destination_port);
typedef bool (JACKBRIDGE_API *jackbridgesym_disconnect)(jack_client_t* client, const char* source_port, const char* destination_port);
typedef bool (JACKBRIDGE_API *jackbridgesym_port_disconnect)(jack_client_t* client, jack_port_t* port);
typedef int      (JACKBRIDGE_API *jackbridgesym_port_name_size)(void);
typedef int      (JACKBRIDGE_API *jackbridgesym_port_type_size)(void);
typedef uint32_t (JACKBRIDGE_API *jackbridgesym_port_type_get_buffer_size)(jack_client_t* client, const char* port_type);
typedef void (JACKBRIDGE_API *jackbridgesym_port_get_latency_range)(jack_port_t* port, uint32_t mode, jack_latency_range_t* range);
typedef void (JACKBRIDGE_API *jackbridgesym_port_set_latency_range)(jack_port_t* port, uint32_t mode, jack_latency_range_t* range);
typedef bool (JACKBRIDGE_API *jackbridgesym_recompute_total_latencies)(jack_client_t* client);
typedef const char** (JACKBRIDGE_API *jackbridgesym_get_ports)(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, uint64_t flags);
typedef jack_port_t* (JACKBRIDGE_API *jackbridgesym_port_by_name)(jack_client_t* client, const char* port_name);
typedef jack_port_t* (JACKBRIDGE_API *jackbridgesym_port_by_id)(jack_client_t* client, jack_port_id_t port_id);
typedef void (JACKBRIDGE_API *jackbridgesym_free)(void* ptr);
typedef uint32_t (JACKBRIDGE_API *jackbridgesym_midi_get_event_count)(void* port_buffer);
typedef bool     (JACKBRIDGE_API *jackbridgesym_midi_event_get)(jack_midi_event_t* event, void* port_buffer, uint32_t event_index);
typedef void     (JACKBRIDGE_API *jackbridgesym_midi_clear_buffer)(void* port_buffer);
typedef bool     (JACKBRIDGE_API *jackbridgesym_midi_event_write)(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, uint32_t data_size);
typedef jack_midi_data_t* (JACKBRIDGE_API *jackbridgesym_midi_event_reserve)(void* port_buffer, jack_nframes_t time, uint32_t data_size);
typedef bool (JACKBRIDGE_API *jackbridgesym_release_timebase)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_sync_callback)(jack_client_t* client, JackSyncCallback sync_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_sync_timeout)(jack_client_t* client, jack_time_t timeout);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_timebase_callback)(jack_client_t* client, bool conditional, JackTimebaseCallback timebase_callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_transport_locate)(jack_client_t* client, jack_nframes_t frame);
typedef uint32_t       (JACKBRIDGE_API *jackbridgesym_transport_query)(const jack_client_t* client, jack_position_t* pos);
typedef jack_nframes_t (JACKBRIDGE_API *jackbridgesym_get_current_transport_frame)(const jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_transport_reposition)(jack_client_t* client, const jack_position_t* pos);
typedef void (JACKBRIDGE_API *jackbridgesym_transport_start)(jack_client_t* client);
typedef void (JACKBRIDGE_API *jackbridgesym_transport_stop)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_property)(jack_client_t* client, jack_uuid_t subject, const char* key, const char* value, const char* type);
typedef bool (JACKBRIDGE_API *jackbridgesym_get_property)(jack_uuid_t subject, const char* key, char** value, char** type);
typedef void (JACKBRIDGE_API *jackbridgesym_free_description)(jack_description_t* desc, bool free_description_itself);
typedef bool (JACKBRIDGE_API *jackbridgesym_get_properties)(jack_uuid_t subject, jack_description_t* desc);
typedef bool (JACKBRIDGE_API *jackbridgesym_get_all_properties)(jack_description_t** descs);
typedef bool (JACKBRIDGE_API *jackbridgesym_remove_property)(jack_client_t* client, jack_uuid_t subject, const char* key);
typedef int  (JACKBRIDGE_API *jackbridgesym_remove_properties)(jack_client_t* client, jack_uuid_t subject);
typedef bool (JACKBRIDGE_API *jackbridgesym_remove_all_properties)(jack_client_t* client);
typedef bool (JACKBRIDGE_API *jackbridgesym_set_property_change_callback)(jack_client_t* client, JackPropertyChangeCallback callback, void* arg);
typedef bool (JACKBRIDGE_API *jackbridgesym_sem_init)(void* sem);
typedef void (JACKBRIDGE_API *jackbridgesym_sem_destroy)(void* sem);
typedef void (JACKBRIDGE_API *jackbridgesym_sem_post)(void* sem);
typedef bool (JACKBRIDGE_API *jackbridgesym_sem_timedwait)(void* sem, uint secs);
typedef bool (JACKBRIDGE_API *jackbridgesym_shm_is_valid)(const void* shm);
typedef void (JACKBRIDGE_API *jackbridgesym_shm_init)(void* shm);
typedef void (JACKBRIDGE_API *jackbridgesym_shm_attach)(void* shm, const char* name);
typedef void (JACKBRIDGE_API *jackbridgesym_shm_close)(void* shm);
typedef void* (JACKBRIDGE_API *jackbridgesym_shm_map)(void* shm, uint64_t size);

// -----------------------------------------------------------------------------

struct _JackBridgeExportedFunctions {
    ulong unique1;
    jackbridgesym_get_version get_version_ptr;
    jackbridgesym_get_version_string get_version_string_ptr;
    jackbridgesym_client_open client_open_ptr;
    jackbridgesym_client_close client_close_ptr;
    jackbridgesym_client_name_size client_name_size_ptr;
    jackbridgesym_get_client_name get_client_name_ptr;
    jackbridgesym_get_uuid_for_client_name get_uuid_for_client_name_ptr;
    jackbridgesym_get_client_name_by_uuid get_client_name_by_uuid_ptr;
    jackbridgesym_activate activate_ptr;
    jackbridgesym_deactivate deactivate_ptr;
    jackbridgesym_is_realtime is_realtime_ptr;
    jackbridgesym_set_thread_init_callback set_thread_init_callback_ptr;
    jackbridgesym_on_shutdown on_shutdown_ptr;
    jackbridgesym_on_info_shutdown on_info_shutdown_ptr;
    jackbridgesym_set_process_callback set_process_callback_ptr;
    jackbridgesym_set_freewheel_callback set_freewheel_callback_ptr;
    jackbridgesym_set_buffer_size_callback set_buffer_size_callback_ptr;
    jackbridgesym_set_sample_rate_callback set_sample_rate_callback_ptr;
    jackbridgesym_set_client_registration_callback set_client_registration_callback_ptr;
    jackbridgesym_set_port_registration_callback set_port_registration_callback_ptr;
    jackbridgesym_set_port_rename_callback set_port_rename_callback_ptr;
    jackbridgesym_set_port_connect_callback set_port_connect_callback_ptr;
    jackbridgesym_set_graph_order_callback set_graph_order_callback_ptr;
    jackbridgesym_set_xrun_callback set_xrun_callback_ptr;
    jackbridgesym_set_latency_callback set_latency_callback_ptr;
    jackbridgesym_set_freewheel set_freewheel_ptr;
    jackbridgesym_set_buffer_size set_buffer_size_ptr;
    jackbridgesym_get_sample_rate get_sample_rate_ptr;
    jackbridgesym_get_buffer_size get_buffer_size_ptr;
    jackbridgesym_cpu_load cpu_load_ptr;
    jackbridgesym_port_register port_register_ptr;
    jackbridgesym_port_unregister port_unregister_ptr;
    jackbridgesym_port_get_buffer port_get_buffer_ptr;
    jackbridgesym_port_name port_name_ptr;
    jackbridgesym_port_uuid port_uuid_ptr;
    jackbridgesym_port_short_name port_short_name_ptr;
    jackbridgesym_port_flags port_flags_ptr;
    jackbridgesym_port_type port_type_ptr;
    jackbridgesym_port_is_mine port_is_mine_ptr;
    jackbridgesym_port_connected port_connected_ptr;
    jackbridgesym_port_connected_to port_connected_to_ptr;
    jackbridgesym_port_get_connections port_get_connections_ptr;
    jackbridgesym_port_get_all_connections port_get_all_connections_ptr;
    jackbridgesym_port_set_name port_set_name_ptr;
    jackbridgesym_port_set_alias port_set_alias_ptr;
    jackbridgesym_port_unset_alias port_unset_alias_ptr;
    jackbridgesym_port_get_aliases port_get_aliases_ptr;
    jackbridgesym_port_request_monitor port_request_monitor_ptr;
    jackbridgesym_port_request_monitor_by_name port_request_monitor_by_name_ptr;
    jackbridgesym_port_ensure_monitor port_ensure_monitor_ptr;
    jackbridgesym_port_monitoring_input port_monitoring_input_ptr;
    ulong unique2;
    jackbridgesym_connect connect_ptr;
    jackbridgesym_disconnect disconnect_ptr;
    jackbridgesym_port_disconnect port_disconnect_ptr;
    jackbridgesym_port_name_size port_name_size_ptr;
    jackbridgesym_port_type_size port_type_size_ptr;
    jackbridgesym_port_type_get_buffer_size port_type_get_buffer_size_ptr;
    jackbridgesym_port_get_latency_range port_get_latency_range_ptr;
    jackbridgesym_port_set_latency_range port_set_latency_range_ptr;
    jackbridgesym_recompute_total_latencies recompute_total_latencies_ptr;
    jackbridgesym_get_ports get_ports_ptr;
    jackbridgesym_port_by_name port_by_name_ptr;
    jackbridgesym_port_by_id port_by_id_ptr;
    jackbridgesym_free free_ptr;
    jackbridgesym_midi_get_event_count midi_get_event_count_ptr;
    jackbridgesym_midi_event_get midi_event_get_ptr;
    jackbridgesym_midi_clear_buffer midi_clear_buffer_ptr;
    jackbridgesym_midi_event_write midi_event_write_ptr;
    jackbridgesym_midi_event_reserve midi_event_reserve_ptr;
    jackbridgesym_release_timebase release_timebase_ptr;
    jackbridgesym_set_sync_callback set_sync_callback_ptr;
    jackbridgesym_set_sync_timeout set_sync_timeout_ptr;
    jackbridgesym_set_timebase_callback set_timebase_callback_ptr;
    jackbridgesym_transport_locate transport_locate_ptr;
    jackbridgesym_transport_query transport_query_ptr;
    jackbridgesym_get_current_transport_frame get_current_transport_frame_ptr;
    jackbridgesym_transport_reposition transport_reposition_ptr;
    jackbridgesym_transport_start transport_start_ptr;
    jackbridgesym_transport_stop transport_stop_ptr;
    jackbridgesym_set_property set_property_ptr;
    jackbridgesym_get_property get_property_ptr;
    jackbridgesym_free_description free_description_ptr;
    jackbridgesym_get_properties get_properties_ptr;
    jackbridgesym_get_all_properties get_all_properties_ptr;
    jackbridgesym_remove_property remove_property_ptr;
    jackbridgesym_remove_properties remove_properties_ptr;
    jackbridgesym_remove_all_properties remove_all_properties_ptr;
    jackbridgesym_set_property_change_callback set_property_change_callback_ptr;
    jackbridgesym_sem_init sem_init_ptr;
    jackbridgesym_sem_destroy sem_destroy_ptr;
    jackbridgesym_sem_post sem_post_ptr;
    jackbridgesym_sem_timedwait sem_timedwait_ptr;
    jackbridgesym_shm_is_valid shm_is_valid_ptr;
    jackbridgesym_shm_init shm_init_ptr;
    jackbridgesym_shm_attach shm_attach_ptr;
    jackbridgesym_shm_close shm_close_ptr;
    jackbridgesym_shm_map shm_map_ptr;
    ulong unique3;
};

typedef struct _JackBridgeExportedFunctions JackBridgeExportedFunctions;

// -----------------------------------------------------------------------------

typedef const JackBridgeExportedFunctions* (JACKBRIDGE_API *jackbridge_exported_function_type)();

// -----------------------------------------------------------------------------

} // extern "C"
