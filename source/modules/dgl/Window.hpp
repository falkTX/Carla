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

#ifndef DGL_WINDOW_HPP_INCLUDED
#define DGL_WINDOW_HPP_INCLUDED

#include "Geometry.hpp"

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class App;
class Widget;

class Window
{
public:
    Window(App& app);
    Window(App& app, Window& parent);
    Window(App& app, intptr_t parentId);
    virtual ~Window();

    void show();
    void hide();
    void close();
    void exec(bool lockWait = false);

    void focus();
    void repaint();

    bool isVisible() const noexcept;
    void setVisible(bool yesNo);

    bool isResizable() const noexcept;
    void setResizable(bool yesNo);

    int getWidth() const noexcept;
    int getHeight() const noexcept;
    Size<int> getSize() const noexcept;
    void setSize(unsigned int width, unsigned int height);

    void setTitle(const char* title);

    App&     getApp() const noexcept;
    uint32_t getEventTimestamp() const;
    int      getModifiers() const;
    intptr_t getWindowId() const;

private:
    class PrivateData;
    PrivateData* const pData;
    friend class App;
    friend class Widget;

    void _addWidget(Widget* const widget);
    void _removeWidget(Widget* const widget);
    void _idle();
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WINDOW_HPP_INCLUDED
