/*
 * Common Carla code
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_SHARED_HPP_INCLUDED
#define CARLA_SHARED_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtCore/QSettings>
#include <QtCore/QStringList>

#include <QtGui/QIcon>

#include <QtWidgets/QMessageBox>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------

class CarlaHost;
class QFontMetrics;
class QLineEdit;

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaDefines.h"

//---------------------------------------------------------------------------------------------------------------------
// Static MIDI CC list

/*
static const char* const* const MIDI_CC_LIST = {
    "0x01 Modulation",
    "0x02 Breath",
    "0x03 (Undefined)",
    "0x04 Foot",
    "0x05 Portamento",
    "0x07 Volume",
    "0x08 Balance",
    "0x09 (Undefined)",
    "0x0A Pan",
    "0x0B Expression",
    "0x0C FX Control 1",
    "0x0D FX Control 2",
    "0x0E (Undefined)",
    "0x0F (Undefined)",
    "0x10 General Purpose 1",
    "0x11 General Purpose 2",
    "0x12 General Purpose 3",
    "0x13 General Purpose 4",
    "0x14 (Undefined)",
    "0x15 (Undefined)",
    "0x16 (Undefined)",
    "0x17 (Undefined)",
    "0x18 (Undefined)",
    "0x19 (Undefined)",
    "0x1A (Undefined)",
    "0x1B (Undefined)",
    "0x1C (Undefined)",
    "0x1D (Undefined)",
    "0x1E (Undefined)",
    "0x1F (Undefined)",
    "0x46 Control 1 [Variation]",
    "0x47 Control 2 [Timbre]",
    "0x48 Control 3 [Release]",
    "0x49 Control 4 [Attack]",
    "0x4A Control 5 [Brightness]",
    "0x4B Control 6 [Decay]",
    "0x4C Control 7 [Vib Rate]",
    "0x4D Control 8 [Vib Depth]",
    "0x4E Control 9 [Vib Delay]",
    "0x4F Control 10 [Undefined]",
    "0x50 General Purpose 5",
    "0x51 General Purpose 6",
    "0x52 General Purpose 7",
    "0x53 General Purpose 8",
    "0x54 Portamento Control",
    "0x5B FX 1 Depth [Reverb]",
    "0x5C FX 2 Depth [Tremolo]",
    "0x5D FX 3 Depth [Chorus]",
    "0x5E FX 4 Depth [Detune]",
    "0x5F FX 5 Depth [Phaser]",
    nullptr
};
*/

//---------------------------------------------------------------------------------------------------------------------
// PatchCanvas defines

// NOTE: must match Qt::CheckState and PatchCanvas API at the same time
#define CANVAS_ANTIALIASING_SMALL Qt::PartiallyChecked
#define CANVAS_EYECANDY_SMALL     Qt::PartiallyChecked

//---------------------------------------------------------------------------------------------------------------------
// Carla Settings keys

#define CARLA_KEY_MAIN_PROJECT_FOLDER   "Main/ProjectFolder"   /* str  */
#define CARLA_KEY_MAIN_USE_PRO_THEME    "Main/UseProTheme"     /* bool */
#define CARLA_KEY_MAIN_PRO_THEME_COLOR  "Main/ProThemeColor"   /* str  */
#define CARLA_KEY_MAIN_REFRESH_INTERVAL "Main/RefreshInterval" /* int  */
#define CARLA_KEY_MAIN_CONFIRM_EXIT     "Main/ConfirmExit"     /* bool */
#define CARLA_KEY_MAIN_SHOW_LOGS        "Main/ShowLogs"        /* bool */
#define CARLA_KEY_MAIN_EXPERIMENTAL     "Main/Experimental"    /* bool */

