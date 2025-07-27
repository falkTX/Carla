// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNative.h"
#include "CarlaMIDI.h"
#include "CarlaUtils.hpp"

#undef DESCFUNCS_WITHCV
#undef DESCFUNCS_WITHOUTCV
#define DESCFUNCS_WITHCV \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr, nullptr, nullptr, nullptr, \
    nullptr, nullptr
#define DESCFUNCS_WITHOUTCV \
    DESCFUNCS_WITHCV, 0, 0, nullptr, nullptr, 0, 0

static const NativePluginDescriptor sNativePluginDescriptors[] = {

// --------------------------------------------------------------------------------------------------------------------
// Simple plugins

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 1,
    /* audioOuts */ 1,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "Audio Gain (Mono)",
    /* label     */ "audiogain",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 3,
    /* paramOuts */ 0,
    /* name      */ "Audio Gain (Stereo)",
    /* label     */ "audiogain_s",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_CONTROL_VOLTAGE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 1,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ "CV to Audio",
    /* label     */ "cv2audio",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHCV,
    /* cvIns      */ 1,
    /* cvOuts     */ 0,
    /* bufnamefn  */ nullptr,
    /* bufrangefn */ nullptr,
    /* ui_width   */ 0,
    /* ui_height  */ 0
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 4,
    /* audioOuts */ 4,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 2,
    /* paramOuts */ 0,
    /* name      */ "Quad-Panner",
    /* label     */ "quadpanner",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 2,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "MIDI Channel A/B",
    /* label     */ "midichanab",
    /* maker     */ "Milk Brewster",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
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
    DESCFUNCS_WITHOUTCV
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
    DESCFUNCS_WITHOUTCV
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_USES_CONTROL_VOLTAGE),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_ALL_SOUND_OFF,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 4,
    /* paramOuts */ 0,
    /* name      */ "MIDI to CV",
    /* label     */ "midi2cv",
    /* maker     */ "falkTX, Bram Giesen, Jarno Verheesen",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHCV,
    /* cvIns      */ 0,
    /* cvOuts     */ 3,
    /* bufnamefn  */ nullptr,
    /* bufrangefn */ nullptr,
    /* ui_width   */ 0,
    /* ui_height  */ 0
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ NATIVE_PLUGIN_IS_RTSAFE,
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 2,
    /* paramOuts */ 0,
    /* name      */ "MIDI Transpose",
    /* label     */ "miditranspose",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
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
    /* name      */ "MIDI Channelize",
    /* label     */ "midichannelize",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},

// --------------------------------------------------------------------------------------------------------------------
// Audio file

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE
                                                  |NATIVE_PLUGIN_USES_CONTROL_VOLTAGE
                                                  |NATIVE_PLUGIN_USES_TIME),
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
    DESCFUNCS_WITHCV,
    /* cvIns      */ 0,
    /* cvOuts     */ 1,
    /* bufnamefn  */ nullptr,
    /* bufrangefn */ nullptr,
    /* ui_width   */ 0,
    /* ui_height  */ 0
},

// --------------------------------------------------------------------------------------------------------------------
// MIDI file and sequencer

{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE
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
    DESCFUNCS_WITHOUTCV
},
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
    DESCFUNCS_WITHOUTCV
},
#endif

// --------------------------------------------------------------------------------------------------------------------
// Carla

#ifdef HAVE_PYQT
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Rack",
    /* label     */ "carlarack",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 0,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Rack (no midi out)",
    /* label     */ "carlarack-nomidiout",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay",
    /* label     */ "carlapatchbay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 3,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay (sidechain)",
    /* label     */ "carlapatchbay3s",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 16,
    /* audioOuts */ 16,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay (16chan)",
    /* label     */ "carlapatchbay16",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 32,
    /* audioOuts */ 32,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay (32chan)",
    /* label     */ "carlapatchbay32",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 64,
    /* audioOuts */ 64,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay (64chan)",
    /* label     */ "carlapatchbay64",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_OTHER,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_SYNTH
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_MAIN_THREAD
                                                  |NATIVE_PLUGIN_USES_CONTROL_VOLTAGE
                                                  |NATIVE_PLUGIN_USES_STATE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(NATIVE_PLUGIN_SUPPORTS_EVERYTHING),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 100,
    /* paramOuts */ 10,
    /* name      */ "Carla-Patchbay (CV)",
    /* label     */ "carlapatchbaycv",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHCV,
    /* cvIns      */ 5,
    /* cvOuts     */ 5,
    /* bufnamefn  */ nullptr,
    /* bufrangefn */ nullptr,
    /* ui_width   */ 0,
    /* ui_height  */ 0
},
#endif

// --------------------------------------------------------------------------------------------------------------------
// External-UI plugins

#ifdef HAVE_PYQT
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
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
    DESCFUNCS_WITHOUTCV
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
    DESCFUNCS_WITHOUTCV
},
{
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 0,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 2,
    /* paramOuts */ 2,
    /* name      */ "XY Controller",
    /* label     */ "xycontroller",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    DESCFUNCS_WITHOUTCV
},
#endif

#ifdef HAVE_EXTERNAL_PLUGINS
# define CARLA_EXTERNAL_PLUGINS_INCLUDED_DIRECTLY
# include "external/_data.cpp"
#endif

};

#undef DESCFUNCS_WITHCV
#undef DESCFUNCS_WITHOUTCV

// --------------------------------------------------------------------------------------------------------------------

const NativePluginDescriptor* carla_get_native_plugins_data(uint32_t* count)
{
    CARLA_SAFE_ASSERT_RETURN(count != nullptr, nullptr);

    *count = static_cast<uint32_t>(sizeof(sNativePluginDescriptors)/sizeof(NativePluginDescriptor));
    return sNativePluginDescriptors;
}

// --------------------------------------------------------------------------------------------------------------------
