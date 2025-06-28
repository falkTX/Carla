/*
 * JackBridge
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef JACKBRIDGE_HPP_INCLUDED
#define JACKBRIDGE_HPP_INCLUDED

#ifdef __WINE__
# if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#  define __WINE64__
# endif
# undef WIN32
# undef WIN64
# undef _WIN32
# undef _WIN64
# undef __WIN32__
# undef __WIN64__
#endif

#include "CarlaDefines.h"

#if (defined(__WINE__) || defined(CARLA_OS_WIN)) && defined(__cdecl)
# define JACKBRIDGE_API __cdecl
#else
# define JACKBRIDGE_API
#endif

#ifdef JACKBRIDGE_DIRECT
# include <jack/jack.h>
# include <jack/midiport.h>
# include <jack/transport.h>
# include <jack/session.h>
# include <jack/metadata.h>
# include <jack/uuid.h>
#else

#include <cstddef>

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifndef POST_PACKED_STRUCTURE
# if defined(__GNUC__)
  /* POST_PACKED_STRUCTURE needs to be a macro which
      expands into a compiler directive. The directive must
      tell the compiler to arrange the preceding structure
      declaration so that it is packed on byte-boundaries rather
      than use the natural alignment of the processor and/or
      compiler.
  */
  #define PRE_PACKED_STRUCTURE
  #define POST_PACKED_STRUCTURE __attribute__((__packed__))
# elif defined(_MSC_VER)
  #define PRE_PACKED_STRUCTURE1 __pragma(pack(push,1))
  #define PRE_PACKED_STRUCTURE    PRE_PACKED_STRUCTURE1
  /* PRE_PACKED_STRUCTURE needs to be a macro which
      expands into a compiler directive. The directive must
      tell the compiler to arrange the following structure
      declaration so that it is packed on byte-boundaries rather
      than use the natural alignment of the processor and/or
      compiler.
  */
  #define POST_PACKED_STRUCTURE ;__pragma(pack(pop))
  /* and POST_PACKED_STRUCTURE needs to be a macro which
      restores the packing to its previous setting */
# else
  #define PRE_PACKED_STRUCTURE
  #define POST_PACKED_STRUCTURE
# endif
#endif

#if (defined(__arm__) || defined(__aarch64__) || defined(__mips__) || defined(__ppc__) || defined(__powerpc__)) && !defined(__APPLE__)
# undef POST_PACKED_STRUCTURE
# define POST_PACKED_STRUCTURE
#endif

#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define JACK_DEFAULT_MIDI_TYPE  "8 bit raw midi"

#define JACK_MAX_FRAMES (4294967295U)

#define JackOpenOptions (JackSessionID|JackServerName|JackNoStartServer|JackUseExactName)
#define JackLoadOptions (JackLoadInit|JackLoadName|JackUseExactName)

#define JACK_POSITION_MASK (JackPositionBBT|JackPositionTimecode|JackBBTFrameOffset|JackAudioVideoRatio|JackVideoFrameOffset)
#define EXTENDED_TIME_INFO

#define JACK_UUID_SIZE 36
#define JACK_UUID_STRING_SIZE (JACK_UUID_SIZE+1) /* includes trailing null */

#define JACK_TICK_DOUBLE

extern "C" {

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
    JackClientZombie  = 0x1000,
    JackBridgeNativeFailed = 0x10000
};

enum JackLatencyCallbackMode {
    JackCaptureLatency,
    JackPlaybackLatency
};

enum JackPortFlags {
    JackPortIsInput    = 0x01,
    JackPortIsOutput   = 0x02,
    JackPortIsPhysical = 0x04,
    JackPortCanMonitor = 0x08,
    JackPortIsTerminal = 0x10,
    JackPortIsCV       = 0x20,
    JackPortIsMIDI2    = 0x20,
};

enum JackTransportState {
    JackTransportStopped  = 0,
    JackTransportRolling  = 1,
    JackTransportLooping  = 2,
    JackTransportStarting = 3
};

enum JackPositionBits {
    JackPositionBBT      = 0x010,
    JackPositionTimecode = 0x020,
    JackBBTFrameOffset   = 0x040,
    JackAudioVideoRatio  = 0x080,
    JackVideoFrameOffset = 0x100,
    JackTickDouble       = 0x200
};

