/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#include "SubWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "../TopLevelWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

SubWidget::SubWidget(Widget* const parentWidget)
    : Widget(parentWidget),
      pData(new PrivateData(this, parentWidget)) {}

SubWidget::~SubWidget()
{
    delete pData;
}

template<typename T>
bool SubWidget::contains(const T x, const T y) const noexcept
{
    return Rectangle<double>(0, 0,
                             static_cast<double>(getWidth()),
                             static_cast<double>(getHeight())).contains(x, y);
}

template<typename T>
bool SubWidget::contains(const Point<T>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

int SubWidget::getAbsoluteX() const noexcept
{
    return pData->absolutePos.getX();
}

int SubWidget::getAbsoluteY() const noexcept
{
    return pData->absolutePos.getY();
}

Point<int> SubWidget::getAbsolutePos() const noexcept
{
    return pData->absolutePos;
}

Rectangle<int> SubWidget::getAbsoluteArea() const noexcept
{
    return Rectangle<int>(getAbsolutePos(), getSize().toInt());
}

Rectangle<uint> SubWidget::getConstrainedAbsoluteArea() const noexcept
{
    const int x = getAbsoluteX();
    const int y = getAbsoluteY();

    if (x >= 0 && y >= 0)
        return Rectangle<uint>(static_cast<uint>(x), static_cast<uint>(y), getSize());

    const int xOffset = std::min(0, x);
    const int yOffset = std::min(0, y);
    const int width   = std::max(0, static_cast<int>(getWidth()) + xOffset);
    const int height  = std::max(0, static_cast<int>(getHeight()) + yOffset);

    return Rectangle<uint>(0, 0, static_cast<uint>(width), static_cast<uint>(height));
}

void SubWidget::setAbsoluteX(const int x) noexcept
{
    setAbsolutePos(Point<int>(x, getAbsoluteY()));
}

void SubWidget::setAbsoluteY(const int y) noexcept
{
    setAbsolutePos(Point<int>(getAbsoluteX(), y));
}

void SubWidget::setAbsolutePos(const int x, const int y) noexcept
{
    setAbsolutePos(Point<int>(x, y));
}

void SubWidget::setAbsolutePos(const Point<int>& pos) noexcept
{
    if (pData->absolutePos == pos)
        return;

    PositionChangedEvent ev;
    ev.oldPos = pData->absolutePos;
    ev.pos = pos;

    pData->absolutePos = pos;
    onPositionChanged(ev);

    repaint();
}

Point<int> SubWidget::getMargin() const noexcept
{
    return pData->margin;
}

void SubWidget::setMargin(const int x, const int y) noexcept
{
    pData->margin = Point<int>(x, y);
}

void SubWidget::setMargin(const Point<int>& offset) noexcept
{
    pData->margin = offset;
}

Widget* SubWidget::getParentWidget() const noexcept
{
    return pData->parentWidget;
}

void SubWidget::repaint() noexcept
{
    if (! isVisible())
        return;

    if (TopLevelWidget* const topw = getTopLevelWidget())
    {
        if (pData->needsFullViewportForDrawing)
            // repaint is virtual and we want precisely the top-level specific implementation, not any higher level
            topw->TopLevelWidget::repaint();
        else
            topw->repaint(getConstrainedAbsoluteArea());
    }
}

void SubWidget::toBottom()
{
    std::list<SubWidget*>& subwidgets(pData->parentWidget->pData->subWidgets);

    subwidgets.remove(this);
    subwidgets.insert(subwidgets.begin(), this);
}

void SubWidget::toFront()
{
    std::list<SubWidget*>& subwidgets(pData->parentWidget->pData->subWidgets);

    subwidgets.remove(this);
    subwidgets.push_back(this);
}

void SubWidget::setNeedsFullViewportDrawing(const bool needsFullViewportForDrawing)
{
    pData->needsFullViewportForDrawing = needsFullViewportForDrawing;
}

void SubWidget::setNeedsViewportScaling(const bool needsViewportScaling, const double autoScaleFactor)
{
    pData->needsViewportScaling = needsViewportScaling;
    pData->viewportScaleFactor = autoScaleFactor;
}

void SubWidget::setSkipDrawing(const bool skipDrawing)
{
    pData->skipDrawing = skipDrawing;
}

void SubWidget::onPositionChanged(const PositionChangedEvent&)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Possible template data types

template<>
bool SubWidget::contains(const Point<double>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

// float, int, uint, short, ushort

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
