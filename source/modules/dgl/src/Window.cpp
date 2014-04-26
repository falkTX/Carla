/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "../App.hpp"
#include "../Widget.hpp"
#include "../Window.hpp"

#include <cassert>
#include <cstdio>
#include <list>

#include "pugl/pugl.h"

#if defined(DISTRHO_OS_WINDOWS)
# include "pugl/pugl_win.cpp"
#elif defined(DISTRHO_OS_MAC)
# include "pugl/pugl_osx_extended.h"
extern "C" {
struct PuglViewImpl {
    int width;
    int height;
};}
#elif defined(DISTRHO_OS_LINUX)
# include <sys/types.h>
# include <unistd.h>
extern "C" {
# include "pugl/pugl_x11.c"
}
#else
# error Unsupported platform
#endif

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

Window* dgl_lastUiParent = nullptr;

// -----------------------------------------------------------------------
// Window Private

class Window::PrivateData
{
public:
    PrivateData(App& app, Window* const self)
        : fApp(app),
          fSelf(self),
          fView(puglCreate(0, "Window", 100, 100, true, false)),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(0)
#elif defined(DISTRHO_OS_LINUX)
          xDisplay(nullptr),
          xWindow(0)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(true)
#else
          _dummy('\0')
#endif
    {
        DBG("Creating window without parent..."); DBGF;
        init();
    }

    PrivateData(App& app, Window* const self, Window& parent)
        : fApp(app),
          fSelf(self),
          fView(puglCreate(0, "Window", 100, 100, true, false)),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
          fModal(parent.pData),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(0)
#elif defined(DISTRHO_OS_LINUX)
          xDisplay(nullptr),
          xWindow(0)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(false)
#else
          _dummy('\0')
#endif
    {
        DBG("Creating window with parent..."); DBGF;
        init();

#ifdef DISTRHO_OS_LINUX
        const PuglInternals* const parentImpl(parent.pData->fView->impl);

        XSetTransientForHint(xDisplay, xWindow, parentImpl->win);
#endif
    }

    PrivateData(App& app, Window* const self, const intptr_t parentId)
        : fApp(app),
          fSelf(self),
          fView(puglCreate(parentId, "Window", 100, 100, (parentId == 0), (parentId != 0))),
          fFirstInit(true),
          fVisible(parentId != 0),
          fResizable(parentId == 0),
          fUsingEmbed(parentId != 0),
#if defined(DISTRHO_OS_WINDOWS)
          hwnd(0)
#elif defined(DISTRHO_OS_LINUX)
          xDisplay(nullptr),
          xWindow(0)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(false)
#else
          _dummy('\0')
#endif
    {
        if (parentId != 0) {
            DBG("Creating embedded window..."); DBGF;
        } else {
            DBG("Creating window without parent..."); DBGF;
        }

        init();

        if (parentId != 0)
        {
            DBG("NOTE: Embed window is always visible and non-resizable\n");
            fApp._oneShown();
            fFirstInit = false;
        }
    }

    void init()
    {
        if (fSelf == nullptr || fView == nullptr)
        {
            DBG("Failed!\n");
            dgl_lastUiParent = nullptr;
            return;
        }

        dgl_lastUiParent = fSelf;

        puglSetHandle(fView, this);
        puglSetDisplayFunc(fView, onDisplayCallback);
        puglSetKeyboardFunc(fView, onKeyboardCallback);
        puglSetMotionFunc(fView, onMotionCallback);
        puglSetMouseFunc(fView, onMouseCallback);
        puglSetScrollFunc(fView, onScrollCallback);
        puglSetSpecialFunc(fView, onSpecialCallback);
        puglSetReshapeFunc(fView, onReshapeCallback);
        puglSetCloseFunc(fView, onCloseCallback);

#if defined(DISTRHO_OS_WINDOWS)
        PuglInternals* impl = fView->impl;
        hwnd = impl->hwnd;
        assert(hwnd != 0);
#elif defined(DISTRHO_OS_LINUX)
        PuglInternals* impl = fView->impl;
        xDisplay = impl->display;
        xWindow  = impl->win;
        assert(xWindow != 0);

        if (! fUsingEmbed)
        {
            pid_t pid = getpid();
            Atom _nwp = XInternAtom(xDisplay, "_NET_WM_PID", True);
            XChangeProperty(xDisplay, xWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&pid, 1);
        }
#endif

        DBG("Success!\n");

#if 0
        // process any initial events
        puglProcessEvents(fView);
#endif

        fApp._addWindow(fSelf);
    }

