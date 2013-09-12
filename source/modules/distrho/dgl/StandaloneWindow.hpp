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

#ifndef DGL_STANDALONE_WINDOW_HPP_INCLUDED
#define DGL_STANDALONE_WINDOW_HPP_INCLUDED

#include "App.hpp"
#include "Widget.hpp"
#include "Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class StandaloneWindow
{
public:
    StandaloneWindow()
        : fApp(),
          fWindow(&fApp)
    {
    }

    App& getApp() const noexcept
    {
        return fApp;
    }

    Window& getWindow() const noexcept
    {
        return fWindow;
    }

    void exec()
    {
        fWindow.show();
        fApp.exec();
    }

    // -------------------------------------------------------------------
    // helpers

    void setResizable(bool yesNo)
    {
        fWindow.setResizable(yesNo);
    }

    void setSize(unsigned int width, unsigned int height)
    {
        fWindow.setSize(width, height);
    }

    void setTitle(const char* title)
    {
        fWindow.setTitle(title);
    }

private:
    App fApp;
    Window fWindow;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_STANDALONE_WINDOW_HPP_INCLUDED
