/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "AppPrivate.hpp"

#include "../Widget.hpp"
#include "../Window.hpp"

#include "pugl/pugl.h"

#if DGL_OS_WINDOWS
# include "pugl/pugl_win.cpp"
#elif DGL_OS_MAC
// compiled separately
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

// -------------------------------------------------
// Window Private

class Window::Private
{
public:
    Private(Window* self, App* app, App::Private* appPriv, Private* parent, intptr_t parentId = 0)
        : kApp(app),
          kAppPriv(appPriv),
          kSelf(self),
          kView(puglCreate(parentId, "Window", 600, 500, (parentId != 0), (parentId != 0))),
          fParent(parent),
          fChildFocus(nullptr),
          fVisible((parentId != 0)),
          fClosed(false),
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
        fWidgets.clear();

        if (kView != nullptr)
        {
            kAppPriv->removeWindow(kSelf);
            puglDestroy(kView);
        }
    }

    void exec()
    {
        fClosed = false;

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

        while (isVisible() && ! fClosed)
        {
            idle();

            if (fParent != nullptr)
                fParent->idle();

            dgl_msleep(10);
        }

        fClosed = true;

        if (fParent != nullptr)
        {
            fParent->fChildFocus = nullptr;
        }
    }

    void focus()
    {
#if DGL_OS_WINDOWS
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
#elif DGL_OS_LINUX
        XRaiseWindow(xDisplay, xWindow);
        XSetInputFocus(xDisplay, xWindow, RevertToPointerRoot, CurrentTime);
        XFlush(xDisplay);
#endif
    }

    void idle()
    {
        puglProcessEvents(kView);
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

    bool isVisible()
    {
        return fVisible;
    }

    void setVisible(bool yesNo)
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
#elif DGL_OS_LINUX
        if (yesNo)
            XMapRaised(xDisplay, xWindow);
        else
            XUnmapWindow(xDisplay, xWindow);

        XFlush(xDisplay);
#endif

        if (yesNo)
            kAppPriv->oneShown();
        else
            kAppPriv->oneHidden();
    }

    void setSize(unsigned int width, unsigned int height)
    {
#if DGL_OS_WINDOWS
        int winFlags = WS_POPUPWINDOW | WS_CAPTION;

        if (fResizable)
            winFlags |= WS_SIZEBOX;

        RECT wr = { 0, 0, (long)width, (long)height };
        AdjustWindowRectEx(&wr, winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(hwnd, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
        UpdateWindow(hwnd);
#elif DGL_OS_LINUX
        XSizeHints sizeHints;
        memset(&sizeHints, 0, sizeof(sizeHints));

        sizeHints.flags      = PMinSize|PMaxSize;
        sizeHints.min_width  = width;
        sizeHints.min_height = height;
        sizeHints.max_width  = width;
        sizeHints.max_height = height;
        XSetNormalHints(xDisplay, xWindow, &sizeHints);
        XFlush(xDisplay);
#endif

        repaint();
    }

    void setWindowTitle(const char* title)
    {
#if DGL_OS_WINDOWS
        SetWindowTextA(hwnd, title);
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
        fClosed = true;

        if (fChildFocus != nullptr)
            fChildFocus->onClose();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->onClose();
        }

        hide();
    }

private:
    App*          const kApp;
    App::Private* const kAppPriv;
    Window*       const kSelf;
    PuglView*     const kView;

    Private* fParent;
    Private* fChildFocus;
    bool     fVisible;
    bool     fClosed;
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

// -------------------------------------------------
// Window

Window::Window(App* app, Window* parent)
    : kPrivate(new Private(this, app, app->kPrivate, (parent != nullptr) ? parent->kPrivate : nullptr))
{
    dgl_lastUiParent = this;
}

Window::Window(App* app, intptr_t parentId)
    : kPrivate(new Private(this, app, app->kPrivate, nullptr, parentId))
{
    dgl_lastUiParent = this;
}

Window::~Window()
{
    delete kPrivate;
}

void Window::exec()
{
    kPrivate->exec();
}

void Window::focus()
{
    kPrivate->focus();
}

void Window::idle()
{
    kPrivate->idle();
}

void Window::repaint()
{
    kPrivate->repaint();
}

bool Window::isVisible()
{
    return kPrivate->isVisible();
}

void Window::setVisible(bool yesNo)
{
    kPrivate->setVisible(yesNo);
}

void Window::setSize(unsigned int width, unsigned int height)
{
    kPrivate->setSize(width, height);
}

void Window::setWindowTitle(const char* title)
{
    kPrivate->setWindowTitle(title);
}

App* Window::getApp() const
{
    return kPrivate->getApp();
}

int Window::getModifiers() const
{
    return kPrivate->getModifiers();
}

intptr_t Window::getWindowId() const
{
    return kPrivate->getWindowId();
}

void Window::addWidget(Widget* widget)
{
    kPrivate->addWidget(widget);
}

void Window::removeWidget(Widget* widget)
{
    kPrivate->removeWidget(widget);
}

// -------------------------------------------------

END_NAMESPACE_DGL
