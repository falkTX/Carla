/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "../ImageBaseWidgets.hpp"
#include "../Color.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseAboutWindow<ImageType>::ImageBaseAboutWindow(Window& transientParentWindow, const ImageType& image)
    : StandaloneWindow(transientParentWindow.getApp(), transientParentWindow),
      img(image)
{
    setResizable(false);
    setTitle("About");

    if (image.isValid())
    {
        setSize(image.getSize());
        setGeometryConstraints(image.getWidth(), image.getHeight(), true, true);
    }

    done();
}

template <class ImageType>
ImageBaseAboutWindow<ImageType>::ImageBaseAboutWindow(TopLevelWidget* const topLevelWidget, const ImageType& image)
    : StandaloneWindow(topLevelWidget->getApp(), topLevelWidget->getWindow()),
      img(image)
{
    setResizable(false);
    setTitle("About");

    if (image.isValid())
    {
        setSize(image.getSize());
        setGeometryConstraints(image.getWidth(), image.getHeight(), true, true);
    }

    done();
}

template <class ImageType>
void ImageBaseAboutWindow<ImageType>::setImage(const ImageType& image)
{
    if (img == image)
        return;

    if (image.isInvalid())
    {
        img = image;
        return;
    }

    reinit();

    img = image;

    setSize(image.getSize());
    setGeometryConstraints(image.getWidth(), image.getHeight(), true, true);

    done();
}

template <class ImageType>
void ImageBaseAboutWindow<ImageType>::onDisplay()
{
    img.draw(getGraphicsContext());
}

