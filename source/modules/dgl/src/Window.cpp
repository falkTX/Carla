/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// we need this for now
//#define PUGL_GRAB_FOCUS 1

#include "../Base.hpp"

#undef PUGL_HAVE_CAIRO
#undef PUGL_HAVE_GL
#define PUGL_HAVE_GL 1

#include "pugl/pugl.h"

#if defined(__GNUC__) && (__GNUC__ >= 7)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#if defined(DISTRHO_OS_WINDOWS)
# include "pugl/pugl_win.cpp"
#elif defined(DISTRHO_OS_MAC)
# include "pugl/pugl_osx.m"
#else
# include <sys/types.h>
# include <unistd.h>
extern "C" {
# include "pugl/pugl_x11.c"
}
#endif

#if defined(__GNUC__) && (__GNUC__ >= 7)
# pragma GCC diagnostic pop
#endif

#include "ApplicationPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "../StandaloneWindow.hpp"
#include "../../distrho/extra/String.hpp"

#define FOR_EACH_WIDGET(it) \
  for (std::list<Widget*>::iterator it = fWidgets.begin(); it != fWidgets.end(); ++it)

#define FOR_EACH_WIDGET_INV(rit) \
  for (std::list<Widget*>::reverse_iterator rit = fWidgets.rbegin(); rit != fWidgets.rend(); ++rit)

#ifdef DEBUG
# define DBG(msg)  std::fprintf(stderr, "%s", msg);
# define DBGp(...) std::fprintf(stderr, __VA_ARGS__);
# define DBGF      std::fflush(stderr);
#else
# define DBG(msg)
# define DBGp(...)
# define DBGF
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Window Private

struct Window::PrivateData {
    PrivateData(Application& app, Window* const self)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
          fWidth(1),
          fHeight(1),
          fTitle(nullptr),
          fWidgets(),
          fModal(),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(true),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        DBG("Creating window without parent..."); DBGF;
        init();
    }

    PrivateData(Application& app, Window* const self, Window& parent)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
          fWidth(1),
          fHeight(1),
          fTitle(nullptr),
          fWidgets(),
          fModal(parent.pData),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(false),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        DBG("Creating window with parent..."); DBGF;
        init();

        const PuglInternals* const parentImpl(parent.pData->fView->impl);

        // NOTE: almost a 1:1 copy of setTransientWinId()
#if defined(DISTRHO_OS_WINDOWS)
        hwndParent = parentImpl->hwnd;
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)hwndParent);
#elif defined(DISTRHO_OS_MAC)
        mParentWindow = parentImpl->window;
#else
        XSetTransientForHint(xDisplay, xWindow, parentImpl->win);
