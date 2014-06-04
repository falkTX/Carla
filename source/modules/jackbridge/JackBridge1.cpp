/*
 * JackBridge (Part 1, JACK functions)
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#if ! (defined(JACKBRIDGE_DIRECT) || defined(JACKBRIDGE_DUMMY))

#include "CarlaLibUtils.hpp"

// -----------------------------------------------------------------------------

typedef void        (*jacksym_get_version)(int*, int*, int*, int*);
typedef const char* (*jacksym_get_version_string)();

typedef jack_client_t* (*jacksym_client_open)(const char*, jack_options_t, jack_status_t*, ...);
typedef const char*    (*jacksym_client_rename)(jack_client_t* client, const char* new_name);
typedef int            (*jacksym_client_close)(jack_client_t*);

typedef int   (*jacksym_client_name_size)();
typedef char* (*jacksym_get_client_name)(jack_client_t*);

typedef char* (*jacksym_get_uuid_for_client_name)(jack_client_t*, const char*);
typedef char* (*jacksym_get_client_name_by_uuid)(jack_client_t*, const char*);

typedef int (*jacksym_activate)(jack_client_t*);
typedef int (*jacksym_deactivate)(jack_client_t*);
typedef int (*jacksym_is_realtime)(jack_client_t*);

typedef int  (*jacksym_set_thread_init_callback)(jack_client_t*, JackThreadInitCallback, void*);
typedef void (*jacksym_on_shutdown)(jack_client_t*, JackShutdownCallback, void*);
typedef void (*jacksym_on_info_shutdown)(jack_client_t*, JackInfoShutdownCallback, void*);
typedef int  (*jacksym_set_process_callback)(jack_client_t*, JackProcessCallback, void*);
typedef int  (*jacksym_set_freewheel_callback)(jack_client_t*, JackFreewheelCallback, void*);
typedef int  (*jacksym_set_buffer_size_callback)(jack_client_t*, JackBufferSizeCallback, void*);
typedef int  (*jacksym_set_sample_rate_callback)(jack_client_t*, JackSampleRateCallback, void*);
typedef int  (*jacksym_set_client_registration_callback)(jack_client_t*, JackClientRegistrationCallback, void*);
typedef int  (*jacksym_set_port_registration_callback)(jack_client_t*, JackPortRegistrationCallback, void*);
typedef int  (*jacksym_set_port_rename_callback)(jack_client_t*, JackPortRenameCallback, void*);
typedef int  (*jacksym_set_port_connect_callback)(jack_client_t*, JackPortConnectCallback, void*);
typedef int  (*jacksym_set_graph_order_callback)(jack_client_t*, JackGraphOrderCallback, void*);
typedef int  (*jacksym_set_xrun_callback)(jack_client_t*, JackXRunCallback, void*);
typedef int  (*jacksym_set_latency_callback)(jack_client_t*, JackLatencyCallback, void*);

typedef int (*jacksym_set_freewheel)(jack_client_t*, int);
typedef int (*jacksym_set_buffer_size)(jack_client_t*, jack_nframes_t);

typedef jack_nframes_t (*jacksym_get_sample_rate)(jack_client_t*);
typedef jack_nframes_t (*jacksym_get_buffer_size)(jack_client_t*);
typedef float          (*jacksym_cpu_load)(jack_client_t*);

typedef jack_port_t* (*jacksym_port_register)(jack_client_t*, const char*, const char*, unsigned long, unsigned long);
typedef int          (*jacksym_port_unregister)(jack_client_t*, jack_port_t*);
typedef void*        (*jacksym_port_get_buffer)(jack_port_t*, jack_nframes_t);

typedef const char*  (*jacksym_port_name)(const jack_port_t*);
typedef jack_uuid_t  (*jacksym_port_uuid)(const jack_port_t*);
typedef const char*  (*jacksym_port_short_name)(const jack_port_t*);
typedef int          (*jacksym_port_flags)(const jack_port_t*);
typedef const char*  (*jacksym_port_type)(const jack_port_t*);
typedef int          (*jacksym_port_is_mine)(const jack_client_t*, const jack_port_t*);
typedef int          (*jacksym_port_connected)(const jack_port_t*);
typedef int          (*jacksym_port_connected_to)(const jack_port_t*, const char*);
typedef const char** (*jacksym_port_get_connections)(const jack_port_t*);
typedef const char** (*jacksym_port_get_all_connections)(const jack_client_t*, const jack_port_t*);

typedef int (*jacksym_port_set_name)(jack_port_t*, const char*);
typedef int (*jacksym_port_set_alias)(jack_port_t*, const char*);
typedef int (*jacksym_port_unset_alias)(jack_port_t*, const char*);
typedef int (*jacksym_port_get_aliases)(const jack_port_t*, char* const aliases[2]);

typedef int (*jacksym_port_request_monitor)(jack_port_t*, int);
typedef int (*jacksym_port_request_monitor_by_name)(jack_client_t*, const char*, int);
typedef int (*jacksym_port_ensure_monitor)(jack_port_t*, int);
typedef int (*jacksym_port_monitoring_input)(jack_port_t*);

typedef int (*jacksym_connect)(jack_client_t*, const char*, const char*);
typedef int (*jacksym_disconnect)(jack_client_t*, const char*, const char*);
typedef int (*jacksym_port_disconnect)(jack_client_t*, jack_port_t*);

typedef int    (*jacksym_port_name_size)();
typedef int    (*jacksym_port_type_size)();
typedef size_t (*jacksym_port_type_get_buffer_size)(jack_client_t*, const char*);

typedef void (*jacksym_port_get_latency_range)(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t*);
typedef void (*jacksym_port_set_latency_range)(jack_port_t*, jack_latency_callback_mode_t, jack_latency_range_t*);
typedef int  (*jacksym_recompute_total_latencies)(jack_client_t*);

typedef const char** (*jacksym_get_ports)(jack_client_t*, const char*, const char*, unsigned long);
typedef jack_port_t* (*jacksym_port_by_name)(jack_client_t*, const char*);
typedef jack_port_t* (*jacksym_port_by_id)(jack_client_t*, jack_port_id_t);

typedef void (*jacksym_free)(void*);

typedef uint32_t (*jacksym_midi_get_event_count)(void*);
typedef int      (*jacksym_midi_event_get)(jack_midi_event_t*, void*, uint32_t);
typedef void     (*jacksym_midi_clear_buffer)(void*);
typedef int      (*jacksym_midi_event_write)(void*, jack_nframes_t, const jack_midi_data_t*, size_t);
typedef jack_midi_data_t* (*jacksym_midi_event_reserve)(void*, jack_nframes_t, size_t);

typedef int (*jacksym_release_timebase)(jack_client_t*);
typedef int (*jacksym_set_sync_callback)(jack_client_t*, JackSyncCallback, void*);
typedef int (*jacksym_set_sync_timeout)(jack_client_t*, jack_time_t);
typedef int (*jacksym_set_timebase_callback)(jack_client_t*, int, JackTimebaseCallback, void*);
typedef int (*jacksym_transport_locate)(jack_client_t*, jack_nframes_t);

typedef jack_transport_state_t (*jacksym_transport_query)(const jack_client_t*, jack_position_t*);
typedef jack_nframes_t         (*jacksym_get_current_transport_frame)(const jack_client_t*);

typedef int  (*jacksym_transport_reposition)(jack_client_t*, const jack_position_t*);
typedef void (*jacksym_transport_start)(jack_client_t*);
typedef void (*jacksym_transport_stop)(jack_client_t*);

typedef int  (*jacksym_set_property)(jack_client_t*, jack_uuid_t, const char*, const char*, const char*);
typedef int  (*jacksym_get_property)(jack_uuid_t, const char*, char**, char**);
typedef void (*jacksym_free_description)(jack_description_t*, int);
typedef int  (*jacksym_get_properties)(jack_uuid_t, jack_description_t*);
typedef int  (*jacksym_get_all_properties)(jack_description_t**);
typedef int  (*jacksym_remove_property)(jack_client_t*, jack_uuid_t, const char*);
typedef int  (*jacksym_remove_properties)(jack_client_t*, jack_uuid_t);
typedef int  (*jacksym_remove_all_properties)(jack_client_t*);
typedef int  (*jacksym_set_property_change_callback)(jack_client_t*, JackPropertyChangeCallback, void*);

// -----------------------------------------------------------------------------

struct JackBridge {
    void* lib;

    jacksym_get_version get_version_ptr;
    jacksym_get_version_string get_version_string_ptr;

    jacksym_client_open client_open_ptr;
    jacksym_client_rename client_rename_ptr;
    jacksym_client_close client_close_ptr;

    jacksym_client_name_size client_name_size_ptr;
    jacksym_get_client_name get_client_name_ptr;

    jacksym_get_uuid_for_client_name get_uuid_for_client_name_ptr;
    jacksym_get_client_name_by_uuid get_client_name_by_uuid_ptr;

    jacksym_activate activate_ptr;
    jacksym_deactivate deactivate_ptr;
    jacksym_is_realtime is_realtime_ptr;

    jacksym_set_thread_init_callback set_thread_init_callback_ptr;
    jacksym_on_shutdown on_shutdown_ptr;
    jacksym_on_info_shutdown on_info_shutdown_ptr;
    jacksym_set_process_callback set_process_callback_ptr;
    jacksym_set_freewheel_callback set_freewheel_callback_ptr;
    jacksym_set_buffer_size_callback set_buffer_size_callback_ptr;
    jacksym_set_sample_rate_callback set_sample_rate_callback_ptr;
    jacksym_set_client_registration_callback set_client_registration_callback_ptr;
    jacksym_set_port_registration_callback set_port_registration_callback_ptr;
    jacksym_set_port_rename_callback set_port_rename_callback_ptr;
    jacksym_set_port_connect_callback set_port_connect_callback_ptr;
    jacksym_set_xrun_callback set_xrun_callback_ptr;
    jacksym_set_latency_callback set_latency_callback_ptr;

    jacksym_set_freewheel set_freewheel_ptr;
    jacksym_set_buffer_size set_buffer_size_ptr;

    jacksym_get_sample_rate get_sample_rate_ptr;
    jacksym_get_buffer_size get_buffer_size_ptr;
    jacksym_cpu_load cpu_load_ptr;

    jacksym_port_register port_register_ptr;
    jacksym_port_unregister port_unregister_ptr;
    jacksym_port_get_buffer port_get_buffer_ptr;

    jacksym_port_name port_name_ptr;
    jacksym_port_uuid port_uuid_ptr;
    jacksym_port_short_name port_short_name_ptr;
    jacksym_port_flags port_flags_ptr;
    jacksym_port_type port_type_ptr;
    jacksym_port_is_mine port_is_mine_ptr;
    jacksym_port_connected port_connected_ptr;
    jacksym_port_connected_to port_connected_to_ptr;
    jacksym_port_get_connections port_get_connections_ptr;
    jacksym_port_get_all_connections port_get_all_connections_ptr;

    jacksym_port_set_name port_set_name_ptr;
    jacksym_port_set_alias port_set_alias_ptr;
    jacksym_port_unset_alias port_unset_alias_ptr;
    jacksym_port_get_aliases port_get_aliases_ptr;

    jacksym_port_request_monitor port_request_monitor_ptr;
    jacksym_port_request_monitor_by_name port_request_monitor_by_name_ptr;
    jacksym_port_ensure_monitor port_ensure_monitor_ptr;
    jacksym_port_monitoring_input port_monitoring_input_ptr;

    jacksym_connect connect_ptr;
    jacksym_disconnect disconnect_ptr;
    jacksym_port_disconnect port_disconnect_ptr;

    jacksym_port_name_size port_name_size_ptr;
    jacksym_port_type_size port_type_size_ptr;
    jacksym_port_type_get_buffer_size port_type_get_buffer_size_ptr;

    jacksym_port_get_latency_range port_get_latency_range_ptr;
    jacksym_port_set_latency_range port_set_latency_range_ptr;
    jacksym_recompute_total_latencies recompute_total_latencies_ptr;

    jacksym_get_ports get_ports_ptr;
    jacksym_port_by_name port_by_name_ptr;
    jacksym_port_by_id port_by_id_ptr;

    jacksym_free free_ptr;

    jacksym_midi_get_event_count midi_get_event_count_ptr;
    jacksym_midi_event_get midi_event_get_ptr;
    jacksym_midi_clear_buffer midi_clear_buffer_ptr;
    jacksym_midi_event_write midi_event_write_ptr;
    jacksym_midi_event_reserve midi_event_reserve_ptr;

    jacksym_release_timebase release_timebase_ptr;
    jacksym_set_sync_callback set_sync_callback_ptr;
    jacksym_set_sync_timeout set_sync_timeout_ptr;
    jacksym_set_timebase_callback set_timebase_callback_ptr;
    jacksym_transport_locate transport_locate_ptr;

    jacksym_transport_query transport_query_ptr;
    jacksym_get_current_transport_frame get_current_transport_frame_ptr;

    jacksym_transport_reposition transport_reposition_ptr;
    jacksym_transport_start transport_start_ptr;
    jacksym_transport_stop transport_stop_ptr;

    jacksym_set_property set_property_ptr;
    jacksym_get_property get_property_ptr;
    jacksym_free_description free_description_ptr;
    jacksym_get_properties get_properties_ptr;
    jacksym_get_all_properties get_all_properties_ptr;
    jacksym_remove_property remove_property_ptr;
    jacksym_remove_properties remove_properties_ptr;
    jacksym_remove_all_properties remove_all_properties_ptr;
    jacksym_set_property_change_callback set_property_change_callback_ptr;

    JackBridge()
        : lib(nullptr),
          get_version_ptr(nullptr),
          get_version_string_ptr(nullptr),
          client_open_ptr(nullptr),
          client_rename_ptr(nullptr),
          client_close_ptr(nullptr),
          client_name_size_ptr(nullptr),
          get_client_name_ptr(nullptr),
          get_uuid_for_client_name_ptr(nullptr),
          get_client_name_by_uuid_ptr(nullptr),
          activate_ptr(nullptr),
          deactivate_ptr(nullptr),
          is_realtime_ptr(nullptr),
          set_thread_init_callback_ptr(nullptr),
          on_shutdown_ptr(nullptr),
          on_info_shutdown_ptr(nullptr),
          set_process_callback_ptr(nullptr),
          set_freewheel_callback_ptr(nullptr),
          set_buffer_size_callback_ptr(nullptr),
          set_sample_rate_callback_ptr(nullptr),
          set_client_registration_callback_ptr(nullptr),
          set_port_registration_callback_ptr(nullptr),
          set_port_rename_callback_ptr(nullptr),
          set_port_connect_callback_ptr(nullptr),
          set_xrun_callback_ptr(nullptr),
          set_latency_callback_ptr(nullptr),
          set_freewheel_ptr(nullptr),
          set_buffer_size_ptr(nullptr),
          get_sample_rate_ptr(nullptr),
          get_buffer_size_ptr(nullptr),
          cpu_load_ptr(nullptr),
          port_register_ptr(nullptr),
          port_unregister_ptr(nullptr),
          port_get_buffer_ptr(nullptr),
          port_name_ptr(nullptr),
          port_uuid_ptr(nullptr),
          port_short_name_ptr(nullptr),
          port_flags_ptr(nullptr),
          port_type_ptr(nullptr),
          port_is_mine_ptr(nullptr),
          port_connected_ptr(nullptr),
          port_connected_to_ptr(nullptr),
          port_get_connections_ptr(nullptr),
          port_get_all_connections_ptr(nullptr),
          port_set_name_ptr(nullptr),
          port_set_alias_ptr(nullptr),
          port_unset_alias_ptr(nullptr),
          port_get_aliases_ptr(nullptr),
          port_request_monitor_ptr(nullptr),
          port_request_monitor_by_name_ptr(nullptr),
          port_ensure_monitor_ptr(nullptr),
          port_monitoring_input_ptr(nullptr),
          connect_ptr(nullptr),
          disconnect_ptr(nullptr),
          port_disconnect_ptr(nullptr),
          port_name_size_ptr(nullptr),
          port_type_size_ptr(nullptr),
          port_type_get_buffer_size_ptr(nullptr),
          port_get_latency_range_ptr(nullptr),
          port_set_latency_range_ptr(nullptr),
          recompute_total_latencies_ptr(nullptr),
          get_ports_ptr(nullptr),
          port_by_name_ptr(nullptr),
          port_by_id_ptr(nullptr),
          free_ptr(nullptr),
          midi_get_event_count_ptr(nullptr),
          midi_event_get_ptr(nullptr),
          midi_clear_buffer_ptr(nullptr),
          midi_event_write_ptr(nullptr),
          midi_event_reserve_ptr(nullptr),
          release_timebase_ptr(nullptr),
          set_sync_callback_ptr(nullptr),
          set_sync_timeout_ptr(nullptr),
          set_timebase_callback_ptr(nullptr),
          transport_locate_ptr(nullptr),
          transport_query_ptr(nullptr),
          get_current_transport_frame_ptr(nullptr),
          transport_reposition_ptr(nullptr),
          transport_start_ptr(nullptr),
          transport_stop_ptr(nullptr),
          set_property_ptr(nullptr),
          get_property_ptr(nullptr),
          free_description_ptr(nullptr),
          get_properties_ptr(nullptr),
          get_all_properties_ptr(nullptr),
          remove_property_ptr(nullptr),
          remove_properties_ptr(nullptr),
          remove_all_properties_ptr(nullptr),
          set_property_change_callback_ptr(nullptr)
    {
# if defined(CARLA_OS_MAC)
        const char* const filename("libjack.dylib");
# elif defined(CARLA_OS_WIN)
        const char* const filename("libjack.dll");
# else
        const char* const filename("libjack.so.0");
# endif

        lib = lib_open(filename);

        if (lib == nullptr)
        {
            fprintf(stderr, "Failed to load JACK DLL, reason:\n%s\n", lib_error(filename));
            return;
        }
        else
        {
            fprintf(stdout, "%s loaded sucessfully!\n", filename);
        }

        #define JOIN(a, b) a ## b
        #define LIB_SYMBOL(NAME) JOIN(NAME, _ptr) = (jacksym_##NAME)lib_symbol(lib, "jack_" #NAME);

        LIB_SYMBOL(get_version)
        LIB_SYMBOL(get_version_string)

        LIB_SYMBOL(client_open)
        LIB_SYMBOL(client_rename)
        LIB_SYMBOL(client_close)

        LIB_SYMBOL(client_name_size)
        LIB_SYMBOL(get_client_name)

        LIB_SYMBOL(get_uuid_for_client_name)
        LIB_SYMBOL(get_client_name_by_uuid)

        LIB_SYMBOL(activate)
        LIB_SYMBOL(deactivate)
        LIB_SYMBOL(is_realtime)

        LIB_SYMBOL(set_thread_init_callback)
        LIB_SYMBOL(on_shutdown)
        LIB_SYMBOL(on_info_shutdown)
        LIB_SYMBOL(set_process_callback)
        LIB_SYMBOL(set_freewheel_callback)
        LIB_SYMBOL(set_buffer_size_callback)
        LIB_SYMBOL(set_sample_rate_callback)
        LIB_SYMBOL(set_client_registration_callback)
        LIB_SYMBOL(set_port_registration_callback)
        LIB_SYMBOL(set_port_rename_callback)
        LIB_SYMBOL(set_port_connect_callback)
        LIB_SYMBOL(set_xrun_callback)
        LIB_SYMBOL(set_latency_callback)

        LIB_SYMBOL(set_freewheel)
        LIB_SYMBOL(set_buffer_size)

        LIB_SYMBOL(get_sample_rate)
        LIB_SYMBOL(get_buffer_size)
        LIB_SYMBOL(cpu_load)

        LIB_SYMBOL(port_register)
        LIB_SYMBOL(port_unregister)
        LIB_SYMBOL(port_get_buffer)

        LIB_SYMBOL(port_name)
        LIB_SYMBOL(port_uuid)
        LIB_SYMBOL(port_short_name)
        LIB_SYMBOL(port_flags)
        LIB_SYMBOL(port_type)
        LIB_SYMBOL(port_is_mine)
        LIB_SYMBOL(port_connected)
        LIB_SYMBOL(port_connected_to)
        LIB_SYMBOL(port_get_connections)
        LIB_SYMBOL(port_get_all_connections)

        LIB_SYMBOL(port_set_name)
        LIB_SYMBOL(port_set_alias)
        LIB_SYMBOL(port_unset_alias)
        LIB_SYMBOL(port_get_aliases)

        LIB_SYMBOL(port_request_monitor)
        LIB_SYMBOL(port_request_monitor_by_name)
        LIB_SYMBOL(port_ensure_monitor)
        LIB_SYMBOL(port_monitoring_input)

        LIB_SYMBOL(connect)
        LIB_SYMBOL(disconnect)
        LIB_SYMBOL(port_disconnect)

        LIB_SYMBOL(port_name_size)
        LIB_SYMBOL(port_type_size)
        LIB_SYMBOL(port_type_get_buffer_size)

        LIB_SYMBOL(port_get_latency_range)
        LIB_SYMBOL(port_set_latency_range)
        LIB_SYMBOL(recompute_total_latencies)

        LIB_SYMBOL(get_ports)
        LIB_SYMBOL(port_by_name)
        LIB_SYMBOL(port_by_id)

        LIB_SYMBOL(free)

        LIB_SYMBOL(midi_get_event_count)
        LIB_SYMBOL(midi_event_get)
        LIB_SYMBOL(midi_clear_buffer)
        LIB_SYMBOL(midi_event_write)
        LIB_SYMBOL(midi_event_reserve)

        LIB_SYMBOL(release_timebase)
        LIB_SYMBOL(set_sync_callback)
        LIB_SYMBOL(set_sync_timeout)
        LIB_SYMBOL(set_timebase_callback)
        LIB_SYMBOL(transport_locate)
        LIB_SYMBOL(transport_query)
        LIB_SYMBOL(get_current_transport_frame)
        LIB_SYMBOL(transport_reposition)
        LIB_SYMBOL(transport_start)
        LIB_SYMBOL(transport_stop)

        LIB_SYMBOL(set_property)
        LIB_SYMBOL(get_property)
        LIB_SYMBOL(free_description)
        LIB_SYMBOL(get_properties)
        LIB_SYMBOL(get_all_properties)
        LIB_SYMBOL(remove_property)
        LIB_SYMBOL(remove_properties)
        LIB_SYMBOL(remove_all_properties)
        LIB_SYMBOL(set_property_change_callback)

        #undef JOIN
        #undef LIB_SYMBOL
    }

    ~JackBridge()
    {
        if (lib != nullptr)
        {
            lib_close(lib);
            lib = nullptr;
        }
    }
};

static const JackBridge bridge;

#endif // ! JACKBRIDGE_DIRECT

// -----------------------------------------------------------------------------

bool jackbridge_is_ok() noexcept
{
#if defined(JACKBRIDGE_DUMMY) || defined(JACKBRIDGE_DIRECT)
    return true;
#else
    return (bridge.lib != nullptr);
#endif
}

// -----------------------------------------------------------------------------

void jackbridge_get_version(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_version(major_ptr, minor_ptr, micro_ptr, proto_ptr);
#else
    if (bridge.get_version_ptr != nullptr)
        return bridge.get_version_ptr(major_ptr, minor_ptr, micro_ptr, proto_ptr);
#endif
    if (major_ptr != nullptr)
        *major_ptr = 0;
    if (minor_ptr != nullptr)
        *minor_ptr = 0;
    if (micro_ptr != nullptr)
        *micro_ptr = 0;
    if (proto_ptr != nullptr)
        *proto_ptr = 0;
}

const char* jackbridge_get_version_string()
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_version_string();
#else
    if (bridge.get_version_string_ptr != nullptr)
        return bridge.get_version_string_ptr();
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

jack_client_t* jackbridge_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_client_open(client_name, options, status);
#else
    if (bridge.client_open_ptr != nullptr)
        return bridge.client_open_ptr(client_name, options, status);
#endif
    if (status != nullptr)
        *status = JackServerError;
    return nullptr;
}

bool jackbridge_client_close(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_client_close(client) == 0);
#else
    if (bridge.client_close_ptr != nullptr)
        return (bridge.client_close_ptr(client) == 0);
#endif
    return false;
}

// -----------------------------------------------------------------------------

int jackbridge_client_name_size()
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_client_name_size();
#else
    if (bridge.client_name_size_ptr != nullptr)
        return bridge.client_name_size_ptr();
#endif
    return 0;
}

char* jackbridge_get_client_name(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_client_name(client);
#else
    if (bridge.get_client_name_ptr != nullptr)
        return bridge.get_client_name_ptr(client);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

char* jackbridge_get_uuid_for_client_name(jack_client_t* client, const char* name)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_uuid_for_client_name(client, name);
#else
    if (bridge.get_uuid_for_client_name_ptr != nullptr)
        return bridge.get_uuid_for_client_name_ptr(client, name);
#endif
    return nullptr;
}

char* jackbridge_get_client_name_by_uuid(jack_client_t* client, const char* uuid)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_client_name_by_uuid(client, uuid);
#else
    if (bridge.get_client_name_by_uuid_ptr != nullptr)
        return bridge.get_client_name_by_uuid_ptr(client, uuid);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_activate(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_activate(client) == 0);
#else
    if (bridge.activate_ptr != nullptr)
        return (bridge.activate_ptr(client) == 0);
#endif
    return false;
}

bool jackbridge_deactivate(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_deactivate(client) == 0);
#else
    if (bridge.deactivate_ptr != nullptr)
        return (bridge.deactivate_ptr(client) == 0);
#endif
    return false;
}

bool jackbridge_is_realtime(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_is_realtime(client);
#else
    if (bridge.is_realtime_ptr != nullptr)
        return bridge.is_realtime_ptr(client);
#endif
    return false;
}

// -----------------------------------------------------------------------------

bool jackbridge_set_thread_init_callback(jack_client_t* client, JackThreadInitCallback thread_init_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_thread_init_callback(client, thread_init_callback, arg) == 0);
#else
    if (bridge.set_thread_init_callback_ptr != nullptr)
        return (bridge.set_thread_init_callback_ptr(client, thread_init_callback, arg) == 0);
#endif
    return false;
}

void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_on_shutdown(client, shutdown_callback, arg);
#else
    if (bridge.on_shutdown_ptr != nullptr)
        bridge.on_shutdown_ptr(client, shutdown_callback, arg);
#endif
}

void jackbridge_on_info_shutdown(jack_client_t* client, JackInfoShutdownCallback shutdown_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_on_info_shutdown(client, shutdown_callback, arg);
#else
    if (bridge.on_info_shutdown_ptr != nullptr)
        bridge.on_info_shutdown_ptr(client, shutdown_callback, arg);
#endif
}

bool jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_process_callback(client, process_callback, arg) == 0);
#else
    if (bridge.set_process_callback_ptr != nullptr)
        return (bridge.set_process_callback_ptr(client, process_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_freewheel_callback(client, freewheel_callback, arg) == 0);
#else
    if (bridge.set_freewheel_callback_ptr != nullptr)
        return (bridge.set_freewheel_callback_ptr(client, freewheel_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_buffer_size_callback(client, bufsize_callback, arg) == 0);
#else
    if (bridge.set_buffer_size_callback_ptr != nullptr)
        return (bridge.set_buffer_size_callback_ptr(client, bufsize_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_sample_rate_callback(client, srate_callback, arg) == 0);
#else
    if (bridge.set_sample_rate_callback_ptr != nullptr)
        return (bridge.set_sample_rate_callback_ptr(client, srate_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_client_registration_callback(client, registration_callback, arg) == 0);
#else
    if (bridge.set_client_registration_callback_ptr != nullptr)
        return (bridge.set_client_registration_callback_ptr(client, registration_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_port_registration_callback(jack_client_t* client, JackPortRegistrationCallback registration_callback, void *arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_port_registration_callback(client, registration_callback, arg) == 0);
#else
    if (bridge.set_port_registration_callback_ptr != nullptr)
        return (bridge.set_port_registration_callback_ptr(client, registration_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_port_rename_callback(jack_client_t* client, JackPortRenameCallback rename_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_port_rename_callback(client, rename_callback, arg) == 0);
#else
    if (bridge.set_port_rename_callback_ptr != nullptr)
        return (bridge.set_port_rename_callback_ptr(client, rename_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_port_connect_callback(jack_client_t* client, JackPortConnectCallback connect_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_port_connect_callback(client, connect_callback, arg) == 0);
#else
    if (bridge.set_port_connect_callback_ptr != nullptr)
        return (bridge.set_port_connect_callback_ptr(client, connect_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_xrun_callback(jack_client_t* client, JackXRunCallback xrun_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_xrun_callback(client, xrun_callback, arg) == 0);
#else
    if (bridge.set_xrun_callback_ptr != nullptr)
        return (bridge.set_xrun_callback_ptr(client, xrun_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_latency_callback(client, latency_callback, arg) == 0);
#else
    if (bridge.set_latency_callback_ptr != nullptr)
        return (bridge.set_latency_callback_ptr(client, latency_callback, arg) == 0);
#endif
    return false;
}

// -----------------------------------------------------------------------------

bool jackbridge_set_freewheel(jack_client_t* client, bool onoff)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_set_freewheel(client, onoff);
#else
    if (bridge.set_freewheel_ptr != nullptr)
        return bridge.set_freewheel_ptr(client, onoff);
#endif
    return false;
}

bool jackbridge_set_buffer_size(jack_client_t* client, jack_nframes_t nframes)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_set_buffer_size(client, nframes);
#else
    if (bridge.set_buffer_size_ptr != nullptr)
        return bridge.set_buffer_size_ptr(client, nframes);
#endif
    return false;
}

// -----------------------------------------------------------------------------

jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_sample_rate(client);
#else
    if (bridge.get_sample_rate_ptr != nullptr)
        return bridge.get_sample_rate_ptr(client);
#endif
    return 0;
}

jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_buffer_size(client);
#else
    if (bridge.get_buffer_size_ptr != nullptr)
        return bridge.get_buffer_size_ptr(client);
#endif
    return 0;
}

float jackbridge_cpu_load(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_cpu_load(client);
#else
    if (bridge.cpu_load_ptr != nullptr)
        return bridge.cpu_load_ptr(client);
#endif
    return 0.0f;
}

// -----------------------------------------------------------------------------

jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_register(client, port_name, port_type, flags, buffer_size);
#else
    if (bridge.port_register_ptr != nullptr)
        return bridge.port_register_ptr(client, port_name, port_type, flags, buffer_size);
#endif
    return nullptr;
}

bool jackbridge_port_unregister(jack_client_t* client, jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_unregister(client, port) == 0);
#else
    if (bridge.port_unregister_ptr != nullptr)
        return (bridge.port_unregister_ptr(client, port) == 0);
#endif
    return false;
}

void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_get_buffer(port, nframes);
#else
    if (bridge.port_get_buffer_ptr != nullptr)
        return bridge.port_get_buffer_ptr(port, nframes);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

const char* jackbridge_port_name(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_name(port);
#else
    if (bridge.port_name_ptr != nullptr)
        return bridge.port_name_ptr(port);
#endif
    return nullptr;
}

jack_uuid_t jackbridge_port_uuid(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_uuid(port);
#else
    if (bridge.port_uuid_ptr != nullptr)
        return bridge.port_uuid_ptr(port);
#endif
    return 0;
}

const char* jackbridge_port_short_name(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_short_name(port);
#else
    if (bridge.port_short_name_ptr != nullptr)
        return bridge.port_short_name_ptr(port);
#endif
    return nullptr;
}

int jackbridge_port_flags(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_flags(port);
#else
    if (bridge.port_flags_ptr != nullptr)
        return bridge.port_flags_ptr(port);
#endif
    return 0x0;
}

const char* jackbridge_port_type(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_type(port);
#else
    if (bridge.port_type_ptr != nullptr)
        return bridge.port_type_ptr(port);
#endif
    return nullptr;
}

bool jackbridge_port_is_mine(const jack_client_t* client, const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_is_mine(client, port);
#else
    if (bridge.port_is_mine_ptr != nullptr)
        return bridge.port_is_mine_ptr(client, port);
#endif
    return false;
}

int jackbridge_port_connected(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_connected(port);
#else
    if (bridge.port_connected_ptr != nullptr)
        return bridge.port_connected_ptr(port);
#endif
    return 0;
}

bool jackbridge_port_connected_to(const jack_port_t* port, const char* port_name)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_connected_to(port, port_name);
#else
    if (bridge.port_connected_to_ptr != nullptr)
        return bridge.port_connected_to_ptr(port, port_name);
#endif
    return false;
}

const char** jackbridge_port_get_connections(const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_get_connections(port);
#else
    if (bridge.port_get_connections_ptr != nullptr)
        return bridge.port_get_connections_ptr(port);
#endif
    return nullptr;
}

const char** jackbridge_port_get_all_connections(const jack_client_t* client, const jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_get_all_connections(client, port);
#else
    if (bridge.port_get_all_connections_ptr != nullptr)
        return bridge.port_get_all_connections_ptr(client, port);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_port_set_name(jack_port_t* port, const char* port_name)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_set_name(port, port_name) == 0);
#else
    if (bridge.port_set_name_ptr != nullptr)
        return (bridge.port_set_name_ptr(port, port_name) == 0);
#endif
    return false;
}

bool jackbridge_port_set_alias(jack_port_t* port, const char* alias)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_set_alias(port, alias) == 0);
#else
    if (bridge.port_set_alias_ptr != nullptr)
        return (bridge.port_set_alias_ptr(port, alias) == 0);
#endif
    return false;
}

bool jackbridge_port_unset_alias(jack_port_t* port, const char* alias)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_unset_alias(port, alias) == 0);
#else
    if (bridge.port_unset_alias_ptr != nullptr)
        return (bridge.port_unset_alias_ptr(port, alias) == 0);
#endif
    return false;
}

int jackbridge_port_get_aliases(const jack_port_t* port, char* const aliases[2])
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_get_aliases(port, aliases) == 0);
#else
    if (bridge.port_get_aliases_ptr != nullptr)
        return bridge.port_get_aliases_ptr(port, aliases);
#endif
    return 0;
}

// -----------------------------------------------------------------------------

bool jackbridge_port_request_monitor(jack_port_t* port, bool onoff)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_request_monitor(port, onoff) == 0);
#else
    if (bridge.port_request_monitor_ptr != nullptr)
        return (bridge.port_request_monitor_ptr(port, onoff) == 0);
#endif
    return false;
}

bool jackbridge_port_request_monitor_by_name(jack_client_t* client, const char* port_name, bool onoff)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_request_monitor_by_name(client, port_name, onoff) == 0);
#else
    if (bridge.port_request_monitor_by_name_ptr != nullptr)
        return (bridge.port_request_monitor_by_name_ptr(client, port_name, onoff) == 0);
#endif
    return false;
}

bool jackbridge_port_ensure_monitor(jack_port_t* port, bool onoff)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_ensure_monitor(port, onoff) == 0);
#else
    if (bridge.port_ensure_monitor_ptr != nullptr)
        return (bridge.port_ensure_monitor_ptr(port, onoff) == 0);
#endif
    return false;
}

bool jackbridge_port_monitoring_input(jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_monitoring_input(port);
#else
    if (bridge.port_monitoring_input_ptr != nullptr)
        return bridge.port_monitoring_input_ptr(port);
#endif
    return false;
}

// -----------------------------------------------------------------------------

bool jackbridge_connect(jack_client_t* client, const char* source_port, const char* destination_port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_connect(client, source_port, destination_port) == 0);
#else
    if (bridge.connect_ptr != nullptr)
        return (bridge.connect_ptr(client, source_port, destination_port) == 0);
#endif
    return false;
}

bool jackbridge_disconnect(jack_client_t* client, const char* source_port, const char* destination_port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_disconnect(client, source_port, destination_port) == 0);
#else
    if (bridge.disconnect_ptr != nullptr)
        return (bridge.disconnect_ptr(client, source_port, destination_port) == 0);
#endif
    return false;
}

bool jackbridge_port_disconnect(jack_client_t* client, jack_port_t* port)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_port_disconnect(client, port) == 0);
#else
    if (bridge.port_disconnect_ptr != nullptr)
        return (bridge.port_disconnect_ptr(client, port) == 0);
#endif
    return false;
}

// -----------------------------------------------------------------------------

int jackbridge_port_name_size()
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_name_size();
#else
    if (bridge.port_name_size_ptr != nullptr)
        return bridge.port_name_size_ptr();
#endif
    return 0;
}

int jackbridge_port_type_size()
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_type_size();
#else
    if (bridge.port_type_size_ptr != nullptr)
        return bridge.port_type_size_ptr();
#endif
    return 0;
}

size_t jackbridge_port_type_get_buffer_size(jack_client_t* client, const char* port_type)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_type_get_buffer_size(client, port_type);
#else
    if (bridge.port_type_get_buffer_size_ptr != nullptr)
        return bridge.port_type_get_buffer_size_ptr(client, port_type);
#endif
    return 0;
}

// -----------------------------------------------------------------------------

void jackbridge_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_get_latency_range(port, mode, range);
#else
    if (bridge.port_get_latency_range_ptr != nullptr)
        return bridge.port_get_latency_range_ptr(port, mode, range);
#endif
    range->min = 0;
    range->max = 0;
}

void jackbridge_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_port_set_latency_range(port, mode, range);
#else
    if (bridge.port_set_latency_range_ptr != nullptr)
        bridge.port_set_latency_range_ptr(port, mode, range);
#endif
}

bool jackbridge_recompute_total_latencies(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_recompute_total_latencies(client) == 0);
#else
    if (bridge.recompute_total_latencies_ptr != nullptr)
        return (bridge.recompute_total_latencies_ptr(client) == 0);
#endif
    return false;
}

// -----------------------------------------------------------------------------

const char** jackbridge_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_ports(client, port_name_pattern, type_name_pattern, flags);
#else
    if (bridge.get_ports_ptr != nullptr)
        return bridge.get_ports_ptr(client, port_name_pattern, type_name_pattern, flags);
#endif
    return nullptr;
}

jack_port_t* jackbridge_port_by_name(jack_client_t* client, const char* port_name)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_by_name(client, port_name);
#else
    if (bridge.port_by_name_ptr != nullptr)
        return bridge.port_by_name_ptr(client, port_name);
#endif
    return nullptr;
}

jack_port_t* jackbridge_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_port_by_id(client, port_id);
#else
    if (bridge.port_by_id_ptr != nullptr)
        return bridge.port_by_id_ptr(client, port_id);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

void jackbridge_free(void* ptr)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_free(ptr);
#else
    if (bridge.free_ptr != nullptr)
        return bridge.free_ptr(ptr);

    // just in case
    std::free(ptr);
#endif
}

// -----------------------------------------------------------------------------

uint32_t jackbridge_midi_get_event_count(void* port_buffer)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_midi_get_event_count(port_buffer);
#else
    if (bridge.midi_get_event_count_ptr != nullptr)
        return bridge.midi_get_event_count_ptr(port_buffer);
#endif
    return 0;
}

bool jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_midi_event_get(event, port_buffer, event_index) == 0);
#else
    if (bridge.midi_event_get_ptr != nullptr)
        return (bridge.midi_event_get_ptr(event, port_buffer, event_index) == 0);
#endif
    return false;
}

void jackbridge_midi_clear_buffer(void* port_buffer)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_midi_clear_buffer(port_buffer);
#else
    if (bridge.midi_clear_buffer_ptr != nullptr)
        bridge.midi_clear_buffer_ptr(port_buffer);
#endif
}

bool jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, size_t data_size)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_midi_event_write(port_buffer, time, data, data_size) == 0);
#else
    if (bridge.midi_event_write_ptr != nullptr)
        return (bridge.midi_event_write_ptr(port_buffer, time, data, data_size) == 0);
#endif
    return false;
}

jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_midi_event_reserve(port_buffer, time, data_size);
#else
    if (bridge.midi_event_reserve_ptr != nullptr)
        return bridge.midi_event_reserve_ptr(port_buffer, time, data_size);
#endif
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_release_timebase(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_release_timebase(client) == 0);
#else
    if (bridge.release_timebase_ptr != nullptr)
        return (bridge.release_timebase_ptr(client) == 0);
#endif
    return false;
}

bool jackbridge_set_sync_callback(jack_client_t* client, JackSyncCallback sync_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_sync_callback(client, sync_callback, arg) == 0);
#else
    if (bridge.set_sync_callback_ptr != nullptr)
        return (bridge.set_sync_callback_ptr(client, sync_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_set_sync_timeout(jack_client_t* client, jack_time_t timeout)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_sync_timeout(client, timeout) == 0);
#else
    if (bridge.set_sync_timeout_ptr != nullptr)
        return (bridge.set_sync_timeout_ptr(client, timeout) == 0);
#endif
    return false;
}

bool jackbridge_set_timebase_callback(jack_client_t* client, bool conditional, JackTimebaseCallback timebase_callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_timebase_callback(client, conditional, timebase_callback, arg) == 0);
#else
    if (bridge.set_timebase_callback_ptr != nullptr)
        return (bridge.set_timebase_callback_ptr(client, conditional, timebase_callback, arg) == 0);
#endif
    return false;
}

bool jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_transport_locate(client, frame) == 0);
#else
    if (bridge.transport_locate_ptr != nullptr)
        return (bridge.transport_locate_ptr(client, frame) == 0);
#endif
    return false;
}

jack_transport_state_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_transport_query(client, pos);
#else
    if (bridge.transport_query_ptr != nullptr)
        return bridge.transport_query_ptr(client, pos);
#endif
    if (pos != nullptr)
    {
        // invalidate
        pos->unique_1 = 0;
        pos->unique_2 = 1;
    }
    return JackTransportStopped;
}

jack_nframes_t jackbridge_get_current_transport_frame(const jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_get_current_transport_frame(client);
#else
    if (bridge.get_current_transport_frame_ptr != nullptr)
        return bridge.get_current_transport_frame_ptr(client);
#endif
    return 0;
}

bool jackbridge_transport_reposition(jack_client_t* client, const jack_position_t* pos)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_transport_reposition(client, pos) == 0);
#else
    if (bridge.transport_reposition_ptr != nullptr)
        return (bridge.transport_reposition_ptr(client, pos) == 0);
#endif
    return false;
}

void jackbridge_transport_start(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_transport_start(client);
#else
    if (bridge.transport_start_ptr != nullptr)
        bridge.transport_start_ptr(client);
#endif
}

void jackbridge_transport_stop(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_transport_stop(client);
#else
    if (bridge.transport_stop_ptr != nullptr)
        bridge.transport_stop_ptr(client);
#endif
}

// -----------------------------------------------------------------------------

bool jackbridge_set_property(jack_client_t* client, jack_uuid_t subject, const char* key, const char* value, const char* type)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_property(client, subject, key, value, type) == 0);
#else
    if (bridge.set_property_ptr != nullptr)
        return (bridge.set_property_ptr(client, subject, key, value, type) == 0);
#endif
    return false;
}

bool jackbridge_get_property(jack_uuid_t subject, const char* key, char** value, char** type)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_get_property(subject, key, value, type) == 0);
#else
    if (bridge.get_property_ptr != nullptr)
        return (bridge.get_property_ptr(subject, key, value, type) == 0);
#endif
    return false;
}

void jackbridge_free_description(jack_description_t* desc, bool free_description_itself)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    jack_free_description(desc, free_description_itself);
#else
    if (bridge.free_description_ptr != nullptr)
        bridge.free_description_ptr(desc, free_description_itself);
#endif
}

bool jackbridge_get_properties(jack_uuid_t subject, jack_description_t* desc)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_get_properties(subject, desc) == 0);
#else
    if (bridge.get_properties_ptr != nullptr)
        return (bridge.get_properties_ptr(subject, desc) == 0);
#endif
    return false;
}

bool jackbridge_get_all_properties(jack_description_t** descs)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_get_all_properties(descs) == 0);
#else
    if (bridge.get_all_properties_ptr != nullptr)
        return (bridge.get_all_properties_ptr(descs) == 0);
#endif
    return false;
}

bool jackbridge_remove_property(jack_client_t* client, jack_uuid_t subject, const char* key)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_remove_property(client, subject, key) == 0);
#else
    if (bridge.remove_property_ptr != nullptr)
        return (bridge.remove_property_ptr(client, subject, key) == 0);
#endif
    return false;
}

int jackbridge_remove_properties(jack_client_t* client, jack_uuid_t subject)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return jack_remove_properties(client, subject);
#else
    if (bridge.remove_properties_ptr != nullptr)
        return bridge.remove_properties_ptr(client, subject);
#endif
    return 0;
}

bool jackbridge_remove_all_properties(jack_client_t* client)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_remove_all_properties(client) == 0);
#else
    if (bridge.remove_all_properties_ptr != nullptr)
        return (bridge.remove_all_properties_ptr(client) == 0);
#endif
    return false;
}

bool jackbridge_set_property_change_callback(jack_client_t* client, JackPropertyChangeCallback callback, void* arg)
{
#if defined(JACKBRIDGE_DUMMY)
#elif defined(JACKBRIDGE_DIRECT)
    return (jack_set_property_change_callback(client, callback, arg) == 0);
#else
    if (bridge.set_property_change_callback_ptr != nullptr)
        return (bridge.set_property_change_callback_ptr(client, callback, arg) == 0);
#endif
    return false;
}

// -----------------------------------------------------------------------------
