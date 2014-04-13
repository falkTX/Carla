/*
 * Carla Native Plugins
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngine.hpp"
#include "CarlaPlugin.hpp"
#include "CarlaHost.h"
#include "CarlaNative.h"

#include "CarlaBackendUtils.hpp"
#include "LinkedList.hpp"

#ifdef CARLA_NATIVE_PLUGIN_LV2
# include "lv2/lv2.h"
#endif

// -----------------------------------------------------------------------
// Plugin List

struct PluginListManager {
    PluginListManager()
    {
        for (size_t i=0, count = CarlaBackend::CarlaPlugin::getNativePluginCount(); i < count; ++i)
        {
            const NativePluginDescriptor* const desc(CarlaBackend::CarlaPlugin::getNativePluginDescriptor(i));

            // Open/Save not possible in plugins
            if (desc->hints & PLUGIN_NEEDS_UI_OPEN_SAVE)
                continue;

            // skip distrho plugins and Vex
            if (
                // distrho mini-series
                std::strcmp(desc->label, "3bandeq")     == 0 || std::strcmp(desc->label, "3bandsplitter")  == 0 ||
                std::strcmp(desc->label, "pingpongpan") == 0 || std::strcmp(desc->label, "stereoenhancer") == 0 ||
                // juice
                std::strcmp(desc->label, "powerjuice")  == 0 || std::strcmp(desc->label, "segmentjuice") == 0 ||
                std::strcmp(desc->label, "vectorjuice") == 0 || std::strcmp(desc->label, "wobblejuice") == 0  ||
                // other synths
                std::strcmp(desc->label, "nekobi")      == 0 || std::strcmp(desc->label, "vexsynth") == 0
               )
            {
                continue;
            }

            // skip midi plugins, not implemented yet
            if (desc->audioIns == 0 && desc->audioOuts == 0 && desc->midiIns == 1 && desc->midiOuts >= 1)
                continue;

            descs.append(desc);
        }
    }

    ~PluginListManager()
    {
#ifdef CARLA_NATIVE_PLUGIN_LV2
        for (LinkedList<const LV2_Descriptor*>::Itenerator it = lv2Descs.begin(); it.valid(); it.next())
        {
            const LV2_Descriptor* const lv2Desc(it.getValue());
            delete[] lv2Desc->URI;
            delete lv2Desc;
        }
        lv2Descs.clear();
#endif

        descs.clear();
    }

    static PluginListManager& getInstance()
    {
        static PluginListManager plm;
        return plm;
    }

#ifdef CARLA_NATIVE_PLUGIN_LV2
    LinkedList<const LV2_Descriptor*> lv2Descs;
#endif
    LinkedList<const NativePluginDescriptor*> descs;
};

// -----------------------------------------------------------------------
