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

#ifdef _MSC_VER
// instantiated template classes whose methods are defined elsewhere
# pragma warning(disable:4661)
#endif

#include "../OpenGL.hpp"
#include "../Color.hpp"
#include "../ImageWidgets.hpp"

#include "WindowPrivateData.hpp"

// templated classes
#include "ImageBaseWidgets.cpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Check for correct build config

#ifndef DGL_OPENGL
# error Build config error, OpenGL was NOT requested while building OpenGL3 code
#endif
#ifndef DGL_USE_OPENGL3
# error Build config error, OpenGL3 not requested while building OpenGL3 code
#endif
#ifdef DGL_CAIRO
# error Build config error, Cairo requested while building OpenGL3 code
#endif
#ifdef DGL_VULKAN
# error Build config error, Vulkan requested while building OpenGL3 code
#endif
#if defined(DGL_USE_GLES2) && defined(DGL_USE_GLES3)
# error Build config error, both GLESv2 and GLESv3 requested at the same time
#endif
#if defined(DGL_USE_GLES2) && !defined(DGL_USE_GLES)
# error Build config error, DGL_USE_GLES2 is defined but DGL_USE_GLES is not
#endif
#if defined(DGL_USE_GLES3) && !defined(DGL_USE_GLES)
# error Build config error, DGL_USE_GLES3 is defined but DGL_USE_GLES is not
#endif
#if defined(DGL_USE_GLES) && !(defined(DGL_USE_GLES2) || defined(DGL_USE_GLES3))
# error Build config error, DGL_USE_GLES is defined which requires either DGL_USE_GLES2 or DGL_USE_GLES3
#endif

// --------------------------------------------------------------------------------------------------------------------
// Load OpenGL3 symbols on Windows

#if defined(DISTRHO_OS_WINDOWS)
# include <windows.h>
# define DGL_EXT(PROC, func) static PROC func;
DGL_EXT(PFNGLACTIVETEXTUREPROC,            glActiveTexture)
DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
DGL_EXT(PFNGLGETATTRIBLOCATIONPROC,        glGetAttribLocation)
DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
DGL_EXT(PFNGLUNIFORM4FVPROC,               glUniform4fv)
DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
# undef DGL_EXT
#endif

// --------------------------------------------------------------------------------------------------------------------

#if DGL_ALLOW_DEPRECATED_METHODS
static void notImplemented(const char* const name)
{
    d_stderr2("OpenGL3 function not implemented: %s", name);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext& context, const bool includeAlpha)
{
    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    const GLfloat color[4] = { red, green, blue, includeAlpha ? alpha : 1.f };
    glUniform4fv(gl3context.color, 1, color);
}

// --------------------------------------------------------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw(const GraphicsContext& context, const T width)
{
    DISTRHO_SAFE_ASSERT_RETURN(width != 0,);

    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    const GLfloat x1 = (static_cast<double>(posStart.x) / gl3context.width) * 2 - 1;
    const GLfloat y1 = (static_cast<double>(posStart.y) / gl3context.height) * -2 + 1;
    const GLfloat x2 = (static_cast<double>(posEnd.x) / gl3context.width) * 2 - 1;
    const GLfloat y2 = (static_cast<double>(posEnd.y) / gl3context.height) * -2 + 1;

    glLineWidth(static_cast<GLfloat>(width));

    const GLfloat vertices[] = { x1, y1, x2, y2, };
    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    const GLubyte order[] = { 0, 1 };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);

    glDrawElements(GL_LINES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(gl3context.bounds);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Line<T>::draw()
{
    notImplemented("Line::draw");
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Circle

template<typename T>
static void drawCircle(const GraphicsContext& context,
                       const Point<T>& pos,
                       const uint numSegments,
                       const float size,
                       const float sin,
                       const float cos,
                       const bool outline)
{
    #define MAX_CIRCLE_SEGMENTS 512
    DISTRHO_SAFE_ASSERT_RETURN(numSegments >= 3 && size > 0.0f,);
    DISTRHO_SAFE_ASSERT_RETURN(numSegments <= MAX_CIRCLE_SEGMENTS,);

    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    const double origx = static_cast<double>(pos.getX());
    const double origy = static_cast<double>(pos.getY());
    double t;
    double x = size;
    double y = 0.0;

    GLfloat vertices[(MAX_CIRCLE_SEGMENTS + 1) * 2];
    for (uint i = 0; i < numSegments; ++i)
    {
        vertices[i * 2 + 0] = ((x + origx) / gl3context.width) * 2 - 1;
        vertices[i * 2 + 1] = ((y + origy) / gl3context.height) * -2 + 1;

        t = x;
        x = cos * x - sin * y;
        y = sin * t + cos * y;
    }

    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (numSegments + 1), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);

    if (outline)
    {
        GLushort order[MAX_CIRCLE_SEGMENTS * 2];
        for (uint i = 0; i < numSegments; ++i)
        {
            order[i * 2 + 0] = i;
            order[i * 2 + 1] = i + 1;
        }
        order[numSegments * 2 - 1] = 0;

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * numSegments * 2, order, GL_DYNAMIC_DRAW);
        glDrawElements(GL_LINES, numSegments * 2, GL_UNSIGNED_SHORT, nullptr);
    }
    else
    {
        // center position
        vertices[numSegments * 2 + 0] = (origx / gl3context.width) * 2 - 1;
        vertices[numSegments * 2 + 1] = (origy / gl3context.height) * -2 + 1;

        GLushort order[MAX_CIRCLE_SEGMENTS * 3];
        for (uint i = 0; i < numSegments; ++i)
        {
            order[i * 3 + 0] = i;
            order[i * 3 + 1] = i + 1;
            order[i * 3 + 2] = numSegments;
        }
        order[numSegments * 3 - 2] = 0;

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * numSegments * 3, order, GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, numSegments * 3, GL_UNSIGNED_SHORT, nullptr);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(gl3context.bounds);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    notImplemented("Circle::draw");
}

