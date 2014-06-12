/*
 * Common Carla code
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef FRONTEND_CARLA_SHARED_HPP_INCLUDED
#define FRONTEND_CARLA_SHARED_HPP_INCLUDED

// ------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtGui/QIcon>
#include <QtWidgets/QMessageBox>

class QLineEdit;

// ------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaBackendUtils.hpp"

// ------------------------------------------------------------------------------------------------------------
// Set Version

#define VERSION "1.9.4 (2.0-beta2)"

// ------------------------------------------------------------------------------------------------------------
// Set TMP

extern QString TMP;

// ------------------------------------------------------------------------------------------------------------
// Set HOME

extern QString HOME;

// ------------------------------------------------------------------------------------------------------------
// Set PATH

extern QStringList PATH;

// ------------------------------------------------------------------------------------------------------------
// Static MIDI CC list

extern QStringList MIDI_CC_LIST;

// ------------------------------------------------------------------------------------------------------------
// Carla Settings keys

#define CARLA_KEY_MAIN_PROJECT_FOLDER   "Main/ProjectFolder"   // str
#define CARLA_KEY_MAIN_USE_PRO_THEME    "Main/UseProTheme"     // bool
#define CARLA_KEY_MAIN_PRO_THEME_COLOR  "Main/ProThemeColor"   // str
#define CARLA_KEY_MAIN_REFRESH_INTERVAL "Main/RefreshInterval" // int

#define CARLA_KEY_CANVAS_THEME            "Canvas/Theme"           // str
#define CARLA_KEY_CANVAS_SIZE             "Canvas/Size"            // str "NxN"
#define CARLA_KEY_CANVAS_USE_BEZIER_LINES "Canvas/UseBezierLines"  // bool
#define CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS "Canvas/AutoHideGroups"  // bool
#define CARLA_KEY_CANVAS_EYE_CANDY        "Canvas/EyeCandy"        // enum
#define CARLA_KEY_CANVAS_USE_OPENGL       "Canvas/UseOpenGL"       // bool
#define CARLA_KEY_CANVAS_ANTIALIASING     "Canvas/Antialiasing"    // enum
#define CARLA_KEY_CANVAS_HQ_ANTIALIASING  "Canvas/HQAntialiasing"  // bool
#define CARLA_KEY_CUSTOM_PAINTING         "UseCustomPainting"      // bool

#define CARLA_KEY_ENGINE_DRIVER_PREFIX         "Engine/Driver-"
#define CARLA_KEY_ENGINE_AUDIO_DRIVER          "Engine/AudioDriver"         // str
#define CARLA_KEY_ENGINE_PROCESS_MODE          "Engine/ProcessMode"         // enum
#define CARLA_KEY_ENGINE_TRANSPORT_MODE        "Engine/TransportMode"       // enum
#define CARLA_KEY_ENGINE_FORCE_STEREO          "Engine/ForceStereo"         // bool
#define CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES "Engine/PreferPluginBridges" // bool
#define CARLA_KEY_ENGINE_PREFER_UI_BRIDGES     "Engine/PreferUiBridges"     // bool
#define CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP     "Engine/UIsAlwaysOnTop"      // bool
#define CARLA_KEY_ENGINE_MAX_PARAMETERS        "Engine/MaxParameters"       // int
#define CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT    "Engine/UiBridgesTimeout"    // int

#define CARLA_KEY_PATHS_LADSPA "Paths/LADSPA"
#define CARLA_KEY_PATHS_DSSI   "Paths/DSSI"
#define CARLA_KEY_PATHS_LV2    "Paths/LV2"
#define CARLA_KEY_PATHS_VST    "Paths/VST"
#define CARLA_KEY_PATHS_VST3   "Paths/VST3"
#define CARLA_KEY_PATHS_AU     "Paths/AU"
#define CARLA_KEY_PATHS_CSD    "Paths/CSD"
#define CARLA_KEY_PATHS_GIG    "Paths/GIG"
#define CARLA_KEY_PATHS_SF2    "Paths/SF2"
#define CARLA_KEY_PATHS_SFZ    "Paths/SFZ"

// ------------------------------------------------------------------------------------------------------------
// Global Carla object

struct CarlaObject {
    // Host library object
    void* host;
    // Host Window
    void* gui;
    // is controller
    bool isControl;
    // is running local
    bool isLocal;
    // is plugin
    bool isPlugin;
    // current buffer size
    uint32_t bufferSize;
    // current sample rate
    double sampleRate;
    // current process mode
    EngineProcessMode processMode;
    // check if process mode is forced (rack/patchbay)
    bool processModeForced;
    // current transport mode
    EngineTransportMode transportMode;
    // current max parameters
    uint maxParameters;
    // binary dir
    QString pathBinaries;
    // resources dir
    QString pathResources;
    // default paths
    QStringList DEFAULT_LADSPA_PATH;
    QStringList DEFAULT_DSSI_PATH;
    QStringList DEFAULT_LV2_PATH;
    QStringList DEFAULT_VST_PATH;
    QStringList DEFAULT_VST3_PATH;
    QStringList DEFAULT_AU_PATH;
    QStringList DEFAULT_CSOUND_PATH;
    QStringList DEFAULT_GIG_PATH;
    QStringList DEFAULT_SF2_PATH;
    QStringList DEFAULT_SFZ_PATH;
};

extern CarlaObject gCarla;

// ------------------------------------------------------------------------------------------------------------
// find tool

QString findTool(const QString& toolName);

// ------------------------------------------------------------------------------------------------------------
// Init host

void initHost(const char* const initName, const char* const libPrefix = nullptr, bool failError = true);

// ------------------------------------------------------------------------------------------------------------
// Get Icon from user theme, using our own as backup (Oxygen)

QIcon getIcon(const QString& icon, const int size = 16);

// ------------------------------------------------------------------------------------------------------------
// Signal handler

void setUpSignals();

// ------------------------------------------------------------------------------------------------------------
// QLineEdit and QPushButton combo

QString getAndSetPath(QWidget* const parent, const QString& currentPath, QLineEdit* const lineEdit);

// ------------------------------------------------------------------------------------------------------------
// Custom MessageBox

int CustomMessageBox(QWidget* const parent, const QMessageBox::Icon icon, const QString& title, const QString& text, const QString& extraText = "",
                     const QMessageBox::StandardButtons buttons = QMessageBox::Yes|QMessageBox::No,
                     const QMessageBox::StandardButton defButton = QMessageBox::No);

// ------------------------------------------------------------------------------------------------------------

#endif // FRONTEND_CARLA_SHARED_HPP_INCLUDED
