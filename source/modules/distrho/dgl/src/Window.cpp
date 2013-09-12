/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "AppPrivate.hpp"

#include "../Widget.hpp"
#include "../Window.hpp"

#include "pugl/pugl.h"

#if DGL_OS_WINDOWS
# include "pugl/pugl_win.cpp"
#elif DGL_OS_MAC
extern "C" {
# include "pugl/pugl_osx_extended.h"
}
#elif DGL_OS_LINUX
extern "C" {
# include "pugl/pugl_x11.c"
}
#else
# error Unsupported platform!
#endif

#include <cassert>

#define FOR_EACH_WIDGET(it) \
  for (std::list<Widget*>::iterator it = fWidgets.begin(); it != fWidgets.end(); ++it)

#define FOR_EACH_WIDGET_INV(rit) \
  for (std::list<Widget*>::reverse_iterator rit = fWidgets.rbegin(); rit != fWidgets.rend(); ++rit)

START_NAMESPACE_DGL

Window* dgl_lastUiParent = nullptr;

// -----------------------------------------------------------------------
// Window Private

class Window::PrivateData
{
public:
    Private(Window& self, App& app, App::PrivateData& appPriv, PrivateData& parent, intptr_t parentId = 0)
        : kApp(app),
          kAppPriv(appPriv),
          kSelf(self),
          kView(puglCreate(parentId, "Window", 100, 100, false, (parentId != 0))),
          fParent(parent),
          fChildFocus(nullptr),
          fVisible((parentId != 0)),
          fOnModal(false),
          fResizable(false),
#if DGL_OS_WINDOWS
          hwnd(0)
#elif DGL_OS_LINUX
          xDisplay(nullptr),
          xWindow(0)
#else
          _dummy(0)
#endif
    {
    }

    void setup()
    {
        if (kView == nullptr)
            return;

        // we can't have both
        if (parent != nullptr)
        {
            assert(parentId == 0);
        }

        puglSetHandle(kView, this);
        puglSetDisplayFunc(kView, onDisplayCallback);
        puglSetKeyboardFunc(kView, onKeyboardCallback);
        puglSetMotionFunc(kView, onMotionCallback);
        puglSetMouseFunc(kView, onMouseCallback);
        puglSetScrollFunc(kView, onScrollCallback);
        puglSetSpecialFunc(kView, onSpecialCallback);
        puglSetReshapeFunc(kView, onReshapeCallback);
        puglSetCloseFunc(kView, onCloseCallback);

#if DGL_OS_WINDOWS
        PuglInternals* impl = kView->impl;
        hwnd = impl->hwnd;
#elif DGL_OS_LINUX
        PuglInternals* impl = kView->impl;
        xDisplay = impl->display;
        xWindow  = impl->win;

        if (parent != nullptr && parentId == 0)
        {
            PuglInternals* parentImpl = parent->kView->impl;

            XSetTransientForHint(xDisplay, xWindow, parentImpl->win);
            XFlush(xDisplay);
        }
#endif

        kAppPriv->addWindow(kSelf);
    }

    ~Private()
    {
        fOnModal = false;
        fWidgets.clear();

        if (kView != nullptr)
        {
            kAppPriv->removeWindow(kSelf);
            puglDestroy(kView);
        }
    }

    void exec_init()
    {
        fOnModal = true;
        assert(fParent != nullptr);

        if (fParent != nullptr)
        {
            fParent->fChildFocus = this;

#if DGL_OS_WINDOWS
            // Center this window
            PuglInternals* parentImpl = fParent->kView->impl;

            RECT curRect;
            RECT parentRect;
            GetWindowRect(hwnd, &curRect);
            GetWindowRect(parentImpl->hwnd, &parentRect);

            int x = parentRect.left+(parentRect.right-curRect.right)/2;
            int y = parentRect.top+(parentRect.bottom-curRect.bottom)/2;

            SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
            UpdateWindow(hwnd);
#endif

            fParent->show();
        }

        show();
    }

    void exec_fini()
    {
        fOnModal = false;

        if (fParent != nullptr)
            fParent->fChildFocus = nullptr;
    }

