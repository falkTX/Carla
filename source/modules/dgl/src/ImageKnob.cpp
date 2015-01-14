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

#include "../ImageKnob.hpp"

#include <cmath>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageKnob::ImageKnob(Window& parent, const Image& image, Orientation orientation) noexcept
    : Widget(parent),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueDef(fValue),
      fValueTmp(fValue),
      fUsingDefault(false),
      fUsingLog(false),
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
      fTextureId(0),
      fIsReady(false),
      leakDetector_ImageKnob()
{
    glGenTextures(1, &fTextureId);
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob::ImageKnob(Widget* widget, const Image& image, Orientation orientation) noexcept
    : Widget(widget->getParentWindow()),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueDef(fValue),
      fValueTmp(fValue),
      fUsingDefault(false),
      fUsingLog(false),
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
      fTextureId(0),
      fIsReady(false),
      leakDetector_ImageKnob()
{
    glGenTextures(1, &fTextureId);
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob::ImageKnob(const ImageKnob& imageKnob)
    : Widget(imageKnob.getParentWindow()),
      fImage(imageKnob.fImage),
      fMinimum(imageKnob.fMinimum),
      fMaximum(imageKnob.fMaximum),
      fStep(imageKnob.fStep),
      fValue(imageKnob.fValue),
      fValueDef(imageKnob.fValueDef),
      fValueTmp(fValue),
      fUsingDefault(imageKnob.fUsingDefault),
      fUsingLog(imageKnob.fUsingLog),
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
      fTextureId(0),
      fIsReady(false),
      leakDetector_ImageKnob()
{
    glGenTextures(1, &fTextureId);
    setSize(fImgLayerSize, fImgLayerSize);
}

ImageKnob& ImageKnob::operator=(const ImageKnob& imageKnob)
{
    fImage    = imageKnob.fImage;
    fMinimum  = imageKnob.fMinimum;
    fMaximum  = imageKnob.fMaximum;
    fStep     = imageKnob.fStep;
    fValue    = imageKnob.fValue;
    fValueDef = imageKnob.fValueDef;
    fValueTmp = fValue;
    fUsingDefault  = imageKnob.fUsingDefault;
    fUsingLog      = imageKnob.fUsingLog;
    fOrientation   = imageKnob.fOrientation;
    fRotationAngle = imageKnob.fRotationAngle;
    fDragging = false;
    fLastX    = 0;
    fLastY    = 0;
    fCallback = imageKnob.fCallback;
    fIsImgVertical = imageKnob.fIsImgVertical;
    fImgLayerSize  = imageKnob.fImgLayerSize;
    fImgLayerCount = imageKnob.fImgLayerCount;
    fKnobArea = imageKnob.fKnobArea;
    fIsReady  = false;

    if (fTextureId != 0)
    {
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }

    glGenTextures(1, &fTextureId);
    setSize(fImgLayerSize, fImgLayerSize);

    return *this;
}

ImageKnob::~ImageKnob()
{
    if (fTextureId != 0)
    {
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }
}

float ImageKnob::getValue() const noexcept
{
    return fValue;
}

// NOTE: value is assumed to be scaled if using log
void ImageKnob::setDefault(float value) noexcept
{
    fValueDef = value;
    fUsingDefault = true;
}

void ImageKnob::setRange(float min, float max) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(max > min,);

    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageKnobValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageKnob::setRange < min");
        }
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageKnobValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageKnob::setRange > max");
        }
    }

    fMinimum = min;
    fMaximum = max;
}

void ImageKnob::setStep(float step) noexcept
{
    fStep = step;
}

// NOTE: value is assumed to be scaled if using log
void ImageKnob::setValue(float value, bool sendCallback) noexcept
{
    if (fValue == value)
        return;

    fValue = value;

    if (fStep == 0.0f)
        fValueTmp = value;

    if (fRotationAngle == 0)
        fIsReady = false;

    repaint();

    if (sendCallback && fCallback != nullptr)
    {
        try {
            fCallback->imageKnobValueChanged(this, fValue);
        } DISTRHO_SAFE_EXCEPTION("ImageKnob::setValue");
    }
}

