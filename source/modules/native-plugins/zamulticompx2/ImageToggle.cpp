/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2014 Damien Zammit <damien@zamaudio.com>
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

#include "ImageToggle.hpp"

#include <cassert>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageToggle::ImageToggle(Window& parent, const Image& image)
    : Widget(parent),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
}

ImageToggle::ImageToggle(Widget* widget, const Image& image)
    : Widget(widget->getParentWindow()),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
}

ImageToggle::ImageToggle(Window& parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown)
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

ImageToggle::ImageToggle(Widget* widget, const Image& imageNormal, const Image& imageHover, const Image& imageDown)
    : Widget(widget->getParentWindow()),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
    setValue(0.f);
}

ImageToggle::ImageToggle(const ImageToggle& imageToggle)
    : Widget(imageToggle.getParentWindow()),
      fImageNormal(imageToggle.fImageNormal),
      fImageHover(imageToggle.fImageHover),
      fImageDown(imageToggle.fImageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(imageToggle.fCallback)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

void ImageToggle::setCallback(Callback* callback)
{
    fCallback = callback;
}

float ImageToggle::getValue()
{
    return (fCurImage == &fImageNormal) ? 0.f : 1.f;
}

void ImageToggle::setValue(float value)
{
    if (value == 1.f)
        fCurImage = &fImageDown;
    else if (value == 0.f)
        fCurImage = &fImageNormal;

    repaint();
}

void ImageToggle::onDisplay()
{
    fCurImage->draw(getPos());
}

bool ImageToggle::onMouse(int button, bool press, int x, int y)
{
    if (press && getArea().contains(x, y))
    {
        if (fCurImage != &fImageDown)
        {
            fCurImage = &fImageDown;
        }
        else if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
        }
        
	repaint();
        fCurButton = button;
	if (fCallback != nullptr)
	    fCallback->imageToggleClicked(this, button);

        return true;
    }

    return false;
}

bool ImageToggle::onMotion(int, int)
{
    return false;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
