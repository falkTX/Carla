/*
 * Carla Plugin UI
 * Copyright (C) 2014-2022 Filipe Coelho <falktx@falktx.com>
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
# include <pthread.h>
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include "CarlaPluginUI_X11Icon.hpp"
#endif

#ifdef CARLA_OS_MAC
# include "CarlaMacUtils.hpp"
# import <Cocoa/Cocoa.h>
#endif

#ifdef CARLA_OS_WIN
# include <ctime>
# include "water/common.hpp"
#endif

#ifndef CARLA_PLUGIN_UI_CLASS_PREFIX
# error CARLA_PLUGIN_UI_CLASS_PREFIX undefined
#endif

// ---------------------------------------------------------------------------------------------------------------------
// X11

#ifdef HAVE_X11
static constexpr const uint X11Key_Escape = 9;

typedef void (*EventProcPtr)(XEvent* ev);

// FIXME put all this inside a scoped class
static bool gErrorTriggered = false;
# if defined(__GNUC__) && (__GNUC__ >= 5) && ! defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
# endif
static pthread_mutex_t gErrorMutex = PTHREAD_MUTEX_INITIALIZER;
# if defined(__GNUC__) && (__GNUC__ >= 5) && ! defined(__clang__)
#  pragma GCC diagnostic pop
# endif

static int temporaryErrorHandler(Display*, XErrorEvent*)
{
    gErrorTriggered = true;
    return 0;
}

class X11PluginUI : public CarlaPluginUI
{
public:
    X11PluginUI(Callback* const cb, const uintptr_t parentId,
                const bool isStandalone, const bool isResizable, const bool canMonitorChildren) noexcept
        : CarlaPluginUI(cb, isStandalone, isResizable),
          fDisplay(nullptr),
          fHostWindow(0),
          fChildWindow(0),
          fChildWindowConfigured(false),
          fChildWindowMonitoring(isResizable || canMonitorChildren),
          fIsVisible(false),
          fFirstShow(true),
          fSetSizeCalledAtLeastOnce(false),
          fMinimumWidth(0),
          fMinimumHeight(0),
          fEventProc(nullptr)
     {
        fDisplay = XOpenDisplay(nullptr);
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const int screen = DefaultScreen(fDisplay);

        XSetWindowAttributes attr;
        carla_zeroStruct(attr);

        attr.event_mask = KeyPressMask|KeyReleaseMask|FocusChangeMask;

        if (fChildWindowMonitoring)
            attr.event_mask |= StructureNotifyMask|SubstructureNotifyMask;

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

        const Atom _wt = XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE", False);

        // Setting the window to both dialog and normal will produce a decorated floating dialog
        // Order is important: DIALOG needs to come before NORMAL
        const Atom _wts[2] = {
            XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE_DIALOG", False),
            XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", False)
        };
        XChangeProperty(fDisplay, fHostWindow, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, 2);

        if (parentId != 0)
            setTransientWinId(parentId);
    }

    ~X11PluginUI() override
    {
        CARLA_SAFE_ASSERT(! fIsVisible);

        if (fDisplay == nullptr)
            return;

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

        XCloseDisplay(fDisplay);
        fDisplay = nullptr;
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
                    int width = 0;
                    int height = 0;

                    XWindowAttributes attrs = {};

                    pthread_mutex_lock(&gErrorMutex);
                    const XErrorHandler oldErrorHandler = XSetErrorHandler(temporaryErrorHandler);
                    gErrorTriggered = false;

                    if (XGetWindowAttributes(fDisplay, childWindow, &attrs))
                    {
                        width = attrs.width;
                        height = attrs.height;
                    }

                    XSetErrorHandler(oldErrorHandler);
                    pthread_mutex_unlock(&gErrorMutex);

                    if (width == 0 && height == 0)
                    {
                        XSizeHints sizeHints = {};

                        if (XGetNormalHints(fDisplay, childWindow, &sizeHints))
                        {
                            if (sizeHints.flags & PSize)
                            {
                                width = sizeHints.width;
                                height = sizeHints.height;
                            }
                            else if (sizeHints.flags & PBaseSize)
                            {
                                width = sizeHints.base_width;
                                height = sizeHints.base_height;
                            }
                        }
                    }

                    if (width > 1 && height > 1)
                        setSize(static_cast<uint>(width), static_cast<uint>(height), false, false);
                }

                const Atom _xevp = XInternAtom(fDisplay, "_XEventProc", False);

                pthread_mutex_lock(&gErrorMutex);
                const XErrorHandler oldErrorHandler(XSetErrorHandler(temporaryErrorHandler));
                gErrorTriggered = false;

                Atom actualType;
                int actualFormat;
                ulong nitems, bytesAfter;
                uchar* data = nullptr;

                XGetWindowProperty(fDisplay, childWindow, _xevp, 0, 1, False, AnyPropertyType,
                                   &actualType, &actualFormat, &nitems, &bytesAfter, &data);

                XSetErrorHandler(oldErrorHandler);
                pthread_mutex_unlock(&gErrorMutex);

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
        XSync(fDisplay, False);
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

        uint nextChildWidth = 0;
        uint nextChildHeight = 0;

        uint nextHostWidth = 0;
        uint nextHostHeight = 0;

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

                    if (event.xconfigure.window == fHostWindow && fHostWindow != 0)
                    {
                        nextHostWidth = static_cast<uint>(event.xconfigure.width);
                        nextHostHeight = static_cast<uint>(event.xconfigure.height);
                    }
                    else if (event.xconfigure.window == fChildWindow && fChildWindow != 0)
                    {
                        nextChildWidth = static_cast<uint>(event.xconfigure.width);
                        nextChildHeight = static_cast<uint>(event.xconfigure.height);
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

            case FocusIn:
                if (fChildWindow == 0)
                    fChildWindow = getChildWindow();

                if (fChildWindow != 0)
                {
                    XWindowAttributes wa;
                    carla_zeroStruct(wa);

                    if (XGetWindowAttributes(fDisplay, fChildWindow, &wa) && wa.map_state == IsViewable)
                        XSetInputFocus(fDisplay, fChildWindow, RevertToPointerRoot, CurrentTime);
                }
                break;
            }

            if (type != nullptr)
                XFree(type);
            else if (fEventProc != nullptr && event.type != FocusIn && event.type != FocusOut)
                fEventProc(&event);
        }

        if (nextChildWidth != 0 && nextChildHeight != 0 && fChildWindow != 0)
        {
            applyHintsFromChildWindow();
            XResizeWindow(fDisplay, fHostWindow, nextChildWidth, nextChildHeight);
            // XFlush(fDisplay);
        }
        else if (nextHostWidth != 0 && nextHostHeight != 0)
        {
            if (fChildWindow != 0 && ! fChildWindowConfigured)
            {
                applyHintsFromChildWindow();
                fChildWindowConfigured = true;
            }

            if (fChildWindow != 0)
                XResizeWindow(fDisplay, fChildWindow, nextHostWidth, nextHostHeight);

            fCallback->handlePluginUIResized(nextHostWidth, nextHostHeight);
        }

        fIsIdling = false;
    }

    void focus() override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XWindowAttributes wa;
        carla_zeroStruct(wa);

        CARLA_SAFE_ASSERT_RETURN(XGetWindowAttributes(fDisplay, fHostWindow, &wa),);

        if (wa.map_state == IsViewable)
        {
            XRaiseWindow(fDisplay, fHostWindow);
            XSetInputFocus(fDisplay, fHostWindow, RevertToPointerRoot, CurrentTime);
            XSync(fDisplay, False);
        }
    }

    void setMinimumSize(const uint width, const uint height) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        fMinimumWidth = width;
        fMinimumHeight = height;

        XSizeHints sizeHints = {};
        if (XGetNormalHints(fDisplay, fHostWindow, &sizeHints))
        {
            sizeHints.flags     |= PMinSize;
            sizeHints.min_width  = static_cast<int>(width);
            sizeHints.min_height = static_cast<int>(height);
            XSetNormalHints(fDisplay, fHostWindow, &sizeHints);
        }
    }

    void setSize(const uint width, const uint height, const bool forceUpdate, const bool resizeChild) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        fSetSizeCalledAtLeastOnce = true;
        XResizeWindow(fDisplay, fHostWindow, width, height);

        if (fChildWindow != 0 && resizeChild)
            XResizeWindow(fDisplay, fChildWindow, width, height);

        if (! fIsResizable)
        {
            XSizeHints sizeHints = {};
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
            XSync(fDisplay, False);
    }

    void setTitle(const char* const title) override
    {
        CARLA_SAFE_ASSERT_RETURN(fDisplay != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fHostWindow != 0,);

        XStoreName(fDisplay, fHostWindow, title);

        const Atom _nwn = XInternAtom(fDisplay, "_NET_WM_NAME", False);
        const Atom utf8 = XInternAtom(fDisplay, "UTF8_STRING", True);

        XChangeProperty(fDisplay, fHostWindow, _nwn, utf8, 8,
                        PropModeReplace,
                        (const uchar*)(title),
                        (int)strlen(title));
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
    void applyHintsFromChildWindow()
    {
        pthread_mutex_lock(&gErrorMutex);
        const XErrorHandler oldErrorHandler = XSetErrorHandler(temporaryErrorHandler);
        gErrorTriggered = false;

        XSizeHints sizeHints = {};
        if (XGetNormalHints(fDisplay, fChildWindow, &sizeHints) && !gErrorTriggered)
        {
            if (fMinimumWidth != 0 && fMinimumHeight != 0)
            {
                sizeHints.flags |= PMinSize;
                sizeHints.min_width = fMinimumWidth;
                sizeHints.min_height = fMinimumHeight;
            }

            XSetNormalHints(fDisplay, fHostWindow, &sizeHints);
        }

        if (gErrorTriggered)
        {
            carla_stdout("Caught errors while accessing child window");
            fChildWindow = 0;
        }

        XSetErrorHandler(oldErrorHandler);
        pthread_mutex_unlock(&gErrorMutex);
    }

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
    bool     fChildWindowConfigured;
    bool     fChildWindowMonitoring;
    bool     fIsVisible;
    bool     fFirstShow;
    bool     fSetSizeCalledAtLeastOnce;
    uint     fMinimumWidth;
    uint     fMinimumHeight;
    EventProcPtr fEventProc;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(X11PluginUI)
};
#endif // HAVE_X11

// ---------------------------------------------------------------------------------------------------------------------
// MacOS / Cocoa

#ifdef CARLA_OS_MAC

#if defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
# define CarlaPluginWindow CARLA_JOIN_MACRO3(CarlaPluginWindowBridgedArch, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
# define CarlaPluginWindowDelegate CARLA_JOIN_MACRO3(CarlaPluginWindowDelegateBridgedArch, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
#elif defined(BUILD_BRIDGE)
# define CarlaPluginWindow CARLA_JOIN_MACRO3(CarlaPluginWindowBridged, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
# define CarlaPluginWindowDelegate CARLA_JOIN_MACRO3(CarlaPluginWindowDelegateBridged, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
#else
# define CarlaPluginWindow CARLA_JOIN_MACRO3(CarlaPluginWindow, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
# define CarlaPluginWindowDelegate CARLA_JOIN_MACRO3(CarlaPluginWindowDelegate, CARLA_BACKEND_NAMESPACE, CARLA_PLUGIN_UI_CLASS_PREFIX)
#endif

@interface CarlaPluginWindow : NSWindow
- (id) initWithContentRect:(NSRect)contentRect
                 styleMask:(unsigned int)aStyle
                   backing:(NSBackingStoreType)bufferingType
                     defer:(BOOL)flag;
- (BOOL) canBecomeKeyWindow;
- (BOOL) canBecomeMainWindow;
@end

@implementation CarlaPluginWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned int)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
    NSWindow* result = [super initWithContentRect:contentRect
                                        styleMask:aStyle
                                          backing:bufferingType
                                            defer:flag];

    [result setAcceptsMouseMovedEvents:YES];

    return (CarlaPluginWindow*)result;

    // unused
    (void)flag;
}

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return NO;
}

@end

@interface CarlaPluginWindowDelegate : NSObject<NSWindowDelegate>
{
    CarlaPluginUI::Callback* callback;
    CarlaPluginWindow* window;
}
- (instancetype)initWithWindowAndCallback:(CarlaPluginWindow*)window
                                 callback:(CarlaPluginUI::Callback*)callback2;
- (BOOL)windowShouldClose:(id)sender;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;
@end

@implementation CarlaPluginWindowDelegate

- (instancetype)initWithWindowAndCallback:(CarlaPluginWindow*)window2
                                 callback:(CarlaPluginUI::Callback*)callback2
{
    if ((self = [super init]))
    {
        callback = callback2;
        window = window2;
    }

  return self;
}

- (BOOL)windowShouldClose:(id)sender
{
    if (callback != nil)
        callback->handlePluginUIClosed();

    return NO;

    // unused
    (void)sender;
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
    for (NSView* subview in [[window contentView] subviews])
    {
        const NSSize minSize = [subview fittingSize];
        if (frameSize.width < minSize.width)
            frameSize.width = minSize.width;
        if (frameSize.height < minSize.height)
            frameSize.height = minSize.height;
        break;
    }

    return frameSize;
}

/*
- (void)windowDidResize:(NSWindow*)sender
{
    carla_stdout("window did resize %p %f %f", sender, [window frame].size.width, [window frame].size.height);

    const NSSize size = [window frame].size;
    NSView* const view = [window contentView];

    for (NSView* subview in [view subviews])
    {
        [subview setFrameSize:size];
        break;
    }
}
*/

