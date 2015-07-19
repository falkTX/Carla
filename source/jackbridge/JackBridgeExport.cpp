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

#include "CarlaLibUtils.hpp"

// -----------------------------------------------------------------------------

class JackBridgeExported
{
public:
    JackBridgeExported() noexcept
        : lib(nullptr),
          func(nullptr)
    {
#ifdef CARLA_OS_WIN64
        lib = lib_open("jackbridge-wine64.dll");
#else
        lib = lib_open("jackbridge-wine32.dll");
#endif
        CARLA_SAFE_ASSERT_RETURN(lib != nullptr,);

        func = lib_symbol<jackbridge_exported_function_type>(lib, "jackbridge_get_exported_functions");
        CARLA_SAFE_ASSERT_RETURN(func != nullptr,);
    }

    ~JackBridgeExported() noexcept
    {
        if (lib == nullptr)
            return;

        lib_close(lib);
        lib  = nullptr;
        func = nullptr;
    }

    static const JackBridgeExportedFunctions& getFunctions() noexcept
    {
        static JackBridgeExportedFunctions fallback;
        carla_zeroStruct(fallback);
        fallback.unique1 = 1;
        fallback.unique2 = 2;
        fallback.unique3 = 3;

        static const JackBridgeExported bridge;
        CARLA_SAFE_ASSERT_RETURN(bridge.func != nullptr, fallback);

        const JackBridgeExportedFunctions* const funcs(bridge.func());
        CARLA_SAFE_ASSERT_RETURN(funcs != nullptr, fallback);
        CARLA_SAFE_ASSERT_RETURN(funcs->unique1 != 0, fallback);
        CARLA_SAFE_ASSERT_RETURN(funcs->unique1 == funcs->unique2, fallback);
        CARLA_SAFE_ASSERT_RETURN(funcs->unique2 == funcs->unique3, fallback);
        CARLA_SAFE_ASSERT_RETURN(funcs->shm_map_ptr != nullptr, fallback);

        return *funcs;
    }

private:
    lib_t lib;
    jackbridge_exported_function_type func;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(JackBridgeExported);
};

// -----------------------------------------------------------------------------

static const JackBridgeExportedFunctions& getBridgeInstance() noexcept
{
    static const JackBridgeExportedFunctions& funcs(JackBridgeExported::getFunctions());
    return funcs;
}

// -----------------------------------------------------------------------------

bool jackbridge_is_ok() noexcept
{
    const JackBridgeExportedFunctions& instance(getBridgeInstance());
    return (instance.unique1 != 0 && instance.unique1 == instance.unique2 && instance.init_ptr != nullptr);
}

void jackbridge_init()
{
    return getBridgeInstance().init_ptr();
}

void jackbridge_get_version(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr)
{
    return getBridgeInstance().get_version_ptr(major_ptr, minor_ptr, micro_ptr, proto_ptr);
}

const char* jackbridge_get_version_string()
{
    return getBridgeInstance().get_version_string_ptr();
}

jack_client_t* jackbridge_client_open(const char* client_name, uint32_t options, jack_status_t* status)
{
    return getBridgeInstance().client_open_ptr(client_name, options, status);
}

bool jackbridge_client_close(jack_client_t* client)
{
    return getBridgeInstance().client_close_ptr(client);
}

int jackbridge_client_name_size()
{
    return getBridgeInstance().client_name_size_ptr();
}

char* jackbridge_get_client_name(jack_client_t* client)
{
    return getBridgeInstance().get_client_name_ptr(client);
}

char* jackbridge_get_uuid_for_client_name(jack_client_t* client, const char* name)
{
    return getBridgeInstance().get_uuid_for_client_name_ptr(client, name);
}

char* jackbridge_get_client_name_by_uuid(jack_client_t* client, const char* uuid)
{
    return getBridgeInstance().get_client_name_by_uuid_ptr(client, uuid);
}

bool jackbridge_activate(jack_client_t* client)
{
    return getBridgeInstance().activate_ptr(client);
}

bool jackbridge_deactivate(jack_client_t* client)
{
    return getBridgeInstance().deactivate_ptr(client);
}

bool jackbridge_is_realtime(jack_client_t* client)
{
    return getBridgeInstance().is_realtime_ptr(client);
}

bool jackbridge_set_thread_init_callback(jack_client_t* client, JackThreadInitCallback thread_init_callback, void* arg)
{
    return getBridgeInstance().set_thread_init_callback_ptr(client, thread_init_callback, arg);
}

