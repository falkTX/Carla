/*
 * Juce Plugin Window Helper
 * Copyright (C) 2013-2021 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaVstUtils.hpp"

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
    JucePluginWindow(const uintptr_t parentId, const bool isStandalone, AEffect* const vstEffect)
        : DialogWindow("JucePluginWindow", Colour(50, 50, 200), true, false),
          fIsStandalone(isStandalone),
          fClosed(false),
          fShown(false),
          fTransientId(parentId),
          fLastKeyIndex(0),
          fLastKeyValue(0),
          fLastModifiers(),
          fVstEffect(vstEffect)
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

    bool keyPressed (const KeyPress& key) override
    {
        if (DialogWindow::keyPressed (key))
            return true;

        if (fVstEffect != nullptr)
        {
            const int juceKeyCode = key.getKeyCode();
            int vstKeyIndex = 0;
            int vstKeyValue = 0;

            do {
                /**/ if (juceKeyCode == KeyPress::spaceKey)     vstKeyValue =  7;
                else if (juceKeyCode == KeyPress::escapeKey)    vstKeyValue =  6;
                else if (juceKeyCode == KeyPress::returnKey)    vstKeyValue = 19; // NOTE: 4 is '\r' while 19 is '\n'
                else if (juceKeyCode == KeyPress::tabKey)       vstKeyValue =  2;

                else if (juceKeyCode == KeyPress::deleteKey)    vstKeyValue = 22;
                else if (juceKeyCode == KeyPress::backspaceKey) vstKeyValue =  1;
                else if (juceKeyCode == KeyPress::insertKey)    vstKeyValue = 21;

                else if (juceKeyCode == KeyPress::upKey)        vstKeyValue = 12;
                else if (juceKeyCode == KeyPress::downKey)      vstKeyValue = 14;
                else if (juceKeyCode == KeyPress::leftKey)      vstKeyValue = 11;
                else if (juceKeyCode == KeyPress::rightKey)     vstKeyValue = 13;
                else if (juceKeyCode == KeyPress::pageUpKey)    vstKeyValue = 15;
                else if (juceKeyCode == KeyPress::pageDownKey)  vstKeyValue = 16;
                else if (juceKeyCode == KeyPress::homeKey)      vstKeyValue = 10;
                else if (juceKeyCode == KeyPress::endKey)       vstKeyValue =  9;

                else if (juceKeyCode == KeyPress::F1Key)  vstKeyValue = 40;
                else if (juceKeyCode == KeyPress::F2Key)  vstKeyValue = 41;
                else if (juceKeyCode == KeyPress::F3Key)  vstKeyValue = 42;
                else if (juceKeyCode == KeyPress::F4Key)  vstKeyValue = 43;
                else if (juceKeyCode == KeyPress::F5Key)  vstKeyValue = 44;
                else if (juceKeyCode == KeyPress::F6Key)  vstKeyValue = 45;
                else if (juceKeyCode == KeyPress::F7Key)  vstKeyValue = 46;
                else if (juceKeyCode == KeyPress::F8Key)  vstKeyValue = 47;
                else if (juceKeyCode == KeyPress::F9Key)  vstKeyValue = 48;
                else if (juceKeyCode == KeyPress::F10Key) vstKeyValue = 49;
                else if (juceKeyCode == KeyPress::F11Key) vstKeyValue = 50;
                else if (juceKeyCode == KeyPress::F12Key) vstKeyValue = 51;

                else if (juceKeyCode == KeyPress::F13Key) break;
                else if (juceKeyCode == KeyPress::F14Key) break;
                else if (juceKeyCode == KeyPress::F15Key) break;
                else if (juceKeyCode == KeyPress::F16Key) break;
                else if (juceKeyCode == KeyPress::F17Key) break;
                else if (juceKeyCode == KeyPress::F18Key) break;
                else if (juceKeyCode == KeyPress::F19Key) break;
                else if (juceKeyCode == KeyPress::F20Key) break;
                else if (juceKeyCode == KeyPress::F21Key) break;
                else if (juceKeyCode == KeyPress::F22Key) break;
                else if (juceKeyCode == KeyPress::F23Key) break;
                else if (juceKeyCode == KeyPress::F24Key) break;
                else if (juceKeyCode == KeyPress::F25Key) break;
                else if (juceKeyCode == KeyPress::F26Key) break;
                else if (juceKeyCode == KeyPress::F27Key) break;
                else if (juceKeyCode == KeyPress::F28Key) break;
                else if (juceKeyCode == KeyPress::F29Key) break;
                else if (juceKeyCode == KeyPress::F30Key) break;
                else if (juceKeyCode == KeyPress::F31Key) break;
                else if (juceKeyCode == KeyPress::F32Key) break;
                else if (juceKeyCode == KeyPress::F33Key) break;
                else if (juceKeyCode == KeyPress::F34Key) break;
                else if (juceKeyCode == KeyPress::F35Key) break;

                else if (juceKeyCode == KeyPress::numberPad0) vstKeyValue = 24;
                else if (juceKeyCode == KeyPress::numberPad1) vstKeyValue = 25;
                else if (juceKeyCode == KeyPress::numberPad2) vstKeyValue = 26;
                else if (juceKeyCode == KeyPress::numberPad3) vstKeyValue = 27;
                else if (juceKeyCode == KeyPress::numberPad4) vstKeyValue = 28;
                else if (juceKeyCode == KeyPress::numberPad5) vstKeyValue = 29;
                else if (juceKeyCode == KeyPress::numberPad6) vstKeyValue = 30;
                else if (juceKeyCode == KeyPress::numberPad7) vstKeyValue = 31;
                else if (juceKeyCode == KeyPress::numberPad8) vstKeyValue = 32;
                else if (juceKeyCode == KeyPress::numberPad9) vstKeyValue = 33;

                else if (juceKeyCode == KeyPress::numberPadAdd)          vstKeyValue = 35;
                else if (juceKeyCode == KeyPress::numberPadSubtract)     vstKeyValue = 37;
                else if (juceKeyCode == KeyPress::numberPadMultiply)     vstKeyValue = 34;
                else if (juceKeyCode == KeyPress::numberPadDivide)       vstKeyValue = 39;
                else if (juceKeyCode == KeyPress::numberPadSeparator)    vstKeyValue = 36;
                else if (juceKeyCode == KeyPress::numberPadDecimalPoint) vstKeyValue = 38;
                else if (juceKeyCode == KeyPress::numberPadEquals)       vstKeyValue = 57;

                else if (juceKeyCode == KeyPress::numberPadDelete) break;

                else if (juceKeyCode == KeyPress::playKey) break;
                else if (juceKeyCode == KeyPress::stopKey) break;
                else if (juceKeyCode == KeyPress::fastForwardKey) break;
                else if (juceKeyCode == KeyPress::rewindKey) break;

                else vstKeyIndex = juceKeyCode;
            } while (false);

            fLastKeyIndex = vstKeyIndex;
            fLastKeyValue = vstKeyValue;
            return fVstEffect->dispatcher (fVstEffect, effEditKeyDown,
                                           vstKeyIndex, vstKeyValue,
                                           nullptr, 0.0f) != 0;
        }

        if (Component* const comp = getContentComponent())
            return comp->keyPressed (key);

        return false;
    }

    bool keyStateChanged (bool isKeyDown) override
    {
        if (DialogWindow::keyStateChanged (isKeyDown))
            return true;

        if (fVstEffect != nullptr && (fLastKeyIndex != 0 || fLastKeyValue != 0) && ! isKeyDown)
        {
            const int vstKeyIndex = fLastKeyIndex;
            const int vstKeyValue = fLastKeyValue;
            fLastKeyIndex = fLastKeyValue = 0;
            return fVstEffect->dispatcher (fVstEffect, effEditKeyUp,
                                           vstKeyIndex, vstKeyValue,
                                           nullptr, 0.0f) != 0;
        }

        if (Component* const comp = getContentComponent())
            return comp->keyStateChanged (isKeyDown);

        return false;
    }

    void modifierKeysChanged (const ModifierKeys& modifiers) override
    {
        DialogWindow::modifierKeysChanged (modifiers);

        if (fVstEffect != nullptr)
        {
            const int oldRawFlags = fLastModifiers.getRawFlags();
            const int newRawFlags = modifiers.getRawFlags();

            if ((oldRawFlags & ModifierKeys::shiftModifier) != (newRawFlags & ModifierKeys::shiftModifier))
            {
                fVstEffect->dispatcher (fVstEffect, (newRawFlags & ModifierKeys::shiftModifier) ? effEditKeyDown : effEditKeyUp, 0,
                                        54, nullptr, 0.0f);
            }

            if ((oldRawFlags & ModifierKeys::ctrlModifier) != (newRawFlags & ModifierKeys::ctrlModifier))
            {
                fVstEffect->dispatcher (fVstEffect, (newRawFlags & ModifierKeys::ctrlModifier) ? effEditKeyDown : effEditKeyUp, 0,
                                        55, nullptr, 0.0f);
            }

            if ((oldRawFlags & ModifierKeys::altModifier) != (newRawFlags & ModifierKeys::altModifier))
            {
                fVstEffect->dispatcher (fVstEffect, (newRawFlags & ModifierKeys::altModifier) ? effEditKeyDown : effEditKeyUp, 0,
                                        56, nullptr, 0.0f);
            }

            if ((oldRawFlags & ModifierKeys::popupMenuClickModifier) != (newRawFlags & ModifierKeys::popupMenuClickModifier))
            {
                fVstEffect->dispatcher (fVstEffect, (newRawFlags & ModifierKeys::popupMenuClickModifier) ? effEditKeyDown : effEditKeyUp, 0,
                                        58, nullptr, 0.0f);
            }
        }

        fLastModifiers = modifiers;
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
        int wflags = 0;
        wflags |= ComponentPeer::windowHasCloseButton;
        wflags |= ComponentPeer::windowHasDropShadow;
        wflags |= ComponentPeer::windowHasTitleBar;
        if (fIsStandalone)
            wflags |= ComponentPeer::windowAppearsOnTaskbar;
        return wflags;
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
    const bool fIsStandalone;
    volatile bool fClosed;
    bool fShown;
    const uintptr_t fTransientId;
    int fLastKeyIndex, fLastKeyValue;
    ModifierKeys fLastModifiers;
    AEffect* const fVstEffect;

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
