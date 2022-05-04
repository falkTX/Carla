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

#ifndef DGL_OPENGL_HPP_INCLUDED
#define DGL_OPENGL_HPP_INCLUDED

#include "ImageBase.hpp"
#include "ImageBaseWidgets.hpp"

// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 1)

#undef DGL_CALLBACK_DEFINED
#undef DGL_WINGDIAPI_DEFINED

#ifdef DISTRHO_OS_WINDOWS

#ifndef APIENTRY
# define APIENTRY __stdcall
#endif // APIENTRY

/* We need WINGDIAPI defined */
#ifndef WINGDIAPI
# if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
#  define WINGDIAPI __declspec(dllimport)
# elif defined(__LCC__)
#  define WINGDIAPI __stdcall
# else
#  define WINGDIAPI extern
# endif
# define DGL_WINGDIAPI_DEFINED
#endif // WINGDIAPI

/* Some <GL/glu.h> files also need CALLBACK defined */
#ifndef CALLBACK
# if defined(_MSC_VER)
#  if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#   define CALLBACK __stdcall
#  else
#   define CALLBACK
#  endif
# else
#  define CALLBACK __stdcall
# endif
# define DGL_CALLBACK_DEFINED
#endif // CALLBACK

/* Most GL/glu.h variants on Windows need wchar_t */
#include <cstddef>

#endif // DISTRHO_OS_WINDOWS

// -----------------------------------------------------------------------
// OpenGL includes

#ifdef DISTRHO_OS_MAC
# ifdef DGL_USE_OPENGL3
#  include <OpenGL/gl3.h>
#  include <OpenGL/gl3ext.h>
# else
#  include <OpenGL/gl.h>
# endif
#else
# ifndef DISTRHO_OS_WINDOWS
#  define GL_GLEXT_PROTOTYPES
# endif
# ifndef __GLEW_H__
#  include <GL/gl.h>
#  include <GL/glext.h>
# endif
#endif

// -----------------------------------------------------------------------
// Missing OpenGL defines

#if defined(GL_BGR_EXT) && !defined(GL_BGR)
# define GL_BGR GL_BGR_EXT
#endif

#if defined(GL_BGRA_EXT) && !defined(GL_BGRA)
# define GL_BGRA GL_BGRA_EXT
#endif

#ifndef GL_CLAMP_TO_BORDER
# define GL_CLAMP_TO_BORDER 0x812D
#endif

// -----------------------------------------------------------------------
// Fix OpenGL includes for Windows, based on glfw code (part 2)

#ifdef DGL_CALLBACK_DEFINED
# undef CALLBACK
# undef DGL_CALLBACK_DEFINED
#endif

#ifdef DGL_WINGDIAPI_DEFINED
# undef WINGDIAPI
# undef DGL_WINGDIAPI_DEFINED
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   OpenGL Graphics context.
 */
struct OpenGLGraphicsContext : GraphicsContext
{
};

// -----------------------------------------------------------------------

static inline
ImageFormat asDISTRHOImageFormat(const GLenum format)
{
    switch (format)
    {
#ifdef DGL_USE_OPENGL3
    case GL_RED:
#else
    case GL_LUMINANCE:
#endif
        return kImageFormatGrayscale;
    case GL_BGR:
        return kImageFormatBGR;
    case GL_BGRA:
        return kImageFormatBGRA;
    case GL_RGB:
        return kImageFormatRGB;
    case GL_RGBA:
        return kImageFormatRGBA;
    }

    return kImageFormatNull;
}

static inline
GLenum asOpenGLImageFormat(const ImageFormat format)
{
    switch (format)
    {
    case kImageFormatNull:
        break;
    case kImageFormatGrayscale:
#ifdef DGL_USE_OPENGL3
        return GL_RED;
#else
        return GL_LUMINANCE;
#endif
    case kImageFormatBGR:
        return GL_BGR;
    case kImageFormatBGRA:
        return GL_BGRA;
    case kImageFormatRGB:
        return GL_RGB;
    case kImageFormatRGBA:
        return GL_RGBA;
    }

    return 0x0;
}

// -----------------------------------------------------------------------

