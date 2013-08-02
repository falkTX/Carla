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

#include "carla-native-base.cpp"

#include "CarlaString.hpp"

// -----------------------------------------------------------------------
// LV2 descriptor functions

// -----------------------------------------------------------------------
// Static LV2 Descriptor objects

// -----------------------------------------------------------------------
// Startup code

// CARLA_EXPORT void lv2_generate_ttl(const char* basename)
// {
//     createLv2Files (basename);
// }

CARLA_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    if (index >= sPluginDescsMgr.descs.count())
        return nullptr;
    if (index < sPluginDescsMgr.lv2Descs.count())
        return sPluginDescsMgr.lv2Descs.getAt(index);

    const PluginDescriptor*& pluginDesc(sPluginDescsMgr.descs.getAt(index));

    CarlaString tmpURI;

    if (std::strcmp(pluginDesc->label, "carla") == 0)
    {
        tmpURI = "http://kxstudio.sf.net/carla";
    }
    else
    {
        tmpURI  = "http://kxstudio.sf.net/carla/plugins/";
        tmpURI += pluginDesc->label;
    }

    LV2_Descriptor* lv2Desc(new LV2_Descriptor);

    lv2Desc->URI          = carla_strdup(tmpURI);
    lv2Desc->instantiate  = nullptr;
    lv2Desc->connect_port = nullptr;
    lv2Desc->activate     = nullptr;
    lv2Desc->run          = nullptr;
    lv2Desc->deactivate   = nullptr;
    lv2Desc->cleanup      = nullptr;
    lv2Desc->extension_data = nullptr;

    sPluginDescsMgr.lv2Descs.append(lv2Desc);

    return lv2Desc;
}

// CARLA_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
// {
//     switch (index)
//     {
//     case 0:
//         return &JuceLv2UI_External;
//     case 1:
//         return &JuceLv2UI_Parent;
//     default:
//         return nullptr;
//     }
// }

// -----------------------------------------------------------------------

int main()
{
    return 0;
}
