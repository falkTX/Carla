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

#include "pugl.hpp"

// --------------------------------------------------------------------------------------------------------------------
// include base headers

#ifdef DGL_CAIRO
# include <cairo.h>
#endif
#ifdef DGL_OPENGL
# include "../OpenGL-include.hpp"
#endif
#ifdef DGL_VULKAN
# include <vulkan/vulkan_core.h>
#endif

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#if defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
# include <dlfcn.h>
# include <mach/mach_time.h>
# ifdef DGL_CAIRO
#  include <cairo-quartz.h>
# endif
# ifdef DGL_OPENGL
#  include <OpenGL/gl.h>
# endif
# ifdef DGL_VULKAN
#  import <QuartzCore/CAMetalLayer.h>
#  include <vulkan/vulkan_macos.h>
# endif
#elif defined(DISTRHO_OS_WASM)
# include <emscripten/emscripten.h>
# include <emscripten/html5.h>
# ifdef DGL_OPENGL
#  include <EGL/egl.h>
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include <wctype.h>
# include <winsock2.h>
# include <windows.h>
# include <windowsx.h>
# ifdef DGL_CAIRO
#  include <cairo-win32.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/gl.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan.h>
#  include <vulkan/vulkan_win32.h>
# endif
#elif defined(HAVE_X11)
# include <dlfcn.h>
# include <limits.h>
# include <unistd.h>
# include <sys/select.h>
// # include <sys/time.h>
# include <X11/X.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xresource.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
// #  include <X11/cursorfont.h>
# endif
# ifdef HAVE_XRANDR
#  include <X11/extensions/Xrandr.h>
# endif
# ifdef HAVE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
# endif
# ifdef DGL_CAIRO
#  include <cairo-xlib.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/glx.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan_xlib.h>
# endif
#endif

#ifndef DGL_FILE_BROWSER_DISABLED
# define FILE_BROWSER_DIALOG_DGL_NAMESPACE
# define FILE_BROWSER_DIALOG_NAMESPACE DGL_NAMESPACE
# define DGL_FILE_BROWSER_DIALOG_HPP_INCLUDED
START_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.cpp"
#endif

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#endif

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_MAC)
# ifndef DISTRHO_MACOS_NAMESPACE_MACRO
#  define DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, SEP, INTERFACE) NS ## SEP ## INTERFACE
#  define DISTRHO_MACOS_NAMESPACE_MACRO(NS, INTERFACE) DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, _, INTERFACE)
#  define PuglCairoView      DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglCairoView)
#  define PuglOpenGLView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglOpenGLView)
#  define PuglStubView       DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglStubView)
#  define PuglVulkanView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglVulkanView)
#  define PuglWindow         DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglWindow)
#  define PuglWindowDelegate DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglWindowDelegate)
#  define PuglWrapperView    DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglWrapperView)
# endif
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
# import "pugl-upstream/src/mac.m"
# import "pugl-upstream/src/mac_stub.m"
# ifdef DGL_CAIRO
#  import "pugl-upstream/src/mac_cairo.m"
# endif
# ifdef DGL_OPENGL
#  import "pugl-upstream/src/mac_gl.m"
# endif
# ifdef DGL_VULKAN
#  import "pugl-upstream/src/mac_vulkan.m"
# endif
# pragma clang diagnostic pop
#elif defined(DISTRHO_OS_WASM)
# include "pugl-upstream/src/wasm.c"
# include "pugl-upstream/src/wasm_stub.c"
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/wasm_gl.c"
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include "pugl-upstream/src/win.c"
# include "pugl-upstream/src/win_stub.c"
# ifdef DGL_CAIRO
#  include "pugl-upstream/src/win_cairo.c"
# endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/win_gl.c"
# endif
# ifdef DGL_VULKAN
#  include "pugl-upstream/src/win_vulkan.c"
# endif
#elif defined(HAVE_X11)
# include "pugl-upstream/src/x11.c"
# include "pugl-upstream/src/x11_stub.c"
# ifdef DGL_CAIRO
#  include "pugl-upstream/src/x11_cairo.c"
# endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/x11_gl.c"
# endif
# ifdef DGL_VULKAN
#  include "pugl-upstream/src/x11_vulkan.c"
# endif
#endif

#include "pugl-upstream/src/common.c"
#include "pugl-upstream/src/internal.c"

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, expose backend enter

