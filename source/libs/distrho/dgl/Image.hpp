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

#ifndef __DGL_IMAGE_HPP__
#define __DGL_IMAGE_HPP__

#include "Geometry.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

class Image
{
public:
    Image();
    Image(const char* rawData, int width, int height, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);
    Image(const char* rawData, const Size<int>& size, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);
    Image(const Image& image);

    void loadFromMemory(const char* rawData, int width, int height, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);
    void loadFromMemory(const char* rawData, const Size<int>& size, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE);

    bool isValid() const;

    int getWidth() const;
    int getHeight() const;
    const Size<int>& getSize() const;

    const char* getRawData() const;
    GLenum getFormat() const;
    GLenum getType() const;

    void draw() const;
    void draw(int x, int y) const;
    void draw(const Point<int>& pos) const;

    Image& operator=(const Image& image);
    bool operator==(const Image& image) const;
    bool operator!=(const Image& image) const;

private:
    const char* fRawData;
    Size<int> fSize;
    GLenum fFormat;
    GLenum fType;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_IMAGE_HPP__
