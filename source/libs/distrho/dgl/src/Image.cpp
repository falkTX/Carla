/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#include "../Image.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

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

void Image::draw()
{
    draw(0, 0);
}

void Image::draw(int x, int y)
{
    if (! isValid())
        return;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glRasterPos2i(x, fSize.getHeight()+y);
    glDrawPixels(fSize.getWidth(), fSize.getHeight(), fFormat, fType, fRawData);
}

void Image::draw(const Point<int>& pos)
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

// -------------------------------------------------

END_NAMESPACE_DGL
