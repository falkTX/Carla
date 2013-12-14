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

#ifndef DGL_CAIRO_WIDGET_HPP_INCLUDED
#define DGL_CAIRO_WIDGET_HPP_INCLUDED

#include "Widget.hpp"

#include <cairo.h>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class CairoWidget : public Widget
{
public:
    CairoWidget(Window& parent)
        : Widget(parent),
          fContext(nullptr),
          fSurface(nullptr),
          fTextureId(0)
    {
    }

protected:
    virtual void cairoDisplay(cairo_t* const context) = 0;

private:
    void onReshape(int width, int height) override
    {
        // handle resize
        setSize(width, height);
        Widget::onReshape(width, height);

        // free previous if needed
        onClose();

        // create new
        fSurface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
        fContext = cairo_create(fSurface);

        glGenTextures(1, &fTextureId);
    }

    void onClose() override
    {
        if (fContext != nullptr)
        {
            cairo_destroy(fContext);
            fContext = nullptr;
        }

        if (fSurface != nullptr)
        {
            cairo_surface_destroy(fSurface);
            fSurface = nullptr;
        }

        if (fTextureId != 0)
        {
            glDeleteTextures(1, &fTextureId);
            fTextureId = 0;
        }
    }

    void onDisplay() override
    {
        // wait for first resize
        if (fSurface == nullptr || fContext == nullptr)
        {
            glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        const int width  = getWidth();
        const int height = getHeight();

        // draw cairo stuff
        cairoDisplay(fContext);

        // get cairo surface data (RGB24)
        unsigned char* const surfaceData = cairo_image_surface_get_data(fSurface);

        // enable GL texture
        glEnable(GL_TEXTURE_RECTANGLE_ARB);

        // set texture params
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // bind texture to surface data
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, fTextureId);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, surfaceData);

        // draw the texture
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_QUADS);
          glTexCoord2i(0, height);
          glVertex2i(0, height);

          glTexCoord2i(width, height);
          glVertex2i(width, height);

          glTexCoord2i(width, 0);
          glVertex2i(width, 0);

          glTexCoord2i(0, 0);
          glVertex2i(0, 0);
        glEnd();

        // cleanup
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

private:
    cairo_t*         fContext;
    cairo_surface_t* fSurface;
    GLuint           fTextureId;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_CAIRO_WIDGET_HPP_INCLUDED