@end

class CocoaPluginUI : public CarlaPluginUI
{
public:
    CocoaPluginUI(Callback* const callback, const uintptr_t parentId, const bool isStandalone, const bool isResizable) noexcept
        : CarlaPluginUI(callback, isStandalone, isResizable),
          fView(nullptr),
          fParentWindow(nullptr),
          fWindow(nullptr)
    {
        carla_debug("CocoaPluginUI::CocoaPluginUI(%p, " P_UINTPTR, "%s)", callback, parentId, bool2str(isResizable));
        const CARLA_BACKEND_NAMESPACE::AutoNSAutoreleasePool arp;
        [NSApplication sharedApplication];

        fView = [[NSView new]retain];
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,)

       #ifdef __MAC_10_12
        uint style = NSWindowStyleMaskClosable | NSWindowStyleMaskTitled;
       #else
        uint style = NSClosableWindowMask | NSTitledWindowMask;
       #endif
        /*
        if (isResizable)
            style |= NSResizableWindowMask;
        */

        const NSRect frame = NSMakeRect(0, 0, 100, 100);

        fWindow = [[[CarlaPluginWindow alloc]
            initWithContentRect:frame
                      styleMask:style
                        backing:NSBackingStoreBuffered
                          defer:NO
                    ] retain];

        if (fWindow == nullptr)
        {
            [fView release];
            fView = nullptr;
            return;
        }

