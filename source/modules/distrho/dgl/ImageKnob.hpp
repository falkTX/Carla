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

#ifndef __DGL_IMAGE_KNOB_HPP__
#define __DGL_IMAGE_KNOB_HPP__

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

class ImageKnob : public Widget
{
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageKnobDragStarted(ImageKnob* imageKnob) = 0;
        virtual void imageKnobDragFinished(ImageKnob* imageKnob) = 0;
        virtual void imageKnobValueChanged(ImageKnob* imageKnob, float value) = 0;
    };

    ImageKnob(Window* parent, const Image& image, Orientation orientation = Vertical);
    ImageKnob(Widget* widget, const Image& image, Orientation orientation = Vertical);
    ImageKnob(const ImageKnob& imageKnob);

    float getValue() const;

    void setOrientation(Orientation orientation);
    void setRange(float min, float max);
    void setValue(float value, bool sendCallback = false);
    void setRotationAngle(int angle);

    void setCallback(Callback* callback);

protected:
     void onDisplay() override;
     bool onMouse(int button, bool press, int x, int y) override;
     bool onMotion(int x, int y) override;
     void onReshape(int width, int height) override;
     void onClose() override;

private:
    Image fImage;
    float fMinimum;
    float fMaximum;
    float fValue;
    Orientation fOrientation;

    int  fRotationAngle;
    bool fDragging;
    int  fLastX;
    int  fLastY;

    Callback* fCallback;

    bool fIsImgVertical;
    int  fImgLayerSize;
    int  fImgLayerCount;
    Rectangle<int> fKnobArea;
    GLuint fTextureId;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_IMAGE_KNOB_HPP__
