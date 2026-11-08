/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#if defined(DISTRHO_OS_HAIKU)
# include <Application.h>
# include <Window.h>
# ifdef DGL_OPENGL
#  include <GL/gl.h>
#  include <opengl/GLView.h>
# endif
#elif defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
# include <dlfcn.h>
# include <mach/mach_time.h>
# ifdef DGL_CAIRO
#  include <cairo-quartz.h>
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

#ifdef DGL_USE_FILE_BROWSER
# define DGL_FILE_BROWSER_DIALOG_HPP_INCLUDED
# define FILE_BROWSER_DIALOG_DGL_NAMESPACE
# define FILE_BROWSER_DIALOG_NAMESPACE DGL_NAMESPACE
START_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.cpp"
#endif

#ifdef DGL_USE_WEB_VIEW
# define DGL_WEB_VIEW_HPP_INCLUDED
# define WEB_VIEW_NAMESPACE DGL_NAMESPACE
# define WEB_VIEW_DGL_NAMESPACE
START_NAMESPACE_DGL
# include "../../distrho/extra/WebViewImpl.hpp"
END_NAMESPACE_DGL
# include "../../distrho/extra/WebViewImpl.cpp"
#endif

#if defined(DGL_USING_X11) && defined(DGL_X11_WINDOW_ICON_NAME)
extern const ulong* DGL_X11_WINDOW_ICON_NAME;
#endif

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#endif

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_HAIKU)
# include "pugl-extra/haiku.cpp"
# include "pugl-extra/haiku_stub.cpp"
# ifdef DGL_OPENGL
#  include "pugl-extra/haiku_gl.cpp"
# endif
#elif defined(DISTRHO_OS_MAC)
# ifndef DISTRHO_MACOS_NAMESPACE_MACRO
#  ifndef DISTRHO_MACOS_NAMESPACE_TIME
#   define DISTRHO_MACOS_NAMESPACE_TIME __apple_build_version__
#  endif
#  define DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, SEP, TIME, INTERFACE) NS ## SEP ## TIME ## SEP ## INTERFACE
#  define DISTRHO_MACOS_NAMESPACE_MACRO(NS, TIME, INTERFACE) DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, _, TIME, INTERFACE)
#  define PuglCairoView      DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglCairoView)
#  define PuglOpenGLView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglOpenGLView)
#  define PuglStubView       DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglStubView)
#  define PuglVulkanView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglVulkanView)
#  define PuglWindow         DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWindow)
#  define PuglWindowDelegate DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWindowDelegate)
#  define PuglWrapperView    DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWrapperView)
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
# include "pugl-extra/wasm.c"
# include "pugl-extra/wasm_stub.c"
# ifdef DGL_OPENGL
#  include "pugl-extra/wasm_gl.c"
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
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
# endif
# include "pugl-upstream/src/x11.c"
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
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

    if (view->backend != nullptr)
    {
       #if defined(DGL_USE_GLES2)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_ES_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 2);
       #elif defined(DGL_USE_GLES3)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_ES_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
       #elif defined(DGL_USE_OPENGL3)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
       #elif defined(DGL_OPENGL)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_COMPATIBILITY_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 2);
       #endif
    }
    else
    {
        puglSetBackend(view, puglStubBackend());
    }
}

// --------------------------------------------------------------------------------------------------------------------
// bring view window into the foreground, aka "raise" window