void ImageKnob::setUsingLogScale(bool yesNo) noexcept
{
    fUsingLog = yesNo;
}

void ImageKnob::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageKnob::setOrientation(Orientation orientation) noexcept
{
    if (fOrientation == orientation)
        return;

    fOrientation = orientation;
}

void ImageKnob::setRotationAngle(int angle)
{
    if (fRotationAngle == angle)
        return;

    fRotationAngle = angle;
    fIsReady = false;
}

void ImageKnob::onDisplay()
{
    const float normValue = ((fUsingLog ? _invlogscale(fValue) : fValue) - fMinimum) / (fMaximum - fMinimum);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

    if (! fIsReady)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        int imageDataOffset = 0;

        if (fRotationAngle == 0)
        {
            int layerDataSize = fImgLayerSize * fImgLayerSize * ((fImage.getFormat() == GL_BGRA || fImage.getFormat() == GL_RGBA) ? 4 : 3);
            imageDataOffset   = layerDataSize * int(normValue * float(fImgLayerCount-1));
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWidth(), getHeight(), 0, fImage.getFormat(), fImage.getType(), fImage.getRawData() + imageDataOffset);

        fIsReady = true;
    }

    if (fRotationAngle != 0)
    {
        glPushMatrix();

        const GLint w2 = getWidth()/2;
        const GLint h2 = getHeight()/2;

        glTranslatef(static_cast<float>(w2), static_cast<float>(h2), 0.0f);
        glRotatef(normValue*static_cast<float>(fRotationAngle), 0.0f, 0.0f, 1.0f);

        Rectangle<int>(-w2, -h2, getWidth(), getHeight()).draw();

        glPopMatrix();
    }
    else
    {
        Rectangle<int>(0, 0, getWidth(), getHeight()).draw();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

bool ImageKnob::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! contains(ev.pos))
            return false;

        if ((ev.mod & MODIFIER_SHIFT) != 0 && fUsingDefault)
        {
            setValue(fValueDef, true);
            fValueTmp = fValue;
            return true;
        }

        fDragging = true;
        fLastX = ev.pos.getX();
        fLastY = ev.pos.getY();

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

bool ImageKnob::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;

    bool doVal = false;
    float d, value = 0.0f;

    if (fOrientation == ImageKnob::Horizontal)
    {
        if (const int movX = ev.pos.getX() - fLastX)
        {
            d     = (ev.mod & MODIFIER_CTRL) ? 2000.0f : 200.0f;
            value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * float(movX));
            doVal = true;
        }
    }
    else if (fOrientation == ImageKnob::Vertical)
    {
        if (const int movY = fLastY - ev.pos.getY())
        {
            d     = (ev.mod & MODIFIER_CTRL) ? 2000.0f : 200.0f;
            value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * float(movY));
            doVal = true;
        }
    }

    if (! doVal)
        return false;

    if (fUsingLog)
        value = _logscale(value);

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

    fLastX = ev.pos.getX();
    fLastY = ev.pos.getY();

    return true;
}

bool ImageKnob::onScroll(const ScrollEvent& ev)
{
    if (! contains(ev.pos))
        return false;

    const float d     = (ev.mod & MODIFIER_CTRL) ? 2000.0f : 200.0f;
    float       value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * 10.f * ev.delta.getY());

    if (fUsingLog)
        value = _logscale(value);

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
    return true;
}

// -----------------------------------------------------------------------

float ImageKnob::_logscale(float value) const
{
    const float b = std::log(fMaximum/fMinimum)/(fMaximum-fMinimum);
    const float a = fMaximum/std::exp(fMaximum*b);
    return a * std::exp(b*value);
}

float ImageKnob::_invlogscale(float value) const
{
    const float b = std::log(fMaximum/fMinimum)/(fMaximum-fMinimum);
    const float a = fMaximum/std::exp(fMaximum*b);
    return std::log(value/a)/b;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
