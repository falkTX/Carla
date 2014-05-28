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

#include "../Widget.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Widget

Widget::Widget(Window& parent)
    : fParent(parent),
      fNeedsFullViewport(false),
      fNeedsScaling(false),
      fVisible(true)
{
    fParent._addWidget(this);
}

Widget::~Widget()
{
    fParent._removeWidget(this);
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

void Widget::setWidth(int width) noexcept
{
    if (fArea.getWidth() == width)
        return;

    ResizeEvent ev;
    ev.oldSize = fArea.getSize();
    ev.size = Size<int>(width, fArea.getHeight());

    fArea.setWidth(width);
    onResize(ev);

    fParent.repaint();
}

void Widget::setHeight(int height) noexcept
{
    if (fArea.getHeight() == height)
        return;

    ResizeEvent ev;
    ev.oldSize = fArea.getSize();
    ev.size = Size<int>(fArea.getWidth(), height);

    fArea.setHeight(height);
    onResize(ev);

    fParent.repaint();
}

void Widget::setSize(int width, int height) noexcept
{
    setSize(Size<int>(width, height));
}

void Widget::setSize(const Size<int>& size) noexcept
{
    if (fArea.getSize() == size)
        return;

    ResizeEvent ev;
    ev.oldSize = fArea.getSize();
    ev.size = size;

    fArea.setSize(size);
    onResize(ev);

    fParent.repaint();
}

int Widget::getAbsoluteX() const noexcept
{
    return fArea.getX();
}

int Widget::getAbsoluteY() const noexcept
{
    return fArea.getY();
}

const Point<int>& Widget::getAbsolutePos() const noexcept
{
    return fArea.getPos();
}

void Widget::setAbsoluteX(int x) noexcept
{
    if (fArea.getX() == x)
        return;

    fArea.setX(x);
    fParent.repaint();
}

void Widget::setAbsoluteY(int y) noexcept
{
    if (fArea.getY() == y)
        return;

    fArea.setY(y);
    fParent.repaint();
}

void Widget::setAbsolutePos(int x, int y) noexcept
{
    setAbsolutePos(Point<int>(x, y));
}

void Widget::setAbsolutePos(const Point<int>& pos) noexcept
{
    if (fArea.getPos() == pos)
        return;

    fArea.setPos(pos);
    fParent.repaint();
}

App& Widget::getParentApp() const noexcept
{
    return fParent.getApp();
}

Window& Widget::getParentWindow() const noexcept
{
    return fParent;
}

bool Widget::contains(int x, int y) const noexcept
{
    return (x >= 0 && y >= 0 && x < fArea.getWidth() && y < fArea.getHeight());
}

bool Widget::contains(const Point<int>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

void Widget::repaint() noexcept
{
    fParent.repaint();
}

bool Widget::onKeyboard(const KeyboardEvent&)
{
    return false;
}

bool Widget::onSpecial(const SpecialEvent&)
{
    return false;
}

bool Widget::onMouse(const MouseEvent&)
{
    return false;
}

bool Widget::onMotion(const MotionEvent&)
{
    return false;
}

bool Widget::onScroll(const ScrollEvent&)
{
    return false;
}

void Widget::onResize(const ResizeEvent&)
{
}

void Widget::setNeedsFullViewport(bool yesNo) noexcept
{
    fNeedsFullViewport = yesNo;
}

void Widget::setNeedsScaling(bool yesNo) noexcept
{
    fNeedsScaling = yesNo;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
