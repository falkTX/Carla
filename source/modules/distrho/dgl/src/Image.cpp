/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
      fType(0)
{
}

Image::Image(const char* rawData, int width, int height, GLenum format, GLenum type)
    : fRawData(rawData),
      fSize(width, height),
      fFormat(format),
      fType(type)
{
}

Image::Image(const char* rawData, const Size<int>& size, GLenum format, GLenum type)
    : fRawData(rawData),
      fSize(size),
      fFormat(format),
      fType(type)
{
}

Image::Image(const Image& image)
    : fRawData(image.fRawData),
      fSize(image.fSize),
      fFormat(image.fFormat),
      fType(image.fType)
{
}

void Image::loadFromMemory(const char* rawData, int width, int height, GLenum format, GLenum type)
{
    loadFromMemory(rawData, Size<int>(width, height), format, type);
}

void Image::loadFromMemory(const char* rawData, const Size<int>& size, GLenum format, GLenum type)
{
    fRawData = rawData;
    fSize    = size;
    fFormat  = format;
    fType    = type;
}

bool Image::isValid() const
{
    return (fRawData != nullptr && getWidth() > 0 && getHeight() > 0);
}

int Image::getWidth() const
{
    return fSize.getWidth();
}

int Image::getHeight() const
{
    return fSize.getHeight();
}

const Size<int>& Image::getSize() const
{
    return fSize;
}

const char* Image::getRawData() const
{
    return fRawData;
}

GLenum Image::getFormat() const
{
    return fFormat;
}

GLenum Image::getType() const
{
    return fType;
}

void Image::draw() const
{
    draw(0, 0);
}

void Image::draw(int x, int y) const
{
    if (! isValid())
        return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glRasterPos2i(x, fSize.getHeight()+y);
    glDrawPixels(fSize.getWidth(), fSize.getHeight(), fFormat, fType, fRawData);
}

void Image::draw(const Point<int>& pos) const
{
    draw(pos.getX(), pos.getY());
}

Image& Image::operator=(const Image& image)
{
    fRawData = image.fRawData;
    fSize    = image.fSize;
    fFormat  = image.fFormat;
    fType    = image.fType;
    return *this;
}

bool Image::operator==(const Image& image) const
{
    return (fRawData == image.fRawData);
}

bool Image::operator!=(const Image& image) const
{
    return (fRawData != image.fRawData);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
