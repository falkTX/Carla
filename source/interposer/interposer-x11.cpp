/*
 * Carla Interposer for X11 Window Mapping
 * Copyright (C) 2014-2015 Filipe Coelho <falktx@falktx.com>
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

// -----------------------------------------------------------------------
// Function typedefs

typedef int (*XMapWindowFunc)(Display*, Window);
typedef int (*XUnmapWindowFunc)(Display*, Window);

// -----------------------------------------------------------------------
// Global counter so we only map the first (hopefully main) window

static int sMapWindowCounter = 0;

// -----------------------------------------------------------------------
// Calling the real functions

static int real_XMapWindow(Display* display, Window w)
{
    static const XMapWindowFunc func = (XMapWindowFunc)::dlsym(RTLD_NEXT, "XMapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, w);
}

static int real_XUnmapWindow(Display* display, Window w)
{
    static const XUnmapWindowFunc func = (XUnmapWindowFunc)::dlsym(RTLD_NEXT, "XUnmapWindow");
    CARLA_SAFE_ASSERT_RETURN(func != nullptr, 0);

    return func(display, w);
}

// -----------------------------------------------------------------------
// Our custom functions

CARLA_EXPORT
int XMapWindow(Display* display, Window w)
{
    carla_stdout("------------------------------- XMapWindow called");

    for (;;)
    {
        if (++sMapWindowCounter != 1)
            break;

        static bool libjackopen = false;

        if (! libjackopen && std::getenv("CARLA_LIBJACK_SETUP") != nullptr)
        {
            libjackopen = true;
            dlopen("libjack.so.0", RTLD_NOW|RTLD_LOCAL);
            carla_stdout("------------------------------- libjack opened");
        }

        if (const char* const winIdStr = std::getenv("CARLA_ENGINE_OPTION_FRONTEND_WIN_ID"))
        {
            CARLA_SAFE_ASSERT_BREAK(winIdStr[0] != '\0');

            const long long winIdLL(std::strtoll(winIdStr, nullptr, 16));
            CARLA_SAFE_ASSERT_BREAK(winIdLL > 0);

            const Window winId(static_cast<Window>(winIdLL));
            XSetTransientForHint(display, w, static_cast<Window>(winId));

            carla_stdout("Transient hint correctly applied before mapping window");
        }

        break;
    }

    return real_XMapWindow(display, w);
}

CARLA_EXPORT
int XUnmapWindow(Display* display, Window w)
{
    carla_stdout("------------------------------- XUnmapWindow called");

    --sMapWindowCounter;

    return real_XUnmapWindow(display, w);
}

// -----------------------------------------------------------------------
