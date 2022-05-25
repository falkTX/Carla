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

#include "message.h"

#include "align_push.h"

/**
 * host application
 */

struct v3_host_application {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API* get_name)(void* self, v3_str_128 name); // wtf?
	v3_result (V3_API* create_instance)(void* self, v3_tuid cid, v3_tuid iid, void** obj);
};

static constexpr const v3_tuid v3_host_application_iid =
	V3_ID(0x58E595CC, 0xDB2D4969, 0x8B6AAF8C, 0x36A664E5);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_host_application_cpp : v3_funknown {
    v3_host_application app;
};

#endif

#include "align_pop.h"
