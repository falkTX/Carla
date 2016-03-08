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

#include "../Geometry.hpp"

#include <cmath>

START_NAMESPACE_DGL

static const float M_2PIf = 3.14159265358979323846f*2.0f;

// -----------------------------------------------------------------------
// Point

template<typename T>
Point<T>::Point() noexcept
    : fX(0),
      fY(0) {}

template<typename T>
Point<T>::Point(const T& x, const T& y) noexcept
    : fX(x),
      fY(y) {}

template<typename T>
Point<T>::Point(const Point<T>& pos) noexcept
    : fX(pos.fX),
      fY(pos.fY) {}

template<typename T>
const T& Point<T>::getX() const noexcept
{
    return fX;
}

template<typename T>
const T& Point<T>::getY() const noexcept
{
    return fY;
}

template<typename T>
void Point<T>::setX(const T& x) noexcept
{
    fX = x;
}

template<typename T>
void Point<T>::setY(const T& y) noexcept
{
    fY = y;
}

template<typename T>
void Point<T>::setPos(const T& x, const T& y) noexcept
{
    fX = x;
    fY = y;
}

template<typename T>
void Point<T>::setPos(const Point<T>& pos) noexcept
{
    fX = pos.fX;
    fY = pos.fY;
}

template<typename T>
void Point<T>::moveBy(const T& x, const T& y) noexcept
{
    fX = static_cast<T>(fX+x);
    fY = static_cast<T>(fY+y);
}

template<typename T>
void Point<T>::moveBy(const Point<T>& pos) noexcept
{
    fX = static_cast<T>(fX+pos.fX);
    fY = static_cast<T>(fY+pos.fY);
}

template<typename T>
bool Point<T>::isZero() const noexcept
{
    return fX == 0 && fY == 0;
}

template<typename T>
bool Point<T>::isNotZero() const noexcept
{
    return fX != 0 || fY != 0;
}

template<typename T>
Point<T> Point<T>::operator+(const Point<T>& pos) noexcept
{
    return Point<T>(fX+pos.fX, fY+pos.fY);
}

template<typename T>
Point<T> Point<T>::operator-(const Point<T>& pos) noexcept
{
    return Point<T>(fX-pos.fX, fY-pos.fY);
}

