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

    explicit ImageKnob(Window& parent, const Image& image, Orientation orientation = Vertical) noexcept;
    explicit ImageKnob(Widget* widget, const Image& image, Orientation orientation = Vertical) noexcept;
    explicit ImageKnob(const ImageKnob& imageKnob);
    ImageKnob& operator=(const ImageKnob& imageKnob);
    ~ImageKnob() override;

    float getValue() const noexcept;

    void setDefault(float def) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;
    void setUsingLogScale(bool yesNo) noexcept;

    void setCallback(Callback* callback) noexcept;
    void setOrientation(Orientation orientation) noexcept;
    void setRotationAngle(int angle);

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;
     bool onScroll(const ScrollEvent&) override;

private:
    Image fImage;
    float fMinimum;
    float fMaximum;
    float fStep;
    float fValue;
    float fValueDef;
    float fValueTmp;
    bool  fUsingDefault;
    bool  fUsingLog;
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
    bool fIsReady;

    float _logscale(float value) const;
    float _invlogscale(float value) const;

    DISTRHO_LEAK_DETECTOR(ImageKnob)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_KNOB_HPP_INCLUDED
