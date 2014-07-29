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

class CarlaPluginUI
{
public:
    class CloseCallback {
    public:
        virtual ~CloseCallback() {}
        virtual void handlePluginUIClosed() = 0;
    };

    virtual ~CarlaPluginUI() {}
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void focus() = 0;
    virtual void idle() = 0;
    virtual void setSize(const uint with, const uint height, const bool forceUpdate) = 0;
    virtual void setTitle(const char* const title) = 0;
    virtual void setTransientWinId(const uintptr_t winId) = 0;
    virtual void* getPtr() const noexcept = 0;
#ifdef HAVE_X11
    virtual void* getDisplay() const noexcept = 0;
#endif

    static bool tryTransientWinIdMatch(const uintptr_t pid, const char* const uiTitle, const uintptr_t winId, const bool centerUI);

#ifdef CARLA_OS_MAC
    static CarlaPluginUI* newCocoa(CloseCallback*, uintptr_t);
#endif
#ifdef CARLA_OS_WIN
    static CarlaPluginUI* newWindows(CloseCallback*, uintptr_t);
#endif
#ifdef HAVE_X11
    static CarlaPluginUI* newX11(CloseCallback*, uintptr_t);
#endif

protected:
    bool fIsIdling;
    CloseCallback* fCallback;
    CarlaPluginUI(CloseCallback* const cb) noexcept : fIsIdling(false), fCallback(cb) {}

    CARLA_DECLARE_NON_COPY_CLASS(CarlaPluginUI)
};

// -----------------------------------------------------

#endif // CARLA_PLUGIN_UI_HPP_INCLUDED
