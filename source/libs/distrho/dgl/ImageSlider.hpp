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

#ifndef __DGL_IMAGE_SLIDER_HPP__
#define __DGL_IMAGE_SLIDER_HPP__

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

class ImageSlider : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSliderDragStarted(ImageSlider* imageSlider) = 0;
        virtual void imageSliderDragFinished(ImageSlider* imageSlider) = 0;
        virtual void imageSliderValueChanged(ImageSlider* imageSlider, float value) = 0;
    };

    ImageSlider(Window* parent, const Image& image);
    ImageSlider(const ImageSlider& imageSlider);

    float getValue() const;

    void setStartPos(const Point<int>& startPos);
    void setEndPos(const Point<int>& endPos);

    void setRange(float min, float max);
    void setValue(float value, bool sendCallback = false);

    void setCallback(Callback* callback);

protected:
     void onDisplay();
     bool onMouse(int button, bool press, int x, int y);
     bool onMotion(int x, int y);

private:
    Image fImage;
    float fMinimum;
    float fMaximum;
    float fValue;

    bool fDragging;
    int  fStartedX;
    int  fStartedY;

    Callback* fCallback;

    Point<int> fStartPos;
    Point<int> fEndPos;
    Rectangle<int> fSliderArea;

    void _recheckArea();
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_IMAGE_SLIDER_HPP__
