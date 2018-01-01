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

#include "../Image.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Image::Image()
    : fRawData(nullptr),
      fSize(0, 0),
      fFormat(0),
      fType(0),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

Image::Image(const char* const rawData, const uint width, const uint height, const GLenum format, const GLenum type)
    : fRawData(rawData),
      fSize(width, height),
      fFormat(format),
      fType(type),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

Image::Image(const char* const rawData, const Size<uint>& size, const GLenum format, const GLenum type)
    : fRawData(rawData),
      fSize(size),
      fFormat(format),
      fType(type),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

Image::Image(const Image& image)
    : fRawData(image.fRawData),
      fSize(image.fSize),
      fFormat(image.fFormat),
      fType(image.fType),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

Image::~Image()
{
    if (fTextureId != 0)
    {
#ifndef DISTRHO_OS_MAC // FIXME
        glDeleteTextures(1, &fTextureId);
#endif
        fTextureId = 0;
    }
}

void Image::loadFromMemory(const char* const rawData, const uint width, const uint height, const GLenum format, const GLenum type) noexcept
{
    loadFromMemory(rawData, Size<uint>(width, height), format, type);
}

void Image::loadFromMemory(const char* const rawData, const Size<uint>& size, const GLenum format, const GLenum type) noexcept
{
    fRawData = rawData;
    fSize    = size;
    fFormat  = format;
    fType    = type;
    fIsReady = false;
}

bool Image::isValid() const noexcept
{
    return (fRawData != nullptr && fSize.getWidth() > 0 && fSize.getHeight() > 0);
}

uint Image::getWidth() const noexcept
{
    return fSize.getWidth();
}

uint Image::getHeight() const noexcept
{
    return fSize.getHeight();
}

const Size<uint>& Image::getSize() const noexcept
{
    return fSize;
}

const char* Image::getRawData() const noexcept
{
    return fRawData;
}

GLenum Image::getFormat() const noexcept
{
    return fFormat;
}

GLenum Image::getType() const noexcept
{
    return fType;
}

void Image::draw()
{
    drawAt(0, 0);
}

void Image::drawAt(const int x, const int y)
{
    drawAt(Point<int>(x, y));
}

void Image::drawAt(const Point<int>& pos)
{
    if (fTextureId == 0 || ! isValid())
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

    if (! fIsReady)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(fSize.getWidth()), static_cast<GLsizei>(fSize.getHeight()), 0,
                     fFormat, fType, fRawData);

        fIsReady = true;
    }

    Rectangle<int>(pos, static_cast<int>(fSize.getWidth()), static_cast<int>(fSize.getHeight())).draw();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

// -----------------------------------------------------------------------

Image& Image::operator=(const Image& image) noexcept
{
    fRawData = image.fRawData;
    fSize    = image.fSize;
    fFormat  = image.fFormat;
    fType    = image.fType;
    fIsReady = false;
    return *this;
}

bool Image::operator==(const Image& image) const noexcept
{
    return (fRawData == image.fRawData && fSize == image.fSize);
}

bool Image::operator!=(const Image& image) const noexcept
{
    return !operator==(image);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
