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

#include "pugl.hpp"

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
# include <dlfcn.h>
# include <mach/mach_time.h>
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-quartz.h>
# endif
# ifdef DGL_OPENGL
#  include <OpenGL/gl.h>
# endif
# ifdef DGL_VULKAN
#  import <QuartzCore/CAMetalLayer.h>
#  include <vulkan/vulkan_core.h>
#  include <vulkan/vulkan_macos.h>
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include <wctype.h>
# include <winsock2.h>
# include <windows.h>
# include <windowsx.h>
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-win32.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/gl.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan.h>
#  include <vulkan/vulkan_win32.h>
# endif
#else
# include <dlfcn.h>
# include <unistd.h>
# include <sys/select.h>
# include <sys/time.h>
# include <X11/X.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xresource.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#  include <X11/cursorfont.h>
# endif
# ifdef HAVE_XRANDR
#  include <X11/extensions/Xrandr.h>
# endif
# ifdef HAVE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
# endif
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-xlib.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/gl.h>
#  include <GL/glx.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan_core.h>
#  include <vulkan/vulkan_xlib.h>
# endif
#endif

#ifndef DGL_FILE_BROWSER_DISABLED
# ifdef DISTRHO_OS_MAC
#  import "../../distrho/extra/FileBrowserDialog.cpp"
# else
#  include "../../distrho/extra/FileBrowserDialog.cpp"
# endif
#endif

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#endif

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_MAC)
# ifndef DISTRHO_MACOS_NAMESPACE_MACRO
#  define DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, SEP, INTERFACE) NS ## SEP ## INTERFACE
#  define DISTRHO_MACOS_NAMESPACE_MACRO(NS, INTERFACE) DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, _, INTERFACE)
#  define PuglStubView    DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglStubView)
#  define PuglWrapperView DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglWrapperView)
#  define PuglWindow      DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PuglWindow)
# endif
# ifndef __MAC_10_9
#  define NSModalResponseOK NSOKButton
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
#else
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

#include "pugl-upstream/src/implementation.c"

// --------------------------------------------------------------------------------------------------------------------
// expose backend enter

