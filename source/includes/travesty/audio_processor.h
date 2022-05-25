/*
 * travesty, pure C VST3-compatible interface
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "base.h"
#include "events.h"

#include "align_push.h"

/**
 * speakers
 */

typedef uint64_t v3_speaker_arrangement;

enum {
	V3_SPEAKER_L = 1 << 0,
	V3_SPEAKER_R = 1 << 1,
	V3_SPEAKER_M = 1 << 19
};

/**
 * process setup
 */

enum v3_process_mode {
	V3_REALTIME,
	V3_PREFETCH,
	V3_OFFLINE
};

static inline
const char* v3_process_mode_str(int32_t d)
{
	switch (d)
	{
	case V3_REALTIME:
		return "V3_REALTIME";
	case V3_PREFETCH:
		return "V3_PREFETCH";
	case V3_OFFLINE:
		return "V3_OFFLINE";
	default:
		return "[unknown]";
	}
}

enum {
	V3_SAMPLE_32,
	V3_SAMPLE_64
};

static inline
const char* v3_sample_size_str(int32_t d)
{
	switch (d)
	{
	case V3_SAMPLE_32:
		return "V3_SAMPLE_32";
	case V3_SAMPLE_64:
		return "V3_SAMPLE_64";
	default:
		return "[unknown]";
	}
}

struct v3_process_setup {
	int32_t process_mode;
	int32_t symbolic_sample_size;
	int32_t max_block_size;
	double sample_rate;
};

/**
 * param changes
 */

struct v3_param_value_queue {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_param_id (V3_API* get_param_id)(void* self);
	int32_t (V3_API* get_point_count)(void* self);
	v3_result (V3_API* get_point)(void* self, int32_t idx, int32_t* sample_offset, double* value);
	v3_result (V3_API* add_point)(void* self, int32_t sample_offset, double value, int32_t* idx);
};

static constexpr const v3_tuid v3_param_value_queue_iid =
	V3_ID(0x01263A18, 0xED074F6F, 0x98C9D356, 0x4686F9BA);

struct v3_param_changes {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	int32_t (V3_API* get_param_count)(void* self);
	struct v3_param_value_queue** (V3_API* get_param_data)(void* self, int32_t idx);
	struct v3_param_value_queue** (V3_API* add_param_data)(void* self, v3_param_id* id, int32_t* index);
};

static constexpr const v3_tuid v3_param_changes_iid =
	V3_ID(0xA4779663, 0x0BB64A56, 0xB44384A8, 0x466FEB9D);

/**
 * process context
 */

struct v3_frame_rate {
	uint32_t fps;
	uint32_t flags;
};

struct v3_chord {
	uint8_t key_note;
	uint8_t root_note;
	int16_t chord_mask;
};

enum {
	V3_PROCESS_CTX_PLAYING            = 1 << 1,
	V3_PROCESS_CTX_CYCLE_ACTIVE       = 1 << 2,
	V3_PROCESS_CTX_RECORDING          = 1 << 3,
	V3_PROCESS_CTX_SYSTEM_TIME_VALID  = 1 << 8,
	V3_PROCESS_CTX_PROJECT_TIME_VALID = 1 << 9,
	V3_PROCESS_CTX_TEMPO_VALID        = 1 << 10,
	V3_PROCESS_CTX_BAR_POSITION_VALID = 1 << 11,
	V3_PROCESS_CTX_CYCLE_VALID        = 1 << 12,
	V3_PROCESS_CTX_TIME_SIG_VALID     = 1 << 13,
	V3_PROCESS_CTX_SMPTE_VALID        = 1 << 14,
	V3_PROCESS_CTX_NEXT_CLOCK_VALID   = 1 << 15,
	V3_PROCESS_CTX_CONT_TIME_VALID    = 1 << 17,
	V3_PROCESS_CTX_CHORD_VALID        = 1 << 18
};

