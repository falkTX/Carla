/*
  ==============================================================================

   Build options for juce_audio_devices static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_AUDIO_DEVICES_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_AUDIO_DEVICES_APPCONFIG_H_INCLUDED

#include "../juce_events/AppConfig.h"
#include "../juce_audio_basics/AppConfig.h"
#include "../juce_audio_formats/AppConfig.h"

//=============================================================================
/** Config: JUCE_ASIO
    Enables ASIO audio devices (MS Windows only).
    Turning this on means that you'll need to have the Steinberg ASIO SDK installed
    on your Windows build machine.

    See the comments in the ASIOAudioIODevice class's header file for more
    info about this.
*/
#if WINDOWS
 #define JUCE_ASIO 1
#else
 #define JUCE_ASIO 0
#endif

/** Config: JUCE_WASAPI
    Enables WASAPI audio devices (Windows Vista and above).
*/
#define JUCE_WASAPI 0

/** Config: JUCE_DIRECTSOUND
    Enables DirectSound audio (MS Windows only).
*/
#if WINDOWS
 #define JUCE_DIRECTSOUND 1
#else
 #define JUCE_DIRECTSOUND 0
#endif

/** Config: JUCE_ALSA
    Enables ALSA audio devices (Linux only).
*/
#if LINUX
 #define JUCE_ALSA 1
 #define JUCE_ALSA_MIDI_INPUT_NAME  "Carla"
 #define JUCE_ALSA_MIDI_OUTPUT_NAME "Carla"
 #define JUCE_ALSA_MIDI_INPUT_PORT_NAME  "Midi In"
 #define JUCE_ALSA_MIDI_OUTPUT_PORT_NAME "Midi Out"
#else
 #define JUCE_ALSA 0
#endif

/** Config: JUCE_JACK
    Enables JACK audio devices (Linux only).
*/
#if LINUX
 #define JUCE_JACK 1
 #define JUCE_JACK_CLIENT_NAME "Carla"
#else
 #define JUCE_JACK 0
#endif

//=============================================================================
/** Config: JUCE_USE_CDREADER
    Enables the AudioCDReader class (on supported platforms).
*/
#define JUCE_USE_CDREADER 0

/** Config: JUCE_USE_CDBURNER
    Enables the AudioCDBurner class (on supported platforms).
*/
#define JUCE_USE_CDBURNER 0

#endif // CARLA_JUCE_AUDIO_DEVICES_APPCONFIG_H_INCLUDED
