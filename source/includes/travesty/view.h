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

/**
 * base view stuff
 */

struct v3_view_rect {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

#define V3_VIEW_PLATFORM_TYPE_HWND "HWND"
#define V3_VIEW_PLATFORM_TYPE_NSVIEW "NSView"
#define V3_VIEW_PLATFORM_TYPE_X11 "X11EmbedWindowID"

#if defined(__APPLE__)
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_NSVIEW
#elif defined(_WIN32)
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_HWND
#elif !defined(__EMSCRIPTEN__)
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_X11
#endif

/**
 * plugin view
 */

struct v3_plugin_frame;

struct v3_plugin_view {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* is_platform_type_supported)(void* self, const char* platform_type);
	v3_result (V3_API* attached)(void* self, void* parent, const char* platform_type);
	v3_result (V3_API* removed)(void* self);
	v3_result (V3_API* on_wheel)(void* self, float distance);
	v3_result (V3_API* on_key_down)(void* self, int16_t key_char, int16_t key_code, int16_t modifiers);
	v3_result (V3_API* on_key_up)(void* self, int16_t key_char, int16_t key_code, int16_t modifiers);
	v3_result (V3_API* get_size)(void* self, struct v3_view_rect*);
	v3_result (V3_API* on_size)(void* self, struct v3_view_rect*);
	v3_result (V3_API* on_focus)(void* self, v3_bool state);
	v3_result (V3_API* set_frame)(void* self, struct v3_plugin_frame**);
	v3_result (V3_API* can_resize)(void* self);
	v3_result (V3_API* check_size_constraint)(void* self, struct v3_view_rect*);
};

static constexpr const v3_tuid v3_plugin_view_iid =
	V3_ID(0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29);

/**
 * plugin frame
 */

struct v3_plugin_frame {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* resize_view)(void* self, struct v3_plugin_view**, struct v3_view_rect*);
};

static constexpr const v3_tuid v3_plugin_frame_iid =
	V3_ID(0x367FAF01, 0xAFA94693, 0x8D4DA2A0, 0xED0882A3);

/**
 * steinberg content scaling support
 * (same IID/iface as presonus view scaling)
 */

struct v3_plugin_view_content_scale {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* set_content_scale_factor)(void* self, float factor);
};

static constexpr const v3_tuid v3_plugin_view_content_scale_iid =
	V3_ID(0x65ED9690, 0x8AC44525, 0x8AADEF7A, 0x72EA703F);

/**
 * support for querying the view to find what control is underneath the mouse
 */

struct v3_plugin_view_parameter_finder {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* find_parameter)(void* self, int32_t x, int32_t y, v3_param_id *);
};

static constexpr const v3_tuid v3_plugin_view_parameter_finder_iid =
	V3_ID(0x0F618302, 0x215D4587, 0xA512073C, 0x77B9D383);

/**
 * linux event handler
 */

struct v3_event_handler {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	void (V3_API* on_fd_is_set)(void* self, int fd);
};

static constexpr const v3_tuid v3_event_handler_iid =
	V3_ID(0x561E65C9, 0x13A0496F, 0x813A2C35, 0x654D7983);

/**
 * linux timer handler
 */

struct v3_timer_handler {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	void (V3_API* on_timer)(void* self);
};

static constexpr const v3_tuid v3_timer_handler_iid =
	V3_ID(0x10BDD94F, 0x41424774, 0x821FAD8F, 0xECA72CA9);

/**
 * linux host run loop
 */

struct v3_run_loop {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* register_event_handler)(void* self, v3_event_handler** handler, int fd);
	v3_result (V3_API* unregister_event_handler)(void* self, v3_event_handler** handler);
	v3_result (V3_API* register_timer)(void* self, v3_timer_handler** handler, uint64_t ms);
	v3_result (V3_API* unregister_timer)(void* self, v3_timer_handler** handler);
};

static constexpr const v3_tuid v3_run_loop_iid =
	V3_ID(0x18C35366, 0x97764F1A, 0x9C5B8385, 0x7A871389);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_plugin_view_cpp : v3_funknown {
	v3_plugin_view view;
};

struct v3_plugin_frame_cpp : v3_funknown {
	v3_plugin_frame frame;
};

struct v3_plugin_view_content_scale_cpp : v3_funknown {
	v3_plugin_view_content_scale scale;
};

struct v3_plugin_view_parameter_finder_cpp : v3_funknown {
	v3_plugin_view_parameter_finder finder;
};

struct v3_event_handler_cpp : v3_funknown {
	v3_event_handler handler;
};

struct v3_timer_handler_cpp : v3_funknown {
	v3_timer_handler timer;
};

struct v3_run_loop_cpp : v3_funknown {
	v3_run_loop loop;
};

#endif
