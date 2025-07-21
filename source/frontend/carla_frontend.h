// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CarlaBackend.h"

#ifdef __cplusplus
using CARLA_BACKEND_NAMESPACE::PluginType;
extern "C" {
#endif

typedef struct _CarlaHostHandle* CarlaHostHandle;

// --------------------------------------------------------------------------------------------------------------------

typedef struct {
    const char* command;
    const char* name;
    const char* labelSetup;
} JackAppDialogResults;

typedef struct _HostSettings {
    bool showPluginBridges;
    bool showWineBridges;
    bool useSystemIcons;
    bool wineAutoPrefix;
    const char* wineExecutable;
    const char* wineFallbackPrefix;
} HostSettings;

typedef struct {
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
    uint parameterIns;
    uint parameterOuts;
} PluginListDialogResults;

#ifdef __cplusplus
class PluginListDialog;
class QWidget;
#else
struct PluginListDialog;
struct QWidget;
#endif

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT void
carla_frontend_createAndExecAboutDialog(QWidget* parent, CarlaHostHandle hostHandle, bool isControl, bool isPlugin);

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT const JackAppDialogResults*
carla_frontend_createAndExecJackAppDialog(QWidget* parent, const char* projectFilename);

// --------------------------------------------------------------------------------------------------------------------

CARLA_PLUGIN_EXPORT PluginListDialog*
carla_frontend_createPluginListDialog(QWidget* parent, const HostSettings* hostSettings);

CARLA_PLUGIN_EXPORT void
carla_frontend_destroyPluginListDialog(PluginListDialog* dialog);

// TODO get favorites

CARLA_PLUGIN_EXPORT void
carla_frontend_setPluginListDialogPath(PluginListDialog* dialog, int ptype, const char* path);

CARLA_PLUGIN_EXPORT const PluginListDialogResults*
carla_frontend_execPluginListDialog(PluginListDialog* dialog);

// CARLA_PLUGIN_EXPORT const PluginListDialogResults*
// carla_frontend_createAndExecPluginListDialog(void* parent, const HostSettings* hostSettings);

// --------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
