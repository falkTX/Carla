/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.h"
#include "CarlaMIDI.h"
#include "CarlaUtils.hpp"

#undef DESCFUNCS
#define DESCFUNCS \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr

static const NativePluginDescriptor sNativePluginDescriptors[] = {

// -----------------------------------------------------------------------
// Simple plugins

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_NONE,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 1,
    /* audioOuts */ 1,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Bypass",
    /* label     */ "bypass",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 5-1,
    /* paramOuts */ 1,
    /* name      */ "LFO",
    /* label     */ "lfo",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Channel Filter",
    /* label     */ "midichanfilter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Gain",
    /* label     */ "midigain",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ MAX_MIDI_CHANNELS,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Join",
    /* label     */ "midijoin",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ MAX_MIDI_CHANNELS,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Split",
    /* label     */ "midisplit",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Through",
    /* label     */ "midithrough",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "MIDI Transpose",
    /* label     */ "miditranspose",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},


// -----------------------------------------------------------------------
// Audio file

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "Audio File",
    /* label     */ "audiofile",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},

// -----------------------------------------------------------------------
// MIDI file and sequencer

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI File",
    /* label     */ "midifile",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
#ifdef CARLA_OS_LINUX
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 1,
    /* paramIns  */ 4,
    /* paramOuts */ 0,
    /* name      */ "MIDI Pattern",
    /* label     */ "midipattern",
    /* maker     */ "falkTX, tatch",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
#endif

// -----------------------------------------------------------------------
// Carla

#ifndef CARLA_OS_WIN
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Rack",
    /* label     */ "carlarack",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay",
    /* label     */ "carlapatchbay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 3,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (sidechain)",
    /* label     */ "carlapatchbay3s",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 16,
    /* audioOuts */ 16,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (16chan)",
    /* label     */ "carlapatchbay16",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 32,
    /* audioOuts */ 32,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Patchbay (32chan)",
    /* label     */ "carlapatchbay32",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
#endif // CARLA_OS_WIN

// -----------------------------------------------------------------------
// DISTRHO Plugins

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_EQ,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "3 Band EQ",
    /* label     */ "3bandeq",
    /* maker     */ "falkTX, Michael Gruhn",
    /* copyright */ "LGPL",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_EQ,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 6,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "3 Band Splitter",
    /* label     */ "3bandsplitter",
    /* maker     */ "falkTX, Michael Gruhn",
    /* copyright */ "LGPL",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
#if defined(HAVE_DGL) && ! defined(CARLA_OS_WIN)
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 9,
    /* paramOuts */ 0,
    /* name      */ "MVerb",
    /* label     */ "mverb",
    /* maker     */ "falkTX, Martin Eastwood",
    /* copyright */ "GPL v3+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_SYNTH,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_IS_SYNTH),
#endif
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES
                                                     |NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF),
    /* audioIns  */ 0,
    /* audioOuts */ 1,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 8,
    /* paramOuts */ 0,
    /* name      */ "Nekobi",
    /* label     */ "nekobi",
    /* maker     */ "falkTX, Sean Bolton and others",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 2,
    /* paramOuts */ 0,
    /* name      */ "Ping Pong Pan",
    /* label     */ "pingpongpan",
    /* maker     */ "falkTX, Michael Gruhn",
    /* copyright */ "LGPL",
    DESCFUNCS
},
#ifdef HAVE_DGL
#ifdef HAVE_PROJECTM
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 1,
    /* audioOuts */ 1,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "ProM",
    /* label     */ "prom",
    /* maker     */ "falkTX",
    /* copyright */ "LGPL",
    DESCFUNCS
},
#endif // HAVE_PROJECTM
#endif // HAVE_DGL

// -----------------------------------------------------------------------
// DISTRHO plugins (Juice)

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DYNAMICS,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID
                                                  |NATIVE_PLUGIN_USES_TIME),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_TIME),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "WobbleJuice",
    /* label     */ "wobblejuice",
    /* maker     */ "Andre Sklenar",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
#ifdef HAVE_DGL
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_PARENT_ID
                                                  |NATIVE_PLUGIN_USES_TIME),
#else
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_TIME),
#endif
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 8,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 13,
    /* paramOuts */ 4,
    /* name      */ "VectorJuice",
    /* label     */ "vectorjuice",
    /* maker     */ "Andre Sklenar",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},

// -----------------------------------------------------------------------
// External-UI plugins

#ifndef CARLA_OS_WIN
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 2,
    /* paramOuts */ 2,
    /* name      */ "Big Meter",
    /* label     */ "bigmeter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "Notes",
    /* label     */ "notes",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
#endif

// -----------------------------------------------------------------------
// ZynAddSubFX

#ifdef HAVE_ZYN_DEPS
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynAlienWah",
    /* label     */ "zynalienwah",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 12-2,
    /* paramOuts */ 0,
    /* name      */ "ZynChorus",
    /* label     */ "zynchorus",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDistortion",
    /* label     */ "zyndistortion",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_FILTER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 10-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDynamicFilter",
    /* label     */ "zyndynamicfilter",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 7-2,
    /* paramOuts */ 0,
    /* name      */ "ZynEcho",
    /* label     */ "zynecho",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 15-2,
    /* paramOuts */ 0,
    /* name      */ "ZynPhaser",
    /* label     */ "zynphaser",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_USES_PANNING
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 13-2,
    /* paramOuts */ 0,
    /* name      */ "ZynReverb",
    /* label     */ "zynreverb",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
# ifdef CARLA_OS_LINUX
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
#  ifdef HAVE_ZYN_UI_DEPS
                                                  |NATIVE_PLUGIN_HAS_UI
#  endif
                                                  |NATIVE_PLUGIN_USES_MULTI_PROGS
                                                  |NATIVE_PLUGIN_USES_STATE),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_CONTROL_CHANGES
                                                     |NATIVE_PLUGIN_SUPPORTS_NOTE_AFTERTOUCH
                                                     |NATIVE_PLUGIN_SUPPORTS_PITCHBEND
                                                     |NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "ZynAddSubFX",
    /* label     */ "zynaddsubfx",
    /* maker     */ "falkTX, Mark McCurry, Nasca Octavian Paul",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
# endif // CARLA_OS_LINUX
#endif // HAVE_ZYN_DEPS


// -----------------------------------------------------------------------
// Experimental plugins

#ifdef HAVE_EXPERIMENTAL_PLUGINS
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_USES_STATE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 1,
    /* audioOuts */ 1,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "AT1",
    /* label     */ "at1",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_FILTER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 6,
    /* paramOuts */ 0,
    /* name      */ "BLS1",
    /* label     */ "bls1",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 4,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 10,
    /* paramOuts */ 0,
    /* name      */ "REV1 (Ambisonic)",
    /* label     */ "rev1-ambisonic",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 10,
    /* paramOuts */ 0,
    /* name      */ "REV1 (Stereo)",
    /* label     */ "rev1-stereo",
    /* maker     */ "falkTX, Fons Adriaensen",
    /* copyright */ "GPL v2+",
    DESCFUNCS
},
#endif // HAVE_EXPERIMENTAL_PLUGINS

};

#undef DESCFUNCS

// -----------------------------------------------------------------------

const NativePluginDescriptor* carla_get_native_plugins_data(uint32_t* count)
{
    CARLA_SAFE_ASSERT_RETURN(count != nullptr, nullptr);

    *count = static_cast<uint32_t>(sizeof(sNativePluginDescriptors)/sizeof(NativePluginDescriptor));
    return sNativePluginDescriptors;
}

// -----------------------------------------------------------------------
