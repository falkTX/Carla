/*
 * Carla Plugin Host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"

#if defined(CARLA_OS_MAC) && !defined(CARLA_PLUGIN_BUILD)
# import <Cocoa/Cocoa.h>
#endif

#ifdef HAVE_X11
# include <X11/Xlib.h>
# include <X11/Xresource.h>
#endif

namespace CB = CARLA_BACKEND_NAMESPACE;

// -------------------------------------------------------------------------------------------------------------------

double carla_get_desktop_scale_factor()
{
    // allow custom scale for testing
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));
    // Qt env var for the same
    if (const char* const scale = getenv("QT_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

#if defined(CARLA_OS_MAC) && !defined(CARLA_PLUGIN_BUILD)
    return [NSScreen mainScreen].backingScaleFactor;
#endif
#ifdef HAVE_X11
    if (::Display* const display = XOpenDisplay(nullptr))
    {
        XrmInitialize();

        if (char* const rms = XResourceManagerString(display))
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
                    const double dpi = std::atof(ret.addr);
                    if (carla_isNotZero(dpi))
                        return dpi / 96;
                }

                XrmDestroyDatabase(sdb);
            }
        }

        XCloseDisplay(display);
    }
#endif

    return 1.0;
}

// -------------------------------------------------------------------------------------------------------------------

int carla_cocoa_get_window(void* nsViewPtr)
{
    CARLA_SAFE_ASSERT_RETURN(nsViewPtr != nullptr, 0);

#if defined(CARLA_OS_MAC) && !defined(CARLA_PLUGIN_BUILD)
    NSView* const nsView = (NSView*)nsViewPtr;
    return [[nsView window] windowNumber];
#else
    return 0;
#endif
}

void carla_cocoa_set_transient_window_for(void* nsViewChildPtr, void* nsViewParentPtr)
{
    CARLA_SAFE_ASSERT_RETURN(nsViewChildPtr != nullptr,);
    CARLA_SAFE_ASSERT_RETURN(nsViewParentPtr != nullptr,);

#if defined(CARLA_OS_MAC) && !defined(CARLA_PLUGIN_BUILD)
    NSView* const nsViewChild  = (NSView*)nsViewChildPtr;
    NSView* const nsViewParent = (NSView*)nsViewParentPtr;
    [[nsViewParent window] addChildWindow:[nsViewChild window]
                                  ordered:NSWindowAbove];
#endif
}

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
    static int pos[4];

    if (winId == 0)
    {
        pos[0] = 0;
        pos[1] = 0;
        pos[2] = 0;
        pos[3] = 0;
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
        pos[2] = xwa.x;
        pos[3] = xwa.y;
    }
#endif
    else
    {
        pos[0] = 0;
        pos[1] = 0;
        pos[2] = 0;
        pos[3] = 0;
    }

    return pos;
}

// -------------------------------------------------------------------------------------------------------------------