struct v3_process_context {
	uint32_t state;
	double sample_rate;
	int64_t project_time_in_samples; // with loop
	int64_t system_time_ns;
	int64_t continuous_time_in_samples; // without loop
	double project_time_quarters;
	double bar_position_quarters;
	double cycle_start_quarters;
	double cycle_end_quarters;
	double bpm;
	int32_t time_sig_numerator;
	int32_t time_sig_denom;
	struct v3_chord chord;
	int32_t smpte_offset_subframes;
	struct v3_frame_rate frame_rate;
	int32_t samples_to_next_clock;
};

/**
 * process context requirements
 */

enum {
	V3_PROCESS_CTX_NEED_SYSTEM_TIME      = 1 << 0,
	V3_PROCESS_CTX_NEED_CONTINUOUS_TIME  = 1 << 1,
	V3_PROCESS_CTX_NEED_PROJECT_TIME     = 1 << 2,
	V3_PROCESS_CTX_NEED_BAR_POSITION     = 1 << 3,
	V3_PROCESS_CTX_NEED_CYCLE            = 1 << 4,
	V3_PROCESS_CTX_NEED_NEXT_CLOCK       = 1 << 5,
	V3_PROCESS_CTX_NEED_TEMPO            = 1 << 6,
	V3_PROCESS_CTX_NEED_TIME_SIG         = 1 << 7,
	V3_PROCESS_CTX_NEED_CHORD            = 1 << 8,
	V3_PROCESS_CTX_NEED_FRAME_RATE       = 1 << 9,
	V3_PROCESS_CTX_NEED_TRANSPORT_STATE  = 1 << 10
};

struct v3_process_context_requirements {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	uint32_t (V3_API* get_process_context_requirements)(void* self);
};

static constexpr const v3_tuid v3_process_context_requirements_iid =
	V3_ID(0x2A654303, 0xEF764E3D, 0x95B5FE83, 0x730EF6D0);

/**
 * process data and context
 */

struct v3_audio_bus_buffers {
	int32_t num_channels;
	uint64_t channel_silence_bitset;
	union {
		float** channel_buffers_32;
		double** channel_buffers_64;
	};
};

struct v3_process_data {
	int32_t process_mode;
	int32_t symbolic_sample_size;
	int32_t nframes;
	int32_t num_input_buses;
	int32_t num_output_buses;
	struct v3_audio_bus_buffers* inputs;
	struct v3_audio_bus_buffers* outputs;
	struct v3_param_changes** input_params;
	struct v3_param_changes** output_params;
	struct v3_event_list** input_events;
	struct v3_event_list** output_events;
	struct v3_process_context* ctx;
};

/**
 * audio processor
 */

struct v3_audio_processor {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* set_bus_arrangements)(void* self, v3_speaker_arrangement* inputs, int32_t num_inputs,
	                                         v3_speaker_arrangement* outputs, int32_t num_outputs);
	v3_result (V3_API* get_bus_arrangement)(void* self, int32_t bus_direction, int32_t idx, v3_speaker_arrangement*);
	v3_result (V3_API* can_process_sample_size)(void* self, int32_t symbolic_sample_size);
	uint32_t (V3_API* get_latency_samples)(void* self);
	v3_result (V3_API* setup_processing)(void* self, struct v3_process_setup* setup);
	v3_result (V3_API* set_processing)(void* self, v3_bool state);
	v3_result (V3_API* process)(void* self, struct v3_process_data* data);
	uint32_t (V3_API* get_tail_samples)(void* self);
};

static constexpr const v3_tuid v3_audio_processor_iid =
	V3_ID(0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_param_value_queue_cpp : v3_funknown {
	v3_param_value_queue queue;
};

struct v3_param_changes_cpp : v3_funknown {
	v3_param_changes changes;
};

struct v3_process_context_requirements_cpp : v3_funknown {
	v3_process_context_requirements req;
};

struct v3_audio_processor_cpp : v3_funknown {
	v3_audio_processor proc;
};

#endif

#include "align_pop.h"
