/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaUtils.hpp"

#ifdef CARLA_OS_MAC
# import <Cocoa/Cocoa.h>
#endif

#ifdef HAVE_X11
# include <X11/Xlib.h>
#endif

namespace CB = CarlaBackend;

// -------------------------------------------------------------------------------------------------------------------

void carla_x11_reparent_window(uintptr_t winId1, uintptr_t winId2)
{
    CARLA_SAFE_ASSERT_RETURN(winId1 != 0,);
    CARLA_SAFE_ASSERT_RETURN(winId2 != 0,);

#ifdef HAVE_X11
    if (::Display* const disp = XOpenDisplay(nullptr))
    {
        XReparentWindow(disp, winId1, winId2, 0, 0);
        XMapWindow(disp, winId1);
        XCloseDisplay(disp);
    }
#endif
}

void carla_x11_move_window(uintptr_t winId, int x, int y)
{
    CARLA_SAFE_ASSERT_RETURN(winId != 0,);

#ifdef HAVE_X11
    if (::Display* const disp = XOpenDisplay(nullptr))
    {
        XMoveWindow(disp, winId, x, y);
        XCloseDisplay(disp);
    }
#else
    // unused
    return; (void)x; (void)y;
#endif
}

int* carla_x11_get_window_pos(uintptr_t winId)
{
    static int pos[2];

    if (winId == 0)
    {
        pos[0] = 0;
        pos[1] = 0;
    }
#ifdef HAVE_X11
    else if (::Display* const disp = XOpenDisplay(nullptr))
    {
        int x, y;
        Window child;
        XWindowAttributes xwa;
        XTranslateCoordinates(disp, winId, XRootWindow(disp, 0), 0, 0, &x, &y, &child);
        XGetWindowAttributes(disp, winId, &xwa);
        XCloseDisplay(disp);
        pos[0] = x - xwa.x;
        pos[1] = y - xwa.y;
    }
#endif
    else
    {
        pos[0] = 0;
        pos[1] = 0;
    }

    return pos;
}

int carla_cocoa_get_window(void* nsViewPtr)
{
    CARLA_SAFE_ASSERT_RETURN(nsViewPtr != nullptr, 0);

#ifdef CARLA_OS_MAC
    NSView* const nsView = (NSView*)nsViewPtr;
    return [[nsView window] windowNumber];
#else
    return 0;
#endif
}

// -------------------------------------------------------------------------------------------------------------------