bool puglBackendEnter(PuglView* const view)
{
    return view->backend->enter(view, nullptr) == PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, expose backend leave

bool puglBackendLeave(PuglView* const view)
{
    return view->backend->leave(view, nullptr) == PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, assigns backend that matches current DGL build

void puglSetMatchingBackendForCurrentBuild(PuglView* const view)
{
#ifdef DGL_CAIRO
    puglSetBackend(view, puglCairoBackend());
#endif
#ifdef DGL_OPENGL
    puglSetBackend(view, puglGlBackend());
#endif
#ifdef DGL_VULKAN
    puglSetBackend(view, puglVulkanBackend());
#endif
    if (view->backend == nullptr)
        puglSetBackend(view, puglStubBackend());
}

// --------------------------------------------------------------------------------------------------------------------
// bring view window into the foreground, aka "raise" window

void puglRaiseWindow(PuglView* const view)
{
#if defined(DISTRHO_OS_MAC)
    if (NSWindow* const window = view->impl->window ? view->impl->window
                                                    : [view->impl->wrapperView window])
        [window orderFrontRegardless];
#elif defined(DISTRHO_OS_WASM)
    // nothing
#elif defined(DISTRHO_OS_WINDOWS)
    SetForegroundWindow(view->impl->hwnd);
    SetActiveWindow(view->impl->hwnd);
#elif defined(HAVE_X11)
    XRaiseWindow(view->world->impl->display, view->impl->win);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// get scale factor from parent window if possible, fallback to puglGetScaleFactor

double puglGetScaleFactorFromParent(const PuglView* const view)
{
    const PuglNativeView parent = view->parent ? view->parent : view->transientParent ? view->transientParent : 0;
#if defined(DISTRHO_OS_MAC)
    // some of these can return 0 as backingScaleFactor, pick the most relevant valid one
    const NSWindow* possibleWindows[] = {
        parent != 0 ? [(NSView*)parent window] : nullptr,
        view->impl->window,
        [view->impl->wrapperView window]
    };
    for (size_t i=0; i<ARRAY_SIZE(possibleWindows); ++i)
    {
        if (possibleWindows[i] == nullptr)
            continue;
        if (const double scaleFactor = [[possibleWindows[i] screen] backingScaleFactor])
            return scaleFactor;
    }
    return [[NSScreen mainScreen] backingScaleFactor];
#elif defined(DISTRHO_OS_WINDOWS)
    const HWND hwnd = parent != 0 ? (HWND)parent : view->impl->hwnd;
    return puglWinGetViewScaleFactor(hwnd);
#else
    return puglGetScaleFactor(view);
    // unused
    (void)parent;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// Combined puglSetSizeHint using PUGL_MIN_SIZE and PUGL_FIXED_ASPECT

PuglStatus puglSetGeometryConstraints(PuglView* const view, const uint width, const uint height, const bool aspect)
{
    view->sizeHints[PUGL_MIN_SIZE].width = width;
    view->sizeHints[PUGL_MIN_SIZE].height = height;

    if (aspect)
    {
        view->sizeHints[PUGL_FIXED_ASPECT].width = width;
        view->sizeHints[PUGL_FIXED_ASPECT].height = height;
    }

#if defined(DISTRHO_OS_MAC)
    if (view->impl->window)
    {
        PuglStatus status;

        if ((status = updateSizeHint(view, PUGL_MIN_SIZE)) != PUGL_SUCCESS)
            return status;

        if (aspect && (status = updateSizeHint(view, PUGL_FIXED_ASPECT)) != PUGL_SUCCESS)
            return status;
    }
#elif defined(DISTRHO_OS_WASM)
    // nothing
#elif defined(DISTRHO_OS_WINDOWS)
    // nothing
#elif defined(HAVE_X11)
    if (const PuglStatus status = updateSizeHints(view))
        return status;

    XFlush(view->world->impl->display);
#endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set view as resizable (or not) during runtime

void puglSetResizable(PuglView* const view, const bool resizable)
{
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);

#if defined(DISTRHO_OS_MAC)
    if (PuglWindow* const window = view->impl->window)
    {
        const uint style = (NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask)
                         | (resizable ? NSResizableWindowMask : 0x0);
        [window setStyleMask:style];
    }
    // FIXME use [view setAutoresizingMask:NSViewNotSizable] ?
#elif defined(DISTRHO_OS_WASM)
    // nothing
#elif defined(DISTRHO_OS_WINDOWS)
    if (const HWND hwnd = view->impl->hwnd)
    {
        const uint winFlags = resizable ? GetWindowLong(hwnd, GWL_STYLE) |  (WS_SIZEBOX | WS_MAXIMIZEBOX)
                                        : GetWindowLong(hwnd, GWL_STYLE) & ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
        SetWindowLong(hwnd, GWL_STYLE, winFlags);
    }
#elif defined(HAVE_X11)
    updateSizeHints(view);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// set window size while also changing default

PuglStatus puglSetSizeAndDefault(PuglView* view, uint width, uint height)
{
    if (width > INT16_MAX || height > INT16_MAX)
        return PUGL_BAD_PARAMETER;

    view->sizeHints[PUGL_DEFAULT_SIZE].width = view->frame.width = static_cast<PuglSpan>(width);
    view->sizeHints[PUGL_DEFAULT_SIZE].height = view->frame.height = static_cast<PuglSpan>(height);

#if defined(DISTRHO_OS_MAC)
    // mostly matches upstream pugl, simplified
    PuglInternals* const impl = view->impl;

    const PuglRect frame = view->frame;
    const NSRect framePx = rectToNsRect(frame);
    const NSRect framePt = nsRectToPoints(view, framePx);

    if (PuglWindow* const window = view->impl->window)
    {
        const NSRect screenPt = rectToScreen(viewScreen(view), framePt);
        const NSRect winFrame = [window frameRectForContentRect:screenPt];
        [window setFrame:winFrame display:NO];
    }

    const NSSize sizePx = NSMakeSize(frame.width, frame.height);
    const NSSize sizePt = [impl->drawView convertSizeFromBacking:sizePx];
    [impl->wrapperView setFrameSize:sizePt];
    [impl->drawView setFrameSize:sizePt];
#elif defined(DISTRHO_OS_WASM)
    d_stdout("className is %s", view->world->className);
    emscripten_set_canvas_element_size(view->world->className, width, height);
#elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we re-enter context after resize
    if (const HWND hwnd = view->impl->hwnd)
    {
        const RECT rect = adjustedWindowRect(view, view->frame.x, view->frame.y,
                                             static_cast<long>(width), static_cast<long>(height));

        if (!SetWindowPos(hwnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                          SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE))
            return PUGL_UNKNOWN_ERROR;

        // make sure to return context back to ourselves
        puglBackendEnter(view);
    }
#elif defined(HAVE_X11)
    // matches upstream pugl, all in one
    if (const Window window = view->impl->win)
    {
        Display* const display = view->world->impl->display;

        if (! XResizeWindow(display, window, width, height))
            return PUGL_UNKNOWN_ERROR;

        if (const PuglStatus status = updateSizeHints(view))
            return status;

        XFlush(display);
    }
#endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific drawing prepare

void puglOnDisplayPrepare(PuglView*)
{
#ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
# ifndef DGL_USE_GLES
    glLoadIdentity();
# endif
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback resize

void puglFallbackOnResize(PuglView* const view)
{
#ifdef DGL_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
# ifndef DGL_USE_GLES
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(view->frame.width), static_cast<GLdouble>(view->frame.height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(view->frame.width), static_cast<GLsizei>(view->frame.height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
# else
    glViewport(0, 0, static_cast<GLsizei>(view->frame.width), static_cast<GLsizei>(view->frame.height));
# endif
#else
    return;
    // unused
    (void)view;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_MAC)

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, add another view's window as child

PuglStatus
puglMacOSAddChildWindow(PuglView* const view, PuglView* const child)
{
    if (NSWindow* const viewWindow = view->impl->window ? view->impl->window
                                                        : [view->impl->wrapperView window])
    {
        if (NSWindow* const childWindow = child->impl->window ? child->impl->window
                                                              : [child->impl->wrapperView window])
        {
            [viewWindow addChildWindow:childWindow ordered:NSWindowAbove];
            return PUGL_SUCCESS;
        }
    }

    return PUGL_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, remove another view's window as child

PuglStatus
puglMacOSRemoveChildWindow(PuglView* const view, PuglView* const child)
{
    if (NSWindow* const viewWindow = view->impl->window ? view->impl->window
                                                        : [view->impl->wrapperView window])
    {
        if (NSWindow* const childWindow = child->impl->window ? child->impl->window
                                                              : [child->impl->wrapperView window])
        {
            [viewWindow removeChildWindow:childWindow];
            return PUGL_SUCCESS;
        }
    }

    return PUGL_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, center view based on parent coordinates (if there is one)

void puglMacOSShowCentered(PuglView* const view)
{
    if (puglShow(view) != PUGL_SUCCESS)
        return;

    if (view->transientParent != 0)
    {
        NSWindow* const transientWindow = [(NSView*)view->transientParent window];
        DISTRHO_SAFE_ASSERT_RETURN(transientWindow != nullptr,);

        const NSRect ourFrame       = [view->impl->window frame];
        const NSRect transientFrame = [transientWindow frame];

        const int x = transientFrame.origin.x + (transientFrame.size.width - ourFrame.size.width) / 2;
        const int y = transientFrame.origin.y + (transientFrame.size.height - ourFrame.size.height) / 2;

        [view->impl->window setFrameTopLeftPoint:NSMakePoint(x, y)];
    }
    else
    {
        [view->impl->window center];
    }
}

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_WINDOWS)

// --------------------------------------------------------------------------------------------------------------------
// win32 specific, call ShowWindow with SW_RESTORE

void puglWin32RestoreWindow(PuglView* const view)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    ShowWindow(impl->hwnd, SW_RESTORE);
    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------
// win32 specific, center view based on parent coordinates (if there is one)

void puglWin32ShowCentered(PuglView* const view)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    RECT rectChild, rectParent;

    if (view->transientParent != 0 &&
        GetWindowRect(impl->hwnd, &rectChild) &&
        GetWindowRect((HWND)view->transientParent, &rectParent))
    {
        SetWindowPos(impl->hwnd, (HWND)view->transientParent,
                     rectParent.left + (rectChild.right-rectChild.left)/2,
                     rectParent.top + (rectChild.bottom-rectChild.top)/2,
                     0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
    }
    else
    {
#ifdef DGL_WINDOWS_ICON_ID
        WNDCLASSEX wClass;
        std::memset(&wClass, 0, sizeof(wClass));

        const HINSTANCE hInstance = GetModuleHandle(nullptr);

        if (GetClassInfoEx(hInstance, view->world->className, &wClass))
            wClass.hIcon = LoadIcon(nullptr, MAKEINTRESOURCE(DGL_WINDOWS_ICON_ID));

        SetClassLongPtr(impl->hwnd, GCLP_HICON, (LONG_PTR) LoadIcon(hInstance, MAKEINTRESOURCE(DGL_WINDOWS_ICON_ID)));
#endif

        MONITORINFO mInfo;
        std::memset(&mInfo, 0, sizeof(mInfo));
        mInfo.cbSize = sizeof(mInfo);

        if (GetMonitorInfo(MonitorFromWindow(impl->hwnd, MONITOR_DEFAULTTOPRIMARY), &mInfo))
            SetWindowPos(impl->hwnd,
                         HWND_TOP,
                         mInfo.rcWork.left + (mInfo.rcWork.right - mInfo.rcWork.left - view->frame.width) / 2,
                         mInfo.rcWork.top + (mInfo.rcWork.bottom - mInfo.rcWork.top - view->frame.height) / 2,
                         0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
        else
            ShowWindow(impl->hwnd, SW_NORMAL);
    }

    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_WASM)

// nothing here yet

// --------------------------------------------------------------------------------------------------------------------

#elif defined(HAVE_X11)

PuglStatus puglX11UpdateWithoutExposures(PuglWorld* const world)
{
    const bool wasDispatchingEvents = world->impl->dispatchingEvents;
    world->impl->dispatchingEvents = true;
    PuglStatus st = PUGL_SUCCESS;

    const double startTime = puglGetTime(world);
    const double endTime  = startTime + 0.03;

    for (double t = startTime; !st && t < endTime; t = puglGetTime(world))
    {
        pollX11Socket(world, endTime - t);
        st = dispatchX11Events(world);
    }

    world->impl->dispatchingEvents = wasDispatchingEvents;
    return st;
}

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, set dialog window type and pid hints

void puglX11SetWindowTypeAndPID(const PuglView* const view, const bool isStandalone)
{
    const PuglInternals* const impl    = view->impl;
    Display*             const display = view->world->impl->display;

    const pid_t pid = getpid();
    const Atom _nwp = XInternAtom(display, "_NET_WM_PID", False);
    XChangeProperty(display, impl->win, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

    const Atom _wt = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

    Atom _wts[2];
    int numAtoms = 0;

    if (! isStandalone)
        _wts[numAtoms++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    _wts[numAtoms++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    XChangeProperty(display, impl->win, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, numAtoms);
}

// --------------------------------------------------------------------------------------------------------------------

#endif // HAVE_X11

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif
