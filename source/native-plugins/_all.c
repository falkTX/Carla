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

#include "CarlaNativeJack.h"

// -----------------------------------------------------------------------

jack_client_t* gLastJackClient = NULL;

// -----------------------------------------------------------------------

// Simple plugins
extern void carla_register_native_plugin_bypass(void);
extern void carla_register_native_plugin_lfo(void);
extern void carla_register_native_plugin_midigain(void);
extern void carla_register_native_plugin_midisplit(void);
extern void carla_register_native_plugin_midithrough(void);
extern void carla_register_native_plugin_miditranspose(void);
extern void carla_register_native_plugin_nekofilter(void);

// Audio file
extern void carla_register_native_plugin_audiofile(void);

// MIDI file and sequencer
extern void carla_register_native_plugin_midifile(void);
extern void carla_register_native_plugin_midisequencer(void);

// Carla
extern void carla_register_native_plugin_carla(void);

// DISTRHO plugins
extern void carla_register_native_plugin_distrho_3bandeq(void);
extern void carla_register_native_plugin_distrho_3bandsplitter(void);
extern void carla_register_native_plugin_distrho_mverb(void);
extern void carla_register_native_plugin_distrho_nekobi(void);
extern void carla_register_native_plugin_distrho_pingpongpan(void);

#ifdef WANT_DISTRHO_PROM
extern void carla_register_native_plugin_distrho_prom(void);
#endif

// External-UI plugins
extern void carla_register_native_plugin_bigmeter(void);
extern void carla_register_native_plugin_notes(void);

#ifdef WANT_ZYNADDSUBFX
// ZynAddSubFX
extern void carla_register_native_plugin_zynaddsubfx_fx(void);
extern void carla_register_native_plugin_zynaddsubfx_synth(void);
#endif

#ifdef WANT_EXPERIMENTAL_PLUGINS
// Experimental plugins
extern void carla_register_native_plugin_zita_bls1(void);
#endif

// -----------------------------------------------------------------------

void carla_register_all_plugins(void)
{
    // Simple plugins
    carla_register_native_plugin_bypass();
    carla_register_native_plugin_lfo();
    carla_register_native_plugin_midigain();
    carla_register_native_plugin_midisplit();
    carla_register_native_plugin_midithrough();
    carla_register_native_plugin_miditranspose();
    carla_register_native_plugin_nekofilter();

    // Audio file
    carla_register_native_plugin_audiofile();

    // MIDI file and sequencer
    carla_register_native_plugin_midifile();
    carla_register_native_plugin_midisequencer();

    // Carla
    carla_register_native_plugin_carla();

    // DISTRHO Plugins
    carla_register_native_plugin_distrho_3bandeq();
    carla_register_native_plugin_distrho_3bandsplitter();
    carla_register_native_plugin_distrho_mverb();
    carla_register_native_plugin_distrho_nekobi();
    carla_register_native_plugin_distrho_pingpongpan();

#ifdef WANT_DISTRHO_PROM
    carla_register_native_plugin_distrho_prom();
#endif

    // External-UI plugins
    carla_register_native_plugin_bigmeter();
    carla_register_native_plugin_notes();

#ifdef WANT_ZYNADDSUBFX
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx_fx();
    carla_register_native_plugin_zynaddsubfx_synth();
#endif

#ifdef WANT_EXPERIMENTAL_PLUGINS
    // Experimental plugins
    carla_register_native_plugin_zita_bls1();
#endif
}

// -----------------------------------------------------------------------
