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

#include "../ImageAboutWindow.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

ImageAboutWindow::ImageAboutWindow(App* app, Window* parent, const Image& image)
    : Window(app, parent),
      Widget(this),
      fImgBackground(image)
{
    Window::setSize(image.getWidth(), image.getHeight());
    Window::setWindowTitle("About");
}

ImageAboutWindow::ImageAboutWindow(Widget* widget, const Image& image)
    : Window(widget->getApp(), widget->getParent()),
      Widget(this),
      fImgBackground(image)
{
    Window::setSize(image.getWidth(), image.getHeight());
    Window::setWindowTitle("About");
}

void ImageAboutWindow::setImage(const Image& image)
{
    fImgBackground = image;
    Window::setSize(image.getWidth(), image.getHeight());
}

void ImageAboutWindow::onDisplay()
{
    fImgBackground.draw();
}

bool ImageAboutWindow::onMouse(int, bool press, int, int)
{
    if (press)
    {
        Window::close();
        return true;
    }

    return false;
}

bool ImageAboutWindow::onKeyboard(bool press, uint32_t key)
{
    if (press && key == DGL_CHAR_ESCAPE)
    {
        Window::close();
        return true;
    }

    return false;
}

// -------------------------------------------------

END_NAMESPACE_DGL
