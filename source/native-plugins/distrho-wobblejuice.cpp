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

// config fix
#include "distrho-wobblejuice/DistrhoPluginInfo.h"

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL)
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

// Plugin Code
#include "distrho-wobblejuice/WobbleJuiceArtwork.cpp"
#include "distrho-wobblejuice/WobbleJuicePlugin.cpp"
#ifdef HAVE_DGL
#include "distrho-wobblejuice/WobbleJuiceUI.cpp"
#endif

// DISTRHO Code
#define DISTRHO_PLUGIN_TARGET_CARLA
#include "DistrhoPluginMain.cpp"
#ifdef HAVE_DGL
#include "DistrhoUIMain.cpp"
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

static const NativePluginDescriptor wobblejuiceDesc = {
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
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ WobbleJuicePlugin::paramCount,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "wobblejuice",
    /* maker     */ "Andre Sklenar",
    /* copyright */ "GPL v2+",
    PluginDescriptorFILL(PluginCarla)
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_distrho_wobblejuice();

CARLA_EXPORT
void carla_register_native_plugin_distrho_wobblejuice()
{
    USE_NAMESPACE_DISTRHO
    carla_register_native_plugin(&wobblejuiceDesc);
}

// -----------------------------------------------------------------------