        ((NSWindow*)fWindow).delegate = [[[CarlaPluginWindowDelegate alloc]
            initWithWindowAndCallback:fWindow
                             callback:callback] retain];

        /*
        if (isResizable)
        {
            [fView setAutoresizingMask:(NSViewWidthSizable |
                                        NSViewHeightSizable |
                                        NSViewMinXMargin |
                                        NSViewMaxXMargin |
                                        NSViewMinYMargin |
                                        NSViewMaxYMargin)];
            [fView setAutoresizesSubviews:YES];
        }
        else
        */
        {
            [fView setAutoresizingMask:NSViewNotSizable];
            [fView setAutoresizesSubviews:NO];
            [[fWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
        }

        [fWindow setContentView:fView];
        [fWindow makeFirstResponder:fView];

        [fView setHidden:NO];

        if (parentId != 0)
            setTransientWinId(parentId);
     }

    ~CocoaPluginUI() override
    {
        carla_debug("CocoaPluginUI::~CocoaPluginUI()");
        if (fView == nullptr)
            return;

        [fView setHidden:YES];
        [fView removeFromSuperview];
        [fWindow close];
        [fView release];
        [fWindow release];
    }

    void show() override
    {
        carla_debug("CocoaPluginUI::show()");
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        if (fParentWindow != nullptr)
        {
            [fParentWindow addChildWindow:fWindow
                                  ordered:NSWindowAbove];
        }
        else
        {
            [fWindow setIsVisible:YES];
        }
    }

    void hide() override
    {
        carla_debug("CocoaPluginUI::hide()");
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fWindow setIsVisible:NO];

        if (fParentWindow != nullptr)
            [fParentWindow removeChildWindow:fWindow];
    }

