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

#include "CarlaDefines.h"
#include "CarlaNative.h"

// --------------------------------------------------------------------------------------------------------------------

// DISTRHO plugins
extern void carla_register_native_plugin_distrho_3bandeq(void);
extern void carla_register_native_plugin_distrho_3bandsplitter(void);
extern void carla_register_native_plugin_distrho_kars(void);
extern void carla_register_native_plugin_distrho_nekobi(void);
extern void carla_register_native_plugin_distrho_pingpongpan(void);
extern void carla_register_native_plugin_distrho_prom(void);

// DISTRHO plugins (Juice)
extern void carla_register_native_plugin_distrho_vectorjuice(void);
extern void carla_register_native_plugin_distrho_wobblejuice(void);

// ZynAddSubFX
extern void carla_register_native_plugin_zynaddsubfx_fx(void);
extern void carla_register_native_plugin_zynaddsubfx_synth(void);

// Experimental plugins
extern void carla_register_native_plugin_zita_at1(void);
extern void carla_register_native_plugin_zita_bls1(void);
extern void carla_register_native_plugin_zita_rev1(void);

// --------------------------------------------------------------------------------------------------------------------

void carla_register_all_native_external_plugins(void)
{
    // DISTRHO Plugins
    carla_register_native_plugin_distrho_3bandeq();
    carla_register_native_plugin_distrho_3bandsplitter();
    carla_register_native_plugin_distrho_kars();
    carla_register_native_plugin_distrho_nekobi();
    carla_register_native_plugin_distrho_pingpongpan();
#if defined(HAVE_DGL) && defined(HAVE_PROJECTM)
    carla_register_native_plugin_distrho_prom();
#endif

    // DISTRHO plugins (Juice)
    carla_register_native_plugin_distrho_vectorjuice();
    carla_register_native_plugin_distrho_wobblejuice();

#ifdef HAVE_ZYN_DEPS
    // ZynAddSubFX
    carla_register_native_plugin_zynaddsubfx_fx();
# ifndef DISABLE_PLUGINS_FOR_WINDOWS_BUILD
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

// --------------------------------------------------------------------------------------------------------------------
