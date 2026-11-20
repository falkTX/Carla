// Copyright 2012-2025 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// The internal API that a platform implementation must define

#ifndef PUGL_PLATFORM_H
#define PUGL_PLATFORM_H

#include "types.h"

#include <pugl/attributes.h>
#include <pugl/pugl.h>

PUGL_BEGIN_DECLS

/// Allocate and initialise world internals (implemented once per platform)
PUGL_MALLOC_FUNC PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags flags);

/// Destroy and free world internals (implemented once per platform)
void
puglFreeWorldInternals(PuglWorld* world);

/// Allocate and initialise view internals (implemented once per platform)
PUGL_MALLOC_FUNC PuglInternals*
puglInitViewInternals(PuglWorld* world);

/// Destroy and free view internals (implemented once per platform)
void
puglFreeViewInternals(PuglView* view);

/// Adapt to the change of a size hint if necessary
PuglStatus
puglApplySizeHint(PuglView* view, PuglSizeHint hint);

/// Set the current position of a view window
PuglStatus
puglSetWindowPosition(PuglView* view, int x, int y);

/// Set the current size of a view window
PuglStatus
puglSetWindowSize(PuglView* view, unsigned width, unsigned height);

PUGL_END_DECLS

#endif // PUGL_PLATFORM_H
