// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaUtils.h"

#if defined(HAVE_FLUIDSYNTH) && !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
# include <fluidsynth.h>
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
# pragma GCC diagnostic ignored "-Wundef"
# pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#ifdef USING_RTAUDIO
# include "rtaudio/RtAudio.h"
# include "rtmidi/RtMidi.h"
#endif

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

#include "extra/String.hpp"
#include "water/files/File.h"

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_complete_license_text()
{
    carla_debug("carla_get_complete_license_text()");

    static String retText;

    if (retText.isEmpty())
    {
        retText =
        "<p>This current Carla build is using the following features and 3rd-party code:</p>"
        "<ul>"

        // Plugin formats
        "<li>LADSPA plugin support</li>"
        "<li>DSSI plugin support</li>"
        "<li>LV2 plugin support</li>"
        "<li>VST2 plugin support (using VeSTige header by Javier Serrano Polo)</li>"
        "<li>VST3 plugin support (using Travesty header files)</li>"
       #ifdef CARLA_OS_MAC
        "<li>AU plugin support (discovery only)</li>"
       #endif
       #ifdef HAVE_YSFX
        "<li>JSFX plugin support (using ysfx)</li>"
       #endif

        // Sample kit libraries
       #if defined(HAVE_FLUIDSYNTH) && !defined(BUILD_BRIDGE_ALTERNATIVE_ARCH)
        "<li>FluidSynth library v" FLUIDSYNTH_VERSION " for SF2/3 support</li>"
       #endif
        "<li>SFZero module for SFZ support</li>"

        // misc libs
        "<li>base64 utilities based on code by Ren\u00E9 Nyffenegger</li>"
        "<li>dr_mp3 for mp3 file support</li>"
       #ifdef HAVE_LIBLO
        "<li>liblo library for OSC support</li>"
       #endif
       #ifdef HAVE_SNDFILE
        "<li>libsndfile library for base audio file support</li>"
       #endif
        "<li>rtmempool library by Nedko Arnaudov</li>"
        "<li>serd, sord, sratom and lilv libraries for LV2 discovery</li>"
       #ifdef USING_RTAUDIO
        "<li>RtAudio v" RTAUDIO_VERSION " and RtMidi v" RTMIDI_VERSION " for native Audio and MIDI support</li>"
       #endif
        "<li>zita-resampler for audio file sample rate resampling</li>"

        // Internal plugins
        "<li>MIDI Sequencer UI code by Perry Nguyen</li>"

        // External plugins
      #ifdef HAVE_EXTERNAL_PLUGINS
        "<li>Nekobi plugin code based on nekobee by Sean Bolton and others</li>"
        "<li>VectorJuice and WobbleJuice plugin code by Andre Sklenar</li>"
       #ifdef HAVE_ZYN_DEPS
        "<li>ZynAddSubFX plugin code by Mark McCurry and Nasca Octavian Paul</li>"
       #endif
      #endif

        // end
        "</ul>";
    }

    return retText;
}

const char* const* carla_get_supported_file_extensions()
{
    carla_debug("carla_get_supported_file_extensions()");

    // NOTE: please keep in sync with CarlaEngine::loadFile!!
    static const char* const extensions[] = {
        // Base types
        "carxp", "carxs",

        // plugin files and resources
       #ifdef HAVE_FLUIDSYNTH
        "sf2", "sf3",
       #endif
       #ifdef HAVE_FLUIDSYNTH_INSTPATCH
        "dls", "gig",
       #endif
       #ifdef HAVE_ZYN_DEPS
        "xmz", "xiz",
       #endif
       #ifdef CARLA_OS_MAC
        "vst",
       #else
        "dll",
        "so",
       #endif
        "vst3",
        "clap",

        // Audio files
       #ifdef HAVE_SNDFILE
        "aif", "aifc", "aiff", "au", "bwf", "flac", "htk", "iff", "mat4", "mat5", "oga", "ogg", "opus",
        "paf", "pvf", "pvf5", "sd2", "sf", "snd", "svx", "vcc", "w64", "wav", "xi",
       #endif
      #ifdef HAVE_FFMPEG
        "3g2", "3gp", "aac", "ac3", "amr", "ape", "mp2", "mp3", "mpc", "wma",
       #ifndef HAVE_SNDFILE
        // FFmpeg without sndfile
        "flac", "oga", "ogg", "w64", "wav",
       #endif
      #else
        // dr_mp3
        "mp3",
      #endif

        // MIDI files
        "mid", "midi",

        // SFZ
        "sfz",

       #ifdef HAVE_YSFX
        // JSFX
        "jsfx",
       #endif

        // terminator
        nullptr
    };

    return extensions;
}

const char* const* carla_get_supported_features()
{
    carla_debug("carla_get_supported_features()");

    static const char* const features[] = {
       #ifdef HAVE_FLUIDSYNTH
        "sf2",
       #endif
       #ifdef HAVE_FLUIDSYNTH_INSTPATCH
        "dls", "gig",
       #endif
       #ifdef HAVE_HYLIA
        "link",
       #endif
       #ifdef HAVE_LIBLO
        "osc",
       #endif
       #if defined(HAVE_LIBMAGIC) || defined(CARLA_OS_WIN)
        "bridges",
       #endif
       #ifdef HAVE_PYQT
        "gui",
       #endif
       #ifdef HAVE_YSFX
        "jsfx",
       #endif
        nullptr
    };

    return features;
}

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_library_filename()
{
    carla_debug("carla_get_library_filename()");

    static String ret;

    if (ret.isEmpty())
    {
        using water::File;
        ret = File(File::getSpecialLocation(File::currentExecutableFile)).getFullPathName().toRawUTF8();
    }

    return ret;
}

const char* carla_get_library_folder()
{
    carla_debug("carla_get_library_folder()");

    static String ret;

    if (ret.isEmpty())
    {
        using water::File;
        ret = File(File::getSpecialLocation(File::currentExecutableFile).getParentDirectory()).getFullPathName().toRawUTF8();
    }

    return ret;
}

// -------------------------------------------------------------------------------------------------------------------