    void idle() override
    {
        // carla_debug("CocoaPluginUI::idle()");

        for (NSView* subview in [fView subviews])
        {
            const NSSize viewSize = [fView frame].size;
            const NSSize subviewSize = [subview frame].size;

            if (viewSize.width != subviewSize.width || viewSize.height != subviewSize.height)
            {
                [fView setFrameSize:subviewSize];
                [fWindow setContentSize:subviewSize];
            }
            break;
        }
    }

    void focus() override
    {
        carla_debug("CocoaPluginUI::focus()");
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        [fWindow makeKeyAndOrderFront:fWindow];
        [fWindow orderFrontRegardless];
        [NSApp activateIgnoringOtherApps:YES];
    }

    void setMinimumSize(uint, uint) override
    {
        // TODO
    }

    void setSize(const uint width, const uint height, const bool forceUpdate, const bool resizeChild) override
    {
        carla_debug("CocoaPluginUI::setSize(%u, %u, %s)", width, height, bool2str(forceUpdate));
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fView != nullptr,);

        const NSSize size = NSMakeSize(width, height);

        [fView setFrameSize:size];
        [fWindow setContentSize:size];

        // this is needed for a few plugins
        if (forceUpdate && resizeChild)
        {
            for (NSView* subview in [fView subviews])
            {
                [subview setFrame:[fView frame]];
                break;
            }
        }

