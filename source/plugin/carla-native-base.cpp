/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.h"
#include "List.hpp"

#include "lv2/lv2.h"

// -----------------------------------------------------------------------
// Plugin List

struct PluginListManager {
    PluginListManager()
    {
        carla_register_all_plugins();
    }

    ~PluginListManager()
    {
        for (List<const LV2_Descriptor*>::Itenerator it = lv2Descs.begin(); it.valid(); it.next())
        {
            const LV2_Descriptor*& lv2Desc(it.getValue());
            delete[] lv2Desc->URI;
            delete lv2Desc;
        }

        descs.clear();
        lv2Descs.clear();
    }

    List<const NativePluginDescriptor*> descs;
    List<const LV2_Descriptor*> lv2Descs;
};

static PluginListManager sPluginDescsMgr;

// -----------------------------------------------------------------------

void carla_register_native_plugin(const NativePluginDescriptor* desc)
{
#ifdef CARLA_NATIVE_PLUGIN_LV2
    // LV2 MIDI Out and Open/Save are not implemented yet
    if (desc->midiOuts > 0 || (desc->hints & PLUGIN_NEEDS_UI_OPEN_SAVE) != 0)
        return;
#endif
    sPluginDescsMgr.descs.append(desc);
}

// -----------------------------------------------------------------------
