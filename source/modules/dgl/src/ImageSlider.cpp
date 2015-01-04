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

#include "../ImageSlider.hpp"

#include <cmath>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageSlider::ImageSlider(Window& parent, const Image& image, int id) noexcept
    : Widget(parent),
      fImage(image),
      fId(id),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueTmp(fValue),
      fDragging(false),
      fInverted(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(nullptr),
      fStartPos(),
      fEndPos(),
      fSliderArea(),
      leakDetector_ImageSlider()
{
    Widget::setNeedsFullViewport(true);
}

ImageSlider::ImageSlider(Widget* widget, const Image& image, int id) noexcept
    : Widget(widget->getParentWindow()),
      fImage(image),
      fId(id),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueTmp(fValue),
      fDragging(false),
      fInverted(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(nullptr),
      fStartPos(),
      fEndPos(),
      fSliderArea(),
      leakDetector_ImageSlider()
{
    Widget::setNeedsFullViewport(true);
}

ImageSlider::ImageSlider(const ImageSlider& imageSlider) noexcept
    : Widget(imageSlider.getParentWindow()),
      fImage(imageSlider.fImage),
      fId(imageSlider.fId),
      fMinimum(imageSlider.fMinimum),
      fMaximum(imageSlider.fMaximum),
      fStep(imageSlider.fStep),
      fValue(imageSlider.fValue),
      fValueTmp(fValue),
      fDragging(false),
      fInverted(imageSlider.fInverted),
      fStartedX(0),
      fStartedY(0),
      fCallback(imageSlider.fCallback),
      fStartPos(imageSlider.fStartPos),
      fEndPos(imageSlider.fEndPos),
      fSliderArea(imageSlider.fSliderArea),
      leakDetector_ImageSlider()
{
    Widget::setNeedsFullViewport(true);
}

ImageSlider& ImageSlider::operator=(const ImageSlider& imageSlider) noexcept
{
    fImage    = imageSlider.fImage;
    fId       = imageSlider.fId;
    fMinimum  = imageSlider.fMinimum;
    fMaximum  = imageSlider.fMaximum;
    fStep     = imageSlider.fStep;
    fValue    = imageSlider.fValue;
    fValueTmp = fValue;
    fDragging = false;
    fInverted = imageSlider.fInverted;
    fStartedX = 0;
    fStartedY = 0;
    fCallback = imageSlider.fCallback;
    fStartPos = imageSlider.fStartPos;
    fEndPos   = imageSlider.fEndPos;
    fSliderArea = imageSlider.fSliderArea;

    return *this;
}

int ImageSlider::getId() const noexcept
{
    return fId;
}

void ImageSlider::setId(int id) noexcept
{
    fId = id;
}

float ImageSlider::getValue() const noexcept
{
    return fValue;
}

void ImageSlider::setStartPos(const Point<int>& startPos) noexcept
{
    fStartPos = startPos;
    _recheckArea();
}

void ImageSlider::setStartPos(int x, int y) noexcept
{
    setStartPos(Point<int>(x, y));
}

void ImageSlider::setEndPos(const Point<int>& endPos) noexcept
{
    fEndPos = endPos;
    _recheckArea();
}

void ImageSlider::setEndPos(int x, int y) noexcept
{
    setEndPos(Point<int>(x, y));
}

void ImageSlider::setInverted(bool inverted) noexcept
{
    if (fInverted == inverted)
        return;

    fInverted = inverted;
    repaint();
}

void ImageSlider::setRange(float min, float max) noexcept
{
    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageSliderValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageSlider::setRange < min");
        }
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageSliderValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageSlider::setRange > max");
        }
    }

    fMinimum = min;
    fMaximum = max;
}

void ImageSlider::setStep(float step) noexcept
{
    fStep = step;
}

void ImageSlider::setValue(float value, bool sendCallback) noexcept
{
    if (fValue == value)
        return;

    fValue = value;

    if (fStep == 0.0f)
        fValueTmp = value;

    repaint();

    if (sendCallback && fCallback != nullptr)
    {
        try {
            fCallback->imageSliderValueChanged(this, fValue);
        } DISTRHO_SAFE_EXCEPTION("ImageSlider::setValue");
    }
}

