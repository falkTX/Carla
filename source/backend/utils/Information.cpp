/*
 * Carla Plugin Host
 * Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.h"

#include "CarlaString.hpp"

#include "rtaudio/RtAudio.h"
#include "rtmidi/RtMidi.h"

#include "water/files/File.h"

// -------------------------------------------------------------------------------------------------------------------

const char* carla_get_complete_license_text()
{
    carla_debug("carla_get_complete_license_text()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        retText =
        "<p>This current Carla build is using the following features and 3rd-party code:</p>"
        "<ul>"

        // Plugin formats
        "<li>LADSPA plugin support</li>"
        "<li>DSSI plugin support</li>"
        "<li>LV2 plugin support</li>"
        "<li>VST2 plugin support using VeSTige header by Javier Serrano Polo</li>"

        // Sample kit libraries
#ifdef HAVE_FLUIDSYNTH
        "<li>FluidSynth library for SF2/3 support</li>"
#endif
        "<li>SFZero module for SFZ support</li>"

        // misc libs
        "<li>base64 utilities based on code by Ren\u00E9 Nyffenegger</li>"
        "<li>liblo library for OSC support</li>"
        "<li>rtmempool library by Nedko Arnaudov"
        "<li>serd, sord, sratom and lilv libraries for LV2 discovery</li>"
        "<li>RtAudio v" RTAUDIO_VERSION " and RtMidi v" RTMIDI_VERSION " for native Audio and MIDI support</li>"

        // Internal plugins
        "<li>MIDI Sequencer UI code by Perry Nguyen</li>"

        // External plugins
#ifdef HAVE_EXTERNAL_PLUGINS
        "<li>Nekobi plugin code based on nekobee by Sean Bolton and others</li>"
        "<li>VectorJuice and WobbleJuice plugin code by Andre Sklenar</li>"
# ifdef HAVE_ZYN_DEPS
        "<li>ZynAddSubFX plugin code by Mark McCurry and Nasca Octavian Paul</li>"
# endif
#endif // HAVE_EXTERNAL_PLUGINS

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
#ifdef HAVE_ZYN_DEPS
        "xmz", "xiz",
#endif
#if defined(CARLA_OS_MAC)
        "vst",
#else
        "dll",
        "so",
#endif

        // Audio files
#ifdef HAVE_SNDFILE
        "aif", "aifc", "aiff", "au", "bwf", "flac", "htk", "iff", "mat4", "mat5", "oga", "ogg",
        "paf", "pvf", "pvf5", "sd2", "sf", "snd", "svx", "vcc", "w64", "wav", "xi",
#endif
#ifdef HAVE_FFMPEG
        "3g2", "3gp", "aac", "ac3", "amr", "ape", "mp2", "mp3", "mpc", "wma",
# ifdef HAVE_SNDFILE
        // FFmpeg without sndfile
        "flac", "oga", "ogg", "w64", "wav",
# endif
#endif

        // MIDI files
        "mid", "midi",

        // SFZ
        "sfz",

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
        nullptr
    };

    return features;
}

// -------------------------------------------------------------------------------------------------------------------

#include "../CarlaHostCommon.cpp"

// -------------------------------------------------------------------------------------------------------------------