#endif
    }

    PrivateData(Application& app, Window* const self, const intptr_t parentId)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(parentId != 0),
          fResizable(parentId == 0),
          fUsingEmbed(parentId != 0),
          fWidth(1),
          fHeight(1),
          fTitle(nullptr),
          fWidgets(),
          fModal(),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(parentId == 0),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        if (fUsingEmbed)
        {
            DBG("Creating embedded window..."); DBGF;
            puglInitWindowParent(fView, parentId);
        }
        else
        {
            DBG("Creating window without parent..."); DBGF;
        }

        init();

        if (fUsingEmbed)
        {
            DBG("NOTE: Embed window is always visible and non-resizable\n");
            puglShowWindow(fView);
            fApp.pData->oneShown();
            fFirstInit = false;
        }
    }

    void init()
    {
        if (fSelf == nullptr || fView == nullptr)
        {
            DBG("Failed!\n");
            return;
        }

        puglInitContextType(fView, PUGL_GL);
        puglInitUserResizable(fView, fResizable);
        puglInitWindowSize(fView, static_cast<int>(fWidth), static_cast<int>(fHeight));

        puglSetHandle(fView, this);
        puglSetDisplayFunc(fView, onDisplayCallback);
        puglSetKeyboardFunc(fView, onKeyboardCallback);
        puglSetMotionFunc(fView, onMotionCallback);
        puglSetMouseFunc(fView, onMouseCallback);
        puglSetScrollFunc(fView, onScrollCallback);
        puglSetSpecialFunc(fView, onSpecialCallback);
        puglSetReshapeFunc(fView, onReshapeCallback);
        puglSetCloseFunc(fView, onCloseCallback);
#ifndef DGL_FILE_BROWSER_DISABLED
        puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
#endif

        puglCreateWindow(fView, nullptr);

        PuglInternals* impl = fView->impl;
#if defined(DISTRHO_OS_WINDOWS)
        hwnd = impl->hwnd;
        DISTRHO_SAFE_ASSERT(hwnd != 0);
#elif defined(DISTRHO_OS_MAC)
        mView   = impl->glview;
        mWindow = impl->window;
        DISTRHO_SAFE_ASSERT(mView != nullptr);
        if (fUsingEmbed) {
            DISTRHO_SAFE_ASSERT(mWindow == nullptr);
        } else {
            DISTRHO_SAFE_ASSERT(mWindow != nullptr);
        }
#else
        xDisplay = impl->display;
        xWindow  = impl->win;
        DISTRHO_SAFE_ASSERT(xWindow != 0);

        if (! fUsingEmbed)
        {
            pid_t pid = getpid();
            Atom _nwp = XInternAtom(xDisplay, "_NET_WM_PID", True);
            XChangeProperty(xDisplay, xWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);
        }
#endif
        puglEnterContext(fView);

        fApp.pData->windows.push_back(fSelf);

        DBG("Success!\n");
    }

    ~PrivateData()
    {
        DBG("Destroying window..."); DBGF;

        if (fModal.enabled)
        {
            exec_fini();
            close();
        }

        fWidgets.clear();

        if (fUsingEmbed)
        {
            puglHideWindow(fView);
            fApp.pData->oneHidden();
        }

        if (fSelf != nullptr)
        {
            fApp.pData->windows.remove(fSelf);
            fSelf = nullptr;
        }

        if (fView != nullptr)
        {
            puglDestroy(fView);
            fView = nullptr;
        }

        if (fTitle != nullptr)
        {
            std::free(fTitle);
            fTitle = nullptr;
        }

#if defined(DISTRHO_OS_WINDOWS)
        hwnd = 0;
#elif defined(DISTRHO_OS_MAC)
        mView   = nullptr;
        mWindow = nullptr;
#else
        xDisplay = nullptr;
        xWindow  = 0;
#endif

        DBG("Success!\n");
    }

    // -------------------------------------------------------------------

    void close()
    {
        DBG("Window close\n");

        if (fUsingEmbed)
            return;

        setVisible(false);

        if (! fFirstInit)
        {
            fApp.pData->oneHidden();
            fFirstInit = true;
        }
    }

    void exec(const bool lockWait)
    {
        DBG("Window exec\n");
        exec_init();

        if (lockWait)
        {
            for (; fVisible && fModal.enabled;)
            {
                idle();
                d_msleep(10);
            }

            exec_fini();
        }
        else
        {
            idle();
        }
    }

    // -------------------------------------------------------------------

    void exec_init()
    {
        DBG("Window modal loop starting..."); DBGF;
        DISTRHO_SAFE_ASSERT_RETURN(fModal.parent != nullptr, setVisible(true));

        fModal.enabled = true;
        fModal.parent->fModal.childFocus = this;

        fModal.parent->setVisible(true);
        setVisible(true);

        DBG("Ok\n");
    }

    void exec_fini()
    {
        DBG("Window modal loop stopping..."); DBGF;
        fModal.enabled = false;

        if (fModal.parent != nullptr)
        {
            fModal.parent->fModal.childFocus = nullptr;

            // the mouse position probably changed since the modal appeared,
            // so send a mouse motion event to the modal's parent window
#if defined(DISTRHO_OS_WINDOWS)
            // TODO
#elif defined(DISTRHO_OS_MAC)
            // TODO
#else
            int i, wx, wy;
            uint u;
            ::Window w;
            if (XQueryPointer(fModal.parent->xDisplay, fModal.parent->xWindow, &w, &w, &i, &i, &wx, &wy, &u) == True)
                fModal.parent->onPuglMotion(wx, wy);
#endif
        }

        DBG("Ok\n");
    }

    // -------------------------------------------------------------------

    void focus()
    {
        DBG("Window focus\n");
#if defined(DISTRHO_OS_WINDOWS)
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
#elif defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
            [mWindow makeKeyWindow];
#else
        XRaiseWindow(xDisplay, xWindow);
        XSetInputFocus(xDisplay, xWindow, RevertToPointerRoot, CurrentTime);
        XFlush(xDisplay);
#endif
    }

    // -------------------------------------------------------------------

    void setVisible(const bool yesNo)
    {
        if (fVisible == yesNo)
        {
            DBG("Window setVisible matches current state, ignoring request\n");
            return;
        }
        if (fUsingEmbed)
        {
            DBG("Window setVisible cannot be called when embedded\n");
            return;
        }

        DBG("Window setVisible called\n");

        fVisible = yesNo;

        if (yesNo && fFirstInit)
            setSize(fWidth, fHeight, true);

#if defined(DISTRHO_OS_WINDOWS)
        if (yesNo)
        {
            if (fFirstInit)
            {
                RECT rectChild, rectParent;

                if (hwndParent != nullptr &&
                    GetWindowRect(hwnd, &rectChild) &&
                    GetWindowRect(hwndParent, &rectParent))
                {
                    SetWindowPos(hwnd, hwndParent,
                                 rectParent.left + (rectChild.right-rectChild.left)/2,
                                 rectParent.top + (rectChild.bottom-rectChild.top)/2,
                                 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
                }
                else
                {
                    ShowWindow(hwnd, SW_SHOWNORMAL);
                }
            }
            else
            {
                ShowWindow(hwnd, SW_RESTORE);
            }
        }
        else
        {
            ShowWindow(hwnd, SW_HIDE);
        }

        UpdateWindow(hwnd);
#elif defined(DISTRHO_OS_MAC)
        if (yesNo)
        {
            if (mWindow != nullptr)
            {
                if (mParentWindow != nullptr)
                    [mParentWindow addChildWindow:mWindow
                                          ordered:NSWindowAbove];

                [mWindow setIsVisible:YES];
            }
            else
            {
                [mView setHidden:NO];
            }
        }
        else
        {
            if (mWindow != nullptr)
            {
                if (mParentWindow != nullptr)
                    [mParentWindow removeChildWindow:mWindow];

                [mWindow setIsVisible:NO];
            }
            else
            {
                [mView setHidden:YES];
            }
        }
#else
        if (yesNo)
            XMapRaised(xDisplay, xWindow);
        else
            XUnmapWindow(xDisplay, xWindow);

        XFlush(xDisplay);
#endif

        if (yesNo)
        {
            if (fFirstInit)
            {
                fApp.pData->oneShown();
                fFirstInit = false;
            }
        }
        else if (fModal.enabled)
            exec_fini();
    }

    // -------------------------------------------------------------------

    void setResizable(const bool yesNo)
    {
        if (fResizable == yesNo)
        {
            DBG("Window setResizable matches current state, ignoring request\n");
            return;
        }
        if (fUsingEmbed)
        {
            DBG("Window setResizable cannot be called when embedded\n");
            return;
        }

        DBG("Window setResizable called\n");

        fResizable = yesNo;

#if defined(DISTRHO_OS_WINDOWS)
        const int winFlags = fResizable ? GetWindowLong(hwnd, GWL_STYLE) |  WS_SIZEBOX
                                        : GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX;
        SetWindowLong(hwnd, GWL_STYLE, winFlags);
#elif defined(DISTRHO_OS_MAC)
        const uint flags(yesNo ? (NSViewWidthSizable|NSViewHeightSizable) : 0x0);
        [mView setAutoresizingMask:flags];
#endif

        setSize(fWidth, fHeight, true);
    }

    // -------------------------------------------------------------------

    void setSize(uint width, uint height, const bool forced = false)
    {
        if (width <= 1 || height <= 1)
        {
            DBGp("Window setSize called with invalid value(s) %i %i, ignoring request\n", width, height);
            return;
        }

        if (fWidth == width && fHeight == height && ! forced)
        {
            DBGp("Window setSize matches current size, ignoring request (%i %i)\n", width, height);
            return;
        }

        fWidth  = width;
        fHeight = height;

        DBGp("Window setSize called %s, size %i %i, resizable %s\n", forced ? "(forced)" : "(not forced)", width, height, fResizable?"true":"false");

#if defined(DISTRHO_OS_WINDOWS)
        const int winFlags = WS_POPUPWINDOW | WS_CAPTION | (fResizable ? WS_SIZEBOX : 0x0);
        RECT wr = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
        AdjustWindowRectEx(&wr, fUsingEmbed ? WS_CHILD : winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(hwnd, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top,
                     SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);

        if (! forced)
            UpdateWindow(hwnd);
#elif defined(DISTRHO_OS_MAC)
        [mView setFrame:NSMakeRect(0, 0, width, height)];

        if (mWindow != nullptr)
        {
            const NSSize size = NSMakeSize(width, height);
            [mWindow setContentSize:size];

            if (fResizable)
            {
                [mWindow setContentMinSize:NSMakeSize(1, 1)];
                [mWindow setContentMaxSize:NSMakeSize(99999, 99999)];
                [[mWindow standardWindowButton:NSWindowZoomButton] setHidden:NO];
            }
            else
            {
                [mWindow setContentMinSize:size];
                [mWindow setContentMaxSize:size];
                [[mWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
            }
        }
#else
        XResizeWindow(xDisplay, xWindow, width, height);

        if (! fResizable)
        {
            XSizeHints sizeHints;
            memset(&sizeHints, 0, sizeof(sizeHints));

            sizeHints.flags      = PSize|PMinSize|PMaxSize;
            sizeHints.width      = static_cast<int>(width);
            sizeHints.height     = static_cast<int>(height);
            sizeHints.min_width  = static_cast<int>(width);
            sizeHints.min_height = static_cast<int>(height);
            sizeHints.max_width  = static_cast<int>(width);
            sizeHints.max_height = static_cast<int>(height);

            XSetNormalHints(xDisplay, xWindow, &sizeHints);
        }

        if (! forced)
            XFlush(xDisplay);
#endif

        puglPostRedisplay(fView);
    }

    // -------------------------------------------------------------------

    const char* getTitle() const noexcept
    {
        static const char* const kFallback = "";

        return fTitle != nullptr ? fTitle : kFallback;
    }

    void setTitle(const char* const title)
    {
        DBGp("Window setTitle \"%s\"\n", title);

        if (fTitle != nullptr)
            std::free(fTitle);

        fTitle = strdup(title);

#if defined(DISTRHO_OS_WINDOWS)
        SetWindowTextA(hwnd, title);
#elif defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
        {
            NSString* titleString = [[NSString alloc]
                                      initWithBytes:title
                                             length:strlen(title)
                                          encoding:NSUTF8StringEncoding];

            [mWindow setTitle:titleString];
        }
#else
        XStoreName(xDisplay, xWindow, title);
#endif
    }

    void setTransientWinId(const uintptr_t winId)
    {
        DISTRHO_SAFE_ASSERT_RETURN(winId != 0,);

#if defined(DISTRHO_OS_WINDOWS)
        hwndParent = (HWND)winId;
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)winId);
#elif defined(DISTRHO_OS_MAC)
        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:winId];
        DISTRHO_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        [parentWindow addChildWindow:mWindow
                             ordered:NSWindowAbove];
#else
        XSetTransientForHint(xDisplay, xWindow, static_cast< ::Window>(winId));
#endif
    }

    // -------------------------------------------------------------------

    void addWidget(Widget* const widget)
    {
        fWidgets.push_back(widget);
    }

    void removeWidget(Widget* const widget)
    {
        fWidgets.remove(widget);
    }

    void idle()
    {
        puglProcessEvents(fView);

#ifdef DISTRHO_OS_MAC
        if (fNeedsIdle)
        {
            NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
            NSEvent* event;

            for (;;)
            {
                event = [NSApp
                         nextEventMatchingMask:NSAnyEventMask
                                     untilDate:[NSDate distantPast]
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];

                if (event == nil)
                    break;

                [NSApp sendEvent: event];
            }

            [pool release];
        }
#endif

        if (fModal.enabled && fModal.parent != nullptr)
            fModal.parent->idle();
    }

    // -------------------------------------------------------------------

    void onPuglDisplay()
    {
        fSelf->onDisplayBefore();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->pData->display(fWidth, fHeight);
        }

        fSelf->onDisplayAfter();
    }

    int onPuglKeyboard(const bool press, const uint key)
    {
        DBGp("PUGL: onKeyboard : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return 0;
        }

        Widget::KeyboardEvent ev;
        ev.press = press;
        ev.key  = key;
        ev.mod  = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onKeyboard(ev))
                return 0;
        }

        return 1;
    }

    int onPuglSpecial(const bool press, const Key key)
    {
        DBGp("PUGL: onSpecial : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return 0;
        }

        Widget::SpecialEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time  = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onSpecial(ev))
                return 0;
        }

        return 1;
    }

    void onPuglMouse(const int button, const bool press, const int x, const int y)
    {
        DBGp("PUGL: onMouse : %i %i %i %i\n", button, press, x, y);

        // FIXME - pugl sends 2 of these for each window on init, don't ask me why. we'll ignore it
        if (press && button == 0 && x == 0 && y == 0) return;

        if (fModal.childFocus != nullptr)
            return fModal.childFocus->focus();

        Widget::MouseEvent ev;
        ev.button = button;
        ev.press  = press;
        ev.mod    = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time   = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onMouse(ev))
                break;
        }
    }

    void onPuglMotion(const int x, const int y)
    {
        DBGp("PUGL: onMotion : %i %i\n", x, y);

        if (fModal.childFocus != nullptr)
            return;

        Widget::MotionEvent ev;
        ev.mod  = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onMotion(ev))
                break;
        }
    }

    void onPuglScroll(const int x, const int y, const float dx, const float dy)
    {
        DBGp("PUGL: onScroll : %i %i %f %f\n", x, y, dx, dy);

        if (fModal.childFocus != nullptr)
            return;

        Widget::ScrollEvent ev;
        ev.delta = Point<float>(dx, dy);
        ev.mod   = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time  = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onScroll(ev))
                break;
        }
    }

    void onPuglReshape(const int width, const int height)
    {
        DBGp("PUGL: onReshape : %i %i\n", width, height);

        if (width <= 1 && height <= 1)
            return;

        fWidth  = static_cast<uint>(width);
        fHeight = static_cast<uint>(height);

        fSelf->onReshape(fWidth, fHeight);

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);

            if (widget->pData->needsFullViewport)
                widget->setSize(fWidth, fHeight);
        }
    }

    void onPuglClose()
    {
        DBG("PUGL: onClose\n");

        if (fModal.enabled)
            exec_fini();

        fSelf->onClose();

        if (fModal.childFocus != nullptr)
            fModal.childFocus->fSelf->onClose();

        close();
    }

    // -------------------------------------------------------------------

    bool handlePluginKeyboard(const bool press, const uint key)
    {
        DBGp("PUGL: handlePluginKeyboard : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return true;
        }

        Widget::KeyboardEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(fView->mods);
        ev.time  = 0;

        if ((ev.mod & kModifierShift) != 0 && ev.key >= 'a' && ev.key <= 'z')
            ev.key -= 'a' - 'A'; // a-z -> A-Z

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onKeyboard(ev))
                return true;
        }

        return false;
    }

    bool handlePluginSpecial(const bool press, const Key key)
    {
        DBGp("PUGL: handlePluginSpecial : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return true;
        }

        int mods = 0x0;

        switch (key)
        {
        case kKeyShift:
            mods |= kModifierShift;
            break;
        case kKeyControl:
            mods |= kModifierControl;
            break;
        case kKeyAlt:
            mods |= kModifierAlt;
            break;
        default:
            break;
        }

        if (mods != 0x0)
        {
            if (press)
                fView->mods |= mods;
            else
                fView->mods &= ~(mods);
        }

        Widget::SpecialEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(fView->mods);
        ev.time  = 0;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onSpecial(ev))
                return true;
        }

        return false;
    }

    // -------------------------------------------------------------------

    Application& fApp;
    Window*      fSelf;
    PuglView*    fView;

    bool fFirstInit;
    bool fVisible;
    bool fResizable;
    bool fUsingEmbed;
    uint fWidth;
    uint fHeight;
    char* fTitle;
    std::list<Widget*> fWidgets;

    struct Modal {
        bool enabled;
        PrivateData* parent;
        PrivateData* childFocus;

        Modal()
            : enabled(false),
              parent(nullptr),
              childFocus(nullptr) {}

        Modal(PrivateData* const p)
            : enabled(false),
              parent(p),
              childFocus(nullptr) {}

        ~Modal()
        {
            DISTRHO_SAFE_ASSERT(! enabled);
            DISTRHO_SAFE_ASSERT(childFocus == nullptr);
        }

        DISTRHO_DECLARE_NON_COPY_STRUCT(Modal)
    } fModal;