void ImageSlider::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageSlider::onDisplay()
{
#if 0 // DEBUG, paints slider area
    glColor3f(0.4f, 0.5f, 0.1f);
    glRecti(fSliderArea.getX(), fSliderArea.getY(), fSliderArea.getX()+fSliderArea.getWidth(), fSliderArea.getY()+fSliderArea.getHeight());
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    float normValue = (fValue - fMinimum) / (fMaximum - fMinimum);

    int x, y;

    if (fStartPos.getY() == fEndPos.getY())
    {
        // horizontal
        if (fInverted)
            x = fEndPos.getX() - static_cast<int>(normValue*static_cast<float>(fEndPos.getX()-fStartPos.getX()));
        else
            x = fStartPos.getX() + static_cast<int>(normValue*static_cast<float>(fEndPos.getX()-fStartPos.getX()));

        y = fStartPos.getY();
    }
    else
    {
        // vertical
        x = fStartPos.getX();

        if (fInverted)
            y = fEndPos.getY() - static_cast<int>(normValue*static_cast<float>(fEndPos.getY()-fStartPos.getY()));
        else
            y = fStartPos.getY() + static_cast<int>(normValue*static_cast<float>(fEndPos.getY()-fStartPos.getY()));
    }

    fImage.drawAt(x, y);
}

bool ImageSlider::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! fSliderArea.contains(ev.pos))
            return false;

        float vper;
        const int x = ev.pos.getX();
        const int y = ev.pos.getY();

        if (fStartPos.getY() == fEndPos.getY())
        {
            // horizontal
            vper = float(x - fSliderArea.getX()) / float(fSliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - fSliderArea.getY()) / float(fSliderArea.getHeight());
        }

        float value;

        if (fInverted)
            value = fMaximum - vper * (fMaximum - fMinimum);
        else
            value = fMinimum + vper * (fMaximum - fMinimum);

        if (value < fMinimum)
        {
            fValueTmp = value = fMinimum;
        }
        else if (value > fMaximum)
        {
            fValueTmp = value = fMaximum;
        }
        else if (fStep != 0.0f)
        {
            fValueTmp = value;
            const float rest = std::fmod(value, fStep);
            value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
        }

        fDragging = true;
        fStartedX = x;
        fStartedY = y;

        if (fCallback != nullptr)
            fCallback->imageSliderDragStarted(this);

        setValue(value, true);

        return true;
    }
    else if (fDragging)
    {
        if (fCallback != nullptr)
            fCallback->imageSliderDragFinished(this);

        fDragging = false;
        return true;
    }

    return false;
}

bool ImageSlider::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;

    const bool horizontal = fStartPos.getY() == fEndPos.getY();
    const int x = ev.pos.getX();
    const int y = ev.pos.getY();

    if ((horizontal && fSliderArea.containsX(x)) || (fSliderArea.containsY(y) && ! horizontal))
    {
        float vper;

        if (horizontal)
        {
            // horizontal
            vper = float(x - fSliderArea.getX()) / float(fSliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - fSliderArea.getY()) / float(fSliderArea.getHeight());
        }

        float value;

        if (fInverted)
            value = fMaximum - vper * (fMaximum - fMinimum);
        else
            value = fMinimum + vper * (fMaximum - fMinimum);

        if (value < fMinimum)
        {
            fValueTmp = value = fMinimum;
        }
        else if (value > fMaximum)
        {
            fValueTmp = value = fMaximum;
        }
        else if (fStep != 0.0f)
        {
            fValueTmp = value;
            const float rest = std::fmod(value, fStep);
            value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
        }

        setValue(value, true);
    }
    else if (horizontal)
    {
        if (x < fSliderArea.getX())
            setValue(fInverted ? fMaximum : fMinimum, true);
        else
            setValue(fInverted ? fMinimum : fMaximum, true);
    }
    else
    {
        if (y < fSliderArea.getY())
            setValue(fInverted ? fMaximum : fMinimum, true);
        else
            setValue(fInverted ? fMinimum : fMaximum, true);
    }

    return true;
}

void ImageSlider::_recheckArea() noexcept
{
    if (fStartPos.getY() == fEndPos.getY())
    {
        // horizontal
        fSliderArea = Rectangle<int>(fStartPos.getX(),
                                     fStartPos.getY(),
                                     fEndPos.getX() + fImage.getWidth() - fStartPos.getX(),
                                     fImage.getHeight());
    }
    else
    {
        // vertical
        fSliderArea = Rectangle<int>(fStartPos.getX(),
                                     fStartPos.getY(),
                                     fImage.getWidth(),
                                     fEndPos.getY() + fImage.getHeight() - fStartPos.getY());
    }
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
