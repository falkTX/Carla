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

#include "JackBridge.hpp"

#ifdef __WINE__
# define __CARLA_UTILS_HPP__
# undef CARLA_ASSERT
# define CARLA_ASSERT(...)
#endif

#include "CarlaLibUtils.hpp"

#include <stdio.h>

#ifndef JACKBRIDGE_DUMMY
# include <time.h>
# include <semaphore.h>
#endif

// -----------------------------------------------------------------------------

typedef const char*    (*jacksym_get_version_string)();
typedef jack_client_t* (*jacksym_client_open)(const char*, jack_options_t, jack_status_t*, ...);

typedef int   (*jacksym_client_close)(jack_client_t*);
typedef int   (*jacksym_client_name_size)();
typedef char* (*jacksym_get_client_name)(jack_client_t*);

typedef int  (*jacksym_activate)(jack_client_t*);
typedef int  (*jacksym_deactivate)(jack_client_t*);
typedef void (*jacksym_on_shutdown)(jack_client_t*, JackShutdownCallback, void*);
typedef int  (*jacksym_set_process_callback)(jack_client_t*, JackProcessCallback, void*);
typedef int  (*jacksym_set_freewheel_callback)(jack_client_t*, JackFreewheelCallback, void*);
typedef int  (*jacksym_set_buffer_size_callback)(jack_client_t*, JackBufferSizeCallback, void*);
typedef int  (*jacksym_set_sample_rate_callback)(jack_client_t*, JackSampleRateCallback, void*);
typedef int  (*jacksym_set_client_registration_callback)(jack_client_t*, JackClientRegistrationCallback, void*);
typedef int  (*jacksym_set_port_registration_callback)(jack_client_t*, JackPortRegistrationCallback, void*);
typedef int  (*jacksym_set_port_connect_callback)(jack_client_t*, JackPortConnectCallback, void*);
typedef int  (*jacksym_set_port_rename_callback)(jack_client_t*, JackPortRenameCallback, void*);
typedef int  (*jacksym_set_latency_callback)(jack_client_t*, JackLatencyCallback, void*);

typedef jack_nframes_t (*jacksym_get_sample_rate)(jack_client_t*);
typedef jack_nframes_t (*jacksym_get_buffer_size)(jack_client_t*);
typedef jack_port_t*   (*jacksym_port_register)(jack_client_t*, const char*, const char*, unsigned long, unsigned long);

typedef int   (*jacksym_port_unregister)(jack_client_t*, jack_port_t*);
typedef void* (*jacksym_port_get_buffer)(jack_port_t*, jack_nframes_t);

typedef const char*  (*jacksym_port_name)(const jack_port_t*);
typedef const char*  (*jacksym_port_short_name)(const jack_port_t*);
typedef int          (*jacksym_port_flags)(const jack_port_t*);
typedef const char*  (*jacksym_port_type)(const jack_port_t*);
typedef const char** (*jacksym_port_get_connections)(const jack_port_t*);

typedef int  (*jacksym_port_set_name)(jack_port_t*, const char*);
typedef int  (*jacksym_connect)(jack_client_t*, const char*, const char*);
typedef int  (*jacksym_disconnect)(jack_client_t*, const char*, const char*);
typedef int  (*jacksym_port_name_size)();
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

typedef int  (*jacksym_transport_locate)(jack_client_t*, jack_nframes_t);
typedef void (*jacksym_transport_start)(jack_client_t*);
typedef void (*jacksym_transport_stop)(jack_client_t*);
typedef jack_transport_state_t (*jacksym_transport_query)(const jack_client_t*, jack_position_t*);

// -----------------------------------------------------------------------------

struct JackBridge {
    void* lib;

