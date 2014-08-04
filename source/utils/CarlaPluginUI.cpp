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

#include "CarlaPluginUI.hpp"

#ifdef HAVE_X11
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
#endif

#ifdef HAVE_X11
# include "CarlaPluginUI_X11Icon.hpp"

// -----------------------------------------------------
// X11

typedef void (*EventProcPtr)(XEvent* ev);

static const int X11Key_Escape = 9;
static bool gErrorTriggered = false;

static int temporaryErrorHandler(Display*, XErrorEvent*)
{
    gErrorTriggered = true;
    return 0;
}

class X11PluginUI : public CarlaPluginUI
{
public:
    X11PluginUI(CloseCallback* const cb, const uintptr_t parentId) noexcept
        : CarlaPluginUI(cb),
          fDisplay(nullptr),
          fWindow(0),
          fIsVisible(false),
          fFirstShow(true),
          fEventProc(nullptr)
     {
        fDisplay = XOpenDisplay(nullptr);
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const int screen = DefaultScreen(fDisplay);

        XSetWindowAttributes attr;
        carla_zeroStruct<XSetWindowAttributes>(attr);

        attr.border_pixel = 0;
        attr.event_mask   = KeyPressMask|KeyReleaseMask;

        fWindow = XCreateWindow(fDisplay, RootWindow(fDisplay, screen),
                                0, 0, 300, 300, 0,
                                DefaultDepth(fDisplay, screen),
                                InputOutput,
                                DefaultVisual(fDisplay, screen),
                                CWBorderPixel|CWEventMask, &attr);

        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        XGrabKey(fDisplay, X11Key_Escape, AnyModifier, fWindow, 1, GrabModeAsync, GrabModeAsync);

        Atom wmDelete = XInternAtom(fDisplay, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(fDisplay, fWindow, &wmDelete, 1);

        const pid_t pid = getpid();
        const Atom _nwp = XInternAtom(fDisplay, "_NET_WM_PID", False);
        XChangeProperty(fDisplay, fWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

        const Atom _nwi = XInternAtom(fDisplay, "_NET_WM_ICON", False);
        XChangeProperty(fDisplay, fWindow, _nwi, XA_CARDINAL, 32, PropModeReplace, (const uchar*)sCarlaX11Icon, sCarlaX11IconSize);

        if (parentId != 0)
            setTransientWinId(parentId);
    }

    ~X11PluginUI() override
    {
        CARLA_SAFE_ASSERT(! fIsVisible);

        if (fIsVisible)
        {
            XUnmapWindow(fDisplay, fWindow);
            fIsVisible = false;
        }

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

        if (fFirstShow)
        {
            if (const Window childWindow = getChildWindow())
            {
                const Atom _xevp = XInternAtom(fDisplay, "_XEventProc", False);

                gErrorTriggered = false;
                const XErrorHandler oldErrorHandler(XSetErrorHandler(temporaryErrorHandler));

                Atom actualType;
                int actualFormat;
                ulong nitems, bytesAfter;
                uchar* data = nullptr;

                XGetWindowProperty(fDisplay, childWindow, _xevp, 0, 1, False, AnyPropertyType, &actualType, &actualFormat, &nitems, &bytesAfter, &data);
                XSetErrorHandler(oldErrorHandler);

                if (nitems == 1 && ! gErrorTriggered)
                {
                    fEventProc = *reinterpret_cast<EventProcPtr*>(data);
                    XMapRaised(fDisplay, childWindow);
                }
            }
        }

        fIsVisible = true;
        fFirstShow = false;

        XMapRaised(fDisplay, fWindow);
        XFlush(fDisplay);
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        fIsVisible = false;
        XUnmapWindow(fDisplay, fWindow);
        XFlush(fDisplay);
    }

    void idle() override
    {
        // prevent recursion
        if (fIsIdling) return;

        fIsIdling = true;

        for (XEvent event; XPending(fDisplay) > 0;)
        {
            XNextEvent(fDisplay, &event);

            if (! fIsVisible)
                continue;

            char* type = nullptr;

            switch (event.type)
            {
            case ClientMessage:
                type = XGetAtomName(fDisplay, event.xclient.message_type);
                CARLA_SAFE_ASSERT_CONTINUE(type != nullptr);

                if (std::strcmp(type, "WM_PROTOCOLS") == 0)
                {
                    fIsVisible = false;
                    CARLA_SAFE_ASSERT_CONTINUE(fCallback != nullptr);
                    fCallback->handlePluginUIClosed();
                }
                break;

            case KeyRelease:
                if (event.xkey.keycode == X11Key_Escape)
                {
                    fIsVisible = false;
                    CARLA_SAFE_ASSERT_CONTINUE(fCallback != nullptr);
                    fCallback->handlePluginUIClosed();
                }
                break;
            }

            if (type != nullptr)
                XFree(type);
            else if (fEventProc != nullptr)
                fEventProc(&event);
        }

        fIsIdling = false;
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

        sizeHints.flags      = PSize|PMinSize|PMaxSize;
        sizeHints.width      = static_cast<int>(width);
        sizeHints.height     = static_cast<int>(height);
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

        XSetTransientForHint(fDisplay, fWindow, static_cast<Window>(winId));
    }

    void* getPtr() const noexcept
    {
        return (void*)fWindow;
    }

    void* getDisplay() const noexcept
    {
        return fDisplay;
    }

protected:
    Window getChildWindow() const
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0, 0);

        Window rootWindow, parentWindow, ret = 0;
        Window* childWindows = nullptr;
        uint numChildren = 0;

        XQueryTree(fDisplay, fWindow, &rootWindow, &parentWindow, &childWindows, &numChildren);

        if (numChildren > 0 && childWindows != nullptr)
        {
            ret = childWindows[0];
            XFree(childWindows);
        }

        return ret;
    }

private:
    Display* fDisplay;
    Window   fWindow;
    bool     fIsVisible;
    bool     fFirstShow;
    EventProcPtr fEventProc;

