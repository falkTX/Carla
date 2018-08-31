/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#include "ApplicationPrivateData.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Application::Application()
    : pData(new PrivateData()) {}

Application::~Application()
{
    delete pData;
}

void Application::idle()
{
    for (std::list<Window*>::iterator it = pData->windows.begin(), ite = pData->windows.end(); it != ite; ++it)
    {
        Window* const window(*it);
        window->_idle();
    }

    for (std::list<IdleCallback*>::iterator it = pData->idleCallbacks.begin(), ite = pData->idleCallbacks.end(); it != ite; ++it)
    {
        IdleCallback* const idleCallback(*it);
        idleCallback->idleCallback();
    }
}

void Application::exec(int idleTime)
{
    for (; pData->doLoop;)
    {
        idle();
        d_msleep(idleTime);
    }
}

void Application::quit()
{
    pData->doLoop = false;

    for (std::list<Window*>::reverse_iterator rit = pData->windows.rbegin(), rite = pData->windows.rend(); rit != rite; ++rit)
    {
        Window* const window(*rit);
        window->close();
    }
}

bool Application::isQuiting() const noexcept
{
    return !pData->doLoop;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