#define CARLA_KEY_CANVAS_THEME             "Canvas/Theme"           /* str */
#define CARLA_KEY_CANVAS_SIZE              "Canvas/Size"            /* str "NxN" */
#define CARLA_KEY_CANVAS_USE_BEZIER_LINES  "Canvas/UseBezierLines"  /* bool */
#define CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS  "Canvas/AutoHideGroups"  /* bool */
#define CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS "Canvas/AutoSelectItems" /* bool */
#define CARLA_KEY_CANVAS_EYE_CANDY         "Canvas/EyeCandy2"       /* bool */
#define CARLA_KEY_CANVAS_FANCY_EYE_CANDY   "Canvas/FancyEyeCandy"   /* bool */
#define CARLA_KEY_CANVAS_USE_OPENGL        "Canvas/UseOpenGL"       /* bool */
#define CARLA_KEY_CANVAS_ANTIALIASING      "Canvas/Antialiasing"    /* enum */
#define CARLA_KEY_CANVAS_HQ_ANTIALIASING   "Canvas/HQAntialiasing"  /* bool */
#define CARLA_KEY_CANVAS_INLINE_DISPLAYS   "Canvas/InlineDisplays"  /* bool */
#define CARLA_KEY_CANVAS_FULL_REPAINTS     "Canvas/FullRepaints"    /* bool */

#define CARLA_KEY_ENGINE_DRIVER_PREFIX         "Engine/Driver-"
#define CARLA_KEY_ENGINE_AUDIO_DRIVER          "Engine/AudioDriver"         /* str  */
#define CARLA_KEY_ENGINE_PROCESS_MODE          "Engine/ProcessMode"         /* enum */
#define CARLA_KEY_ENGINE_TRANSPORT_MODE        "Engine/TransportMode"       /* enum */
#define CARLA_KEY_ENGINE_TRANSPORT_EXTRA       "Engine/TransportExtra"      /* str  */
#define CARLA_KEY_ENGINE_FORCE_STEREO          "Engine/ForceStereo"         /* bool */
#define CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES "Engine/PreferPluginBridges" /* bool */
#define CARLA_KEY_ENGINE_PREFER_UI_BRIDGES     "Engine/PreferUiBridges"     /* bool */
#define CARLA_KEY_ENGINE_MANAGE_UIS            "Engine/ManageUIs"           /* bool */
#define CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP     "Engine/UIsAlwaysOnTop"      /* bool */
#define CARLA_KEY_ENGINE_MAX_PARAMETERS        "Engine/MaxParameters"       /* int  */
#define CARLA_KEY_ENGINE_RESET_XRUNS           "Engine/ResetXruns"          /* bool */
#define CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT    "Engine/UiBridgesTimeout"    /* int  */

#define CARLA_KEY_OSC_ENABLED          "OSC/Enabled"
#define CARLA_KEY_OSC_TCP_PORT_ENABLED "OSC/TCPEnabled"
#define CARLA_KEY_OSC_TCP_PORT_NUMBER  "OSC/TCPNumber"
#define CARLA_KEY_OSC_TCP_PORT_RANDOM  "OSC/TCPRandom"
#define CARLA_KEY_OSC_UDP_PORT_ENABLED "OSC/UDPEnabled"
#define CARLA_KEY_OSC_UDP_PORT_NUMBER  "OSC/UDPNumber"
#define CARLA_KEY_OSC_UDP_PORT_RANDOM  "OSC/UDPRandom"

#define CARLA_KEY_PATHS_AUDIO "Paths/Audio"
#define CARLA_KEY_PATHS_MIDI  "Paths/MIDI"

#define CARLA_KEY_PATHS_LADSPA "Paths/LADSPA"
#define CARLA_KEY_PATHS_DSSI   "Paths/DSSI"
#define CARLA_KEY_PATHS_LV2    "Paths/LV2"
#define CARLA_KEY_PATHS_VST2   "Paths/VST2"
#define CARLA_KEY_PATHS_VST3   "Paths/VST3"
#define CARLA_KEY_PATHS_SF2    "Paths/SF2"
#define CARLA_KEY_PATHS_SFZ    "Paths/SFZ"
#define CARLA_KEY_PATHS_JSFX   "Paths/JSFX"

