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

struct v3_unit_info {
	int32_t id; // 0 for root unit
	int32_t parent_unit_id; // -1 for none
	v3_str_128 name;
	int32_t program_list_id; // -1 for none
};

struct v3_program_list_info {
	int32_t id;
	v3_str_128 name;
	int32_t programCount;
};

struct v3_unit_information {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	int32_t (V3_API* get_unit_count)(void* self);
	v3_result (V3_API* get_unit_info)(void* self, int32_t unit_idx, v3_unit_info* info);
	int32_t (V3_API* get_program_list_count)(void* self);
	v3_result (V3_API* get_program_list_info)(void* self, int32_t list_idx, v3_program_list_info* info);
	v3_result (V3_API* get_program_name)(void* self, int32_t list_id, int32_t program_idx, v3_str_128 name);
	v3_result (V3_API* get_program_info)(void* self, int32_t list_id, int32_t program_idx, const char *attribute_id, v3_str_128 attribute_value);
	v3_result (V3_API* has_program_pitch_names)(void* self, int32_t list_id, int32_t program_idx);
	v3_result (V3_API* get_program_pitch_name)(void* self, int32_t list_id, int32_t program_idx, int16_t midi_pitch, v3_str_128 name);
	int32_t (V3_API* get_selected_unit)(void* self);
	v3_result (V3_API* select_unit)(void* self, int32_t unit_id);
	v3_result (V3_API* get_unit_by_bus)(void* self, int32_t type, int32_t bus_direction, int32_t bus_idx, int32_t channel, int32_t* unit_id);
	v3_result (V3_API* set_unit_program_data)(void* self, int32_t list_or_unit_id, int32_t program_idx, struct v3_bstream** data);
};

static constexpr const v3_tuid v3_unit_information_iid =
	V3_ID(0x3D4BD6B5, 0x913A4FD2, 0xA886E768, 0xA5EB92C1);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_unit_information_cpp : v3_funknown {
	v3_unit_information unit;
};

#endif

#include "align_pop.h"
