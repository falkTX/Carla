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

#ifdef _MSC_VER
# pragma warning(disable:4661) /* instantiated template classes whose methods are defined elsewhere */
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
#endif

#include "../Geometry.hpp"

#define DGL_GEOMETRY_CPP_INCLUDED

#include <cmath>

START_NAMESPACE_DGL

static const float M_2PIf = 3.14159265358979323846f*2.0f;

// -----------------------------------------------------------------------
// Point

template<typename T>
Point<T>::Point() noexcept
    : x(0),
      y(0) {}

template<typename T>
Point<T>::Point(const T& x2, const T& y2) noexcept
    : x(x2),
      y(y2) {}

template<typename T>
Point<T>::Point(const Point<T>& pos) noexcept
    : x(pos.x),
      y(pos.y) {}

template<typename T>
const T& Point<T>::getX() const noexcept
{
    return x;
}

template<typename T>
const T& Point<T>::getY() const noexcept
{
    return y;
}

template<typename T>
void Point<T>::setX(const T& x2) noexcept
{
    x = x2;
}

template<typename T>
void Point<T>::setY(const T& y2) noexcept
{
    y = y2;
}

template<typename T>
void Point<T>::setPos(const T& x2, const T& y2) noexcept
{
    x = x2;
    y = y2;
}

template<typename T>
void Point<T>::setPos(const Point<T>& pos) noexcept
{
    x = pos.x;
    y = pos.y;
}

template<typename T>
void Point<T>::moveBy(const T& x2, const T& y2) noexcept
{
    x = static_cast<T>(x+x2);
    y = static_cast<T>(y+y2);
}

template<typename T>
void Point<T>::moveBy(const Point<T>& pos) noexcept
{
    x = static_cast<T>(x+pos.x);
    y = static_cast<T>(y+pos.y);
}

template<typename T>
bool Point<T>::isZero() const noexcept
{
    return x == 0 && y == 0;
}

template<typename T>
bool Point<T>::isNotZero() const noexcept
{
    return x != 0 || y != 0;
}

template<typename T>
Point<T> Point<T>::operator+(const Point<T>& pos) noexcept
{
    return Point<T>(x+pos.x, y+pos.y);
}

template<typename T>
Point<T> Point<T>::operator-(const Point<T>& pos) noexcept
{
    return Point<T>(x-pos.x, y-pos.y);
}

