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
#include "LinkedList.hpp"

#include <png.h>
#include <clxclient.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#define EV_X11  16
#define EV_EXIT 31

// -----------------------------------------------------------------------

struct x_cairo_t {
    cairo_t        * type;
    cairo_surface_t* surf;

    x_cairo_t() noexcept
        : type(nullptr),
          surf(nullptr) {}

    ~x_cairo_t()
    {
        cairo_destroy(type);
        cairo_surface_destroy(surf);
    }

    void initIfNeeded(X_display* const disp)
    {
        if (surf != nullptr)
            return;

        surf = cairo_xlib_surface_create(disp->dpy(), 0, disp->dvi(), 50, 50);
        type = cairo_create(surf);
    }
};

// -----------------------------------------------------------------------

struct X_handler_Param {
    uint32_t index;
    float value;
};

typedef LinkedList<X_handler_Param> ParamList;

template<class MainwinType>
class X_handler_thread : public CarlaThread
{
public:
    struct SetValueCallback {
        virtual ~SetValueCallback() {}
        virtual void setParameterValueFromHandlerThread(const uint32_t index, const float value) = 0;
    };

    X_handler_thread(SetValueCallback* const cb)
        : CarlaThread("X_handler"),
          fCallback(cb),
          fMutex(),
          fHandler(nullptr),
          fRootwin(nullptr),
          fMainwin(nullptr),
          fClosed(false),
          fParamMutex(),
          fParamChanges() {}

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

    void setParameterValueLater(const uint32_t index, const float value) noexcept
    {
        const CarlaMutexLocker cml(fParamMutex);

        for (ParamList::Itenerator it = fParamChanges.begin(); it.valid(); it.next())
        {
            X_handler_Param& param(it.getValue());

            if (param.index != index)
                continue;

            param.value = value;
            return;
        }

        const X_handler_Param param = { index, value };
        fParamChanges.append(param);
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
    SetValueCallback* const fCallback;

    CarlaMutex    fMutex;
    X_handler*    fHandler;
    X_rootwin*    fRootwin;
    MainwinType*  fMainwin;
    volatile bool fClosed;

    CarlaMutex fParamMutex;
    ParamList  fParamChanges;

    void run() override
    {
        for (; ! shouldThreadExit();)
        {
            const CarlaMutexLocker cml(fMutex);

            CARLA_SAFE_ASSERT_RETURN(fMainwin != nullptr,);

            {
                const CarlaMutexLocker cml(fParamMutex);

                for (ParamList::Itenerator it = fParamChanges.begin(); it.valid(); it.next())
                {
                    const X_handler_Param& param(it.getValue());
                    fCallback->setParameterValueFromHandlerThread(param.index, param.value);
                }

                fParamChanges.clear();
            }

            switch (fMainwin->process())
            {
            case EV_X11:
                fRootwin->handle_event();
                fHandler->next_event();
                break;
            case EV_EXIT:
                fClosed  = true;
                fHandler = nullptr;
                fMainwin = nullptr;
                fRootwin = nullptr;
                return;
            case Esync::EV_TIME:
                fRootwin->handle_event();
                break;
            default:
                carla_stdout("custom X11 event for zita plugs");
                break;
            }
        }
    }
};

// -----------------------------------------------------------------------

#endif // CARLA_ZITA_COMMON_HPP_INCLUDED
