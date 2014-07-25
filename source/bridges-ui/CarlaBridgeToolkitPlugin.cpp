/*
 * Carla Bridge Toolkit, Plugin version
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeClient.hpp"
#include "CarlaBridgeToolkit.hpp"

#include "CarlaPluginUI.hpp"

CARLA_BRIDGE_START_NAMESPACE

// -------------------------------------------------------------------------

CarlaBridgeToolkit* CarlaBridgeToolkit::createNew(CarlaBridgeClient* const client, const char* const uiTitle)
{
    return nullptr;
    (void)client; (void)uiTitle;
}

// -------------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#include "CarlaPluginUI.cpp"

// -------------------------------------------------------------------------
