/*
 * Carla Plugin UI
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

#include "CarlaJuceUtils.hpp"
#include "CarlaPluginUI.hpp"

#ifdef HAVE_X11
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
#endif

#ifdef CARLA_OS_MAC
# import <Cocoa/Cocoa.h>
#endif

#ifdef CARLA_OS_WIN
# include <ctime>
#endif

// ---------------------------------------------------------------------------------------------------------------------
// X11

#ifdef HAVE_X11
# include "CarlaPluginUI_X11Icon.hpp"

typedef void (*EventProcPtr)(XEvent* ev);

static const uint X11Key_Escape = 9;
static bool gErrorTriggered = false;

static int temporaryErrorHandler(Display*, XErrorEvent*)
{
    gErrorTriggered = true;
    return 0;
}

class X11PluginUI : public CarlaPluginUI
{
public:
    X11PluginUI(Callback* const cb, const uintptr_t parentId, const bool isResizable) noexcept
        : CarlaPluginUI(cb, isResizable),
          fDisplay(nullptr),
          fHostWindow(0),
          fChildWindow(0),
          fIsVisible(false),
          fFirstShow(true),
          fSetSizeCalledAtLeastOnce(false),
          fEventProc(nullptr)
     {
        fDisplay = XOpenDisplay(nullptr);
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const int screen = DefaultScreen(fDisplay);

        XSetWindowAttributes attr;
        carla_zeroStruct(attr);

        attr.border_pixel = 0;
        attr.event_mask   = KeyPressMask|KeyReleaseMask;

        if (fIsResizable)
            attr.event_mask |= StructureNotifyMask;

        fHostWindow = XCreateWindow(fDisplay, RootWindow(fDisplay, screen),
                                    0, 0, 300, 300, 0,
                                    DefaultDepth(fDisplay, screen),
                                    InputOutput,
                                    DefaultVisual(fDisplay, screen),
                                    CWBorderPixel|CWEventMask, &attr);

        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XGrabKey(fDisplay, X11Key_Escape, AnyModifier, fHostWindow, 1, GrabModeAsync, GrabModeAsync);

        Atom wmDelete = XInternAtom(fDisplay, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(fDisplay, fHostWindow, &wmDelete, 1);

        const pid_t pid = getpid();
        const Atom _nwp = XInternAtom(fDisplay, "_NET_WM_PID", False);
        XChangeProperty(fDisplay, fHostWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

        const Atom _nwi = XInternAtom(fDisplay, "_NET_WM_ICON", False);
        XChangeProperty(fDisplay, fHostWindow, _nwi, XA_CARDINAL, 32, PropModeReplace, (const uchar*)sCarlaX11Icon, sCarlaX11IconSize);

        if (parentId != 0)
            setTransientWinId(parentId);
    }

    ~X11PluginUI() override
    {
        CARLA_SAFE_ASSERT(! fIsVisible);

        if (fIsVisible)
        {
            XUnmapWindow(fDisplay, fHostWindow);
            fIsVisible = false;
        }

        if (fHostWindow != 0)
        {
            XDestroyWindow(fDisplay, fHostWindow);
            fHostWindow = 0;
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
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        if (fFirstShow)
        {
            if (const Window childWindow = getChildWindow())
            {
                if (! fSetSizeCalledAtLeastOnce)
                {
                    XSizeHints hints;
                    carla_zeroStruct(hints);

                    if (XGetNormalHints(fDisplay, childWindow, &hints) && hints.width > 0 && hints.height > 0)
                        setSize(hints.width, hints.height, false);
                }

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

        XMapRaised(fDisplay, fHostWindow);
        XFlush(fDisplay);
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        fIsVisible = false;
        XUnmapWindow(fDisplay, fHostWindow);
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
            case ConfigureNotify:
                    CARLA_SAFE_ASSERT_CONTINUE(fCallback != nullptr);
                    CARLA_SAFE_ASSERT_CONTINUE(event.xconfigure.width > 0);
                    CARLA_SAFE_ASSERT_CONTINUE(event.xconfigure.height > 0);

                    {
                        const uint width  = static_cast<uint>(event.xconfigure.width);
                        const uint height = static_cast<uint>(event.xconfigure.height);

                        if (fChildWindow != 0)
                            XResizeWindow(fDisplay, fChildWindow, width, height);

                        fCallback->handlePluginUIResized(width, height);
                    }
                    break;

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
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XRaiseWindow(fDisplay, fHostWindow);
        XSetInputFocus(fDisplay, fHostWindow, RevertToPointerRoot, CurrentTime);
        XFlush(fDisplay);
    }

    void setSize(const uint width, const uint height, const bool forceUpdate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        fSetSizeCalledAtLeastOnce = true;
        XResizeWindow(fDisplay, fHostWindow, width, height);

        if (fChildWindow != 0)
            XResizeWindow(fDisplay, fChildWindow, width, height);

        if (! fIsResizable)
        {
            XSizeHints sizeHints;
            carla_zeroStruct(sizeHints);

            sizeHints.flags      = PSize|PMinSize|PMaxSize;
            sizeHints.width      = static_cast<int>(width);
            sizeHints.height     = static_cast<int>(height);
            sizeHints.min_width  = static_cast<int>(width);
            sizeHints.min_height = static_cast<int>(height);
            sizeHints.max_width  = static_cast<int>(width);
            sizeHints.max_height = static_cast<int>(height);

            XSetNormalHints(fDisplay, fHostWindow, &sizeHints);
        }

        if (forceUpdate)
            XFlush(fDisplay);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XStoreName(fDisplay, fHostWindow, title);
    }

    void setTransientWinId(const uintptr_t winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XSetTransientForHint(fDisplay, fHostWindow, static_cast<Window>(winId));
    }

    void setChildWindow(void* const winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(winId != nullptr,);

        fChildWindow = (Window)winId;
    }

    void* getPtr() const noexcept override
    {
        return (void*)fHostWindow;
    }

    void* getDisplay() const noexcept override
    {
        return fDisplay;
    }

protected:
    Window getChildWindow() const
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr, 0);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0, 0);

        Window rootWindow, parentWindow, ret = 0;
        Window* childWindows = nullptr;
        uint numChildren = 0;

        XQueryTree(fDisplay, fHostWindow, &rootWindow, &parentWindow, &childWindows, &numChildren);

        if (numChildren > 0 && childWindows != nullptr)
        {
            ret = childWindows[0];
            XFree(childWindows);
        }

        return ret;
    }

private:
    Display* fDisplay;
    Window   fHostWindow;
    Window   fChildWindow;
    bool     fIsVisible;
    bool     fFirstShow;
    bool     fSetSizeCalledAtLeastOnce;
    EventProcPtr fEventProc;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(X11PluginUI)
};
#endif // HAVE_X11

// ---------------------------------------------------------------------------------------------------------------------
// MacOS / Cocoa

#ifdef CARLA_OS_MAC

@interface CarlaPluginWindow : NSWindow
{
@public
    CarlaPluginUI::Callback* callback;
    NSView* view;
}

- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(unsigned int)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag;
- (void) setup:(CarlaPluginUI::Callback*)cb view:(NSView*)v;
- (BOOL) canBecomeKeyWindow;
- (BOOL) windowShouldClose:(id)sender;
- (NSSize) windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;
@end

@implementation CarlaPluginWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned int)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
    callback = nil;
    view = nil;

    NSWindow* result = [super initWithContentRect:contentRect
                                        styleMask:(NSClosableWindowMask |
                                                   NSTitledWindowMask |
                                                   NSResizableWindowMask)
                                          backing:NSBackingStoreBuffered defer:NO];

    [result setAcceptsMouseMovedEvents:YES];
    [result setContentSize:NSMakeSize(1, 1)];
    [result setIsVisible:NO];

    return (CarlaPluginWindow*)result;

    // unused
    (void)aStyle; (void)bufferingType; (void)flag;
}

- (void)setup:(CarlaPluginUI::Callback*)cb view:(NSView*)v
{
    callback = cb;
    view = v;
}

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)windowShouldClose:(id)sender
{
    if (callback != nil)
        callback->handlePluginUIClosed();

    return NO;

    // unused
    (void)sender;
}

- (NSSize) windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
    if (callback != nil)
        callback->handlePluginUIResized(frameSize.width, frameSize.height);

    return frameSize;

    // unused
    (void)sender;
}

@end

class CocoaPluginUI : public CarlaPluginUI
{
public:
    CocoaPluginUI(Callback* const cb, const uintptr_t parentId, const bool isResizable) noexcept
        : CarlaPluginUI(cb, isResizable),
          fView(nullptr),
          fWindow(0)
    {
        [NSAutoreleasePool new];
        [NSApplication sharedApplication];

        fView = [NSView new];
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,)

        if (isResizable)
            [fView setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];

        fWindow = [[CarlaPluginWindow new]retain];

        if (fWindow == 0)
        {
            [fView release];
            fView = nullptr;
            return;
        }

        if (! isResizable)
            [[fWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];

        [fWindow setup:cb view:fView];
        [fWindow setContentView:fView];
        [fWindow makeFirstResponder:fView];
        [fWindow makeKeyAndOrderFront:fWindow];

        [NSApp activateIgnoringOtherApps:YES];
        [fWindow center];

        if (parentId != 0)
            setTransientWinId(parentId);
     }

    ~CocoaPluginUI() override
    {
        if (fView == nullptr)
            return;

        [fWindow close];
        [fView release];
        [fWindow release];
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fView setHidden:NO];
        [fWindow setIsVisible:YES];
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fWindow setIsVisible:NO];
        [fView setHidden:YES];
    }

    void idle() override
    {
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        [fWindow makeKeyWindow];
    }

    void setSize(const uint width, const uint height, const bool forceUpdate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fView setFrame:NSMakeRect(0, 0, width, height)];

        const NSSize size = NSMakeSize(width, height);
        [fWindow setContentSize:size];

        if (fIsResizable)
        {
            [fWindow setContentMinSize:NSMakeSize(1, 1)];
            [fWindow setContentMaxSize:NSMakeSize(99999, 99999)];
            [[fWindow standardWindowButton:NSWindowZoomButton] setHidden:NO];
        }
        else
        {
            [fWindow setContentMinSize:size];
            [fWindow setContentMaxSize:size];
            [[fWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
        }
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        NSString* titleString = [[NSString alloc]
                                  initWithBytes:title
                                         length:strlen(title)
                                       encoding:NSUTF8StringEncoding];

        [fWindow setTitle:titleString];
    }

    void setTransientWinId(const uintptr_t winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:winId];
        CARLA_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        [parentWindow addChildWindow:fWindow
                             ordered:NSWindowAbove];
    }

    void setChildWindow(void* const winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(winId != nullptr,);
    }

    void* getPtr() const noexcept override
    {
        return (void*)fView;
    }

    void* getDisplay() const noexcept
    {
        return (void*)fWindow;
    }

private:
    NSView* fView;
    id      fWindow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CocoaPluginUI)
};

#endif // CARLA_OS_MAC

// ---------------------------------------------------------------------------------------------------------------------
// Windows

#ifdef CARLA_OS_WIN

#define PUGL_LOCAL_CLOSE_MSG (WM_USER + 50)

static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class WindowsPluginUI : public CarlaPluginUI
{
public:
    WindowsPluginUI(Callback* const cb, const uintptr_t parentId, const bool isResizable) noexcept
        : CarlaPluginUI(cb, isResizable),
          fWindow(nullptr),
          fParentWindow(nullptr),
          fIsVisible(false),
          fFirstShow(true)
     {
        // FIXME
        static int wc_count = 0;
        char classNameBuf[256];
        std::srand((std::time(NULL)));
        _snprintf(classNameBuf, sizeof(classNameBuf), "CaWin_%d-%d", std::rand(), ++wc_count);
        classNameBuf[sizeof(classNameBuf)-1] = '\0';

        const HINSTANCE hInstance = GetModuleHandleA(nullptr);

        carla_zeroStruct(fWindowClass);
        fWindowClass.style         = CS_OWNDC;
        fWindowClass.lpfnWndProc   = wndProc;
        fWindowClass.hInstance     = hInstance;
        fWindowClass.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
        fWindowClass.hCursor       = LoadCursor(hInstance, IDC_ARROW);
        fWindowClass.lpszClassName = strdup(classNameBuf);

        if (!RegisterClass(&fWindowClass)) {
            free((void*)fWindowClass.lpszClassName);
            return;
        }

        int winFlags = WS_POPUPWINDOW | WS_CAPTION;
        if (isResizable)
            winFlags |= WS_SIZEBOX;

        fWindow = CreateWindowEx(WS_EX_TOPMOST,
                                 classNameBuf, "Carla Plugin UI", winFlags,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 NULL, NULL, hInstance, NULL);

        if (! fWindow) {
            UnregisterClass(fWindowClass.lpszClassName, NULL);
            free((void*)fWindowClass.lpszClassName);
            return;
        }

        SetWindowLongPtr(fWindow, GWLP_USERDATA, (LONG_PTR)this);

        if (parentId != 0)
            setTransientWinId(parentId);
     }

    ~WindowsPluginUI() override
    {
        CARLA_SAFE_ASSERT(! fIsVisible);

        if (fWindow != 0)
        {
            if (fIsVisible)
                ShowWindow(fWindow, SW_HIDE);

            DestroyWindow(fWindow);
            fWindow = 0;
        }

        // FIXME
        UnregisterClass(fWindowClass.lpszClassName, NULL);
        free((void*)fWindowClass.lpszClassName);
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        if (fFirstShow)
        {
            fFirstShow = false;
            RECT rectChild, rectParent;

            if (fParentWindow != nullptr &&
                GetWindowRect(fWindow, &rectChild) &&
                GetWindowRect(fParentWindow, &rectParent))
            {
                SetWindowPos(fWindow, fParentWindow,
                             rectParent.left + (rectChild.right-rectChild.left)/2,
                             rectParent.top + (rectChild.bottom-rectChild.top)/2,
                             0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
            }
            else
            {
                ShowWindow(fWindow, SW_SHOWNORMAL);
            }
        }
        else
        {
            ShowWindow(fWindow, SW_RESTORE);
        }

        fIsVisible = true;
        UpdateWindow(fWindow);
    }

    void hide() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        ShowWindow(fWindow, SW_HIDE);
        fIsVisible = false;
        UpdateWindow(fWindow);
    }

    void idle() override
    {
        if (fIsIdling || fWindow == 0)
            return;

        MSG msg;
        fIsIdling = true;

        while (::PeekMessage(&msg, fWindow, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
            case WM_QUIT:
            case PUGL_LOCAL_CLOSE_MSG:
                fIsVisible = false;
                CARLA_SAFE_ASSERT_BREAK(fCallback != nullptr);
                fCallback->handlePluginUIClosed();
                break;
            }

            DispatchMessageA(&msg);
        }

        fIsIdling = false;
    }

    LRESULT checkAndHandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (fWindow == hwnd)
        {
            switch (message)
            {
            case WM_QUIT:
            case PUGL_LOCAL_CLOSE_MSG:
                    fIsVisible = false;
                    CARLA_SAFE_ASSERT_BREAK(fCallback != nullptr);
                    fCallback->handlePluginUIClosed();
                    break;
            }
        }

        return DefWindowProcA(hwnd, message, wParam, lParam);
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        SetForegroundWindow(fWindow);
        SetActiveWindow(fWindow);
        SetFocus(fWindow);
    }

    void setSize(const uint width, const uint height, const bool forceUpdate) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        const int winFlags = WS_POPUPWINDOW | WS_CAPTION | (fIsResizable ? WS_SIZEBOX : 0x0);
        RECT wr = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
        AdjustWindowRectEx(&wr, winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(fWindow, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top,
                     SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);

        if (forceUpdate)
            UpdateWindow(fWindow);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        SetWindowTextA(fWindow, title);
    }

    void setTransientWinId(const uintptr_t winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != 0,);

        fParentWindow = (HWND)winId;
        SetWindowLongPtr(fWindow, GWLP_HWNDPARENT, (LONG_PTR)winId);
    }

    void setChildWindow(void* const winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(winId != nullptr,);
    }

    void* getPtr() const noexcept override
    {
        return (void*)fWindow;
    }

    void* getDisplay() const noexcept
    {
        return nullptr;
    }

private:
    HWND     fWindow;
    HWND     fParentWindow;
    WNDCLASS fWindowClass;

    bool fIsVisible;
    bool fFirstShow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowsPluginUI)
};

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        PostMessage(hwnd, PUGL_LOCAL_CLOSE_MSG, wParam, lParam);
        return 0;

#if 0
    case WM_CREATE:
        PostMessage(hwnd, WM_SHOWWINDOW, TRUE, 0);
        return 0;

    case WM_DESTROY:
        return 0;
#endif

    default:
        if (WindowsPluginUI* const ui = (WindowsPluginUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA))
            return ui->checkAndHandleMessage(hwnd, message, wParam, lParam);
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }
}
#endif // CARLA_OS_WIN

// -----------------------------------------------------

bool CarlaPluginUI::tryTransientWinIdMatch(const uintptr_t pid, const char* const uiTitle, const uintptr_t winId, const bool centerUI)
{
    CARLA_SAFE_ASSERT_RETURN(uiTitle != nullptr && uiTitle[0] != '\0', true);
    CARLA_SAFE_ASSERT_RETURN(winId != 0, true);

#if defined(HAVE_X11)
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
    Window  lastGoodWindowPID = 0, lastGoodWindowNameSimple = 0, lastGoodWindowNameUTF8 = 0;

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
                    lastGoodWindowPID = window;
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

            if (nameSize != 0 && std::strstr((const char*)nameData, uiTitle) != nullptr)
                lastGoodWindowNameUTF8 = window;
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
                lastGoodWindowNameSimple = window;
        }
    }

    if (lastGoodWindowPID == 0 && lastGoodWindowNameSimple == 0 && lastGoodWindowNameUTF8 == 0)
        return false;

    Window windowToMap;

    if (lastGoodWindowPID != 0)
    {
        if (lastGoodWindowPID == lastGoodWindowNameSimple && lastGoodWindowPID == lastGoodWindowNameUTF8)
        {
            carla_stdout("Match found using pid, simple and UTF-8 name all at once, nice!");
            windowToMap = lastGoodWindowPID;
        }
        else if (lastGoodWindowPID == lastGoodWindowNameUTF8)
        {
            carla_stdout("Match found using pid and UTF-8 name");
            windowToMap = lastGoodWindowPID;
        }
        else if (lastGoodWindowPID == lastGoodWindowNameSimple)
        {
            carla_stdout("Match found using pid and simple name");
            windowToMap = lastGoodWindowPID;
        }
        else
        {
            carla_stdout("Match found using pid");
            windowToMap = lastGoodWindowPID;
        }
    }
    else if (lastGoodWindowNameUTF8 != 0)
    {
        if (lastGoodWindowNameUTF8 == lastGoodWindowNameSimple)
        {
            carla_stdout("Match found using simple and UTF-8 name");
            windowToMap = lastGoodWindowNameUTF8;
        }
        else
        {
            carla_stdout("Match found using simple and UTF-8 name");
            windowToMap = lastGoodWindowNameUTF8;
        }
    }
    else
    {
        carla_stdout("Match found using simple name");
        windowToMap = lastGoodWindowNameSimple;
    }

    const Atom _nwt    = XInternAtom(sd.display ,"_NET_WM_STATE", False);
    const Atom _nws[2] = {
        XInternAtom(sd.display, "_NET_WM_STATE_SKIP_TASKBAR", False),
        XInternAtom(sd.display, "_NET_WM_STATE_SKIP_PAGER", False)
    };
    XChangeProperty(sd.display, windowToMap, _nwt, XA_ATOM, 32, PropModeAppend, (const uchar*)_nws, 2);

    const Atom _nwi = XInternAtom(sd.display, "_NET_WM_ICON", False);
    XChangeProperty(sd.display, windowToMap, _nwi, XA_CARDINAL, 32, PropModeReplace, (const uchar*)sCarlaX11Icon, sCarlaX11IconSize);

    const Window hostWinId((Window)winId);

    XSetTransientForHint(sd.display, windowToMap, hostWinId);

    if (centerUI && false /* moving the window after being shown isn't pretty... */)
    {
        int hostX, hostY, pluginX, pluginY;
        uint hostWidth, hostHeight, pluginWidth, pluginHeight, border, depth;
        Window retWindow;

        if (XGetGeometry(sd.display, hostWinId,   &retWindow, &hostX,   &hostY,   &hostWidth,   &hostHeight,   &border, &depth) != 0 &&
            XGetGeometry(sd.display, windowToMap, &retWindow, &pluginX, &pluginY, &pluginWidth, &pluginHeight, &border, &depth) != 0)
        {
            if (XTranslateCoordinates(sd.display, hostWinId,   rootWindow, hostX,   hostY,   &hostX,   &hostY,   &retWindow) == True &&
                XTranslateCoordinates(sd.display, windowToMap, rootWindow, pluginX, pluginY, &pluginX, &pluginY, &retWindow) == True)
            {
                const int newX = hostX + int(hostWidth/2  - pluginWidth/2);
                const int newY = hostY + int(hostHeight/2 - pluginHeight/2);

                XMoveWindow(sd.display, windowToMap, newX, newY);
            }
        }
    }

    // focusing the host UI and then the plugin UI forces the WM to repaint the plugin window icon
    XRaiseWindow(sd.display, hostWinId);
    XSetInputFocus(sd.display, hostWinId, RevertToPointerRoot, CurrentTime);

    XRaiseWindow(sd.display, windowToMap);
    XSetInputFocus(sd.display, windowToMap, RevertToPointerRoot, CurrentTime);

    XFlush(sd.display);
    return true;
