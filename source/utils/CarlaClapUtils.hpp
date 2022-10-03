/*
 * Carla CLAP utils
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_CLAP_UTILS_HPP_INCLUDED
#define CARLA_CLAP_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"
#include "CarlaUtils.hpp"

#include "clap/entry.h"
#include "clap/plugin-factory.h"
#include "clap/plugin-features.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/note-ports.h"
#include "clap/ext/gui.h"
#include "clap/ext/params.h"
#include "clap/ext/state.h"
#include "clap/ext/timer-support.h"

#if defined(CARLA_OS_WIN)
# define CLAP_WINDOW_API_NATIVE CLAP_WINDOW_API_WIN32
#elif defined(CARLA_OS_MAC)
# define CLAP_WINDOW_API_NATIVE CLAP_WINDOW_API_COCOA
#elif defined(HAVE_X11)
# define CLAP_WINDOW_API_NATIVE CLAP_WINDOW_API_X11
#endif

// --------------------------------------------------------------------------------------------------------------------

extern "C" {

typedef struct clap_audio_buffer_const {
   // Either data32 or data64 pointer will be set.
   const float* const* data32;
   const double* const* data64;
   uint32_t channel_count;
   uint32_t latency;       // latency from/to the audio interface
   uint64_t constant_mask;
} clap_audio_buffer_const_t;

}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

static inline
PluginCategory getPluginCategoryFromClapFeatures(const char* const* const features) noexcept
{
    // 1st pass for main categories
    for (uint32_t i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_INSTRUMENT) == 0)
            return PLUGIN_CATEGORY_SYNTH;
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_NOTE_EFFECT) == 0)
            return PLUGIN_CATEGORY_UTILITY;
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_ANALYZER) == 0)
            return PLUGIN_CATEGORY_UTILITY;
    }

    // 2nd pass for FX sub categories
    /*
    #define CLAP_PLUGIN_FEATURE_DEESSER "de-esser"
    #define CLAP_PLUGIN_FEATURE_PHASE_VOCODER "phase-vocoder"
    #define CLAP_PLUGIN_FEATURE_GRANULAR "granular"
    #define CLAP_PLUGIN_FEATURE_FREQUENCY_SHIFTER "frequency-shifter"
    #define CLAP_PLUGIN_FEATURE_PITCH_SHIFTER "pitch-shifter"
    #define CLAP_PLUGIN_FEATURE_TREMOLO "tremolo"
    #define CLAP_PLUGIN_FEATURE_GLITCH "glitch"
    */
    for (uint32_t i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_DELAY) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_REVERB) == 0)
        {
            return PLUGIN_CATEGORY_DELAY;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_EQUALIZER) == 0)
        {
            return PLUGIN_CATEGORY_EQ;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_FILTER) == 0)
        {
            return PLUGIN_CATEGORY_FILTER;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_DISTORTION) == 0)
        {
            return PLUGIN_CATEGORY_DISTORTION;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_COMPRESSOR) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_LIMITER) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_MASTERING) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_MIXING) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_TRANSIENT_SHAPER) == 0)
        {
            return PLUGIN_CATEGORY_DYNAMICS;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_CHORUS) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_FLANGER) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_PHASER) == 0
        )
        {
            return PLUGIN_CATEGORY_MODULATOR;
        }
        if (std::strcmp(features[i], CLAP_PLUGIN_FEATURE_PITCH_CORRECTION) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_RESTORATION) == 0 ||
            std::strcmp(features[i], CLAP_PLUGIN_FEATURE_UTILITY) == 0
        )
        {
            return PLUGIN_CATEGORY_UTILITY;
        }
    }

    return PLUGIN_CATEGORY_OTHER;
}

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_CLAP_UTILS_HPP_INCLUDED