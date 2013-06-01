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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaNative.hpp"

#if 0
// Plugin Code
#include "notes/DistrhoPluginNotes.cpp"
#include "notes/DistrhoUINotes.cpp"

// Carla DISTRHO Plugin
#include "distrho/DistrhoPluginCarla.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

static const PluginDescriptor notesDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_GUI|PLUGIN_USES_SINGLE_THREAD),
    /* audioIns  */ DISTRHO_PLUGIN_NUM_INPUTS,
    /* audioOuts */ DISTRHO_PLUGIN_NUM_OUTPUTS,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
    /* paramOuts */ 0,
    /* name      */ DISTRHO_PLUGIN_NAME,
    /* label     */ "Notes",
    /* maker     */ "falkTX",
    /* copyright */ "GPL v2+",
    PluginDescriptorFILL(PluginCarla)
};

END_NAMESPACE_DISTRHO
#endif

// -----------------------------------------------------------------------

void carla_register_native_plugin_Notes()
{
    //USE_NAMESPACE_DISTRHO
    //carla_register_native_plugin(&notesDesc);
}

// -----------------------------------------------------------------------
