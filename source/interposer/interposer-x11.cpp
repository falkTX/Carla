/*
 * Carla Interposer for X11 Window Mapping
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

// -----------------------------------------------------------------------
// Function typedefs

typedef int (*XMapWindowFunc)(Display*, Window);
typedef int (*XUnmapWindowFunc)(Display*, Window);

// -----------------------------------------------------------------------
// Current mapped window

static Window sCurrentlyMappedWindow = 0;

// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
// Our custom functions

CARLA_EXPORT
int XMapWindow(Display* display, Window window)
{
    for (;;)
    {
#if 0
        if (sCurrentlyMappedWindow != 0)
            break;

        Atom atom;
        int atomFormat;
        unsigned char* atomPtrs;
        unsigned long numItems, ignored;

        const Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", True);

        if (XGetWindowProperty(display, window, wmWindowType, 0, ~0L, False, AnyPropertyType,
                               &atom, &atomFormat, &numItems, &ignored, &atomPtrs) == Success)
        {
            const Atom* const atomValues = (const Atom*)atomPtrs;

            for (ulong i=0; i<numItems; ++i)
            {
                const char* const atomValue(XGetAtomName(display, atomValues[i]));
                CARLA_SAFE_ASSERT_CONTINUE(atomValue != nullptr && atomValue[0] != '\0');

                if (std::strcmp(atomValue, "_NET_WM_WINDOW_TYPE_NORMAL") == 0)
                {
                    sCurrentlyMappedWindow = window;
                    break;
                }
            }
        }
#endif

        if (sCurrentlyMappedWindow == 0)
            sCurrentlyMappedWindow = window;

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

    return real_XMapWindow(display, window);
}

CARLA_EXPORT
int XUnmapWindow(Display* display, Window window)
{
    if (sCurrentlyMappedWindow == window)
        sCurrentlyMappedWindow = 0;

    return real_XUnmapWindow(display, window);
}

// -----------------------------------------------------------------------
