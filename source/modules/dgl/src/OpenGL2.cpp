/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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
// instantiated template classes whose methods are defined elsewhere
# pragma warning(disable:4661)
#endif

#include "../OpenGL.hpp"
#include "../Color.hpp"
#include "../ImageWidgets.hpp"

// #include "SubWidgetPrivateData.hpp"
// #include "TopLevelWidgetPrivateData.hpp"
// #include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

// templated classes
#include "ImageBaseWidgets.cpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Check for correct build config

#ifndef DGL_OPENGL
# error Build config error, OpenGL was NOT requested while building OpenGL2 code
#endif
#ifdef DGL_CAIRO
# error Build config error, Cairo requested while building OpenGL2 code
#endif
#ifdef DGL_VULKAN
# error Build config error, Vulkan requested while building OpenGL2 code
#endif
#ifdef DGL_USE_GLES2
# error Build config error, GLESv2 requested while building OpenGL2 code
#endif
#ifdef DGL_USE_GLES3
# error Build config error, GLESv3 requested while building OpenGL2 code
#endif
#ifdef DGL_USE_OPENGL3
# error Build config error, OpenGL3 requested while building OpenGL2 code
#endif

// --------------------------------------------------------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext&, const bool includeAlpha)
{
    if (includeAlpha)
        glColor4f(red, green, blue, alpha);
    else
        glColor3f(red, green, blue);
}

// --------------------------------------------------------------------------------------------------------------------
// Line

template<typename T>
static void drawLine(const Point<T>& posStart, const Point<T>& posEnd)
{
    DISTRHO_SAFE_ASSERT_RETURN(posStart != posEnd,);

    glBegin(GL_LINES);

    {
        glVertex2d(posStart.getX(), posStart.getY());
        glVertex2d(posEnd.getX(), posEnd.getY());
    }

    glEnd();
}

template<typename T>
void Line<T>::draw(const GraphicsContext&, const T width)
{
    DISTRHO_SAFE_ASSERT_RETURN(width != 0,);

    glLineWidth(static_cast<GLfloat>(width));
    drawLine<T>(posStart, posEnd);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Line<T>::draw()
{
    drawLine<T>(posStart, posEnd);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Circle

template<typename T>
static void drawCircle(const Point<T>& pos,
                       const uint numSegments,
                       const float size,
                       const float sin,
                       const float cos,
                       const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(numSegments >= 3 && size > 0.0f,);

    const double origx = static_cast<double>(pos.getX());
    const double origy = static_cast<double>(pos.getY());
    double t;
    double x = size;
    double y = 0.0;

    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i = 0; i < numSegments; ++i)
    {
        glVertex2d(x + origx, y + origy);

        t = x;
        x = cos * x - sin * y;
        y = sin * t + cos * y;
    }

    glEnd();
}

template<typename T>
static void drawCircle(const GraphicsContext&,
                       const Point<T>& pos,
                       const uint numSegments,
                       const float size,
                       const float sin,
                       const float cos,
                       const bool outline)
{
    drawCircle<T>(pos, numSegments, size, sin, cos, outline);
}

template<typename T>
void Circle<T>::draw(const GraphicsContext& context)
{
    drawCircle<T>(context, fPos, fNumSegments, fSize, fSin, fCos, false);
}

template<typename T>
void Circle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(static_cast<GLfloat>(lineWidth));
    drawCircle<T>(context, fPos, fNumSegments, fSize, fSin, fCos, true);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Circle<T>::draw()
{
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, false);
}

template<typename T>
void Circle<T>::drawOutline()
{
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, true);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Triangle

template<typename T>
static void drawTriangle(const Point<T>& pos1,
                         const Point<T>& pos2,
                         const Point<T>& pos3,
                         const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(pos1 != pos2 && pos1 != pos3,);

    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLES);

    {
        glVertex2d(pos1.getX(), pos1.getY());
        glVertex2d(pos2.getX(), pos2.getY());
        glVertex2d(pos3.getX(), pos3.getY());
    }

    glEnd();
}

template<typename T>
static void drawTriangle(const GraphicsContext&,
                         const Point<T>& pos1,
                         const Point<T>& pos2,
                         const Point<T>& pos3,
                         const bool outline)
{
    drawTriangle<T>(pos1, pos2, pos3, outline);
}