        /*
        if (fIsResizable)
        {
            [fWindow setContentMinSize:NSMakeSize(1, 1)];
            [fWindow setContentMaxSize:NSMakeSize(99999, 99999)];
        }
        else
        {
            [fWindow setContentMinSize:size];
            [fWindow setContentMaxSize:size];
        }
        */

        if (forceUpdate)
        {
            // FIXME, not enough
            [fView setNeedsDisplay:YES];
        }
    }

    void setTitle(const char* const title) override
    {
        carla_debug("CocoaPluginUI::setTitle(\"%s\")", title);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        NSString* titleString = [[NSString alloc]
                                  initWithBytes:title
                                         length:strlen(title)
                                       encoding:NSUTF8StringEncoding];

        [fWindow setTitle:titleString];
    }

    void setTransientWinId(const uintptr_t winId) override
    {
        carla_debug("CocoaPluginUI::setTransientWinId(" P_UINTPTR ")", winId);
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:winId];
        CARLA_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        fParentWindow = parentWindow;

        if ([fWindow isVisible])
            [fParentWindow addChildWindow:fWindow
                                   ordered:NSWindowAbove];
    }

    void setChildWindow(void* const childWindow) override
    {
        carla_debug("CocoaPluginUI::setChildWindow(%p)", childWindow);
        CARLA_SAFE_ASSERT_RETURN(childWindow != nullptr,);

        NSView* const view = (NSView*)childWindow;
        const NSRect frame = [view frame];

        [fWindow setContentSize:frame.size];
        [fView setFrame:frame];
        [fView setNeedsDisplay:YES];
    }

    void* getPtr() const noexcept override
    {
        carla_debug("CocoaPluginUI::getPtr()");
        return (void*)fView;
    }

    void* getDisplay() const noexcept
    {
        carla_debug("CocoaPluginUI::getDisplay()");
        return (void*)fWindow;
    }

private:
    NSView*            fView;
    NSWindow*          fParentWindow;
    CarlaPluginWindow* fWindow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CocoaPluginUI)
};

#endif // CARLA_OS_MAC

// ---------------------------------------------------------------------------------------------------------------------
// Windows

#ifdef CARLA_OS_WIN

#define CARLA_LOCAL_CLOSE_MSG (WM_USER + 50)

