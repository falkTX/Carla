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

#include "../ImageButton.hpp"

#include <cassert>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageButton::ImageButton(Window* parent, const Image& image)
    : Widget(parent),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
}

ImageButton::ImageButton(Widget* widget, const Image& image)
    : Widget(widget->getParent()),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
}

ImageButton::ImageButton(Window* parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown)
    : Widget(parent),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton::ImageButton(Widget* widget, const Image& imageNormal, const Image& imageHover, const Image& imageDown)
    : Widget(widget->getParent()),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton::ImageButton(const ImageButton& imageButton)
    : Widget(imageButton.getParent()),
      fImageNormal(imageButton.fImageNormal),
      fImageHover(imageButton.fImageHover),
      fImageDown(imageButton.fImageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(imageButton.fCallback)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

void ImageButton::setCallback(Callback* callback)
{
    fCallback = callback;
}

void ImageButton::onDisplay()
{
    fCurImage->draw(getPos());
}

bool ImageButton::onMouse(int button, bool press, int x, int y)
{
    if (fCurButton != -1 && ! press)
    {
        if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
            repaint();
        }

        if (! getArea().contains(x, y))
        {
            fCurButton = -1;
            return false;
        }

        if (fCallback != nullptr)
            fCallback->imageButtonClicked(this, fCurButton);

        //if (getArea().contains(x, y))
        //{
        //    fCurImage = &fImageHover;
        //    repaint();
        //}

        fCurButton = -1;

        return true;
    }

    if (press && getArea().contains(x, y))
    {
        if (fCurImage != &fImageDown)
        {
            fCurImage = &fImageDown;
            repaint();
        }

        fCurButton = button;
        return true;
    }

    return false;
}

bool ImageButton::onMotion(int x, int y)
{
    if (fCurButton != -1)
        return true;

    if (getArea().contains(x, y))
    {
        if (fCurImage != &fImageHover)
        {
            fCurImage = &fImageHover;
            repaint();
        }

        return true;
    }
    else
    {
        if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
            repaint();
        }

        return false;
    }
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
