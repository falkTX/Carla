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

#ifndef DGL_IMAGE_SLIDER_HPP_INCLUDED
#define DGL_IMAGE_SLIDER_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

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

    explicit ImageSlider(Window& parent, const Image& image, int id = 0) noexcept;
    explicit ImageSlider(Widget* widget, const Image& image, int id = 0) noexcept;
    explicit ImageSlider(const ImageSlider& imageSlider) noexcept;
    ImageSlider& operator=(const ImageSlider& imageSlider) noexcept;

    int getId() const noexcept;
    void setId(int id) noexcept;

    float getValue() const noexcept;

    void setStartPos(const Point<int>& startPos) noexcept;
    void setStartPos(int x, int y) noexcept;
    void setEndPos(const Point<int>& endPos) noexcept;
    void setEndPos(int x, int y) noexcept;

    void setInverted(bool inverted) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    Image fImage;
    int   fId;
    float fMinimum;
    float fMaximum;
    float fStep;
    float fValue;
    float fValueTmp;

    bool fDragging;
    bool fInverted;
    int  fStartedX;
    int  fStartedY;

    Callback* fCallback;

    Point<int> fStartPos;
    Point<int> fEndPos;
    Rectangle<int> fSliderArea;

    void _recheckArea() noexcept;

    // these should not be used
    void setAbsoluteX(int) const noexcept {}
    void setAbsoluteY(int) const noexcept {}
    void setAbsolutePos(int, int) const noexcept {}
    void setAbsolutePos(const Point<int>&) const noexcept {}
    void setNeedsFullViewport(bool) const noexcept {}

    DISTRHO_LEAK_DETECTOR(ImageSlider)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_SLIDER_HPP_INCLUDED
