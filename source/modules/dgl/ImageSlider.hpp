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

    ImageSlider(Window& parent, const Image& image);
    ImageSlider(Widget* widget, const Image& image);
    ImageSlider(const ImageSlider& imageSlider);

    float getValue() const;

    void setStartPos(const Point<int>& startPos);
    void setStartPos(int x, int y);
    void setEndPos(const Point<int>& endPos);
    void setEndPos(int x, int y);

    void setRange(float min, float max);
    void setValue(float value, bool sendCallback = false);
    void setIsSwitch(bool yesNo);

    void setCallback(Callback* callback);

protected:
     void onDisplay() override;
     bool onMouse(int button, bool press, int x, int y) override;
     bool onMotion(int x, int y) override;

private:
    Image fImage;
    float fMinimum;
    float fMaximum;
    float fValue;

    bool fIsSwitch;
    bool fDragging;
    int  fStartedX;
    int  fStartedY;

    Callback* fCallback;

    Point<int> fStartPos;
    Point<int> fEndPos;
    Rectangle<int> fSliderArea;

    void _recheckArea();
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_SLIDER_HPP_INCLUDED
