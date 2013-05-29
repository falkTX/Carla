/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "../ImageSlider.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

ImageSlider::ImageSlider(Window* parent, const Image& image)
    : Widget(parent),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fValue(0.5f),
      fDragging(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(nullptr)
{
    setSize(fImage.getSize());
}

ImageSlider::ImageSlider(Widget* widget, const Image& image)
    : Widget(widget->getParent()),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fValue(0.5f),
      fDragging(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(nullptr)
{
    setSize(fImage.getSize());
}

ImageSlider::ImageSlider(const ImageSlider& imageSlider)
    : Widget(imageSlider.getParent()),
      fImage(imageSlider.fImage),
      fMinimum(imageSlider.fMinimum),
      fMaximum(imageSlider.fMaximum),
      fValue(imageSlider.fValue),
      fDragging(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(imageSlider.fCallback),
      fStartPos(imageSlider.fStartPos),
      fEndPos(imageSlider.fEndPos),
      fSliderArea(imageSlider.fSliderArea)
{
    setSize(fImage.getSize());
}

float ImageSlider::getValue() const
{
    return fValue;
}

void ImageSlider::setStartPos(const Point<int>& startPos)
{
    fStartPos = startPos;
    _recheckArea();
}

void ImageSlider::setStartPos(int x, int y)
{
    setStartPos(Point<int>(x, y));
}

void ImageSlider::setEndPos(const Point<int>& endPos)
{
    fEndPos = endPos;
    _recheckArea();
}

void ImageSlider::setEndPos(int x, int y)
{
    setEndPos(Point<int>(x, y));
}

void ImageSlider::setRange(float min, float max)
{
    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr)
            fCallback->imageSliderValueChanged(this, fValue);
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr)
            fCallback->imageSliderValueChanged(this, fValue);
    }

    fMinimum = min;
    fMaximum = max;
}

void ImageSlider::setValue(float value, bool sendCallback)
{
    if (fValue == value)
        return;

    fValue = value;
    repaint();

    if (sendCallback && fCallback != nullptr)
        fCallback->imageSliderValueChanged(this, fValue);
}

void ImageSlider::setCallback(Callback* callback)
{
    fCallback = callback;
}

void ImageSlider::onDisplay()
{
#if 0 // DEBUG, paints slider area
    glColor3f(0.4f, 0.5f, 0.1f);
    glRecti(fSliderArea.getX(), fSliderArea.getY(), fSliderArea.getX()+fSliderArea.getWidth(), fSliderArea.getY()+fSliderArea.getHeight());
#endif

    float normValue = (fValue - fMinimum) / (fMaximum - fMinimum);

    int x, y;

    if (fStartPos.getX() == fEndPos.getX())
    {
        x = fStartPos.getX();
        y = fEndPos.getY() - normValue*(fEndPos.getY()-fStartPos.getY());
    }
    else if (fStartPos.getY() == fEndPos.getY())
    {
        x = fEndPos.getX() - normValue*(fEndPos.getX()-fStartPos.getX());
        y = fStartPos.getY();
    }
    else
        return;

    fImage.draw(x, y);
}

bool ImageSlider::onMouse(int button, bool press, int x, int y)
{
    if (button != 1)
        return false;

    if (press)
    {
        if (! fSliderArea.contains(x, y))
            return false;

        float vper;

        if (fStartPos.getX() == fEndPos.getX())
        {
            // vertical
            vper = float(y - fSliderArea.getY()) / float(fSliderArea.getHeight());
        }
        else if (fStartPos.getY() == fEndPos.getY())
        {
            // horizontal
            vper = float(x - fSliderArea.getX()) / float(fSliderArea.getWidth());
        }
        else
            return false;

        float value = fMaximum - vper * (fMaximum - fMinimum);

        if (value < fMinimum)
            value = fMinimum;
        else if (value > fMaximum)
            value = fMaximum;

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

bool ImageSlider::onMotion(int x, int y)
{
    if (! fDragging)
        return false;

    bool horizontal = fStartPos.getY() == fEndPos.getY();

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

        float value = fMaximum - vper * (fMaximum - fMinimum);

        if (value < fMinimum)
            value = fMinimum;
        else if (value > fMaximum)
            value = fMaximum;

        setValue(value, true);
    }
    else if (y < fSliderArea.getY())
    {
        setValue(fMaximum, true);
    }
    else
    {
        setValue(fMinimum, true);
    }

    return true;
}

void ImageSlider::_recheckArea()
{
    if (fStartPos.getX() == fEndPos.getX())
    {
        fSliderArea = Rectangle<int>(fStartPos.getX(),
                                     fStartPos.getY(),
                                     fImage.getWidth(),
                                     fEndPos.getY() + fImage.getHeight() - fStartPos.getY());
    }
    else if (fStartPos.getY() == fEndPos.getY())
    {
        fSliderArea = Rectangle<int>(fStartPos.getX(),
                                     fStartPos.getY(),
                                     fEndPos.getX() + fImage.getWidth() - fStartPos.getX(),
                                     fImage.getHeight());
    }
}

// -------------------------------------------------

END_NAMESPACE_DGL
