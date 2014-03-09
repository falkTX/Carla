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
#ifndef VESTIGE_HEADER
# define JUCE_PLUGINHOST_VST 1
#else
# define JUCE_PLUGINHOST_VST 0
#endif

/** Config: JUCE_PLUGINHOST_VST3
    Enables the VST3 audio plugin hosting classes. This requires the Steinberg VST3 SDK to be
    installed on your machine.

    @see VSTPluginFormat, VST3PluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST, JUCE_PLUGINHOST_AU
*/
#if defined(JUCE_WINDOWS) || defined(JUCE_MAC)
# define JUCE_PLUGINHOST_VST3 1 // FIXME
#else
# define JUCE_PLUGINHOST_VST3 0
#endif

/** Config: JUCE_PLUGINHOST_AU
    Enables the AudioUnit plugin hosting classes. This is Mac-only, of course.

    @see AudioUnitPluginFormat, AudioPluginFormat, AudioPluginFormatManager, JUCE_PLUGINHOST_VST
*/
#define JUCE_PLUGINHOST_AU 0 // FIXME - my OSX version is still at 10.5...

#define JUCE_PLUGINHOST_LADSPA 1

#endif // CARLA_JUCE_AUDIO_PROCESSORS_APPCONFIG_H_INCLUDED
