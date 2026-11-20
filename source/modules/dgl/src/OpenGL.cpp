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
#include "../ImageWidgets.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// OpenGLImage

OpenGLImage::OpenGLImage()
    : ImageBase(),
      setupCalled(false),
      textureInit(false),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
}

OpenGLImage::OpenGLImage(const char* const rdata, const uint w, const uint h, const ImageFormat fmt)
    : ImageBase(rdata, w, h, fmt),
      setupCalled(false),
      textureInit(true),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const char* const rdata, const Size<uint>& s, const ImageFormat fmt)
    : ImageBase(rdata, s, fmt),
      setupCalled(false),
      textureInit(true),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const OpenGLImage& image)
    : ImageBase(image),
      setupCalled(false),
      textureInit(true),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::~OpenGLImage()
{
    if (textureId != 0)
        glDeleteTextures(1, &textureId);

   #ifdef DGL_USE_GLES
    std::free(convertedData);
   #endif
}

void OpenGLImage::loadFromMemory(const char* const rdata, const Size<uint>& s, const ImageFormat fmt) noexcept
{
    if (!textureInit)
    {
        textureInit = true;
        glGenTextures(1, &textureId);
        DISTRHO_SAFE_ASSERT(textureId != 0);
    }
    setupCalled = false;
    ImageBase::loadFromMemory(rdata, s, fmt);
}

OpenGLImage& OpenGLImage::operator=(const OpenGLImage& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    setupCalled = false;

    if (image.isValid() && !textureInit)
    {
        textureInit = true;
        glGenTextures(1, &textureId);
        DISTRHO_SAFE_ASSERT(textureId != 0);
    }

    return *this;
}

#if DGL_ALLOW_DEPRECATED_METHODS
OpenGLImage::OpenGLImage(const char* const rdata, const uint w, const uint h, const GLenum fmt)
    : ImageBase(rdata, w, h, asDISTRHOImageFormat(fmt)),
      setupCalled(false),
      textureInit(true),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const char* const rdata, const Size<uint>& s, const GLenum fmt)
    : ImageBase(rdata, s, asDISTRHOImageFormat(fmt)),
      setupCalled(false),
      textureInit(true),
      textureId(0)
   #ifdef DGL_USE_GLES
    , convertedData(nullptr)
    , rawDataLast(nullptr)
    #endif
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    if (skipDrawing)
        return;

    bool needsDisableScissor = false;

    if (needsViewportScaling)
    {
        // limit viewport to widget bounds
        const int x = absolutePos.getX();
        const int w = static_cast<int>(self->getWidth());
        const int h = static_cast<int>(self->getHeight());

        if (d_isNotZero(viewportScaleFactor) && d_isNotEqual(viewportScaleFactor, 1.0))
        {
            glViewport(x,
                       -d_roundToIntPositive(height * viewportScaleFactor - height + absolutePos.getY()),
                       d_roundToIntPositive(width * viewportScaleFactor),
                       d_roundToIntPositive(height * viewportScaleFactor));
        }
        else
        {
            const int y = static_cast<int>(height - self->getHeight()) - absolutePos.getY();
            glViewport(x, y, w, h);
        }
    }
    else if (needsFullViewportForDrawing || (absolutePos.isZero() && self->getSize() == Size<uint>(width, height)))
    {
        // full viewport size
        glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    }
    else
    {
        // set viewport pos
        glViewport(d_roundToIntPositive(absolutePos.getX() * autoScaleFactor),
                   -d_roundToIntPositive(absolutePos.getY() * autoScaleFactor),
                   static_cast<int>(width),
                   static_cast<int>(height));

        // then cut the outer bounds
        glScissor(d_roundToIntPositive(absolutePos.getX() * autoScaleFactor),
                  d_roundToIntPositive(height - (static_cast<int>(self->getHeight()) + absolutePos.getY()) * autoScaleFactor),
                  d_roundToIntPositive(self->getWidth() * autoScaleFactor),
                  d_roundToIntPositive(self->getHeight() * autoScaleFactor));

        glEnable(GL_SCISSOR_TEST);
        needsDisableScissor = true;
    }

    // display widget
    self->onDisplay();

    if (needsDisableScissor)
        glDisable(GL_SCISSOR_TEST);

    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// --------------------------------------------------------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    if (! selfw->pData->visible)
        return;

    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    // full viewport size
    glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));

    // main widget drawing
    self->onDisplay();

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, window.pData->autoScaleFactor);
}

// --------------------------------------------------------------------------------------------------------------------

void Window::PrivateData::renderToPicture(const char* const filename,
                                          const GraphicsContext&,
                                          const uint width,
                                          const uint height)
{
    FILE* const f = fopen(filename, "w");
    DISTRHO_SAFE_ASSERT_RETURN(f != nullptr,);

    GLubyte* const pixels = new GLubyte[width * height * 3 * sizeof(GLubyte)];

    glFlush();
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGB, GL_UNSIGNED_BYTE, pixels);

    fprintf(f, "P3\n%d %d\n255\n", width, height);
    for (uint y = 0; y < height; y++)
    {
        for (uint i, x = 0; x < width; x++)
        {
            i = 3 * ((height - y - 1) * width + x);
            fprintf(f, "%3d %3d %3d ", pixels[i], pixels[i+1], pixels[i+2]);
        }
        fprintf(f, "\n");
    }

    delete[] pixels;
    fclose(f);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
