/*
 * Carla Tests
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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
 */

#include <stdint.h>

#define PRE_PACKED_STRUCTURE
#define POST_PACKED_STRUCTURE __attribute__((__packed__))

extern "C" {

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
    JackVideoFrameOffset = 0x100
};

typedef uint32_t jack_nframes_t;
typedef uint64_t jack_time_t;
typedef uint64_t jack_unique_t;

typedef enum JackTransportState jack_transport_state_t;
typedef enum JackPositionBits jack_position_bits_t;

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
    int32_t        padding[7];
    jack_unique_t  unique_2;
} POST_PACKED_STRUCTURE;

typedef struct _jack_client jack_client_t;
typedef struct _jack_position jack_position_t;

jack_transport_state_t jackbridge_transport_query(const jack_client_t*, jack_position_t*);

}

void testcall()
{
    const jack_transport_state_t state = jackbridge_transport_query(0, 0);

    return; // unused
    (void)state;
}
