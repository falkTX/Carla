/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#include "WidgetPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Widget

Widget::Widget(Window& parent)
    : pData(new PrivateData(this, parent, nullptr, false))
{
    parent._addWidget(this);
}

Widget::Widget(Widget* groupWidget)
    : pData(new PrivateData(this, groupWidget->getParentWindow(), groupWidget, true))
{
    pData->parent._addWidget(this);
}

Widget::Widget(Widget* groupWidget, bool addToSubWidgets)
    : pData(new PrivateData(this, groupWidget->getParentWindow(), groupWidget, addToSubWidgets))
{
    pData->parent._addWidget(this);
}

Widget::~Widget()
{
    pData->parent._removeWidget(this);
    delete pData;
}

bool Widget::isVisible() const noexcept
{
    return pData->visible;
}

void Widget::setVisible(bool yesNo)
{
    if (pData->visible == yesNo)
        return;

    pData->visible = yesNo;
    pData->parent.repaint();
}

void Widget::show()
{
    setVisible(true);
}

void Widget::hide()
{
    setVisible(false);
}

uint Widget::getWidth() const noexcept
{
    return pData->size.getWidth();
}

uint Widget::getHeight() const noexcept
{
    return pData->size.getHeight();
}

const Size<uint>& Widget::getSize() const noexcept
{
    return pData->size;
}

void Widget::setWidth(uint width) noexcept
{
    if (pData->size.getWidth() == width)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = Size<uint>(width, pData->size.getHeight());

    pData->size.setWidth(width);
    onResize(ev);

    pData->parent.repaint();
}

void Widget::setHeight(uint height) noexcept
{
    if (pData->size.getHeight() == height)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = Size<uint>(pData->size.getWidth(), height);

    pData->size.setHeight(height);
    onResize(ev);

    pData->parent.repaint();
}

void Widget::setSize(uint width, uint height) noexcept
{
    setSize(Size<uint>(width, height));
}

void Widget::setSize(const Size<uint>& size) noexcept
{
    if (pData->size == size)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = size;

    pData->size = size;
    onResize(ev);

    pData->parent.repaint();
}

int Widget::getAbsoluteX() const noexcept
{
    return pData->absolutePos.getX();
}

int Widget::getAbsoluteY() const noexcept
{
    return pData->absolutePos.getY();
}

const Point<int>& Widget::getAbsolutePos() const noexcept
{
    return pData->absolutePos;
}

void Widget::setAbsoluteX(int x) noexcept
{
    if (pData->absolutePos.getX() == x)
        return;

    pData->absolutePos.setX(x);
    pData->parent.repaint();
}

void Widget::setAbsoluteY(int y) noexcept
{
    if (pData->absolutePos.getY() == y)
        return;

    pData->absolutePos.setY(y);
    pData->parent.repaint();
}

void Widget::setAbsolutePos(int x, int y) noexcept
{
    setAbsolutePos(Point<int>(x, y));
}

void Widget::setAbsolutePos(const Point<int>& pos) noexcept
{
    if (pData->absolutePos == pos)
        return;

    pData->absolutePos = pos;
    pData->parent.repaint();
}

Application& Widget::getParentApp() const noexcept
{
    return pData->parent.getApp();
}

Window& Widget::getParentWindow() const noexcept
{
    return pData->parent;
}

bool Widget::contains(int x, int y) const noexcept
{
    return (x >= 0 && y >= 0 && static_cast<uint>(x) < pData->size.getWidth() && static_cast<uint>(y) < pData->size.getHeight());
}

bool Widget::contains(const Point<int>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

void Widget::repaint() noexcept
{
    pData->parent.repaint();
}

uint Widget::getId() const noexcept
{
    return pData->id;
}

void Widget::setId(uint id) noexcept
{
    pData->id = id;
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

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