static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class WindowsPluginUI : public CarlaPluginUI
{
public:
    WindowsPluginUI(Callback* const cb, const uintptr_t parentId, const bool isStandalone, const bool isResizable) noexcept
        : CarlaPluginUI(cb, isStandalone, isResizable),
          fWindow(nullptr),
          fChildWindow(nullptr),
          fParentWindow(nullptr),
          fIsVisible(false),
          fFirstShow(true)
     {
        // FIXME
        static int wc_count = 0;
        char classNameBuf[32];
        std::srand((std::time(nullptr)));
        std::snprintf(classNameBuf, 32, "CarlaWin-%d-%d", ++wc_count, std::rand());
        classNameBuf[31] = '\0';

        const HINSTANCE hInstance = water::getCurrentModuleInstanceHandle();

        carla_zeroStruct(fWindowClass);
        fWindowClass.style         = CS_OWNDC;
        fWindowClass.lpfnWndProc   = wndProc;
        fWindowClass.hInstance     = hInstance;
        fWindowClass.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
        fWindowClass.hCursor       = LoadCursor(hInstance, IDC_ARROW);
        fWindowClass.lpszClassName = strdup(classNameBuf);

        if (!RegisterClassA(&fWindowClass)) {
            free((void*)fWindowClass.lpszClassName);
            return;
        }

        int winFlags = WS_POPUPWINDOW | WS_CAPTION;

        if (isResizable)
            winFlags |= WS_SIZEBOX;

#ifdef BUILDING_CARLA_FOR_WINE
        const uint winType = WS_EX_DLGMODALFRAME;
        const HWND parent  = nullptr;
#else
        const uint winType = WS_EX_TOOLWINDOW;
        const HWND parent  = (HWND)parentId;
#endif

        fWindow = CreateWindowExA(winType,
                                  classNameBuf, "Carla Plugin UI", winFlags,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  parent, nullptr,
                                  hInstance, nullptr);

        if (fWindow == nullptr)
        {
            const DWORD errorCode = ::GetLastError();
            carla_stderr2("CreateWindowEx failed with error code 0x%x, class name was '%s'",
                          errorCode, fWindowClass.lpszClassName);
            UnregisterClassA(fWindowClass.lpszClassName, nullptr);
            free((void*)fWindowClass.lpszClassName);
            return;
        }

        SetWindowLongPtr(fWindow, GWLP_USERDATA, (LONG_PTR)this);

#ifndef BUILDING_CARLA_FOR_WINE
        if (parentId != 0)
            setTransientWinId(parentId);
#endif
        return;

        // maybe unused
        (void)parentId;
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
        UnregisterClassA(fWindowClass.lpszClassName, nullptr);
        free((void*)fWindowClass.lpszClassName);
    }

    void show() override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        if (fFirstShow)
        {
            fFirstShow = false;
            RECT rectChild, rectParent;

            if (fChildWindow != nullptr && GetWindowRect(fChildWindow, &rectChild))
                setSize(rectChild.right - rectChild.left, rectChild.bottom - rectChild.top, false, false);

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
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        ShowWindow(fWindow, SW_HIDE);
        fIsVisible = false;
        UpdateWindow(fWindow);
    }

    void idle() override
    {
        if (fIsIdling || fWindow == nullptr)
            return;

        MSG msg;
        fIsIdling = true;

        while (::PeekMessage(&msg, fWindow, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
            case WM_QUIT:
            case CARLA_LOCAL_CLOSE_MSG:
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
            case WM_SIZE:
                if (fChildWindow != nullptr)
                {
                    RECT rect;
                    GetClientRect(fWindow, &rect);
                    SetWindowPos(fChildWindow, 0, 0, 0, rect.right, rect.bottom,
                                 SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
                }
                break;
            case WM_QUIT:
            case CARLA_LOCAL_CLOSE_MSG:
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
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        SetForegroundWindow(fWindow);
        SetActiveWindow(fWindow);
        SetFocus(fWindow);
    }

    void setMinimumSize(uint, uint) override
    {
        // TODO
    }

    void setSize(const uint width, const uint height, const bool forceUpdate, bool) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

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
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        SetWindowTextA(fWindow, title);
    }

    void setTransientWinId(const uintptr_t winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        fParentWindow = (HWND)winId;
        SetWindowLongPtr(fWindow, GWLP_HWNDPARENT, (LONG_PTR)winId);
    }

    void setChildWindow(void* const winId) override
    {
        CARLA_SAFE_ASSERT_RETURN(winId != nullptr,);

        fChildWindow = (HWND)winId;
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
    HWND fWindow;
    HWND fChildWindow;
    HWND fParentWindow;
    WNDCLASSA fWindowClass;

    bool fIsVisible;
    bool fFirstShow;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowsPluginUI)
};

LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        PostMessage(hwnd, CARLA_LOCAL_CLOSE_MSG, wParam, lParam);
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

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
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
        CARLA_DECLARE_NON_COPYABLE(ScopedDisplay)
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
        CARLA_DECLARE_NON_COPYABLE(ScopedFreeData)
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
        else if (lastGoodWindowNameUTF8 != 0)
        {
            if (lastGoodWindowNameUTF8 == lastGoodWindowNameSimple)
            {
                carla_stdout("Match found using simple and UTF-8 name (ignoring pid)");
                windowToMap = lastGoodWindowNameUTF8;
            }
            else
            {
                carla_stdout("Match found using UTF-8 name (ignoring pid)");
                windowToMap = lastGoodWindowNameUTF8;
            }
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
            carla_stdout("Match found using UTF-8 name");
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
    uint const hints = kCGWindowListOptionOnScreenOnly|kCGWindowListExcludeDesktopElements;

    CFArrayRef const windowListRef = CGWindowListCopyWindowInfo(hints, kCGNullWindowID);
    const NSArray* const windowList = (const NSArray*)windowListRef;

    int windowToMap, windowWithPID = 0, windowWithNameAndPID = 0;

    const NSDictionary* entry;
    for (entry in windowList)
    {
        // FIXME: is this needed? is old version safe?
#if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
        if ([entry[(id)kCGWindowSharingState] intValue] == kCGWindowSharingNone)
            continue;

        NSString* const windowName   =  entry[(id)kCGWindowName];
        int       const windowNumber = [entry[(id)kCGWindowNumber] intValue];
        uintptr_t const windowPID    = [entry[(id)kCGWindowOwnerPID] intValue];
#else
        if ([[entry objectForKey:(id)kCGWindowSharingState] intValue] == kCGWindowSharingNone)
            continue;

        NSString* const windowName   =  [entry objectForKey:(id)kCGWindowName];
        int       const windowNumber = [[entry objectForKey:(id)kCGWindowNumber] intValue];
        uintptr_t const windowPID    = [[entry objectForKey:(id)kCGWindowOwnerPID] intValue];
#endif

        if (windowPID != pid)
            continue;

        windowWithPID = windowNumber;

        if (windowName != nullptr && std::strcmp([windowName UTF8String], uiTitle) == 0)
            windowWithNameAndPID = windowNumber;
    }

    CFRelease(windowListRef);

    if (windowWithNameAndPID != 0)
    {
        carla_stdout("Match found using pid and name");
        windowToMap = windowWithNameAndPID;
    }
    else if (windowWithPID != 0)
    {
        carla_stdout("Match found using pid");
        windowToMap = windowWithPID;
    }
    else
    {
        return false;
    }

    NSWindow* const parentWindow = [NSApp windowWithWindowNumber:winId];
    CARLA_SAFE_ASSERT_RETURN(parentWindow != nullptr, false);

    [parentWindow orderWindow:NSWindowBelow
                   relativeTo:windowToMap];
    return true;
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
#endif // BUILD_BRIDGE_ALTERNATIVE_ARCH

// -----------------------------------------------------

#ifdef HAVE_X11
CarlaPluginUI* CarlaPluginUI::newX11(Callback* const cb,
                                     const uintptr_t parentId,
                                     const bool isStandalone,
                                     const bool isResizable,
                                     const bool isLV2)
{
    return new X11PluginUI(cb, parentId, isStandalone, isResizable, isLV2);
}
#endif

#ifdef CARLA_OS_MAC
CarlaPluginUI* CarlaPluginUI::newCocoa(Callback* const cb,
                                       const uintptr_t parentId,
                                       const bool isStandalone,
                                       const bool isResizable)
{
    return new CocoaPluginUI(cb, parentId, isStandalone, isResizable);
}
#endif

#ifdef CARLA_OS_WIN
CarlaPluginUI* CarlaPluginUI::newWindows(Callback* const cb,
                                         const uintptr_t parentId,
                                         const bool isStandalone,
                                         const bool isResizable)
{
    return new WindowsPluginUI(cb, parentId, isStandalone, isResizable);
}
#endif

// -----------------------------------------------------
