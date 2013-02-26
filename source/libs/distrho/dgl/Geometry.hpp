/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DGL_GEOMETRY_HPP__
#define __DGL_GEOMETRY_HPP__

#include "Base.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

template<typename T>
class Point
{
public:
    Point();
    Point(T x, T y);
    Point(const Point<T>& pos);

    T getX() const;
    T getY() const;

    void setX(T x);
    void setY(T y);

    void move(T x, T y);
    void move(const Point<T>& pos);

    Point<T>& operator=(const Point<T>& pos);
    Point<T>& operator+=(const Point<T>& pos);
    Point<T>& operator-=(const Point<T>& pos);
    bool operator==(const Point<T>& pos) const;
    bool operator!=(const Point<T>& pos) const;

private:
    T fX, fY;
    template<typename> friend class Rectangle;
};

// -------------------------------------------------

template<typename T>
class Size
{
public:
    Size();
    Size(T width, T height);
    Size(const Size<T>& size);

    T getWidth() const;
    T getHeight() const;

    void setWidth(T width);
    void setHeight(T height);

    Size<T>& operator=(const Size<T>& size);
    Size<T>& operator+=(const Size<T>& size);
    Size<T>& operator-=(const Size<T>& size);
    Size<T>& operator*=(T m);
    Size<T>& operator/=(T d);
    bool operator==(const Size<T>& size) const;
    bool operator!=(const Size<T>& size) const;

private:
    T fWidth, fHeight;
    template<typename> friend class Rectangle;
};

// -------------------------------------------------

template<typename T>
class Rectangle
{
public:
    Rectangle();
    Rectangle(T x, T y, T width, T height);
    Rectangle(T x, T y, const Size<T>& size);
    Rectangle(const Point<T>& pos, T width, T height);
    Rectangle(const Point<T>& pos, const Size<T>& size);
    Rectangle(const Rectangle<T>& rect);

    T getX() const;
    T getY() const;
    T getWidth() const;
    T getHeight() const;

    const Point<T>& getPos() const;
    const Size<T>&  getSize() const;

    bool contains(T x, T y) const;
    bool contains(const Point<T>& pos) const;
    bool containsX(T x) const;
    bool containsY(T y) const;

    void setX(T x);
    void setY(T y);
    void setPos(T x, T y);
    void setPos(const Point<T>& pos);

    void move(T x, T y);
    void move(const Point<T>& pos);

    void setWidth(T width);
    void setHeight(T height);
    void setSize(T width, T height);
    void setSize(const Size<T>& size);

    Rectangle<T>& operator=(const Rectangle<T>& rect);

private:
    Point<T> fPos;
    Size<T>  fSize;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_GEOMETRY_HPP__
