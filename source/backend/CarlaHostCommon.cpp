/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaHost.h"
#include "CarlaString.hpp"

#include "juce_audio_formats.h"

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
#ifdef VESTIGE_HEADER
        "<li>VST plugin support using VeSTige header by Javier Serrano Polo</li>"
#else
        "<li>VST plugin support using official VST SDK 2.4 [1]</li>"
#endif
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN)
        "<li>VST3 plugin support using official VST SDK 3.6 [1]</li>"
#endif
#ifdef CARLA_OS_MAC
        "<li>AU plugin support</li>"
#endif

        // Sample kit libraries
#ifdef HAVE_FLUIDSYNTH
        "<li>FluidSynth library for SF2 support</li>"
#endif
#ifdef HAVE_LINUXSAMPLER
        "<li>LinuxSampler library for GIG and SFZ support [2]</li>"
#endif

        // Internal plugins
        "<li>NekoFilter plugin code based on lv2fil by Nedko Arnaudov and Fons Adriaensen</li>"
#ifdef WANT_ZYNADDSUBFX
        "<li>ZynAddSubFX plugin code</li>"
#endif

        // misc libs
        "<li>base64 utilities based on code by Ren\u00E9 Nyffenegger</li>"
#ifdef CARLA_OS_MAC
        "<li>sem_timedwait for Mac OS by Keith Shortridge</li>"
#endif
        "<li>liblo library for OSC support</li>"
        "<li>rtmempool library by Nedko Arnaudov"
        "<li>serd, sord, sratom and lilv libraries for LV2 discovery</li>"
#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
        "<li>RtAudio and RtMidi libraries for extra Audio and MIDI support</li>"
#endif

        // end
        "</ul>"

        "<p>"
#if defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN) || ! defined(VESTIGE_HEADER)
        // Required by VST SDK
        "&nbsp;[1] Trademark of Steinberg Media Technologies GmbH.<br/>"
#endif
#ifdef HAVE_LINUXSAMPLER
        // LinuxSampler GPL exception
        "&nbsp;[2] Using LinuxSampler code in commercial hardware or software products is not allowed without prior written authorization by the authors."
#endif
        "</p>"
        ;
    }

    return retText;
}

const char* carla_get_juce_version()
{
    carla_debug("carla_get_juce_version()");

    static CarlaString retVersion;

    if (retVersion.isEmpty())
    {
        if (const char* const version = juce::SystemStats::getJUCEVersion().toRawUTF8())
            retVersion = version+6;
        else
            retVersion = "3.0";
    }

    return retVersion;
}

const char* carla_get_supported_file_extensions()
{
    carla_debug("carla_get_supported_file_extensions()");

    static CarlaString retText;

    if (retText.isEmpty())
    {
        retText =
        // Base types
        "*.carxp;*.carxs"
        // MIDI files
        ";*.mid;*.midi"
#ifdef HAVE_FLUIDSYNTH
        // fluidsynth (sf2)
        ";*.sf2"
#endif
#ifdef HAVE_LINUXSAMPLER
        // linuxsampler (gig and sfz)
        ";*.gig;*.sfz"
#endif
#ifdef WANT_ZYNADDSUBFX
        // zynaddsubfx presets
        ";*.xmz;*.xiz"
#endif
        ;

#ifndef BUILD_BRIDGE
        // Audio files
        {
            using namespace juce;

            AudioFormatManager afm;
            afm.registerBasicFormats();

            String juceFormats;

            for (AudioFormat **it=afm.begin(), **end=afm.end(); it != end; ++it)
            {
                const StringArray& exts((*it)->getFileExtensions());

                for (String *eit=exts.begin(), *eend=exts.end(); eit != eend; ++eit)
                    juceFormats += String(";*" + (*eit)).toRawUTF8();
            }

            retText += juceFormats.toRawUTF8();
        }
#endif
    }

    return retText;
}

// -------------------------------------------------------------------------------------------------------------------
