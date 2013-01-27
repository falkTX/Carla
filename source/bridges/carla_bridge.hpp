/*
 * Carla Bridge
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

#ifndef CARLA_BRIDGE_HPP
#define CARLA_BRIDGE_HPP

#include "carla_defines.hpp"

#define CARLA_BRIDGE_START_NAMESPACE namespace CarlaBridge {
#define CARLA_BRIDGE_END_NAMESPACE }
#define CARLA_BRIDGE_USE_NAMESPACE using namespace CarlaBridge;

CARLA_BRIDGE_START_NAMESPACE

class CarlaBridgeClient;
class CarlaBridgeToolkit;

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_HPP
