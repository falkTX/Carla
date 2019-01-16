/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifdef JUCE_AUDIO_PROCESSORS_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#define JUCE_CORE_INCLUDE_OBJC_HELPERS 1

#include "juce_audio_processors.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
#if JUCE_MAC
 #if JUCE_SUPPORT_CARBON \
      && ((JUCE_PLUGINHOST_VST || JUCE_PLUGINHOST_AU) \
           || ! (defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6))
  #include <Carbon/Carbon.h>
  #include "../juce_gui_extra/native/juce_mac_CarbonViewWrapperComponent.h"
 #endif
#endif

#if JUCE_PLUGINHOST_VST && JUCE_LINUX && ! JUCE_AUDIOPROCESSOR_NO_GUI
 #include <X11/Xlib.h>
 #include <X11/Xutil.h>
 #undef KeyPress
#endif

#if ! JUCE_WINDOWS && ! JUCE_MAC
 #undef JUCE_PLUGINHOST_VST3
 #define JUCE_PLUGINHOST_VST3 0
#endif

#if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
 #include <AudioUnit/AudioUnit.h>
#endif

//==============================================================================
namespace juce
{

static inline bool arrayContainsPlugin (const OwnedArray<PluginDescription>& list,
                                        const PluginDescription& desc)
{
    for (auto* p : list)
        if (p->isDuplicateOf (desc))
            return true;

    return false;
}

#if JUCE_MAC || JUCE_IOS

#if JUCE_IOS
 #define JUCE_IOS_MAC_VIEW  UIView
 typedef UIViewComponent  ViewComponentBaseClass;
#else
 #define JUCE_IOS_MAC_VIEW  NSView
 typedef NSViewComponent  ViewComponentBaseClass;
#endif

//==============================================================================
struct AutoResizingNSViewComponent  : public ViewComponentBaseClass,
                                      private AsyncUpdater
{
    AutoResizingNSViewComponent();
    void childBoundsChanged (Component*) override;
    void handleAsyncUpdate() override;
    bool recursive;
};

//==============================================================================
struct AutoResizingNSViewComponentWithParent  : public AutoResizingNSViewComponent,
                                                private Timer
{
    AutoResizingNSViewComponentWithParent();
    JUCE_IOS_MAC_VIEW* getChildView() const;
    void timerCallback() override;
};

//==============================================================================
AutoResizingNSViewComponent::AutoResizingNSViewComponent()
    : recursive (false) {}

void AutoResizingNSViewComponent::childBoundsChanged (Component*) override
{
    if (recursive)
    {
        triggerAsyncUpdate();
    }
    else
    {
        recursive = true;
        resizeToFitView();
        recursive = true;
    }
}

void AutoResizingNSViewComponent::handleAsyncUpdate() override
{
    resizeToFitView();
}

//==============================================================================
AutoResizingNSViewComponentWithParent::AutoResizingNSViewComponentWithParent()
{
    JUCE_IOS_MAC_VIEW* v = [[JUCE_IOS_MAC_VIEW alloc] init];
    setView (v);
    [v release];

    startTimer (30);
}

JUCE_IOS_MAC_VIEW* AutoResizingNSViewComponentWithParent::getChildView() const
{
    if (JUCE_IOS_MAC_VIEW* parent = (JUCE_IOS_MAC_VIEW*) getView())
        if ([[parent subviews] count] > 0)
            return [[parent subviews] objectAtIndex: 0];

    return nil;
}

void AutoResizingNSViewComponentWithParent::timerCallback() override
{
    if (JUCE_IOS_MAC_VIEW* child = getChildView())
    {
        stopTimer();
        setView (child);
    }
}
#endif

} // namespace juce

#if JUCE_CLANG
 #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "format/juce_AudioPluginFormat.cpp"
#include "format/juce_AudioPluginFormatManager.cpp"
#include "processors/juce_AudioProcessor.cpp"
#include "processors/juce_AudioProcessorGraph.cpp"
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
 #include "processors/juce_AudioProcessorEditor.cpp"
 #include "processors/juce_GenericAudioProcessorEditor.cpp"
#endif
#include "processors/juce_PluginDescription.cpp"
#include "format_types/juce_LADSPAPluginFormat.cpp"
#include "format_types/juce_VSTPluginFormat.cpp"
#include "format_types/juce_VST3PluginFormat.cpp"
#include "format_types/juce_AudioUnitPluginFormat.mm"
#include "scanning/juce_KnownPluginList.cpp"
#include "scanning/juce_PluginDirectoryScanner.cpp"
#include "scanning/juce_PluginListComponent.cpp"
#include "utilities/juce_AudioProcessorParameters.cpp"
#include "utilities/juce_AudioProcessorValueTreeState.cpp"