#endif

#ifdef CARLA_OS_MAC
    // TODO
#endif

#ifdef CARLA_OS_WIN
    if (HWND const childWindow = FindWindowA(nullptr, uiTitle))
    {
        HWND const parentWindow = (HWND)winId;
        SetWindowLongPtr(childWindow, GWLP_HWNDPARENT, (LONG_PTR)parentWindow);

        if (centerUI)
        {
            RECT rectChild, rectParent;

            if (GetWindowRect(childWindow, &rectChild) && GetWindowRect(parentWindow, &rectParent))
            {
                SetWindowPos(childWindow, parentWindow,
                             rectParent.left + (rectChild.right-rectChild.left)/2,
                             rectParent.top + (rectChild.bottom-rectChild.top)/2,
                             0, 0, SWP_NOSIZE);
            }
        }

        carla_stdout("Match found using window name");
        return true;
    }

    return false;
#endif

    // fallback, may be unused
    return true;
    (void)pid; (void)centerUI;
}

// -----------------------------------------------------

#ifdef HAVE_X11
CarlaPluginUI* CarlaPluginUI::newX11(Callback* cb, uintptr_t parentId, bool isResizable)
{
    return new X11PluginUI(cb, parentId, isResizable);
}
#endif

#ifdef CARLA_OS_MAC
CarlaPluginUI* CarlaPluginUI::newCocoa(Callback* cb, uintptr_t parentId, bool isResizable)
{
    return new CocoaPluginUI(cb, parentId, isResizable);
}
#endif

#ifdef CARLA_OS_WIN
CarlaPluginUI* CarlaPluginUI::newWindows(Callback* cb, uintptr_t parentId, bool isResizable)
{
    return new WindowsPluginUI(cb, parentId, isResizable);
}
#endif

// -----------------------------------------------------