void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg)
{
    return getBridgeInstance().on_shutdown_ptr(client, shutdown_callback, arg);
}

void jackbridge_on_info_shutdown(jack_client_t* client, JackInfoShutdownCallback shutdown_callback, void* arg)
{
    return getBridgeInstance().on_info_shutdown_ptr(client, shutdown_callback, arg);
}

bool jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
    return getBridgeInstance().set_process_callback_ptr(client, process_callback, arg);
}

bool jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg)
{
    return getBridgeInstance().set_freewheel_callback_ptr(client, freewheel_callback, arg);
}

bool jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg)
{
    return getBridgeInstance().set_buffer_size_callback_ptr(client, bufsize_callback, arg);
}

bool jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg)
{
    return getBridgeInstance().set_sample_rate_callback_ptr(client, srate_callback, arg);
}

bool jackbridge_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg)
{
    return getBridgeInstance().set_client_registration_callback_ptr(client, registration_callback, arg);
}

bool jackbridge_set_port_registration_callback(jack_client_t* client, JackPortRegistrationCallback registration_callback, void *arg)
{
    return getBridgeInstance().set_port_registration_callback_ptr(client, registration_callback, arg);
}

bool jackbridge_set_port_rename_callback(jack_client_t* client, JackPortRenameCallback rename_callback, void* arg)
{
    return getBridgeInstance().set_port_rename_callback_ptr(client, rename_callback, arg);
}

bool jackbridge_set_port_connect_callback(jack_client_t* client, JackPortConnectCallback connect_callback, void* arg)
{
    return getBridgeInstance().set_port_connect_callback_ptr(client, connect_callback, arg);
}

bool jackbridge_set_xrun_callback(jack_client_t* client, JackXRunCallback xrun_callback, void* arg)
{
    return getBridgeInstance().set_xrun_callback_ptr(client, xrun_callback, arg);
}

bool jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg)
{
    return getBridgeInstance().set_latency_callback_ptr(client, latency_callback, arg);
}

bool jackbridge_set_freewheel(jack_client_t* client, bool onoff)
{
    return getBridgeInstance().set_freewheel_ptr(client, onoff);
}

bool jackbridge_set_buffer_size(jack_client_t* client, jack_nframes_t nframes)
{
    return getBridgeInstance().set_buffer_size_ptr(client, nframes);
}

jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client)
{
    return getBridgeInstance().get_sample_rate_ptr(client);
}

jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client)
{
    return getBridgeInstance().get_buffer_size_ptr(client);
}

float jackbridge_cpu_load(jack_client_t* client)
{
    return getBridgeInstance().cpu_load_ptr(client);
}

jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, uint64_t flags, uint64_t buffer_size)
{
    return getBridgeInstance().port_register_ptr(client, port_name, port_type, flags, buffer_size);
}

bool jackbridge_port_unregister(jack_client_t* client, jack_port_t* port)
{
    return getBridgeInstance().port_unregister_ptr(client, port);
}

void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes)
{
    return getBridgeInstance().port_get_buffer_ptr(port, nframes);
}

const char* jackbridge_port_name(const jack_port_t* port)
{
    return getBridgeInstance().port_name_ptr(port);
}

jack_uuid_t jackbridge_port_uuid(const jack_port_t* port)
{
    return getBridgeInstance().port_uuid_ptr(port);
}

const char* jackbridge_port_short_name(const jack_port_t* port)
{
    return getBridgeInstance().port_short_name_ptr(port);
}

int jackbridge_port_flags(const jack_port_t* port)
{
    return getBridgeInstance().port_flags_ptr(port);
}

const char* jackbridge_port_type(const jack_port_t* port)
{
    return getBridgeInstance().port_type_ptr(port);
}

bool jackbridge_port_is_mine(const jack_client_t* client, const jack_port_t* port)
{
    return getBridgeInstance().port_is_mine_ptr(client, port);
}

int jackbridge_port_connected(const jack_port_t* port)
{
    return getBridgeInstance().port_connected_ptr(port);
}

bool jackbridge_port_connected_to(const jack_port_t* port, const char* port_name)
{
    return getBridgeInstance().port_connected_to_ptr(port, port_name);
}

const char** jackbridge_port_get_connections(const jack_port_t* port)
{
    return getBridgeInstance().port_get_connections_ptr(port);
}

