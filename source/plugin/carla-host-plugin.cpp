/*
 * Carla Plugin Host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaHost.h"
#include "CarlaUtils.h"

#define CARLA_PLUGIN_EXPORT

// -------------------------------------------------------------------------------------------------------------------
// Include utils code first

#include "utils/CachedPlugins.cpp"
#include "utils/Information.cpp"
#include "utils/PipeClient.cpp"
#include "utils/System.cpp"
#include "utils/Windows.cpp"

// -------------------------------------------------------------------------------------------------------------------
// Always return a valid string ptr

static const char* const gNullCharPtr = "";

static void checkStringPtr(const char*& charPtr) noexcept
{
    if (charPtr == nullptr)
        charPtr = gNullCharPtr;
}

// -------------------------------------------------------------------------------------------------------------------
// Include all standalone code

#include "CarlaStandalone.cpp"

// -------------------------------------------------------------------------------------------------------------------
