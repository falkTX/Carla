/*
 * Juce Plugin Window Helper
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef JUCE_PLUGIN_WINDOW_HPP_INCLUDED
#define JUCE_PLUGIN_WINDOW_HPP_INCLUDED

#include "CarlaJuceUtils.hpp"

#include "AppConfig.h"
#include "juce_gui_basics/juce_gui_basics.h"

#if defined(CARLA_OS_LINUX) && defined(HAVE_X11)
# include <X11/Xlib.h>
#elif defined(CARLA_OS_MAC)
# define Component CocoaComponent
# define MemoryBlock CocoaMemoryBlock
# define Point CocoaPoint
# import <Cocoa/Cocoa.h>
# undef Component
# undef MemoryBlock
# undef Point
#endif

// -----------------------------------------------------------------------

namespace juce {

class JucePluginWindow : public DialogWindow
{
public:
    JucePluginWindow(const uintptr_t parentId)
        : DialogWindow("JucePluginWindow", Colour(50, 50, 200), true, false),
          fClosed(false),
          fShown(false),
          fTransientId(parentId)
    {
        setVisible(false);
        setOpaque(true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
    }

    void show(Component* const comp)
    {
        fClosed = false;
        fShown = true;

        centreWithSize(comp->getWidth(), comp->getHeight());
        setContentNonOwned(comp, true);

        if (! isOnDesktop())
            addToDesktop();

#ifndef CARLA_OS_LINUX
        setAlwaysOnTop(true);
#endif
        setTransient();
        setVisible(true);
        toFront(true);

#ifndef CARLA_OS_LINUX
        postCommandMessage(0);
#endif
    }

    void hide()
    {
        setVisible(false);

        if (isOnDesktop())
            removeFromDesktop();

        clearContentComponent();
    }

    bool wasClosedByUser() const noexcept
    {
        return fClosed;
    }

protected:
    void closeButtonPressed() override
    {
        fClosed = true;
    }

    bool escapeKeyPressed() override
    {
        fClosed = true;
        return true;
    }

    int getDesktopWindowStyleFlags() const override
    {
        return ComponentPeer::windowAppearsOnTaskbar | ComponentPeer::windowHasDropShadow | ComponentPeer::windowHasTitleBar | ComponentPeer::windowHasCloseButton;
    }

#ifndef CARLA_OS_LINUX
    void handleCommandMessage(const int comamndId) override
    {
        CARLA_SAFE_ASSERT_RETURN(comamndId == 0,);

        if (fShown)
        {
            fShown = false;
            setAlwaysOnTop(false);
        }
    }
#endif

private:
    volatile bool fClosed;
    bool fShown;
    const uintptr_t fTransientId;

    void setTransient()
    {
        if (fTransientId == 0)
            return;

#if defined(CARLA_OS_LINUX) && defined(HAVE_X11)
        Display* const display = XWindowSystem::getInstance()->getDisplay();
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        ::Window window = (::Window)getWindowHandle();

        CARLA_SAFE_ASSERT_RETURN(window != 0,);

        XSetTransientForHint(display, window, static_cast<::Window>(fTransientId));
#endif

#ifdef CARLA_OS_MAC
        NSView* const view = (NSView*)getWindowHandle();
        CARLA_SAFE_ASSERT_RETURN(view != nullptr,);

        NSWindow* const window = [view window];
        CARLA_SAFE_ASSERT_RETURN(window != nullptr,);

        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:fTransientId];
        CARLA_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        [parentWindow addChildWindow:window
                             ordered:NSWindowAbove];
#endif

#ifdef CARLA_OS_WIN
        const HWND window = (HWND)getWindowHandle();
        CARLA_SAFE_ASSERT_RETURN(window != nullptr,);

        SetWindowLongPtr(window, GWLP_HWNDPARENT, static_cast<LONG_PTR>(fTransientId));
#endif
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePluginWindow)
};

} // namespace juce

using juce::JucePluginWindow;

// -----------------------------------------------------------------------

#endif // JUCE_PLUGIN_WINDOW_HPP_INCLUDED
