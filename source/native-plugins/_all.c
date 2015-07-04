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

#include "CarlaDefines.h"
#include "CarlaNative.h"

// -----------------------------------------------------------------------

// Simple plugins
extern void carla_register_native_plugin_bypass(void);
extern void carla_register_native_plugin_lfo(void);
extern void carla_register_native_plugin_midichanfilter(void);
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

// DISTRHO plugins
extern void carla_register_native_plugin_distrho_3bandeq(void);
extern void carla_register_native_plugin_distrho_3bandsplitter(void);
extern void carla_register_native_plugin_distrho_mverb(void);
extern void carla_register_native_plugin_distrho_nekobi(void);
extern void carla_register_native_plugin_distrho_pingpongpan(void);
extern void carla_register_native_plugin_distrho_prom(void);

// DISTRHO plugins (Juice)
extern void carla_register_native_plugin_distrho_vectorjuice(void);
extern void carla_register_native_plugin_distrho_wobblejuice(void);

// External-UI plugins
extern void carla_register_native_plugin_bigmeter(void);
extern void carla_register_native_plugin_notes(void);

// ZynAddSubFX
extern void carla_register_native_plugin_zynaddsubfx_fx(void);
extern void carla_register_native_plugin_zynaddsubfx_synth(void);

// Experimental plugins
extern void carla_register_native_plugin_zita_at1(void);
extern void carla_register_native_plugin_zita_bls1(void);
extern void carla_register_native_plugin_zita_rev1(void);

// -----------------------------------------------------------------------

void carla_register_all_native_plugins(void)
{
    // Simple plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_lfo();
    carla_register_native_plugin_midichanfilter();
    carla_register_native_plugin_midigain();
    carla_register_native_plugin_midijoin();
    carla_register_native_plugin_midisplit();
    carla_register_native_plugin_midithrough();
    carla_register_native_plugin_miditranspose();

    // Audio file
    carla_register_native_plugin_audiofile();

    // MIDI file and sequencer
    carla_register_native_plugin_midifile();
#ifdef CARLA_OS_LINUX
    carla_register_native_plugin_midipattern();
#endif

#ifndef CARLA_OS_WIN
    // Carla
    carla_register_native_plugin_carla();
#endif

    // DISTRHO Plugins
    carla_register_native_plugin_distrho_3bandeq();
    carla_register_native_plugin_distrho_3bandsplitter();
    carla_register_native_plugin_distrho_mverb();
    carla_register_native_plugin_distrho_nekobi();
    carla_register_native_plugin_distrho_pingpongpan();
#ifdef HAVE_DGL
#ifdef HAVE_PROJECTM
    carla_register_native_plugin_distrho_prom();
#endif
#endif

    // DISTRHO plugins (Juice)
    carla_register_native_plugin_distrho_vectorjuice();
    carla_register_native_plugin_distrho_wobblejuice();

#ifndef CARLA_OS_WIN
    // External-UI plugins
    carla_register_native_plugin_bigmeter();
    carla_register_native_plugin_notes();
#endif

#ifdef HAVE_ZYN_DEPS
# ifdef CARLA_OS_LINUX
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx_fx();
    carla_register_native_plugin_zynaddsubfx_synth();
# endif
#endif

#ifdef HAVE_EXPERIMENTAL_PLUGINS
    // Experimental plugins
    carla_register_native_plugin_zita_at1();
    carla_register_native_plugin_zita_bls1();
    carla_register_native_plugin_zita_rev1();
#endif
}

// -----------------------------------------------------------------------
