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

#include <cstdio>

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Widget

Widget::Widget(Window* parent)
    : fParent(parent),
      fVisible(true)
{
    parent->addWidget(this);
}

Widget::~Widget()
{
}

bool Widget::isVisible()
{
    return fVisible;
}

void Widget::setVisible(bool yesNo)
{
    if (yesNo == fVisible)
        return;

    fVisible = yesNo;
    fParent->repaint();
}

void Widget::onDisplay()
{
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