#if defined(DISTRHO_OS_WINDOWS)
    HWND hwnd;
    HWND hwndParent;
#elif defined(DISTRHO_OS_MAC)
    bool            fNeedsIdle;
    PuglOpenGLView* mView;
    id              mWindow;
    id              mParentWindow;
#else
    Display* xDisplay;
    ::Window xWindow;
#endif

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((PrivateData*)puglGetHandle(view))

    static void onDisplayCallback(PuglView* view)
    {
        handlePtr->onPuglDisplay();
    }

    static int onKeyboardCallback(PuglView* view, bool press, uint32_t key)
    {
        return handlePtr->onPuglKeyboard(press, key);
    }

    static int onSpecialCallback(PuglView* view, bool press, PuglKey key)
    {
        return handlePtr->onPuglSpecial(press, static_cast<Key>(key));
    }

    static void onMouseCallback(PuglView* view, int button, bool press, int x, int y)
    {
        handlePtr->onPuglMouse(button, press, x, y);
    }

    static void onMotionCallback(PuglView* view, int x, int y)
    {
        handlePtr->onPuglMotion(x, y);
    }

    static void onScrollCallback(PuglView* view, int x, int y, float dx, float dy)
    {
        handlePtr->onPuglScroll(x, y, dx, dy);
    }

    static void onReshapeCallback(PuglView* view, int width, int height)
    {
        handlePtr->onPuglReshape(width, height);
    }

    static void onCloseCallback(PuglView* view)
    {
        handlePtr->onPuglClose();
    }

