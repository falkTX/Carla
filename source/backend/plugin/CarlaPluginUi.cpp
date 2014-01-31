/*
 * Carla Plugin UI
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginUi.hpp"

#ifdef HAVE_X11
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
#endif

#ifdef HAVE_X11
// -----------------------------------------------------
// X11

class X11PluginUi : public CarlaPluginUi
{
public:
    X11PluginUi(CloseCallback* cb) noexcept
        : CarlaPluginUi(cb),
          fDisplay(nullptr),
          fWindow(0)
     {
        fDisplay = XOpenDisplay(0);
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const int screen = DefaultScreen(fDisplay);

        XSetWindowAttributes attr;
        carla_zeroStruct<XSetWindowAttributes>(attr);

        fWindow = XCreateWindow(fDisplay, RootWindow(fDisplay, screen),
                                0, 0, 300, 300, 0,
                                DefaultDepth(fDisplay, screen),
                                InputOutput,
                                DefaultVisual(fDisplay, screen),
                                CWBorderPixel | CWColormap | CWEventMask, &attr);

        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        Atom wmDelete = XInternAtom(fDisplay, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(fDisplay, fWindow, &wmDelete, 1);
    }

    ~X11PluginUi() override
    {
        if (fWindow != 0)
        {
            XDestroyWindow(fDisplay, fWindow);
            fWindow = 0;
        }

        if (fDisplay != nullptr)
        {
            XCloseDisplay(fDisplay);
            fDisplay = nullptr;
        }
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XMapRaised(fDisplay, fWindow);
        XFlush(fDisplay);
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XUnmapWindow(fDisplay, fWindow);
        XFlush(fDisplay);
    }

    void idle() override
    {
        for (XEvent event; XPending(fDisplay) > 0;)
        {
            XNextEvent(fDisplay, &event);

            switch (event.type)
            {
            case ClientMessage:
                if (std::strcmp(XGetAtomName(fDisplay, event.xclient.message_type), "WM_PROTOCOLS") == 0)
                {
                    CARLA_SAFE_ASSERT_BREAK(fCallback != nullptr);
                    fCallback->handlePluginUiClosed();
                }
                break;
            default:
                break;
            }
        }
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XRaiseWindow(fDisplay, fWindow);
        XSetInputFocus(fDisplay, fWindow, RevertToPointerRoot, CurrentTime);
        XFlush(fDisplay);
    }

    void setSize(const uint width, const uint height, const bool forceUpdate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XResizeWindow(fDisplay, fWindow, width, height);

        XSizeHints sizeHints;
        carla_zeroStruct<XSizeHints>(sizeHints);

        sizeHints.flags      = PMinSize|PMaxSize;
        sizeHints.min_width  = width;
        sizeHints.min_height = height;
        sizeHints.max_width  = width;
        sizeHints.max_height = height;
        XSetNormalHints(fDisplay, fWindow, &sizeHints);

        if (forceUpdate)
            XFlush(fDisplay);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XStoreName(fDisplay, fWindow, title);
    }

    void* getPtr() const noexcept
    {
        return (void*)fWindow;
    }

private:
    Display* fDisplay;
    Window   fWindow;
};
#endif

// -----------------------------------------------------

#ifdef CARLA_OS_MAC
CarlaPluginUi* CarlaPluginUi::newCocoa(CloseCallback* cb)
{
    return new CocoaPluginUi(cb);
}
#endif

#ifdef CARLA_OS_WIN
CarlaPluginUi* CarlaPluginUi::newWindows(CloseCallback* cb)
{
    return new WindowsPluginUi(cb);
}
#endif

#ifdef HAVE_X11
CarlaPluginUi* CarlaPluginUi::newX11(CloseCallback* cb)
{
    return new X11PluginUi(cb);
}
#endif

// -----------------------------------------------------
