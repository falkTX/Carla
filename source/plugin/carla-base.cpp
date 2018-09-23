/*
 * Carla Native Plugins
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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
#include "CarlaNativePlugin.h"
#include "CarlaJuceUtils.hpp"
#include "LinkedList.hpp"

// -----------------------------------------------------------------------

#ifdef CARLA_NATIVE_PLUGIN_LV2
#include "lv2/lv2.h"

CARLA_EXTERN_C
std::size_t carla_getNativePluginCount() noexcept;

CARLA_EXTERN_C
const NativePluginDescriptor* carla_getNativePluginDescriptor(const std::size_t index) noexcept;
#endif

// -----------------------------------------------------------------------
// Plugin List

struct PluginListManager {
    PluginListManager()
        : descs()
#ifdef CARLA_NATIVE_PLUGIN_LV2
        , lv2Descs()
#endif
    {
#ifdef CARLA_NATIVE_PLUGIN_LV2
        for (std::size_t i=0, count = carla_getNativePluginCount(); i < count; ++i)
        {
            const NativePluginDescriptor* const desc(carla_getNativePluginDescriptor(i));

            if (std::strcmp(desc->label, "lfo"             ) == 0 ||
                std::strcmp(desc->label, "midichanfilter"  ) == 0 ||
                std::strcmp(desc->label, "midichanab"      ) == 0 ||
                std::strcmp(desc->label, "midigain"        ) == 0 ||
                std::strcmp(desc->label, "midijoin"        ) == 0 ||
                std::strcmp(desc->label, "midisplit"       ) == 0 ||
                std::strcmp(desc->label, "midithrough"     ) == 0 ||
                std::strcmp(desc->label, "miditranspose"   ) == 0 ||
                std::strcmp(desc->label, "midipattern"     ) == 0 ||
                std::strcmp(desc->label, "carlarack"       ) == 0 ||
                std::strcmp(desc->label, "carlapatchbay"   ) == 0 ||
                std::strcmp(desc->label, "carlapatchbay3s" ) == 0 ||
                std::strcmp(desc->label, "carlapatchbay16" ) == 0 ||
                std::strcmp(desc->label, "carlapatchbay32" ) == 0 ||
                std::strcmp(desc->label, "bigmeter"        ) == 0 /*||
                std::strcmp(desc->label, "notes"           ) == 0*/)
            {
                descs.append(desc);
            }
        }
#else
        descs.append(carla_get_native_rack_plugin());
        descs.append(carla_get_native_patchbay_plugin());
        descs.append(carla_get_native_patchbay16_plugin());
        descs.append(carla_get_native_patchbay32_plugin());
#endif
    }

    ~PluginListManager()
    {
#ifdef CARLA_NATIVE_PLUGIN_LV2
        for (LinkedList<const LV2_Descriptor*>::Itenerator it = lv2Descs.begin2(); it.valid(); it.next())
        {
            const LV2_Descriptor* const lv2Desc(it.getValue(nullptr));
            CARLA_SAFE_ASSERT_CONTINUE(lv2Desc != nullptr);

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

    LinkedList<const NativePluginDescriptor*> descs;
#ifdef CARLA_NATIVE_PLUGIN_LV2
    LinkedList<const LV2_Descriptor*> lv2Descs;
#endif
};

// -----------------------------------------------------------------------
