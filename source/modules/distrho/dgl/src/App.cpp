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

#include "../App.hpp"
#include "../Window.hpp"

#include <list>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

struct App::PrivateData {
    bool     doLoop;
    unsigned visibleWindows;
    std::list<Window*> windows;

    PrivateData()
        : doLoop(false),
          visibleWindows(0) {}
};

// -----------------------------------------------------------------------

App::App()
    : pData(new PrivateData())
{
}

App::~App()
{
    pData->windows.clear();
    delete pData;
}

void App::idle()
{
    for (std::list<Window*>::iterator it = pData->windows.begin(); it != pData->windows.end(); ++it)
    {
        Window* const window(*it);
        window->idle();
    }
}

void App::exec()
{
    while (pData->doLoop)
    {
        idle();
        msleep(10);
    }
}

void App::quit()
{
    pData->doLoop = false;

    for (std::list<Window*>::reverse_iterator rit = pData->windows.rbegin(); rit != pData->windows.rend(); ++rit)
    {
        Window* const window(*rit);
        window->close();
    }
}

bool App::isQuiting() const
{
    return !pData->doLoop;
}

// -----------------------------------------------------------------------

void App::addWindow(Window* const window)
{
    if (window != nullptr)
        pData->windows.push_back(window);
}

void App::removeWindow(Window* const window)
{
    if (window != nullptr)
        pData->windows.remove(window);
}

void App::oneShown()
{
    if (++pData->visibleWindows == 1)
        pData->doLoop = true;
}

void App::oneHidden()
{
    if (--pData->visibleWindows == 0)
        pData->doLoop = false;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
