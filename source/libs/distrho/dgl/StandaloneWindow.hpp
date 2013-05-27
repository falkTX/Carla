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

#ifndef __DGL_STANDALONE_WINDOW_HPP__
#define __DGL_STANDALONE_WINDOW_HPP__

#include "App.hpp"
#include "Widget.hpp"
#include "Window.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

class StandaloneWindow
{
public:
    StandaloneWindow()
        : fApp(),
          fWindow(&fApp)
    {
    }

    App* getApp()
    {
        return &fApp;
    }

    Window* getWindow()
    {
        return &fWindow;
    }

    void exec()
    {
        fWindow.show();
        fApp.exec();
    }

    // ----------------------------------------------------
    // helpers

    void setSize(unsigned int width, unsigned int height)
    {
        fWindow.setSize(width, height);
    }

    void setWindowTitle(const char* title)
    {
        fWindow.setWindowTitle(title);
    }

private:
    App fApp;
    Window fWindow;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_STANDALONE_WINDOW_HPP__
