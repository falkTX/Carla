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

#include "../utils/CarlaUtils.hpp"

//---------------------------------------------------------------------------------------------------------------------

#include <QtCore/QSettings>
#include <QtCore/QStringList>

#include <QtGui/QFontMetrics>

class QMainWindow;

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
#define CARLA_DEFAULT_MAIN_PROJECT_FOLDER   HOME
#define CARLA_DEFAULT_MAIN_USE_PRO_THEME    true
#define CARLA_DEFAULT_MAIN_PRO_THEME_COLOR  "Black"
#define CARLA_DEFAULT_MAIN_REFRESH_INTERVAL 20
#define CARLA_DEFAULT_MAIN_CONFIRM_EXIT     true
#define CARLA_DEFAULT_MAIN_SHOW_LOGS        bool(not WINDOWS)
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
#define CARLA_DEFAULT_OSC_ENABLED !(not WINDOWS)
#define CARLA_DEFAULT_OSC_TCP_PORT_ENABLED true
#define CARLA_DEFAULT_OSC_TCP_PORT_NUMBER  22752
#define CARLA_DEFAULT_OSC_TCP_PORT_RANDOM  false
#define CARLA_DEFAULT_OSC_UDP_PORT_ENABLED true
#define CARLA_DEFAULT_OSC_UDP_PORT_NUMBER  22752
#define CARLA_DEFAULT_OSC_UDP_PORT_RANDOM  false

// Wine
#define CARLA_DEFAULT_WINE_EXECUTABLE      "wine"
#define CARLA_DEFAULT_WINE_AUTO_PREFIX     true
#define CARLA_DEFAULT_WINE_FALLBACK_PREFIX os.path.expanduser("~/.wine")
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
// Global Carla object

struct CarlaObject {
    QMainWindow* gui; // Host Window
    bool nogui;       // Skip UI
    bool term;        // Terminated by OS signal

    CarlaObject()
        : gui(nullptr),
          nogui(false),
          term(false) {}
};

extern CarlaObject gCarla;

//---------------------------------------------------------------------------------------------------------------------
// Signal handler

/*
void signalHandler(const int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        gCarla.term = true;
        if (gCarla.gui != nullptr)
            gCarla.gui.SIGTERM.emit();
    }
    else if (sig == SIGUSR1)
    {
        if (gCarla.gui != nullptr)
            gCarla.gui.SIGUSR1.emit();
    }
}
*/

inline void setUpSignals()
{
    /*
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGUSR1, signalHandler);
    */
}

//---------------------------------------------------------------------------------------------------------------------
// Handle some basic command-line arguments shared between all carla variants

inline
QString handleInitialCommandLineArguments(const int argc, char* argv[])
{
    static const QStringList listArgsNoGUI   = { "-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui" };
    static const QStringList listArgsHelp    = { "-h", "--h", "-help", "--help" };
    static const QStringList listArgsVersion = { "-v", "--v", "-version", "--version" };

    QString initName(argv[0]); // = os.path.basename(file) if (file is not None and os.path.dirname(file) in PATH) else sys.argv[0]
    // libPrefix = None

    for (int i=1; i<argc; ++i)
    {
        const QString arg(argv[i]);

        if (arg.startsWith("--with-appname="))
        {
            // initName = os.path.basename(arg.replace("--with-appname=", ""));
        }
        else if (arg.startsWith("--with-libprefix=") || arg == "--gdb")
        {
            pass();
        }
        else if (listArgsNoGUI.contains(arg))
        {
            gCarla.nogui = true;
        }
        else if (listArgsHelp.contains(arg))
        {
            carla_stdout("Usage: %s [OPTION]... [FILE|URL]", initName);
            carla_stdout("");
            carla_stdout(" where FILE can be a Carla project or preset file to be loaded, or URL if using Carla-Control");
            carla_stdout("");
            carla_stdout(" and OPTION can be one or more of the following:");
            carla_stdout("");
            carla_stdout("    --gdb    \t Run Carla inside gdb.");
            carla_stdout(" -n,--no-gui \t Run Carla headless, don't show UI.");
            carla_stdout("");
            carla_stdout(" -h,--help   \t Print this help text and exit.");
            carla_stdout(" -v,--version\t Print version information and exit.");
            carla_stdout("");

            std::exit(0);
        }
        else if (listArgsVersion.contains(arg))
        {
            /*
            QString pathBinaries, pathResources = getPaths();
            */

            carla_stdout("Using Carla version %s", CARLA_VERSION_STRING);
            /*
            carla_stdout("  Qt version:     %s", QT_VERSION_STR);
            carla_stdout("  Binary dir:     %s", pathBinaries.toUtf8());
            carla_stdout("  Resources dir:  %s", pathResources.toUtf8());
            */

            std::exit(0);
        }
    }

    return initName;
}

