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

#ifndef DGL_APP_PRIVATE_HPP_INCLUDED
#define DGL_APP_PRIVATE_HPP_INCLUDED

#include "../App.hpp"

#include <list>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class Window;

class App::PrivateData
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

    void addWindow(Window* const window)
    {
        if (window != nullptr)
            fWindows.push_back(window);
    }

    void removeWindow(Window* const window)
    {
        if (window != nullptr)
            fWindows.remove(window);
    }

    void oneShown()
    {
        if (++fVisibleWindows == 1)
            fDoLoop = true;
    }

    void oneHidden()
    {
        if (--fVisibleWindows == 0)
            fDoLoop = false;
    }

private:
    bool     fDoLoop;
    unsigned fVisibleWindows;

    std::list<Window*> fWindows;

    friend class App;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_HPP_INCLUDED
