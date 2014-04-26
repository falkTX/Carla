/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_IMAGE_HPP_INCLUDED
#define DGL_IMAGE_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class Image
{
public:
    Image() noexcept;
    Image(const char* rawData, int width, int height, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE) noexcept;
    Image(const char* rawData, const Size<int>& size, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE) noexcept;
    Image(const Image& image) noexcept;
    ~Image();

    void loadFromMemory(const char* rawData, int width, int height, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE) noexcept;
    void loadFromMemory(const char* rawData, const Size<int>& size, GLenum format = GL_BGRA, GLenum type = GL_UNSIGNED_BYTE) noexcept;

    bool isValid() const noexcept;

    int getWidth() const noexcept;
    int getHeight() const noexcept;
    const Size<int>& getSize() const noexcept;

    const char* getRawData() const noexcept;
    GLenum getFormat() const noexcept;
    GLenum getType() const noexcept;

    void draw();
    void draw(int x, int y);
    void draw(const Point<int>& pos);

    Image& operator=(const Image& image) noexcept;
    bool operator==(const Image& image) const noexcept;
    bool operator!=(const Image& image) const noexcept;

private:
    const char* fRawData;
    Size<int> fSize;
    GLenum fFormat;
    GLenum fType;
    GLuint fTextureId;

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_HPP_INCLUDED
