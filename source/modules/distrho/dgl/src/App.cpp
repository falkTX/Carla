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

#include "AppPrivate.hpp"

#include "../Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

App::App()
    : kPrivate(new Private)
{
}

App::~App()
{
    delete kPrivate;
}

void App::idle()
{
    for (std::list<Window*>::iterator it = kPrivate->fWindows.begin(); it != kPrivate->fWindows.end(); ++it)
    {
        Window* const window(*it);
        window->idle();
    }
}

void App::exec()
{
    while (kPrivate->fDoLoop)
    {
        idle();
        dgl_msleep(10);
    }
}

void App::quit()
{
    kPrivate->fDoLoop = false;

    for (std::list<Window*>::iterator it = kPrivate->fWindows.begin(); it != kPrivate->fWindows.end(); ++it)
    {
        Window* const window(*it);
        window->close();
    }
}

bool App::isQuiting() const
{
    return !kPrivate->fDoLoop;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
