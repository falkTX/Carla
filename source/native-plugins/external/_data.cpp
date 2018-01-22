/*
 * Carla Native Plugins
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_EXTERNAL_PLUGINS_INCLUDED_DIRECTLY
# define DESCFUNCS \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr

static const NativePluginDescriptor sNativePluginDescriptors[] = {
#endif

// --------------------------------------------------------------------------------------------------------------------
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
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 1,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "Kars",
    /* label     */ "kars",
    /* maker     */ "falkTX, Chris Cannam",
    /* copyright */ "ISC",
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

// --------------------------------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------------------------------
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
# ifndef SKIP_ZYN_SYNTH
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_SYNTH,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
# ifdef HAVE_ZYN_UI_DEPS
                                                  |NATIVE_PLUGIN_HAS_UI
# endif
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
# endif // ! SKIP_ZYN_SYNTH
#endif // HAVE_ZYN_DEPS

// --------------------------------------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------------------------------------

#ifndef CARLA_EXTERNAL_PLUGINS_INCLUDED_DIRECTLY
}
#endif
