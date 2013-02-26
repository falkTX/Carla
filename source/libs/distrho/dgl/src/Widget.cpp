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

int Widget::getX() const
{
    return fArea.getX();
}

int Widget::getY() const
{
    return fArea.getY();
}

const Point<int>& Widget::getPos() const
{
    return fArea.getPos();
}

void Widget::setX(int x)
{
    if (fArea.getX() == x)
        return;

    fArea.setX(x);
    repaint();
}

void Widget::setY(int y)
{
    if (fArea.getY() == y)
        return;

    fArea.setY(y);
    repaint();
}

void Widget::setPos(int x, int y)
{
    setPos(Point<int>(x, y));
}

void Widget::setPos(const Point<int>& pos)
{
    if (fArea.getPos() == pos)
        return;

    fArea.setPos(pos);
    repaint();
}

void Widget::move(int x, int y)
{
    fArea.move(x, y);
    repaint();
}

void Widget::move(const Point<int>& pos)
{
    fArea.move(pos);
    repaint();
}

int Widget::getWidth() const
{
    return fArea.getWidth();
}

int Widget::getHeight() const
{
    return fArea.getHeight();
}

const Size<int>& Widget::getSize() const
{
    return fArea.getSize();
}

void Widget::setWidth(int width)
{
    if (fArea.getWidth() == width)
        return;

    fArea.setWidth(width);
    fParent->repaint();
}

void Widget::setHeight(int height)
{
    if (fArea.getHeight() == height)
        return;

    fArea.setHeight(height);
    fParent->repaint();
}

void Widget::setSize(int width, int height)
{
    setSize(Size<int>(width, height));
}

void Widget::setSize(const Size<int>& size)
{
    if (fArea.getSize() == size)
        return;

    fArea.setSize(size);
    fParent->repaint();
}

const Rectangle<int>& Widget::getArea() const
{
    return fArea;
}

int Widget::getModifiers()
{
    return fParent->getModifiers();
}

Window* Widget::getParent() const
{
    return fParent;
}

void Widget::repaint()
{
    fParent->repaint();
}

void Widget::onDisplay()
{
}

bool Widget::onKeyboard(bool, uint32_t)
{
    return false;
}

bool Widget::onMouse(int, bool, int, int)
{
    return false;
}

bool Widget::onMotion(int, int)
{
    return false;
}

bool Widget::onScroll(float, float)
{
    return false;
}

bool Widget::onSpecial(bool, Key)
{
    return false;
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
