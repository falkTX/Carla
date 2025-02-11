// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNativePlugin.h"

#define CARLA_PLUGIN_BUILD
#define CARLA_HOST_PLUGIN_BUILD

#include "carla-native-plugin.cpp"

// -------------------------------------------------------------------------------------------------------------------
// Include utils code first

#include "utils/CachedPlugins.cpp"
#include "utils/Information.cpp"
#include "utils/PipeClient.cpp"
#include "utils/PluginDiscovery.cpp"
#include "utils/System.cpp"
#include "utils/Windows.cpp"

// -------------------------------------------------------------------------------------------------------------------
// Include all standalone code

#include "CarlaStandalone.cpp"

// -------------------------------------------------------------------------------------------------------------------
