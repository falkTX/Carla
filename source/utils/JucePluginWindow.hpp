/*
 * Juce Plugin Window Helper
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "juce_gui_basics.h"

#ifdef HAVE_X11
# include <X11/Xlib.h>
#endif

// -----------------------------------------------------------------------

namespace juce {

#ifdef HAVE_X11
extern Display* display;
#endif

class JucePluginWindow : public DocumentWindow
{
public:
    JucePluginWindow()
        : DocumentWindow("JucePluginWindow", Colour(50, 50, 200), DocumentWindow::closeButton, false),
          fClosed(false)
    {
        setVisible(false);
        //setAlwaysOnTop(true);
        setOpaque(true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
    }

    void show(Component* const comp, const bool useContentOwned = false)
    {
        fClosed = false;

        centreWithSize(comp->getWidth(), comp->getHeight());

        if (useContentOwned)
            setContentOwned(comp, false);
        else
            setContentNonOwned(comp, true);

        if (! isOnDesktop())
            addToDesktop();

        setVisible(true);
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

    void setTransientWinId(const uintptr_t winId) const
    {
        CARLA_SAFE_ASSERT_RETURN(winId != 0,);

#ifdef HAVE_X11
        CARLA_SAFE_ASSERT_RETURN(display != nullptr,);

        ::Window window = (::Window)getWindowHandle();

        CARLA_SAFE_ASSERT_RETURN(window != 0,);

        XSetTransientForHint(display, window, static_cast<Window>(winId));
#endif
    }

protected:
    void closeButtonPressed() override
    {
        fClosed = true;
    }

private:
    volatile bool fClosed;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePluginWindow)
};

} // namespace juce

using juce::JucePluginWindow;

// -----------------------------------------------------------------------

#endif // JUCE_PLUGIN_WINDOW_HPP_INCLUDED
