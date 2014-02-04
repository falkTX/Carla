/*
 * Carla Plugin UI
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PLUGIN_UI_HPP_INCLUDED
#define CARLA_PLUGIN_UI_HPP_INCLUDED

#include "CarlaUtils.hpp"

// -----------------------------------------------------

class CarlaPluginUi
{
public:
    class CloseCallback {
    public:
        virtual ~CloseCallback() {}
        virtual void handlePluginUiClosed() = 0;
    };

    virtual ~CarlaPluginUi() {}
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void focus() = 0;
    virtual void idle() = 0;
    virtual void setSize(const uint with, const uint height, const bool forceUpdate) = 0;
    virtual void setTitle(const char* const title) = 0;
    virtual void setTransientWinId(const uintptr_t winId) = 0;
    virtual void* getPtr() const noexcept = 0;

    static bool tryTransientWinIdMatch(const ulong pid, const char* const uiTitle, const uintptr_t winId);

#ifdef CARLA_OS_MAC
    static CarlaPluginUi* newCocoa(CloseCallback*, uintptr_t);
#endif
#ifdef CARLA_OS_WIN
    static CarlaPluginUi* newWindows(CloseCallback*, uintptr_t);
#endif
#ifdef HAVE_X11
    static CarlaPluginUi* newX11(CloseCallback*, uintptr_t);
#endif

protected:
    CloseCallback* fCallback;
    CarlaPluginUi(CloseCallback* const cb, const uintptr_t) noexcept : fCallback(cb) {}
};

// -----------------------------------------------------

#endif // CARLA_PLUGIN_UI_HPP_INCLUDED
