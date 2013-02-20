/*
 * Carla Bridge Toolkit
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaBridgeToolkit.hpp"
#include "CarlaUtils.hpp"

CARLA_BRIDGE_START_NAMESPACE

CarlaBridgeToolkit::CarlaBridgeToolkit(CarlaBridgeClient* const client, const char* const uiTitle)
    : kClient(client),
      kUiTitle(carla_strdup((uiTitle != nullptr) ? uiTitle : "(null)"))
{
    carla_debug("CarlaBridgeToolkit::CarlaBridgeToolkit(%p, \"%s\")", client, uiTitle);
    CARLA_ASSERT(client != nullptr);
    CARLA_ASSERT(uiTitle != nullptr);
}

CarlaBridgeToolkit::~CarlaBridgeToolkit()
{
    carla_debug("CarlaBridgeToolkit::~CarlaBridgeToolkit()");

    delete[] kUiTitle;
}

void* CarlaBridgeToolkit::getContainerId()
{
    carla_debug("CarlaBridgeToolkit::getContainerId()");
    return nullptr;
}

CARLA_BRIDGE_END_NAMESPACE
