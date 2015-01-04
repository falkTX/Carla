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

#include "../ImageSwitch.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageSwitch::ImageSwitch(Window& parent, const Image& imageNormal, const Image& imageDown, int id) noexcept
    : Widget(parent),
      fImageNormal(imageNormal),
      fImageDown(imageDown),
      fIsDown(false),
      fId(id),
      fCallback(nullptr),
      leakDetector_ImageSwitch()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

    setSize(fImageNormal.getSize());
}

ImageSwitch::ImageSwitch(Widget* widget, const Image& imageNormal, const Image& imageDown, int id) noexcept
    : Widget(widget->getParentWindow()),
      fImageNormal(imageNormal),
      fImageDown(imageDown),
      fIsDown(false),
      fId(id),
      fCallback(nullptr),
      leakDetector_ImageSwitch()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

    setSize(fImageNormal.getSize());
}

ImageSwitch::ImageSwitch(const ImageSwitch& imageSwitch) noexcept
    : Widget(imageSwitch.getParentWindow()),
      fImageNormal(imageSwitch.fImageNormal),
      fImageDown(imageSwitch.fImageDown),
      fIsDown(imageSwitch.fIsDown),
      fId(imageSwitch.fId),
      fCallback(imageSwitch.fCallback),
      leakDetector_ImageSwitch()
{
    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

    setSize(fImageNormal.getSize());
}

ImageSwitch& ImageSwitch::operator=(const ImageSwitch& imageSwitch) noexcept
{
    fImageNormal = imageSwitch.fImageNormal;
    fImageDown   = imageSwitch.fImageDown;
    fIsDown      = imageSwitch.fIsDown;
    fId          = imageSwitch.fId;
    fCallback    = imageSwitch.fCallback;

    DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

    setSize(fImageNormal.getSize());

    return *this;
}

int ImageSwitch::getId() const noexcept
{
    return fId;
}

void ImageSwitch::setId(int id) noexcept
{
    fId = id;;
}

bool ImageSwitch::isDown() const noexcept
{
    return fIsDown;
}

void ImageSwitch::setDown(bool down) noexcept
{
    if (fIsDown == down)
        return;

    fIsDown = down;
    repaint();
}

void ImageSwitch::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageSwitch::onDisplay()
{
    if (fIsDown)
        fImageNormal.draw();
    else
        fImageDown.draw();
}

bool ImageSwitch::onMouse(const MouseEvent& ev)
{
    if (ev.press && contains(ev.pos))
    {
        fIsDown = !fIsDown;

        repaint();

        if (fCallback != nullptr)
            fCallback->imageSwitchClicked(this, fIsDown);

        return true;
    }

    return false;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
