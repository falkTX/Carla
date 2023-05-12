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

enum v3_seek_mode {
	V3_SEEK_SET = 0,
	V3_SEEK_CUR,
	V3_SEEK_END
};

struct v3_bstream {
#ifndef __cplusplus
	struct v3_funknown;
#endif
	v3_result (V3_API *read)(void* self, void* buffer, int32_t num_bytes, int32_t* bytes_read);
	v3_result (V3_API *write)(void* self, void* buffer, int32_t num_bytes, int32_t* bytes_written);
	v3_result (V3_API *seek)(void* self, int64_t pos, int32_t seek_mode, int64_t* result);
	v3_result (V3_API *tell)(void* self, int64_t* pos);
};

static constexpr const v3_tuid v3_bstream_iid =
	V3_ID(0xC3BF6EA2, 0x30994752, 0x9B6BF990, 0x1EE33E9B);

#ifdef __cplusplus

/**
 * C++ variants
 */

struct v3_bstream_cpp : v3_funknown {
	v3_bstream stream;
};

#endif
