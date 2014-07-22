/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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
extern void carla_register_native_plugin_bypass();
extern void carla_register_native_plugin_lfo();
extern void carla_register_native_plugin_midigain();
extern void carla_register_native_plugin_midisplit();
extern void carla_register_native_plugin_midithrough();
extern void carla_register_native_plugin_miditranspose();
extern void carla_register_native_plugin_nekofilter();

// Audio File
extern void carla_register_native_plugin_audiofile();

// MIDI File
extern void carla_register_native_plugin_midifile();

#ifndef CARLA_OS_WIN
// Carla
extern void carla_register_native_plugin_carla();

// External-UI plugins
extern void carla_register_native_plugin_bigmeter();
extern void carla_register_native_plugin_notes();
#endif

#ifdef WANT_ZYNADDSUBFX
// ZynAddSubFX
extern void carla_register_native_plugin_zynaddsubfx_fx();
extern void carla_register_native_plugin_zynaddsubfx_synth();
#endif

void carla_register_all_plugins()
{
    // Simple plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_lfo();
    carla_register_native_plugin_midigain();
    carla_register_native_plugin_midisplit();
    carla_register_native_plugin_midithrough();
    carla_register_native_plugin_miditranspose();
    carla_register_native_plugin_nekofilter();

    // Audio File
    carla_register_native_plugin_audiofile();

    // MIDI File
    carla_register_native_plugin_midifile();

#ifndef CARLA_OS_WIN
    // Carla
    carla_register_native_plugin_carla();

    // External-UI plugins
    carla_register_native_plugin_bigmeter();
    carla_register_native_plugin_notes();
#endif

#ifdef WANT_ZYNADDSUBFX
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx_fx();
    carla_register_native_plugin_zynaddsubfx_synth();
#endif
}
