/*
 * Carla Plugin Host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "CarlaDefines.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------------------------------------------------

typedef struct {
    const char* command;
    const char* name;
    const char* labelSetup;
} JackAppDialogResults;

typedef struct {
    uint API;
    uint build;
    uint type;
    uint hints;
    const char* category;
    const char* filename;
    const char* name;
    const char* label;
    const char* maker;
    uint64_t uniqueId;
    uint audioIns;
    uint audioOuts;
    uint cvIns;
    uint cvOuts;
    uint midiIns;
    uint midiOuts;
    uint parametersIns;
    uint parametersOuts;
} PluginListDialogResults;

typedef struct {
    char todo;
} PluginListRefreshDialogResults;

// --------------------------------------------------------------------------------------------------------------------

CARLA_API
void carla_frontend_createAndExecAboutJuceDialog(void* parent);

CARLA_API JackAppDialogResults* carla_frontend_createAndExecJackAppDialog(void* parent, const char* projectFilename);

CARLA_API
PluginListDialogResults* carla_frontend_createAndExecPluginListDialog(void* parent/*, const HostSettings& hostSettings*/);

CARLA_API
PluginListRefreshDialogResults* carla_frontend_createAndExecPluginListRefreshDialog(void* parent, bool useSystemIcons);

// --------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
