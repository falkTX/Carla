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

#ifndef DGL_WIDGET_HPP_INCLUDED
#define DGL_WIDGET_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Forward class names

class App;
class Window;

// -----------------------------------------------------------------------

/**
   Base DGL Widget class.

   This is the base Widget class, from which all widgets are built.

   All widgets have a parent Window where they'll be drawn.
   This parent is never changed during the widget lifetime.

   Widgets receive events in relative coordinates.
   (0, 0) means its top-left position.

   Windows paint widgets in the order they are constructed.
   Early widgets are drawn first, at the bottom, then newer ones on top.
   Events are sent in the inverse order so that the top-most widget gets
   a chance to catch the event and stop its propagation.

   All widget event callbacks do nothing by default.
 */
class Widget
{
public:
   /**
      Base event data.
      @param mod  The currently active modifiers.
      @param time The timestamp (if any) of the currently-processing event.
    */
    struct BaseEvent {
        Modifier mod;
        uint32_t time;
    };

   /**
      Keyboard event.
      @param press True if the key was pressed, false if released.
      @param key   Unicode point of the key pressed.
      @see onKeyboard
    */
    struct KeyboardEvent : BaseEvent {
        bool press;
        uint key;
    };

   /**
      Special keyboard event.
      @param press True if the key was pressed, false if released.
      @param key   The key pressed.
      @see onSpecial
    */
    struct SpecialEvent : BaseEvent {
        bool press;
        Key key;
    };

   /**
      Mouse event.
      @param button The button number (1 = left, 2 = middle, 3 = right).
      @param press  True if the key was pressed, false if released.
      @param pos    The widget-relative coordinates of the pointer.
      @see onMouse
    */
    struct MouseEvent : BaseEvent {
        int button;
        bool press;
        Point<int> pos;
    };

   /**
      Mouse motion event.
      @param pos The widget-relative coordinates of the pointer.
      @see onMotion
    */
    struct MotionEvent : BaseEvent {
        Point<int> pos;
    };

   /**
      Mouse scroll event.
      @param pos   The widget-relative coordinates of the pointer.
      @param delta The scroll distance.
      @see onScroll
    */
    struct ScrollEvent : BaseEvent {
        Point<int> pos;
        Point<float> delta;
    };

   /**
      Resize event.
      @param size    The new widget size.
      @param oldSize The previous size, may be null.
      @see onResize
    */
    struct ResizeEvent {
        Size<int> size;
        Size<int> oldSize;
    };

   /**
      Constructor.
    */
    explicit Widget(Window& parent);

   /**
      Destructor.
    */
    virtual ~Widget();

   /**
      Check if this widget is visible within its parent window.
      Invisible widgets do not receive events except resize.
    */
    bool isVisible() const noexcept;

   /**
      Set widget visible (or not) according to @a yesNo.
    */
    void setVisible(bool yesNo);

   /**
      Show widget.
      This is the same as calling setVisible(true).
    */
    void show();

   /**
      Hide widget.
      This is the same as calling setVisible(false).
    */
    void hide();

   /**
      Get width.
    */
    int getWidth() const noexcept;

   /**
      Get height.
    */
    int getHeight() const noexcept;

   /**
      Get size.
    */
    const Size<int>& getSize() const noexcept;

   /**
      Set width.
    */
    virtual void setWidth(int width) noexcept;

   /**
      Set height.
    */
    virtual void setHeight(int height) noexcept;

   /**
      Set size using @a width and @a height values.
    */
    virtual void setSize(int width, int height) noexcept;

   /**
      Set size.
    */
    virtual void setSize(const Size<int>& size) noexcept;

   /**
      Get absolute X.
    */
    int getAbsoluteX() const noexcept;

   /**
      Get absolute Y.
    */
    int getAbsoluteY() const noexcept;

   /**
      Get absolute position.
    */
    const Point<int>& getAbsolutePos() const noexcept;

   /**
      Set absolute X.
    */
    void setAbsoluteX(int x) noexcept;

   /**
      Set absolute Y.
    */
    void setAbsoluteY(int y) noexcept;

   /**
      Set absolute position using @a x and @a y values.
    */
    void setAbsolutePos(int x, int y) noexcept;

   /**
      Set absolute position.
    */
    void setAbsolutePos(const Point<int>& pos) noexcept;

   /**
      Get this widget's window application.
      Same as calling getParentWindow().getApp().
    */
    App& getParentApp() const noexcept;

   /**
      Get parent window, as passed in the constructor.
    */
    Window& getParentWindow() const noexcept;

   /**
      Check if this widget contains the point defined by @a X and @a Y.
    */
    bool contains(int x, int y) const noexcept;

   /**
      Check if this widget contains the point @a pos.
    */
    bool contains(const Point<int>& pos) const noexcept;

   /**
      Tell this widget's window to repaint itself.
    */
    void repaint() noexcept;

protected:
   /**
      A function called to draw the view contents with OpenGL.
    */
    virtual void onDisplay() = 0;

   /**
      A function called when a key is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onKeyboard(const KeyboardEvent&);

   /**
      A function called when a special key is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onSpecial(const SpecialEvent&);

   /**
      A function called when a mouse button is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onMouse(const MouseEvent&);

   /**
      A function called when the pointer moves.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onMotion(const MotionEvent&);

   /**
      A function called on scrolling (e.g. mouse wheel or track pad).
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onScroll(const ScrollEvent&);

   /**
      A function called when the widget is resized.
    */
    virtual void onResize(const ResizeEvent&);

   /**
      Tell the parent window this widget this the full viewport.
      When enabled, the local widget coordinates are ignored.
      @note: This is an internal function;
             You do not need it under normal circumstances.
    */
    void setNeedsFullViewport(bool yesNo) noexcept;

private:
    Window& fParent;
    bool    fNeedsFullViewport;
    bool    fVisible;
    Rectangle<int> fArea;

    friend class CairoWidget;
    friend class Window;
    friend class StandaloneWindow;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Widget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_HPP_INCLUDED
