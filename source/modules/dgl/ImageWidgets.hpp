/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_IMAGE_WIDGETS_HPP_INCLUDED
#define DGL_IMAGE_WIDGETS_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"
#include "Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class ImageAboutWindow : public Window,
                         public Widget
{
public:
    explicit ImageAboutWindow(Window& parent, const Image& image = Image());
    explicit ImageAboutWindow(Widget* widget, const Image& image = Image());

    void setImage(const Image& image);

protected:
    void onDisplay() override;
    bool onKeyboard(const KeyboardEvent&) override;
    bool onMouse(const MouseEvent&) override;
    void onReshape(uint width, uint height) override;

private:
    Image fImgBackground;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageAboutWindow)
};

// -----------------------------------------------------------------------

class ImageButton : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageButtonClicked(ImageButton* imageButton, int button) = 0;
    };

    explicit ImageButton(Window& parent, const Image& image);
    explicit ImageButton(Window& parent, const Image& imageNormal, const Image& imageDown);
    explicit ImageButton(Window& parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown);

    explicit ImageButton(Widget* widget, const Image& image);
    explicit ImageButton(Widget* widget, const Image& imageNormal, const Image& imageDown);
    explicit ImageButton(Widget* widget, const Image& imageNormal, const Image& imageHover, const Image& imageDown);

    ~ImageButton() override;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageButton)
};

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

    void setImageLayerCount(uint count) noexcept;

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
    uint fImgLayerWidth;
    uint fImgLayerHeight;
    uint fImgLayerCount;
    bool fIsReady;
    GLuint fTextureId;

    float _logscale(float value) const;
    float _invlogscale(float value) const;

    DISTRHO_LEAK_DETECTOR(ImageKnob)
};

// -----------------------------------------------------------------------

// note set range and step before setting the value

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

    explicit ImageSlider(Window& parent, const Image& image) noexcept;
    explicit ImageSlider(Widget* widget, const Image& image) noexcept;

    float getValue() const noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;

    void setStartPos(const Point<int>& startPos) noexcept;
    void setStartPos(int x, int y) noexcept;
    void setEndPos(const Point<int>& endPos) noexcept;
    void setEndPos(int x, int y) noexcept;

    void setInverted(bool inverted) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    Image fImage;
    float fMinimum;
    float fMaximum;
    float fStep;
    float fValue;
    float fValueTmp;

    bool fDragging;
    bool fInverted;
    bool fValueIsSet;
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

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageSlider)
};

// -----------------------------------------------------------------------

class ImageSwitch : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSwitchClicked(ImageSwitch* imageButton, bool down) = 0;
    };

    explicit ImageSwitch(Window& parent, const Image& imageNormal, const Image& imageDown) noexcept;
    explicit ImageSwitch(Widget* widget, const Image& imageNormal, const Image& imageDown) noexcept;
    explicit ImageSwitch(const ImageSwitch& imageSwitch) noexcept;
    ImageSwitch& operator=(const ImageSwitch& imageSwitch) noexcept;

    bool isDown() const noexcept;
    void setDown(bool down) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;

private:
    Image fImageNormal;
    Image fImageDown;
    bool  fIsDown;

    Callback* fCallback;

    DISTRHO_LEAK_DETECTOR(ImageSwitch)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_WIDGETS_HPP_INCLUDED
