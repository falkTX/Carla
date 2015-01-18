/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_ZITA_COMMON_HPP_INCLUDED
#define CARLA_ZITA_COMMON_HPP_INCLUDED

#include "CarlaMutex.hpp"
#include "CarlaThread.hpp"

#include <png.h>
#include <clxclient.h>

#define EV_X11  16
#define EV_EXIT 31

// -----------------------------------------------------------------------

template<class MainwinType>
class X_handler_thread : public CarlaThread
{
public:
    X_handler_thread()
        : CarlaThread("X_handler"),
          fMutex(),
          fHandler(nullptr),
          fRootwin(nullptr),
          fMainwin(nullptr),
          fClosed(false) {}

    void setupAndRun(X_handler* const h, X_rootwin* const r, MainwinType* const m) noexcept
    {
        const CarlaMutexLocker cml(fMutex);

        fHandler = h;
        fRootwin = r;
        fMainwin = m;

        startThread();
    }

    void stopThread() noexcept
    {
        signalThreadShouldExit();

        {
            const CarlaMutexLocker cml(fMutex);
            fHandler = nullptr;
            fRootwin = nullptr;
            fMainwin = nullptr;
        }

        CarlaThread::stopThread(1000);
    }

    CarlaMutex& getLock() noexcept
    {
        return fMutex;
    }

    bool wasClosed() noexcept
    {
        if (fClosed)
        {
            fClosed = false;
            return true;
        }
        return false;
    }

private:
    CarlaMutex    fMutex;
    X_handler*    fHandler;
    X_rootwin*    fRootwin;
    MainwinType*  fMainwin;
    volatile bool fClosed;

    void run() override
    {
        for (; ! shouldThreadExit();)
        {
            int ev;

            {
                const CarlaMutexLocker cml(fMutex);

                CARLA_SAFE_ASSERT_RETURN(fMainwin != nullptr,);

                for (; (ev = fMainwin->process()) == EV_X11;)
                {
                    fRootwin->handle_event();
                    fHandler->next_event();
                }

                if (ev == Esync::EV_TIME)
                {
                    fRootwin->handle_event();
                }
                else if (ev == EV_EXIT)
                {
                    fClosed  = true;
                    fHandler = nullptr;
                    fMainwin = nullptr;
                    fRootwin = nullptr;
                    return;
                }
            }

            carla_msleep(10);
        }
    }
};

// -----------------------------------------------------------------------

#endif // CARLA_ZITA_COMMON_HPP_INCLUDED
