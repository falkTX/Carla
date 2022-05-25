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

#ifndef DGL_EVENT_HANDLERS_HPP_INCLUDED
#define DGL_EVENT_HANDLERS_HPP_INCLUDED

#include "Widget.hpp"

START_NAMESPACE_DGL

/* NOTE none of these classes get assigned to a widget automatically
   Manual plugging into Widget events is needed, like so:

    ```
    bool onMouse(const MouseEvent& ev) override
    {
        return ButtonEventHandler::mouseEvent(ev);
    }
    ```
*/

// --------------------------------------------------------------------------------------------------------------------

class ButtonEventHandler
{
public:
    enum State {
        kButtonStateDefault = 0x0,
        kButtonStateHover = 0x1,
        kButtonStateActive = 0x2,
        kButtonStateActiveHover = kButtonStateActive|kButtonStateHover
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void buttonClicked(SubWidget* widget, int button) = 0;
    };

    explicit ButtonEventHandler(SubWidget* self);
    virtual ~ButtonEventHandler();

    bool isActive() noexcept;
    void setActive(bool active, bool sendCallback) noexcept;

    bool isChecked() const noexcept;
    void setChecked(bool checked, bool sendCallback) noexcept;

    bool isCheckable() const noexcept;
    void setCheckable(bool checkable) noexcept;

    Point<double> getLastClickPosition() const noexcept;
    Point<double> getLastMotionPosition() const noexcept;

    void setCallback(Callback* callback) noexcept;

    bool mouseEvent(const Widget::MouseEvent& ev);
    bool motionEvent(const Widget::MotionEvent& ev);

protected:
    State getState() const noexcept;
    void clearState() noexcept;

    virtual void stateChanged(State state, State oldState);

    void setInternalCallback(Callback* callback) noexcept;
    void triggerUserCallback(SubWidget* widget, int button);

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonEventHandler)
};

// --------------------------------------------------------------------------------------------------------------------

class KnobEventHandler
{
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    // NOTE hover not implemented yet
    enum State {
        kKnobStateDefault = 0x0,
        kKnobStateHover = 0x1,
        kKnobStateDragging = 0x2,
        kKnobStateDraggingHover = kKnobStateDragging|kKnobStateHover
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void knobDragStarted(SubWidget* widget) = 0;
        virtual void knobDragFinished(SubWidget* widget) = 0;
        virtual void knobValueChanged(SubWidget* widget, float value) = 0;
    };

    explicit KnobEventHandler(SubWidget* self);
    explicit KnobEventHandler(SubWidget* self, const KnobEventHandler& other);
    KnobEventHandler& operator=(const KnobEventHandler& other);
    virtual ~KnobEventHandler();

    // returns raw value, is assumed to be scaled if using log
    float getValue() const noexcept;

    // NOTE: value is assumed to be scaled if using log
    virtual bool setValue(float value, bool sendCallback = false) noexcept;

    // returns 0-1 ranged value, already with log scale as needed
    float getNormalizedValue() const noexcept;

    // NOTE: value is assumed to be scaled if using log
    void setDefault(float def) noexcept;

    // NOTE: value is assumed to be scaled if using log
    void setRange(float min, float max) noexcept;

    void setStep(float step) noexcept;

    void setUsingLogScale(bool yesNo) noexcept;

    Orientation getOrientation() const noexcept;
    void setOrientation(const Orientation orientation) noexcept;

    void setCallback(Callback* callback) noexcept;

    bool mouseEvent(const Widget::MouseEvent& ev);
    bool motionEvent(const Widget::MotionEvent& ev);
    bool scrollEvent(const Widget::ScrollEvent& ev);

protected:
     State getState() const noexcept;

private:
    struct PrivateData;
    PrivateData* const pData;

    /* not for use */
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
    KnobEventHandler(KnobEventHandler& other) = delete;
    KnobEventHandler(const KnobEventHandler& other) = delete;
#else
    KnobEventHandler(KnobEventHandler& other);
    KnobEventHandler(const KnobEventHandler& other);
#endif

    DISTRHO_LEAK_DETECTOR(KnobEventHandler)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_EVENT_HANDLERS_HPP_INCLUDED

