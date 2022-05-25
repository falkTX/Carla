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

#ifndef DGL_IMAGE_BASE_HPP_INCLUDED
#define DGL_IMAGE_BASE_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

enum ImageFormat {
    kImageFormatNull,
    kImageFormatGrayscale,
    kImageFormatBGR,
    kImageFormatBGRA,
    kImageFormatRGB,
    kImageFormatRGBA,
};

/**
   Base DGL Image class.

   This is an Image class that handles raw image data in pixels.
   It is an abstract class that provides the common methods to build on top.
   Cairo and OpenGL Image classes are based upon this one.

   @see CairoImage, OpenGLImage
 */
class ImageBase
{
protected:
   /**
      Constructor for a null Image.
    */
    ImageBase();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageBase(const char* rawData, uint width, uint height, ImageFormat format);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageBase(const char* rawData, const Size<uint>& size, ImageFormat format);

   /**
      Constructor using another image data.
    */
    ImageBase(const ImageBase& image);

public:
   /**
      Destructor.
    */
    virtual ~ImageBase();

   /**
      Check if this image is valid.
    */
    bool isValid() const noexcept;

   /**
      Check if this image is not valid.
    */
    bool isInvalid() const noexcept;

   /**
      Get width.
    */
    uint getWidth() const noexcept;

   /**
      Get height.
    */
    uint getHeight() const noexcept;

   /**
      Get size.
    */
    const Size<uint>& getSize() const noexcept;

   /**
      Get the raw image data.
    */
    const char* getRawData() const noexcept;

   /**
      Get the image format.
    */
    ImageFormat getFormat() const noexcept;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* rawData, uint width, uint height, ImageFormat format = kImageFormatBGRA) noexcept;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    virtual void loadFromMemory(const char* rawData,
                                const Size<uint>& size,
                                ImageFormat format = kImageFormatBGRA) noexcept;

   /**
      Draw this image at (0, 0) point using the current OpenGL context.
    */
    void draw(const GraphicsContext& context);

   /**
      Draw this image at (x, y) point using the current OpenGL context.
    */
    void drawAt(const GraphicsContext& context, int x, int y);

   /**
      Draw this image at position @a pos using the current OpenGL context.
    */
    virtual void drawAt(const GraphicsContext& context, const Point<int>& pos) = 0;

   /**
      TODO document this.
    */
    ImageBase& operator=(const ImageBase& image) noexcept;
    bool operator==(const ImageBase& image) const noexcept;
    bool operator!=(const ImageBase& image) const noexcept;

protected:
    const char* rawData;
    Size<uint> size;
    ImageFormat format;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_HPP_INCLUDED