    void exec(bool block)
    {
        exec_init();

        if (block)
        {
            while (fVisible && fOnModal)
            {
                // idle()
                puglProcessEvents(kView);

                if (fParent != nullptr)
                    fParent->idle();

                dgl_msleep(10);
            }

            exec_fini();
        }
        else
        {
            idle();
        }
    }

    void focus()
    {
#if DGL_OS_WINDOWS
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
#elif DGL_OS_MAC
		puglImplFocus(kView);
#elif DGL_OS_LINUX
        XRaiseWindow(xDisplay, xWindow);
        XSetInputFocus(xDisplay, xWindow, RevertToPointerRoot, CurrentTime);
        XFlush(xDisplay);
#endif
    }

    void idle()
    {
        puglProcessEvents(kView);

        if (fVisible && fOnModal && fParent != nullptr)
            fParent->idle();
    }

    void repaint()
    {
        puglPostRedisplay(kView);
    }

    void show()
    {
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

    void close()
    {
        setVisible(false, true);
    }

    bool isVisible()
    {
        return fVisible;
    }

    void setResizable(bool yesNo)
    {
        if (fResizable == yesNo)
            return;

        fResizable = yesNo;

        //setSize(kView->width, kView->height, true);
    }

    void setVisible(bool yesNo, bool closed = false)
    {
        if (fVisible == yesNo)
            return;

        fVisible = yesNo;

#if DGL_OS_WINDOWS
        if (yesNo)
        {
            ShowWindow(hwnd, WS_VISIBLE);
            ShowWindow(hwnd, SW_RESTORE);
            //SetForegroundWindow(hwnd);
        }
        else
        {
            ShowWindow(hwnd, SW_HIDE);
        }

        UpdateWindow(hwnd);
#elif DGL_OS_MAC
		puglImplSetVisible(kView, yesNo);
#elif DGL_OS_LINUX
        if (yesNo)
            XMapRaised(xDisplay, xWindow);
        else
            XUnmapWindow(xDisplay, xWindow);

        XFlush(xDisplay);
#endif

        if (yesNo)
        {
            kAppPriv->oneShown();
        }
        else
        {
            if (fOnModal)
                exec_fini();

            if (closed)
                kAppPriv->oneHidden();
        }
    }

    void setSize(unsigned int width, unsigned int height /*, bool forced = false*/)
    {
        if (width == 0)
            width = 1;
        if (height == 0)
            height = 1;

        kView->width  = width;
        kView->height = height;

        //if (kView->width == width && kView->height == height && ! forced)
        //    return;

#if DGL_OS_WINDOWS
        int winFlags = WS_POPUPWINDOW | WS_CAPTION;

        if (fResizable)
            winFlags |= WS_SIZEBOX;

        RECT wr = { 0, 0, (long)width, (long)height };
        AdjustWindowRectEx(&wr, winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(hwnd, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
        UpdateWindow(hwnd);
#elif DGL_OS_MAC
        puglImplSetSize(kView, width, height);
#elif DGL_OS_LINUX
        XResizeWindow(xDisplay, xWindow, width, height);

        if (! fResizable)
        {
            XSizeHints sizeHints;
            memset(&sizeHints, 0, sizeof(sizeHints));

            sizeHints.flags      = PMinSize|PMaxSize;
            sizeHints.min_width  = width;
            sizeHints.min_height = height;
            sizeHints.max_width  = width;
            sizeHints.max_height = height;

            XSetNormalHints(xDisplay, xWindow, &sizeHints);
        }

        XFlush(xDisplay);
#endif

        repaint();
    }

    void setWindowTitle(const char* title)
    {
#if DGL_OS_WINDOWS
        SetWindowTextA(hwnd, title);
#elif DGL_OS_MAC
		puglImplSetTitle(kView, title);
#elif DGL_OS_LINUX
        XStoreName(xDisplay, xWindow, title);
        XFlush(xDisplay);
#endif
    }

    App* getApp() const
    {
        return kApp;
    }

    int getModifiers() const
    {
        return puglGetModifiers(kView);
    }

    intptr_t getWindowId() const
    {
        return puglGetNativeWindow(kView);
    }

    void addWidget(Widget* widget)
    {
        fWidgets.push_back(widget);
    }

    void removeWidget(Widget* widget)
    {
        fWidgets.remove(widget);
    }

protected:
    void onDisplay()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            if (widget->isVisible())
                widget->onDisplay();
        }
    }

    void onKeyboard(bool press, uint32_t key)
    {
        if (fChildFocus != nullptr)
            return fChildFocus->focus();

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);
            if (widget->isVisible())
            {
                if (widget->onKeyboard(press, key))
                    break;
            }
        }
    }

    void onMouse(int button, bool press, int x, int y)
    {
        if (fChildFocus != nullptr)
            return fChildFocus->focus();

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);
            if (widget->isVisible())
            {
                if (widget->onMouse(button, press, x, y))
                    break;
            }
        }
    }

    void onMotion(int x, int y)
    {
        if (fChildFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);
            if (widget->isVisible())
            {
                if (widget->onMotion(x, y))
                    break;
            }
        }
    }

    void onScroll(float dx, float dy)
    {
        if (fChildFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);
            if (widget->isVisible())
            {
                if (widget->onScroll(dx, dy))
                    break;
            }
        }
    }

    void onSpecial(bool press, Key key)
    {
        if (fChildFocus != nullptr)
            return;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);
            if (widget->isVisible())
            {
                if (widget->onSpecial(press, key))
                    break;
            }
        }
    }

    void onReshape(int width, int height)
    {
        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->onReshape(width, height);
        }
    }

    void onClose()
    {
        fOnModal = false;

        if (fChildFocus != nullptr)
            fChildFocus->onClose();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->onClose();
        }

        close();
    }

