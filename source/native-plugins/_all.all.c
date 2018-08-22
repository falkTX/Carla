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

// Audio file
extern void carla_register_native_plugin_audiofile(void);

// MIDI file and sequencer
extern void carla_register_native_plugin_midifile(void);
extern void carla_register_native_plugin_midipattern(void);

// Carla
extern void carla_register_native_plugin_carla(void);

// External-UI plugins
extern void carla_register_native_plugin_bigmeter(void);
extern void carla_register_native_plugin_notes(void);

#ifdef HAVE_EXTERNAL_PLUGINS
# define CARLA_EXTERNAL_PLUGINS_INCLUDED_DIRECTLY
# include "external/_all.c"
#endif

// --------------------------------------------------------------------------------------------------------------------

void carla_register_all_native_plugins(void)
{
    puts("carla register all plugins\n");
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

    // Audio file
    carla_register_native_plugin_audiofile();

    // MIDI file
    carla_register_native_plugin_midifile();

#ifdef HAVE_PYQT
    // Carla
    carla_register_native_plugin_carla();

    // External-UI plugins
    carla_register_native_plugin_bigmeter();
    carla_register_native_plugin_midipattern();
    carla_register_native_plugin_notes();
#endif // HAVE_PYQT

#ifdef HAVE_EXTERNAL_PLUGINS
    // Experimental plugins
    carla_register_all_native_external_plugins();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
