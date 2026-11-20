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

#ifndef DGL_OPENGL_HPP_INCLUDED
#define DGL_OPENGL_HPP_INCLUDED

#include "ImageBase.hpp"
#include "ImageBaseWidgets.hpp"

#include "OpenGL-include.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

#ifdef DGL_USE_OPENGL3
/**
   OpenGL3 Graphics context.

   This provides access to the program, shaders and uniforms used by the underlying DPF implementation.
 */
struct OpenGL3GraphicsContext : GraphicsContext
{
   /**
      The OpenGL3 program used for this context.
      It is activated automatically before any widget onDisplay() is called.
      If changing the current OpenGL program make sure to revert back to this one at the end of your pipeline.

      @code
      // use custom program
      glUseProgram(context.program);

      // custom stuff here

      // revert back
      glUseProgram(context.program);
      @endcode
    */
    GLuint program;

   /**
      A vec4 uniform used to set the next drawing color.

      @code
      const GLfloat color[4] = { red, green, blue, alpha };
      glUniform4fv(context.color, 1, color);
      @endcode
    */
    GLuint color;

   /**
      A vertex shader attribute directly linked to gl_Position.
      Use this to set the bounds for drawing, normalized as -1.0 to +1.0.
      The @a width and @a height provide the total window size for convenience.

      @code
      const GLfloat triangle[] = { x1, y1, x2, y2, x3, y3 };
      glEnableVertexAttribArray(context.bounds);
      glVertexAttribPointer(context.bounds, 2, GL_FLOAT, GL_FALSE, 0, triangle);
      @endcode
    */
    GLuint bounds;

   /**
      A vertex shader attribute directly linked to GL_TEXTURE0 map.
      // TODO find the correct wording, map??.

      @code
      const GLfloat map[] = { 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f };
      glEnableVertexAttribArray(context.textureMap);
      glVertexAttribPointer(context.textureMap, 2, GL_FLOAT, GL_FALSE, 0, map);
      @endcode
    */
    GLuint textureMap;

   /**
      A boolean uniform used to indicate if next drawing should @a texture or @a color.
      Set to 0 for color mode, 1 for texture.
      Default mode is color, if changed make sure to revert to color mode at the end of your pipeline.

      @code
      // setup for drawing based on texture
      glUniform1i(context.usingTexture, 1);

      // bind texture
      glBindTexture(GL_TEXTURE_2D, myTextureId);
      // etc..

      // glDrawElements or similar

      // unbind texture
      glBindTexture(GL_TEXTURE_2D, 0);
      // etc..

      // revert to color mode
      glUniform1i(context.usingTexture, 0);
      @endcode
    */
    GLuint usingTexture;

   /**
      Set of buffers created with glGenBuffers.
      Used internally in DPF to draw generic shapes, can be reused in custom code.
      Unbound by default, make sure to leave them unbound at the end of your pipeline.
    */
    GLuint buffers[2];

   /**
      Total width of the window used for this context.
    */
    uint width;

   /**
      Total height of the window used for this context.
    */
    uint height;
};
#endif

// --------------------------------------------------------------------------------------------------------------------

static inline
ImageFormat asDISTRHOImageFormat(const GLenum format)
{
    switch (format)
    {
   #if defined(DGL_USE_OPENGL3) && !defined(DGL_USE_GLES2)
    case GL_RED:
   #else
    case GL_LUMINANCE:
   #endif
        return kImageFormatGrayscale;
   #ifndef DGL_USE_GLES
    case GL_BGR:
        return kImageFormatBGR;
    case GL_BGRA:
        return kImageFormatBGRA;
   #endif
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
       #if defined(DGL_USE_OPENGL3) && !defined(DGL_USE_GLES2)
        return GL_RED;
       #else
        return GL_LUMINANCE;
       #endif
    case kImageFormatBGR:
       #ifndef DGL_USE_GLES
        return GL_BGR;
       #else
        return 0;
       #endif
    case kImageFormatBGRA:
       #ifndef DGL_USE_GLES
        return GL_BGRA;
       #else
        return 0;
       #endif
    case kImageFormatRGB:
        return GL_RGB;
    case kImageFormatRGBA:
        return GL_RGBA;
    }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

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

   #ifdef DGL_USE_GLES
   /**
      Get the image format.
    */
    ImageFormat getFormat() const noexcept;

   /**
      Get the raw image data.
    */
    const char* getRawData() const noexcept;
   #endif

   /**
      TODO document this.
    */
    OpenGLImage& operator=(const OpenGLImage& image) noexcept;

    // FIXME this should not be needed
    inline void loadFromMemory(const char* rdata, uint w, uint h, ImageFormat fmt = kImageFormatBGRA)
    { loadFromMemory(rdata, Size<uint>(w, h), fmt); }
    inline void draw(const GraphicsContext& context)
    { drawAt(context, Point<int>(0, 0)); }
    inline void drawAt(const GraphicsContext& context, int x, int y)
    { drawAt(context, Point<int>(x, y)); }

   #if DGL_ALLOW_DEPRECATED_METHODS
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
   #endif // DGL_ALLOW_DEPRECATED_METHODS

private:
    bool setupCalled;
    bool textureInit;
    GLuint textureId;
   #ifdef DGL_USE_GLES
    mutable char* convertedData;
    mutable const char* rawDataLast;
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

typedef ImageBaseAboutWindow<OpenGLImage> OpenGLImageAboutWindow;
typedef ImageBaseButton<OpenGLImage> OpenGLImageButton;
typedef ImageBaseKnob<OpenGLImage> OpenGLImageKnob;
typedef ImageBaseSlider<OpenGLImage> OpenGLImageSlider;
typedef ImageBaseSwitch<OpenGLImage> OpenGLImageSwitch;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_OPENGL_HPP_INCLUDED