#define CARLA_KEY_WINE_EXECUTABLE      "Wine/Executable"     /* str  */
#define CARLA_KEY_WINE_AUTO_PREFIX     "Wine/AutoPrefix"     /* bool */
#define CARLA_KEY_WINE_FALLBACK_PREFIX "Wine/FallbackPrefix" /* str  */
#define CARLA_KEY_WINE_RT_PRIO_ENABLED "Wine/RtPrioEnabled"  /* bool */
#define CARLA_KEY_WINE_BASE_RT_PRIO    "Wine/BaseRtPrio"     /* int  */
#define CARLA_KEY_WINE_SERVER_RT_PRIO  "Wine/ServerRtPrio"   /* int  */

#define CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES        "Experimental/PluginBridges"       /* bool */
#define CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES          "Experimental/WineBridges"         /* bool */
#define CARLA_KEY_EXPERIMENTAL_JACK_APPS             "Experimental/JackApplications"    /* bool */
#define CARLA_KEY_EXPERIMENTAL_EXPORT_LV2            "Experimental/ExportLV2"           /* bool */
#define CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR "Experimental/PreventBadBehaviour" /* bool */
#define CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL       "Experimental/LoadLibGlobal"       /* bool */

// if pro theme is on and color is black
#define CARLA_KEY_CUSTOM_PAINTING "UseCustomPainting" /* bool */

//---------------------------------------------------------------------------------------------------------------------
// Carla Settings defaults

// Main
#define CARLA_DEFAULT_MAIN_PROJECT_FOLDER   QDir::homePath()
#define CARLA_DEFAULT_MAIN_USE_PRO_THEME    true
#define CARLA_DEFAULT_MAIN_PRO_THEME_COLOR  "Black"
#define CARLA_DEFAULT_MAIN_REFRESH_INTERVAL 20
#define CARLA_DEFAULT_MAIN_CONFIRM_EXIT     true
#ifdef CARLA_OS_WIN
# define CARLA_DEFAULT_MAIN_SHOW_LOGS       false
#else
# define CARLA_DEFAULT_MAIN_SHOW_LOGS       true
#endif
#define CARLA_DEFAULT_MAIN_EXPERIMENTAL     false

// Canvas
#define CARLA_DEFAULT_CANVAS_THEME             "Modern Dark"
#define CARLA_DEFAULT_CANVAS_SIZE              "3100x2400"
#define CARLA_DEFAULT_CANVAS_SIZE_WIDTH        3100
#define CARLA_DEFAULT_CANVAS_SIZE_HEIGHT       2400
#define CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES  true
#define CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS  true
#define CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS false
#define CARLA_DEFAULT_CANVAS_EYE_CANDY         true
#define CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY   false
#define CARLA_DEFAULT_CANVAS_USE_OPENGL        false
#define CARLA_DEFAULT_CANVAS_ANTIALIASING      CANVAS_ANTIALIASING_SMALL
#define CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING   false
#define CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS   false
#define CARLA_DEFAULT_CANVAS_FULL_REPAINTS     false

// Engine
#define CARLA_DEFAULT_FORCE_STEREO          false
#define CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES false
#define CARLA_DEFAULT_PREFER_UI_BRIDGES     true
#define CARLA_DEFAULT_MANAGE_UIS            true
#define CARLA_DEFAULT_UIS_ALWAYS_ON_TOP     false
#define CARLA_DEFAULT_MAX_PARAMETERS        MAX_DEFAULT_PARAMETERS
#define CARLA_DEFAULT_RESET_XRUNS           false
#define CARLA_DEFAULT_UI_BRIDGES_TIMEOUT    4000

#define CARLA_DEFAULT_AUDIO_BUFFER_SIZE     512
#define CARLA_DEFAULT_AUDIO_SAMPLE_RATE     44100
#define CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER   false

#if defined(CARLA_OS_WIN)
# define CARLA_DEFAULT_AUDIO_DRIVER "DirectSound"
#elif defined(CARLA_OS_MAC)
# define CARLA_DEFAULT_AUDIO_DRIVER "CoreAudio"
#else
// if os.path.exists("/usr/bin/jackd") or os.path.exists("/usr/bin/jackdbus"):
# define CARLA_DEFAULT_AUDIO_DRIVER "JACK"
// # define CARLA_DEFAULT_AUDIO_DRIVER "PulseAudio"
#endif