    CARLA_DECLARE_NON_COPY_CLASS(X11PluginUI)
};
#endif

// -----------------------------------------------------

bool CarlaPluginUI::tryTransientWinIdMatch(const uintptr_t pid, const char* const uiTitle, const uintptr_t winId, const bool centerUI)
{
    CARLA_SAFE_ASSERT_RETURN(uiTitle != nullptr && uiTitle[0] != '\0', true);
    CARLA_SAFE_ASSERT_RETURN(winId != 0, true);

#if defined(CARLA_OS_MAC)
    return true;
    (void)pid; (void)centerUI;
#elif defined(CARLA_OS_WIN)
    return true;
    (void)pid; (void)centerUI;
#elif defined(HAVE_X11)
    struct ScopedDisplay {
        Display* display;
        ScopedDisplay() : display(XOpenDisplay(nullptr)) {}
        ~ScopedDisplay() { if (display!=nullptr) XCloseDisplay(display); }
        // c++ compat stuff
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedDisplay)
    };
    struct ScopedFreeData {
        union {
            char* data;
            uchar* udata;
        };
        ScopedFreeData(char* d) : data(d) {}
        ScopedFreeData(uchar* d) : udata(d) {}
        ~ScopedFreeData() { XFree(data); }
        // c++ compat stuff
        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPY_CLASS(ScopedFreeData)
    };

    const ScopedDisplay sd;
    CARLA_SAFE_ASSERT_RETURN(sd.display != nullptr, true);

    const Window rootWindow(DefaultRootWindow(sd.display));

    const Atom _ncl = XInternAtom(sd.display, "_NET_CLIENT_LIST" , False);
    const Atom _nwn = XInternAtom(sd.display, "_NET_WM_NAME", False);
    const Atom _nwp = XInternAtom(sd.display, "_NET_WM_PID", False);
    const Atom utf8 = XInternAtom(sd.display, "UTF8_STRING", True);

    Atom actualType;
    int actualFormat;
    ulong numWindows, bytesAfter;
    uchar* data = nullptr;

    int status = XGetWindowProperty(sd.display, rootWindow, _ncl, 0L, (~0L), False, AnyPropertyType, &actualType, &actualFormat, &numWindows, &bytesAfter, &data);

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
            ulong  pidSize;
            uchar* pidData = nullptr;

            status = XGetWindowProperty(sd.display, window, _nwp, 0L, (~0L), False, XA_CARDINAL, &actualType, &actualFormat, &pidSize, &bytesAfter, &pidData);

            if (pidData != nullptr)
            {
                const ScopedFreeData sfd2(pidData);

                CARLA_SAFE_ASSERT_CONTINUE(status == Success);
                CARLA_SAFE_ASSERT_CONTINUE(pidSize != 0);

                if (*(ulong*)pidData == static_cast<ulong>(pid))
                {
                    CARLA_SAFE_ASSERT_RETURN(lastGoodWindow == window || lastGoodWindow == 0, true);
                    lastGoodWindow = window;
                    carla_stdout("Match found using pid");
                    break;
                }
            }
        }

        // ------------------------------------------------
        // try using name (UTF-8)

        ulong nameSize;
        uchar* nameData = nullptr;

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
                carla_stdout("Match found using UTF-8 name");
            }
        }

        // ------------------------------------------------
        // try using name (simple)

        char* wmName = nullptr;

        status = XFetchName(sd.display, window, &wmName);

        if (wmName != nullptr)
        {
            const ScopedFreeData sfd2(wmName);

            CARLA_SAFE_ASSERT_CONTINUE(status != 0);

            if (std::strstr(wmName, uiTitle) != nullptr)
            {
                CARLA_SAFE_ASSERT_RETURN(lastGoodWindow == window || lastGoodWindow == 0,  true);
                lastGoodWindow = window;
                carla_stdout("Match found using simple name");
            }
        }
    }

    if (lastGoodWindow == 0)
        return false;

    const Atom _nwt    = XInternAtom(sd.display ,"_NET_WM_STATE", False);
    const Atom _nws[2] = {
        XInternAtom(sd.display, "_NET_WM_STATE_SKIP_TASKBAR", False),
        XInternAtom(sd.display, "_NET_WM_STATE_SKIP_PAGER", False)
    };
    XChangeProperty(sd.display, lastGoodWindow, _nwt, XA_ATOM, 32, PropModeAppend, (const uchar*)_nws, 2);

    const Atom _nwi = XInternAtom(sd.display, "_NET_WM_ICON", False);
    XChangeProperty(sd.display, lastGoodWindow, _nwi, XA_CARDINAL, 32, PropModeReplace, (const uchar*)sCarlaX11Icon, sCarlaX11IconSize);

    const Window hostWinId((Window)winId);

    XSetTransientForHint(sd.display, lastGoodWindow, hostWinId);

    if (centerUI)
    {
        int hostX, hostY, pluginX, pluginY;
        uint hostWidth, hostHeight, pluginWidth, pluginHeight, border, depth;
        Window retWindow;

        if (XGetGeometry(sd.display, hostWinId,      &retWindow, &hostX,   &hostY,   &hostWidth,   &hostHeight,   &border, &depth) != 0 &&
            XGetGeometry(sd.display, lastGoodWindow, &retWindow, &pluginX, &pluginY, &pluginWidth, &pluginHeight, &border, &depth) != 0)
        {
            if (XTranslateCoordinates(sd.display, hostWinId,      rootWindow, hostX,   hostY,   &hostX,   &hostY,   &retWindow) == True &&
                XTranslateCoordinates(sd.display, lastGoodWindow, rootWindow, pluginX, pluginY, &pluginX, &pluginY, &retWindow) == True)
            {
                const int newX = hostX + int(hostWidth/2  - pluginWidth/2);
                const int newY = hostY + int(hostHeight/2 - pluginHeight/2);

                XMoveWindow(sd.display, lastGoodWindow, newX, newY);
            }
        }
    }

    // focusing the host UI and then the plugin UI forces the WM to repaint the plugin window icon
    XRaiseWindow(sd.display, hostWinId);
    XSetInputFocus(sd.display, hostWinId, RevertToPointerRoot, CurrentTime);

    XRaiseWindow(sd.display, lastGoodWindow);
    XSetInputFocus(sd.display, lastGoodWindow, RevertToPointerRoot, CurrentTime);

    XFlush(sd.display);
    return true;
#else
    return true;
    (void)pid;
#endif
}

// -----------------------------------------------------

#ifdef CARLA_OS_MAC
CarlaPluginUI* CarlaPluginUI::newCocoa(CloseCallback*, uintptr_t)
{
    //return new CocoaPluginUi(cb, parentId);
    return nullptr;
}
#endif

#ifdef CARLA_OS_WIN
CarlaPluginUI* CarlaPluginUI::newWindows(CloseCallback*, uintptr_t)
{
    //return new WindowsPluginUi(cb, parentId);
    return nullptr;
}
#endif

#ifdef HAVE_X11
CarlaPluginUI* CarlaPluginUI::newX11(CloseCallback* cb, uintptr_t parentId)
{
    return new X11PluginUI(cb, parentId);
}
#endif

// -----------------------------------------------------