#ifndef DGL_FILE_BROWSER_DISABLED
    static void fileBrowserSelectedCallback(PuglView* view, const char* filename)
    {
        handlePtr->fSelf->fileBrowserSelected(filename);
    }
#endif

    #undef handlePtr

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app, this)) {}

Window::Window(Application& app, Window& parent)
    : pData(new PrivateData(app, this, parent)) {}

Window::Window(Application& app, intptr_t parentId)
    : pData(new PrivateData(app, this, parentId)) {}

Window::~Window()
{
    delete pData;
}

void Window::show()
{
    pData->setVisible(true);
}

void Window::hide()
{
    pData->setVisible(false);
}

void Window::close()
{
    pData->close();
}

void Window::exec(bool lockWait)
{
    pData->exec(lockWait);
}

void Window::focus()
{
    pData->focus();
}

void Window::repaint() noexcept
{
    puglPostRedisplay(pData->fView);
}

// static int fib_filter_filename_filter(const char* const name)
// {
//     return 1;
//     (void)name;
// }

#ifndef DGL_FILE_BROWSER_DISABLED
bool Window::openFileBrowser(const FileBrowserOptions& options)
{
# ifdef SOFD_HAVE_X11
    using DISTRHO_NAMESPACE::String;

    // --------------------------------------------------------------------------
    // configure start dir

    // TODO: get abspath if needed
    // TODO: cross-platform

    String startDir(options.startDir);

    if (startDir.isEmpty())
    {
        if (char* const dir_name = get_current_dir_name())
        {
            startDir = dir_name;
            std::free(dir_name);
        }
    }

    DISTRHO_SAFE_ASSERT_RETURN(startDir.isNotEmpty(), false);

    if (! startDir.endsWith('/'))
        startDir += "/";

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(0, startDir) == 0, false);

    // --------------------------------------------------------------------------
    // configure title

    String title(options.title);

    if (title.isEmpty())
    {
        title = pData->getTitle();

        if (title.isEmpty())
            title = "FileBrowser";
    }

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(1, title) == 0, false);

    // --------------------------------------------------------------------------
    // configure filters

    x_fib_cfg_filter_callback(nullptr); //fib_filter_filename_filter);

    // --------------------------------------------------------------------------
    // configure buttons

    x_fib_cfg_buttons(3, options.buttons.listAllFiles-1);
    x_fib_cfg_buttons(1, options.buttons.showHidden-1);
    x_fib_cfg_buttons(2, options.buttons.showPlaces-1);

    // --------------------------------------------------------------------------
    // show

    return (x_fib_show(pData->xDisplay, pData->xWindow, /*options.width*/0, /*options.height*/0) == 0);
