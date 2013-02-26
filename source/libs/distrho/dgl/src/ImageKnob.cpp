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

#include "../ImageKnob.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

ImageKnob::ImageKnob(Window* parent, const Image& image, Orientation orientation)
    : Widget(parent),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fValue(0.5f),
      fOrientation(orientation),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(nullptr),
      fIsImgVertical(image.getHeight() > image.getWidth()),
      fImgLayerSize(fIsImgVertical ? image.getWidth() : image.getHeight()),
      fImgLayerCount(fIsImgVertical ? image.getHeight()/fImgLayerSize : image.getWidth()/fImgLayerSize),
      fKnobArea(0, 0, fImgLayerSize, fImgLayerSize)
{
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob::ImageKnob(const ImageKnob& imageKnob)
    : Widget(imageKnob.getParent()),
      fImage(imageKnob.fImage),
      fMinimum(imageKnob.fMinimum),
      fMaximum(imageKnob.fMaximum),
      fValue(imageKnob.fValue),
      fOrientation(imageKnob.fOrientation),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(nullptr),
      fIsImgVertical(imageKnob.fIsImgVertical),
      fImgLayerSize(imageKnob.fImgLayerSize),
      fImgLayerCount(imageKnob.fImgLayerCount),
      fKnobArea(imageKnob.fKnobArea)
{
    setSize(fImgLayerSize, fImgLayerSize);
}

float ImageKnob::getValue() const
{
    return fValue;
}

void ImageKnob::setOrientation(Orientation orientation)
{
    if (fOrientation == orientation)
        return;

    fOrientation = orientation;
}

void ImageKnob::setRange(float min, float max)
{
    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr)
            fCallback->imageKnobValueChanged(this, fValue);
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr)
            fCallback->imageKnobValueChanged(this, fValue);
    }

    fMinimum = min;
    fMaximum = max;
}

void ImageKnob::setValue(float value, bool sendCallback)
{
    if (fValue == value)
        return;

    fValue = value;
    repaint();

    if (sendCallback && fCallback != nullptr)
        fCallback->imageKnobValueChanged(this, fValue);
}

void ImageKnob::setCallback(Callback* callback)
{
    fCallback = callback;
}

void ImageKnob::onDisplay()
{
    float normValue = (fValue - fMinimum) / (fMaximum - fMinimum);

    // FIXME: assuming GL_BGRA data (* 4)
    int layerDataSize   = fImgLayerSize * fImgLayerSize * 4;
    int imageDataSize   = layerDataSize * fImgLayerCount;
    int imageDataOffset = imageDataSize - layerDataSize - (layerDataSize * int(normValue * float(fImgLayerCount-1)));

    glRasterPos2i(getX(), getY()+getHeight());
    glDrawPixels(fImgLayerSize, fImgLayerSize, fImage.getFormat(), fImage.getType(), fImage.getRawData() + imageDataOffset);
}

bool ImageKnob::onMouse(int button, bool press, int x, int y)
{
    if (button != 1)
        return false;

    if (press)
    {
        if (! getArea().contains(x, y))
            return false;

        fDragging = true;
        fLastX = x;
        fLastY = y;

        if (fCallback != nullptr)
            fCallback->imageKnobDragStarted(this);

        return true;
    }
    else if (fDragging)
    {
        if (fCallback != nullptr)
            fCallback->imageKnobDragFinished(this);

        fDragging = false;
        return true;
    }

    return false;
}

bool ImageKnob::onMotion(int x, int y)
{
    if (! fDragging)
        return false;

    if (fOrientation == ImageKnob::Horizontal)
    {
        int movX = x - fLastX;

        if (movX != 0)
        {
            float d     = (getModifiers() & MODIFIER_SHIFT) ? 2000.0f : 200.0f;
            float value = fValue + (float(fMaximum - fMinimum) / d * float(movX));

            if (value < fMinimum)
                value = fMinimum;
            else if (value > fMaximum)
                value = fMaximum;

            setValue(value, true);
        }
    }
    else if (fOrientation == ImageKnob::Vertical)
    {
        int movY = fLastY - y;

        if (movY != 0)
        {
            float d     = (getModifiers() & MODIFIER_SHIFT) ? 2000.0f : 200.0f;
            float value = fValue + (float(fMaximum - fMinimum) / d * float(movY));

            if (value < fMinimum)
                value = fMinimum;
            else if (value > fMaximum)
                value = fMaximum;

            setValue(value, true);
        }
    }

    fLastX = x;
    fLastY = y;

    return true;
}

// -------------------------------------------------

END_NAMESPACE_DISTRHO
