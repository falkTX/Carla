/*
 * Carla Plugin UI
 * Copyright (C) 2014-2022 Filipe Coelho <falktx@falktx.com>
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
    class Callback {
    public:
        virtual ~Callback() {}
        virtual void handlePluginUIClosed() = 0;
        virtual void handlePluginUIResized(uint width, uint height) = 0;
    };

    virtual ~CarlaPluginUI() {}
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void focus() = 0;
    virtual void idle() = 0;
    virtual void setMinimumSize(uint with, uint height) = 0;
    virtual void setSize(uint with, uint height, bool forceUpdate, bool resizeChild) = 0;
    virtual void setTitle(const char* title) = 0;
    virtual void setChildWindow(void* ptr) = 0;
    virtual void setTransientWinId(uintptr_t winId) = 0;
    virtual void* getPtr() const noexcept = 0;
#ifdef HAVE_X11
    virtual void* getDisplay() const noexcept = 0;
#endif

#ifndef BUILD_BRIDGE_ALTERNATIVE_ARCH
    static bool tryTransientWinIdMatch(uintptr_t pid, const char* uiTitle, uintptr_t winId, bool centerUI);
#endif

#ifdef CARLA_OS_MAC
    static CarlaPluginUI* newCocoa(Callback*, uintptr_t, bool isStandalone, bool isResizable);
#endif
#ifdef CARLA_OS_WIN
    static CarlaPluginUI* newWindows(Callback*, uintptr_t, bool isStandalone, bool isResizable);
#endif
#ifdef HAVE_X11
    static CarlaPluginUI* newX11(Callback*, uintptr_t, bool isStandalone, bool isResizable, bool canMonitorChildren);
#endif

protected:
    bool fIsIdling;
    bool fIsStandalone;
    bool fIsResizable;
    Callback* fCallback;

    CarlaPluginUI(Callback* const cb,
                  const bool isStandalone,
                  const bool isResizable) noexcept
        : fIsIdling(false),
          fIsStandalone(isStandalone),
          fIsResizable(isResizable),
          fCallback(cb) {}

    CARLA_DECLARE_NON_COPYABLE(CarlaPluginUI)
};

// -----------------------------------------------------

#endif // CARLA_PLUGIN_UI_HPP_INCLUDED
