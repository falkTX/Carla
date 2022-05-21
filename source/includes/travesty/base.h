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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * deal with C vs C++ differences
 */

#if !defined(__cplusplus) && !defined(constexpr)
# define constexpr
#endif

/**
 * various types
 */

typedef int32_t v3_result;

typedef int16_t v3_str_128[128];
typedef uint8_t v3_bool;

typedef uint32_t v3_param_id;

/**
 * low-level ABI nonsense
 */

typedef uint8_t v3_tuid[16];

static inline
bool v3_tuid_match(const v3_tuid a, const v3_tuid b)
{
	return memcmp(a, b, sizeof(v3_tuid)) == 0;
}

#if defined(_WIN32)
# define V3_COM_COMPAT 1
# define V3_API __stdcall
#else
# define V3_COM_COMPAT 0
# define V3_API
#endif

#if V3_COM_COMPAT
enum {
	V3_NO_INTERFACE    = 0x80004002L,
	V3_OK              = 0,
	V3_TRUE            = 0,
	V3_FALSE           = 1,
	V3_INVALID_ARG     = 0x80070057L,
	V3_NOT_IMPLEMENTED = 0x80004001L,
	V3_INTERNAL_ERR    = 0x80004005L,
	V3_NOT_INITIALIZED = 0x8000FFFFL,
	V3_NOMEM           = 0x8007000EL
};

# define V3_ID(a, b, c, d) {   \
	((a) & 0x000000FF),        \
	((a) & 0x0000FF00) >>  8,  \
	((a) & 0x00FF0000) >> 16,  \
	((a) & 0xFF000000) >> 24,  \
	                           \
	((b) & 0x00FF0000) >> 16,  \
	((b) & 0xFF000000) >> 24,  \
	((b) & 0x000000FF),        \
	((b) & 0x0000FF00) >>  8,  \
	                           \
	((c) & 0xFF000000) >> 24,  \
	((c) & 0x00FF0000) >> 16,  \
	((c) & 0x0000FF00) >>  8,  \
	((c) & 0x000000FF),        \
	                           \
	((d) & 0xFF000000) >> 24,  \
	((d) & 0x00FF0000) >> 16,  \
	((d) & 0x0000FF00) >>  8,  \
	((d) & 0x000000FF),        \
}

#else // V3_COM_COMPAT
enum {
	V3_NO_INTERFACE = -1,
	V3_OK,
	V3_TRUE = V3_OK,
	V3_FALSE,
	V3_INVALID_ARG,
	V3_NOT_IMPLEMENTED,
	V3_INTERNAL_ERR,
	V3_NOT_INITIALIZED,
	V3_NOMEM
};

# define V3_ID(a, b, c, d) {   \
	((a) & 0xFF000000) >> 24,  \
	((a) & 0x00FF0000) >> 16,  \
	((a) & 0x0000FF00) >>  8,  \
	((a) & 0x000000FF),        \
	                           \
	((b) & 0xFF000000) >> 24,  \
	((b) & 0x00FF0000) >> 16,  \
	((b) & 0x0000FF00) >>  8,  \
	((b) & 0x000000FF),        \
	                           \
	((c) & 0xFF000000) >> 24,  \
	((c) & 0x00FF0000) >> 16,  \
	((c) & 0x0000FF00) >>  8,  \
	((c) & 0x000000FF),        \
	                           \
	((d) & 0xFF000000) >> 24,  \
	((d) & 0x00FF0000) >> 16,  \
	((d) & 0x0000FF00) >>  8,  \
	((d) & 0x000000FF),        \
}
#endif // V3_COM_COMPAT

#define V3_ID_COPY(iid) \
	{ iid[0], iid[1], iid[ 2], iid[ 3], iid[ 4], iid[ 5], iid[ 6], iid[ 7], \
	  iid[8], iid[9], iid[10], iid[11], iid[12], iid[13], iid[14], iid[15]  }

/**
 * funknown
 */

struct v3_funknown {
	v3_result (V3_API* query_interface)(void* self, const v3_tuid iid, void** obj);
	uint32_t (V3_API* ref)(void* self);
	uint32_t (V3_API* unref)(void* self);
};

static constexpr const v3_tuid v3_funknown_iid =
	V3_ID(0x00000000, 0x00000000, 0xC0000000, 0x00000046);

/**
 * plugin base
 */

struct v3_plugin_base {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* initialize)(void* self, struct v3_funknown** context);
	v3_result (V3_API* terminate)(void* self);
};

static constexpr const v3_tuid v3_plugin_base_iid =
	V3_ID(0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625);

#ifdef __cplusplus

/**
 * cast object into its proper C++ type.
 * this is needed because `struct v3_funknown;` on a C++ class does not inherit `v3_funknown`'s fields.
 *
 * we can use this as a little helper for keeping both C and C++ compatiblity.
 * specialized templated calls are defined where required
 * (that is, object inherits from something other than `v3_funknown`)
 *
 * example usage: `v3_cpp_obj(obj)->method(obj, args...);`
 */

template<class T> static inline
constexpr T* v3_cpp_obj(T** obj)
{
	/**
	 * this ugly piece of code is required due to C++ assuming `reinterpret_cast` by default,
	 * but we need everything to be `static_cast` for it to be `constexpr` compatible.
	 */
	return static_cast<T*>(static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*3));
}

/**
 * helper C++ functions to manually call v3_funknown methods on an object.
 */

template<class T, class M> static inline
v3_result v3_cpp_obj_query_interface(T** obj, const v3_tuid iid, M*** obj2)
{
	return static_cast<v3_funknown*>(static_cast<void*>(*obj))->query_interface(obj, iid, (void**)obj2);
}

template<class T> static inline
uint32_t v3_cpp_obj_ref(T** obj)
{
	return static_cast<v3_funknown*>(static_cast<void*>(*obj))->ref(obj);
}

template<class T> static inline
uint32_t v3_cpp_obj_unref(T** obj)
{
	return static_cast<v3_funknown*>(static_cast<void*>(*obj))->unref(obj);
}

template<class T> static inline
v3_result v3_cpp_obj_initialize(T** obj, v3_funknown** context)
{
	return static_cast<v3_plugin_base*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*3))->initialize(obj, context);
}

template<class T> static inline
v3_result v3_cpp_obj_terminate(T** obj)
{
	return static_cast<v3_plugin_base*>(
		static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(*obj)) + sizeof(void*)*3))->terminate(obj);
}

#endif