bool puglBackendEnter(PuglView* const view)
{
    return view->backend->enter(view, nullptr) == PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// expose backend leave

void puglBackendLeave(PuglView* const view)
{
    view->backend->leave(view, nullptr);
}

// --------------------------------------------------------------------------------------------------------------------
// clear minimum size to 0

void puglClearMinSize(PuglView* const view)
{
    view->minWidth  = 0;
    view->minHeight = 0;
}

// --------------------------------------------------------------------------------------------------------------------
// missing in pugl, directly returns transient parent

PuglNativeView puglGetTransientParent(const PuglView* const view)
{
    return view->transientParent;
}

// --------------------------------------------------------------------------------------------------------------------
// missing in pugl, directly returns title char* pointer

const char* puglGetWindowTitle(const PuglView* const view)
{
    return view->title;
}

// --------------------------------------------------------------------------------------------------------------------
// get global scale factor

double puglGetDesktopScaleFactor(const PuglView* const view)
{
#if defined(DISTRHO_OS_MAC)
    if (NSWindow* const window = view->impl->window ? view->impl->window
                                                    : [view->impl->wrapperView window])
        return [window screen].backingScaleFactor;
    return [NSScreen mainScreen].backingScaleFactor;
#elif defined(DISTRHO_OS_WINDOWS)
    if (const HMODULE Shcore = LoadLibraryA("Shcore.dll"))
    {
        typedef HRESULT(WINAPI* PFN_GetProcessDpiAwareness)(HANDLE, DWORD*);
        typedef HRESULT(WINAPI* PFN_GetScaleFactorForMonitor)(HMONITOR, DWORD*);

# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
        const PFN_GetProcessDpiAwareness GetProcessDpiAwareness
            = (PFN_GetProcessDpiAwareness)GetProcAddress(Shcore, "GetProcessDpiAwareness");
        const PFN_GetScaleFactorForMonitor GetScaleFactorForMonitor
            = (PFN_GetScaleFactorForMonitor)GetProcAddress(Shcore, "GetScaleFactorForMonitor");
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif

        DWORD dpiAware = 0;
        if (GetProcessDpiAwareness && GetScaleFactorForMonitor
            && GetProcessDpiAwareness(NULL, &dpiAware) == 0 && dpiAware != 0)
        {
            const HMONITOR hMon = MonitorFromWindow(view->impl->hwnd, MONITOR_DEFAULTTOPRIMARY);

            DWORD scaleFactor = 0;
            if (GetScaleFactorForMonitor(hMon, &scaleFactor) == 0 && scaleFactor != 0)
            {
                FreeLibrary(Shcore);
                return static_cast<double>(scaleFactor) / 100.0;
            }
        }

        FreeLibrary(Shcore);
    }
#elif defined(HAVE_X11)
    XrmInitialize();

    if (char* const rms = XResourceManagerString(view->world->impl->display))
    {
        if (const XrmDatabase sdb = XrmGetStringDatabase(rms))
        {
            char* type = nullptr;
            XrmValue ret;

            if (XrmGetResource(sdb, "Xft.dpi", "String", &type, &ret)
                && ret.addr != nullptr
                && type != nullptr
                && std::strncmp("String", type, 6) == 0)
            {
                if (const double dpi = std::atof(ret.addr))
                    return dpi / 96;
            }
        }
    }
#else
    // unused
    (void)view;
#endif

    return 1.0;
}

// --------------------------------------------------------------------------------------------------------------------
// bring view window into the foreground, aka "raise" window

void puglRaiseWindow(PuglView* const view)
{
#if defined(DISTRHO_OS_MAC)
    if (NSWindow* const window = view->impl->window ? view->impl->window
                                                    : [view->impl->wrapperView window])
        [window orderFrontRegardless];
#elif defined(DISTRHO_OS_WINDOWS)
    SetForegroundWindow(view->impl->hwnd);
    SetActiveWindow(view->impl->hwnd);
#else
    XRaiseWindow(view->impl->display, view->impl->win);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// set backend that matches current build

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
// Combine puglSetMinSize and puglSetAspectRatio

PuglStatus puglSetGeometryConstraints(PuglView* const view, const uint width, const uint height, const bool aspect)
{
    view->minWidth  = (int)width;
    view->minHeight = (int)height;

    if (aspect) {
        view->minAspectX = (int)width;
        view->minAspectY = (int)height;
        view->maxAspectX = (int)width;
        view->maxAspectY = (int)height;
    }

#if defined(DISTRHO_OS_MAC)
    puglSetMinSize(view, width, height);

    if (aspect) {
        puglSetAspectRatio(view, width, height, width, height);
    }
#elif defined(DISTRHO_OS_WINDOWS)
    // nothing
#else
    if (const PuglStatus status = updateSizeHints(view))
        return status;

    XFlush(view->impl->display);
#endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set window offset without changing size

PuglStatus puglSetWindowOffset(PuglView* const view, const int x, const int y)
{
    // TODO custom setFrame version
    PuglRect rect = puglGetFrame(view);
    rect.x = x;
    rect.y = y;
    return puglSetFrame(view, rect);
}

// --------------------------------------------------------------------------------------------------------------------
// set window size with default size and without changing frame x/y position

PuglStatus puglSetWindowSize(PuglView* const view, const uint width, const uint height)
{
    view->defaultWidth  = width;
    view->defaultHeight = height;
    view->frame.width   = width;
    view->frame.height  = height;

#if defined(DISTRHO_OS_MAC)
    // replace setFrame with setFrameSize
    PuglInternals* const impl = view->impl;

    const PuglRect frame = view->frame;
    const NSRect framePx = rectToNsRect(frame);
    const NSRect framePt = nsRectToPoints(view, framePx);

    if (impl->window)
    {
        // Resize window to fit new content rect
        const NSRect screenPt = rectToScreen(viewScreen(view), framePt);
        const NSRect winFrame = [impl->window frameRectForContentRect:screenPt];

        [impl->window setFrame:winFrame display:NO];
    }

    // Resize views
    const NSSize sizePx = NSMakeSize(frame.width, frame.height);
    const NSSize sizePt = [impl->drawView convertSizeFromBacking:sizePx];

    [impl->wrapperView setFrameSize:(impl->window ? sizePt : framePt.size)];
    [impl->drawView setFrameSize:sizePt];
#elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we add SWP_NOMOVE flag
    if (view->impl->hwnd)
    {
        const PuglRect frame = view->frame;

        RECT rect = { (long)frame.x,
                      (long)frame.y,
                      (long)frame.x + (long)frame.width,
                      (long)frame.y + (long)frame.height };

        AdjustWindowRectEx(&rect, puglWinGetWindowFlags(view), FALSE, puglWinGetWindowExFlags(view));

        if (SetWindowPos(view->impl->hwnd,
                         HWND_TOP,
                         rect.left,
                         rect.top,
                         rect.right - rect.left,
                         rect.bottom - rect.top,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER))
        {
            // make sure to return context back to ourselves
            view->backend->enter(view, nullptr);
            return PUGL_SUCCESS;
        }

        return PUGL_UNKNOWN_ERROR;
    }
#else
    // matches upstream pugl, except we use XResizeWindow instead of XMoveResizeWindow
    if (view->impl->win)
    {
        Display* const display = view->impl->display;

        if (! XResizeWindow(display, view->impl->win, width, height))
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
    glLoadIdentity();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback resize

void puglFallbackOnResize(PuglView* const view)
{
#ifdef DGL_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(view->frame.width), static_cast<GLdouble>(view->frame.height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(view->frame.width), static_cast<GLsizei>(view->frame.height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
}

#if defined(DISTRHO_OS_MAC)

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, allow standalone window to gain focus

void puglMacOSActivateApp()
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

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

        const int x = transientFrame.origin.x + transientFrame.size.width / 2 - ourFrame.size.width / 2;
        const int y = transientFrame.origin.y + transientFrame.size.height / 2  + ourFrame.size.height / 2;

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
                         mInfo.rcWork.left + (mInfo.rcWork.right - view->frame.width) / 2,
                         mInfo.rcWork.top + (mInfo.rcWork.bottom - view->frame.height) / 2,
                         0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
        else
            ShowWindow(impl->hwnd, SW_NORMAL);
    }

    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------
// win32 specific, set or unset WS_SIZEBOX style flag

void puglWin32SetWindowResizable(PuglView* const view, const bool resizable)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    const int winFlags = resizable ? GetWindowLong(impl->hwnd, GWL_STYLE) |  WS_SIZEBOX
                                   : GetWindowLong(impl->hwnd, GWL_STYLE) & ~WS_SIZEBOX;
    SetWindowLong(impl->hwnd, GWL_STYLE, winFlags);
}

// --------------------------------------------------------------------------------------------------------------------

#elif defined(HAVE_X11)

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, safer way to grab focus

PuglStatus puglX11GrabFocus(const PuglView* const view)
{
    const PuglInternals* const impl = view->impl;

    XWindowAttributes wa;
    std::memset(&wa, 0, sizeof(wa));

    DISTRHO_SAFE_ASSERT_RETURN(XGetWindowAttributes(impl->display, impl->win, &wa), PUGL_UNKNOWN_ERROR);

    if (wa.map_state == IsViewable)
    {
        XRaiseWindow(impl->display, impl->win);
        XSetInputFocus(impl->display, impl->win, RevertToPointerRoot, CurrentTime);
        XSync(impl->display, False);
    }

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, set dialog window type and pid hints

void puglX11SetWindowTypeAndPID(const PuglView* const view, const bool isStandalone)
{
    const PuglInternals* const impl = view->impl;

    const pid_t pid = getpid();
    const Atom _nwp = XInternAtom(impl->display, "_NET_WM_PID", False);
    XChangeProperty(impl->display, impl->win, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

    const Atom _wt = XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE", False);

    Atom _wts[2];
    int numAtoms = 0;

    if (! isStandalone)
        _wts[numAtoms++] = XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    _wts[numAtoms++] = XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    XChangeProperty(impl->display, impl->win, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, numAtoms);
}

// --------------------------------------------------------------------------------------------------------------------

#endif // HAVE_X11

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif
