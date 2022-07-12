/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

/* we will include all header files used in pugl.h in their C++ friendly form, then pugl stuff in custom namespace */
#include <cstddef>
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdbool>
# include <cstdint>
#else
# include <stdbool.h>
# include <stdint.h>
#endif

// hidden api
#define PUGL_API
#define PUGL_DISABLE_DEPRECATED
#define PUGL_NO_INCLUDE_GL_H
#define PUGL_NO_INCLUDE_GLU_H

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#endif

#include "pugl-upstream/include/pugl/pugl.h"

// --------------------------------------------------------------------------------------------------------------------

// DGL specific, expose backend enter
bool puglBackendEnter(PuglView* view);

// DGL specific, expose backend leave
bool puglBackendLeave(PuglView* view);

// DGL specific, assigns backend that matches current DGL build
void puglSetMatchingBackendForCurrentBuild(PuglView* view);

// bring view window into the foreground, aka "raise" window
void puglRaiseWindow(PuglView* view);

// get scale factor from parent window if possible, fallback to puglGetScaleFactor
double puglGetScaleFactorFromParent(const PuglView* view);

// combined puglSetSizeHint using PUGL_MIN_SIZE, PUGL_MIN_ASPECT and PUGL_MAX_ASPECT
PuglStatus puglSetGeometryConstraints(PuglView* view, uint width, uint height, bool aspect);

// set view as resizable (or not) during runtime
void puglSetResizable(PuglView* view, bool resizable);

// set window size while also changing default
PuglStatus puglSetSizeAndDefault(PuglView* view, uint width, uint height);

// DGL specific, build-specific drawing prepare
void puglOnDisplayPrepare(PuglView* view);

// DGL specific, build-specific fallback resize
void puglFallbackOnResize(PuglView* view);

#if defined(DISTRHO_OS_MAC)

// macOS specific, add another view's window as child
PuglStatus puglMacOSAddChildWindow(PuglView* view, PuglView* child);

// macOS specific, remove another view's window as child
PuglStatus puglMacOSRemoveChildWindow(PuglView* view, PuglView* child);

// macOS specific, center view based on parent coordinates (if there is one)
void puglMacOSShowCentered(PuglView* view);

#elif defined(DISTRHO_OS_WASM)

// nothing here yet

#elif defined(DISTRHO_OS_WINDOWS)

// win32 specific, call ShowWindow with SW_RESTORE
void puglWin32RestoreWindow(PuglView* view);

// win32 specific, center view based on parent coordinates (if there is one)
void puglWin32ShowCentered(PuglView* view);

#elif defined(HAVE_X11)

#define DGL_USING_X11

// X11 specific, update world without triggering exposure evente
PuglStatus puglX11UpdateWithoutExposures(PuglWorld* world);

// X11 specific, set dialog window type and pid hints
void puglX11SetWindowTypeAndPID(const PuglView* view, bool isStandalone);

#endif

// --------------------------------------------------------------------------------------------------------------------

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif

#endif // DGL_PUGL_HPP_INCLUDED
