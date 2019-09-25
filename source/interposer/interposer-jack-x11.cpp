/*
 * Carla Interposer for JACK Applications X11 control
 * Copyright (C) 2014-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaLibJackHints.h"
#include "CarlaUtils.hpp"

#include <dlfcn.h>
#include <X11/Xlib.h>

// --------------------------------------------------------------------------------------------------------------------

struct ScopedLibOpen {
    void* handle;
    long long winId;

    ScopedLibOpen() noexcept
        : handle(dlopen("libjack.so.0", RTLD_NOW|RTLD_LOCAL)),
          winId(-1)
    {
        CARLA_SAFE_ASSERT_RETURN(handle != nullptr,);

        if (const char* const winIdStr = std::getenv("CARLA_FRONTEND_WIN_ID"))
        {
            CARLA_SAFE_ASSERT_RETURN(winIdStr[0] != '\0',);

            winId = std::strtoll(winIdStr, nullptr, 16);
        }
    }

    ~ScopedLibOpen() noexcept
    {
        if (handle != nullptr)
        {
            dlclose(handle);
            handle = nullptr;
        }
    }

    static const ScopedLibOpen& getInstance() noexcept
    {
        static const ScopedLibOpen slo;
        return slo;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(ScopedLibOpen);
};

// --------------------------------------------------------------------------------------------------------------------
// Function typedefs

typedef int (*XWindowFunc)(Display*, Window);
typedef int (*XNextEventFunc)(Display*, XEvent*);

// --------------------------------------------------------------------------------------------------------------------
// Current state

typedef enum {
    WindowMapNone,
    WindowMapNormal,
    WindowMapRaised,
    WindowMapSubwindows
} WindowMappingType;

static Display* gCurrentlyMappedDisplay = nullptr;
static Window gCurrentlyMappedWindow = 0;
static CarlaInterposedCallback gInterposedCallback = nullptr;
static uint gInterposedSessionManager = LIBJACK_SESSION_MANAGER_NONE;
static uint gInterposedHints = 0x0;
static WindowMappingType gCurrentWindowType = WindowMapNone;
static bool gCurrentWindowMapped = false;
static bool gCurrentWindowVisible = false;

// --------------------------------------------------------------------------------------------------------------------
// Calling the real functions

static int real_XMapWindow(Display* display, Window window)
{
    static const XWindowFunc func = (XWindowFunc)::dlsym(RTLD_NEXT, "XMapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

static int real_XMapRaised(Display* display, Window window)
{
    static const XWindowFunc func = (XWindowFunc)::dlsym(RTLD_NEXT, "XMapRaised");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

static int real_XMapSubwindows(Display* display, Window window)
{
    static const XWindowFunc func = (XWindowFunc)::dlsym(RTLD_NEXT, "XMapSubwindows");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

static int real_XUnmapWindow(Display* display, Window window)
{
    static const XWindowFunc func = (XWindowFunc)::dlsym(RTLD_NEXT, "XUnmapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

static int real_XNextEvent(Display* display, XEvent* event)
{
    static const XNextEventFunc func = (XNextEventFunc)::dlsym(RTLD_NEXT, "XNextEvent");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, event);
}

// --------------------------------------------------------------------------------------------------------------------
// Custom carla window handling

static int carlaWindowMap(Display* const display, const Window window, const WindowMappingType fallbackFnType)
{
    const ScopedLibOpen& slo(ScopedLibOpen::getInstance());

    for (;;)
    {
        if (slo.winId < 0)
            break;

        Atom atom;
        int atomFormat;
        unsigned char* atomPtrs;
        unsigned long numItems, ignored;

        const Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

        if (XGetWindowProperty(display, window, wmWindowType, 0, ~0L, False, AnyPropertyType,
                               &atom, &atomFormat, &numItems, &ignored, &atomPtrs) != Success)
        {
            carla_debug("carlaWindowMap(%p, %lu, %i) - XGetWindowProperty failed", display, window, fallbackFnType);
            break;
        }

        const Atom* const atomValues = (const Atom*)atomPtrs;
        bool isMainWindow = (numItems == 0);

        for (ulong i=0; i<numItems; ++i)
        {
            const char* const atomValue(XGetAtomName(display, atomValues[i]));
            CARLA_SAFE_ASSERT_CONTINUE(atomValue != nullptr && atomValue[0] != '\0');

            if (std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_COMBO"        ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_DIALOG"       ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_DND"          ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_DOCK"         ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU") == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_MENU"         ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_NOTIFICATION" ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_POPUP_MENU"   ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_SPLASH"       ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_TOOLBAR"      ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_TOOLTIP"      ) == 0 ||
                std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_UTILITY"      ) == 0)
            {
                isMainWindow = false;
                continue;
            }

            if (std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_NORMAL") == 0)
            {
                // window is good, use it if no other types are set
                isMainWindow = true;
            }
            else
            {
                carla_stdout("=======================================> %s", atomValue);
            }
        }

        if (! isMainWindow)
        {
            // this has always bothered me...
            if (gCurrentlyMappedWindow != 0 && gCurrentWindowMapped && gCurrentWindowVisible)
                XSetTransientForHint(display, window, gCurrentlyMappedWindow);
            break;
        }

        Window transientWindow = 0;
        if (XGetTransientForHint(display, window, &transientWindow) == Success && transientWindow != 0)
        {
            carla_stdout("Window has transient set already, ignoring it");
            break;
        }

        // got a new window, we may need to forget last one
        if (gCurrentlyMappedDisplay != nullptr && gCurrentlyMappedWindow != 0)
        {
            // ignore requests against the current mapped window
            if (gCurrentlyMappedWindow == window)
                return 0;

            // we already have a mapped window, with carla visible button on, should be a dialog of sorts..
            if (gCurrentWindowMapped && gCurrentWindowVisible)
            {
                XSetTransientForHint(display, window, gCurrentlyMappedWindow);
                break;
            }

            // ignore empty windows created after the main one
            if (numItems == 0)
                break;

            carla_stdout("NOTICE: XMapWindow now showing previous window");
            switch (gCurrentWindowType)
            {
            case WindowMapNone:
                break;
            case WindowMapNormal:
                real_XMapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
                break;
            case WindowMapRaised:
                real_XMapRaised(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
                break;
            case WindowMapSubwindows:
                real_XMapSubwindows(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
                break;
            }
        }

        gCurrentlyMappedDisplay = display;
        gCurrentlyMappedWindow  = window;
        gCurrentWindowMapped    = true;
        gCurrentWindowType      = fallbackFnType;

        if (slo.winId > 0)
            XSetTransientForHint(display, window, static_cast<Window>(slo.winId));

        if (gCurrentWindowVisible)
        {
            carla_stdout("JACK application window found, showing it now");
            break;
        }

        gCurrentWindowMapped = false;
        carla_stdout("JACK application window found and captured");
        return 0;
    }

    carla_debug("carlaWindowMap(%p, %lu, %i) - not captured", display, window, fallbackFnType);

    switch (fallbackFnType)
    {
    case 1:
        return real_XMapWindow(display, window);
    case 2:
        return real_XMapRaised(display, window);
    case 3:
        return real_XMapSubwindows(display, window);
    default:
        return 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Our custom X11 functions

CARLA_EXPORT
int XMapWindow(Display* display, Window window)
{
    carla_debug("XMapWindow(%p, %lu)", display, window);
    return carlaWindowMap(display, window, WindowMapNormal);
}

CARLA_EXPORT
int XMapRaised(Display* display, Window window)
{
    carla_debug("XMapRaised(%p, %lu)", display, window);
    return carlaWindowMap(display, window, WindowMapRaised);
}

CARLA_EXPORT
int XMapSubwindows(Display* display, Window window)
{
    carla_debug("XMapSubwindows(%p, %lu)", display, window);
    return carlaWindowMap(display, window, WindowMapSubwindows);
}

CARLA_EXPORT
int XUnmapWindow(Display* display, Window window)
{
    carla_debug("XUnmapWindow(%p, %lu)", display, window);

    if (gCurrentlyMappedWindow == window)
    {
        gCurrentlyMappedDisplay = nullptr;
        gCurrentlyMappedWindow  = 0;
        gCurrentWindowType      = WindowMapNone;
        gCurrentWindowMapped    = false;
        gCurrentWindowVisible   = false;

        if (gInterposedCallback != nullptr)
            gInterposedCallback(LIBJACK_INTERPOSER_CALLBACK_UI_HIDE, nullptr);
    }

    return real_XUnmapWindow(display, window);
}

CARLA_EXPORT
int XNextEvent(Display* display, XEvent* event)
{
    const int ret = real_XNextEvent(display, event);

    if (ret != 0)
        return ret;
    if (gCurrentlyMappedWindow == 0)
        return ret;
    if (event->type != ClientMessage)
        return ret;
    if (event->xclient.window != gCurrentlyMappedWindow)
        return ret;

    char* const type = XGetAtomName(display, event->xclient.message_type);
    CARLA_SAFE_ASSERT_RETURN(type != nullptr, 0);

    if (std::strcmp(type, "WM_PROTOCOLS") != 0)
        return ret;
    if ((Atom)event->xclient.data.l[0] != XInternAtom(display, "WM_DELETE_WINDOW", False))
        return ret;

    gCurrentWindowVisible = false;
    gCurrentWindowMapped = false;

    if (gInterposedCallback != nullptr)
        gInterposedCallback(LIBJACK_INTERPOSER_CALLBACK_UI_HIDE, nullptr);

    event->type = 0;
    carla_stdout("XNextEvent close event caught, hiding UI instead");
    return real_XUnmapWindow(display, gCurrentlyMappedWindow);
}

// --------------------------------------------------------------------------------------------------------------------
// Full control helper

CARLA_EXPORT
int jack_carla_interposed_action(uint action, uint value, void* ptr)
{
    carla_debug("jack_carla_interposed_action(%i, %i, %p)", action, value, ptr);

    switch (action)
    {
    case LIBJACK_INTERPOSER_ACTION_SET_HINTS_AND_CALLBACK:
        gInterposedHints = value;
        gInterposedCallback = (CarlaInterposedCallback)ptr;
        return 1;

    case LIBJACK_INTERPOSER_ACTION_SET_SESSION_MANAGER:
        gInterposedSessionManager = value;
        return 1;

    case LIBJACK_INTERPOSER_ACTION_SHOW_HIDE_GUI:
        // show gui
        if (value != 0)
        {
            gCurrentWindowVisible = true;
            if (gCurrentlyMappedDisplay == nullptr || gCurrentlyMappedWindow == 0)
            {
                carla_stdout("NOTICE: Interposer show-gui request ignored");
                return 0;
            }

            gCurrentWindowMapped = true;

            switch (gCurrentWindowType)
            {
            case WindowMapNormal:
                return real_XMapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
            case WindowMapRaised:
                return real_XMapRaised(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
            case WindowMapSubwindows:
                return real_XMapSubwindows(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
            default:
                return 0;
            }
        }
        // hide gui
        else
        {
            gCurrentWindowVisible = false;
            if (gCurrentlyMappedDisplay == nullptr || gCurrentlyMappedWindow == 0)
            {
                carla_stdout("NOTICE: Interposer hide-gui request ignored");
                return 0;
            }

            gCurrentWindowMapped = false;
            return real_XUnmapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
        }
        break;

    case LIBJACK_INTERPOSER_ACTION_CLOSE_EVERYTHING:
        gCurrentWindowType      = WindowMapNone;
        gCurrentWindowMapped    = false;
        gCurrentWindowVisible   = false;
        gCurrentlyMappedDisplay = nullptr;
        gCurrentlyMappedWindow  = 0;
        return 0;
    }

    return -1;
}

// --------------------------------------------------------------------------------------------------------------------
