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
#include "CarlaHost.h"

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

        if (const uintptr_t transientId = carla_standalone_get_transient_win_id())
            setTransientWinId(transientId);
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
        sizeHints.min_width  = static_cast<int>(width);
        sizeHints.min_height = static_cast<int>(height);
        sizeHints.max_width  = static_cast<int>(width);
        sizeHints.max_height = static_cast<int>(height);
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

    void setTransientWinId(const uintptr_t winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XSetTransientForHint(fDisplay, fWindow, (Window)winId);
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

bool CarlaPluginUi::tryTransientWinIdMatch(const ulong pid, const char* const uiTitle, const uintptr_t winId)
{
    CARLA_SAFE_ASSERT_RETURN(uiTitle != nullptr && uiTitle[0] != '\0', true);
    CARLA_SAFE_ASSERT_RETURN(winId != 0, true);

#if defined(CARLA_OS_MAC)
#elif defined(CARLA_OS_WIN)
#elif defined(HAVE_X11)
    struct ScopedDisplay {
        Display* display;
        ScopedDisplay() : display(XOpenDisplay(0)) {}
        ~ScopedDisplay() { if (display!=nullptr) XCloseDisplay(display); }
    };
    struct ScopedFreeData {
        uchar* data;
        ScopedFreeData(uchar* d) : data(d) {}
        ~ScopedFreeData() { XFree(data); }
    };

    const ScopedDisplay sd;
    CARLA_SAFE_ASSERT_RETURN(sd.display != nullptr, true);

    Atom _ncl = XInternAtom(sd.display, "_NET_CLIENT_LIST" , True);
    Atom _nwn = XInternAtom(sd.display, "_NET_WM_NAME", True);
    Atom _nwp = XInternAtom(sd.display, "_NET_WM_PID", True);
    Atom utf8 = XInternAtom(sd.display, "UTF8_STRING", True);

    Atom actualType;
    int actualFormat;
    unsigned long numWindows, bytesAfter;
    unsigned char* data = nullptr;

    int status = XGetWindowProperty(sd.display, DefaultRootWindow(sd.display), _ncl, 0L, (~0L), False, AnyPropertyType, &actualType, &actualFormat, &numWindows, &bytesAfter, &data);

    CARLA_SAFE_ASSERT_RETURN(data != nullptr, true);
    const ScopedFreeData sfd(data);

    CARLA_SAFE_ASSERT_RETURN(status == Success, true);
    CARLA_SAFE_ASSERT_RETURN(actualFormat == 32, true);
    CARLA_SAFE_ASSERT_RETURN(numWindows != 0, true);

    Window* windows = (Window*)data;
    Window  lastGoodWindow = 0;

    for (ulong i = 0; i < numWindows; i++)
    {
        const Window window(windows[i]);
        CARLA_SAFE_ASSERT_CONTINUE(window != 0);

        // ------------------------------------------------
        // try using pid

        if (pid != 0)
        {
            unsigned long pidSize;
            unsigned char* pidData = nullptr;

            status = XGetWindowProperty(sd.display, window, _nwp, 0L, (~0L), False, XA_CARDINAL, &actualType, &actualFormat, &pidSize, &bytesAfter, &pidData);

            if (pidData != nullptr)
            {
                const ScopedFreeData sfd2(pidData);

                CARLA_SAFE_ASSERT_CONTINUE(status == Success);
                CARLA_SAFE_ASSERT_CONTINUE(pidSize != 0);

                if (*(ulong*)pidData == pid)
                {
                    CARLA_SAFE_ASSERT_RETURN(lastGoodWindow == window || lastGoodWindow == 0,  true);
                    lastGoodWindow = window;
                    carla_stdout("Match found using pid");
                    break;
                }
            }
        }

        // ------------------------------------------------
        // try using name

        unsigned long nameSize;
        unsigned char* nameData = nullptr;

        status = XGetWindowProperty(sd.display, window, _nwn, 0L, (~0L), False, utf8, &actualType, &actualFormat, &nameSize, &bytesAfter, &nameData);

        if (nameData != nullptr)
        {
            const ScopedFreeData sfd2(nameData);

            CARLA_SAFE_ASSERT_CONTINUE(status == Success);
            CARLA_SAFE_ASSERT_CONTINUE(nameSize != 0);

            if (std::strstr((const char*)nameData, uiTitle) != nullptr)
            {
                CARLA_SAFE_ASSERT_RETURN(lastGoodWindow == window || lastGoodWindow == 0,  true);
                lastGoodWindow = window;
                carla_stdout("Match found using name");
            }
        }
    }

    if (lastGoodWindow == 0)
        return false;

    Atom _nwt = XInternAtom(sd.display ,"_NET_WM_WINDOW_TYPE", True);
    Atom _nwd = XInternAtom(sd.display, "_NET_WM_WINDOW_TYPE_DIALOG", True);

    XUnmapWindow(sd.display, lastGoodWindow);
    XChangeProperty(sd.display, lastGoodWindow, _nwt, XA_ATOM, 32, PropModeReplace, (uchar*)&_nwd, 1);
    XSetTransientForHint(sd.display, lastGoodWindow, (Window)winId);
    XMapWindow(sd.display, lastGoodWindow);
    XFlush(sd.display);
    return true;
#endif
}

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