    jacksym_get_version_string get_version_string_ptr;
    jacksym_client_open client_open_ptr;
    jacksym_client_close client_close_ptr;
    jacksym_client_name_size client_name_size_ptr;
    jacksym_get_client_name get_client_name_ptr;
    jacksym_activate activate_ptr;
    jacksym_deactivate deactivate_ptr;
    jacksym_on_shutdown on_shutdown_ptr;
    jacksym_set_process_callback set_process_callback_ptr;
    jacksym_set_freewheel_callback set_freewheel_callback_ptr;
    jacksym_set_buffer_size_callback set_buffer_size_callback_ptr;
    jacksym_set_sample_rate_callback set_sample_rate_callback_ptr;
    jacksym_set_client_registration_callback set_client_registration_callback_ptr;
    jacksym_set_port_registration_callback set_port_registration_callback_ptr;
    jacksym_set_port_connect_callback set_port_connect_callback_ptr;
    jacksym_set_port_rename_callback set_port_rename_callback_ptr;
    jacksym_set_latency_callback set_latency_callback_ptr;
    jacksym_get_sample_rate get_sample_rate_ptr;
    jacksym_get_buffer_size get_buffer_size_ptr;
    jacksym_port_register port_register_ptr;
    jacksym_port_unregister port_unregister_ptr;
    jacksym_port_get_buffer port_get_buffer_ptr;
    jacksym_port_name port_name_ptr;
    jacksym_port_short_name port_short_name_ptr;
    jacksym_port_flags port_flags_ptr;
    jacksym_port_type port_type_ptr;
    jacksym_port_get_connections port_get_connections_ptr;
    jacksym_port_set_name port_set_name_ptr;
    jacksym_connect connect_ptr;
    jacksym_disconnect disconnect_ptr;
    jacksym_port_name_size port_name_size_ptr;
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

    jacksym_transport_locate transport_locate_ptr;
    jacksym_transport_start transport_start_ptr;
    jacksym_transport_stop transport_stop_ptr;
    jacksym_transport_query transport_query_ptr;

    JackBridge()
        : lib(nullptr),
          get_version_string_ptr(nullptr),
          client_open_ptr(nullptr),
          client_close_ptr(nullptr),
          client_name_size_ptr(nullptr),
          get_client_name_ptr(nullptr),
          activate_ptr(nullptr),
          deactivate_ptr(nullptr),
          on_shutdown_ptr(nullptr),
          set_process_callback_ptr(nullptr),
          set_freewheel_callback_ptr(nullptr),
          set_buffer_size_callback_ptr(nullptr),
          set_sample_rate_callback_ptr(nullptr),
          set_client_registration_callback_ptr(nullptr),
          set_port_registration_callback_ptr(nullptr),
          set_port_connect_callback_ptr(nullptr),
          set_port_rename_callback_ptr(nullptr),
          set_latency_callback_ptr(nullptr),
          get_sample_rate_ptr(nullptr),
          get_buffer_size_ptr(nullptr),
          port_register_ptr(nullptr),
          port_unregister_ptr(nullptr),
          port_get_buffer_ptr(nullptr),
          port_name_ptr(nullptr),
          port_short_name_ptr(nullptr),
          port_flags_ptr(nullptr),
          port_type_ptr(nullptr),
          port_get_connections_ptr(nullptr),
          port_set_name_ptr(nullptr),
          connect_ptr(nullptr),
          disconnect_ptr(nullptr),
          port_name_size_ptr(nullptr),
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
          transport_locate_ptr(nullptr),
          transport_start_ptr(nullptr),
          transport_stop_ptr(nullptr),
          transport_query_ptr(nullptr)
    {
#if defined(CARLA_OS_MAC)
        lib = lib_open("libjack.dylib");
        fprintf(stderr, "load JACK DLL FOR MAC\n");
#elif defined(CARLA_OS_WIN) && ! defined(__WINE__)
        lib = lib_open("libjack.dll");
        fprintf(stderr, "load JACK DLL FOR WINDOWS\n");
#else
        lib = lib_open("libjack.so");
        fprintf(stderr, "load JACK DLL FOR LINUX\n");
#endif

        if (lib == nullptr)
        {
            fprintf(stderr, "Failed to load JACK DLL\n");
            return;
        }

        #define JOIN(a, b) a ## b
        #define LIB_SYMBOL(NAME) JOIN(NAME, _ptr) = (jacksym_##NAME)lib_symbol(lib, "jack_" #NAME);

        LIB_SYMBOL(get_version_string)
        LIB_SYMBOL(client_open)
        LIB_SYMBOL(client_close)
        LIB_SYMBOL(client_name_size)
        LIB_SYMBOL(get_client_name)
        LIB_SYMBOL(activate)
        LIB_SYMBOL(deactivate)
        LIB_SYMBOL(on_shutdown)
        LIB_SYMBOL(set_process_callback)
        LIB_SYMBOL(set_freewheel_callback)
        LIB_SYMBOL(set_buffer_size_callback)
        LIB_SYMBOL(set_sample_rate_callback)
        LIB_SYMBOL(set_client_registration_callback)
        LIB_SYMBOL(set_port_registration_callback)
        LIB_SYMBOL(set_port_connect_callback)
        LIB_SYMBOL(set_port_rename_callback)
        LIB_SYMBOL(set_latency_callback)
        LIB_SYMBOL(get_sample_rate)
        LIB_SYMBOL(get_buffer_size)
        LIB_SYMBOL(port_register)
        LIB_SYMBOL(port_unregister)
        LIB_SYMBOL(port_get_buffer)
        LIB_SYMBOL(port_name)
        LIB_SYMBOL(port_short_name)
        LIB_SYMBOL(port_flags)
        LIB_SYMBOL(port_type)
        LIB_SYMBOL(port_get_connections)
        LIB_SYMBOL(port_set_name)
        LIB_SYMBOL(connect)
        LIB_SYMBOL(disconnect)
        LIB_SYMBOL(port_name_size)
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

        LIB_SYMBOL(transport_locate)
        LIB_SYMBOL(transport_start)
        LIB_SYMBOL(transport_stop)
        LIB_SYMBOL(transport_query)

        #undef JOIN
        #undef LIB_SYMBOL
    }

