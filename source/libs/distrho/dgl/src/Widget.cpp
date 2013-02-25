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

#include "../App.hpp"
#include "../Widget.hpp"
#include "../Window.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Window Private

class Widget::Private
{
public:
    Private(Window* parent)
        : fApp(nullptr),
          fWindow(nullptr)
    {
        if (parent == nullptr)
        {
            fApp = new App;
            fWindow = new Window(fApp);
        }
    }

    ~Private()
    {
        if (fWindow != nullptr)
            delete fWindow;
        if (fApp != nullptr)
            delete fApp;
    }

private:
    App*    fApp;
    Window* fWindow;
};

// -------------------------------------------------
// Widget

Widget::Widget(Window* parent)
    : kPrivate(new Private(parent))
{
}

Widget::~Widget()
{
}

void Widget::onDisplay()
{
    //if (fParent == nullptr)
    //    glColor3f(0.0f, 1.0f, 0.0f);
    //else
    glColor3f(0.0f, 0.0f, 1.0f);
    glRectf(-0.75f, 0.75f, 0.75f, -0.75f);

}

void Widget::onKeyboard(bool, uint32_t)
{
}

void Widget::onMouse(int, bool, int, int)
{
}

void Widget::onMotion(int, int)
{
}

void Widget::onScroll(float, float)
{
}

void Widget::onSpecial(bool, Key)
{
}

void Widget::onReshape(int width, int height)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Widget::onClose()
{
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