const char** jackbridge_port_get_all_connections(const jack_client_t* client, const jack_port_t* port)
{
    return getBridgeInstance().port_get_all_connections_ptr(client, port);
}

bool jackbridge_port_rename(jack_client_t* client, jack_port_t* port, const char* port_name)
{
    return getBridgeInstance().port_rename_ptr(client, port, port_name);
}

bool jackbridge_port_set_alias(jack_port_t* port, const char* alias)
{
    return getBridgeInstance().port_set_alias_ptr(port, alias);
}

bool jackbridge_port_unset_alias(jack_port_t* port, const char* alias)
{
    return getBridgeInstance().port_unset_alias_ptr(port, alias);
}

int jackbridge_port_get_aliases(const jack_port_t* port, char* const aliases[2])
{
    return getBridgeInstance().port_get_aliases_ptr(port, aliases);
}

bool jackbridge_port_request_monitor(jack_port_t* port, bool onoff)
{
    return getBridgeInstance().port_request_monitor_ptr(port, onoff);
}

bool jackbridge_port_request_monitor_by_name(jack_client_t* client, const char* port_name, bool onoff)
{
    return getBridgeInstance().port_request_monitor_by_name_ptr(client, port_name, onoff);
}

bool jackbridge_port_ensure_monitor(jack_port_t* port, bool onoff)
{
    return getBridgeInstance().port_ensure_monitor_ptr(port, onoff);
}

bool jackbridge_port_monitoring_input(jack_port_t* port)
{
    return getBridgeInstance().port_monitoring_input_ptr(port);
}

bool jackbridge_connect(jack_client_t* client, const char* source_port, const char* destination_port)
{
    return getBridgeInstance().connect_ptr(client, source_port, destination_port);
}

bool jackbridge_disconnect(jack_client_t* client, const char* source_port, const char* destination_port)
{
    return getBridgeInstance().disconnect_ptr(client, source_port, destination_port);
}

bool jackbridge_port_disconnect(jack_client_t* client, jack_port_t* port)
{
    return getBridgeInstance().port_disconnect_ptr(client, port);
}

int jackbridge_port_name_size()
{
    return getBridgeInstance().port_name_size_ptr();
}

int jackbridge_port_type_size()
{
    return getBridgeInstance().port_type_size_ptr();
}

uint32_t jackbridge_port_type_get_buffer_size(jack_client_t* client, const char* port_type)
{
    return getBridgeInstance().port_type_get_buffer_size_ptr(client, port_type);
}

void jackbridge_port_get_latency_range(jack_port_t* port, uint32_t mode, jack_latency_range_t* range)
{
    return getBridgeInstance().port_get_latency_range_ptr(port, mode, range);
}

void jackbridge_port_set_latency_range(jack_port_t* port, uint32_t mode, jack_latency_range_t* range)
{
    return getBridgeInstance().port_set_latency_range_ptr(port, mode, range);
}

bool jackbridge_recompute_total_latencies(jack_client_t* client)
{
    return getBridgeInstance().recompute_total_latencies_ptr(client);
}

const char** jackbridge_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, uint64_t flags)
{
    return getBridgeInstance().get_ports_ptr(client, port_name_pattern, type_name_pattern, flags);
}

jack_port_t* jackbridge_port_by_name(jack_client_t* client, const char* port_name)
{
    return getBridgeInstance().port_by_name_ptr(client, port_name);
}

jack_port_t* jackbridge_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    return getBridgeInstance().port_by_id_ptr(client, port_id);
}

void jackbridge_free(void* ptr)
{
    return getBridgeInstance().free_ptr(ptr);
}

uint32_t jackbridge_midi_get_event_count(void* port_buffer)
{
    return getBridgeInstance().midi_get_event_count_ptr(port_buffer);
}

bool jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index)
{
    return getBridgeInstance().midi_event_get_ptr(event, port_buffer, event_index);
}

void jackbridge_midi_clear_buffer(void* port_buffer)
{
    return getBridgeInstance().midi_clear_buffer_ptr(port_buffer);
}

bool jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, uint32_t data_size)
{
    return getBridgeInstance().midi_event_write_ptr(port_buffer, time, data, data_size);
}

jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, uint32_t data_size)
{
    return getBridgeInstance().midi_event_reserve_ptr(port_buffer, time, data_size);
}