template<typename T>
Point<T>& Point<T>::operator=(const Point<T>& pos) noexcept
{
    x = pos.x;
    y = pos.y;
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator+=(const Point<T>& pos) noexcept
{
    x = static_cast<T>(x+pos.x);
    y = static_cast<T>(y+pos.y);
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator-=(const Point<T>& pos) noexcept
{
    x = static_cast<T>(x-pos.x);
    y = static_cast<T>(y-pos.y);
    return *this;
}

template<typename T>
bool Point<T>::operator==(const Point<T>& pos) const noexcept
{
    return (x == pos.x && y == pos.y);
}

template<typename T>
bool Point<T>::operator!=(const Point<T>& pos) const noexcept
{
    return (x != pos.x || y != pos.y);
}

// -----------------------------------------------------------------------
// Size

template<typename T>
Size<T>::Size() noexcept
    : fWidth(0),
      fHeight(0) {}

template<typename T>
Size<T>::Size(const T& width, const T& height) noexcept
    : fWidth(width),
      fHeight(height) {}

template<typename T>
Size<T>::Size(const Size<T>& size) noexcept
    : fWidth(size.fWidth),
      fHeight(size.fHeight) {}

template<typename T>
const T& Size<T>::getWidth() const noexcept
{
    return fWidth;
}

template<typename T>
const T& Size<T>::getHeight() const noexcept
{
    return fHeight;
}

template<typename T>
void Size<T>::setWidth(const T& width) noexcept
{
    fWidth = width;
}

template<typename T>
void Size<T>::setHeight(const T& height) noexcept
{
    fHeight = height;
}

template<typename T>
void Size<T>::setSize(const T& width, const T& height) noexcept
{
    fWidth  = width;
    fHeight = height;
}

template<typename T>
void Size<T>::setSize(const Size<T>& size) noexcept
{
    fWidth  = size.fWidth;
    fHeight = size.fHeight;
}

template<typename T>
void Size<T>::growBy(double multiplier) noexcept
{
    fWidth  = static_cast<T>(static_cast<double>(fWidth)*multiplier);
    fHeight = static_cast<T>(static_cast<double>(fHeight)*multiplier);
}

template<typename T>
void Size<T>::shrinkBy(double divider) noexcept
{
    fWidth  = static_cast<T>(static_cast<double>(fWidth)/divider);
    fHeight = static_cast<T>(static_cast<double>(fHeight)/divider);
}

template<typename T>
bool Size<T>::isNull() const noexcept
{
    return fWidth == 0 && fHeight == 0;
}

template<typename T>
bool Size<T>::isNotNull() const noexcept
{
    return fWidth != 0 || fHeight != 0;
}

template<typename T>
bool Size<T>::isValid() const noexcept
{
    return fWidth > 0 && fHeight > 0;
}

template<typename T>
bool Size<T>::isInvalid() const noexcept
{
    return fWidth <= 0 || fHeight <= 0;
}

template<typename T>
Size<int> Size<T>::toInt() const noexcept
{
    return Size<int>(static_cast<int>(fWidth),
                     static_cast<int>(fHeight));
}

template<>
Size<int> Size<double>::toInt() const noexcept
{
    return Size<int>(static_cast<int>(fWidth + 0.5),
                     static_cast<int>(fHeight + 0.5));
}

template<>
Size<int> Size<float>::toInt() const noexcept
{
    return Size<int>(static_cast<int>(fWidth + 0.5f),
                     static_cast<int>(fHeight + 0.5f));
}

template<typename T>
Size<T> Size<T>::operator+(const Size<T>& size) noexcept
{
    return Size<T>(fWidth+size.fWidth, fHeight+size.fHeight);
}

template<typename T>
Size<T> Size<T>::operator-(const Size<T>& size) noexcept
{
    return Size<T>(fWidth-size.fWidth, fHeight-size.fHeight);
}

template<typename T>
Size<T>& Size<T>::operator=(const Size<T>& size) noexcept
{
    fWidth  = size.fWidth;
    fHeight = size.fHeight;
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator+=(const Size<T>& size) noexcept
{
    fWidth  = static_cast<T>(fWidth+size.fWidth);
    fHeight = static_cast<T>(fHeight+size.fHeight);
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator-=(const Size<T>& size) noexcept
{
    fWidth  = static_cast<T>(fWidth-size.fWidth);
    fHeight = static_cast<T>(fHeight-size.fHeight);
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator*=(double m) noexcept
{
    fWidth  = static_cast<T>(static_cast<double>(fWidth)*m);
    fHeight = static_cast<T>(static_cast<double>(fHeight)*m);
    return *this;
}

template<typename T>
Size<T>& Size<T>::operator/=(double d) noexcept
{
    fWidth  = static_cast<T>(static_cast<double>(fWidth)/d);
    fHeight = static_cast<T>(static_cast<double>(fHeight)/d);
    return *this;
}

template<typename T>
Size<T> Size<T>::operator*(const double m) const noexcept
{
    Size<T> size(fWidth, fHeight);
    size *= m;
    return size;
}

template<typename T>
Size<T> Size<T>::operator/(const double m) const noexcept
{
    Size<T> size(fWidth, fHeight);
    size /= m;
    return size;
}

template<typename T>
bool Size<T>::operator==(const Size<T>& size) const noexcept
{
    return (fWidth == size.fWidth && fHeight == size.fHeight);
}

template<typename T>
bool Size<T>::operator!=(const Size<T>& size) const noexcept
{
    return (fWidth != size.fWidth || fHeight != size.fHeight);
}

// -----------------------------------------------------------------------
// Line

template<typename T>
Line<T>::Line() noexcept
    : posStart(0, 0),
      posEnd(0, 0) {}

template<typename T>
Line<T>::Line(const T& startX, const T& startY, const T& endX, const T& endY) noexcept
    : posStart(startX, startY),
      posEnd(endX, endY) {}

template<typename T>
Line<T>::Line(const T& startX, const T& startY, const Point<T>& endPos) noexcept
    : posStart(startX, startY),
      posEnd(endPos) {}

template<typename T>
Line<T>::Line(const Point<T>& startPos, const T& endX, const T& endY) noexcept
    : posStart(startPos),
      posEnd(endX, endY) {}

template<typename T>
Line<T>::Line(const Point<T>& startPos, const Point<T>& endPos) noexcept
    : posStart(startPos),
      posEnd(endPos) {}

template<typename T>
Line<T>::Line(const Line<T>& line) noexcept
    : posStart(line.posStart),
      posEnd(line.posEnd) {}

template<typename T>
const T& Line<T>::getStartX() const noexcept
{
    return posStart.x;
}

template<typename T>
const T& Line<T>::getStartY() const noexcept
{
    return posStart.y;
}

template<typename T>
const T& Line<T>::getEndX() const noexcept
{
    return posEnd.x;
}

template<typename T>
const T& Line<T>::getEndY() const noexcept
{
    return posEnd.y;
}

template<typename T>
const Point<T>& Line<T>::getStartPos() const noexcept
{
    return posStart;
}

template<typename T>
const Point<T>& Line<T>::getEndPos() const noexcept
{
    return posEnd;
}

template<typename T>
void Line<T>::setStartX(const T& x) noexcept
{
    posStart.x = x;
}

template<typename T>
void Line<T>::setStartY(const T& y) noexcept
{
    posStart.y = y;
}

template<typename T>
void Line<T>::setStartPos(const T& x, const T& y) noexcept
{
    posStart = Point<T>(x, y);
}

template<typename T>
void Line<T>::setStartPos(const Point<T>& pos) noexcept
{
    posStart = pos;
}

template<typename T>
void Line<T>::setEndX(const T& x) noexcept
{
    posEnd.x = x;
}

template<typename T>
void Line<T>::setEndY(const T& y) noexcept
{
    posEnd.y = y;
}

template<typename T>
void Line<T>::setEndPos(const T& x, const T& y) noexcept
{
    posEnd = Point<T>(x, y);
}

template<typename T>
void Line<T>::setEndPos(const Point<T>& pos) noexcept
{
    posEnd = pos;
}

template<typename T>
void Line<T>::moveBy(const T& x, const T& y) noexcept
{
    posStart.moveBy(x, y);
    posEnd.moveBy(x, y);
}

template<typename T>
void Line<T>::moveBy(const Point<T>& pos) noexcept
{
    posStart.moveBy(pos);
    posEnd.moveBy(pos);
}

template<typename T>
bool Line<T>::isNull() const noexcept
{
    return posStart == posEnd;
}

template<typename T>
bool Line<T>::isNotNull() const noexcept
{
    return posStart != posEnd;
}

template<typename T>
Line<T>& Line<T>::operator=(const Line<T>& line) noexcept
{
    posStart = line.posStart;
    posEnd   = line.posEnd;
    return *this;
}

template<typename T>
bool Line<T>::operator==(const Line<T>& line) const noexcept
{
    return (posStart == line.posStart && posEnd == line.posEnd);
}

template<typename T>
bool Line<T>::operator!=(const Line<T>& line) const noexcept
{
    return (posStart != line.posStart || posEnd != line.posEnd);
}

// -----------------------------------------------------------------------
// Circle

template<typename T>
Circle<T>::Circle() noexcept
    : fPos(0, 0),
      fSize(0.0f),
      fNumSegments(0),
      fTheta(0.0f),
      fCos(0.0f),
      fSin(0.0f) {}

template<typename T>
Circle<T>::Circle(const T& x, const T& y, const float size, const uint numSegments)
    : fPos(x, y),
      fSize(size),
      fNumSegments(numSegments >= 3 ? numSegments : 3),
      fTheta(M_2PIf / static_cast<float>(fNumSegments)),
      fCos(std::cos(fTheta)),
      fSin(std::sin(fTheta))
{
    DISTRHO_SAFE_ASSERT(fSize > 0.0f);
}

template<typename T>
Circle<T>::Circle(const Point<T>& pos, const float size, const uint numSegments)
    : fPos(pos),
      fSize(size),
      fNumSegments(numSegments >= 3 ? numSegments : 3),
      fTheta(M_2PIf / static_cast<float>(fNumSegments)),
      fCos(std::cos(fTheta)),
      fSin(std::sin(fTheta))
{
    DISTRHO_SAFE_ASSERT(fSize > 0.0f);
}

template<typename T>
Circle<T>::Circle(const Circle<T>& cir) noexcept
    : fPos(cir.fPos),
      fSize(cir.fSize),
      fNumSegments(cir.fNumSegments),
      fTheta(cir.fTheta),
      fCos(cir.fCos),
      fSin(cir.fSin)
{
    DISTRHO_SAFE_ASSERT(fSize > 0.0f);
}

template<typename T>
const T& Circle<T>::getX() const noexcept
{
    return fPos.x;
}

template<typename T>
const T& Circle<T>::getY() const noexcept
{
    return fPos.y;
}

template<typename T>
const Point<T>& Circle<T>::getPos() const noexcept
{
    return fPos;
}

template<typename T>
void Circle<T>::setX(const T& x) noexcept
{
    fPos.x = x;
}

template<typename T>
void Circle<T>::setY(const T& y) noexcept
{
    fPos.y = y;
}

template<typename T>
void Circle<T>::setPos(const T& x, const T& y) noexcept
{
    fPos.x = x;
    fPos.y = y;
}

template<typename T>
void Circle<T>::setPos(const Point<T>& pos) noexcept
{
    fPos = pos;
}

template<typename T>
float Circle<T>::getSize() const noexcept
{
    return fSize;
}

template<typename T>
void Circle<T>::setSize(const float size) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0.0f,);

    fSize = size;
}

template<typename T>
uint Circle<T>::getNumSegments() const noexcept
{
    return fNumSegments;
}

template<typename T>
void Circle<T>::setNumSegments(const uint num)
{
    DISTRHO_SAFE_ASSERT_RETURN(num >= 3,);

    if (fNumSegments == num)
        return;

    fNumSegments = num;

    fTheta = M_2PIf / static_cast<float>(fNumSegments);
    fCos = std::cos(fTheta);
    fSin = std::sin(fTheta);
}

template<typename T>
Circle<T>& Circle<T>::operator=(const Circle<T>& cir) noexcept
{
    fPos   = cir.fPos;
    fSize  = cir.fSize;
    fTheta = cir.fTheta;
    fCos   = cir.fCos;
    fSin   = cir.fSin;
    fNumSegments = cir.fNumSegments;
    return *this;
}

template<typename T>
bool Circle<T>::operator==(const Circle<T>& cir) const noexcept
{
    return (fPos == cir.fPos && d_isEqual(fSize, cir.fSize) && fNumSegments == cir.fNumSegments);
}

template<typename T>
bool Circle<T>::operator!=(const Circle<T>& cir) const noexcept
{
    return (fPos != cir.fPos || d_isNotEqual(fSize, cir.fSize) || fNumSegments != cir.fNumSegments);
}

// -----------------------------------------------------------------------
// Triangle

template<typename T>
Triangle<T>::Triangle() noexcept
    : pos1(0, 0),
      pos2(0, 0),
      pos3(0, 0) {}

template<typename T>
Triangle<T>::Triangle(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3) noexcept
    : pos1(x1, y1),
      pos2(x2, y2),
      pos3(x3, y3) {}

template<typename T>
Triangle<T>::Triangle(const Point<T>& p1, const Point<T>& p2, const Point<T>& p3) noexcept
    : pos1(p1),
      pos2(p2),
      pos3(p3) {}

template<typename T>
Triangle<T>::Triangle(const Triangle<T>& tri) noexcept
    : pos1(tri.pos1),
      pos2(tri.pos2),
      pos3(tri.pos3) {}

template<typename T>
bool Triangle<T>::isNull() const noexcept
{
    return pos1 == pos2 && pos1 == pos3;
}

template<typename T>
bool Triangle<T>::isNotNull() const noexcept
{
    return pos1 != pos2 || pos1 != pos3;
}

template<typename T>
bool Triangle<T>::isValid() const noexcept
{
    return pos1 != pos2 && pos1 != pos3;
}

template<typename T>
bool Triangle<T>::isInvalid() const noexcept
{
    return pos1 == pos2 || pos1 == pos3;
}

template<typename T>
Triangle<T>& Triangle<T>::operator=(const Triangle<T>& tri) noexcept
{
    pos1 = tri.pos1;
    pos2 = tri.pos2;
    pos3 = tri.pos3;
    return *this;
}

template<typename T>
bool Triangle<T>::operator==(const Triangle<T>& tri) const noexcept
{
    return (pos1 == tri.pos1 && pos2 == tri.pos2 && pos3 == tri.pos3);
}

template<typename T>
bool Triangle<T>::operator!=(const Triangle<T>& tri) const noexcept
{
    return (pos1 != tri.pos1 || pos2 != tri.pos2 || pos3 != tri.pos3);
}

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
Rectangle<T>::Rectangle() noexcept
    : pos(0, 0),
      size(0, 0) {}

template<typename T>
Rectangle<T>::Rectangle(const T& x, const T& y, const T& w, const T& h) noexcept
    : pos(x, y),
      size(w, h) {}

template<typename T>
Rectangle<T>::Rectangle(const T& x, const T& y, const Size<T>& s) noexcept
    : pos(x, y),
      size(s) {}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& p, const T& w, const T& h) noexcept
    : pos(p),
      size(w, h) {}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& p, const Size<T>& s) noexcept
    : pos(p),
      size(s) {}

template<typename T>
Rectangle<T>::Rectangle(const Rectangle<T>& rect) noexcept
    : pos(rect.pos),
      size(rect.size) {}

template<typename T>
const T& Rectangle<T>::getX() const noexcept
{
    return pos.x;
}

template<typename T>
const T& Rectangle<T>::getY() const noexcept
{
    return pos.y;
}

template<typename T>
const T& Rectangle<T>::getWidth() const noexcept
{
    return size.fWidth;
}

template<typename T>
const T& Rectangle<T>::getHeight() const noexcept
{
    return size.fHeight;
}

template<typename T>
const Point<T>& Rectangle<T>::getPos() const noexcept
{
    return pos;
}

template<typename T>
const Size<T>& Rectangle<T>::getSize() const noexcept
{
    return size;
}

template<typename T>
void Rectangle<T>::setX(const T& x) noexcept
{
    pos.x = x;
}

template<typename T>
void Rectangle<T>::setY(const T& y) noexcept
{
    pos.y = y;
}

template<typename T>
void Rectangle<T>::setPos(const T& x, const T& y) noexcept
{
    pos.x = x;
    pos.y = y;
}

template<typename T>
void Rectangle<T>::setPos(const Point<T>& pos2) noexcept
{
    pos = pos2;
}

template<typename T>
void Rectangle<T>::moveBy(const T& x, const T& y) noexcept
{
    pos.moveBy(x, y);
}

template<typename T>
void Rectangle<T>::moveBy(const Point<T>& pos2) noexcept
{
    pos.moveBy(pos2);
}

template<typename T>
void Rectangle<T>::setWidth(const T& width) noexcept
{
    size.fWidth = width;
}

template<typename T>
void Rectangle<T>::setHeight(const T& height) noexcept
{
    size.fHeight = height;
}

template<typename T>
void Rectangle<T>::setSize(const T& width, const T& height) noexcept
{
    size.fWidth  = width;
    size.fHeight = height;
}

template<typename T>
void Rectangle<T>::setSize(const Size<T>& size2) noexcept
{
    size = size2;
}

template<typename T>
void Rectangle<T>::growBy(double multiplier) noexcept
{
    size.growBy(multiplier);
}

template<typename T>
void Rectangle<T>::shrinkBy(double divider) noexcept
{
    size.shrinkBy(divider);
}

template<typename T>
void Rectangle<T>::setRectangle(const Point<T>& pos2, const Size<T>& size2) noexcept
{
    pos  = pos2;
    size = size2;
}

template<typename T>
void Rectangle<T>::setRectangle(const Rectangle<T>& rect) noexcept
{
    pos  = rect.pos;
    size = rect.size;
}

template<typename T>
bool Rectangle<T>::contains(const T& x, const T& y) const noexcept
{
    return (x >= pos.x && y >= pos.y && x <= pos.x+size.fWidth && y <= pos.y+size.fHeight);
}

template<typename T>
bool Rectangle<T>::contains(const Point<T>& p) const noexcept
{
    return contains(p.x, p.y);
}

template<typename T>
template<typename T2>
bool Rectangle<T>::contains(const Point<T2>& p) const noexcept
{
    return (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x+size.fWidth && p.y <= pos.y+size.fHeight);
}

template<> template<>
bool Rectangle<int>::contains(const Point<double>& p) const noexcept
{
    return (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x+size.fWidth && p.y <= pos.y+size.fHeight);
}

template<> template<>
bool Rectangle<uint>::contains(const Point<double>& p) const noexcept
{
    return (p.x >= pos.x && p.y >= pos.y && p.x <= pos.x+size.fWidth && p.y <= pos.y+size.fHeight);
}

template<typename T>
bool Rectangle<T>::containsAfterScaling(const Point<T>& p, const double scaling) const noexcept
{
    return (p.x >= pos.x && p.y >= pos.y && p.x/scaling <= pos.x+size.fWidth && p.y/scaling <= pos.y+size.fHeight);
}

template<typename T>
bool Rectangle<T>::containsX(const T& x) const noexcept
{
    return (x >= pos.x && x <= pos.x + size.fWidth);
}

template<typename T>
bool Rectangle<T>::containsY(const T& y) const noexcept
{
    return (y >= pos.y && y <= pos.y + size.fHeight);
}

template<typename T>
bool Rectangle<T>::isNull() const noexcept
{
    return size.isNull();
}

template<typename T>
bool Rectangle<T>::isNotNull() const noexcept
{
    return size.isNotNull();
}

template<typename T>
bool Rectangle<T>::isValid() const noexcept
{
    return size.isValid();
}

template<typename T>
bool Rectangle<T>::isInvalid() const noexcept
{
    return size.isInvalid();
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator=(const Rectangle<T>& rect) noexcept
{
    pos  = rect.pos;
    size = rect.size;
    return *this;
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator*=(double m) noexcept
{
    size *= m;
    return *this;
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator/=(double d) noexcept
{
    size /= d;
    return *this;
}

template<typename T>
bool Rectangle<T>::operator==(const Rectangle<T>& rect) const noexcept
{
    return (pos == rect.pos && size == rect.size);
}

template<typename T>
bool Rectangle<T>::operator!=(const Rectangle<T>& rect) const noexcept
{
    return (pos != rect.pos || size != rect.size);
}

// -----------------------------------------------------------------------
// Possible template data types

template class Point<double>;
template class Point<float>;
template class Point<int>;
template class Point<uint>;
template class Point<short>;
template class Point<ushort>;

template class Size<double>;
template class Size<float>;
template class Size<int>;
template class Size<uint>;
template class Size<short>;
template class Size<ushort>;

template class Line<double>;
template class Line<float>;
template class Line<int>;
template class Line<uint>;
template class Line<short>;
template class Line<ushort>;

template class Circle<double>;
template class Circle<float>;
template class Circle<int>;
template class Circle<uint>;
template class Circle<short>;
template class Circle<ushort>;

template class Triangle<double>;
template class Triangle<float>;
template class Triangle<int>;
template class Triangle<uint>;
template class Triangle<short>;
template class Triangle<ushort>;

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
