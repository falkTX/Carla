/*
 * Carla LV2 Single Plugin
 * Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaLv2Utils.hpp"

// -----------------------------------------------------------------------
// Startup code

CARLA_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    carla_debug("lv2_descriptor(%i)", index);

    return nullptr;
}

CARLA_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

    return nullptr;
}

// -----------------------------------------------------------------------