bool jackbridge_release_timebase(jack_client_t* client)
{
    return getBridgeInstance().release_timebase_ptr(client);
}

bool jackbridge_set_sync_callback(jack_client_t* client, JackSyncCallback sync_callback, void* arg)
{
    return getBridgeInstance().set_sync_callback_ptr(client, sync_callback, arg);
}

bool jackbridge_set_sync_timeout(jack_client_t* client, jack_time_t timeout)
{
    return getBridgeInstance().set_sync_timeout_ptr(client, timeout);
}

bool jackbridge_set_timebase_callback(jack_client_t* client, bool conditional, JackTimebaseCallback timebase_callback, void* arg)
{
    return getBridgeInstance().set_timebase_callback_ptr(client, conditional, timebase_callback, arg);
}

bool jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame)
{
    return getBridgeInstance().transport_locate_ptr(client, frame);
}

uint32_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos)
{
    return getBridgeInstance().transport_query_ptr(client, pos);
}

jack_nframes_t jackbridge_get_current_transport_frame(const jack_client_t* client)
{
    return getBridgeInstance().get_current_transport_frame_ptr(client);
}

bool jackbridge_transport_reposition(jack_client_t* client, const jack_position_t* pos)
{
    return getBridgeInstance().transport_reposition_ptr(client, pos);
}

void jackbridge_transport_start(jack_client_t* client)
{
    return getBridgeInstance().transport_start_ptr(client);
}

void jackbridge_transport_stop(jack_client_t* client)
{
    return getBridgeInstance().transport_stop_ptr(client);
}

bool jackbridge_set_property(jack_client_t* client, jack_uuid_t subject, const char* key, const char* value, const char* type)
{
    return getBridgeInstance().set_property_ptr(client, subject, key, value, type);
}

bool jackbridge_get_property(jack_uuid_t subject, const char* key, char** value, char** type)
{
    return getBridgeInstance().get_property_ptr(subject, key, value, type);
}

void jackbridge_free_description(jack_description_t* desc, bool free_description_itself)
{
    return getBridgeInstance().free_description_ptr(desc, free_description_itself);
}

bool jackbridge_get_properties(jack_uuid_t subject, jack_description_t* desc)
{
    return getBridgeInstance().get_properties_ptr(subject, desc);
}

bool jackbridge_get_all_properties(jack_description_t** descs)
{
    return getBridgeInstance().get_all_properties_ptr(descs);
}

bool jackbridge_remove_property(jack_client_t* client, jack_uuid_t subject, const char* key)
{
    return getBridgeInstance().remove_property_ptr(client, subject, key);
}

int jackbridge_remove_properties(jack_client_t* client, jack_uuid_t subject)
{
    return getBridgeInstance().remove_properties_ptr(client, subject);
}

bool jackbridge_remove_all_properties(jack_client_t* client)
{
    return getBridgeInstance().remove_all_properties_ptr(client);
}

bool jackbridge_set_property_change_callback(jack_client_t* client, JackPropertyChangeCallback callback, void* arg)
{
    return getBridgeInstance().set_property_change_callback_ptr(client, callback, arg);
}

bool jackbridge_sem_init(void* sem) noexcept
{
    return getBridgeInstance().sem_init_ptr(sem);
}

void jackbridge_sem_destroy(void* sem) noexcept
{
    getBridgeInstance().sem_destroy_ptr(sem);
}

void jackbridge_sem_post(void* sem) noexcept
{
    getBridgeInstance().sem_post_ptr(sem);
}

bool jackbridge_sem_timedwait(void* sem, uint secs) noexcept
{
    return getBridgeInstance().sem_timedwait_ptr(sem, secs);
}

bool jackbridge_shm_is_valid(const void* shm) noexcept
{
    return getBridgeInstance().shm_is_valid_ptr(shm);
}

void jackbridge_shm_init(void* shm) noexcept
{
    return getBridgeInstance().shm_init_ptr(shm);
}

void jackbridge_shm_attach(void* shm, const char* name) noexcept
{
    return getBridgeInstance().shm_attach_ptr(shm, name);
}

void jackbridge_shm_close(void* shm) noexcept
{
    return getBridgeInstance().shm_close_ptr(shm);
}

void* jackbridge_shm_map(void* shm, uint64_t size) noexcept
{
    return getBridgeInstance().shm_map_ptr(shm, size);
}

// -----------------------------------------------------------------------------
