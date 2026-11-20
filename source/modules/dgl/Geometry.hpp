/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_GEOMETRY_HPP_INCLUDED
#define DGL_GEOMETRY_HPP_INCLUDED

#include "Base.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Forward class names

template<typename> class Line;
template<typename> class Circle;
template<typename> class Triangle;
template<typename> class Rectangle;

// --------------------------------------------------------------------------------------------------------------------

/**
   DGL Point class.

   This class describes a single point in space, defined by an X and Y value.
 */
template<typename T>
class Point
{
public:
   /**
      Constructor for (0, 0) point.
    */
    Point() noexcept;

   /**
      Constructor using custom X and Y values.
    */
    Point(const T& x, const T& y) noexcept;

   /**
      Constructor using another Point class values.
    */
    Point(const Point<T>& pos) noexcept;

   /**
      Get X value.
    */
    const T& getX() const noexcept;

   /**
      Get Y value.
    */
    const T& getY() const noexcept;

   /**
      Set X value to @a x.
    */
    void setX(const T& x) noexcept;

   /**
      Set Y value to @a y.
    */
    void setY(const T& y) noexcept;

   /**
      Set X and Y values to @a x and @a y respectively.
    */
    void setPos(const T& x, const T& y) noexcept;

   /**
      Set X and Y values according to @a pos.
    */
    void setPos(const Point<T>& pos) noexcept;

   /**
      Move this point by @a x and @a y values.
    */
    void moveBy(const T& x, const T& y) noexcept;

   /**
      Move this point by @a pos.
    */
    void moveBy(const Point<T>& pos) noexcept;

   /**
      Return true if point is (0, 0).
    */
    bool isZero() const noexcept;

   /**
      Return true if point is not (0, 0).
    */
    bool isNotZero() const noexcept;

    Point<T> operator+(const Point<T>& pos) noexcept;
    Point<T> operator-(const Point<T>& pos) noexcept;
    Point<T>& operator=(const Point<T>& pos) noexcept;
    Point<T>& operator+=(const Point<T>& pos) noexcept;
    Point<T>& operator-=(const Point<T>& pos) noexcept;
    bool operator==(const Point<T>& pos) const noexcept;
    bool operator!=(const Point<T>& pos) const noexcept;

private:
    T x, y;
    template<typename> friend class Line;
    template<typename> friend class Circle;
    template<typename> friend class Triangle;
    template<typename> friend class Rectangle;
};

// --------------------------------------------------------------------------------------------------------------------

/**
   DGL Size class.

   This class describes a size, defined by a width and height value.
 */
template<typename T>
class Size
{
public:
   /**
      Constructor for null size (0x0).
    */
    Size() noexcept;

   /**
      Constructor using custom width and height values.
    */
    Size(const T& width, const T& height) noexcept;

   /**
      Constructor using another Size class values.
    */
    Size(const Size<T>& size) noexcept;

   /**
      Get width.
    */
    const T& getWidth() const noexcept;

   /**
      Get height.
    */
    const T& getHeight() const noexcept;

   /**
      Set width.
    */
    void setWidth(const T& width) noexcept;

   /**
      Set height.
    */
    void setHeight(const T& height) noexcept;

   /**
      Set size to @a width and @a height.
    */
    void setSize(const T& width, const T& height) noexcept;

   /**
      Set size.
    */
    void setSize(const Size<T>& size) noexcept;

   /**
      Grow size by @a multiplier.
    */
    void growBy(double multiplier) noexcept;

   /**
      Shrink size by @a divider.
    */
    void shrinkBy(double divider) noexcept;

   /**
      Return true if size is null (0x0).
      An null size is also invalid.
    */
    bool isNull() const noexcept;

   /**
      Return true if size is not null (0x0).
      A non-null size is still invalid if its width or height are negative.
    */
    bool isNotNull() const noexcept;

   /**
      Return true if size is valid (width and height are higher than zero).
    */
    bool isValid() const noexcept;

   /**
      Return true if size is invalid (width or height are lower or equal to zero).
      An invalid size might not be null under some circumstances.
    */
    bool isInvalid() const noexcept;

    Size<int> toInt() const noexcept;

    Size<T> operator+(const Size<T>& size) noexcept;
    Size<T> operator-(const Size<T>& size) noexcept;
    Size<T>& operator=(const Size<T>& size) noexcept;
    Size<T>& operator+=(const Size<T>& size) noexcept;
    Size<T>& operator-=(const Size<T>& size) noexcept;
    Size<T>& operator*=(double m) noexcept;
    Size<T>& operator/=(double d) noexcept;
    Size<T> operator*(double m) const noexcept;
    Size<T> operator/(double m) const noexcept;
    bool operator==(const Size<T>& size) const noexcept;
    bool operator!=(const Size<T>& size) const noexcept;

private:
    T fWidth, fHeight;
    template<typename> friend class Rectangle;
};

// -----------------------------------------------------------------------

/**
   DGL Line class.

   This class describes a line, defined by two points.
 */
template<typename T>
class Line
{
public:
   /**
      Constructor for a null line ([0,0] to [0,0]).
    */
    Line() noexcept;

   /**
      Constructor using custom start X, start Y, end X and end Y values.
    */
    Line(const T& startX, const T& startY, const T& endX, const T& endY) noexcept;

   /**
      Constructor using custom start X, start Y and end pos values.
    */
    Line(const T& startX, const T& startY, const Point<T>& endPos) noexcept;

   /**
      Constructor using custom start pos, end X and end Y values.
    */
    Line(const Point<T>& startPos, const T& endX, const T& endY) noexcept;

   /**
      Constructor using custom start and end pos values.
    */
    Line(const Point<T>& startPos, const Point<T>& endPos) noexcept;

   /**
      Constructor using another Line class values.
    */
    Line(const Line<T>& line) noexcept;

   /**
      Get start X value.
    */
    const T& getStartX() const noexcept;

   /**
      Get start Y value.
    */
    const T& getStartY() const noexcept;

   /**
      Get end X value.
    */
    const T& getEndX() const noexcept;

   /**
      Get end Y value.
    */
    const T& getEndY() const noexcept;

   /**
      Get start position.
    */
    const Point<T>& getStartPos() const noexcept;

   /**
      Get end position.
    */
    const Point<T>& getEndPos() const noexcept;

   /**
      Set start X value to @a x.
    */
    void setStartX(const T& x) noexcept;

   /**
      Set start Y value to @a y.
    */
    void setStartY(const T& y) noexcept;

   /**
      Set start X and Y values to @a x and @a y respectively.
    */
    void setStartPos(const T& x, const T& y) noexcept;

   /**
      Set start X and Y values according to @a pos.
    */
    void setStartPos(const Point<T>& pos) noexcept;

   /**
      Set end X value to @a x.
    */
    void setEndX(const T& x) noexcept;

   /**
      Set end Y value to @a y.
    */
    void setEndY(const T& y) noexcept;

   /**
      Set end X and Y values to @a x and @a y respectively.
    */
    void setEndPos(const T& x, const T& y) noexcept;

   /**
      Set end X and Y values according to @a pos.
    */
    void setEndPos(const Point<T>& pos) noexcept;

   /**
      Move this line by @a x and @a y values.
    */
    void moveBy(const T& x, const T& y) noexcept;

   /**
      Move this line by @a pos.
    */
    void moveBy(const Point<T>& pos) noexcept;

   /**
      Return true if line is null (start and end pos are equal).
    */
    bool isNull() const noexcept;

   /**
      Return true if line is not null (start and end pos are different).
    */
    bool isNotNull() const noexcept;

   /**
      Draw this line using the provided graphics context, optionally specifying line width.
    */
    void draw(const GraphicsContext& context, T width = 1);

    Line<T>& operator=(const Line<T>& line) noexcept;
    bool operator==(const Line<T>& line) const noexcept;
    bool operator!=(const Line<T>& line) const noexcept;

   #if DGL_ALLOW_DEPRECATED_METHODS
   /**
      Draw this line using the current OpenGL state.@n
      DEPRECATED Please use draw(const GraphicsContext&) instead.
    */
    DISTRHO_DEPRECATED_BY("draw(const GraphicsContext&)")
    void draw();
   #endif

private:
    Point<T> posStart, posEnd;
};

// -----------------------------------------------------------------------

/**
   DGL Circle class.

   This class describes a circle, defined by position, size and a minimum of 3 segments.

   TODO: report if circle starts at top-left, bottom-right or center.
         and size grows from which point?
 */
template<typename T>
class Circle
{
public:
   /**
      Constructor for a null circle.
    */
    Circle() noexcept;

   /**
      Constructor using custom X, Y and size values.
    */
    Circle(const T& x, const T& y, const float size, const uint numSegments = 300);

   /**
      Constructor using custom position and size values.
    */
    Circle(const Point<T>& pos, const float size, const uint numSegments = 300);

   /**
      Constructor using another Circle class values.
    */
    Circle(const Circle<T>& cir) noexcept;

   /**
      Get X value.
    */
    const T& getX() const noexcept;

   /**
      Get Y value.
    */
    const T& getY() const noexcept;

   /**
      Get position.
    */
    const Point<T>& getPos() const noexcept;

   /**
      Set X value to @a x.
    */
    void setX(const T& x) noexcept;

   /**
      Set Y value to @a y.
    */
    void setY(const T& y) noexcept;

   /**
      Set X and Y values to @a x and @a y respectively.
    */
    void setPos(const T& x, const T& y) noexcept;

   /**
      Set X and Y values according to @a pos.
    */
    void setPos(const Point<T>& pos) noexcept;

   /**
      Get size.
    */
    float getSize() const noexcept;

   /**
      Set size.
      @note Must always be > 0
    */
    void setSize(const float size) noexcept;

   /**
      Get the current number of line segments that make this circle.
    */
    uint getNumSegments() const noexcept;

   /**
      Set the number of line segments that will make this circle.
      @note Must always be >= 3
    */
    void setNumSegments(const uint num);

   /**
      Draw this circle using the provided graphics context.
    */
    void draw(const GraphicsContext& context);

   /**
      Draw lines (outline of this circle) using the provided graphics context, optionally specifying line width.
    */
    void drawOutline(const GraphicsContext& context, T lineWidth = 1);

    Circle<T>& operator=(const Circle<T>& cir) noexcept;
    bool operator==(const Circle<T>& cir) const noexcept;
    bool operator!=(const Circle<T>& cir) const noexcept;

   #if DGL_ALLOW_DEPRECATED_METHODS
   /**
      Draw this circle using the current OpenGL state.@n
      DEPRECATED Please use draw(const GraphicsContext&) instead.
    */
    DISTRHO_DEPRECATED_BY("draw(const GraphicsContext&)")
    void draw();

   /**
      Draw lines (outline of this circle) using the current OpenGL state.@n
      DEPRECATED Please use drawOutline(const GraphicsContext&,T) instead.
    */
    DISTRHO_DEPRECATED_BY("drawOutline(const GraphicsContext&)")
    void drawOutline();
   #endif

private:
    Point<T> fPos;
    float    fSize;
    uint     fNumSegments;

    // cached values
    float fTheta, fCos, fSin;
};

// -----------------------------------------------------------------------

/**
   DGL Triangle class.

   This class describes a triangle, defined by 3 points.
 */
template<typename T>
class Triangle
{
public:
   /**
      Constructor for a null triangle.
    */
    Triangle() noexcept;

   /**
      Constructor using custom X and Y values.
    */
    Triangle(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3) noexcept;

   /**
      Constructor using custom position values.
    */
    Triangle(const Point<T>& pos1, const Point<T>& pos2, const Point<T>& pos3) noexcept;

   /**
      Constructor using another Triangle class values.
    */
    Triangle(const Triangle<T>& tri) noexcept;

   /**
      Return true if triangle is null (all its points are equal).
      An null triangle is also invalid.
    */
    bool isNull() const noexcept;

   /**
      Return true if triangle is not null (one its points is different from the others).
      A non-null triangle is still invalid if two of its points are equal.
    */
    bool isNotNull() const noexcept;

   /**
      Return true if triangle is valid (all its points are different).
    */
    bool isValid() const noexcept;

   /**
      Return true if triangle is invalid (one or two of its points are equal).
      An invalid triangle might not be null under some circumstances.
    */
    bool isInvalid() const noexcept;

   /**
      Draw this triangle using the provided graphics context.
    */
    void draw(const GraphicsContext& context);

   /**
      Draw lines (outline of this triangle) using the provided graphics context, optionally specifying line width.
    */
    void drawOutline(const GraphicsContext& context, T lineWidth = 1);

    Triangle<T>& operator=(const Triangle<T>& tri) noexcept;
    bool operator==(const Triangle<T>& tri) const noexcept;
    bool operator!=(const Triangle<T>& tri) const noexcept;

   #if DGL_ALLOW_DEPRECATED_METHODS
   /**
      Draw this triangle using the current OpenGL state.@n
      DEPRECATED Please use draw(const GraphicsContext&) instead.
    */
    DISTRHO_DEPRECATED_BY("draw(const GraphicsContext&)")
    void draw();

   /**
      Draw lines (outline of this triangle) using the current OpenGL state.@n
      DEPRECATED Please use drawOutline(const GraphicsContext&,T) instead.
    */
    DISTRHO_DEPRECATED_BY("drawOutline(const GraphicsContext&)")
    void drawOutline();
   #endif

private:
    Point<T> pos1, pos2, pos3;
};

// -----------------------------------------------------------------------

/**
   DGL Rectangle class.

   This class describes a rectangle, defined by a starting point and a size.
 */
template<typename T>
class Rectangle
{
public:
   /**
      Constructor for a null rectangle.
    */
    Rectangle() noexcept;

   /**
      Constructor using custom X, Y, width and height values.
    */
    Rectangle(const T& x, const T& y, const T& width, const T& height) noexcept;

   /**
      Constructor using custom X, Y and size values.
    */
    Rectangle(const T& x, const T& y, const Size<T>& size) noexcept;

   /**
      Constructor using custom pos, width and height values.
    */
    Rectangle(const Point<T>& pos, const T& width, const T& height) noexcept;

   /**
      Constructor using custom position and size.
    */
    Rectangle(const Point<T>& pos, const Size<T>& size) noexcept;

   /**
      Constructor using another Rectangle class values.
    */
    Rectangle(const Rectangle<T>& rect) noexcept;

   /**
      Get X value.
    */
    const T& getX() const noexcept;

   /**
      Get Y value.
    */
    const T& getY() const noexcept;

   /**
      Get width.
    */
    const T& getWidth() const noexcept;

   /**
      Get height.
    */
    const T& getHeight() const noexcept;

   /**
      Get position.
    */
    const Point<T>& getPos() const noexcept;

   /**
      Get size.
    */
    const Size<T>& getSize() const noexcept;

   /**
      Set X value as @a x.
    */
    void setX(const T& x) noexcept;

   /**
      Set Y value as @a y.
    */
    void setY(const T& y) noexcept;

   /**
      Set X and Y values as @a x and @a y respectively.
    */
    void setPos(const T& x, const T& y) noexcept;

   /**
      Set X and Y values according to @a pos.
    */
    void setPos(const Point<T>& pos) noexcept;

   /**
      Move this rectangle by @a x and @a y values.
    */
    void moveBy(const T& x, const T& y) noexcept;

   /**
      Move this rectangle by @a pos.
    */
    void moveBy(const Point<T>& pos) noexcept;

   /**
      Set width.
    */
    void setWidth(const T& width) noexcept;

   /**
      Set height.
    */
    void setHeight(const T& height) noexcept;

   /**
      Set size using @a width and @a height.
    */
    void setSize(const T& width, const T& height) noexcept;

   /**
      Set size.
    */
    void setSize(const Size<T>& size) noexcept;

   /**
      Grow size by @a multiplier.
    */
    void growBy(double multiplier) noexcept;

   /**
      Shrink size by @a divider.
    */
    void shrinkBy(double divider) noexcept;

   /**
      Set rectangle using @a pos and @a size.
    */
    void setRectangle(const Point<T>& pos, const Size<T>& size) noexcept;

   /**
      Set rectangle.
    */
    void setRectangle(const Rectangle<T>& rect) noexcept;

   /**
      Check if this rectangle contains the point defined by @a X and @a Y.
    */
    bool contains(const T& x, const T& y) const noexcept;

   /**
      Check if this rectangle contains the point @a pos.
    */
    bool contains(const Point<T>& pos) const noexcept;

   /**
      Check if this rectangle contains the point @a pos affected by a custom scale.
    */
    bool containsAfterScaling(const Point<T>& pos, double scaling) const noexcept;

   /**
      Check if this rectangle contains the point @a pos of another type.
    */
    template<typename T2>
    bool contains(const Point<T2>& pos) const noexcept;

   /**
      Check if this rectangle contains X.
    */
    bool containsX(const T& x) const noexcept;

   /**
      Check if this rectangle contains Y.
    */
    bool containsY(const T& y) const noexcept;

   /**
      Return true if size is null (0x0).
      An null size is also invalid.
    */
    bool isNull() const noexcept;

   /**
      Return true if size is not null (0x0).
      A non-null size is still invalid if its width or height are negative.
    */
    bool isNotNull() const noexcept;

   /**
      Return true if size is valid (width and height are higher than zero).
    */
    bool isValid() const noexcept;

   /**
      Return true if size is invalid (width or height are lower or equal to zero).
      An invalid size might not be null under some circumstances.
    */
    bool isInvalid() const noexcept;

   /**
      Draw this rectangle using the provided graphics context.
    */
    void draw(const GraphicsContext& context);

   /**
      Draw lines (outline of this rectangle) using the provided graphics context, optionally specifying line width.
    */
    void drawOutline(const GraphicsContext& context, T lineWidth = 1);

    Rectangle<T>& operator=(const Rectangle<T>& rect) noexcept;
    Rectangle<T>& operator*=(double m) noexcept;
    Rectangle<T>& operator/=(double d) noexcept;
    bool operator==(const Rectangle<T>& size) const noexcept;
    bool operator!=(const Rectangle<T>& size) const noexcept;

   #if DGL_ALLOW_DEPRECATED_METHODS
   /**
      Draw this rectangle using the current OpenGL state.@n
      DEPRECATED Please use draw(const GraphicsContext&) instead.
    */
    DISTRHO_DEPRECATED_BY("draw(const GraphicsContext&)")
    void draw();

   /**
      Draw lines (outline of this rectangle) using the current OpenGL state.@n
      DEPRECATED Please use drawOutline(const GraphicsContext&,T) instead.
    */
    DISTRHO_DEPRECATED_BY("drawOutline(const GraphicsContext&)")
    void drawOutline();
   #endif

private:
    Point<T> pos;
    Size<T> size;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_GEOMETRY_HPP_INCLUDED
