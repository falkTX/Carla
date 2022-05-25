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
#include "bstream.h"

#include "align_push.h"

/**
 * buses
 */

enum v3_media_types {
	V3_AUDIO = 0,
	V3_EVENT
};

static inline
const char* v3_media_type_str(int32_t type)
{
	switch (type)
	{
	case V3_AUDIO:
		return "V3_AUDIO";
	case V3_EVENT:
		return "V3_EVENT";
	default:
		return "[unknown]";
	}
}

enum v3_bus_direction {
	V3_INPUT = 0,
	V3_OUTPUT
};

static inline
const char* v3_bus_direction_str(int32_t d)
{
	switch (d)
	{
	case V3_INPUT:
		return "V3_INPUT";
	case V3_OUTPUT:
		return "V3_OUTPUT";
	default:
		return "[unknown]";
	}
}

enum v3_bus_types {
	V3_MAIN = 0,
	V3_AUX
};

enum v3_bus_flags {
	V3_DEFAULT_ACTIVE     = 1 << 0,
	V3_IS_CONTROL_VOLTAGE = 1 << 1
};

struct v3_bus_info {
	int32_t media_type;
	int32_t direction;
	int32_t channel_count;
	v3_str_128 bus_name;
	int32_t bus_type;
	uint32_t flags;
};

/**
 * component
 */

struct v3_routing_info;

struct v3_component {
#ifndef __cplusplus
	struct v3_plugin_base;
#endif
	v3_result (V3_API *get_controller_class_id)(void* self, v3_tuid class_id);
	v3_result (V3_API *set_io_mode)(void* self, int32_t io_mode);
	int32_t (V3_API *get_bus_count)(void* self, int32_t media_type, int32_t bus_direction);
	v3_result (V3_API *get_bus_info)(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, struct v3_bus_info* bus_info);
	v3_result (V3_API *get_routing_info)(void* self, struct v3_routing_info* input, struct v3_routing_info* output);
	v3_result (V3_API *activate_bus)(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bool state);
	v3_result (V3_API *set_active)(void* self, v3_bool state);
	v3_result (V3_API *set_state)(void* self, struct v3_bstream **);
	v3_result (V3_API *get_state)(void* self, struct v3_bstream **);
};

static constexpr const v3_tuid v3_component_iid =
	V3_ID(0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_component_cpp : v3_funknown {
	v3_plugin_base base;
	v3_component comp;
};

template<> inline
constexpr v3_component* v3_cpp_obj(v3_component** obj)
{
	/**
	 * this ugly piece of code is required due to C++ assuming `reinterpret_cast` by default,
	 * but we need everything to be `static_cast` for it to be `constexpr` compatible.
	 */
	return static_cast<v3_component*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*5));
}

#endif

#include "align_pop.h"