    ~JackBridge()
    {
        if (lib != nullptr)
            lib_close(lib);
    }
};

static JackBridge bridge;

// -----------------------------------------------------------------------------

const char* jackbridge_get_version_string()
{
    if (bridge.get_version_string_ptr != nullptr)
        return bridge.get_version_string_ptr();
    return nullptr;
}

jack_client_t* jackbridge_client_open(const char* client_name, jack_options_t options, jack_status_t* status, ...)
{
    if (bridge.client_open_ptr != nullptr)
        return bridge.client_open_ptr(client_name, options, status);
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_client_close(jack_client_t* client)
{
    if (bridge.client_close_ptr != nullptr)
        return (bridge.client_close_ptr(client) == 0);
    return false;
}

int jackbridge_client_name_size()
{
    if (bridge.client_name_size_ptr != nullptr)
        return bridge.client_name_size_ptr();
    return 0;
}

char* jackbridge_get_client_name(jack_client_t* client)
{
    if (bridge.get_client_name_ptr != nullptr)
        return bridge.get_client_name_ptr(client);
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_activate(jack_client_t* client)
{
    if (bridge.activate_ptr != nullptr)
        return (bridge.activate_ptr(client) == 0);
    return false;
}

bool jackbridge_deactivate(jack_client_t* client)
{
    if (bridge.deactivate_ptr != nullptr)
        return (bridge.deactivate_ptr(client) == 0);
    return false;
}

void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg)
{
    if (bridge.on_shutdown_ptr != nullptr)
        bridge.on_shutdown_ptr(client, shutdown_callback, arg);
}

bool jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg)
{
    if (bridge.set_process_callback_ptr != nullptr)
        return (bridge.set_process_callback_ptr(client, process_callback, arg) == 0);
    return false;
}

bool jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg)
{
    if (bridge.set_freewheel_callback_ptr != nullptr)
        return (bridge.set_freewheel_callback_ptr(client, freewheel_callback, arg) == 0);
    return false;
}

bool jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg)
{
    if (bridge.set_buffer_size_callback_ptr != nullptr)
        return (bridge.set_buffer_size_callback_ptr(client, bufsize_callback, arg) == 0);
    return false;
}

bool jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg)
{
    if (bridge.set_sample_rate_callback_ptr != nullptr)
        return (bridge.set_sample_rate_callback_ptr(client, srate_callback, arg) == 0);
    return false;
}

bool jackbridge_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg)
{
    if (bridge.set_client_registration_callback_ptr != nullptr)
        return (bridge.set_client_registration_callback_ptr(client, registration_callback, arg) == 0);
    return false;
}