enum JackSessionEventType {
    JackSessionSave         = 1,
    JackSessionSaveAndQuit  = 2,
    JackSessionSaveTemplate = 3
};

enum JackSessionFlags {
    JackSessionSaveError    = 0x1,
    JackSessionNeedTerminal = 0x2
};

enum JackPropertyChange {
    PropertyCreated,
    PropertyChanged,
    PropertyDeleted
};

typedef uint32_t jack_nframes_t;
typedef uint32_t jack_port_id_t;
typedef uint64_t jack_time_t;
typedef uint64_t jack_uuid_t;
typedef uint64_t jack_unique_t;
typedef uchar jack_midi_data_t;
typedef float jack_default_audio_sample_t;

typedef enum JackOptions jack_options_t;
typedef enum JackStatus jack_status_t;
typedef enum JackLatencyCallbackMode jack_latency_callback_mode_t;
typedef enum JackTransportState jack_transport_state_t;
typedef enum JackPositionBits jack_position_bits_t;
typedef enum JackSessionEventType jack_session_event_type_t;
typedef enum JackSessionFlags jack_session_flags_t;
typedef enum JackPropertyChange jack_property_change_t;

struct _jack_midi_event {
    jack_nframes_t    time;
    size_t            size;
    jack_midi_data_t* buffer;
};

// NOTE: packed in JACK2 but not in JACK1
PRE_PACKED_STRUCTURE
struct _jack_latency_range {
    jack_nframes_t min;
    jack_nframes_t max;
} POST_PACKED_STRUCTURE;

PRE_PACKED_STRUCTURE
struct _jack_position {
    jack_unique_t  unique_1;
    jack_time_t    usecs;
    jack_nframes_t frame_rate;
    jack_nframes_t frame;
    jack_position_bits_t valid;
    int32_t bar;
    int32_t beat;
    int32_t tick;
    double  bar_start_tick;
    float   beats_per_bar;
    float   beat_type;
    double  ticks_per_beat;
    double  beats_per_minute;
    double  frame_time;
    double  next_time;
    jack_nframes_t bbt_offset;
    float          audio_frames_per_video_frame;
    jack_nframes_t video_offset;
    double         tick_double;
    int32_t        padding[5];
    jack_unique_t  unique_2;
} POST_PACKED_STRUCTURE;

struct _jack_session_event {
    jack_session_event_type_t type;
    const char* session_dir;
    const char* client_uuid;
    char*       command_line;
    jack_session_flags_t flags;
    uint32_t future;
};

struct _jack_session_command_t {
    const char* uuid;
    const char* client_name;
    const char* command;
    jack_session_flags_t flags;
};

typedef struct {
    const char* key;
    const char* data;
    const char* type;
} jack_property_t;

typedef struct {
    jack_uuid_t      subject;
    uint32_t         property_cnt;
    jack_property_t* properties;
    uint32_t         property_size;
} jack_description_t;

typedef struct _jack_port jack_port_t;
typedef struct _jack_client jack_client_t;
typedef struct _jack_midi_event jack_midi_event_t;
typedef struct _jack_latency_range jack_latency_range_t;
typedef struct _jack_position jack_position_t;
typedef struct _jack_session_event jack_session_event_t;
typedef struct _jack_session_command_t jack_session_command_t;

