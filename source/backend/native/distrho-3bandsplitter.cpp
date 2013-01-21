/*
 * Carla Native Plugins
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_native.hpp"

// Plugin Code
#include "3bandsplitter/DistrhoArtwork3BandSplitter.cpp"
#include "3bandsplitter/DistrhoPlugin3BandSplitter.cpp"
#include "3bandsplitter/DistrhoUI3BandSplitter.cpp"

// Carla DISTRHO Plugin
#include "distrho/DistrhoPluginCarla.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

static PluginDescriptor tBandSplitterDesc = {
    /* category  */ ::PLUGIN_CATEGORY_EQ,
    /* hints     */ ::PLUGIN_HAS_GUI,
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ DistrhoPlugin3BandSplitter::paramCount,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "3BandSplitter",
    /* maker     */ "falkTX",
    /* copyright */ "LGPL",
    PluginDescriptorFILL(PluginCarla)
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

void carla_register_native_plugin_3BandSplitter()
{
    USE_NAMESPACE_DISTRHO
    carla_register_native_plugin(&tBandSplitterDesc);
}

// -----------------------------------------------------------------------
