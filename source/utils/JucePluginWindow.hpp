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

#if JUCE_LINUX && defined(HAVE_X11)
# include <X11/Xlib.h>
#elif JUCE_MAC
# import <Cocoa/Cocoa.h>
#endif

// -----------------------------------------------------------------------

namespace juce {

#if JUCE_LINUX && defined(HAVE_X11)
extern Display* display;
#endif

class JucePluginWindow : public DialogWindow
{
public:
    JucePluginWindow(const uintptr_t parentId)
        : DialogWindow("JucePluginWindow", Colour(50, 50, 200), true, false),
          fClosed(false),
          fTransientId(parentId)
    {
        setVisible(false);
        //setAlwaysOnTop(true);
        setOpaque(true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
    }

    void show(Component* const comp)
    {
        fClosed = false;

        centreWithSize(comp->getWidth(), comp->getHeight());
        setContentNonOwned(comp, true);

        if (! isOnDesktop())
            addToDesktop();

        setVisible(true);
        setTransient();
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

private:
    volatile bool fClosed;
    const uintptr_t fTransientId;

    void setTransient()
    {
        if (fTransientId == 0)
            return;

#if JUCE_LINUX && defined(HAVE_X11)
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        ::Window window = (::Window)getWindowHandle();

        CARLA_SAFE_ASSERT_RETURN(window != 0,);

        XSetTransientForHint(display, window, static_cast<::Window>(fTransientId));
#endif

#if JUCE_MAC
        NSView* const view = (NSView*)getWindowHandle();
        CARLA_SAFE_ASSERT_RETURN(view != nullptr,);

        NSWindow* const window = [view window];
        CARLA_SAFE_ASSERT_RETURN(window != nullptr,);

        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:fTransientId];
        CARLA_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        [parentWindow addChildWindow:window
                             ordered:NSWindowAbove];
#endif

#if JUCE_WINDOWS
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