/**
   OpenGL Image class.

   This is an Image class that handles raw image data in pixels.
   You can init the image data on the contructor or later on by calling loadFromMemory().

   To generate raw data useful for this class see the utils/png2rgba.py script.
   Be careful when using a PNG without alpha channel, for those the format is 'GL_BGR'
   instead of the default 'GL_BGRA'.

   Images are drawn on screen via 2D textures.
 */
class OpenGLImage : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    OpenGLImage();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    OpenGLImage(const char* rawData, uint width, uint height, ImageFormat format = kImageFormatBGRA);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    OpenGLImage(const char* rawData, const Size<uint>& size, ImageFormat format = kImageFormatBGRA);

   /**
      Constructor using another image data.
    */
    OpenGLImage(const OpenGLImage& image);

   /**
      Destructor.
    */
    ~OpenGLImage() override;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* rawData,
                        const Size<uint>& size,
                        ImageFormat format = kImageFormatBGRA) noexcept override;

   /**
      Draw this image at position @a pos using the graphics context @a context.
    */
    void drawAt(const GraphicsContext& context, const Point<int>& pos) override;

   /**
      TODO document this.
    */
    OpenGLImage& operator=(const OpenGLImage& image) noexcept;

    // FIXME this should not be needed
    inline void loadFromMemory(const char* rdata, uint w, uint h, ImageFormat fmt = kImageFormatBGRA)
    { loadFromMemory(rdata, Size<uint>(w, h), fmt); };
    inline void draw(const GraphicsContext& context)
    { drawAt(context, Point<int>(0, 0)); };
    inline void drawAt(const GraphicsContext& context, int x, int y)
    { drawAt(context, Point<int>(x, y)); };

   /**
      Constructor using raw image data, specifying an OpenGL image format.
      @note @a rawData must remain valid for the lifetime of this Image.
      DEPRECATED This constructor uses OpenGL image format instead of DISTRHO one.
    */
    DISTRHO_DEPRECATED_BY("OpenGLImage(const char*, uint, uint, ImageFormat)")
    explicit OpenGLImage(const char* rawData, uint width, uint height, GLenum glFormat);

   /**
      Constructor using raw image data, specifying an OpenGL image format.
      @note @a rawData must remain valid for the lifetime of this Image.
      DEPRECATED This constructor uses OpenGL image format instead of DISTRHO one.
    */
    DISTRHO_DEPRECATED_BY("OpenGLImage(const char*, const Size<uint>&, ImageFormat)")
    explicit OpenGLImage(const char* rawData, const Size<uint>& size, GLenum glFormat);

   /**
      Draw this image at (0, 0) point using the current OpenGL context.
      DEPRECATED This function does not take into consideration the current graphics context and only works in OpenGL.
    */
    DISTRHO_DEPRECATED_BY("draw(const GraphicsContext&)")
    void draw();

   /**
      Draw this image at (x, y) point using the current OpenGL context.
      DEPRECATED This function does not take into consideration the current graphics context and only works in OpenGL.
    */
    DISTRHO_DEPRECATED_BY("drawAt(const GraphicsContext&, int, int)")
    void drawAt(int x, int y);

   /**
      Draw this image at position @a pos using the current OpenGL context.
      DEPRECATED This function does not take into consideration the current graphics context and only works in OpenGL.
    */
    DISTRHO_DEPRECATED_BY("drawAt(const GraphicsContext&, const Point<int>&)")
    void drawAt(const Point<int>& pos);

   /**
      Get the image type.
      DEPRECATED Type is always assumed to be GL_UNSIGNED_BYTE.
    */
    DISTRHO_DEPRECATED
    GLenum getType() const noexcept { return GL_UNSIGNED_BYTE; }

private:
    GLuint textureId;
    bool setupCalled;
};

// -----------------------------------------------------------------------

typedef ImageBaseAboutWindow<OpenGLImage> OpenGLImageAboutWindow;
typedef ImageBaseButton<OpenGLImage> OpenGLImageButton;
typedef ImageBaseKnob<OpenGLImage> OpenGLImageKnob;
typedef ImageBaseSlider<OpenGLImage> OpenGLImageSlider;
typedef ImageBaseSwitch<OpenGLImage> OpenGLImageSwitch;

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