bool jackbridge_set_port_registration_callback (jack_client_t* client, JackPortRegistrationCallback registration_callback, void *arg)
{
    if (bridge.set_port_registration_callback_ptr != nullptr)
        return (bridge.set_port_registration_callback_ptr(client, registration_callback, arg) == 0);
    return false;
}

bool jackbridge_set_port_connect_callback (jack_client_t* client, JackPortConnectCallback connect_callback, void* arg)
{
    if (bridge.set_port_connect_callback_ptr != nullptr)
        return (bridge.set_port_connect_callback_ptr(client, connect_callback, arg) == 0);
    return false;
}

bool jackbridge_set_port_rename_callback (jack_client_t* client, JackPortRenameCallback rename_callback, void* arg)
{
    if (bridge.set_port_rename_callback_ptr != nullptr)
        return (bridge.set_port_rename_callback_ptr(client, rename_callback, arg) == 0);
    return false;
}

bool jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg)
{
    if (bridge.set_latency_callback_ptr != nullptr)
        return (bridge.set_latency_callback_ptr(client, latency_callback, arg) == 0);
    return false;
}

// -----------------------------------------------------------------------------

jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client)
{
    if (bridge.get_sample_rate_ptr != nullptr)
        return bridge.get_sample_rate_ptr(client);
    return 0;
}

jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client)
{
    if (bridge.get_buffer_size_ptr != nullptr)
        return bridge.get_buffer_size_ptr(client);
    return 0;
}

jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, unsigned long flags, unsigned long buffer_size)
{
    if (bridge.port_register_ptr != nullptr)
        return bridge.port_register_ptr(client, port_name, port_type, flags, buffer_size);
    return nullptr;
}

bool jackbridge_port_unregister(jack_client_t* client, jack_port_t* port)
{
    if (bridge.port_unregister_ptr != nullptr)
        return (bridge.port_unregister_ptr(client, port) == 0);
    return false;
}

void* jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes)
{
    if (bridge.port_get_buffer_ptr != nullptr)
        return bridge.port_get_buffer_ptr(port, nframes);
    return nullptr;
}

// -----------------------------------------------------------------------------

const char* jackbridge_port_name(const jack_port_t* port)
{
    if (bridge.port_name_ptr != nullptr)
        return bridge.port_name_ptr(port);
    return nullptr;
}

const char* jackbridge_port_short_name(const jack_port_t* port)
{
    if (bridge.port_short_name_ptr != nullptr)
        return bridge.port_short_name_ptr(port);
    return nullptr;
}

int jackbridge_port_flags(const jack_port_t* port)
{
    if (bridge.port_flags_ptr != nullptr)
        return bridge.port_flags_ptr(port);
    return 0;
}

const char* jackbridge_port_type(const jack_port_t* port)
{
    if (bridge.port_type_ptr != nullptr)
        return bridge.port_type_ptr(port);
    return nullptr;
}

const char** jackbridge_port_get_connections(const jack_port_t* port)
{
    if (bridge.port_get_connections_ptr != nullptr)
        return bridge.port_get_connections_ptr(port);
    return nullptr;
}

// -----------------------------------------------------------------------------

bool jackbridge_port_set_name(jack_port_t* port, const char* port_name)
{
    if (bridge.port_set_name_ptr != nullptr)
        return (bridge.port_set_name_ptr(port, port_name) == 0);
    return false;
}

bool jackbridge_connect(jack_client_t* client, const char* source_port, const char* destination_port)
{
    if (bridge.connect_ptr != nullptr)
        return (bridge.connect_ptr(client, source_port, destination_port) == 0);
    return false;
}

bool jackbridge_disconnect(jack_client_t* client, const char* source_port, const char* destination_port)
{
    if (bridge.disconnect_ptr != nullptr)
        return (bridge.disconnect_ptr(client, source_port, destination_port) == 0);
    return false;
}

int jackbridge_port_name_size()
{
    if (bridge.port_name_size_ptr != nullptr)
        return bridge.port_name_size_ptr();
    return 0;
}

void jackbridge_port_get_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
    if (bridge.port_get_latency_range_ptr != nullptr)
        bridge.port_get_latency_range_ptr(port, mode, range);
}

