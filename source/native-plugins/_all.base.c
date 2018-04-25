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

#include "CarlaDefines.h"
#include "CarlaNative.h"

#ifdef CARLA_OS_WIN
# undef HAVE_PYQT
#endif

// --------------------------------------------------------------------------------------------------------------------

// Simple plugins
extern void carla_register_native_plugin_bypass(void);
extern void carla_register_native_plugin_lfo(void);
extern void carla_register_native_plugin_midichanfilter(void);
extern void carla_register_native_plugin_midichanab(void);
extern void carla_register_native_plugin_midigain(void);
extern void carla_register_native_plugin_midijoin(void);
extern void carla_register_native_plugin_midisplit(void);
extern void carla_register_native_plugin_midithrough(void);
extern void carla_register_native_plugin_miditranspose(void);

// MIDI sequencer
extern void carla_register_native_plugin_midipattern(void);

// Carla
extern void carla_register_native_plugin_carla(void);

// External-UI plugins
extern void carla_register_native_plugin_bigmeter(void);
extern void carla_register_native_plugin_notes(void);

// --------------------------------------------------------------------------------------------------------------------

void carla_register_all_native_plugins(void)
{
    // Simple plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_lfo();
    carla_register_native_plugin_midichanfilter();
    carla_register_native_plugin_midichanab();
    carla_register_native_plugin_midigain();
    carla_register_native_plugin_midijoin();
    carla_register_native_plugin_midisplit();
    carla_register_native_plugin_midithrough();
    carla_register_native_plugin_miditranspose();

#ifdef HAVE_PYQT
    // MIDI sequencer
    carla_register_native_plugin_midipattern();

    // Carla
    carla_register_native_plugin_carla();
#endif

    // External-UI plugins
    carla_register_native_plugin_bigmeter();
    carla_register_native_plugin_notes();
}

// --------------------------------------------------------------------------------------------------------------------
