/*
 * Carla CLAP Plugin
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaPluginInternal.hpp"
#include "CarlaEngine.hpp"

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newCLAP(const Initializer& init)
{
    carla_debug("CarlaPlugin::newCLAP({%p, \"%s\", \"%s\", \"%s\"})",
                init.engine, init.filename, init.name, init.label);

    init.engine->setLastError("Not implemented yet");
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
