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

#ifndef DGL_WIDGET_HPP_INCLUDED
#define DGL_WIDGET_HPP_INCLUDED

#include "Geometry.hpp"

#include <vector>

// -----------------------------------------------------------------------
// Forward class names

START_NAMESPACE_DISTRHO
class UI;
END_NAMESPACE_DISTRHO

START_NAMESPACE_DGL

class Application;
class ImageSlider;
class NanoWidget;
class Window;
class StandaloneWindow;

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
      @a mod  The currently active keyboard modifiers, @see Modifier.
      @a time The timestamp (if any).
    */
    struct BaseEvent {
        uint     mod;
        uint32_t time;

        /** Constuctor */
        BaseEvent() noexcept : mod(0x0), time(0) {}
        /** Destuctor */
        virtual ~BaseEvent() noexcept {}
    };

   /**
      Keyboard event.
      @a press True if the key was pressed, false if released.
      @a key   Unicode point of the key pressed.
      @see onKeyboard
    */
    struct KeyboardEvent : BaseEvent {
        bool press;
        uint key;

        /** Constuctor */
        KeyboardEvent() noexcept
            : BaseEvent(),
              press(false),
              key(0) {}
    };

   /**
      Special keyboard event.
      @a press True if the key was pressed, false if released.
      @a key   The key pressed.
      @see onSpecial
    */
    struct SpecialEvent : BaseEvent {
        bool press;
        Key  key;

        /** Constuctor */
        SpecialEvent() noexcept
            : BaseEvent(),
              press(false),
              key(Key(0)) {}
    };

   /**
      Mouse event.
      @a button The button number (1 = left, 2 = middle, 3 = right).
      @a press  True if the key was pressed, false if released.
      @a pos    The widget-relative coordinates of the pointer.
      @see onMouse
    */
    struct MouseEvent : BaseEvent {
        int  button;
        bool press;
        Point<int> pos;

        /** Constuctor */
        MouseEvent() noexcept
            : BaseEvent(),
              button(0),
              press(false),
              pos(0, 0) {}
    };

   /**
      Mouse motion event.
      @a pos The widget-relative coordinates of the pointer.
      @see onMotion
    */
    struct MotionEvent : BaseEvent {
        Point<int> pos;

        /** Constuctor */
        MotionEvent() noexcept
            : BaseEvent(),
              pos(0, 0) {}
    };

   /**
      Mouse scroll event.
      @a pos   The widget-relative coordinates of the pointer.
      @a delta The scroll distance.
      @see onScroll
    */
    struct ScrollEvent : BaseEvent {
        Point<int> pos;
        Point<float> delta;

        /** Constuctor */
        ScrollEvent() noexcept
            : BaseEvent(),
              pos(0, 0),
              delta(0.0f, 0.0f) {}
    };

   /**
      Resize event.
      @a size    The new widget size.
      @a oldSize The previous size, may be null.
      @see onResize
    */
    struct ResizeEvent {
        Size<uint> size;
        Size<uint> oldSize;

        /** Constuctor */
        ResizeEvent() noexcept
            : size(0, 0),
              oldSize(0, 0) {}
    };

   /**
      Constructor.
    */
    explicit Widget(Window& parent);

   /**
      Constructor for a subwidget.
    */
    explicit Widget(Widget* groupWidget);

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
    uint getWidth() const noexcept;

   /**
      Get height.
    */
    uint getHeight() const noexcept;

   /**
      Get size.
    */
    const Size<uint>& getSize() const noexcept;

   /**
      Set width.
    */
    void setWidth(uint width) noexcept;

   /**
      Set height.
    */
    void setHeight(uint height) noexcept;

   /**
      Set size using @a width and @a height values.
    */
    void setSize(uint width, uint height) noexcept;

   /**
      Set size.
    */
    void setSize(const Size<uint>& size) noexcept;

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
    Application& getParentApp() const noexcept;

   /**
      Get parent window, as passed in the constructor.
    */
    Window& getParentWindow() const noexcept;

   /**
      Check if this widget contains the point defined by @a x and @a y.
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

   /**
      Get the Id associated with this widget.
      @see setId
    */
    uint getId() const noexcept;

   /**
      Set an Id to be associated with this widget.
      @see getId
    */
    void setId(uint id) noexcept;

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

private:
    struct PrivateData;
    PrivateData* const pData;

   /** @internal */
    explicit Widget(Widget* groupWidget, bool addToSubWidgets);

    friend class ImageSlider;
    friend class NanoWidget;
    friend class Window;
    friend class StandaloneWindow;
    friend class DISTRHO_NAMESPACE::UI;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Widget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_HPP_INCLUDED