// #if CARLA_DEFAULT_AUDIO_DRIVER == "JACK"
// # define CARLA_DEFAULT_PROCESS_MODE   ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS
// # define CARLA_DEFAULT_TRANSPORT_MODE ENGINE_TRANSPORT_MODE_JACK
// #else
# define CARLA_DEFAULT_PROCESS_MODE   ENGINE_PROCESS_MODE_PATCHBAY
# define CARLA_DEFAULT_TRANSPORT_MODE ENGINE_TRANSPORT_MODE_INTERNAL
// #endif

// OSC
#ifdef CARLA_OS_WIN
# define CARLA_DEFAULT_OSC_ENABLED false
#else
# define CARLA_DEFAULT_OSC_ENABLED true
#endif
#define CARLA_DEFAULT_OSC_TCP_PORT_ENABLED true
#define CARLA_DEFAULT_OSC_TCP_PORT_NUMBER  22752
#define CARLA_DEFAULT_OSC_TCP_PORT_RANDOM  false
#define CARLA_DEFAULT_OSC_UDP_PORT_ENABLED true
#define CARLA_DEFAULT_OSC_UDP_PORT_NUMBER  22752
#define CARLA_DEFAULT_OSC_UDP_PORT_RANDOM  false

// Wine
#define CARLA_DEFAULT_WINE_EXECUTABLE      "wine"
#define CARLA_DEFAULT_WINE_AUTO_PREFIX     true
#define CARLA_DEFAULT_WINE_FALLBACK_PREFIX QDir::homePath() + "/.wine"
#define CARLA_DEFAULT_WINE_RT_PRIO_ENABLED true
#define CARLA_DEFAULT_WINE_BASE_RT_PRIO    15
#define CARLA_DEFAULT_WINE_SERVER_RT_PRIO  10

// Experimental
#define CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES        false
#define CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES          false
#define CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS             false
#define CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT            false
#define CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR false
#define CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL       false

//---------------------------------------------------------------------------------------------------------------------
// Default File Folders

#define CARLA_DEFAULT_FILE_PATH_AUDIO {}
#define CARLA_DEFAULT_FILE_PATH_MIDI  {}

//---------------------------------------------------------------------------------------------------------------------
// Default Plugin Folders (get)

#define DEFAULT_LADSPA_PATH ""
#define DEFAULT_DSSI_PATH   ""
#define DEFAULT_LV2_PATH    ""
#define DEFAULT_VST2_PATH   ""
#define DEFAULT_VST3_PATH   ""
#define DEFAULT_SF2_PATH    ""
#define DEFAULT_SFZ_PATH    ""
#define DEFAULT_JSFX_PATH    ""

#ifdef CARLA_OS_WIN
# define CARLA_PATH_SPLITTER ";"
#else
# define CARLA_PATH_SPLITTER ":"
#endif

//---------------------------------------------------------------------------------------------------------------------
// Default Plugin Folders (set)

/*
readEnvVars = True

if WINDOWS:
    # Check if running Wine. If yes, ignore env vars
    from winreg import ConnectRegistry, OpenKey, CloseKey, HKEY_CURRENT_USER
    reg = ConnectRegistry(None, HKEY_CURRENT_USER)

    try:
        key = OpenKey(reg, r"SOFTWARE\Wine")
        CloseKey(key)
        del key
        readEnvVars = False
    except:
        pass

    CloseKey(reg)
    del reg
*/

