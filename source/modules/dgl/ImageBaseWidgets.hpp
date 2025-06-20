/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
#define DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED

#include "EventHandlers.hpp"
#include "StandaloneWindow.hpp"
#include "SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   DGL Image About Window class.

   This is a Window attached (transient) to another Window that simply shows an Image as its content.
   It is typically used for "about this project" style pop-up Windows.

   Pressing 'Esc' or clicking anywhere on the window will automatically close it.

   @see CairoImageAboutWindow, OpenGLImageAboutWindow, Window::runAsModal(bool)
 */
template <class ImageType>
class ImageBaseAboutWindow : public StandaloneWindow
{
public:
   /**
      Constructor taking an existing Window as the parent transient window and an optional image.
      If @a image is valid, the about window size will match the image size.
    */
    explicit ImageBaseAboutWindow(Window& transientParentWindow, const ImageType& image = ImageType());

   /**
      Constructor taking a top-level-widget's Window as the parent transient window and an optional image.
      If @a image is valid, the about window size will match the image size.
    */
    explicit ImageBaseAboutWindow(TopLevelWidget* topLevelWidget, const ImageType& image = ImageType());

   /**
      Set a new image to use as background for this window.
      Window size will adjust to match the image size.
    */
    void setImage(const ImageType& image);

protected:
    void onDisplay() override;
    bool onKeyboard(const KeyboardEvent&) override;
    bool onMouse(const MouseEvent&) override;

private:
    ImageType img;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseAboutWindow)
};

// --------------------------------------------------------------------------------------------------------------------

/**
   DGL Image Button class.

   This is a typical button, where the drawing comes from a pregenerated set of images.
   The button can be under "normal", "hover" and "down" states, with one separate image possible for each.

   The event logic for this button comes from the ButtonEventHandler class.

   @see CairoImageButton, OpenGLImageButton
 */
template <class ImageType>
class ImageBaseButton : public SubWidget,
                        public ButtonEventHandler
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageButtonClicked(ImageBaseButton* imageButton, int button) = 0;
    };

    explicit ImageBaseButton(Widget* parentWidget, const ImageType& image);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageDown);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageHover, const ImageType& imageDown);

    ~ImageBaseButton() override;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseButton)
};

// --------------------------------------------------------------------------------------------------------------------

/**
   DGL Image Knob class.

   This is a typical knob/dial, where the drawing comes from a pregenerated image "filmstrip".
   The knob's "filmstrip" image can be either horizontal or vertical,
   with the number of steps automatically based on the largest value (ie, horizontal if width>height, vertical if height>width).
   There are no different images for "hover" or "down" states.

   The event logic for this knob comes from the KnobEventHandler class.

   @see CairoImageKnob, OpenGLImageKnob
 */
template <class ImageType>
class ImageBaseKnob : public SubWidget,
                      public KnobEventHandler
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageKnobDragStarted(ImageBaseKnob* imageKnob) = 0;
        virtual void imageKnobDragFinished(ImageBaseKnob* imageKnob) = 0;
        virtual void imageKnobValueChanged(ImageBaseKnob* imageKnob, float value) = 0;
        virtual void imageKnobDoubleClicked(ImageBaseKnob*) {};
    };

    explicit ImageBaseKnob(Widget* parentWidget, const ImageType& image, Orientation orientation = Vertical) noexcept;
    explicit ImageBaseKnob(const ImageBaseKnob& imageKnob);
    ImageBaseKnob& operator=(const ImageBaseKnob& imageKnob);
    ~ImageBaseKnob() override;

    void setCallback(Callback* callback) noexcept;
    void setImageLayerCount(uint count) noexcept;
    void setRotationAngle(int angle);
    bool setValue(float value, bool sendCallback = false) noexcept override;

protected:
    void onDisplay() override;
    bool onMouse(const MouseEvent&) override;
    bool onMotion(const MotionEvent&) override;
    bool onScroll(const ScrollEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_LEAK_DETECTOR(ImageBaseKnob)
};

// --------------------------------------------------------------------------------------------------------------------

// note set range and step before setting the value

template <class ImageType>
class ImageBaseSlider : public SubWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSliderDragStarted(ImageBaseSlider* imageSlider) = 0;
        virtual void imageSliderDragFinished(ImageBaseSlider* imageSlider) = 0;
        virtual void imageSliderValueChanged(ImageBaseSlider* imageSlider, float value) = 0;
    };

    explicit ImageBaseSlider(Widget* parentWidget, const ImageType& image) noexcept;
    ~ImageBaseSlider() override;

    float getValue() const noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;
    void setDefault(float def) noexcept;

    void setStartPos(const Point<int>& startPos) noexcept;
    void setStartPos(int x, int y) noexcept;
    void setEndPos(const Point<int>& endPos) noexcept;
    void setEndPos(int x, int y) noexcept;

    void setCheckable(bool checkable) noexcept;
    void setInverted(bool inverted) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    // these should not be used
    void setAbsoluteX(int) const noexcept {}
    void setAbsoluteY(int) const noexcept {}
    void setAbsolutePos(int, int) const noexcept {}
    void setAbsolutePos(const Point<int>&) const noexcept {}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseSlider)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseSwitch : public SubWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSwitchClicked(ImageBaseSwitch* imageSwitch, bool down) = 0;
    };

    explicit ImageBaseSwitch(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageDown) noexcept;
    explicit ImageBaseSwitch(const ImageBaseSwitch& imageSwitch) noexcept;
    ImageBaseSwitch& operator=(const ImageBaseSwitch& imageSwitch) noexcept;
    ~ImageBaseSwitch() override;

    bool isDown() const noexcept;
    void setDown(bool down) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_LEAK_DETECTOR(ImageBaseSwitch)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
