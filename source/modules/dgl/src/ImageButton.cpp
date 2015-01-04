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

#include "../ImageButton.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageButton::ImageButton(Window& parent, const Image& image) noexcept
    : Widget(parent),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr),
      leakDetector_ImageButton() {}

ImageButton::ImageButton(Window& parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown) noexcept
    : Widget(parent),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr),
      leakDetector_ImageButton()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton::ImageButton(Widget* widget, const Image& image) noexcept
    : Widget(widget->getParentWindow()),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr),
      leakDetector_ImageButton() {}

ImageButton::ImageButton(Widget* widget, const Image& imageNormal, const Image& imageHover, const Image& imageDown) noexcept
    : Widget(widget->getParentWindow()),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr),
      leakDetector_ImageButton()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton::ImageButton(const ImageButton& imageButton) noexcept
    : Widget(imageButton.getParentWindow()),
      fImageNormal(imageButton.fImageNormal),
      fImageHover(imageButton.fImageHover),
      fImageDown(imageButton.fImageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(imageButton.fCallback),
      leakDetector_ImageButton()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton& ImageButton::operator=(const ImageButton& imageButton) noexcept
{
    fImageNormal = imageButton.fImageNormal;
    fImageHover  = imageButton.fImageHover;
    fImageDown   = imageButton.fImageDown;
    fCurImage    = &fImageNormal;
    fCurButton   = -1;
    fCallback    = imageButton.fCallback;

    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());

    return *this;
}

void ImageButton::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageButton::onDisplay()
{
    fCurImage->draw();
}

bool ImageButton::onMouse(const MouseEvent& ev)
{
    if (fCurButton != -1 && ! ev.press)
    {
        if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
            repaint();
        }

        if (! contains(ev.pos))
        {
            fCurButton = -1;
            return false;
        }

        if (fCallback != nullptr)
            fCallback->imageButtonClicked(this, fCurButton);

#if 0
        if (contains(ev.pos))
        {
           fCurImage = &fImageHover;
           repaint();
        }
#endif

        fCurButton = -1;

        return true;
    }

    if (ev.press && contains(ev.pos))
    {
        if (fCurImage != &fImageDown)
        {
            fCurImage = &fImageDown;
            repaint();
        }

        fCurButton = ev.button;
        return true;
    }

    return false;
}

bool ImageButton::onMotion(const MotionEvent& ev)
{
    if (fCurButton != -1)
        return true;

    if (contains(ev.pos))
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
