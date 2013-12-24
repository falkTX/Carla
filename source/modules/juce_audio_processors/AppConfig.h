/*
  ==============================================================================

   Build options for juce_audio_processors static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_AUDIO_PROCESSORS_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_AUDIO_PROCESSORS_APPCONFIG_H_INCLUDED

#include "../juce_gui_basics/AppConfig.h"
#include "../juce_audio_basics/AppConfig.h"

//=============================================================================
/** Config: JUCE_PLUGINHOST_VST
    Enables the VST audio plugin hosting classes. This requires the Steinberg VST SDK to be
    installed on your machine.

    @see VSTPluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_AU
*/
#define JUCE_PLUGINHOST_VST 1

/** Config: JUCE_PLUGINHOST_AU
    Enables the AudioUnit plugin hosting classes. This is Mac-only, of course.

    @see AudioUnitPluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST
*/
#define JUCE_PLUGINHOST_AU 1

#endif // CARLA_JUCE_AUDIO_PROCESSORS_APPCONFIG_H_INCLUDED