template<typename T>
void Triangle<T>::draw(const GraphicsContext&)
{
    drawTriangle<T>(pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext&, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(static_cast<GLfloat>(lineWidth));
    drawTriangle<T>(pos1, pos2, pos3, true);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Triangle<T>::draw()
{
    drawTriangle<T>(pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline()
{
    drawTriangle<T>(pos1, pos2, pos3, true);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Rectangle

template<typename T>
static void drawRectangle(const Rectangle<T>& rect, const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(rect.isValid(),);

    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);

    {
        const T x = rect.getX();
        const T y = rect.getY();
        const T w = rect.getWidth();
        const T h = rect.getHeight();

        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(x, y);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(x+w, y);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(x+w, y+h);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(x, y+h);
    }

    glEnd();
}

template<typename T>
void Rectangle<T>::draw(const GraphicsContext&)
{
    drawRectangle<T>(*this, false);
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext&, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(static_cast<GLfloat>(lineWidth));
    drawRectangle<T>(*this, true);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Rectangle<T>::draw()
{
    drawRectangle<T>(*this, false);
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    drawRectangle<T>(*this, true);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// OpenGLImage

static void setupOpenGLImage(const OpenGLImage& image, GLuint textureId)
{
    DISTRHO_SAFE_ASSERT_RETURN(image.isValid(),);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 static_cast<GLsizei>(image.getWidth()),
                 static_cast<GLsizei>(image.getHeight()),
                 0,
                 asOpenGLImageFormat(image.getFormat()),
                 GL_UNSIGNED_BYTE,
                 image.getRawData());

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

static void drawOpenGLImage(const OpenGLImage& image, const Point<int>& pos, const GLuint textureId, bool& setupCalled)
{
    if (textureId == 0 || image.isInvalid())
        return;

    if (! setupCalled)
    {
        setupOpenGLImage(image, textureId);
        setupCalled = true;
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBegin(GL_QUADS);

    {
        const int x = pos.getX();
        const int y = pos.getY();
        const int w = static_cast<int>(image.getWidth());
        const int h = static_cast<int>(image.getHeight());

        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(x, y);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(x+w, y);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(x+w, y+h);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(x, y+h);
    }

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void OpenGLImage::drawAt(const GraphicsContext&, const Point<int>& pos)
{
    drawOpenGLImage(*this, pos, textureId, setupCalled);
}

#if DGL_ALLOW_DEPRECATED_METHODS
void OpenGLImage::draw()
{
    drawOpenGLImage(*this, Point<int>(0, 0), textureId, setupCalled);
}

void OpenGLImage::drawAt(const int x, const int y)
{
    drawOpenGLImage(*this, Point<int>(x, y), textureId, setupCalled);
}

void OpenGLImage::drawAt(const Point<int>& pos)
{
    drawOpenGLImage(*this, pos, textureId, setupCalled);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseAboutWindow

#if 0
template <>
void ImageBaseAboutWindow<OpenGLImage>::onDisplay()
{
const GraphicsContext& context(getGraphicsContext());
img.draw(context);
}
#endif

template class ImageBaseAboutWindow<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseButton

template class ImageBaseButton<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseKnob

template <>
void ImageBaseKnob<OpenGLImage>::PrivateData::init()
{
    glTextureId = 0;
    glGenTextures(1, &glTextureId);
}

template <>
void ImageBaseKnob<OpenGLImage>::PrivateData::cleanup()
{
    if (glTextureId == 0)
        return;

    glDeleteTextures(1, &glTextureId);
    glTextureId = 0;
}

template <>
void ImageBaseKnob<OpenGLImage>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());
    const float normValue = getNormalizedValue();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pData->glTextureId);

    if (! pData->isReady)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        uint imageDataOffset = 0;

        if (pData->rotationAngle == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(pData->imgLayerCount > 0,);
            DISTRHO_SAFE_ASSERT_RETURN(normValue >= 0.0f,);

            const uint& v1(pData->isImgVertical ? pData->imgLayerWidth : pData->imgLayerHeight);
            const uint& v2(pData->isImgVertical ? pData->imgLayerHeight : pData->imgLayerWidth);

            // TODO kImageFormatGreyscale
            const uint layerDataSize   = v1 * v2 * ((pData->image.getFormat() == kImageFormatBGRA ||
                                                     pData->image.getFormat() == kImageFormatRGBA) ? 4 : 3);
            /*      */ imageDataOffset = layerDataSize * uint(normValue * float(pData->imgLayerCount-1));
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(getWidth()), static_cast<GLsizei>(getHeight()), 0,
                     asOpenGLImageFormat(pData->image.getFormat()), GL_UNSIGNED_BYTE, pData->image.getRawData() + imageDataOffset);

        pData->isReady = true;
    }

    const int w = static_cast<int>(getWidth());
    const int h = static_cast<int>(getHeight());

    if (pData->rotationAngle != 0)
    {
        glPushMatrix();

        const int w2 = w/2;
        const int h2 = h/2;

        glTranslatef(static_cast<float>(w2), static_cast<float>(h2), 0.0f);
        glRotatef(normValue*static_cast<float>(pData->rotationAngle), 0.0f, 0.0f, 1.0f);

        Rectangle<int>(-w2, -h2, w, h).draw(context);

        glPopMatrix();
    }
    else
    {
        Rectangle<int>(0, 0, w, h).draw(context);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

template class ImageBaseKnob<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseSlider

template class ImageBaseSlider<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseSwitch

template class ImageBaseSwitch<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------

void Window::PrivateData::createContextIfNeeded()
{
}

void Window::PrivateData::destroyContext()
{
}

void Window::PrivateData::startContext()
{
    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

    glViewport(0, 0, static_cast<GLsizei>(size.width), static_cast<GLsizei>(size.height));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0,
            static_cast<GLdouble>(size.width) / autoScaleFactor,
            static_cast<GLdouble>(size.height) / autoScaleFactor,
            0.0,
            0.0,
            1.0);
    glViewport(0, 0, static_cast<GLsizei>(size.width), static_cast<GLsizei>(size.height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.f, 1.f, 1.f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Window::PrivateData::endContext()
{
}

// --------------------------------------------------------------------------------------------------------------------

#ifndef DGL_GEOMETRY_CPP_INCLUDED
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
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
