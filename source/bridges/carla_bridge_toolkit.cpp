/*
 * Carla Bridge Toolkit
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "carla_bridge_toolkit.hpp"
#include "carla_utils.hpp"

#include <cstdlib>
#include <cstring>

CARLA_BRIDGE_START_NAMESPACE

CarlaBridgeToolkit::CarlaBridgeToolkit(CarlaBridgeClient* const client_, const char* const newTitle)
    : client(client_)
{
    qDebug("CarlaBridgeToolkit::CarlaBridgeToolkit(%p, \"%s\")", client, newTitle);
    CARLA_ASSERT(client);
    CARLA_ASSERT(newTitle);

    uiTitle  = strdup(newTitle ? newTitle : "(null)");
}

CarlaBridgeToolkit::~CarlaBridgeToolkit()
{
    qDebug("CarlaBridgeToolkit::~CarlaBridgeToolkit()");

    free(uiTitle);
}

void* CarlaBridgeToolkit::getContainerId()
{
    qDebug("CarlaBridgeToolkit::getContainerId()");
    return nullptr;
}

CARLA_BRIDGE_END_NAMESPACE