template <class ImageType>
bool ImageBaseAboutWindow<ImageType>::onKeyboard(const KeyboardEvent& ev)
{
    if (ev.press && ev.key == kKeyEscape)
    {
        close();
        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseAboutWindow<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.press)
    {
        close();
        return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseButton<ImageType>::PrivateData : public ButtonEventHandler::Callback {
    ImageBaseButton<ImageType>::Callback* callback;
    ImageType imageNormal;
    ImageType imageHover;
    ImageType imageDown;

    PrivateData(const ImageType& normal, const ImageType& hover, const ImageType& down)
        : callback(nullptr),
          imageNormal(normal),
          imageHover(hover),
          imageDown(down) {}

    void buttonClicked(SubWidget* widget, int button) override
    {
        if (callback != nullptr)
            if (ImageBaseButton* const imageButton = dynamic_cast<ImageBaseButton*>(widget))
                callback->imageButtonClicked(imageButton, button);
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& image)
    : SubWidget(parentWidget),
      ButtonEventHandler(this),
      pData(new PrivateData(image, image, image))
{
    ButtonEventHandler::setCallback(pData);
    setSize(image.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageDown)
    : SubWidget(parentWidget),
      ButtonEventHandler(this),
      pData(new PrivateData(imageNormal, imageNormal, imageDown))
{
    DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());

    ButtonEventHandler::setCallback(pData);
    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageHover, const ImageType& imageDown)
    : SubWidget(parentWidget),
      ButtonEventHandler(this),
      pData(new PrivateData(imageNormal, imageHover, imageDown))
{
    DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageHover.getSize() && imageHover.getSize() == imageDown.getSize());

    ButtonEventHandler::setCallback(pData);
    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::~ImageBaseButton()
{
    delete pData;
}

template <class ImageType>
void ImageBaseButton<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseButton<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

    const State state = ButtonEventHandler::getState();

    if (ButtonEventHandler::isCheckable())
    {
        if (ButtonEventHandler::isChecked())
            pData->imageDown.draw(context);
        else if (state & kButtonStateHover)
            pData->imageHover.draw(context);
        else
            pData->imageNormal.draw(context);
    }
    else
    {
        if (state & kButtonStateActive)
            pData->imageDown.draw(context);
        else if (state & kButtonStateHover)
            pData->imageHover.draw(context);
        else
            pData->imageNormal.draw(context);
    }
}

template <class ImageType>
bool ImageBaseButton<ImageType>::onMouse(const MouseEvent& ev)
{
    if (SubWidget::onMouse(ev))
        return true;
    return ButtonEventHandler::mouseEvent(ev);
}

template <class ImageType>
bool ImageBaseButton<ImageType>::onMotion(const MotionEvent& ev)
{
    if (SubWidget::onMotion(ev))
        return true;
    return ButtonEventHandler::motionEvent(ev);
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseKnob<ImageType>::PrivateData : public KnobEventHandler::Callback {
    ImageBaseKnob<ImageType>::Callback* callback;
    ImageType image;

    int rotationAngle;

    bool alwaysRepaint;
    bool isImgVertical;
    uint imgLayerWidth;
    uint imgLayerHeight;
    uint imgLayerCount;
    bool isReady;

    union {
        uint glTextureId;
        void* cairoSurface;
    };

    explicit PrivateData(const ImageType& img)
        : callback(nullptr),
          image(img),
          rotationAngle(0),
          alwaysRepaint(false),
          isImgVertical(img.getHeight() > img.getWidth()),
          imgLayerWidth(isImgVertical ? img.getWidth() : img.getHeight()),
          imgLayerHeight(imgLayerWidth),
          imgLayerCount(isImgVertical ? img.getHeight()/imgLayerHeight : img.getWidth()/imgLayerWidth),
          isReady(false)
    {
        init();
    }

    explicit PrivateData(PrivateData* const other)
        : callback(other->callback),
          image(other->image),
          rotationAngle(other->rotationAngle),
          alwaysRepaint(other->alwaysRepaint),
          isImgVertical(other->isImgVertical),
          imgLayerWidth(other->imgLayerWidth),
          imgLayerHeight(other->imgLayerHeight),
          imgLayerCount(other->imgLayerCount),
          isReady(false)
    {
        init();
    }

    void assignFrom(PrivateData* const other)
    {
        cleanup();
        image          = other->image;
        rotationAngle  = other->rotationAngle;
        callback       = other->callback;
        alwaysRepaint  = other->alwaysRepaint;
        isImgVertical  = other->isImgVertical;
        imgLayerWidth  = other->imgLayerWidth;
        imgLayerHeight = other->imgLayerHeight;
        imgLayerCount  = other->imgLayerCount;
        isReady        = false;
        init();
    }

    ~PrivateData()
    {
        cleanup();
    }

    void knobDragStarted(SubWidget* const widget) override
    {
        if (callback != nullptr)
            if (ImageBaseKnob* const imageKnob = dynamic_cast<ImageBaseKnob*>(widget))
                callback->imageKnobDragStarted(imageKnob);
    }

    void knobDragFinished(SubWidget* const widget) override
    {
        if (callback != nullptr)
            if (ImageBaseKnob* const imageKnob = dynamic_cast<ImageBaseKnob*>(widget))
                callback->imageKnobDragFinished(imageKnob);
    }

    void knobValueChanged(SubWidget* const widget, const float value) override
    {
        if (rotationAngle == 0 || alwaysRepaint)
            isReady = false;

        if (callback != nullptr)
            if (ImageBaseKnob* const imageKnob = dynamic_cast<ImageBaseKnob*>(widget))
                callback->imageKnobValueChanged(imageKnob, value);
    }

    void knobDoubleClicked(SubWidget* const widget) override
    {
        if (callback != nullptr)
            if (ImageBaseKnob* const imageKnob = dynamic_cast<ImageBaseKnob*>(widget))
                callback->imageKnobDoubleClicked(imageKnob);
    }

    // implemented independently per graphics backend
    void init();
    void cleanup();

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseKnob<ImageType>::ImageBaseKnob(Widget* const parentWidget,
                                        const ImageType& image,
                                        const Orientation orientation) noexcept
    : SubWidget(parentWidget),
      KnobEventHandler(this),
      pData(new PrivateData(image))
{
    KnobEventHandler::setCallback(pData);
    setOrientation(orientation);
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
ImageBaseKnob<ImageType>::ImageBaseKnob(const ImageBaseKnob<ImageType>& imageKnob)
    : SubWidget(imageKnob.getParentWidget()),
      KnobEventHandler(this, imageKnob),
      pData(new PrivateData(imageKnob.pData))
{
    KnobEventHandler::setCallback(pData);
    setOrientation(imageKnob.getOrientation());
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
ImageBaseKnob<ImageType>& ImageBaseKnob<ImageType>::operator=(const ImageBaseKnob<ImageType>& imageKnob)
{
    KnobEventHandler::operator=(imageKnob);
    pData->assignFrom(imageKnob.pData);
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
    return *this;
}

template <class ImageType>
ImageBaseKnob<ImageType>::~ImageBaseKnob()
{
    delete pData;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setImageLayerCount(uint count) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(count > 1,);

    pData->imgLayerCount = count;

    if (pData->isImgVertical)
        pData->imgLayerHeight = pData->image.getHeight()/count;
    else
        pData->imgLayerWidth = pData->image.getWidth()/count;

    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setRotationAngle(int angle)
{
    if (pData->rotationAngle == angle)
        return;

    pData->rotationAngle = angle;
    pData->isReady = false;
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::setValue(float value, bool sendCallback) noexcept
{
    if (KnobEventHandler::setValue(value, sendCallback))
    {
        if (pData->rotationAngle == 0 || pData->alwaysRepaint)
            pData->isReady = false;

        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onMouse(const MouseEvent& ev)
{
    if (SubWidget::onMouse(ev))
        return true;
    return KnobEventHandler::mouseEvent(ev, getTopLevelWidget()->getScaleFactor());
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onMotion(const MotionEvent& ev)
{
    if (SubWidget::onMotion(ev))
        return true;
    return KnobEventHandler::motionEvent(ev, getTopLevelWidget()->getScaleFactor());
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onScroll(const ScrollEvent& ev)
{
    if (SubWidget::onScroll(ev))
        return true;
    return KnobEventHandler::scrollEvent(ev);
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseSlider<ImageType>::PrivateData {
    ImageType image;
    float minimum;
    float maximum;
    float step;
    float value;
    float valueDef;
    float valueTmp;
    bool  usingDefault;

    bool dragging;
    bool checkable;
    bool inverted;
    bool valueIsSet;
    double startedX;
    double startedY;

    Callback* callback;

    Point<int> startPos;
    Point<int> endPos;
    Rectangle<double> sliderArea;

    PrivateData(const ImageType& img)
        : image(img),
          minimum(0.0f),
          maximum(1.0f),
          step(0.0f),
          value(0.5f),
          valueDef(value),
          valueTmp(value),
          usingDefault(false),
          dragging(false),
          checkable(false),
          inverted(false),
          valueIsSet(false),
          startedX(0.0),
          startedY(0.0),
          callback(nullptr),
          startPos(),
          endPos(),
          sliderArea() {}

    void recheckArea() noexcept
    {
        if (startPos.getY() == endPos.getY())
        {
            // horizontal
            sliderArea = Rectangle<double>(startPos.getX(),
                                           startPos.getY(),
                                           endPos.getX() + static_cast<int>(image.getWidth()) - startPos.getX(),
                                           static_cast<int>(image.getHeight()));
        }
        else
        {
            // vertical
            sliderArea = Rectangle<double>(startPos.getX(),
                                           startPos.getY(),
                                           static_cast<int>(image.getWidth()),
                                           endPos.getY() + static_cast<int>(image.getHeight()) - startPos.getY());
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseSlider<ImageType>::ImageBaseSlider(Widget* const parentWidget, const ImageType& image) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(image))
{
    setNeedsFullViewportDrawing();
}

template <class ImageType>
ImageBaseSlider<ImageType>::~ImageBaseSlider()
{
    delete pData;
}

template <class ImageType>
float ImageBaseSlider<ImageType>::getValue() const noexcept
{
    return pData->value;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setValue(float value, bool sendCallback) noexcept
{
    if (! pData->valueIsSet)
        pData->valueIsSet = true;

    if (d_isEqual(pData->value, value))
        return;

    pData->value = value;

    if (d_isZero(pData->step))
        pData->valueTmp = value;

    repaint();

    if (sendCallback && pData->callback != nullptr)
    {
        try {
            pData->callback->imageSliderValueChanged(this, pData->value);
        } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setValue");
    }
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStartPos(const Point<int>& startPos) noexcept
{
    pData->startPos = startPos;
    pData->recheckArea();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStartPos(int x, int y) noexcept
{
    setStartPos(Point<int>(x, y));
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setEndPos(const Point<int>& endPos) noexcept
{
    pData->endPos = endPos;
    pData->recheckArea();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setEndPos(int x, int y) noexcept
{
    setEndPos(Point<int>(x, y));
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setCheckable(bool checkable) noexcept
{
    if (pData->checkable == checkable)
        return;

    pData->checkable = checkable;
    repaint();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setInverted(bool inverted) noexcept
{
    if (pData->inverted == inverted)
        return;

    pData->inverted = inverted;
    repaint();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setDefault(float value) noexcept
{
    pData->valueDef = value;
    pData->usingDefault = true;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setRange(float min, float max) noexcept
{
    pData->minimum = min;
    pData->maximum = max;

    if (pData->value < min)
    {
        pData->value = min;
        repaint();

        if (pData->callback != nullptr && pData->valueIsSet)
        {
            try {
                pData->callback->imageSliderValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setRange < min");
        }
    }
    else if (pData->value > max)
    {
        pData->value = max;
        repaint();

        if (pData->callback != nullptr && pData->valueIsSet)
        {
            try {
                pData->callback->imageSliderValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setRange > max");
        }
    }
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStep(float step) noexcept
{
    pData->step = step;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

#if 0 // DEBUG, paints slider area
    Color(1.0f, 1.0f, 1.0f, 0.5f).setFor(context, true);
    Rectangle<int>(pData->sliderArea.getX(),
                   pData->sliderArea.getY(),
                   pData->sliderArea.getX()+pData->sliderArea.getWidth(),
                   pData->sliderArea.getY()+pData->sliderArea.getHeight()).draw(context);
    Color(1.0f, 1.0f, 1.0f, 1.0f).setFor(context, true);
#endif

    const float normValue = (pData->value - pData->minimum) / (pData->maximum - pData->minimum);

    int x, y;

    if (pData->startPos.getY() == pData->endPos.getY())
    {
        // horizontal
        if (pData->inverted)
            x = pData->endPos.getX() - static_cast<int>(normValue*static_cast<float>(pData->endPos.getX()-pData->startPos.getX()));
        else
            x = pData->startPos.getX() + static_cast<int>(normValue*static_cast<float>(pData->endPos.getX()-pData->startPos.getX()));

        y = pData->startPos.getY();
    }
    else
    {
        // vertical
        x = pData->startPos.getX();

        if (pData->inverted)
            y = pData->endPos.getY() - static_cast<int>(normValue*static_cast<float>(pData->endPos.getY()-pData->startPos.getY()));
        else
            y = pData->startPos.getY() + static_cast<int>(normValue*static_cast<float>(pData->endPos.getY()-pData->startPos.getY()));
    }

    pData->image.drawAt(context, x, y);
}

template <class ImageType>
bool ImageBaseSlider<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! pData->sliderArea.contains(ev.pos))
            return false;

        if ((ev.mod & kModifierShift) != 0 && pData->usingDefault)
        {
            setValue(pData->valueDef, true);
            pData->valueTmp = pData->value;
            return true;
        }

        if (pData->checkable)
        {
            const float value = d_isEqual(pData->valueTmp, pData->minimum) ? pData->maximum : pData->minimum;
            setValue(value, true);
            pData->valueTmp = pData->value;
            return true;
        }

        float vper;
        const double x = ev.pos.getX();
        const double y = ev.pos.getY();

        if (pData->startPos.getY() == pData->endPos.getY())
        {
            // horizontal
            vper = float(x - pData->sliderArea.getX()) / float(pData->sliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - pData->sliderArea.getY()) / float(pData->sliderArea.getHeight());
        }

        float value;

        if (pData->inverted)
            value = pData->maximum - vper * (pData->maximum - pData->minimum);
        else
            value = pData->minimum + vper * (pData->maximum - pData->minimum);

        if (value < pData->minimum)
        {
            pData->valueTmp = value = pData->minimum;
        }
        else if (value > pData->maximum)
        {
            pData->valueTmp = value = pData->maximum;
        }
        else if (d_isNotZero(pData->step))
        {
            pData->valueTmp = value;
            const float rest = std::fmod(value, pData->step);
            value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
        }

        pData->dragging = true;
        pData->startedX = x;
        pData->startedY = y;

        if (pData->callback != nullptr)
            pData->callback->imageSliderDragStarted(this);

        setValue(value, true);

        return true;
    }
    else if (pData->dragging)
    {
        if (pData->callback != nullptr)
            pData->callback->imageSliderDragFinished(this);

        pData->dragging = false;
        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseSlider<ImageType>::onMotion(const MotionEvent& ev)
{
    if (! pData->dragging)
        return false;

    const bool horizontal = pData->startPos.getY() == pData->endPos.getY();
    const double x = ev.pos.getX();
    const double y = ev.pos.getY();

    if ((horizontal && pData->sliderArea.containsX(x)) || (pData->sliderArea.containsY(y) && ! horizontal))
    {
        float vper;

        if (horizontal)
        {
            // horizontal
            vper = float(x - pData->sliderArea.getX()) / float(pData->sliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - pData->sliderArea.getY()) / float(pData->sliderArea.getHeight());
        }

        float value;

        if (pData->inverted)
            value = pData->maximum - vper * (pData->maximum - pData->minimum);
        else
            value = pData->minimum + vper * (pData->maximum - pData->minimum);

        if (value < pData->minimum)
        {
            pData->valueTmp = value = pData->minimum;
        }
        else if (value > pData->maximum)
        {
            pData->valueTmp = value = pData->maximum;
        }
        else if (d_isNotZero(pData->step))
        {
            pData->valueTmp = value;
            const float rest = std::fmod(value, pData->step);
            value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
        }

        setValue(value, true);
    }
    else if (horizontal)
    {
        if (x < pData->sliderArea.getX())
            setValue(pData->inverted ? pData->maximum : pData->minimum, true);
        else
            setValue(pData->inverted ? pData->minimum : pData->maximum, true);
    }
    else
    {
        if (y < pData->sliderArea.getY())
            setValue(pData->inverted ? pData->maximum : pData->minimum, true);
        else
            setValue(pData->inverted ? pData->minimum : pData->maximum, true);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseSwitch<ImageType>::PrivateData {
    ImageType imageNormal;
    ImageType imageDown;
    bool isDown;
    Callback* callback;

    PrivateData(const ImageType& normal, const ImageType& down)
        : imageNormal(normal),
          imageDown(down),
          isDown(false),
          callback(nullptr)
    {
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    PrivateData(PrivateData* const other)
        : imageNormal(other->imageNormal),
          imageDown(other->imageDown),
          isDown(other->isDown),
          callback(other->callback)
    {
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    void assignFrom(PrivateData* const other)
    {
        imageNormal = other->imageNormal;
        imageDown   = other->imageDown;
        isDown      = other->isDown;
        callback    = other->callback;
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageDown) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(imageNormal, imageDown))
{
    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(const ImageBaseSwitch<ImageType>& imageSwitch) noexcept
    : SubWidget(imageSwitch.getParentWidget()),
      pData(new PrivateData(imageSwitch.pData))
{
    setSize(pData->imageNormal.getSize());
}

template <class ImageType>
ImageBaseSwitch<ImageType>& ImageBaseSwitch<ImageType>::operator=(const ImageBaseSwitch<ImageType>& imageSwitch) noexcept
{
    pData->assignFrom(imageSwitch.pData);
    setSize(pData->imageNormal.getSize());
    return *this;
}

template <class ImageType>
ImageBaseSwitch<ImageType>::~ImageBaseSwitch()
{
    delete pData;
}

template <class ImageType>
bool ImageBaseSwitch<ImageType>::isDown() const noexcept
{
    return pData->isDown;
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::setDown(const bool down) noexcept
{
    if (pData->isDown == down)
        return;

    pData->isDown = down;
    repaint();
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::setCallback(Callback* const callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

    if (pData->isDown)
        pData->imageDown.draw(context);
    else
        pData->imageNormal.draw(context);
}

template <class ImageType>
bool ImageBaseSwitch<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.press && contains(ev.pos))
    {
        pData->isDown = !pData->isDown;

        repaint();

        if (pData->callback != nullptr)
            pData->callback->imageSwitchClicked(this, pData->isDown);

        return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