# else
    // not implemented
    return false;
# endif
}
#endif

bool Window::isVisible() const noexcept
{
    return pData->fVisible;
}

void Window::setVisible(bool yesNo)
{
    pData->setVisible(yesNo);
}

bool Window::isResizable() const noexcept
{
    return pData->fResizable;
}

void Window::setResizable(bool yesNo)
{
    pData->setResizable(yesNo);
}

uint Window::getWidth() const noexcept
{
    return pData->fWidth;
}

uint Window::getHeight() const noexcept
{
    return pData->fHeight;
}

Size<uint> Window::getSize() const noexcept
{
    return Size<uint>(pData->fWidth, pData->fHeight);
}

void Window::setSize(uint width, uint height)
{
    pData->setSize(width, height);
}

void Window::setSize(Size<uint> size)
{
    pData->setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return pData->getTitle();
}

void Window::setTitle(const char* title)
{
    pData->setTitle(title);
}

void Window::setTransientWinId(uintptr_t winId)
{
    pData->setTransientWinId(winId);
}

Application& Window::getApp() const noexcept
{
    return pData->fApp;
}

intptr_t Window::getWindowId() const noexcept
{
    return puglGetNativeWindow(pData->fView);
}

void Window::_addWidget(Widget* const widget)
{
    pData->addWidget(widget);
}