template<typename T>
Point<T>& Point<T>::operator=(const Point<T>& pos) noexcept
{
    fX = pos.fX;
    fY = pos.fY;
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator+=(const Point<T>& pos) noexcept
{
    fX = static_cast<T>(fX+pos.fX);
    fY = static_cast<T>(fY+pos.fY);
    return *this;
}

template<typename T>
Point<T>& Point<T>::operator-=(const Point<T>& pos) noexcept
{
    fX = static_cast<T>(fX-pos.fX);
    fY = static_cast<T>(fY-pos.fY);
    return *this;
}

template<typename T>
bool Point<T>::operator==(const Point<T>& pos) const noexcept
{
    return (fX == pos.fX && fY == pos.fY);
}

template<typename T>
bool Point<T>::operator!=(const Point<T>& pos) const noexcept
{
    return (fX != pos.fX || fY != pos.fY);
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
    return fWidth > 1 && fHeight > 1;
}

template<typename T>
bool Size<T>::isInvalid() const noexcept
{
    return fWidth <= 0 || fHeight <= 0;
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
    : fPosStart(0, 0),
      fPosEnd(0, 0) {}

template<typename T>
Line<T>::Line(const T& startX, const T& startY, const T& endX, const T& endY) noexcept
    : fPosStart(startX, startY),
      fPosEnd(endX, endY) {}

template<typename T>
Line<T>::Line(const T& startX, const T& startY, const Point<T>& endPos) noexcept
    : fPosStart(startX, startY),
      fPosEnd(endPos) {}

template<typename T>
Line<T>::Line(const Point<T>& startPos, const T& endX, const T& endY) noexcept
    : fPosStart(startPos),
      fPosEnd(endX, endY) {}

template<typename T>
Line<T>::Line(const Point<T>& startPos, const Point<T>& endPos) noexcept
    : fPosStart(startPos),
      fPosEnd(endPos) {}

template<typename T>
Line<T>::Line(const Line<T>& line) noexcept
    : fPosStart(line.fPosStart),
      fPosEnd(line.fPosEnd) {}

template<typename T>
const T& Line<T>::getStartX() const noexcept
{
    return fPosStart.fX;
}

template<typename T>
const T& Line<T>::getStartY() const noexcept
{
    return fPosStart.fY;
}

template<typename T>
const T& Line<T>::getEndX() const noexcept
{
    return fPosEnd.fX;
}

template<typename T>
const T& Line<T>::getEndY() const noexcept
{
    return fPosEnd.fY;
}

template<typename T>
const Point<T>& Line<T>::getStartPos() const noexcept
{
    return fPosStart;
}

template<typename T>
const Point<T>& Line<T>::getEndPos() const noexcept
{
    return fPosEnd;
}

template<typename T>
void Line<T>::setStartX(const T& x) noexcept
{
    fPosStart.fX = x;
}

template<typename T>
void Line<T>::setStartY(const T& y) noexcept
{
    fPosStart.fY = y;
}

template<typename T>
void Line<T>::setStartPos(const T& x, const T& y) noexcept
{
    fPosStart = Point<T>(x, y);
}

template<typename T>
void Line<T>::setStartPos(const Point<T>& pos) noexcept
{
    fPosStart = pos;
}

template<typename T>
void Line<T>::setEndX(const T& x) noexcept
{
    fPosEnd.fX = x;
}

template<typename T>
void Line<T>::setEndY(const T& y) noexcept
{
    fPosEnd.fY = y;
}

template<typename T>
void Line<T>::setEndPos(const T& x, const T& y) noexcept
{
    fPosEnd = Point<T>(x, y);
}

template<typename T>
void Line<T>::setEndPos(const Point<T>& pos) noexcept
{
    fPosEnd = pos;
}

template<typename T>
void Line<T>::moveBy(const T& x, const T& y) noexcept
{
    fPosStart.moveBy(x, y);
    fPosEnd.moveBy(x, y);
}

template<typename T>
void Line<T>::moveBy(const Point<T>& pos) noexcept
{
    fPosStart.moveBy(pos);
    fPosEnd.moveBy(pos);
}

template<typename T>
void Line<T>::draw()
{
    DISTRHO_SAFE_ASSERT_RETURN(fPosStart != fPosEnd,);

    glBegin(GL_LINES);

    {
        glVertex2d(fPosStart.fX, fPosStart.fY);
        glVertex2d(fPosEnd.fX, fPosEnd.fY);
    }

    glEnd();
}

template<typename T>
bool Line<T>::isNull() const noexcept
{
    return fPosStart == fPosEnd;
}

template<typename T>
bool Line<T>::isNotNull() const noexcept
{
    return fPosStart != fPosEnd;
}

template<typename T>
Line<T>& Line<T>::operator=(const Line<T>& line) noexcept
{
    fPosStart = line.fPosStart;
    fPosEnd   = line.fPosEnd;
    return *this;
}

template<typename T>
bool Line<T>::operator==(const Line<T>& line) const noexcept
{
    return (fPosStart == line.fPosStart && fPosEnd == line.fPosEnd);
}

template<typename T>
bool Line<T>::operator!=(const Line<T>& line) const noexcept
{
    return (fPosStart != line.fPosStart || fPosEnd != line.fPosEnd);
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
    return fPos.fX;
}

template<typename T>
const T& Circle<T>::getY() const noexcept
{
    return fPos.fX;
}

template<typename T>
const Point<T>& Circle<T>::getPos() const noexcept
{
    return fPos;
}

template<typename T>
void Circle<T>::setX(const T& x) noexcept
{
    fPos.fX = x;
}

template<typename T>
void Circle<T>::setY(const T& y) noexcept
{
    fPos.fY = y;
}

template<typename T>
void Circle<T>::setPos(const T& x, const T& y) noexcept
{
    fPos.fX = x;
    fPos.fY = y;
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
void Circle<T>::draw()
{
    _draw(false);
}

template<typename T>
void Circle<T>::drawOutline()
{
    _draw(true);
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

template<typename T>
void Circle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fNumSegments >= 3 && fSize > 0.0f,);

    double t, x = fSize, y = 0.0;

    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i=0; i<fNumSegments; ++i)
    {
        glVertex2d(x + fPos.fX, y + fPos.fY);

        t = x;
        x = fCos * x - fSin * y;
        y = fSin * t + fCos * y;
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Triangle

template<typename T>
Triangle<T>::Triangle() noexcept
    : fPos1(0, 0),
      fPos2(0, 0),
      fPos3(0, 0) {}

template<typename T>
Triangle<T>::Triangle(const T& x1, const T& y1, const T& x2, const T& y2, const T& x3, const T& y3) noexcept
    : fPos1(x1, y1),
      fPos2(x2, y2),
      fPos3(x3, y3) {}

template<typename T>
Triangle<T>::Triangle(const Point<T>& pos1, const Point<T>& pos2, const Point<T>& pos3) noexcept
    : fPos1(pos1),
      fPos2(pos2),
      fPos3(pos3) {}

template<typename T>
Triangle<T>::Triangle(const Triangle<T>& tri) noexcept
    : fPos1(tri.fPos1),
      fPos2(tri.fPos2),
      fPos3(tri.fPos3) {}

template<typename T>
void Triangle<T>::draw()
{
    _draw(false);
}

template<typename T>
void Triangle<T>::drawOutline()
{
    _draw(true);
}

template<typename T>
bool Triangle<T>::isNull() const noexcept
{
    return fPos1 == fPos2 && fPos1 == fPos3;
}

template<typename T>
bool Triangle<T>::isNotNull() const noexcept
{
    return fPos1 != fPos2 || fPos1 != fPos3;
}

template<typename T>
bool Triangle<T>::isValid() const noexcept
{
    return fPos1 != fPos2 && fPos1 != fPos3;
}

template<typename T>
bool Triangle<T>::isInvalid() const noexcept
{
    return fPos1 == fPos2 || fPos1 == fPos3;
}

template<typename T>
Triangle<T>& Triangle<T>::operator=(const Triangle<T>& tri) noexcept
{
    fPos1 = tri.fPos1;
    fPos2 = tri.fPos2;
    fPos3 = tri.fPos3;
    return *this;
}

template<typename T>
bool Triangle<T>::operator==(const Triangle<T>& tri) const noexcept
{
    return (fPos1 == tri.fPos1 && fPos2 == tri.fPos2 && fPos3 == tri.fPos3);
}

template<typename T>
bool Triangle<T>::operator!=(const Triangle<T>& tri) const noexcept
{
    return (fPos1 != tri.fPos1 || fPos2 != tri.fPos2 || fPos3 != tri.fPos3);
}

template<typename T>
void Triangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fPos1 != fPos2 && fPos1 != fPos3,);

    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLES);

    {
        glVertex2d(fPos1.fX, fPos1.fY);
        glVertex2d(fPos2.fX, fPos2.fY);
        glVertex2d(fPos3.fX, fPos3.fY);
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
Rectangle<T>::Rectangle() noexcept
    : fPos(0, 0),
      fSize(0, 0) {}

template<typename T>
Rectangle<T>::Rectangle(const T& x, const T& y, const T& width, const T& height) noexcept
    : fPos(x, y),
      fSize(width, height) {}

template<typename T>
Rectangle<T>::Rectangle(const T& x, const T& y, const Size<T>& size) noexcept
    : fPos(x, y),
      fSize(size) {}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& pos, const T& width, const T& height) noexcept
    : fPos(pos),
      fSize(width, height) {}

template<typename T>
Rectangle<T>::Rectangle(const Point<T>& pos, const Size<T>& size) noexcept
    : fPos(pos),
      fSize(size) {}

template<typename T>
Rectangle<T>::Rectangle(const Rectangle<T>& rect) noexcept
    : fPos(rect.fPos),
      fSize(rect.fSize) {}

template<typename T>
const T& Rectangle<T>::getX() const noexcept
{
    return fPos.fX;
}

template<typename T>
const T& Rectangle<T>::getY() const noexcept
{
    return fPos.fY;
}

template<typename T>
const T& Rectangle<T>::getWidth() const noexcept
{
    return fSize.fWidth;
}

template<typename T>
const T& Rectangle<T>::getHeight() const noexcept
{
    return fSize.fHeight;
}

template<typename T>
const Point<T>& Rectangle<T>::getPos() const noexcept
{
    return fPos;
}

template<typename T>
const Size<T>& Rectangle<T>::getSize() const noexcept
{
    return fSize;
}

template<typename T>
void Rectangle<T>::setX(const T& x) noexcept
{
    fPos.fX = x;
}

template<typename T>
void Rectangle<T>::setY(const T& y) noexcept
{
    fPos.fY = y;
}

template<typename T>
void Rectangle<T>::setPos(const T& x, const T& y) noexcept
{
    fPos.fX = x;
    fPos.fY = y;
}

template<typename T>
void Rectangle<T>::setPos(const Point<T>& pos) noexcept
{
    fPos = pos;
}

template<typename T>
void Rectangle<T>::moveBy(const T& x, const T& y) noexcept
{
    fPos.moveBy(x, y);
}

template<typename T>
void Rectangle<T>::moveBy(const Point<T>& pos) noexcept
{
    fPos.moveBy(pos);
}

template<typename T>
void Rectangle<T>::setWidth(const T& width) noexcept
{
    fSize.fWidth = width;
}

template<typename T>
void Rectangle<T>::setHeight(const T& height) noexcept
{
    fSize.fHeight = height;
}

template<typename T>
void Rectangle<T>::setSize(const T& width, const T& height) noexcept
{
    fSize.fWidth  = width;
    fSize.fHeight = height;
}

template<typename T>
void Rectangle<T>::setSize(const Size<T>& size) noexcept
{
    fSize = size;
}

template<typename T>
void Rectangle<T>::growBy(double multiplier) noexcept
{
    fSize.growBy(multiplier);
}

template<typename T>
void Rectangle<T>::shrinkBy(double divider) noexcept
{
    fSize.shrinkBy(divider);
}

template<typename T>
void Rectangle<T>::setRectangle(const Point<T>& pos, const Size<T>& size) noexcept
{
    fPos  = pos;
    fSize = size;
}

template<typename T>
void Rectangle<T>::setRectangle(const Rectangle<T>& rect) noexcept
{
    fPos  = rect.fPos;
    fSize = rect.fSize;
}

template<typename T>
bool Rectangle<T>::contains(const T& x, const T& y) const noexcept
{
    return (x >= fPos.fX && y >= fPos.fY && x <= fPos.fX+fSize.fWidth && y <= fPos.fY+fSize.fHeight);
}

template<typename T>
bool Rectangle<T>::contains(const Point<T>& pos) const noexcept
{
    return contains(pos.fX, pos.fY);
}

template<typename T>
bool Rectangle<T>::containsX(const T& x) const noexcept
{
    return (x >= fPos.fX && x <= fPos.fX + fSize.fWidth);
}

template<typename T>
bool Rectangle<T>::containsY(const T& y) const noexcept
{
    return (y >= fPos.fY && y <= fPos.fY + fSize.fHeight);
}

template<typename T>
void Rectangle<T>::draw()
{
    _draw(false);
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    _draw(true);
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator=(const Rectangle<T>& rect) noexcept
{
    fPos  = rect.fPos;
    fSize = rect.fSize;
    return *this;
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator*=(double m) noexcept
{
    fSize *= m;
    return *this;
}

template<typename T>
Rectangle<T>& Rectangle<T>::operator/=(double d) noexcept
{
    fSize /= d;
    return *this;
}

template<typename T>
bool Rectangle<T>::operator==(const Rectangle<T>& rect) const noexcept
{
    return (fPos == rect.fPos && fSize == rect.fSize);
}

template<typename T>
bool Rectangle<T>::operator!=(const Rectangle<T>& rect) const noexcept
{
    return (fPos != rect.fPos || fSize != rect.fSize);
}

template<typename T>
void Rectangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fSize.isValid(),);

    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);

    {
        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(fPos.fX, fPos.fY);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY+fSize.fHeight);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(fPos.fX, fPos.fY+fSize.fHeight);
    }

    glEnd();
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
