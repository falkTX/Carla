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
 * attribute list
 */

struct v3_attribute_list {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* set_int)(void* self, const char* id, int64_t value);
	v3_result (V3_API* get_int)(void* self, const char* id, int64_t* value);
	v3_result (V3_API* set_float)(void* self, const char* id, double value);
	v3_result (V3_API* get_float)(void* self, const char* id, double* value);
	v3_result (V3_API* set_string)(void* self, const char* id, const int16_t* string);
	v3_result (V3_API* get_string)(void* self, const char* id, int16_t* string, uint32_t size);
	v3_result (V3_API* set_binary)(void* self, const char* id, const void* data, uint32_t size);
	v3_result (V3_API* get_binary)(void* self, const char* id, const void** data, uint32_t* size);
};

static constexpr const v3_tuid v3_attribute_list_iid =
	V3_ID(0x1E5F0AEB, 0xCC7F4533, 0xA2544011, 0x38AD5EE4);

/**
 * message
 */

struct v3_message {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	const char* (V3_API* get_message_id)(void* self);
	void (V3_API* set_message_id)(void* self, const char* id);
	v3_attribute_list** (V3_API* get_attributes)(void* self);
};

static constexpr const v3_tuid v3_message_iid =
	V3_ID(0x936F033B, 0xC6C047DB, 0xBB0882F8, 0x13C1E613);

/**
 * connection point
 */

struct v3_connection_point {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* connect)(void* self, struct v3_connection_point** other);
	v3_result (V3_API* disconnect)(void* self, struct v3_connection_point** other);
	v3_result (V3_API* notify)(void* self, struct v3_message** message);
};

static constexpr const v3_tuid v3_connection_point_iid =
	V3_ID(0x70A4156F, 0x6E6E4026, 0x989148BF, 0xAA60D8D1);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_attribute_list_cpp : v3_funknown {
	v3_attribute_list attrlist;
};

struct v3_message_cpp : v3_funknown {
	v3_message msg;
};

struct v3_connection_point_cpp : v3_funknown {
	v3_connection_point point;
};

#endif

#include "align_pop.h"
