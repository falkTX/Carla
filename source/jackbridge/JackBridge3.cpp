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

#include "JackBridgeExport.hpp"

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------------

#if defined(CARLA_OS_WIN) && ! defined(__WINE__)
# define JACKBRIDGE_EXPORT extern "C" __declspec (dllexport)
#else
# define JACKBRIDGE_EXPORT extern "C" __attribute__ ((visibility("default")))
#endif

// -----------------------------------------------------------------------------

JACKBRIDGE_EXPORT
const JackBridgeExportedFunctions* JACKBRIDGE_API jackbridge_get_exported_functions();

JACKBRIDGE_EXPORT
const JackBridgeExportedFunctions* JACKBRIDGE_API jackbridge_get_exported_functions()
{
    static JackBridgeExportedFunctions funcs;
    carla_zeroStruct(funcs);

    funcs.init_ptr                             = jackbridge_init;
    funcs.get_version_ptr                      = jackbridge_get_version;
    funcs.get_version_string_ptr               = jackbridge_get_version_string;
    funcs.client_open_ptr                      = jackbridge_client_open;
    funcs.client_close_ptr                     = jackbridge_client_close;
    funcs.client_name_size_ptr                 = jackbridge_client_name_size;
    funcs.get_client_name_ptr                  = jackbridge_get_client_name;
    funcs.get_uuid_for_client_name_ptr         = jackbridge_get_uuid_for_client_name;
    funcs.get_client_name_by_uuid_ptr          = jackbridge_get_client_name_by_uuid;
    funcs.activate_ptr                         = jackbridge_activate;
    funcs.deactivate_ptr                       = jackbridge_deactivate;
    funcs.is_realtime_ptr                      = jackbridge_is_realtime;
    funcs.set_thread_init_callback_ptr         = jackbridge_set_thread_init_callback;
    funcs.on_shutdown_ptr                      = jackbridge_on_shutdown;
    funcs.on_info_shutdown_ptr                 = jackbridge_on_info_shutdown;
    funcs.set_process_callback_ptr             = jackbridge_set_process_callback;
    funcs.set_freewheel_callback_ptr           = jackbridge_set_freewheel_callback;
    funcs.set_buffer_size_callback_ptr         = jackbridge_set_buffer_size_callback;
    funcs.set_sample_rate_callback_ptr         = jackbridge_set_sample_rate_callback;
    funcs.set_client_registration_callback_ptr = jackbridge_set_client_registration_callback;
    funcs.set_port_registration_callback_ptr   = jackbridge_set_port_registration_callback;
    funcs.set_port_rename_callback_ptr         = jackbridge_set_port_rename_callback;
    funcs.set_port_connect_callback_ptr        = jackbridge_set_port_connect_callback;
    funcs.set_graph_order_callback_ptr         = jackbridge_set_graph_order_callback;
    funcs.set_xrun_callback_ptr                = jackbridge_set_xrun_callback;
    funcs.set_latency_callback_ptr             = jackbridge_set_latency_callback;
    funcs.set_freewheel_ptr                    = jackbridge_set_freewheel;
    funcs.set_buffer_size_ptr                  = jackbridge_set_buffer_size;
    funcs.get_sample_rate_ptr                  = jackbridge_get_sample_rate;
    funcs.get_buffer_size_ptr                  = jackbridge_get_buffer_size;
    funcs.cpu_load_ptr                         = jackbridge_cpu_load;
    funcs.port_register_ptr                    = jackbridge_port_register;
    funcs.port_unregister_ptr                  = jackbridge_port_unregister;
    funcs.port_get_buffer_ptr                  = jackbridge_port_get_buffer;
    funcs.port_name_ptr                        = jackbridge_port_name;
    funcs.port_uuid_ptr                        = jackbridge_port_uuid;
    funcs.port_short_name_ptr                  = jackbridge_port_short_name;
    funcs.port_flags_ptr                       = jackbridge_port_flags;
    funcs.port_type_ptr                        = jackbridge_port_type;
    funcs.port_is_mine_ptr                     = jackbridge_port_is_mine;
    funcs.port_connected_ptr                   = jackbridge_port_connected;
    funcs.port_connected_to_ptr                = jackbridge_port_connected_to;
    funcs.port_get_connections_ptr             = jackbridge_port_get_connections;
    funcs.port_get_all_connections_ptr         = jackbridge_port_get_all_connections;
    funcs.port_rename_ptr                      = jackbridge_port_rename;
    funcs.port_set_alias_ptr                   = jackbridge_port_set_alias;
    funcs.port_unset_alias_ptr                 = jackbridge_port_unset_alias;
    funcs.port_get_aliases_ptr                 = jackbridge_port_get_aliases;
    funcs.port_request_monitor_ptr             = jackbridge_port_request_monitor;
    funcs.port_request_monitor_by_name_ptr     = jackbridge_port_request_monitor_by_name;
    funcs.port_ensure_monitor_ptr              = jackbridge_port_ensure_monitor;
    funcs.port_monitoring_input_ptr            = jackbridge_port_monitoring_input;
    funcs.connect_ptr                          = jackbridge_connect;
    funcs.disconnect_ptr                       = jackbridge_disconnect;
    funcs.port_disconnect_ptr                  = jackbridge_port_disconnect;
    funcs.port_name_size_ptr                   = jackbridge_port_name_size;
    funcs.port_type_size_ptr                   = jackbridge_port_type_size;
    funcs.port_type_get_buffer_size_ptr        = jackbridge_port_type_get_buffer_size;
    funcs.port_get_latency_range_ptr           = jackbridge_port_get_latency_range;
    funcs.port_set_latency_range_ptr           = jackbridge_port_set_latency_range;
    funcs.recompute_total_latencies_ptr        = jackbridge_recompute_total_latencies;
    funcs.get_ports_ptr                        = jackbridge_get_ports;
    funcs.port_by_name_ptr                     = jackbridge_port_by_name;
    funcs.port_by_id_ptr                       = jackbridge_port_by_id;
    funcs.free_ptr                             = jackbridge_free;
    funcs.midi_get_event_count_ptr             = jackbridge_midi_get_event_count;
    funcs.midi_event_get_ptr                   = jackbridge_midi_event_get;
    funcs.midi_clear_buffer_ptr                = jackbridge_midi_clear_buffer;
    funcs.midi_event_write_ptr                 = jackbridge_midi_event_write;
    funcs.midi_event_reserve_ptr               = jackbridge_midi_event_reserve;
    funcs.release_timebase_ptr                 = jackbridge_release_timebase;
    funcs.set_sync_callback_ptr                = jackbridge_set_sync_callback;
    funcs.set_sync_timeout_ptr                 = jackbridge_set_sync_timeout;
    funcs.set_timebase_callback_ptr            = jackbridge_set_timebase_callback;
    funcs.transport_locate_ptr                 = jackbridge_transport_locate;
    funcs.transport_query_ptr                  = jackbridge_transport_query;
    funcs.get_current_transport_frame_ptr      = jackbridge_get_current_transport_frame;
    funcs.transport_reposition_ptr             = jackbridge_transport_reposition;
    funcs.transport_start_ptr                  = jackbridge_transport_start;
    funcs.transport_stop_ptr                   = jackbridge_transport_stop;
    funcs.set_property_ptr                     = jackbridge_set_property;
    funcs.get_property_ptr                     = jackbridge_get_property;
    funcs.free_description_ptr                 = jackbridge_free_description;
    funcs.get_properties_ptr                   = jackbridge_get_properties;
    funcs.get_all_properties_ptr               = jackbridge_get_all_properties;
    funcs.remove_property_ptr                  = jackbridge_remove_property;
    funcs.remove_properties_ptr                = jackbridge_remove_properties;
    funcs.remove_all_properties_ptr            = jackbridge_remove_all_properties;
    funcs.set_property_change_callback_ptr     = jackbridge_set_property_change_callback;
    funcs.sem_init_ptr                         = jackbridge_sem_init;
    funcs.sem_destroy_ptr                      = jackbridge_sem_destroy;
    funcs.sem_post_ptr                         = jackbridge_sem_post;
    funcs.sem_timedwait_ptr                    = jackbridge_sem_timedwait;
    funcs.shm_is_valid_ptr                     = jackbridge_shm_is_valid;
    funcs.shm_init_ptr                         = jackbridge_shm_init;
    funcs.shm_attach_ptr                       = jackbridge_shm_attach;
    funcs.shm_close_ptr                        = jackbridge_shm_close;
    funcs.shm_map_ptr                          = jackbridge_shm_map;

    funcs.unique1 = funcs.unique2 = funcs.unique3 = 0xdeadf00d;

    return &funcs;
}

// -----------------------------------------------------------------------------
