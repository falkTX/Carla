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

#ifndef __DGL_WINDOW_HPP__
#define __DGL_WINDOW_HPP__

#include "Base.hpp"

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class App;

class Window
{
public:
    Window(App* app, Window* parent = nullptr);
    Window(App* app, intptr_t parentId);
    virtual ~Window();

    void exec();
    void focus();
    void idle();
    void repaint();

    bool isVisible();
    void setVisible(bool yesNo);
    void setWindowTitle(const char* title);

    intptr_t getWindowId();

    void show()
    {
        setVisible(true);
    }

    void hide()
    {
        setVisible(false);
    }

private:
    class Private;
    Private* const kPrivate;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DGL_WINDOW_HPP__