void puglRaiseWindow(PuglView* const view)
{
    // this does the same as puglShow(view, PUGL_SHOW_FORCE_RAISE) + puglShow(view, PUGL_SHOW_RAISE)
   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    NSWindow* const window = [view->impl->wrapperView window];
    [window orderFrontRegardless];
    [window orderFront:view->impl->wrapperView];
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
// Combined puglSetSizeHint using PUGL_MIN_SIZE and PUGL_FIXED_ASPECT

PuglStatus puglSetGeometryConstraints(PuglView* const view, const uint width, const uint height, const bool aspect)
{
    view->sizeHints[PUGL_MIN_SIZE].width = static_cast<PuglSpan>(width);
    view->sizeHints[PUGL_MIN_SIZE].height = static_cast<PuglSpan>(height);

    if (aspect)
    {
        view->sizeHints[PUGL_FIXED_ASPECT].width = static_cast<PuglSpan>(width);
        view->sizeHints[PUGL_FIXED_ASPECT].height = static_cast<PuglSpan>(height);
    }
    else
    {
        view->sizeHints[PUGL_FIXED_ASPECT].width = 0;
        view->sizeHints[PUGL_FIXED_ASPECT].height = 0;
    }

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    if (view->impl->window)
    {
        if (const PuglStatus status = puglUpdateSizeHints(view))
            return status;
    }
   #elif defined(DISTRHO_OS_WASM)
    const char* const className = view->world->strings[PUGL_CLASS_NAME];
    EM_ASM({
      var canvasWrapper = document.getElementById(UTF8ToString($0)).parentElement;
      canvasWrapper.style.setProperty("min-width", parseInt($1 / window.devicePixelRatio) + 'px');
      canvasWrapper.style.setProperty("min-height", parseInt($2 / window.devicePixelRatio) + 'px');
    }, className, width, height);
   #elif defined(DISTRHO_OS_WINDOWS)
    // nothing
   #elif defined(HAVE_X11)
    if (view->impl->win)
    {
        if (const PuglStatus status = puglUpdateSizeHints(view))
            return status;

        XFlush(view->world->impl->display);
    }
   #endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set view as resizable (or not) during runtime

void puglSetResizable(PuglView* const view, const bool resizable)
{
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    if (PuglWindow* const window = view->impl->window)
    {
        const uint style = (NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask)
                         | (resizable ? NSResizableWindowMask : 0);
        [window setStyleMask:style];
    }
    // FIXME use [view setAutoresizingMask:NSViewNotSizable] ?
   #elif defined(DISTRHO_OS_WASM)
    puglUpdateSizeHints(view);
   #elif defined(DISTRHO_OS_WINDOWS)
    if (const HWND hwnd = view->impl->hwnd)
    {
        const uint winFlags = resizable ? GetWindowLong(hwnd, GWL_STYLE) |  (WS_SIZEBOX | WS_MAXIMIZEBOX)
                                        : GetWindowLong(hwnd, GWL_STYLE) & ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
        SetWindowLong(hwnd, GWL_STYLE, winFlags);
    }
   #elif defined(HAVE_X11)
    puglUpdateSizeHints(view);
   #endif
}

// --------------------------------------------------------------------------------------------------------------------
// set window size while also changing default

PuglStatus puglSetSizeAndDefault(PuglView* const view, const uint width, const uint height)
{
    // set default size first
    view->sizeHints[PUGL_DEFAULT_SIZE].width = view->sizeHints[PUGL_CURRENT_SIZE].width = width;
    view->sizeHints[PUGL_DEFAULT_SIZE].height = view->sizeHints[PUGL_CURRENT_SIZE].height = height;

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    // matches upstream pugl
    if (view->impl->wrapperView)
    {
        // nothing to do for PUGL_DEFAULT_SIZE hint

        if (const PuglStatus status = puglSetWindowSize(view, width, height))
            return status;
    }
   #elif defined(DISTRHO_OS_WASM)
    if (const PuglStatus status = puglUpdateSizeHints(view))
        return status;

    emscripten_set_canvas_element_size(view->world->strings[PUGL_CLASS_NAME], width, height);
   #elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we re-enter context after resize
    if (view->impl->hwnd)
    {
        // nothing to do for PUGL_DEFAULT_SIZE hint

        if (const PuglStatus status = puglSetWindowSize(view, width, height))
            return status;

        // make sure to return context back to ourselves
        puglBackendEnter(view);
    }
   #elif defined(HAVE_X11)
    // matches upstream pugl, adds flush at the end
    if (view->impl->win)
    {
        if (const PuglStatus status = puglUpdateSizeHints(view))
            return status;

        if (const PuglStatus status = puglSetWindowSize(view, width, height))
            return status;

        // flush size changes
        XFlush(view->world->impl->display);
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
   #ifndef DGL_USE_OPENGL3
    glLoadIdentity();
   #endif
  #endif
}

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_HAIKU)

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_MAC)

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
    if (puglShow(view, PUGL_SHOW_RAISE) != PUGL_SUCCESS)
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
        SetWindowPos(impl->hwnd, HWND_TOP,
                     rectParent.left + (rectParent.right-rectParent.left)/2 - (rectChild.right-rectChild.left)/2,
                     rectParent.top + (rectParent.bottom-rectParent.top)/2 - (rectChild.bottom-rectChild.top)/2,
                     0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
    }
    else
    {
        MONITORINFO mInfo;
        std::memset(&mInfo, 0, sizeof(mInfo));
        mInfo.cbSize = sizeof(mInfo);

        if (GetMonitorInfo(MonitorFromWindow(impl->hwnd, MONITOR_DEFAULTTOPRIMARY), &mInfo))
            SetWindowPos(impl->hwnd, HWND_TOP,
                         mInfo.rcWork.left + (mInfo.rcWork.right - mInfo.rcWork.left - view->lastConfigure.width) / 2,
                         mInfo.rcWork.top + (mInfo.rcWork.bottom - mInfo.rcWork.top - view->lastConfigure.height) / 2,
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

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, update world without triggering exposure events

PuglStatus puglX11UpdateWithoutExposures(PuglWorld* const world)
{
    const PuglWorldState startState = world->state;
    world->state = PUGL_WORLD_UPDATING;
    PuglStatus st = PUGL_SUCCESS;

    const double startTime = puglGetTime(world);
    const double endTime   = startTime + 0.03;

    for (double t = startTime; !st && t < endTime; t = puglGetTime(world))
    {
        if (!(st = pollX11Socket(world, endTime - t)))
            st = dispatchX11Events(world);
    }

    world->state = startState;
    return st;
}

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, set dialog window type

void puglX11SetWindowType(const PuglView* const view, const bool isStandalone)
{
    const PuglInternals* const impl    = view->impl;
    Display*             const display = view->world->impl->display;

   #if defined(DGL_X11_WINDOW_ICON_NAME) && defined(DGL_X11_WINDOW_ICON_SIZE)
    if (isStandalone)
    {
        const Atom NET_WM_ICON = XInternAtom(display, "_NET_WM_ICON", False);
        XChangeProperty(display,
                        impl->win,
                        NET_WM_ICON,
                        XA_CARDINAL,
                        32,
                        PropModeReplace,
                        reinterpret_cast<const uchar*>(DGL_X11_WINDOW_ICON_NAME),
                        DGL_X11_WINDOW_ICON_SIZE);
    }
   #endif

    const Atom NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

    Atom windowTypes[2];
    int numWindowTypes = 0;

    if (! isStandalone)
        windowTypes[numWindowTypes++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    windowTypes[numWindowTypes++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    XChangeProperty(display,
                    impl->win,
                    NET_WM_WINDOW_TYPE,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<const uchar*>(&windowTypes),
                    numWindowTypes);
}

// --------------------------------------------------------------------------------------------------------------------

#endif // HAVE_X11

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif
