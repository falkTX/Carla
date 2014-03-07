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

#ifndef DGL_GEOMETRY_HPP_INCLUDED
#define DGL_GEOMETRY_HPP_INCLUDED

#include "Base.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

template<typename T>
class Point
{
public:
    Point() noexcept;
    Point(T x, T y) noexcept;
    Point(const Point<T>& pos) noexcept;

    T getX() const noexcept;
    T getY() const noexcept;

    void setX(T x) noexcept;
    void setY(T y) noexcept;

    void move(T x, T y) noexcept;
    void move(const Point<T>& pos) noexcept;

    Point<T>& operator=(const Point<T>& pos) noexcept;
    Point<T>& operator+=(const Point<T>& pos) noexcept;
    Point<T>& operator-=(const Point<T>& pos) noexcept;
    bool operator==(const Point<T>& pos) const noexcept;
    bool operator!=(const Point<T>& pos) const noexcept;

private:
    T fX, fY;
    template<typename> friend class Rectangle;
};

// -----------------------------------------------------------------------

template<typename T>
class Size
{
public:
    Size() noexcept;
    Size(T width, T height) noexcept;
    Size(const Size<T>& size) noexcept;

    T getWidth() const noexcept;
    T getHeight() const noexcept;

    void setWidth(T width) noexcept;
    void setHeight(T height) noexcept;

    Size<T>& operator=(const Size<T>& size) noexcept;
    Size<T>& operator+=(const Size<T>& size) noexcept;
    Size<T>& operator-=(const Size<T>& size) noexcept;
    Size<T>& operator*=(T m) noexcept;
    Size<T>& operator/=(T d) noexcept;
    bool operator==(const Size<T>& size) const noexcept;
    bool operator!=(const Size<T>& size) const noexcept;

private:
    T fWidth, fHeight;
    template<typename> friend class Rectangle;
};

// -----------------------------------------------------------------------

template<typename T>
class Rectangle
{
public:
    Rectangle() noexcept;
    Rectangle(T x, T y, T width, T height) noexcept;
    Rectangle(T x, T y, const Size<T>& size) noexcept;
    Rectangle(const Point<T>& pos, T width, T height) noexcept;
    Rectangle(const Point<T>& pos, const Size<T>& size) noexcept;
    Rectangle(const Rectangle<T>& rect) noexcept;

    T getX() const noexcept;
    T getY() const noexcept;
    T getWidth() const noexcept;
    T getHeight() const noexcept;

    const Point<T>& getPos() const noexcept;
    const Size<T>&  getSize() const noexcept;

    bool contains(T x, T y) const noexcept;
    bool contains(const Point<T>& pos) const noexcept;
    bool containsX(T x) const noexcept;
    bool containsY(T y) const noexcept;

    void setX(T x) noexcept;
    void setY(T y) noexcept;
    void setPos(T x, T y) noexcept;
    void setPos(const Point<T>& pos) noexcept;

    void move(T x, T y) noexcept;
    void move(const Point<T>& pos) noexcept;

    void setWidth(T width) noexcept;
    void setHeight(T height) noexcept;
    void setSize(T width, T height) noexcept;
    void setSize(const Size<T>& size) noexcept;

    Rectangle<T>& operator=(const Rectangle<T>& rect) noexcept;

private:
    Point<T> fPos;
    Size<T>  fSize;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_GEOMETRY_HPP_INCLUDED
