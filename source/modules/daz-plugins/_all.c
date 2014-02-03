/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"

// Simple plugins
extern void carla_register_daz_plugin_bypass();
extern void carla_register_daz_plugin_lfo();

// Simple plugins
extern void carla_register_native_plugin_midiGain();
extern void carla_register_native_plugin_midiSplit();
extern void carla_register_native_plugin_midiThrough();
extern void carla_register_native_plugin_midiTranspose();
extern void carla_register_native_plugin_nekofilter();

// Simple plugins (C++)
extern void carla_register_native_plugin_sunvoxfile();

#ifndef CARLA_OS_WIN
extern void carla_register_native_plugin_carla();
#endif

#ifdef HAVE_JUCE
extern void carla_register_native_plugin_vex_fx();
extern void carla_register_native_plugin_vex_synth();
#endif

#ifdef WANT_AUDIOFILE
// AudioFile
extern void carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
// MidiFile
extern void carla_register_native_plugin_midifile();
#endif

#ifdef HAVE_DGL
// DISTRHO plugins (OpenGL)
extern void carla_register_native_plugin_3BandEQ();
extern void carla_register_native_plugin_3BandSplitter();
extern void carla_register_native_plugin_Nekobi();
extern void carla_register_native_plugin_PingPongPan();
extern void carla_register_native_plugin_StereoEnhancer();
#endif

// DISTRHO plugins (PyQt)
// extern void carla_register_native_plugin_BigMeter();
// extern void carla_register_native_plugin_BigMeterM();
// extern void carla_register_native_plugin_Notes();

#ifdef WANT_ZYNADDSUBFX
// ZynAddSubFX
extern void carla_register_native_plugin_zynaddsubfx_fx();
extern void carla_register_native_plugin_zynaddsubfx_synth();
#endif

void carla_register_all_plugins()
{
#if 0
    // Simple plugins
    carla_register_daz_plugin_bypass();
    carla_register_daz_plugin_lfo();
#endif

    // Simple plugins
    carla_register_native_plugin_midiGain();
    carla_register_native_plugin_midiSplit();
    carla_register_native_plugin_midiThrough();
    carla_register_native_plugin_midiTranspose();
    carla_register_native_plugin_nekofilter();

    // Simple plugins (C++)
    //carla_register_native_plugin_sunvoxfile(); // unfinished

#ifndef CARLA_OS_WIN
    carla_register_native_plugin_carla();
#endif

#ifdef HAVE_JUCE
    carla_register_native_plugin_vex_fx();
    carla_register_native_plugin_vex_synth();
#endif

#ifdef WANT_AUDIOFILE
    // AudioFile
    carla_register_native_plugin_audiofile();
#endif

#ifdef WANT_MIDIFILE
    // MidiFile
    carla_register_native_plugin_midifile();
#endif

#ifdef HAVE_DGL
    // DISTRHO plugins (OpenGL)
    carla_register_native_plugin_3BandEQ();
    carla_register_native_plugin_3BandSplitter();
    //carla_register_native_plugin_Nekobi();
    carla_register_native_plugin_PingPongPan();
    //carla_register_native_plugin_StereoEnhancer(); // unfinished
#endif

    // DISTRHO plugins (PyQt)
//     carla_register_native_plugin_BigMeter();
//     carla_register_native_plugin_BigMeterM();
//     carla_register_native_plugin_Notes();

#ifdef WANT_ZYNADDSUBFX
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx_fx();
    carla_register_native_plugin_zynaddsubfx_synth();
#endif
}
