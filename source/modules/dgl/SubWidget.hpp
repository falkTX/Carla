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

#ifndef DGL_SUBWIDGET_HPP_INCLUDED
#define DGL_SUBWIDGET_HPP_INCLUDED

#include "Widget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Sub-Widget class.

   This class is the main entry point for creating any reusable widgets from within DGL.
   It can be freely positioned from within a parent widget, thus being named subwidget.

   Many subwidgets can share the same parent, and subwidgets themselves can also have its own subwidgets.
   It is subwidgets all the way down.

   TODO check absolute vs relative position and see what makes more sense.

   @see CairoSubWidget
 */
class SubWidget : public Widget
{
public:
   /**
      Constructor.
    */
    explicit SubWidget(Widget* parentWidget);

   /**
      Destructor.
    */
    ~SubWidget() override;

   /**
      Check if this widget contains the point defined by @a x and @a y.
    */
    // TODO rename as containsRelativePos
    template<typename T>
    bool contains(T x, T y) const noexcept;

   /**
      Check if this widget contains the point @a pos.
    */
    // TODO rename as containsRelativePos
    template<typename T>
    bool contains(const Point<T>& pos) const noexcept;

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
    Point<int> getAbsolutePos() const noexcept;

   /**
      Get absolute area of this subwidget.
      This is the same as `Rectangle<int>(getAbsolutePos(), getSize());`
      @see getConstrainedAbsoluteArea()
    */
    Rectangle<int> getAbsoluteArea() const noexcept;

   /**
      Get absolute area of this subwidget, with special consideration for not allowing negative values.
      @see getAbsoluteArea()
    */
    Rectangle<uint> getConstrainedAbsoluteArea() const noexcept;

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
      Get the margin currently in use for widget coordinates.
      By default this value is (0,0).
    */
    Point<int> getMargin() const noexcept;

   /**
      Set a margin to be used for widget coordinates using @a x and @a y values.
    */
    void setMargin(int x, int y) noexcept;

   /**
      Set a margin to be used for widget coordinates.
    */
    void setMargin(const Point<int>& offset) noexcept;

   /**
      Get parent Widget, as passed in the constructor.
    */
    Widget* getParentWidget() const noexcept;

   /**
      Request repaint of this subwidget's area to the window this widget belongs to.
    */
    void repaint() noexcept override;

   /**
      Pushes this widget to the "bottom" of the parent widget.
      Makes the widget behave as if it was the first to be registered on the parent widget, thus being "on bottom".
    */
    virtual void toBottom();

   /**
      Bring this widget to the "front" of the parent widget.
      Makes the widget behave as if it was the last to be registered on the parent widget, thus being "in front".
    */
    virtual void toFront();

   /**
      Indicate that this subwidget will draw out of bounds, and thus needs the entire viewport available for drawing.
    */
    void setNeedsFullViewportDrawing(bool needsFullViewportForDrawing = true);

   /**
      Indicate that this subwidget will always draw at its own internal size and needs scaling to fit target size.
    */
    void setNeedsViewportScaling(bool needsViewportScaling = true, double autoScaleFactor = 0.0);

   /**
      Indicate that this subwidget should not be drawn on screen, typically because it is managed by something else.
    */
    void setSkipDrawing(bool skipDrawing = true);

protected:
   /**
      A function called when the subwidget's absolute position is changed.
    */
    virtual void onPositionChanged(const PositionChangedEvent&);

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Widget;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubWidget)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_SUBWIDGET_HPP_INCLUDED