typedef void (JACKBRIDGE_API *JackLatencyCallback)(jack_latency_callback_mode_t mode, void* arg);
typedef int  (JACKBRIDGE_API *JackProcessCallback)(jack_nframes_t nframes, void* arg);
typedef void (JACKBRIDGE_API *JackThreadInitCallback)(void* arg);
typedef int  (JACKBRIDGE_API *JackGraphOrderCallback)(void* arg);
typedef int  (JACKBRIDGE_API *JackXRunCallback)(void* arg);
typedef int  (JACKBRIDGE_API *JackBufferSizeCallback)(jack_nframes_t nframes, void* arg);
typedef int  (JACKBRIDGE_API *JackSampleRateCallback)(jack_nframes_t nframes, void* arg);
typedef void (JACKBRIDGE_API *JackPortRegistrationCallback)(jack_port_id_t port, int register_, void* arg);
typedef void (JACKBRIDGE_API *JackClientRegistrationCallback)(const char* name, int register_, void* arg);
typedef void (JACKBRIDGE_API *JackPortConnectCallback)(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
typedef void (JACKBRIDGE_API *JackPortRenameCallback)(jack_port_id_t port, const char* old_name, const char* new_name, void* arg);
typedef void (JACKBRIDGE_API *JackFreewheelCallback)(int starting, void* arg);
typedef void (JACKBRIDGE_API *JackShutdownCallback)(void* arg);
typedef void (JACKBRIDGE_API *JackInfoShutdownCallback)(jack_status_t code, const char* reason, void* arg);
typedef int  (JACKBRIDGE_API *JackSyncCallback)(jack_transport_state_t state, jack_position_t* pos, void* arg);
typedef void (JACKBRIDGE_API *JackTimebaseCallback)(jack_transport_state_t state, jack_nframes_t nframes, jack_position_t* pos, int new_pos, void* arg);
typedef void (JACKBRIDGE_API *JackSessionCallback)(jack_session_event_t* event, void* arg);
typedef void (JACKBRIDGE_API *JackPropertyChangeCallback)(jack_uuid_t subject, const char* key, jack_property_change_t change, void* arg);

} // extern "C"

#endif // ! JACKBRIDGE_DIRECT

JACKBRIDGE_API bool jackbridge_is_ok() noexcept;
JACKBRIDGE_API void jackbridge_init();

JACKBRIDGE_API void        jackbridge_get_version(int* major_ptr, int* minor_ptr, int* micro_ptr, int* proto_ptr);
JACKBRIDGE_API const char* jackbridge_get_version_string();

JACKBRIDGE_API jack_client_t* jackbridge_client_open(const char* client_name, uint32_t options, jack_status_t* status);
JACKBRIDGE_API bool           jackbridge_client_close(jack_client_t* client);

JACKBRIDGE_API int   jackbridge_client_name_size();
JACKBRIDGE_API char* jackbridge_get_client_name(jack_client_t* client);

JACKBRIDGE_API char* jackbridge_client_get_uuid(jack_client_t* client);
JACKBRIDGE_API char* jackbridge_get_uuid_for_client_name(jack_client_t* client, const char* name);
JACKBRIDGE_API char* jackbridge_get_client_name_by_uuid(jack_client_t* client, const char* uuid);

JACKBRIDGE_API bool jackbridge_uuid_parse(const char* buf, jack_uuid_t* uuid);
JACKBRIDGE_API void jackbridge_uuid_unparse(jack_uuid_t uuid, char buf[JACK_UUID_STRING_SIZE]);

JACKBRIDGE_API bool jackbridge_activate(jack_client_t* client);
JACKBRIDGE_API bool jackbridge_deactivate(jack_client_t* client);
JACKBRIDGE_API bool jackbridge_is_realtime(jack_client_t* client);

