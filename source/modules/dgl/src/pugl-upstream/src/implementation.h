/*
  Copyright 2012-2020 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef PUGL_IMPLEMENTATION_H
#define PUGL_IMPLEMENTATION_H

#include "types.h"

#include "pugl/pugl.h"

#include <stddef.h>
#include <stdint.h>

PUGL_BEGIN_DECLS

/// Set `blob` to `data` with length `len`, reallocating if necessary
void
puglSetBlob(PuglBlob* dest, const void* data, size_t len);

/// Reallocate and set `*dest` to `string`
void
puglSetString(char** dest, const char* string);

/// Allocate and initialise world internals (implemented once per platform)
PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags flags);

/// Destroy and free world internals (implemented once per platform)
void
puglFreeWorldInternals(PuglWorld* world);

/// Allocate and initialise view internals (implemented once per platform)
PuglInternals*
puglInitViewInternals(void);

/// Destroy and free view internals (implemented once per platform)
void
puglFreeViewInternals(PuglView* view);

/// Return the Unicode code point for `buf` or the replacement character
uint32_t
puglDecodeUTF8(const uint8_t* buf);

/// Dispatch an event with a simple `type` to `view`
void
puglDispatchSimpleEvent(PuglView* view, PuglEventType type);

/// Process configure event while already in the graphics context
void
puglConfigure(PuglView* view, const PuglEvent* event);

/// Process expose event while already in the graphics context
void
puglExpose(PuglView* view, const PuglEvent* event);

/// Dispatch `event` to `view`, entering graphics context if necessary
void
puglDispatchEvent(PuglView* view, const PuglEvent* event);

/// Set internal (stored in view) clipboard contents
const void*
puglGetInternalClipboard(const PuglView* view, const char** type, size_t* len);

/// Set internal (stored in view) clipboard contents
PuglStatus
puglSetInternalClipboard(PuglView*   view,
                         const char* type,
                         const void* data,
                         size_t      len);

PUGL_END_DECLS

#endif // PUGL_IMPLEMENTATION_H
