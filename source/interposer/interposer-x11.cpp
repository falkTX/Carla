/*
 * Carla Interposer for X11 Window Mapping
 * Copyright (C) 2014-2018 Filipe Coelho <falktx@falktx.com>
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

// --------------------------------------------------------------------------------------------------------------------
// Function typedefs

typedef int (*XWindowFunc)(Display*, Window);

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

// --------------------------------------------------------------------------------------------------------------------
// Custom carla window handling

static int carlaWindowMap(Display* const display, const Window window, const int fallbackFnType)
{
    for (;;)
    {
        if (const char* const winIdStr = std::getenv("CARLA_ENGINE_OPTION_FRONTEND_WIN_ID"))
        {
            CARLA_SAFE_ASSERT_BREAK(winIdStr[0] != '\0');

            const long long winIdLL(std::strtoll(winIdStr, nullptr, 16));
            CARLA_SAFE_ASSERT_BREAK(winIdLL > 0);

            const Window winId(static_cast<Window>(winIdLL));
            XSetTransientForHint(display, window, static_cast<Window>(winId));

            carla_stdout("Transient hint correctly applied before mapping window");
        }

        break;
    }

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
    return carlaWindowMap(display, window, 1);
}

CARLA_EXPORT
int XMapRaised(Display* display, Window window)
{
    carla_debug("XMapRaised(%p, %lu)", display, window);
    return carlaWindowMap(display, window, 2);
}

CARLA_EXPORT
int XMapSubwindows(Display* display, Window window)
{
    carla_debug("XMapSubwindows(%p, %lu)", display, window);
    return carlaWindowMap(display, window, 3);
}

CARLA_EXPORT
int XUnmapWindow(Display* display, Window window)
{
    carla_debug("XUnmapWindow(%p, %lu)", display, window);
    return real_XUnmapWindow(display, window);
}

// --------------------------------------------------------------------------------------------------------------------
