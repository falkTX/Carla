// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"
#include "CarlaUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

// -------------------------------------------------------------------------------------------------------------------

CarlaPluginPtr CarlaPlugin::newAU(const Initializer& init)
{
    carla_debug("CarlaPlugin::newAU({%p, \"%s\", \"%s\", \"%s\", " P_INT64 "})",
                init.engine, init.filename, init.name, init.label, init.uniqueId);

    init.engine->setLastError("AU support not available");
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
