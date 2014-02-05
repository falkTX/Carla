/*
 * Carla VST3 Plugin
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

CarlaPlugin* CarlaPlugin::newVST3(const Initializer& init)
{
    carla_debug("CarlaPlugin::newVST3({%p, \"%s\", \"%s\"})", init.engine, init.filename, init.name);

#if defined(WANT_VST) && defined(HAVE_JUCE)
    return newJuce(init, "VST3");
#else
    init.engine->setLastError("VST3 support not available");
    return nullptr;
#endif
}

CARLA_BACKEND_END_NAMESPACE