/*
#ifndef CARLA_OS_WIN
# define CARLA_DEFAULT_LADSPA_PATH = std::getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_DSSI_PATH   = std::getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_LV2_PATH    = std::getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_VST2_PATH   = std::getenv("VST_PATH",    DEFAULT_VST2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_VST3_PATH   = std::getenv("VST3_PATH",   DEFAULT_VST3_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_SF2_PATH    = std::getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_SFZ_PATH    = std::getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_JSFX_PATH   = std::getenv("JSFX_PATH",    DEFAULT_JSFX_PATH).split(CARLA_PATH_SPLITTER)
#else
*/
# define CARLA_DEFAULT_LADSPA_PATH QString(DEFAULT_LADSPA_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_DSSI_PATH   QString(DEFAULT_DSSI_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_LV2_PATH    QString(DEFAULT_LV2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_VST2_PATH   QString(DEFAULT_VST2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_VST3_PATH   QString(DEFAULT_VST3_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_SF2_PATH    QString(DEFAULT_SF2_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_SFZ_PATH    QString(DEFAULT_SFZ_PATH).split(CARLA_PATH_SPLITTER)
# define CARLA_DEFAULT_JSFX_PATH   QString(DEFAULT_JSFX_PATH).split(CARLA_PATH_SPLITTER)
/*
#endif
*/

//---------------------------------------------------------------------------------------------------------------------
// Global Carla object

struct CarlaObject {
    CarlaHost* host;
    QWidget* gui;
    bool nogui;      // Skip UI
    bool term;       // Terminated by OS signal

    CarlaObject() noexcept;
};

extern CarlaObject gCarla;

//---------------------------------------------------------------------------------------------------------------------
// Set DLL_EXTENSION

#if defined(CARLA_OS_WIN)
# define DLL_EXTENSION "dll"
#elif defined(CARLA_OS_MAC)
# define DLL_EXTENSION "dylib"
#else
# define DLL_EXTENSION "so"
#endif

//---------------------------------------------------------------------------------------------------------------------
// Get Icon from user theme, using our own as backup (Oxygen)

QIcon getIcon(QString icon, int size = 16);

//---------------------------------------------------------------------------------------------------------------------
// Handle some basic command-line arguments shared between all carla variants

QString handleInitialCommandLineArguments(const int argc, char* argv[]);

//---------------------------------------------------------------------------------------------------------------------
// Get initial project file (as passed in the command-line parameters)

QString getInitialProjectFile(bool skipExistCheck = false);

//---------------------------------------------------------------------------------------------------------------------
// Get paths (binaries, resources)

bool getPaths(QString& pathBinaries, QString& pathResources);

//---------------------------------------------------------------------------------------------------------------------
// Signal handler

void setUpSignals();

//---------------------------------------------------------------------------------------------------------------------
// QLineEdit and QPushButton combo

QString getAndSetPath(QWidget* parent, QLineEdit* lineEdit);

//---------------------------------------------------------------------------------------------------------------------
// fill up a qlists from a C arrays

void fillQStringListFromStringArray(QStringList& list, const char* const* const stringArray);
void fillQDoubleListFromDoubleArray(QList<double>& list, const double* const doubleArray);
void fillQUIntListFromUIntArray(QList<uint>& list, const uint* const uintArray);

//---------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on Qt version

int fontMetricsHorizontalAdvance(const QFontMetrics& fm, const QString& s);

//---------------------------------------------------------------------------------------------------------------------
// Check if a string array contains a string

bool stringArrayContainsString(const char* const* const stringArray, const char* const string) noexcept;

//---------------------------------------------------------------------------------------------------------------------
// Get index of a QList<double> value

int getIndexOfQDoubleListValue(const QList<double>& list, const double value);

//---------------------------------------------------------------------------------------------------------------------
// Check if two QList<double> instances match

bool isQDoubleListEqual(const QList<double>& list1, const QList<double>& list2);

//---------------------------------------------------------------------------------------------------------------------
// Custom QMessageBox which resizes itself to fit text

class QMessageBoxWithBetterWidth : public QMessageBox
{
public:
    inline QMessageBoxWithBetterWidth(QWidget* const parent)
        : QMessageBox(parent) {}

protected:
    void showEvent(QShowEvent* event);
};

//---------------------------------------------------------------------------------------------------------------------
// Custom MessageBox

int CustomMessageBox(QWidget* parent, QMessageBox::Icon icon, QString title, QString text,
                     QString extraText = "",
                     QMessageBox::StandardButtons buttons = QMessageBox::Yes|QMessageBox::No,
                     QMessageBox::StandardButton defButton = QMessageBox::No);

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_SHARED_HPP_INCLUDED
