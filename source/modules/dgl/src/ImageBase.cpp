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

#include "../ImageBase.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// protected constructors

ImageBase::ImageBase()
    : rawData(nullptr),
      size(0, 0),
      format(kImageFormatNull) {}

ImageBase::ImageBase(const char* const rdata, const uint width, const uint height, const ImageFormat fmt)
  : rawData(rdata),
    size(width, height),
    format(fmt) {}

ImageBase::ImageBase(const char* const rdata, const Size<uint>& s, const ImageFormat fmt)
  : rawData(rdata),
    size(s),
    format(fmt) {}

ImageBase::ImageBase(const ImageBase& image)
  : rawData(image.rawData),
    size(image.size),
    format(image.format) {}

// --------------------------------------------------------------------------------------------------------------------
// public methods

ImageBase::~ImageBase() {}

bool ImageBase::isValid() const noexcept
{
    return (rawData != nullptr && size.isValid());
}

bool ImageBase::isInvalid() const noexcept
{
    return (rawData == nullptr || size.isInvalid());
}

uint ImageBase::getWidth() const noexcept
{
    return size.getWidth();
}

uint ImageBase::getHeight() const noexcept
{
    return size.getHeight();
}

const Size<uint>& ImageBase::getSize() const noexcept
{
    return size;
}

const char* ImageBase::getRawData() const noexcept
{
    return rawData;
}

ImageFormat ImageBase::getFormat() const noexcept
{
    return format;
}

void ImageBase::loadFromMemory(const char* const rdata,
                               const uint width,
                               const uint height,
                               const ImageFormat fmt) noexcept
{
    loadFromMemory(rdata, Size<uint>(width, height), fmt);
}

void ImageBase::loadFromMemory(const char* const rdata, const Size<uint>& s, const ImageFormat fmt) noexcept
{
    rawData = rdata;
    size    = s;
    format  = fmt;
}

void ImageBase::draw(const GraphicsContext& context)
{
    drawAt(context, Point<int>(0, 0));
}

void ImageBase::drawAt(const GraphicsContext& context, const int x, const int y)
{
    drawAt(context, Point<int>(x, y));
}

// --------------------------------------------------------------------------------------------------------------------
// public operators

ImageBase& ImageBase::operator=(const ImageBase& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    return *this;
}

bool ImageBase::operator==(const ImageBase& image) const noexcept
{
    return (rawData == image.rawData && size == image.size && format == image.format);
}

bool ImageBase::operator!=(const ImageBase& image) const noexcept
{
    return !operator==(image);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
