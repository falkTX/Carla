/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

TopLevelWidget::PrivateData::PrivateData(TopLevelWidget* const s, Window& w)
    : self(s),
      selfw(s),
      window(w)
{
    /* if window already has a top-level-widget, make the new one match the first one in size
     * this is needed because window creation and resize is a synchronous operation in some systems.
     * as such, there's a chance the non-1st top-level-widgets would never get a valid size.
     */
    if (!window.pData->topLevelWidgets.empty())
    {
        TopLevelWidget* const first = window.pData->topLevelWidgets.front();

        selfw->pData->size = first->getSize();
    }

    window.pData->topLevelWidgets.push_back(self);
}

TopLevelWidget::PrivateData::~PrivateData()
{
    window.pData->topLevelWidgets.remove(self);
}

bool TopLevelWidget::PrivateData::keyboardEvent(const KeyboardEvent& ev)
{
    // ignore event if we are not visible
    if (! selfw->pData->visible)
        return false;

    // propagate event to all subwidgets recursively
    return selfw->pData->giveKeyboardEventForSubWidgets(ev);
}

bool TopLevelWidget::PrivateData::characterInputEvent(const CharacterInputEvent& ev)
{
    // ignore event if we are not visible
    if (! selfw->pData->visible)
        return false;

    // propagate event to all subwidgets recursively
    return selfw->pData->giveCharacterInputEventForSubWidgets(ev);
}

bool TopLevelWidget::PrivateData::mouseEvent(const MouseEvent& ev)
{
    // ignore event if we are not visible
    if (! selfw->pData->visible)
        return false;

    MouseEvent rev = ev;

    if (window.pData->autoScaling)
    {
        const double autoScaleFactor = window.pData->autoScaleFactor;

        rev.pos.setX(ev.pos.getX() / autoScaleFactor);
        rev.pos.setY(ev.pos.getY() / autoScaleFactor);
        rev.absolutePos.setX(ev.absolutePos.getX() / autoScaleFactor);
        rev.absolutePos.setY(ev.absolutePos.getY() / autoScaleFactor);
    }

    // propagate event to all subwidgets recursively
    return selfw->pData->giveMouseEventForSubWidgets(rev);
}

bool TopLevelWidget::PrivateData::motionEvent(const MotionEvent& ev)
{
    // ignore event if we are not visible
    if (! selfw->pData->visible)
        return false;

    MotionEvent rev = ev;

    if (window.pData->autoScaling)
    {
        const double autoScaleFactor = window.pData->autoScaleFactor;

        rev.pos.setX(ev.pos.getX() / autoScaleFactor);
        rev.pos.setY(ev.pos.getY() / autoScaleFactor);
        rev.absolutePos.setX(ev.absolutePos.getX() / autoScaleFactor);
        rev.absolutePos.setY(ev.absolutePos.getY() / autoScaleFactor);
    }

    // propagate event to all subwidgets recursively
    return selfw->pData->giveMotionEventForSubWidgets(rev);
}

bool TopLevelWidget::PrivateData::scrollEvent(const ScrollEvent& ev)
{
    // ignore event if we are not visible
    if (! selfw->pData->visible)
        return false;

    ScrollEvent rev = ev;

    if (window.pData->autoScaling)
    {
        const double autoScaleFactor = window.pData->autoScaleFactor;

        rev.pos.setX(ev.pos.getX() / autoScaleFactor);
        rev.pos.setY(ev.pos.getY() / autoScaleFactor);
        rev.absolutePos.setX(ev.absolutePos.getX() / autoScaleFactor);
        rev.absolutePos.setY(ev.absolutePos.getY() / autoScaleFactor);
        rev.delta.setX(ev.delta.getX() / autoScaleFactor);
        rev.delta.setY(ev.delta.getY() / autoScaleFactor);
    }

    // propagate event to all subwidgets recursively
    return selfw->pData->giveScrollEventForSubWidgets(rev);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
