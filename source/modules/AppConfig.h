
#ifndef CARLA_JUCE_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_APPCONFIG_H_INCLUDED

// --------------------------------------------------------------------------------------------------------------------
// Check OS

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
# define APPCONFIG_OS_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define APPCONFIG_OS_WIN32
#elif defined(__APPLE__)
# define APPCONFIG_OS_MAC
#elif defined(__HAIKU__)
# define APPCONFIG_OS_HAIKU
#elif defined(__linux__) || defined(__linux)
# define APPCONFIG_OS_LINUX
#else
# warning Unsupported platform!
#endif

#if defined(APPCONFIG_OS_WIN32) || defined(APPCONFIG_OS_WIN64)
# define APPCONFIG_OS_WIN
#elif defined(APPCONFIG_OS_LINUX) || defined(APPCONFIG_OS_MAC)
# define APPCONFIG_OS_UNIX
#endif

// --------------------------------------------------------------------------------------------------------------------

// always enabled
#define JUCE_MODULE_AVAILABLE_juce_core                  1

// always disabled
#define JUCE_MODULE_AVAILABLE_juce_audio_plugin_client   0
#define JUCE_MODULE_AVAILABLE_juce_audio_utils           0
#define JUCE_MODULE_AVAILABLE_juce_cryptography          0
#define JUCE_MODULE_AVAILABLE_juce_opengl                0
#define JUCE_MODULE_AVAILABLE_juce_video                 0

// also disabled
#define JUCE_MODULE_AVAILABLE_juce_audio_basics          0
#define JUCE_MODULE_AVAILABLE_juce_audio_devices         0
#define JUCE_MODULE_AVAILABLE_juce_audio_formats         0
#define JUCE_MODULE_AVAILABLE_juce_audio_processors      0
#define JUCE_MODULE_AVAILABLE_juce_data_structures       0
#define JUCE_MODULE_AVAILABLE_juce_events                0
#define JUCE_MODULE_AVAILABLE_juce_graphics              0
#define JUCE_MODULE_AVAILABLE_juce_gui_basics            0
#define JUCE_MODULE_AVAILABLE_juce_gui_extra             0

// misc
#define JUCE_DISABLE_JUCE_VERSION_PRINTING 1
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 0
#define JUCE_REPORT_APP_USAGE 0
#define JUCE_DISPLAY_SPLASH_SCREEN 0
#define JUCE_USE_DARK_SPLASH_SCREEN 0
#define JUCE_STRING_UTF_TYPE 8
#define JUCE_USE_VFORK 1

#if ! (defined(APPCONFIG_OS_MAC) || defined(APPCONFIG_OS_WIN))
# define JUCE_MODAL_LOOPS_PERMITTED 0
# define JUCE_AUDIOPROCESSOR_NO_GUI 1
#endif

// --------------------------------------------------------------------------------------------------------------------
// juce_core

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

/** Config: JUCE_ALLOW_STATIC_NULL_VARIABLES
    If disabled, this will turn off dangerous static globals like String::empty, var::null, etc
    which can cause nasty order-of-initialisation problems if they are referenced during static
    constructor code.
*/
#define JUCE_ALLOW_STATIC_NULL_VARIABLES 0

// --------------------------------------------------------------------------------------------------------------------

#endif // CARLA_JUCE_APPCONFIG_H_INCLUDED
