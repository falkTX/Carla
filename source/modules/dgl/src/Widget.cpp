/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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
#include "../TopLevelWidget.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Widget

Widget::Widget(TopLevelWidget* const topLevelWidget)
    : pData(new PrivateData(this, topLevelWidget)) {}

Widget::Widget(Widget* const parentWidget)
    : pData(new PrivateData(this, parentWidget)) {}

Widget::~Widget()
{
    delete pData;
}

bool Widget::isVisible() const noexcept
{
    return pData->visible;
}

void Widget::setVisible(bool visible)
{
    if (pData->visible == visible)
        return;

    pData->visible = visible;
    repaint();

    // FIXME check case of hiding a previously visible widget, does it trigger a repaint?
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

const Size<uint> Widget::getSize() const noexcept
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

    repaint();
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

    repaint();
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

    repaint();
}

Application& Widget::getApp() const noexcept
{
    DISTRHO_SAFE_ASSERT(pData->topLevelWidget != nullptr);
    return pData->topLevelWidget->getApp();
}

Window& Widget::getWindow() const noexcept
{
    DISTRHO_SAFE_ASSERT(pData->topLevelWidget != nullptr);
    return pData->topLevelWidget->getWindow();
}

const GraphicsContext& Widget::getGraphicsContext() const noexcept
{
    DISTRHO_SAFE_ASSERT(pData->topLevelWidget != nullptr);
    return pData->topLevelWidget->getWindow().getGraphicsContext();
}

TopLevelWidget* Widget::getTopLevelWidget() const noexcept
{
    return pData->topLevelWidget;
}

std::list<SubWidget*> Widget::getChildren() const noexcept
{
    return pData->subWidgets;
}

void Widget::repaint() noexcept
{
}

uint Widget::getId() const noexcept
{
    return pData->id;
}

const char* Widget::getName() const noexcept
{
    return pData->name != nullptr ? pData->name : "";
}

void Widget::setId(uint id) noexcept
{
    pData->id = id;
}

void Widget::setName(const char* const name) noexcept
{
    std::free(pData->name);
    pData->name = strdup(name);
}

bool Widget::onKeyboard(const KeyboardEvent& ev)
{
    return pData->giveKeyboardEventForSubWidgets(ev);
}

bool Widget::onCharacterInput(const CharacterInputEvent& ev)
{
    return pData->giveCharacterInputEventForSubWidgets(ev);
}

bool Widget::onMouse(const MouseEvent& ev)
{
    MouseEvent rev = ev;
    return pData->giveMouseEventForSubWidgets(rev);
}

bool Widget::onMotion(const MotionEvent& ev)
{
    MotionEvent rev = ev;
    return pData->giveMotionEventForSubWidgets(rev);
}

bool Widget::onScroll(const ScrollEvent& ev)
{
    ScrollEvent rev = ev;
    return pData->giveScrollEventForSubWidgets(rev);
}

void Widget::onResize(const ResizeEvent&)
{
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
