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

#include "../Geometry.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Point

template<typename T>
Point<T>::Point()
    : fX(0),
      fY(0)
{
}

template<typename T>
Point<T>::Point(T x, T y)
    : fX(x),
      fY(y)
{
}

template<typename T>
Point<T>::Point(const Point& pos)
    : fX(pos.fX),
      fY(pos.fY)
{
}

template<typename T>
T Point<T>::getX() const
{
    return fX;
}

template<typename T>
T Point<T>::getY() const
{
    return fY;
}

template<typename T>
void Point<T>::setX(T x)
{
    fX = x;
}

template<typename T>
void Point<T>::setY(T y)
{
    fY = y;
}

template<typename T>
void Point<T>::move(T x, T y)
{
    fX += x;
    fY += y;
}

template<typename T>
void Point<T>::move(const Point& pos)
{
    fX += pos.fX;
    fY += pos.fY;
}

template<typename T>
Point<T>& Point<T>::operator=(const Point<T>& pos)
{
    fX = pos.fX;
    fY = pos.fY;
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator+=(const Point<T>& pos)
{
    fX += pos.fX;
    fY += pos.fY;
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator-=(const Point<T>& pos)
{
    fX -= pos.fX;
    fY -= pos.fY;
    return *this;
}

template<typename T>
bool Point<T>::operator==(const Point<T>& pos) const
{
    return (fX == pos.fX && fY== pos.fY);
}

template<typename T>
bool Point<T>::operator!=(const Point<T>& pos) const
{
    return !operator==(pos);
}

// -------------------------------------------------
// Size

template<typename T>
Size<T>::Size()
    : fWidth(0),
      fHeight(0)
{
}

template<typename T>
Size<T>::Size(T width, T height)
    : fWidth(width),
      fHeight(height)
{
}

template<typename T>
Size<T>::Size(const Size<T>& size)
    : fWidth(size.fWidth),
      fHeight(size.fHeight)
{
}

template<typename T>
T Size<T>::getWidth() const
{
    return fWidth;
}

template<typename T>
T Size<T>::getHeight() const
{
    return fHeight;
}

template<typename T>
void Size<T>::setWidth(T width)
{
    fWidth = width;
}

template<typename T>
void Size<T>::setHeight(T height)
{
    fHeight = height;
}

template<typename T>
Size<T>& Size<T>::operator=(const Size<T>& size)
{
    fWidth  = size.fWidth;
    fHeight = size.fHeight;
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator+=(const Size<T>& size)
{
    fWidth  += size.fWidth;
    fHeight += size.fHeight;
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator-=(const Size<T>& size)
{
    fWidth  -= size.fWidth;
    fHeight -= size.fHeight;
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator*=(T m)
{
    fWidth  *= m;
    fHeight *= m;
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator/=(T d)
{
    fWidth  /= d;
    fHeight /= d;
    return *this;
}

template<typename T>
bool Size<T>::operator==(const Size<T>& size) const
{
    return (fWidth == size.fWidth && fHeight == size.fHeight);
}

template<typename T>
bool Size<T>::operator!=(const Size<T>& size) const
{
    return !operator==(size);
}

// -------------------------------------------------
// Rectangle

template<typename T>
Rectangle<T>::Rectangle()
    : fPos(0, 0),
      fSize(0, 0)
{
}

template<typename T>
Rectangle<T>::Rectangle(T x, T y, T width, T height)
    : fPos(x, y),
      fSize(width, height)
{
}

template<typename T>
Rectangle<T>::Rectangle(T x, T y, const Size<T>& size)
    : fPos(x, y),
      fSize(size)
{
}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& pos, T width, T height)
    : fPos(pos),
      fSize(width, height)
{
}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& pos, const Size<T>& size)
    : fPos(pos),
      fSize(size)
{
}

template<typename T>
Rectangle<T>::Rectangle(const Rectangle<T>& rect)
    : fPos(rect._pos),
      fSize(rect._size)
{
}

template<typename T>
T Rectangle<T>::getX() const
{
    return fPos.fX;
}

template<typename T>
T Rectangle<T>::getY() const
{
    return fPos.fY;
}

template<typename T>
T Rectangle<T>::getWidth() const
{
    return fSize.fWidth;
}

template<typename T>
T Rectangle<T>::getHeight() const
{
    return fSize.fHeight;
}

template<typename T>
const Point<T>& Rectangle<T>::getPos() const
{
    return fPos;
}

template<typename T>
const Size<T>& Rectangle<T>::getSize() const
{
    return fSize;
}

template<typename T>
bool Rectangle<T>::contains(T x, T y) const
{
    return (x >= fPos.fX && y >= fPos.fY && x <= fPos.fX+fSize.fWidth && y <= fPos.fY+fSize.fHeight);
}

template<typename T>
bool Rectangle<T>::contains(const Point<T>& pos) const
{
    return contains(pos.fX, pos.fY);
}

#if 0
template<typename T>
void Rectangle::setX(int x)
{
    _pos._x = x;
}

template<typename T>
void Rectangle::setY(int y)
{
    _pos._y = y;
}

void Rectangle::setPos(int x, int y)
{
    _pos._x = x;
    _pos._y = y;
}

void Rectangle::setPos(const Point& pos)
{
    _pos = pos;
}

void Rectangle::move(int x, int y)
{
    _pos._x += x;
    _pos._y +=  y;
}

void Rectangle::move(const Point& pos)
{
    _pos += pos;
}

void Rectangle::setWidth(int width)
{
    _size._width = width;
}

void Rectangle::setHeight(int height)
{
    _size._height = height;
}

void Rectangle::setSize(int width, int height)
{
    _size._width  = width;
    _size._height = height;
}

void Rectangle::setSize(const Size& size)
{
    _size = size;
}

Rectangle& Rectangle::operator=(const Rectangle& rect)
{
    _pos  = rect._pos;
    _size = rect._size;
    return *this;
}

#endif

// -------------------------------------------------

END_NAMESPACE_DISTRHO