#if 0
//---------------------------------------------------------------------------------------------------------------------
// Get Icon from user theme, using our own as backup (Oxygen)

def getIcon(icon, size = 16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

//---------------------------------------------------------------------------------------------------------------------
// QLineEdit and QPushButton combo

def getAndSetPath(parent, lineEdit):
    newPath = QFileDialog.getExistingDirectory(parent, parent.tr("Set Path"), lineEdit.text(), QFileDialog.ShowDirsOnly)
    if newPath:
        lineEdit.setText(newPath)
    return newPath
#endif

//---------------------------------------------------------------------------------------------------------------------
// Check if a string array contains a string

static inline
bool stringArrayContainsString(const char* const* const stringArray, const char* const string) noexcept
{
    for (uint i=0; stringArray[i] != nullptr; ++i)
    {
        if (std::strcmp(stringArray[i], string) == 0)
            return true;
    }

    return false;
}

static inline
void fillQStringListFromStringArray(QStringList& list, const char* const* const stringArray)
{
    uint count = 0;

    // count number of strings first
    for (; stringArray[count] != nullptr; ++count) {}

    // allocate list
    list.reserve(count);

    // fill in strings
    for (count = 0; stringArray[count] != nullptr; ++count)
        list.replace(count, stringArray[count]);
}

//---------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on qt version

static inline
int fontMetricsHorizontalAdvance(const QFontMetrics& fm, const QString& s)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    return fm.horizontalAdvance(s);
#else
    return fm.width(s);
#endif
}

//---------------------------------------------------------------------------------------------------------------------
// Custom QString class with a few extra methods

class QCarlaString : public QString
{
public:
    QCarlaString()
        : QString() {}

    QCarlaString(const char* const ch)
        : QString(ch) {}

    QCarlaString(const QString& s)
        : QString(s) {}

    inline bool isNotEmpty() const
    {
        return !isEmpty();
    }

    inline QCarlaString& operator=(const char* const ch)
    {
        return (*this = fromUtf8(ch));
    }
};

#if 0
//---------------------------------------------------------------------------------------------------------------------
// Custom QMessageBox which resizes itself to fit text

class QMessageBoxWithBetterWidth(QMessageBox):
    def __init__(self, parent):
        QMessageBox.__init__(self, parent)

    def showEvent(self, event):
        fontMetrics = self.fontMetrics()

        lines = self.text().strip().split("\n") + self.informativeText().strip().split("\n")

        if len(lines) > 0:
            width = 0

            for line in lines:
                width = max(fontMetrics.width(line), width)

            self.layout().setColumnMinimumWidth(2, width + 12)

        QMessageBox.showEvent(self, event)
#endif

//---------------------------------------------------------------------------------------------------------------------
// Safer QSettings class, which does not throw if type mismatches

class QSafeSettings : public QSettings
{
public:
    QSafeSettings()
        : QSettings() {}

    QSafeSettings(const QString organizationName, const QString applicationName)
        : QSettings(organizationName, applicationName) {}

    bool valueBool(const QString key, const bool defaultValue) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::Bool), defaultValue);

        return var.isValid() ? var.toBool() : defaultValue;
    }

    uint valueUInt(const QString key, const uint defaultValue) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::UInt), defaultValue);

        return var.isValid() ? var.toUInt() : defaultValue;
    }

    double valueDouble(const QString key, const double defaultValue) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::Double), defaultValue);

        return var.isValid() ? var.toDouble() : defaultValue;
    }

    QString valueString(const QString key, const QString defaultValue) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::String), defaultValue);

        return var.isValid() ? var.toString() : defaultValue;
    }

    QByteArray valueByteArray(const QString key, const QByteArray defaultValue = QByteArray()) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::ByteArray), defaultValue);

        return var.isValid() ? var.toByteArray() : defaultValue;
    }

    QStringList valueStringList(const QString key, const QStringList defaultValue = QStringList()) const
    {
        QVariant var(value(key, defaultValue));
        CARLA_SAFE_ASSERT_RETURN(var.convert(QVariant::StringList), defaultValue);

        return var.isValid() ? var.toStringList() : defaultValue;
    }
};

#if 0
//---------------------------------------------------------------------------------------------------------------------
// Custom MessageBox

def CustomMessageBox(parent, icon, title, text,
                     extraText="",
                     buttons=QMessageBox.Yes|QMessageBox.No,
                     defButton=QMessageBox.No):
    msgBox = QMessageBoxWithBetterWidth(parent)
    msgBox.setIcon(icon)
    msgBox.setWindowTitle(title)
    msgBox.setText(text)
    msgBox.setInformativeText(extraText)
    msgBox.setStandardButtons(buttons)
    msgBox.setDefaultButton(defButton)
    return msgBox.exec_()
#endif

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_SHARED_HPP_INCLUDED
