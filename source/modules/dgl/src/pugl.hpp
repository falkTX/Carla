/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_PUGL_HPP_INCLUDED
#define DGL_PUGL_HPP_INCLUDED

#include "../Base.hpp"

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cstddef>
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdbool>
# include <cstdint>
#else
# include <stdbool.h>
# include <stdint.h>
#endif

#define PUGL_API
#define PUGL_DISABLE_DEPRECATED
#define PUGL_NO_INCLUDE_GLU_H

// --------------------------------------------------------------------------------------------------------------------

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#else
USE_NAMESPACE_DGL
#endif

#include "pugl-upstream/include/pugl/pugl.h"

// --------------------------------------------------------------------------------------------------------------------

PUGL_BEGIN_DECLS

// expose backend enter
PUGL_API bool
puglBackendEnter(PuglView* view);

// expose backend leave
PUGL_API void
puglBackendLeave(PuglView* view);

// clear minimum size to 0
PUGL_API void
puglClearMinSize(PuglView* view);

// missing in pugl, directly returns transient parent
PUGL_API PuglNativeView
puglGetTransientParent(const PuglView* view);

// missing in pugl, directly returns title char* pointer
PUGL_API const char*
puglGetWindowTitle(const PuglView* view);

// get global scale factor
PUGL_API double
puglGetDesktopScaleFactor(const PuglView* view);

// bring view window into the foreground, aka "raise" window
PUGL_API void
puglRaiseWindow(PuglView* view);

// DGL specific, assigns backend that matches current DGL build
PUGL_API void
puglSetMatchingBackendForCurrentBuild(PuglView* view);

// Combine puglSetMinSize and puglSetAspectRatio
PUGL_API PuglStatus
puglSetGeometryConstraints(PuglView* view, uint width, uint height, bool aspect);

// set window offset without changing size
PUGL_API PuglStatus
puglSetWindowOffset(PuglView* view, int x, int y);

// set window size with default size and without changing frame x/y position
PUGL_API PuglStatus
puglSetWindowSize(PuglView* view, uint width, uint height);

// DGL specific, build-specific drawing prepare
PUGL_API void
puglOnDisplayPrepare(PuglView* view);

// DGL specific, build-specific fallback resize
PUGL_API void
puglFallbackOnResize(PuglView* view);

#if defined(DISTRHO_OS_MAC)

// macOS specific, allow standalone window to gain focus
PUGL_API void
puglMacOSActivateApp();

// macOS specific, add another view's window as child
PUGL_API PuglStatus
puglMacOSAddChildWindow(PuglView* view, PuglView* child);

// macOS specific, remove another view's window as child
PUGL_API PuglStatus
puglMacOSRemoveChildWindow(PuglView* view, PuglView* child);

// macOS specific, center view based on parent coordinates (if there is one)
PUGL_API void
puglMacOSShowCentered(PuglView* view);

#elif defined(DISTRHO_OS_WINDOWS)

// win32 specific, call ShowWindow with SW_RESTORE
PUGL_API void
puglWin32RestoreWindow(PuglView* view);

// win32 specific, center view based on parent coordinates (if there is one)
PUGL_API void
puglWin32ShowCentered(PuglView* view);

// win32 specific, set or unset WS_SIZEBOX style flag
PUGL_API void
puglWin32SetWindowResizable(PuglView* view, bool resizable);

#elif defined(HAVE_X11)

// X11 specific, safer way to grab focus
PUGL_API PuglStatus
puglX11GrabFocus(const PuglView* view);

// X11 specific, set dialog window type and pid hints
PUGL_API void
puglX11SetWindowTypeAndPID(const PuglView* view, bool isStandalone);

#endif

PUGL_END_DECLS

// --------------------------------------------------------------------------------------------------------------------

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif

#endif // DGL_PUGL_HPP_INCLUDED
