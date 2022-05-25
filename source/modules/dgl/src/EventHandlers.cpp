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

#include "../EventHandlers.hpp"
#include "../SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct ButtonEventHandler::PrivateData {
    ButtonEventHandler* const self;
    SubWidget* const widget;
    ButtonEventHandler::Callback* internalCallback;
    ButtonEventHandler::Callback* userCallback;

    int button;
    int state;
    bool checkable;
    bool checked;

    Point<double> lastClickPos;
    Point<double> lastMotionPos;

    PrivateData(ButtonEventHandler* const s, SubWidget* const w)
        : self(s),
          widget(w),
          internalCallback(nullptr),
          userCallback(nullptr),
          button(-1),
          state(kButtonStateDefault),
          checkable(false),
          checked(false),
          lastClickPos(0, 0),
          lastMotionPos(0, 0)  {}

    bool mouseEvent(const Widget::MouseEvent& ev)
    {
        lastClickPos = ev.pos;

        // button was released, handle it now
        if (button != -1 && ! ev.press)
        {
            DISTRHO_SAFE_ASSERT(state & kButtonStateActive);

            // release button
            const int button2 = button;
            button = -1;

            const int state2 = state;
            state &= ~kButtonStateActive;

            self->stateChanged(static_cast<State>(state), static_cast<State>(state2));
            widget->repaint();

            // cursor was moved outside the button bounds, ignore click
            if (! widget->contains(ev.pos))
                return true;

            // still on bounds, register click
            if (checkable)
                checked = !checked;

            if (internalCallback != nullptr)
                internalCallback->buttonClicked(widget, button2);
            else if (userCallback != nullptr)
                userCallback->buttonClicked(widget, button2);

            return true;
        }

        // button was pressed, wait for release
        if (ev.press && widget->contains(ev.pos))
        {
            const int state2 = state;
            button = static_cast<int>(ev.button);
            state |= kButtonStateActive;
            self->stateChanged(static_cast<State>(state), static_cast<State>(state2));
            widget->repaint();
            return true;
        }

        return false;
    }

    bool motionEvent(const Widget::MotionEvent& ev)
    {
        // keep pressed
        if (button != -1)
        {
            lastMotionPos = ev.pos;
            return true;
        }

        bool ret = false;

        if (widget->contains(ev.pos))
        {
            // check if entering hover
            if ((state & kButtonStateHover) == 0x0)
            {
                const int state2 = state;
                state |= kButtonStateHover;
                ret = widget->contains(lastMotionPos);
                self->stateChanged(static_cast<State>(state), static_cast<State>(state2));
                widget->repaint();
            }
        }
        else
        {
            // check if exiting hover
            if (state & kButtonStateHover)
            {
                const int state2 = state;
                state &= ~kButtonStateHover;
                ret = widget->contains(lastMotionPos);
                self->stateChanged(static_cast<State>(state), static_cast<State>(state2));
                widget->repaint();
            }
        }

        lastMotionPos = ev.pos;
        return ret;
    }

    void setActive(const bool active2, const bool sendCallback) noexcept
    {
        const bool active = state & kButtonStateActive;
        if (active == active2)
            return;

        state |= kButtonStateActive;
        widget->repaint();

        if (sendCallback)
        {
            if (internalCallback != nullptr)
                internalCallback->buttonClicked(widget, -1);
            else if (userCallback != nullptr)
                userCallback->buttonClicked(widget, -1);
        }
    }

    void setChecked(const bool checked2, const bool sendCallback) noexcept
    {
        if (checked == checked2)
            return;

        checked = checked2;
        widget->repaint();

        if (sendCallback)
        {
            if (internalCallback != nullptr)
                internalCallback->buttonClicked(widget, -1);
            else if (userCallback != nullptr)
                userCallback->buttonClicked(widget, -1);
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

ButtonEventHandler::ButtonEventHandler(SubWidget* const self)
    : pData(new PrivateData(this, self)) {}

ButtonEventHandler::~ButtonEventHandler()
{
    delete pData;
}

bool ButtonEventHandler::isActive() noexcept
{
    return pData->state & kButtonStateActive;
}

void ButtonEventHandler::setActive(const bool active, const bool sendCallback) noexcept
{
    pData->setActive(active, sendCallback);
}

bool ButtonEventHandler::isChecked() const noexcept
{
    return pData->checked;
}

void ButtonEventHandler::setChecked(const bool checked, const bool sendCallback) noexcept
{
    pData->setChecked(checked, sendCallback);
}

bool ButtonEventHandler::isCheckable() const noexcept
{
    return pData->checkable;
}

void ButtonEventHandler::setCheckable(const bool checkable) noexcept
{
    if (pData->checkable == checkable)
        return;

    pData->checkable = checkable;
}

Point<double> ButtonEventHandler::getLastClickPosition() const noexcept
{
    return pData->lastClickPos;
}

Point<double> ButtonEventHandler::getLastMotionPosition() const noexcept
{
    return pData->lastMotionPos;
}

void ButtonEventHandler::setCallback(Callback* const callback) noexcept
{
    pData->userCallback = callback;
}

bool ButtonEventHandler::mouseEvent(const Widget::MouseEvent& ev)
{
    return pData->mouseEvent(ev);
}

bool ButtonEventHandler::motionEvent(const Widget::MotionEvent& ev)
{
    return pData->motionEvent(ev);
}

ButtonEventHandler::State ButtonEventHandler::getState() const noexcept
{
    return static_cast<State>(pData->state);
}

void ButtonEventHandler::clearState() noexcept
{
    pData->state = kButtonStateDefault;
}

void ButtonEventHandler::stateChanged(State, State)
{
}

void ButtonEventHandler::setInternalCallback(Callback* const callback) noexcept
{
    pData->internalCallback = callback;
}

void ButtonEventHandler::triggerUserCallback(SubWidget* const widget, const int button)
{
    if (pData->userCallback != nullptr)
        pData->userCallback->buttonClicked(widget, button);
}

// --------------------------------------------------------------------------------------------------------------------

struct KnobEventHandler::PrivateData {
    KnobEventHandler* const self;
    SubWidget* const widget;
    KnobEventHandler::Callback* callback;

    float minimum;
    float maximum;
    float step;
    float value;
    float valueDef;
    float valueTmp;
    bool usingDefault;
    bool usingLog;
    Orientation orientation;
    int state;

    double lastX;
    double lastY;

    PrivateData(KnobEventHandler* const s, SubWidget* const w)
        : self(s),
          widget(w),
          callback(nullptr),
          minimum(0.0f),
          maximum(1.0f),
          step(0.0f),
          value(0.5f),
          valueDef(value),
          valueTmp(value),
          usingDefault(false),
          usingLog(false),
          orientation(Vertical),
          state(kKnobStateDefault),
          lastX(0.0),
          lastY(0.0) {}

    PrivateData(KnobEventHandler* const s, SubWidget* const w, PrivateData* const other)
        : self(s),
          widget(w),
          callback(other->callback),
          minimum(other->minimum),
          maximum(other->maximum),
          step(other->step),
          value(other->value),
          valueDef(other->valueDef),
          valueTmp(value),
          usingDefault(other->usingDefault),
          usingLog(other->usingLog),
          orientation(other->orientation),
          state(kKnobStateDefault),
          lastX(0.0),
          lastY(0.0) {}

    void assignFrom(PrivateData* const other)
    {
        callback     = other->callback;
        minimum      = other->minimum;
        maximum      = other->maximum;
        step         = other->step;
        value        = other->value;
        valueDef     = other->valueDef;
        valueTmp     = value;
        usingDefault = other->usingDefault;
        usingLog     = other->usingLog;
        orientation  = other->orientation;
        state        = kKnobStateDefault;
        lastX        = 0.0;
        lastY        = 0.0;
    }

    inline float logscale(const float v) const
    {
        const float b = std::log(maximum/minimum)/(maximum-minimum);
        const float a = maximum/std::exp(maximum*b);
        return a * std::exp(b*v);
    }

    inline float invlogscale(const float v) const
    {
        const float b = std::log(maximum/minimum)/(maximum-minimum);
        const float a = maximum/std::exp(maximum*b);
        return std::log(v/a)/b;
    }

    bool mouseEvent(const Widget::MouseEvent& ev)
    {
        if (ev.button != 1)
            return false;

        if (ev.press)
        {
            if (! widget->contains(ev.pos))
                return false;

            if ((ev.mod & kModifierShift) != 0 && usingDefault)
            {
                setValue(valueDef, true);
                valueTmp = value;
                return true;
            }

            state |= kKnobStateDragging;
            lastX = ev.pos.getX();
            lastY = ev.pos.getY();
            widget->repaint();

            if (callback != nullptr)
                callback->knobDragStarted(widget);

            return true;
        }
        else if (state & kKnobStateDragging)
        {
            state &= ~kKnobStateDragging;
            widget->repaint();

            if (callback != nullptr)
                callback->knobDragFinished(widget);

            return true;
        }

        return false;
    }

    bool motionEvent(const Widget::MotionEvent& ev)
    {
        if ((state & kKnobStateDragging) == 0x0)
            return false;

        bool doVal = false;
        float d, value2 = 0.0f;

        if (orientation == Horizontal)
        {
            if (const double movX = ev.pos.getX() - lastX)
            {
                d      = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
                value2 = (usingLog ? invlogscale(valueTmp) : valueTmp) + (float(maximum - minimum) / d * float(movX));
                doVal  = true;
            }
        }
        else if (orientation == Vertical)
        {
            if (const double movY = lastY - ev.pos.getY())
            {
                d      = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
                value2 = (usingLog ? invlogscale(valueTmp) : valueTmp) + (float(maximum - minimum) / d * float(movY));
                doVal  = true;
            }
        }

        if (! doVal)
            return false;

        if (usingLog)
            value2 = logscale(value2);

        if (value2 < minimum)
        {
            valueTmp = value2 = minimum;
        }
        else if (value2 > maximum)
        {
            valueTmp = value2 = maximum;
        }
        else
        {
            valueTmp = value2;

            if (d_isNotZero(step))
            {
                const float rest = std::fmod(value2, step);
                value2 -= rest + (rest > step/2.0f ? step : 0.0f);
            }
        }

        setValue(value2, true);

        lastX = ev.pos.getX();
        lastY = ev.pos.getY();

        return true;
    }

    bool scrollEvent(const Widget::ScrollEvent& ev)
    {
        if (! widget->contains(ev.pos))
            return false;

        const float dir    = (ev.delta.getY() > 0.f) ? 1.f : -1.f;
        const float d      = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
        float       value2 = (usingLog ? invlogscale(valueTmp) : valueTmp)
                           + ((maximum - minimum) / d * 10.f * dir);

        if (usingLog)
            value2 = logscale(value2);

        if (value2 < minimum)
        {
            valueTmp = value2 = minimum;
        }
        else if (value2 > maximum)
        {
            valueTmp = value2 = maximum;
        }
        else
        {
            valueTmp = value2;

            if (d_isNotZero(step))
            {
                const float rest = std::fmod(value2, step);
                value2 = value2 - rest + (rest > step/2.0f ? step : 0.0f);
            }
        }

        setValue(value2, true);
        return true;
    }

    float getNormalizedValue() const noexcept
    {
        const float diff = maximum - minimum;
        return ((usingLog ? invlogscale(value) : value) - minimum) / diff;
    }

    void setRange(const float min, const float max) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(max > min,);

        if (value < min)
        {
            valueTmp = value = min;
            widget->repaint();
        }
        else if (value > max)
        {
            valueTmp = value = max;
            widget->repaint();
        }

        minimum = min;
        maximum = max;
    }

    bool setValue(const float value2, const bool sendCallback)
    {
        if (d_isEqual(value, value2))
            return false;

        valueTmp = value = value2;
        widget->repaint();

        if (sendCallback && callback != nullptr)
        {
            try {
                callback->knobValueChanged(widget, value);
            } DISTRHO_SAFE_EXCEPTION("KnobEventHandler::setValue");
        }

        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

KnobEventHandler::KnobEventHandler(SubWidget* const self)
    : pData(new PrivateData(this, self)) {}

KnobEventHandler::KnobEventHandler(SubWidget* const self, const KnobEventHandler& other)
    : pData(new PrivateData(this, self, other.pData)) {}

KnobEventHandler& KnobEventHandler::operator=(const KnobEventHandler& other)
{
    pData->assignFrom(other.pData);
    return *this;
}

KnobEventHandler::~KnobEventHandler()
{
    delete pData;
}

float KnobEventHandler::getValue() const noexcept
{
    return pData->value;
}

bool KnobEventHandler::setValue(const float value, const bool sendCallback) noexcept
{
    return pData->setValue(value, sendCallback);
}

float KnobEventHandler::getNormalizedValue() const noexcept
{
    return pData->getNormalizedValue();
}

void KnobEventHandler::setDefault(const float def) noexcept
{
    pData->valueDef = def;
    pData->usingDefault = true;
}

void KnobEventHandler::setRange(const float min, const float max) noexcept
{
    pData->setRange(min, max);
}

void KnobEventHandler::setStep(const float step) noexcept
{
    pData->step = step;
}

void KnobEventHandler::setUsingLogScale(const bool yesNo) noexcept
{
    pData->usingLog = yesNo;
}

KnobEventHandler::Orientation KnobEventHandler::getOrientation() const noexcept
{
    return pData->orientation;
}

void KnobEventHandler::setOrientation(const Orientation orientation) noexcept
{
    if (pData->orientation == orientation)
        return;

    pData->orientation = orientation;
}

void KnobEventHandler::setCallback(Callback* const callback) noexcept
{
    pData->callback = callback;
}

bool KnobEventHandler::mouseEvent(const Widget::MouseEvent& ev)
{
    return pData->mouseEvent(ev);
}

bool KnobEventHandler::motionEvent(const Widget::MotionEvent& ev)
{
    return pData->motionEvent(ev);
}

bool KnobEventHandler::scrollEvent(const Widget::ScrollEvent& ev)
{
    return pData->scrollEvent(ev);
}

KnobEventHandler::State KnobEventHandler::getState() const noexcept
{
    return static_cast<State>(pData->state);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