    ~PrivateData()
    {
        DBG("Destroying window..."); DBGF;

        //fOnModal = false;
        fWidgets.clear();

        if (fSelf != nullptr)
        {
            fApp._removeWindow(fSelf);
            fSelf = nullptr;
        }

        if (fView != nullptr)
        {
            puglDestroy(fView);
            fView = nullptr;
        }

#if defined(DISTRHO_OS_WINDOWS)
          hwnd = 0;
#elif defined(DISTRHO_OS_LINUX)
          xDisplay = nullptr;
          xWindow  = 0;
#endif

        DBG("Success!\n");
    }

    // -------------------------------------------------------------------

    void close()
    {
        DBG("Window close\n");
        setVisible(false);

        if (! fFirstInit)
        {
            fApp._oneHidden();
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

    void focus()
    {
        DBG("Window focus\n");
#if defined(DISTRHO_OS_WINDOWS)
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
#elif defined(DISTRHO_OS_MAC)
        puglImplFocus(fView);
#elif defined(DISTRHO_OS_LINUX)
        XRaiseWindow(xDisplay, xWindow);
        XSetInputFocus(xDisplay, xWindow, RevertToPointerRoot, CurrentTime);
        XFlush(xDisplay);
#endif
    }

    void repaint()
    {
        //DBG("Window repaint\n");
        puglPostRedisplay(fView);
    }

    // -------------------------------------------------------------------

    bool isVisible() const noexcept
    {
        return fVisible;
    }

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
            setSize(static_cast<unsigned int>(fView->width), static_cast<unsigned int>(fView->height), true);

#if defined(DISTRHO_OS_WINDOWS)
        if (yesNo)
            ShowWindow(hwnd, fFirstInit ? SW_SHOWNORMAL : SW_RESTORE);
        else
            ShowWindow(hwnd, SW_HIDE);

        UpdateWindow(hwnd);
#elif defined(DISTRHO_OS_MAC)
        puglImplSetVisible(fView, yesNo);
#elif defined(DISTRHO_OS_LINUX)
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
                fApp._oneShown();
                fFirstInit = false;
            }
        }
        else if (fModal.enabled)
            exec_fini();
    }

    // -------------------------------------------------------------------

    bool isResizable() const noexcept
    {
        return fResizable;
    }

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

        setSize(static_cast<unsigned int>(fView->width), static_cast<unsigned int>(fView->height), true);
    }

    // -------------------------------------------------------------------

    int getWidth() const noexcept
    {
        return fView->width;
    }

    int getHeight() const noexcept
    {
        return fView->height;
    }

    Size<int> getSize() const noexcept
    {
        return Size<int>(fView->width, fView->height);
    }

    void setSize(unsigned int width, unsigned int height, const bool forced = false)
    {
        if (width == 0 || height == 0)
        {
            DBGp("Window setSize called with invalid value(s) %i %i, ignoring request\n", width, height);
            return;
        }

        if (fView->width == static_cast<int>(width) && fView->height == static_cast<int>(height) && ! forced)
        {
            DBGp("Window setSize matches current size, ignoring request (%i %i)\n", width, height);
            return;
        }

        fView->width  = static_cast<int>(width);
        fView->height = static_cast<int>(height);

        DBGp("Window setSize called %s, size %i %i\n", forced ? "(forced)" : "(not forced)", width, height);

#if defined(DISTRHO_OS_WINDOWS)
        int winFlags = WS_POPUPWINDOW | WS_CAPTION;

        if (fResizable)
            winFlags |= WS_SIZEBOX;

        RECT wr = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
        AdjustWindowRectEx(&wr, winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(hwnd, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);

        if (! forced)
            UpdateWindow(hwnd);
#elif defined(DISTRHO_OS_MAC)
        puglImplSetSize(fView, width, height, forced);
#elif defined(DISTRHO_OS_LINUX)
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

        repaint();
    }

    // -------------------------------------------------------------------

    void setTitle(const char* const title)
    {
        DBGp("Window setTitle \"%s\"\n", title);

#if defined(DISTRHO_OS_WINDOWS)
        SetWindowTextA(hwnd, title);
#elif defined(DISTRHO_OS_MAC)
        puglImplSetTitle(fView, title);
#elif defined(DISTRHO_OS_LINUX)
        XStoreName(xDisplay, xWindow, title);
#endif
    }

    void setTransientWinId(const intptr_t winId)
    {
#if defined(DISTRHO_OS_LINUX)
        XSetTransientForHint(xDisplay, xWindow, static_cast< ::Window>(winId));
#else
        return;
        // unused
        (void)winId;
#endif
    }

    // -------------------------------------------------------------------

    App& getApp() const noexcept
    {
        return fApp;
    }

    int getModifiers() const
    {
        return puglGetModifiers(fView);
    }

    uint32_t getEventTimestamp() const
    {
        return puglGetEventTimestamp(fView);
    }

    intptr_t getWindowId() const
    {
        return puglGetNativeWindow(fView);
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
            puglImplIdle(fView);
#endif

        if (fModal.enabled && fModal.parent != nullptr)
            fModal.parent->idle();
    }

    // -------------------------------------------------------------------

    void exec_init()
    {
        DBG("Window modal loop starting..."); DBGF;
        assert(fModal.parent != nullptr);

        if (fModal.parent == nullptr)
        {
            DBG("Failed, there's no modal parent!\n");
            return setVisible(true);
        }

        fModal.enabled = true;
        fModal.parent->fModal.childFocus = this;

#ifdef DISTRHO_OS_WINDOWS
        // Center this window
        PuglInternals* const parentImpl = fModal.parent->fView->impl;

        RECT curRect;
        RECT parentRect;
        GetWindowRect(hwnd, &curRect);
        GetWindowRect(parentImpl->hwnd, &parentRect);

        int x = parentRect.left+(parentRect.right-curRect.right)/2;
        int y = parentRect.top +(parentRect.bottom-curRect.bottom)/2;

        SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        UpdateWindow(hwnd);
#endif

        fModal.parent->setVisible(true);
        setVisible(true);

        DBG("Ok\n");
    }

    void exec_fini()
    {
        DBG("Window modal loop stopping..."); DBGF;
        fModal.enabled = false;

        if (fModal.parent != nullptr)
            fModal.parent->fModal.childFocus = nullptr;

        DBG("Ok\n");
    }

    // -------------------------------------------------------------------

protected:
    void onDisplay()
    {
        //DBG("PUGL: onDisplay\n");

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);

            if (widget->isVisible())
                widget->onDisplay();
        }
    }

    void onKeyboard(const bool press, const uint32_t key)
    {
        DBGp("PUGL: onKeyboard : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
            return fModal.childFocus->focus();

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onKeyboard(press, key))
                break;
        }
    }

    void onMouse(const int button, const bool press, const int x, const int y)
    {
        DBGp("PUGL: onMouse : %i %i %i %i\n", button, press, x, y);

        if (fModal.childFocus != nullptr)
            return fModal.childFocus->focus();

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onMouse(button, press, x, y))
                break;
        }
    }

    void onMotion(const int x, const int y)
    {
        DBGp("PUGL: onMotion : %i %i\n", x, y);

        if (fModal.childFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onMotion(x, y))
                break;
        }
    }

    void onScroll(const int x, const int y, const float dx, const float dy)
    {
        DBGp("PUGL: onScroll : %i %i %f %f\n", x, y, dx, dy);

        if (fModal.childFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onScroll(x, y, dx, dy))
                break;
        }
    }

    void onSpecial(const bool press, const Key key)
    {
        DBGp("PUGL: onSpecial : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onSpecial(press, key))
                break;
        }
    }

    void onReshape(const int width, const int height)
    {
        DBGp("PUGL: onReshape : %i %i\n", width, height);

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->onReshape(width, height);
        }
    }

    void onClose()
    {
        DBG("PUGL: onClose\n");

        if (fModal.enabled && fModal.parent != nullptr)
            exec_fini();

        if (fModal.childFocus != nullptr)
            fModal.childFocus->onClose();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->onClose();
        }

        close();
    }

    // -------------------------------------------------------------------