private:
    App*          const kApp;
    App::Private* const kAppPriv;
    Window*       const kSelf;
    PuglView*     const kView;

    Private* fParent;
    Private* fChildFocus;
    bool     fVisible;
    bool     fOnModal;
    bool     fResizable;

    std::list<Widget*> fWidgets;

#if DGL_OS_WINDOWS
    HWND     hwnd;
#elif DGL_OS_LINUX
    Display* xDisplay;
    ::Window xWindow;
#else
    int      _dummy;
#endif

    // Callbacks
    #define handlePtr ((Private*)puglGetHandle(view))

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

    static void onScrollCallback(PuglView* view, float dx, float dy)
    {
        handlePtr->onScroll(dx, dy);
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
    : pData(new Private(this, app, app->pData, nullptr))
{
    dgl_lastUiParent = this;
}

Window::Window(App& app, Window& parent)
    : pData(new Private(this, app, app->pData, parent->pData))
{
    dgl_lastUiParent = this;
}

Window::Window(App*&app, intptr_t parentId)
    : pData(new Private(this, app, app->pData, nullptr, parentId))
{
    dgl_lastUiParent = this;
}

Window::~Window()
{
    delete pData;
}

void Window::exec(bool lock)
{
    pData->exec(lock);
}

void Window::focus()
{
    pData->focus();
}

void Window::idle()
{
    pData->idle();
}

void Window::repaint()
{
    pData->repaint();
}

bool Window::isVisible()
{
    return pData->isVisible();
}

void Window::setResizable(bool yesNo)
{
    pData->setResizable(yesNo);
}

void Window::setVisible(bool yesNo)
{
    pData->setVisible(yesNo);
}

void Window::setSize(unsigned int width, unsigned int height)
{
    pData->setSize(width, height);
}

void Window::setWindowTitle(const char* title)
{
    pData->setWindowTitle(title);
}

App* Window::getApp() const
{
    return pData->getApp();
}

int Window::getModifiers() const
{
    return pData->getModifiers();
}

intptr_t Window::getWindowId() const
{
    return pData->getWindowId();
}

void Window::addWidget(Widget* widget)
{
    pData->addWidget(widget);
}

void Window::removeWidget(Widget* widget)
{
    pData->removeWidget(widget);
}

void Window::show()
{
    setVisible(true);
}

void Window::hide()
{
    setVisible(false);
}

void Window::close()
{
    pData->close();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
