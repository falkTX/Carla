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

#include "../ImageKnob.hpp"

#include <cassert>
#include <cstdio>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageKnob::ImageKnob(Window& parent, const Image& image, Orientation orientation)
    : Widget(parent),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fValue(0.5f),
      fOrientation(orientation),
      fRotationAngle(0),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(nullptr),
      fIsImgVertical(image.getHeight() > image.getWidth()),
      fImgLayerSize(fIsImgVertical ? image.getWidth() : image.getHeight()),
      fImgLayerCount(fIsImgVertical ? image.getHeight()/fImgLayerSize : image.getWidth()/fImgLayerSize),
      fKnobArea(0, 0, fImgLayerSize, fImgLayerSize),
      fTextureId(0)
{
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob::ImageKnob(Widget* widget, const Image& image, Orientation orientation)
    : Widget(widget->getParentWindow()),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fValue(0.5f),
      fOrientation(orientation),
      fRotationAngle(0),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(nullptr),
      fIsImgVertical(image.getHeight() > image.getWidth()),
      fImgLayerSize(fIsImgVertical ? image.getWidth() : image.getHeight()),
      fImgLayerCount(fIsImgVertical ? image.getHeight()/fImgLayerSize : image.getWidth()/fImgLayerSize),
      fKnobArea(0, 0, fImgLayerSize, fImgLayerSize),
      fTextureId(0)
{
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob::ImageKnob(const ImageKnob& imageKnob)
    : Widget(imageKnob.getParentWindow()),
      fImage(imageKnob.fImage),
      fMinimum(imageKnob.fMinimum),
      fMaximum(imageKnob.fMaximum),
      fValue(imageKnob.fValue),
      fOrientation(imageKnob.fOrientation),
      fRotationAngle(imageKnob.fRotationAngle),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(imageKnob.fCallback),
      fIsImgVertical(imageKnob.fIsImgVertical),
      fImgLayerSize(imageKnob.fImgLayerSize),
      fImgLayerCount(imageKnob.fImgLayerCount),
      fKnobArea(imageKnob.fKnobArea),
      fTextureId(0)
{
    setSize(fImgLayerSize, fImgLayerSize);

    if (fRotationAngle != 0)
    {
        // force new texture creation
        fRotationAngle = 0;
        setRotationAngle(imageKnob.fRotationAngle);
    }
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

void ImageKnob::setRotationAngle(int angle)
{
    if (fRotationAngle == angle)
        return;

    if (fRotationAngle != 0)
    {
        // delete old texture
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }

    fRotationAngle = angle;

    if (angle != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &fTextureId);
        glBindTexture(GL_TEXTURE_2D, fTextureId);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWidth(), getHeight(), 0, fImage.getFormat(), fImage.getType(), fImage.getRawData());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}

void ImageKnob::setCallback(Callback* callback)
{
    fCallback = callback;
}

void ImageKnob::onDisplay()
{
    const float normValue = (fValue - fMinimum) / (fMaximum - fMinimum);

    if (fRotationAngle != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, fTextureId);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWidth(), getHeight(), 0, fImage.getFormat(), fImage.getType(), fImage.getRawData());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glPushMatrix();

        const GLint w2 = getWidth()/2;
        const GLint h2 = getHeight()/2;

        glTranslatef(getX()+w2, getY()+h2, 0.0f);
        glRotatef(normValue*fRotationAngle, 0.0f, 0.0f, 1.0f);

        glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 0.0f);
          glVertex2i(-w2, -h2);

          glTexCoord2f(1.0f, 0.0f);
          glVertex2i(getWidth()-w2, -h2);

          glTexCoord2f(1.0f, 1.0f);
          glVertex2i(getWidth()-w2, getHeight()-h2);

          glTexCoord2f(0.0f, 1.0f);
          glVertex2i(-w2, getHeight()-h2);
        glEnd();

        glPopMatrix();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        const int layerDataSize   = fImgLayerSize * fImgLayerSize * ((fImage.getFormat() == GL_BGRA || fImage.getFormat() == GL_RGBA) ? 4 : 3);
        const int imageDataSize   = layerDataSize * fImgLayerCount;
        const int imageDataOffset = imageDataSize - layerDataSize - (layerDataSize * int(normValue * float(fImgLayerCount-1)));

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glRasterPos2i(getX(), getY()+getHeight());
        glDrawPixels(fImgLayerSize, fImgLayerSize, fImage.getFormat(), fImage.getType(), fImage.getRawData() + imageDataOffset);
    }
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

void ImageKnob::onReshape(int width, int height)
{
//     if (fRotationAngle != 0)
//         glEnable(GL_TEXTURE_2D);

    Widget::onReshape(width, height);
}

void ImageKnob::onClose()
{
    // delete old texture
    setRotationAngle(0);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