private:
    App&      fApp;
    Window*   fSelf;
    PuglView* fView;

    bool fFirstInit;
    bool fVisible;
    bool fResizable;
    bool fUsingEmbed;
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
            assert(! enabled);
            assert(childFocus == nullptr);
        }
    } fModal;

#if defined(DISTRHO_OS_WINDOWS)
    HWND     hwnd;
#elif defined(DISTRHO_OS_LINUX)
    Display* xDisplay;
    ::Window xWindow;
#elif defined(DISTRHO_OS_MAC)
    bool     fNeedsIdle;
#else
    char      _dummy;
#endif

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((PrivateData*)puglGetHandle(view))

    static void onDisplayCallback(PuglView* view)
    {
        handlePtr->onDisplay();
    }

    static void onKeyboardCallback(PuglView* view, bool press, uint32_t key)
    {
        handlePtr->onKeyboard(press, key);
    }

    static void onMouseCallback(PuglView* view, int button, bool press, int x, int y)
    {
        handlePtr->onMouse(button, press, x, y);
    }

    static void onMotionCallback(PuglView* view, int x, int y)
    {
        handlePtr->onMotion(x, y);
    }

    static void onScrollCallback(PuglView* view, int x, int y, float dx, float dy)
    {
        handlePtr->onScroll(x, y, dx, dy);
    }

    static void onSpecialCallback(PuglView* view, bool press, PuglKey key)
    {
        handlePtr->onSpecial(press, static_cast<Key>(key));
    }

    static void onReshapeCallback(PuglView* view, int width, int height)
    {
        handlePtr->onReshape(width, height);
    }

    static void onCloseCallback(PuglView* view)
    {
        handlePtr->onClose();
    }

    #undef handlePtr
};

