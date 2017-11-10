/*
 * Carla Native Plugins
 * Copyright (C) 2012-2017 Filipe Coelho <falktx@falktx.com>
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

#ifdef CARLA_OS_WIN
# define DISABLE_PLUGINS_FOR_WINDOWS_BUILD
# undef HAVE_PYQT
#endif

#undef DESCFUNCS
#define DESCFUNCS \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr

static const NativePluginDescriptor sNativePluginDescriptors[] = {

// --------------------------------------------------------------------------------------------------------------------
// Simple plugins

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

// --------------------------------------------------------------------------------------------------------------------
// MIDI sequencer

#ifdef HAVE_PYQT
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

// --------------------------------------------------------------------------------------------------------------------
// Carla

#ifdef HAVE_PYQT
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
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
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Carla-Rack (no midi out)",
    /* label     */ "carlarack-nomidiout",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
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
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
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
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
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
                                                  //|NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
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
#endif

// --------------------------------------------------------------------------------------------------------------------
// External-UI plugins

#ifdef HAVE_PYQT
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

};

#undef DESCFUNCS

// --------------------------------------------------------------------------------------------------------------------

const NativePluginDescriptor* carla_get_native_plugins_data(uint32_t* count)
{
    CARLA_SAFE_ASSERT_RETURN(count != nullptr, nullptr);

    *count = static_cast<uint32_t>(sizeof(sNativePluginDescriptors)/sizeof(NativePluginDescriptor));
    return sNativePluginDescriptors;
}

// --------------------------------------------------------------------------------------------------------------------
