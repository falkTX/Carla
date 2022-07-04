// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Internal utilities available to platform implementations

#ifndef PUGL_INTERNAL_H
#define PUGL_INTERNAL_H

#include "attributes.h"
#include "types.h"

#include "pugl/pugl.h"

#include <stddef.h>
#include <stdint.h>

PUGL_BEGIN_DECLS

/// Set `blob` to `data` with length `len`, reallocating if necessary
PuglStatus
puglSetBlob(PuglBlob* dest, const void* data, size_t len);

/// Reallocate and set `*dest` to `string`
void
puglSetString(char** dest, const char* string);

/// Return the Unicode code point for `buf` or the replacement character
uint32_t
puglDecodeUTF8(const uint8_t* buf);

/// Dispatch an event with a simple `type` to `view`
PuglStatus
puglDispatchSimpleEvent(PuglView* view, PuglEventType type);

/// Process configure event while already in the graphics context
PUGL_WARN_UNUSED_RESULT
PuglStatus
puglConfigure(PuglView* view, const PuglEvent* event);

/// Process expose event while already in the graphics context
PUGL_WARN_UNUSED_RESULT
PuglStatus
puglExpose(PuglView* view, const PuglEvent* event);

/// Dispatch `event` to `view`, entering graphics context if necessary
PuglStatus
puglDispatchEvent(PuglView* view, const PuglEvent* event);

PUGL_END_DECLS

#endif // PUGL_INTERNAL_H