JACKBRIDGE_API bool jackbridge_set_thread_init_callback(jack_client_t* client, JackThreadInitCallback thread_init_callback, void* arg);
JACKBRIDGE_API void jackbridge_on_shutdown(jack_client_t* client, JackShutdownCallback shutdown_callback, void* arg);
JACKBRIDGE_API void jackbridge_on_info_shutdown(jack_client_t* client, JackInfoShutdownCallback shutdown_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_process_callback(jack_client_t* client, JackProcessCallback process_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_freewheel_callback(jack_client_t* client, JackFreewheelCallback freewheel_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback bufsize_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback srate_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_client_registration_callback(jack_client_t* client, JackClientRegistrationCallback registration_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_port_registration_callback(jack_client_t* client, JackPortRegistrationCallback registration_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_port_rename_callback(jack_client_t* client, JackPortRenameCallback rename_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_port_connect_callback(jack_client_t* client, JackPortConnectCallback connect_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_graph_order_callback(jack_client_t* client, JackGraphOrderCallback graph_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_xrun_callback(jack_client_t* client, JackXRunCallback xrun_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_latency_callback(jack_client_t* client, JackLatencyCallback latency_callback, void* arg);

JACKBRIDGE_API bool jackbridge_set_freewheel(jack_client_t* client, bool onoff);
JACKBRIDGE_API bool jackbridge_set_buffer_size(jack_client_t* client, jack_nframes_t nframes);

JACKBRIDGE_API jack_nframes_t jackbridge_get_sample_rate(jack_client_t* client);
JACKBRIDGE_API jack_nframes_t jackbridge_get_buffer_size(jack_client_t* client);
JACKBRIDGE_API float          jackbridge_cpu_load(jack_client_t* client);

JACKBRIDGE_API jack_port_t* jackbridge_port_register(jack_client_t* client, const char* port_name, const char* port_type, uint64_t flags, uint64_t buffer_size);
JACKBRIDGE_API bool         jackbridge_port_unregister(jack_client_t* client, jack_port_t* port);
JACKBRIDGE_API void*        jackbridge_port_get_buffer(jack_port_t* port, jack_nframes_t nframes);

JACKBRIDGE_API const char*  jackbridge_port_name(const jack_port_t* port);
JACKBRIDGE_API jack_uuid_t  jackbridge_port_uuid(const jack_port_t* port);
JACKBRIDGE_API const char*  jackbridge_port_short_name(const jack_port_t* port);
JACKBRIDGE_API int          jackbridge_port_flags(const jack_port_t* port);
JACKBRIDGE_API const char*  jackbridge_port_type(const jack_port_t* port);
JACKBRIDGE_API bool         jackbridge_port_is_mine(const jack_client_t* client, const jack_port_t* port);
JACKBRIDGE_API int          jackbridge_port_connected(const jack_port_t* port);
JACKBRIDGE_API bool         jackbridge_port_connected_to(const jack_port_t* port, const char* port_name);
JACKBRIDGE_API const char** jackbridge_port_get_connections(const jack_port_t* port);
JACKBRIDGE_API const char** jackbridge_port_get_all_connections(const jack_client_t* client, const jack_port_t* port);

JACKBRIDGE_API bool jackbridge_port_rename(jack_client_t* client, jack_port_t* port, const char* port_name);
JACKBRIDGE_API bool jackbridge_port_set_alias(jack_port_t* port, const char* alias);
JACKBRIDGE_API bool jackbridge_port_unset_alias(jack_port_t* port, const char* alias);
JACKBRIDGE_API int  jackbridge_port_get_aliases(const jack_port_t* port, char* const al[2]);

JACKBRIDGE_API bool jackbridge_port_request_monitor(jack_port_t* port, bool onoff);
JACKBRIDGE_API bool jackbridge_port_request_monitor_by_name(jack_client_t* client, const char* port_name, bool onoff);
JACKBRIDGE_API bool jackbridge_port_ensure_monitor(jack_port_t* port, bool onoff);
JACKBRIDGE_API bool jackbridge_port_monitoring_input(jack_port_t* port);

JACKBRIDGE_API bool jackbridge_connect(jack_client_t* client, const char* source_port, const char* destination_port);
JACKBRIDGE_API bool jackbridge_disconnect(jack_client_t* client, const char* source_port, const char* destination_port);
JACKBRIDGE_API bool jackbridge_port_disconnect(jack_client_t* client, jack_port_t* port);

JACKBRIDGE_API int      jackbridge_port_name_size();
JACKBRIDGE_API int      jackbridge_port_type_size();
JACKBRIDGE_API uint32_t jackbridge_port_type_get_buffer_size(jack_client_t* client, const char* port_type);

JACKBRIDGE_API void jackbridge_port_get_latency_range(jack_port_t* port, uint32_t mode, jack_latency_range_t* range);
JACKBRIDGE_API void jackbridge_port_set_latency_range(jack_port_t* port, uint32_t mode, jack_latency_range_t* range);
JACKBRIDGE_API bool jackbridge_recompute_total_latencies(jack_client_t* client);

JACKBRIDGE_API const char** jackbridge_get_ports(jack_client_t* client, const char* port_name_pattern, const char* type_name_pattern, uint64_t flags);
JACKBRIDGE_API jack_port_t* jackbridge_port_by_name(jack_client_t* client, const char* port_name);
JACKBRIDGE_API jack_port_t* jackbridge_port_by_id(jack_client_t* client, jack_port_id_t port_id);

JACKBRIDGE_API void jackbridge_free(void* ptr);

JACKBRIDGE_API uint32_t jackbridge_midi_get_event_count(void* port_buffer);
JACKBRIDGE_API bool     jackbridge_midi_event_get(jack_midi_event_t* event, void* port_buffer, uint32_t event_index);
JACKBRIDGE_API void     jackbridge_midi_clear_buffer(void* port_buffer);
JACKBRIDGE_API bool     jackbridge_midi_event_write(void* port_buffer, jack_nframes_t time, const jack_midi_data_t* data, uint32_t data_size);
JACKBRIDGE_API jack_midi_data_t* jackbridge_midi_event_reserve(void* port_buffer, jack_nframes_t time, uint32_t data_size);

JACKBRIDGE_API bool jackbridge_release_timebase(jack_client_t* client);
JACKBRIDGE_API bool jackbridge_set_sync_callback(jack_client_t* client, JackSyncCallback sync_callback, void* arg);
JACKBRIDGE_API bool jackbridge_set_sync_timeout(jack_client_t* client, jack_time_t timeout);
JACKBRIDGE_API bool jackbridge_set_timebase_callback(jack_client_t* client, bool conditional, JackTimebaseCallback timebase_callback, void* arg);
JACKBRIDGE_API bool jackbridge_transport_locate(jack_client_t* client, jack_nframes_t frame);

JACKBRIDGE_API uint32_t       jackbridge_transport_query(const jack_client_t* client, jack_position_t* pos);
JACKBRIDGE_API jack_nframes_t jackbridge_get_current_transport_frame(const jack_client_t* client);

JACKBRIDGE_API bool jackbridge_transport_reposition(jack_client_t* client, const jack_position_t* pos);
JACKBRIDGE_API void jackbridge_transport_start(jack_client_t* client);
JACKBRIDGE_API void jackbridge_transport_stop(jack_client_t* client);

JACKBRIDGE_API bool jackbridge_set_property(jack_client_t* client, jack_uuid_t subject, const char* key, const char* value, const char* type);
JACKBRIDGE_API bool jackbridge_get_property(jack_uuid_t subject, const char* key, char** value, char** type);
JACKBRIDGE_API void jackbridge_free_description(jack_description_t* desc, bool free_description_itself);
JACKBRIDGE_API bool jackbridge_get_properties(jack_uuid_t subject, jack_description_t* desc);
JACKBRIDGE_API bool jackbridge_get_all_properties(jack_description_t** descs);
JACKBRIDGE_API bool jackbridge_remove_property(jack_client_t* client, jack_uuid_t subject, const char* key);
JACKBRIDGE_API int  jackbridge_remove_properties(jack_client_t* client, jack_uuid_t subject);
JACKBRIDGE_API bool jackbridge_remove_all_properties(jack_client_t* client);
JACKBRIDGE_API bool jackbridge_set_property_change_callback(jack_client_t* client, JackPropertyChangeCallback callback, void* arg);

JACKBRIDGE_API bool jackbridge_sem_init(void* sem) noexcept;
JACKBRIDGE_API void jackbridge_sem_destroy(void* sem) noexcept;
JACKBRIDGE_API bool jackbridge_sem_connect(void* sem) noexcept;
JACKBRIDGE_API void jackbridge_sem_post(void* sem, bool server) noexcept;
#ifndef CARLA_OS_WASM
JACKBRIDGE_API bool jackbridge_sem_timedwait(void* sem, uint msecs, bool server) noexcept;
#endif

JACKBRIDGE_API bool  jackbridge_shm_is_valid(const void* shm) noexcept;
JACKBRIDGE_API void  jackbridge_shm_init(void* shm) noexcept;
JACKBRIDGE_API void  jackbridge_shm_attach(void* shm, const char* name) noexcept;
JACKBRIDGE_API void  jackbridge_shm_close(void* shm) noexcept;
JACKBRIDGE_API void* jackbridge_shm_map(void* shm, uint64_t size) noexcept;
JACKBRIDGE_API void  jackbridge_shm_unmap(void* shm, void* ptr) noexcept;

JACKBRIDGE_API void* jackbridge_discovery_pipe_create(const char* argv[]);
JACKBRIDGE_API void  jackbridge_discovery_pipe_message(void* pipe, const char* key, const char* value);
JACKBRIDGE_API void  jackbridge_discovery_pipe_destroy(void* pipe);

JACKBRIDGE_API void jackbridge_parent_deathsig(bool kill) noexcept;

#endif // JACKBRIDGE_HPP_INCLUDED
