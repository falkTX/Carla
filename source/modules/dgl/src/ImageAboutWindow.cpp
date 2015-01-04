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

#include "../ImageAboutWindow.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageAboutWindow::ImageAboutWindow(Window& parent, const Image& image)
    : Window(parent.getApp(), parent),
      Widget((Window&)*this),
      fImgBackground(image)/*,
      leakDetector_ImageAboutWindow()*/
{
    Window::setResizable(false);
    Window::setSize(static_cast<uint>(image.getWidth()), static_cast<uint>(image.getHeight()));
    Window::setTitle("About");
}

ImageAboutWindow::ImageAboutWindow(Widget* widget, const Image& image)
    : Window(widget->getParentApp(), widget->getParentWindow()),
      Widget((Window&)*this),
      fImgBackground(image)/*,
      leakDetector_ImageAboutWindow()*/
{
    Window::setResizable(false);
    Window::setSize(static_cast<uint>(image.getWidth()), static_cast<uint>(image.getHeight()));
    Window::setTitle("About");
}

void ImageAboutWindow::setImage(const Image& image)
{
    if (fImgBackground == image)
        return;

    fImgBackground = image;
    Window::setSize(static_cast<uint>(image.getWidth()), static_cast<uint>(image.getHeight()));
}

void ImageAboutWindow::onDisplay()
{
    fImgBackground.draw();
}

bool ImageAboutWindow::onKeyboard(const KeyboardEvent& ev)
{
    if (ev.press && ev.key == CHAR_ESCAPE)
    {
        Window::close();
        return true;
    }

    return false;
}

bool ImageAboutWindow::onMouse(const MouseEvent& ev)
{
    if (ev.press)
    {
        Window::close();
        return true;
    }

    return false;
}

void ImageAboutWindow::onReshape(uint width, uint height)
{
    Widget::setSize(width, height);
    Window::onReshape(width, height);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
