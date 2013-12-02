/*
 * Carla Native Plugins
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"

// Plugin Code
#include "stereoenhancer/DistrhoArtworkStereoEnhancer.cpp"
#include "stereoenhancer/DistrhoPluginStereoEnhancer.cpp"
#include "stereoenhancer/DistrhoUIStereoEnhancer.cpp"

// Carla DISTRHO Plugin
#include "distrho/DistrhoPluginCarla.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

static const NativePluginDescriptor stereoenhancerDesc = {
    /* category  */ PLUGIN_CATEGORY_EQ,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_UI),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ DistrhoPluginStereoEnhancer::paramCount,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "StereoEnhancer",
    /* maker     */ "falkTX",
    /* copyright */ "LGPL",
    PluginDescriptorFILL(PluginCarla)
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_StereoEnhancer()
{
    USE_NAMESPACE_DISTRHO
    carla_register_native_plugin(&stereoenhancerDesc);
}

// -----------------------------------------------------------------------
