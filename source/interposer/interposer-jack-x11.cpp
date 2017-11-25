/*
 * Carla Interposer for JACK Applications X11 control
 * Copyright (C) 2014-2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.hpp"

#include <dlfcn.h>
#include <X11/Xlib.h>

struct ScopedLibOpen {
    void* handle;
    long long winId;

    ScopedLibOpen()
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

    ~ScopedLibOpen()
    {
        if (handle != nullptr)
        {
            dlclose(handle);
            handle = nullptr;
        }
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Function typedefs

typedef int (*XMapWindowFunc)(Display*, Window);
typedef int (*XUnmapWindowFunc)(Display*, Window);
typedef int (*CarlaInterposedCallback)(int, void*);

// ---------------------------------------------------------------------------------------------------------------------
// Current state

static Display* gCurrentlyMappedDisplay = nullptr;
static Window gCurrentlyMappedWindow = 0;
static CarlaInterposedCallback gInterposedCallback = nullptr;
static int gInterposedSessionManager = 0;
static int gInterposedHints = 0;
static bool gCurrentWindowMapped = false;
static bool gCurrentWindowVisible = false;

// ---------------------------------------------------------------------------------------------------------------------
// Calling the real functions

static int real_XMapWindow(Display* display, Window window)
{
    static const XMapWindowFunc func = (XMapWindowFunc)::dlsym(RTLD_NEXT, "XMapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

static int real_XUnmapWindow(Display* display, Window window)
{
    static const XUnmapWindowFunc func = (XUnmapWindowFunc)::dlsym(RTLD_NEXT, "XUnmapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, window);
}

// ---------------------------------------------------------------------------------------------------------------------
// Our custom functions

CARLA_EXPORT
int XMapWindow(Display* display, Window window)
{
    static const ScopedLibOpen slo;

    for (;;)
    {
        if (slo.winId < 0)
            break;

        Atom atom;
        int atomFormat;
        unsigned char* atomPtrs;
        unsigned long numItems, ignored;

        const Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", True);

        if (XGetWindowProperty(display, window, wmWindowType, 0, ~0L, False, AnyPropertyType,
                               &atom, &atomFormat, &numItems, &ignored, &atomPtrs) != Success)
            break;

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
                break;
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
            real_XMapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
        }

        gCurrentlyMappedDisplay = display;
        gCurrentlyMappedWindow  = window;
        gCurrentWindowMapped    = true;

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

    return real_XMapWindow(display, window);
}

CARLA_EXPORT
int XUnmapWindow(Display* display, Window window)
{
    if (gCurrentlyMappedWindow == window)
    {
        gCurrentlyMappedDisplay = nullptr;
        gCurrentlyMappedWindow  = 0;
        gCurrentWindowMapped    = false;
        gCurrentWindowVisible   = false;

        if (gInterposedCallback != nullptr)
            gInterposedCallback(1, nullptr);
    }

    return real_XUnmapWindow(display, window);
}

// ---------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT
int jack_carla_interposed_action(int action, int value, void* ptr)
{
    carla_debug("jack_carla_interposed_action(%i, %i, %p)", action, value, ptr);

    switch (action)
    {
    case 1:
        // set hints and callback
        gInterposedHints = value;
        gInterposedCallback = (CarlaInterposedCallback)ptr;
        return 1;

    case 2:
        // session manager
        gInterposedSessionManager = value;
        return 1;

    case 3:
        // show gui
        if (value != 0)
        {
            gCurrentWindowVisible = true;
            if (gCurrentlyMappedDisplay == nullptr || gCurrentlyMappedWindow == 0)
                return 0;

            gCurrentWindowMapped = true;
            return real_XMapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
        }
        // hide gui
        else
        {
            gCurrentWindowVisible = false;
            if (gCurrentlyMappedDisplay == nullptr || gCurrentlyMappedWindow == 0)
                return 0;

            gCurrentWindowMapped = false;
            return real_XUnmapWindow(gCurrentlyMappedDisplay, gCurrentlyMappedWindow);
        }
        break;

    case 4: // close everything
        gCurrentWindowMapped  = false;
        gCurrentWindowVisible = false;
        gCurrentlyMappedDisplay = nullptr;
        gCurrentlyMappedWindow  = 0;
        return 0;
    }

    return -1;
}

// ---------------------------------------------------------------------------------------------------------------------