template<typename T>
void Circle<T>::drawOutline()
{
    notImplemented("Circle::drawOutline");
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Triangle

template<typename T>
static void drawTriangle(const GraphicsContext& context,
                         const Point<T>& pos1,
                         const Point<T>& pos2,
                         const Point<T>& pos3,
                         const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(pos1 != pos2 && pos1 != pos3,);

    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    const GLfloat x1 = (static_cast<double>(pos1.getX()) / gl3context.width) * 2 - 1;
    const GLfloat y1 = (static_cast<double>(pos1.getY()) / gl3context.height) * -2 + 1;
    const GLfloat x2 = (static_cast<double>(pos2.getX()) / gl3context.width) * 2 - 1;
    const GLfloat y2 = (static_cast<double>(pos2.getY()) / gl3context.height) * -2 + 1;
    const GLfloat x3 = (static_cast<double>(pos3.getX()) / gl3context.width) * 2 - 1;
    const GLfloat y3 = (static_cast<double>(pos3.getY()) / gl3context.height) * -2 + 1;

    const GLfloat vertices[] = { x1, y1, x2, y2, x3, y3 };

    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    if (outline)
    {
        static constexpr const GLubyte order[] = { 0, 1, 1, 2, 2, 0 };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);
        glDrawElements(GL_LINES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glDisableVertexAttribArray(gl3context.bounds);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename T>
void Triangle<T>::draw(const GraphicsContext& context)
{
    drawTriangle<T>(context, pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(static_cast<GLfloat>(lineWidth));
    drawTriangle<T>(context, pos1, pos2, pos3, true);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Triangle<T>::draw()
{
    notImplemented("Triangle::draw");
}

template<typename T>
void Triangle<T>::drawOutline()
{
    notImplemented("Triangle::drawOutline");
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Rectangle

template<typename T>
static void drawRectangle(const GraphicsContext& context, const Rectangle<T>& rect, const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(rect.isValid(),);

    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    const GLfloat x = (static_cast<double>(rect.getX()) / gl3context.width) * 2 - 1;
    const GLfloat y = (static_cast<double>(rect.getY()) / gl3context.height) * -2 + 1;
    const GLfloat w = (static_cast<double>(rect.getWidth()) / gl3context.width) * 2;
    const GLfloat h = (static_cast<double>(rect.getHeight()) / gl3context.height) * -2;

    const GLfloat vertices[] = { x, y, x, y + h, x + w, y + h, x + w, y };

    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);

    if (outline)
    {
        static constexpr const GLubyte order[] = { 0, 1, 1, 2, 2, 3, 3, 0 };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);
        glDrawElements(GL_LINES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);
    }
    else
    {
        static constexpr const GLubyte order[] = { 0, 1, 2, 0, 2, 3 };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);
        glDrawElements(GL_TRIANGLES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(gl3context.bounds);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename T>
void Rectangle<T>::draw(const GraphicsContext& context)
{
    drawRectangle<T>(context, *this, false);
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(static_cast<GLfloat>(lineWidth));
    drawRectangle<T>(context, *this, true);
}

#if DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Rectangle<T>::draw()
{
    notImplemented("Rectangle::draw");
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    notImplemented("Rectangle::drawOutline");
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// OpenGLImage

static void setupOpenGLImage(const OpenGLImage& image, GLuint textureId)
{
    DISTRHO_SAFE_ASSERT_RETURN(image.isValid(),);

    const ImageFormat imageFormat = image.getFormat();
    GLint intformat;

   #ifdef DGL_USE_GLES
    // GLES does not support BGR
    DISTRHO_SAFE_ASSERT_RETURN(imageFormat != kImageFormatBGR && imageFormat != kImageFormatBGRA,);
   #endif

    glBindTexture(GL_TEXTURE_2D, textureId);

    switch (imageFormat)
    {
    case kImageFormatBGR:
    case kImageFormatRGB:
        intformat = GL_RGB;
        break;
    case kImageFormatGrayscale:
       #ifdef DGL_USE_GLES2
        intformat = GL_LUMINANCE;
       #else
        intformat = GL_RED;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
       #endif
        break;
    default:
        intformat = GL_RGBA;
        break;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   #ifndef DGL_USE_GLES
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);
   #endif

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 intformat,
                 static_cast<GLsizei>(image.getWidth()),
                 static_cast<GLsizei>(image.getHeight()),
                 0,
                 asOpenGLImageFormat(imageFormat),
                 GL_UNSIGNED_BYTE,
                 image.getRawData());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLImage::drawAt(const GraphicsContext& context, const Point<int>& pos)
{
    if (textureId == 0 || isInvalid())
        return;

    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(context);

    if (gl3context.program == 0)
        return;

    if (! setupCalled)
    {
        setupOpenGLImage(*this, textureId);
        setupCalled = true;
    }

    const GLfloat x = (static_cast<double>(pos.getX()) / gl3context.width) * 2 - 1;
    const GLfloat y = (static_cast<double>(pos.getY()) / gl3context.height) * -2 + 1;
    const GLfloat w = (static_cast<double>(getWidth()) / gl3context.width) * 2;
    const GLfloat h = (static_cast<double>(getHeight()) / gl3context.height) * -2;

    const GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glUniform4fv(gl3context.color, 1, color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(gl3context.usingTexture, 1);

    const GLfloat vertices[] = {
        x, y, x, y + h, x + w, y + h, x + w, y,
        0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glEnableVertexAttribArray(gl3context.textureMap);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(gl3context.textureMap, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(sizeof(GLfloat) * 8));

    static constexpr const GLubyte order[] = { 0, 1, 2, 0, 2, 3 };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(gl3context.textureMap);
    glDisableVertexAttribArray(gl3context.bounds);
    glUniform1i(gl3context.usingTexture, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

#ifdef DGL_USE_GLES
ImageFormat OpenGLImage::getFormat() const noexcept
{
    switch (format)
    {
    case kImageFormatBGR:
        return kImageFormatRGB;
    case kImageFormatBGRA:
        return kImageFormatRGBA;
    default:
        return format;
    }
}

const char* OpenGLImage::getRawData() const noexcept
{
    if (rawDataLast != rawData)
    {
        if (format == kImageFormatBGR || format == kImageFormatBGRA)
        {
            const uint w = size.getWidth();
            const uint h = size.getHeight();

            std::free(convertedData);

            if (format == kImageFormatBGR)
            {
                convertedData = static_cast<char*>(std::malloc(sizeof(char) * 3 * w * h));
                for (int i = 0; i < w * h; ++i)
                {
                    convertedData[i * 3 + 0] = rawData[i * 3 + 2];
                    convertedData[i * 3 + 1] = rawData[i * 3 + 1];
                    convertedData[i * 3 + 2] = rawData[i * 3 + 0];
                }
            }
            else
            {
                convertedData = static_cast<char*>(std::malloc(sizeof(char) * 4 * w * h));
                for (int i = 0; i < w * h; ++i)
                {
                    convertedData[i * 4 + 0] = rawData[i * 4 + 2];
                    convertedData[i * 4 + 1] = rawData[i * 4 + 1];
                    convertedData[i * 4 + 2] = rawData[i * 4 + 0];
                    convertedData[i * 4 + 3] = rawData[i * 4 + 3];
                }
            }
        }
        else
        {
            std::free(convertedData);
            convertedData = nullptr;
        }

        rawDataLast = rawData;
    }

    return
     #ifdef DGL_USE_GLES
      convertedData != nullptr ? convertedData :
     #endif
      rawData;
}
#endif

#if DGL_ALLOW_DEPRECATED_METHODS
void OpenGLImage::draw()
{
    notImplemented("OpenGLImage::draw");
}

void OpenGLImage::drawAt(int, int)
{
    notImplemented("OpenGLImage::drawAt");
}

void OpenGLImage::drawAt(const Point<int>&)
{
    notImplemented("OpenGLImage::drawAt");
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
    const OpenGL3GraphicsContext& gl3context = static_cast<const OpenGL3GraphicsContext&>(getGraphicsContext());

    if (gl3context.program == 0)
        return;

    const ImageFormat imageFormat = pData->image.getFormat();
    const float normValue = getNormalizedValue();

   #ifdef DGL_USE_GLES
    // GLES does not support BGR
    DISTRHO_SAFE_ASSERT_RETURN(imageFormat != kImageFormatBGR && imageFormat != kImageFormatBGRA,);
   #endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glUniform4fv(gl3context.color, 1, color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pData->glTextureId);
    glUniform1i(gl3context.usingTexture, 1);

    if (! pData->isReady)
    {
        GLint intformat;

        switch (imageFormat)
        {
        case kImageFormatBGR:
        case kImageFormatRGB:
            intformat = GL_RGB;
            break;
        case kImageFormatGrayscale:
           #ifdef DGL_USE_GLES2
            intformat = GL_LUMINANCE;
           #else
            intformat = GL_RED;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
           #endif
            break;
        default:
            intformat = GL_RGBA;
            break;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       #ifndef DGL_USE_GLES
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);
       #endif

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
            const uint layerDataSize   = v1 * v2 * ((imageFormat == kImageFormatBGRA ||
                                                     imageFormat == kImageFormatRGBA) ? 4 : 3);
            /*      */ imageDataOffset = layerDataSize * uint(normValue * float(pData->imgLayerCount - 1));
        }

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     intformat,
                     static_cast<GLsizei>(getWidth()),
                     static_cast<GLsizei>(getHeight()),
                     0,
                     asOpenGLImageFormat(imageFormat),
                     GL_UNSIGNED_BYTE,
                     pData->image.getRawData() + imageDataOffset);

        pData->isReady = true;
    }

    GLfloat x, y, w, h;

    if (pData->rotationAngle != 0)
    {
        // TODO
        x = -1;
        y = 1;
        w = (static_cast<double>(getWidth()) / gl3context.width) * 2;
        h = (static_cast<double>(getHeight()) / gl3context.height) * -2;
    }
    else
    {
        x = -1;
        y = 1;
        w = (static_cast<double>(getWidth()) / gl3context.width) * 2;
        h = (static_cast<double>(getHeight()) / gl3context.height) * -2;
    }

    const GLfloat vertices[] = {
        x, y, x, y + h, x + w, y + h, x + w, y,
        0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, gl3context.buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(gl3context.bounds);
    glEnableVertexAttribArray(gl3context.textureMap);
    glVertexAttribPointer(gl3context.bounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glVertexAttribPointer(gl3context.textureMap, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(sizeof(GLfloat) * 8));

    static constexpr const GLubyte order[] = { 0, 1, 2, 0, 2, 3 };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3context.buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(gl3context.textureMap);
    glDisableVertexAttribArray(gl3context.bounds);
    glUniform1i(gl3context.usingTexture, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
}

template class ImageBaseKnob<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseSlider

template class ImageBaseSlider<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------
// ImageBaseSwitch

template class ImageBaseSwitch<OpenGLImage>;

// --------------------------------------------------------------------------------------------------------------------

static void shaderCreationFail(const GLuint shaderErr, const GLuint shader2 = 0)
{
    if (shaderErr != 0)
    {
        GLint len = 0;
        glGetShaderiv(shaderErr, GL_INFO_LOG_LENGTH, &len);

        std::vector<GLchar> errorLog(len);
        glGetShaderInfoLog(shaderErr, len, &len, errorLog.data());

        d_stderr2("OpenGL3 shader compilation error: %s", errorLog.data());

        glDeleteShader(shaderErr);
    }

    glDeleteShader(shader2);
}

static void contextCreationFail(const GLuint program, const GLuint shader1, const GLuint shader2)
{
    glDeleteProgram(program);
    glDeleteShader(shader1);
    glDeleteShader(shader2);
}

void Window::PrivateData::createContextIfNeeded()
{
    OpenGL3GraphicsContext& gl3context = reinterpret_cast<OpenGL3GraphicsContext&>(graphicsContext);

    if (gl3context.program != 0)
        return;

#if defined(DISTRHO_OS_WINDOWS)
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
    static bool needsInit = true;
# define DGL_EXT(PROC, func) \
      if (needsInit) func = (PROC) wglGetProcAddress ( #func ); \
      DISTRHO_SAFE_ASSERT_RETURN(func != nullptr,);
# define DGL_EXT2(PROC, func, fallback) \
      if (needsInit) { \
        func = (PROC) wglGetProcAddress ( #func ); \
        if (func == nullptr) func = (PROC) wglGetProcAddress ( #fallback ); \
      } DISTRHO_SAFE_ASSERT_RETURN(func != nullptr,);
DGL_EXT(PFNGLACTIVETEXTUREPROC,            glActiveTexture)
DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
DGL_EXT(PFNGLGETATTRIBLOCATIONPROC,        glGetAttribLocation)
DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
DGL_EXT(PFNGLUNIFORM4FVPROC,               glUniform4fv)
DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
# undef DGL_EXT
# undef DGL_EXT2
    needsInit = false;
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#endif

    int status;

    const GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    DISTRHO_SAFE_ASSERT_RETURN(fragment != 0, shaderCreationFail(fragment));

    const GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    DISTRHO_SAFE_ASSERT_RETURN(vertex != 0, shaderCreationFail(vertex, fragment));

    const GLuint program = glCreateProgram();
    DISTRHO_SAFE_ASSERT_RETURN(program != 0,);

   #if defined(DGL_USE_GLES2)
    #define DGL_SHADER_HEADER "#version 100\n"
   #elif defined(DGL_USE_GLES3)
    #define DGL_SHADER_HEADER "#version 300 es\n"
   #else
    #define DGL_SHADER_HEADER "#version 150 core\n"
   #endif

    {
        static constexpr const char* const src = DGL_SHADER_HEADER
            "precision mediump float;"
            "uniform vec4 color;"
            "uniform sampler2D stex;"
            "uniform bool texok;"
           #ifdef DGL_USE_GLES3
            "in vec2 vtex;"
            "out vec4 FragColor;"
            "void main() { FragColor = texok ? texture(stex, vtex) : color; }";
           #else
            "varying vec2 vtex;"
            "void main() { gl_FragColor = texok ? texture2D(stex, vtex) : color; }";
           #endif

        glShaderSource(fragment, 1, &src, nullptr);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
        DISTRHO_SAFE_ASSERT_RETURN(status != 0, contextCreationFail(program, fragment, vertex));
    }

    {
        static constexpr const char* const src = DGL_SHADER_HEADER
           #ifdef DGL_USE_GLES3
            "in vec4 pos;"
            "in vec2 tex;"
            "out vec2 vtex;"
           #else
            "attribute vec4 pos;"
            "attribute vec2 tex;"
            "varying vec2 vtex;"
           #endif
            "void main() { gl_Position = pos; vtex = tex; }";

        glShaderSource(vertex, 1, &src, nullptr);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
        DISTRHO_SAFE_ASSERT_RETURN(status != 0, contextCreationFail(program, fragment, vertex));
    }

    glAttachShader(program, fragment);
    glAttachShader(program, vertex);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    DISTRHO_SAFE_ASSERT_RETURN(status != 0, contextCreationFail(program, fragment, vertex));

    glGenBuffers(2, gl3context.buffers);
    DISTRHO_SAFE_ASSERT_RETURN(gl3context.buffers[0] != 0, contextCreationFail(program, fragment, vertex));
    DISTRHO_SAFE_ASSERT_RETURN(gl3context.buffers[1] != 0, contextCreationFail(program, fragment, vertex));

    glDeleteShader(fragment);
    glDeleteShader(vertex);

    gl3context.program = program;
    gl3context.color = glGetUniformLocation(gl3context.program, "color");
    gl3context.usingTexture = glGetUniformLocation(gl3context.program, "texok");
    gl3context.bounds = glGetAttribLocation(gl3context.program, "pos");
    gl3context.textureMap = glGetAttribLocation(gl3context.program, "tex");
}

void Window::PrivateData::destroyContext()
{
    OpenGL3GraphicsContext& gl3context = reinterpret_cast<OpenGL3GraphicsContext&>(graphicsContext);

    if (gl3context.program == 0)
        return;

    glDeleteBuffers(2, gl3context.buffers);
    glDeleteProgram(gl3context.program);
    gl3context.program = 0;
}

void Window::PrivateData::startContext()
{
    OpenGL3GraphicsContext& gl3context = reinterpret_cast<OpenGL3GraphicsContext&>(graphicsContext);
    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);

    gl3context.width = size.width;
    gl3context.height = size.height;
    glUseProgram(gl3context.program);

    glViewport(0, 0, static_cast<GLsizei>(size.width), static_cast<GLsizei>(size.height));
}

void Window::PrivateData::endContext()
{
    glUseProgram(0);
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