// -----------------------------------------------------------------------
// Window

Window::Window(App& app)
    : pData(new PrivateData(app, this))
{
}

Window::Window(App& app, Window& parent)
    : pData(new PrivateData(app, this, parent))
{
}

Window::Window(App& app, intptr_t parentId)
    : pData(new PrivateData(app, this, parentId))
{
}

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

void Window::repaint()
{
    pData->repaint();
}

bool Window::isVisible() const noexcept
{
    return pData->isVisible();
}

void Window::setVisible(bool yesNo)
{
    pData->setVisible(yesNo);
}

bool Window::isResizable() const noexcept
{
    return pData->isResizable();
}

void Window::setResizable(bool yesNo)
{
    pData->setResizable(yesNo);
}

int Window::getWidth() const noexcept
{
    return pData->getWidth();
}

int Window::getHeight() const noexcept
{
    return pData->getHeight();
}

Size<int> Window::getSize() const noexcept
{
    return pData->getSize();
}

void Window::setSize(unsigned int width, unsigned int height)
{
    pData->setSize(width, height);
}

void Window::setTitle(const char* title)
{
    pData->setTitle(title);
}

void Window::setTransientWinId(intptr_t winId)
{
    pData->setTransientWinId(winId);
}

App& Window::getApp() const noexcept
{
    return pData->getApp();
}

int Window::getModifiers() const
{
    return pData->getModifiers();
}

uint32_t Window::getEventTimestamp() const
{
    return pData->getEventTimestamp();
}

intptr_t Window::getWindowId() const
{
    return pData->getWindowId();
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

END_NAMESPACE_DGL

#undef DBG
#undef DBGF
