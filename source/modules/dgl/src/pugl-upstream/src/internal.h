// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Internal utilities available to platform implementations

#ifndef PUGL_INTERNAL_H
#define PUGL_INTERNAL_H

#include "types.h"

#include <pugl/attributes.h>
#include <pugl/pugl.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

PUGL_BEGIN_DECLS

/// Return true if `x`,`y` is a valid position
PUGL_CONST_FUNC bool
puglIsValidPosition(int x, int y);

/// Return true if `width`,`height` is a valid position
PUGL_CONST_FUNC bool
puglIsValidSize(unsigned width, unsigned height);

/// Return true if `size` is a valid view size
PUGL_CONST_FUNC bool
puglIsValidArea(PuglArea size);

/// Return the center point of some "soft" ancestor (parent window or screen)
PuglPoint
puglGetAncestorCenter(const PuglView* view);

/// Return the initial size of a view
PuglArea
puglGetInitialSize(const PuglView* view);

/// Return the initial position of a view if known, or an invalid position
PuglPoint
puglGetInitialPosition(const PuglView* view, PuglArea size);

/// Set hint to a default value if it is unset (PUGL_DONT_CARE)
void
puglEnsureHint(PuglView* view, PuglViewHint hint, int value);

/// Set `blob` to `data` with length `len`, reallocating if necessary
PuglStatus
puglSetBlob(PuglBlob* dest, const void* data, size_t len);

/// Reallocate and set `*dest` to `string`
PuglStatus
puglSetString(char** dest, const char* string);

/// Store `width` and `height` as the current value of a size `hint`
PuglStatus
puglStoreSizeHint(PuglView*    view,
                  PuglSizeHint hint,
                  unsigned     width,
                  unsigned     height);

/// Apply a change to a string property
PuglStatus
puglApplyViewString(PuglView* view, PuglStringHint key, const char* value);

/// Return the Unicode code point for `buf` or the replacement character
uint32_t
puglDecodeUTF8(const uint8_t* buf);

/// Return `state` with any flags related to `key` removed
PUGL_CONST_FUNC PuglMods
puglFilterMods(PuglMods state, PuglKey key);

/// Prepare a view to be realized by the platform implementation if possible
PuglStatus
puglPreRealize(PuglView* view);

/// Dispatch an event with a simple `type` to `view`
PuglStatus
puglDispatchSimpleEvent(PuglView* view, PuglEventType type);

/// Dispatch `event` to `view`, entering graphics context if necessary
PuglStatus
puglDispatchEvent(PuglView* view, const PuglEvent* event);

PUGL_END_DECLS

#endif // PUGL_INTERNAL_H
