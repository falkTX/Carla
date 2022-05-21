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

/**
 * plugin factory v1
 */

struct v3_factory_info {
	char vendor[64];
	char url[256];
	char email[128];
	int32_t flags; // set to 0x10 (unicode)
};

struct v3_class_info {
	v3_tuid class_id;
	int32_t cardinality; // set to 0x7FFFFFFF (many instances)
	char category[32];
	char name[64];
};

struct v3_plugin_factory {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API *get_factory_info)(void* self, struct v3_factory_info*);
	int32_t (V3_API *num_classes)(void* self);
	v3_result (V3_API *get_class_info)(void* self, int32_t idx, struct v3_class_info*);
	v3_result (V3_API *create_instance)(void* self, const v3_tuid class_id, const v3_tuid iid, void** instance);
};

static constexpr const v3_tuid v3_plugin_factory_iid =
	V3_ID(0x7A4D811C, 0x52114A1F, 0xAED9D2EE, 0x0B43BF9F);

/**
 * plugin factory v2
 */

enum {
	V3_DISTRIBUTABLE = 1 << 0,
	V3_SIMPLE_MODE   = 1 << 1
};

struct v3_class_info_2 {
	v3_tuid class_id;
	int32_t cardinality; // set to 0x7FFFFFFF
	char category[32];
	char name[64];
	uint32_t class_flags;
	char sub_categories[128];
	char vendor[64];
	char version[64];
	char sdk_version[64];
};

struct v3_plugin_factory_2 {
#ifndef __cplusplus
	struct v3_plugin_factory;
#endif
	v3_result (V3_API *get_class_info_2)(void* self, int32_t idx, struct v3_class_info_2*);
};

static constexpr const v3_tuid v3_plugin_factory_2_iid =
	V3_ID(0x0007B650, 0xF24B4C0B, 0xA464EDB9, 0xF00B2ABB);

/**
 * plugin factory v3
 * (we got it right this time I swear)
 *
 * same as above, but "unicode" (really just utf-16, thanks microsoft!)
 */

struct v3_class_info_3 {
	v3_tuid class_id;
	int32_t cardinality; // set to 0x7FFFFFFF
	char category[32];
	int16_t name[64];
	uint32_t class_flags;
	char sub_categories[128];
	int16_t vendor[64];
	int16_t version[64];
	int16_t sdk_version[64];
};

struct v3_plugin_factory_3 {
#ifndef __cplusplus
	struct v3_plugin_factory_2;
#endif
	v3_result (V3_API *get_class_info_utf16)(void* self, int32_t idx, struct v3_class_info_3*);
	v3_result (V3_API *set_host_context)(void* self, struct v3_funknown** host);
};

static constexpr const v3_tuid v3_plugin_factory_3_iid =
	V3_ID(0x4555A2AB, 0xC1234E57, 0x9B122910, 0x36878931);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_plugin_factory_cpp : v3_funknown {
	v3_plugin_factory   v1;
	v3_plugin_factory_2 v2;
	v3_plugin_factory_3 v3;
};

template<> inline
constexpr v3_plugin_factory_2* v3_cpp_obj(v3_plugin_factory_2** obj)
{
	/**
	 * this ugly piece of code is required due to C++ assuming `reinterpret_cast` by default,
	 * but we need everything to be `static_cast` for it to be `constexpr` compatible.
	 */
	return static_cast<v3_plugin_factory_2*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*7));
}

template<> inline
constexpr v3_plugin_factory_3* v3_cpp_obj(v3_plugin_factory_3** obj)
{
	/**
	 * this ugly piece of code is required due to C++ assuming `reinterpret_cast` by default,
	 * but we need everything to be `static_cast` for it to be `constexpr` compatible.
	 */
	return static_cast<v3_plugin_factory_3*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*8));
}

#endif
