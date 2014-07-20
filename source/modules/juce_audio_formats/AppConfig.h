/*
  ==============================================================================

   Build options for juce_audio_formats static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_AUDIO_FORMATS_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_AUDIO_FORMATS_APPCONFIG_H_INCLUDED

#include "../juce_audio_basics/AppConfig.h"

//=============================================================================
/** Config: JUCE_USE_FLAC
    Enables the FLAC audio codec classes (available on all platforms).
    If your app doesn't need to read FLAC files, you might want to disable this to
    reduce the size of your codebase and build time.
*/
#define JUCE_USE_FLAC 1

/** Config: JUCE_USE_OGGVORBIS
    Enables the Ogg-Vorbis audio codec classes (available on all platforms).
    If your app doesn't need to read Ogg-Vorbis files, you might want to disable this to
    reduce the size of your codebase and build time.
*/
#define JUCE_USE_OGGVORBIS 1

/** Config: JUCE_USE_MP3AUDIOFORMAT
    Enables the software-based MP3AudioFormat class.
    IMPORTANT DISCLAIMER: By choosing to enable the JUCE_USE_MP3AUDIOFORMAT flag and to compile
    this MP3 code into your software, you do so AT YOUR OWN RISK! By doing so, you are agreeing
    that Raw Material Software is in no way responsible for any patent, copyright, or other
    legal issues that you may suffer as a result.

    The code in juce_MP3AudioFormat.cpp is NOT guaranteed to be free from infringements of 3rd-party
    intellectual property. If you wish to use it, please seek your own independent advice about the
    legality of doing so. If you are not willing to accept full responsibility for the consequences
    of using this code, then do not enable this setting.
*/
#define JUCE_USE_MP3AUDIOFORMAT 0

/** Config: JUCE_USE_LAME_AUDIO_FORMAT
    Enables the LameEncoderAudioFormat class.
*/
#define JUCE_USE_LAME_AUDIO_FORMAT 1

/** Config: JUCE_USE_WINDOWS_MEDIA_FORMAT
    Enables the Windows Media SDK codecs.
*/
#define JUCE_USE_WINDOWS_MEDIA_FORMAT 0

#endif // CARLA_JUCE_AUDIO_FORMATS_APPCONFIG_H_INCLUDED