void jackbridge_port_set_latency_range(jack_port_t* port, jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
    if (bridge.port_set_latency_range_ptr != nullptr)
        bridge.port_set_latency_range_ptr(port, mode, range);
}

bool jackbridge_recompute_total_latencies(jack_client_t* client)
{
    if (bridge.recompute_total_latencies_ptr != nullptr)
        return (bridge.recompute_total_latencies_ptr(client) == 0);
    return false;
}

const char** jackbridge_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, unsigned long flags)
{
    if (bridge.get_ports_ptr != nullptr)
        return bridge.get_ports_ptr(client, port_name_pattern, type_name_pattern, flags);
    return nullptr;
}

jack_port_t* jackbridge_port_by_name(jack_client_t* client, const char* port_name)
{
    if (bridge.port_by_name_ptr != nullptr)
        return bridge.port_by_name_ptr(client, port_name);
    return nullptr;
}

jack_port_t* jackbridge_port_by_id(jack_client_t* client, jack_port_id_t port_id)
{
    if (bridge.port_by_id_ptr != nullptr)
        return bridge.port_by_id_ptr(client, port_id);
    return nullptr;
}

// -----------------------------------------------------------------------------

void jackbridge_free(void* ptr)
{
    if (bridge.free_ptr != nullptr)
        return bridge.free_ptr(ptr);

    // just in case
    free(ptr);
}

// -----------------------------------------------------------------------------

uint32_t jackbridge_midi_get_event_count(void* port_buffer)
{
    if (bridge.midi_get_event_count_ptr != nullptr)
        return bridge.midi_get_event_count_ptr(port_buffer);
    return 0;
}

bool jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index)
{
    if (bridge.midi_event_get_ptr != nullptr)
        return (bridge.midi_event_get_ptr(event, port_buffer, event_index) == 0);
    return false;
}

void jackbridge_midi_clear_buffer(void* port_buffer)
{
    if (bridge.midi_clear_buffer_ptr != nullptr)
        bridge.midi_clear_buffer_ptr(port_buffer);
}

bool jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, size_t data_size)
{
    if (bridge.midi_event_write_ptr != nullptr)
        return (bridge.midi_event_write_ptr(port_buffer, time, data, data_size) == 0);
    return false;
}

jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, size_t data_size)
{
    if (bridge.midi_event_reserve_ptr != nullptr)
        return bridge.midi_event_reserve_ptr(port_buffer, time, data_size);
    return nullptr;
}

// -----------------------------------------------------------------------------

int jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame)
{
    if (bridge.transport_locate_ptr != nullptr)
        return (bridge.transport_locate_ptr(client, frame) == 0);
    return false;
}

void jackbridge_transport_start(jack_client_t* client)
{
    if (bridge.transport_start_ptr != nullptr)
        bridge.transport_start_ptr(client);
}

void jackbridge_transport_stop(jack_client_t* client)
{
    if (bridge.transport_stop_ptr != nullptr)
        bridge.transport_stop_ptr(client);
}

jack_transport_state_t jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos)
{
    if (bridge.transport_query_ptr != nullptr)
        return bridge.transport_query_ptr(client, pos);
    return JackTransportStopped;
}

// -----------------------------------------------------------------------------

#ifdef JACKBRIDGE_DUMMY
bool jackbridge_sem_post(void* sem)
{
    return false;
}

bool jackbridge_sem_timedwait(void* sem, int secs)
{
    return false;
}
#else
bool jackbridge_sem_post(void* sem)
{
    return (sem_post((sem_t*)sem) == 0);
}

bool jackbridge_sem_timedwait(void* sem, int secs)
{
# ifdef CARLA_OS_MAC
        alarm(secs);
        return (sem_wait((sem_t*)sem) == 0);
# else
        timespec timeout;

#  ifdef CARLA_LIB_WINDOWS
        timeval now;
        gettimeofday(&now, nullptr);
        timeout.tv_sec  = now.tv_sec;
        timeout.tv_nsec = now.tv_usec * 1000;
#  else
        clock_gettime(CLOCK_REALTIME, &timeout);
#  endif

        timeout.tv_sec += secs;

        return (sem_timedwait((sem_t*)sem, &timeout) == 0);
# endif
}
#endif
