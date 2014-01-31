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

ulong carla_standalone_get_transient_win_id()
{
    return 0;
}

const char* carla_standalone_file_callback(FileCallbackOpcode action, bool isDir, const char* title, const char* filter)
{
    CARLA_SAFE_ASSERT_RETURN(title != nullptr && title[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(filter != nullptr && filter[0] != '\0', nullptr);
    carla_debug("carla_file_callback(%i:%s, %s, \"%s\", \"%s\")", action, CarlaBackend::FileCallbackOpcode2Str(action), bool2str(isDir), title, filter);

    return nullptr;

    // unused
    (void)action;
    (void)isDir;
}

// -----------------------------------------------------------------------
// Plugin List

struct PluginListManager {
    PluginListManager()
    {
        for (size_t i=0, count = CarlaBackend::CarlaPlugin::getNativePluginCount(); i < count; ++i)
        {
            const NativePluginDescriptor* const desc(CarlaBackend::CarlaPlugin::getNativePluginDescriptor(i));

#if 0 //def CARLA_NATIVE_PLUGIN_LV2 // TESTING!!!
            // LV2 MIDI Out and Open/Save are not implemented yet
            if (desc->midiOuts > 0 || (desc->hints & PLUGIN_NEEDS_UI_OPEN_SAVE) != 0)
               continue;
#endif
            descs.append(desc);
        }
    }

    ~PluginListManager()
    {
#ifdef CARLA_NATIVE_PLUGIN_LV2
        for (LinkedList<const LV2_Descriptor*>::Itenerator it = lv2Descs.begin(); it.valid(); it.next())
        {
            const LV2_Descriptor*& lv2Desc(it.getValue());
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
