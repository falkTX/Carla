/*
 * travesty, pure C VST3-compatible interface
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
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
#include "bstream.h"
#include "view.h"

#include "align_push.h"

/**
 * component handler
 */

enum {
	V3_RESTART_RELOAD_COMPONENT             = 1 << 0,
	V3_RESTART_IO_CHANGED                   = 1 << 1,
	V3_RESTART_PARAM_VALUES_CHANGED         = 1 << 2,
	V3_RESTART_LATENCY_CHANGED              = 1 << 3,
	V3_RESTART_PARAM_TITLES_CHANGED         = 1 << 4,
	V3_RESTART_MIDI_CC_ASSIGNMENT_CHANGED   = 1 << 5,
	V3_RESTART_NOTE_EXPRESSION_CHANGED      = 1 << 6,
	V3_RESTART_IO_TITLES_CHANGED            = 1 << 7,
	V3_RESTART_PREFETCHABLE_SUPPORT_CHANGED = 1 << 8,
	V3_RESTART_ROUTING_INFO_CHANGED         = 1 << 9
};

struct v3_component_handler {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* begin_edit)(void* self, v3_param_id);
	v3_result (V3_API* perform_edit)(void* self, v3_param_id, double value_normalised);
	v3_result (V3_API* end_edit)(void* self, v3_param_id);
	v3_result (V3_API* restart_component)(void* self, int32_t flags);
};

static constexpr const v3_tuid v3_component_handler_iid =
	V3_ID(0x93A0BEA3, 0x0BD045DB, 0x8E890B0C, 0xC1E46AC6);

struct v3_component_handler2 {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* set_dirty)(void* self, v3_bool state);
	v3_result (V3_API* request_open_editor)(void* self, const char* name);
	v3_result (V3_API* start_group_edit)(void* self);
	v3_result (V3_API* finish_group_edit)(void* self);
};

static constexpr const v3_tuid v3_component_handler2_iid =
	V3_ID(0xF040B4B3, 0xA36045EC, 0xABCDC045, 0xB4D5A2CC);

/**
 * edit controller
 */

enum {
	V3_PARAM_CAN_AUTOMATE   = 1 << 0,
	V3_PARAM_READ_ONLY      = 1 << 1,
	V3_PARAM_WRAP_AROUND    = 1 << 2,
	V3_PARAM_IS_LIST        = 1 << 3,
	V3_PARAM_IS_HIDDEN      = 1 << 4,
	V3_PARAM_PROGRAM_CHANGE = 1 << 15,
	V3_PARAM_IS_BYPASS      = 1 << 16
};

struct v3_param_info {
	v3_param_id param_id;
	v3_str_128 title;
	v3_str_128 short_title;
	v3_str_128 units;
	int32_t step_count;
	double default_normalised_value;
	int32_t unit_id;
	int32_t flags;
};

struct v3_edit_controller {
#ifndef __cplusplus
	struct v3_plugin_base;
#endif
	v3_result (V3_API* set_component_state)(void* self, struct v3_bstream**);
	v3_result (V3_API* set_state)(void* self, struct v3_bstream**);
	v3_result (V3_API* get_state)(void* self, struct v3_bstream**);
	int32_t (V3_API* get_parameter_count)(void* self);
	v3_result (V3_API* get_parameter_info)(void* self, int32_t param_idx, struct v3_param_info*);
	v3_result (V3_API* get_parameter_string_for_value)(void* self, v3_param_id, double normalised, v3_str_128 output);
	v3_result (V3_API* get_parameter_value_for_string)(void* self, v3_param_id, int16_t* input, double* output);
	double (V3_API* normalised_parameter_to_plain)(void* self, v3_param_id, double normalised);
	double (V3_API* plain_parameter_to_normalised)(void* self, v3_param_id, double plain);
	double (V3_API* get_parameter_normalised)(void* self, v3_param_id);
	v3_result (V3_API* set_parameter_normalised)(void* self, v3_param_id, double normalised);
	v3_result (V3_API* set_component_handler)(void* self, struct v3_component_handler**);
	struct v3_plugin_view** (V3_API* create_view)(void* self, const char* name);
};

static constexpr const v3_tuid v3_edit_controller_iid =
	V3_ID(0xDCD7BBE3, 0x7742448D, 0xA874AACC, 0x979C759E);

/**
 * midi mapping
 */

struct v3_midi_mapping {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* get_midi_controller_assignment)(void* self, int32_t bus, int16_t channel, int16_t cc, v3_param_id* id);
};

static constexpr const v3_tuid v3_midi_mapping_iid =
	V3_ID(0xDF0FF9F7, 0x49B74669, 0xB63AB732, 0x7ADBF5E5);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_component_handler_cpp : v3_funknown {
	v3_component_handler comp;
};

struct v3_component_handler2_cpp : v3_funknown {
	v3_component_handler2 comp2;
};

struct v3_edit_controller_cpp : v3_funknown {
	v3_plugin_base base;
	v3_edit_controller ctrl;
};

struct v3_midi_mapping_cpp : v3_funknown {
	v3_midi_mapping map;
};

template<> inline
constexpr v3_edit_controller* v3_cpp_obj(v3_edit_controller** obj)
{
	/**
	 * this ugly piece of code is required due to C++ assuming `reinterpret_cast` by default,
	 * but we need everything to be `static_cast` for it to be `constexpr` compatible.
	 */
	return static_cast<v3_edit_controller*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*5));
}

#endif

#include "align_pop.h"
