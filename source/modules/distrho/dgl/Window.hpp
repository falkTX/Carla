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

#ifndef DGL_WINDOW_HPP_INCLUDED
#define DGL_WINDOW_HPP_INCLUDED

#include "Base.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class App;
class Widget;

class Window
{
public:
    Window(App* app, Window* parent = nullptr);
    Window(App* app, intptr_t parentId);
    virtual ~Window();

    void exec(bool lock = false);
    void focus();
    void idle();
    void repaint();

    bool isVisible();
    void setVisible(bool yesNo);
    void setResizable(bool yesNo);
    void setSize(unsigned int width, unsigned int height);
    void setWindowTitle(const char* title);

    App* getApp() const;
    int getModifiers() const;
    intptr_t getWindowId() const;

    void addWidget(Widget* widget);
    void removeWidget(Widget* widget);

    void show();
    void hide();
    void close();

private:
    class Private;
    Private* const kPrivate;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WINDOW_HPP_INCLUDED
