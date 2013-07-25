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

#ifndef DGL_IMAGE_KNOB_HPP_INCLUDED
#define DGL_IMAGE_KNOB_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

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

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_KNOB_HPP_INCLUDED
