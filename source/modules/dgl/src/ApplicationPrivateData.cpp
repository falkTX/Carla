/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#include "pugl.hpp"

#include <ctime>

START_NAMESPACE_DGL

typedef std::list<DGL_NAMESPACE::Window*>::iterator WindowListIterator;
typedef std::list<DGL_NAMESPACE::Window*>::reverse_iterator WindowListReverseIterator;

static d_ThreadHandle getCurrentThreadHandle() noexcept
{
   #ifdef DISTRHO_OS_WINDOWS
    return GetCurrentThread();
   #else
    return pthread_self();
   #endif
}

static bool isThisTheMainThread(const d_ThreadHandle mainThreadHandle) noexcept
{
   #ifdef DISTRHO_OS_WINDOWS
    return GetCurrentThread() == mainThreadHandle; // IsGUIThread ?
   #else
    return pthread_equal(getCurrentThreadHandle(), mainThreadHandle) != 0;
   #endif
}

// --------------------------------------------------------------------------------------------------------------------

const char* Application::getClassName() const noexcept
{
    return puglGetWorldString(pData->world, PUGL_CLASS_NAME);
}

// --------------------------------------------------------------------------------------------------------------------

Application::PrivateData::PrivateData(const bool standalone, const Type type)
    : world(puglNewWorld(standalone ? PUGL_PROGRAM : PUGL_MODULE,
                         (standalone ? PUGL_WORLD_THREADS : 0))),
      isModern(false),
      isStandalone(standalone),
      isStarting(true),
      isQuitting(false),
      isQuittingInNextCycle(false),
      needsRepaint(false),
      visibleWindows(0),
      mainThreadHandle(getCurrentThreadHandle()),
      windows(),
      idleCallbacks()
{
    DISTRHO_SAFE_ASSERT_RETURN(world != nullptr,);

   #ifdef __EMSCRIPTEN__
    puglSetWorldString(world, PUGL_CLASS_NAME, "canvas");
   #else
    puglSetWorldString(world, PUGL_CLASS_NAME, DISTRHO_MACRO_AS_STRING(DGL_NAMESPACE));
   #endif
}

Application::PrivateData::~PrivateData()
{
    DISTRHO_SAFE_ASSERT(isStarting || isQuitting);
    DISTRHO_SAFE_ASSERT(visibleWindows == 0);

    windows.clear();
    idleCallbacks.clear();

    if (world != nullptr)
        puglFreeWorld(world);
}

// --------------------------------------------------------------------------------------------------------------------

void Application::PrivateData::oneWindowShown() noexcept
{
    if (++visibleWindows == 1)
    {
        isQuitting = false;
        isStarting = false;
    }
}

void Application::PrivateData::oneWindowClosed() noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(visibleWindows != 0,);

    if (--visibleWindows == 0)
        isQuitting = true;
}

// --------------------------------------------------------------------------------------------------------------------

void Application::PrivateData::idle(const uint timeoutInMs)
{
    if (isQuittingInNextCycle)
    {
        quit();
        isQuittingInNextCycle = false;
    }

    if (world != nullptr)
    {
        const double timeoutInSeconds = timeoutInMs != 0
                                      ? static_cast<double>(timeoutInMs) / 1000.0
                                      : 0.0;

        puglUpdate(world, timeoutInSeconds);
    }

    triggerIdleCallbacks();
}

void Application::PrivateData::triggerIdleCallbacks()
{
    for (std::list<IdleCallback*>::iterator it = idleCallbacks.begin(), ite = idleCallbacks.end(); it != ite; ++it)
    {
        IdleCallback* const idleCallback(*it);
        idleCallback->idleCallback();
    }
}

void Application::PrivateData::repaintIfNeeeded()
{
    if (needsRepaint)
    {
        needsRepaint = false;

        for (WindowListIterator it = windows.begin(), ite = windows.end(); it != ite; ++it)
        {
            DGL_NAMESPACE::Window* const window(*it);
            window->repaint();
        }
    }
}

void Application::PrivateData::quit()
{
    if (! isThisTheMainThread(mainThreadHandle))
    {
        if (! isQuittingInNextCycle)
        {
            isQuittingInNextCycle = true;
            return;
        }
    }

    isQuitting = true;

    for (WindowListReverseIterator rit = windows.rbegin(), rite = windows.rend(); rit != rite; ++rit)
    {
        DGL_NAMESPACE::Window* const window(*rit);
        window->close();
    }
}

double Application::PrivateData::getTime() const
{
    return world != nullptr ? puglGetTime(world) : 0.0;
}

void Application::PrivateData::setClassName(const char* const name)
{
    DISTRHO_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

    if (world != nullptr)
        puglSetWorldString(world, PUGL_CLASS_NAME, name);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
