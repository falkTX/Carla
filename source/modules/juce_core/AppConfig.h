/*
  ==============================================================================

   Build options for juce_core static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_CORE_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_CORE_APPCONFIG_H_INCLUDED

#include "system/juce_TargetPlatform.h"

//=============================================================================
/** Config: JUCE_FORCE_DEBUG

    Normally, JUCE_DEBUG is set to 1 or 0 based on compiler and project settings,
    but if you define this value, you can override this to force it to be true or false.
*/
#define JUCE_FORCE_DEBUG 0

//=============================================================================
/** Config: JUCE_LOG_ASSERTIONS

    If this flag is enabled, the the jassert and jassertfalse macros will always use Logger::writeToLog()
    to write a message when an assertion happens.

    Enabling it will also leave this turned on in release builds. When it's disabled,
    however, the jassert and jassertfalse macros will not be compiled in a
    release build.

    @see jassert, jassertfalse, Logger
*/
#define JUCE_LOG_ASSERTIONS 1

//=============================================================================
/** Config: JUCE_CHECK_MEMORY_LEAKS

    Enables a memory-leak check for certain objects when the app terminates. See the LeakedObjectDetector
    class and the JUCE_LEAK_DETECTOR macro for more details about enabling leak checking for specific classes.
*/
#ifdef DEBUG
 #define JUCE_CHECK_MEMORY_LEAKS 1
#else
 #define JUCE_CHECK_MEMORY_LEAKS 0
#endif

//=============================================================================
/** Config: JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES

    In a Visual C++  build, this can be used to stop the required system libs being
    automatically added to the link stage.
*/
#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES 0

/** Config: JUCE_INCLUDE_ZLIB_CODE
    This can be used to disable Juce's embedded 3rd-party zlib code.
    You might need to tweak this if you're linking to an external zlib library in your app,
    but for normal apps, this option should be left alone.

    If you disable this, you might also want to set a value for JUCE_ZLIB_INCLUDE_PATH, to
    specify the path where your zlib headers live.
*/
#define JUCE_INCLUDE_ZLIB_CODE 1

/** Config: JUCE_USE_CURL
    Enables http/https support via libcurl (Linux only). Enabling this will add an additional
    run-time dynmic dependency to libcurl.

    If you disable this then https/ssl support will not be available on linux.
*/
#define JUCE_USE_CURL 0

/*  Config: JUCE_CATCH_UNHANDLED_EXCEPTIONS
    If enabled, this will add some exception-catching code to forward unhandled exceptions
    to your JUCEApplicationBase::unhandledException() callback.
*/
#define JUCE_CATCH_UNHANDLED_EXCEPTIONS 0

// misc
#define JUCE_DISABLE_JUCE_VERSION_PRINTING 1
#define JUCE_STANDALONE_APPLICATION 0
#define JUCE_STRING_UTF_TYPE 8
#define JUCE_USE_VFORK 1

// not used/wanted
#define JUCE_USE_XRANDR 0
#define JUCE_USE_XINERAMA 0

#if ! (JUCE_MAC || JUCE_WINDOWS)
# define JUCE_MODAL_LOOPS_PERMITTED  0
# define JUCE_AUDIO_PROCESSOR_NO_GUI 1
#endif

// always enabled
#define JUCE_MODULE_AVAILABLE_juce_audio_basics          1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats         1
#define JUCE_MODULE_AVAILABLE_juce_core                  1

// always disabled
#define JUCE_MODULE_AVAILABLE_juce_audio_plugin_client   0
#define JUCE_MODULE_AVAILABLE_juce_audio_utils           0
#define JUCE_MODULE_AVAILABLE_juce_cryptography          0
#define JUCE_MODULE_AVAILABLE_juce_opengl                0
#define JUCE_MODULE_AVAILABLE_juce_video                 0

// conditional
#if JUCE_MAC || JUCE_WINDOWS
# define JUCE_MODULE_AVAILABLE_juce_audio_devices        1
# define JUCE_MODULE_AVAILABLE_juce_audio_processors     1
# define JUCE_MODULE_AVAILABLE_juce_data_structures      1
# define JUCE_MODULE_AVAILABLE_juce_events               1
# define JUCE_MODULE_AVAILABLE_juce_graphics             1
# define JUCE_MODULE_AVAILABLE_juce_gui_basics           1
# define JUCE_MODULE_AVAILABLE_juce_gui_extra            1
#else
# define JUCE_MODULE_AVAILABLE_juce_audio_devices        0
# define JUCE_MODULE_AVAILABLE_juce_audio_processors     0
# define JUCE_MODULE_AVAILABLE_juce_data_structures      0
# define JUCE_MODULE_AVAILABLE_juce_events               0
# define JUCE_MODULE_AVAILABLE_juce_graphics             0
# define JUCE_MODULE_AVAILABLE_juce_gui_basics           0
# define JUCE_MODULE_AVAILABLE_juce_gui_extra            0
#endif

#endif // CARLA_JUCE_CORE_APPCONFIG_H_INCLUDED
