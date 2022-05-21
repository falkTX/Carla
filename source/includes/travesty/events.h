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

#include "align_push.h"

/**
 * note events
 */

struct v3_event_note_on {
	int16_t channel;
	int16_t pitch; // MIDI note number
	float tuning;
	float velocity;
	int32_t length;
	int32_t note_id;
};

struct v3_event_note_off {
	int16_t channel;
	int16_t pitch; // MIDI note number
	float velocity;
	int32_t note_id;
	float tuning;
};

struct v3_event_data {
	uint32_t size;
	uint32_t type;
	const uint8_t* bytes;
};

struct v3_event_poly_pressure {
	int16_t channel;
	int16_t pitch;
	float pressure;
	int32_t note_id;
};

struct v3_event_chord {
	int16_t root;
	int16_t bass_note;
	int16_t mask;
	uint16_t text_len;
	const int16_t* text;
};

struct v3_event_scale {
	int16_t root;
	int16_t mask;
	uint16_t text_len;
	const int16_t* text;
};

struct v3_event_legacy_midi_cc_out {
	uint8_t cc_number;
	int8_t channel;
	int8_t value;
	int8_t value2;
};

struct v3_event_note_expression_value {
	uint32_t type_id;
	int32_t note_id;
	double value;
};

struct v3_event_note_expression_text {
	int32_t note_id;
	uint32_t text_len;
	const int16_t* text;
};

/**
 * event
 */

enum v3_event_flags {
	V3_EVENT_IS_LIVE = 1 << 0
};

enum v3_event_type {
	V3_EVENT_NOTE_ON        = 0,
	V3_EVENT_NOTE_OFF       = 1,
	V3_EVENT_DATA           = 2,
	V3_EVENT_POLY_PRESSURE  = 3,
	V3_EVENT_NOTE_EXP_VALUE = 4,
	V3_EVENT_NOTE_EXP_TEXT  = 5,
	V3_EVENT_CHORD          = 6,
	V3_EVENT_SCALE          = 7,
	V3_EVENT_LEGACY_MIDI_CC_OUT = 65535
};

struct v3_event {
	int32_t bus_index;
	int32_t sample_offset;
	double ppq_position;
	uint16_t flags;
	uint16_t type;
	union {
		struct v3_event_note_on note_on;
		struct v3_event_note_off note_off;
		struct v3_event_data data;
		struct v3_event_poly_pressure poly_pressure;
		struct v3_event_chord chord;
		struct v3_event_scale scale;
		struct v3_event_legacy_midi_cc_out midi_cc_out;
		struct v3_event_note_expression_value note_exp_value;
		struct v3_event_note_expression_text note_exp_text;
	};
};

/**
 * event list
 */

struct v3_event_list {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	uint32_t (V3_API* get_event_count)(void* self);
	v3_result (V3_API* get_event)(void* self, int32_t idx, struct v3_event* event);
	v3_result (V3_API* add_event)(void* self, struct v3_event* event);
};

static constexpr const v3_tuid v3_event_list_iid =
	V3_ID(0x3A2C4214, 0x346349FE, 0xB2C4F397, 0xB9695A44);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_event_list_cpp : v3_funknown {
	v3_event_list list;
};

#endif

#include "align_pop.h"
