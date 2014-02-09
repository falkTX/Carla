/*
 * Carla AU Plugin
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaPlugin.hpp"
#include "CarlaEngine.hpp"
#include "CarlaUtils.hpp"

CARLA_BACKEND_START_NAMESPACE

CarlaPlugin* CarlaPlugin::newAU(const Initializer& init)
{
    carla_debug("CarlaPlugin::newAU({%p, \"%s\", \"%s\", " P_INT64 "})", init.engine, init.filename, init.name, init.uniqueId);

#if defined(WANT_AU) && defined(HAVE_JUCE)
    return newJuce(init, "AU");
#else
    init.engine->setLastError("AU support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
