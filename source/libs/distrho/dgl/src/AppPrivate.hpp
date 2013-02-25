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

#ifndef __APP_PRIVATE_HPP__
#define __APP_PRIVATE_HPP__

#include "../App.hpp"

#include <list>

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class Window;

class App::Private
{
public:
    Private()
        : fDoLoop(true),
          fVisibleWindows(0)
    {
    }

    ~Private()
    {
        fWindows.clear();
    }

    void addWindow(Window* window)
    {
        if (window != nullptr)
            fWindows.push_back(window);
    }

    void removeWindow(Window* window)
    {
        if (window != nullptr)
            fWindows.remove(window);
    }

    void oneShown()
    {
        fVisibleWindows++;
    }

    void oneHidden()
    {
        fVisibleWindows--;

        if (fVisibleWindows == 0)
            fDoLoop = false;
    }

private:
    bool     fDoLoop;
    unsigned fVisibleWindows;

    std::list<Window*> fWindows;

    friend class App;
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __APP_PRIVATE_HPP__
