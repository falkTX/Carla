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

#include "../App.hpp"
#include "../Widget.hpp"
#include "../Window.hpp"

#include <cassert>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Widget

Widget::Widget(Window& parent)
    : fParent(parent),
      fVisible(true)
{
    fParent.addWidget(this);
}

Widget::~Widget()
{
    fParent.removeWidget(this);
}

bool Widget::isVisible() const noexcept
{
    return fVisible;
}

void Widget::setVisible(bool yesNo)
{
    if (fVisible == yesNo)
        return;

    fVisible = yesNo;
    fParent.repaint();
}

void Widget::show()
{
    setVisible(true);
}

void Widget::hide()
{
    setVisible(false);
}

int Widget::getX() const noexcept
{
    return fArea.getX();
}

int Widget::getY() const noexcept
{
    return fArea.getY();
}

const Point<int>& Widget::getPos() const noexcept
{
    return fArea.getPos();
}

void Widget::setX(int x)
{
    if (fArea.getX() == x)
        return;

    fArea.setX(x);
    fParent.repaint();
}

void Widget::setY(int y)
{
    if (fArea.getY() == y)
        return;

    fArea.setY(y);
    fParent.repaint();
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
    fParent.repaint();
}

void Widget::move(int x, int y)
{
    fArea.move(x, y);
    fParent.repaint();
}

void Widget::move(const Point<int>& pos)
{
    fArea.move(pos);
    fParent.repaint();
}

int Widget::getWidth() const noexcept
{
    return fArea.getWidth();
}

int Widget::getHeight() const noexcept
{
    return fArea.getHeight();
}

const Size<int>& Widget::getSize() const noexcept
{
    return fArea.getSize();
}

void Widget::setWidth(int width)
{
    if (fArea.getWidth() == width)
        return;

    fArea.setWidth(width);
    fParent.repaint();
}

void Widget::setHeight(int height)
{
    if (fArea.getHeight() == height)
        return;

    fArea.setHeight(height);
    fParent.repaint();
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
    fParent.repaint();
}

const Rectangle<int>& Widget::getArea() const noexcept
{
    return fArea;
}

uint32_t Widget::getEventTimestamp()
{
    return fParent.getEventTimestamp();
}

int Widget::getModifiers()
{
    return fParent.getModifiers();
}

App& Widget::getParentApp() const noexcept
{
    return fParent.getApp();
}

Window& Widget::getParentWindow() const noexcept
{
    return fParent;
}

void Widget::repaint()
{
    fParent.repaint();
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
    glOrtho(0, width, height, 0, 0.0f, 1.0f);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Widget::onClose()
{
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