void Window::_removeWidget(Widget* const widget)
{
    pData->removeWidget(widget);
}

void Window::_idle()
{
    pData->idle();
}

// -----------------------------------------------------------------------

void Window::addIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.push_back(callback);
}

void Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.remove(callback);
}

// -----------------------------------------------------------------------

void Window::onDisplayBefore()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
}

void Window::onDisplayAfter()
{
}

void Window::onReshape(uint width, uint height)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Window::onClose()
{
}

#ifndef DGL_FILE_BROWSER_DISABLED
void Window::fileBrowserSelected(const char*)
{
}
#endif

bool Window::handlePluginKeyboard(const bool press, const uint key)
{
    return pData->handlePluginKeyboard(press, key);
}

bool Window::handlePluginSpecial(const bool press, const Key key)
{
    return pData->handlePluginSpecial(press, key);
}

// -----------------------------------------------------------------------

StandaloneWindow::StandaloneWindow()
    : Application(),
      Window((Application&)*this),
      fWidget(nullptr) {}

void StandaloneWindow::exec()
{
    Window::show();
    Application::exec();
}

void StandaloneWindow::onReshape(uint width, uint height)
{
    if (fWidget != nullptr)
        fWidget->setSize(width, height);
    Window::onReshape(width, height);
}

void StandaloneWindow::_addWidget(Widget* widget)
{
    if (fWidget == nullptr)
    {
        fWidget = widget;
        fWidget->pData->needsFullViewport = true;
    }
    Window::_addWidget(widget);
}

void StandaloneWindow::_removeWidget(Widget* widget)
{
    if (fWidget == widget)
    {
        fWidget->pData->needsFullViewport = false;
        fWidget = nullptr;
    }
    Window::_removeWidget(widget);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#undef DBG
#undef DBGF
