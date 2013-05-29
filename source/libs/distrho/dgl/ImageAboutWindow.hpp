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

#ifndef __DGL_IMAGE_ABOUT_WINDOW_HPP__
#define __DGL_IMAGE_ABOUT_WINDOW_HPP__

#include "Image.hpp"
#include "Widget.hpp"
#include "Window.hpp"

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

START_NAMESPACE_DGL

// -------------------------------------------------

class ImageAboutWindow : public Window,
                         public Widget
{
public:
    ImageAboutWindow(App* app, Window* parent, const Image& image = Image());
    ImageAboutWindow(Widget* widget, const Image& image = Image());

    void setImage(const Image& image);

protected:
    void onDisplay() override;
    bool onMouse(int button, bool press, int x, int y) override;
    bool onKeyboard(bool press, uint32_t key) override;

private:
    Image fImgBackground;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_IMAGE_ABOUT_WINDOW_HPP__
