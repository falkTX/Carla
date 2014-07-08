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

#include "carla_shared.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLineEdit>

//#ifdef CARLA_OS_MAC
//#endif

CARLA_BACKEND_USE_NAMESPACE

// ------------------------------------------------------------------------------------------------------------
// Misc functions

static
QString getenvWithFallback(const char* const key, const QString& fallback)
{
    if (const char* const value = std::getenv(key))
        return key;
    return fallback;
}

// ------------------------------------------------------------------------------------------------------------
// Global objects

QString TMP;
QString HOME;
QStringList PATH;
QStringList MIDI_CC_LIST;
CarlaObject gCarla;

// ------------------------------------------------------------------------------------------------------------
// init

static
void init()
{
    // --------------------------------------------------------------------------------------------------------
    // Platform specific stuff

#if defined(CARLA_OS_MAC)
    qt_mac_set_menubar_icons(false);
#elif defined(CARLA_OS_WIN)
    QString WINDIR = std::getenv("WINDIR");
#endif

    // --------------------------------------------------------------------------------------------------------
    // Set TMP

    const char* const envTMP = std::getenv("TMP");

    if (envTMP == nullptr)
    {
#ifdef CARLA_OS_WIN
        qWarning("TMP variable not set");
#endif
        TMP = QDir::tempPath();
    }
    else
        TMP = envTMP;

    if (! QDir(TMP).exists())
    {
        qWarning("TMP does not exist");
        TMP = "/";
    }

    // --------------------------------------------------------------------------------------------------------
    // Set HOME

    const char* const envHOME = std::getenv("HOME");

    if (envHOME == nullptr)
    {
#if defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC)
        qWarning("HOME variable not set");
#endif
        HOME = QDir::homePath();
    }
    else
        HOME = envHOME;

    if (! QDir(HOME).exists())
    {
        qWarning("HOME does not exist");
        HOME = TMP;
    }

    // --------------------------------------------------------------------------------------------------------
    // Set PATH

    const char* const envPATH = std::getenv("PATH");

    if (envPATH == nullptr)
    {
        qWarning("PATH variable not set");
        PATH.clear();

#if defined(CARLA_OS_MAC)
        PATH << "/opt/local/bin";
        PATH << "/usr/local/bin";
        PATH << "/usr/bin";
        PATH << "/bin";
#elif defined(CARLA_OS_WIN)
        PATH << QDir(WINDIR).filePath("system32");
        PATH << WINDIR;
#else
        PATH << "/usr/local/bin";
        PATH << "/usr/bin";
        PATH << "/bin";
#endif
    }
    else
    {
#ifdef CARLA_OS_WIN
        PATH = QString(envPATH).split(":");
#else
        PATH = QString(envPATH).split(";");
#endif
    }

    // --------------------------------------------------------------------------------------------------------
    // Static MIDI CC list

    MIDI_CC_LIST.clear();

    MIDI_CC_LIST << "0x01 Modulation";
    MIDI_CC_LIST << "0x02 Breath";
    MIDI_CC_LIST << "0x03 (Undefined)";
    MIDI_CC_LIST << "0x04 Foot";
    MIDI_CC_LIST << "0x05 Portamento";
    MIDI_CC_LIST << "0x07 Volume";
    MIDI_CC_LIST << "0x08 Balance";
    MIDI_CC_LIST << "0x09 (Undefined)";
    MIDI_CC_LIST << "0x0A Pan";
    MIDI_CC_LIST << "0x0B Expression";
    MIDI_CC_LIST << "0x0C FX Control 1";
    MIDI_CC_LIST << "0x0D FX Control 2";
    MIDI_CC_LIST << "0x0E (Undefined)";
    MIDI_CC_LIST << "0x0F (Undefined)";
    MIDI_CC_LIST << "0x10 General Purpose 1";
    MIDI_CC_LIST << "0x11 General Purpose 2";
    MIDI_CC_LIST << "0x12 General Purpose 3";
    MIDI_CC_LIST << "0x13 General Purpose 4";
    MIDI_CC_LIST << "0x14 (Undefined)";
    MIDI_CC_LIST << "0x15 (Undefined)";
    MIDI_CC_LIST << "0x16 (Undefined)";
    MIDI_CC_LIST << "0x17 (Undefined)";
    MIDI_CC_LIST << "0x18 (Undefined)";
    MIDI_CC_LIST << "0x19 (Undefined)";
    MIDI_CC_LIST << "0x1A (Undefined)";
    MIDI_CC_LIST << "0x1B (Undefined)";
    MIDI_CC_LIST << "0x1C (Undefined)";
    MIDI_CC_LIST << "0x1D (Undefined)";
    MIDI_CC_LIST << "0x1E (Undefined)";
    MIDI_CC_LIST << "0x1F (Undefined)";
    MIDI_CC_LIST << "0x46 Control 1 [Variation]";
    MIDI_CC_LIST << "0x47 Control 2 [Timbre]";
    MIDI_CC_LIST << "0x48 Control 3 [Release]";
    MIDI_CC_LIST << "0x49 Control 4 [Attack]";
    MIDI_CC_LIST << "0x4A Control 5 [Brightness]";
    MIDI_CC_LIST << "0x4B Control 6 [Decay]";
    MIDI_CC_LIST << "0x4C Control 7 [Vib Rate]";
    MIDI_CC_LIST << "0x4D Control 8 [Vib Depth]";
    MIDI_CC_LIST << "0x4E Control 9 [Vib Delay]";
    MIDI_CC_LIST << "0x4F Control 10 [Undefined]";
    MIDI_CC_LIST << "0x50 General Purpose 5";
    MIDI_CC_LIST << "0x51 General Purpose 6";
    MIDI_CC_LIST << "0x52 General Purpose 7";
    MIDI_CC_LIST << "0x53 General Purpose 8";
    MIDI_CC_LIST << "0x54 Portamento Control";
    MIDI_CC_LIST << "0x5B FX 1 Depth [Reverb]";
    MIDI_CC_LIST << "0x5C FX 2 Depth [Tremolo]";
    MIDI_CC_LIST << "0x5D FX 3 Depth [Chorus]";
    MIDI_CC_LIST << "0x5E FX 4 Depth [Detune]";
    MIDI_CC_LIST << "0x5F FX 5 Depth [Phaser]";

    // --------------------------------------------------------------------------------------------------------
    // Global Carla object

    gCarla.host = nullptr;
    gCarla.gui = nullptr;
    gCarla.isControl = false;
    gCarla.isLocal = true;
    gCarla.isPlugin = false;
    gCarla.bufferSize = 0;
    gCarla.sampleRate = 0.0;
#ifdef CARLA_OS_LINUX
    gCarla.processMode = CarlaBackend::ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS;
#else
    gCarla.processMode = CarlaBackend::ENGINE_PROCESS_MODE_CONTINUOUS_RACK;
#endif
    gCarla.processModeForced = false;
#ifdef CARLA_OS_LINUX
    gCarla.transportMode = CarlaBackend::ENGINE_TRANSPORT_MODE_JACK;
#else
    gCarla.transportMode = CarlaBackend::ENGINE_TRANSPORT_MODE_INTERNAL;
#endif
    gCarla.maxParameters = CarlaBackend::MAX_DEFAULT_PARAMETERS;
    gCarla.pathBinaries  = "";
    gCarla.pathResources = "";

    // --------------------------------------------------------------------------------------------------------
    // Default Plugin Folders (get)

    QString splitter;
    QString DEFAULT_LADSPA_PATH, DEFAULT_DSSI_PATH, DEFAULT_LV2_PATH, DEFAULT_VST_PATH, DEFAULT_VST3_PATH, DEFAULT_AU_PATH;
    QString DEFAULT_GIG_PATH, DEFAULT_SF2_PATH, DEFAULT_SFZ_PATH;

#if defined(CARLA_OS_WIN)
    splitter = ";";

    const char* const envAPPDATA = std::getenv("APPDATA");
    const char* const envPROGRAMFILES = std::getenv("PROGRAMFILES");
    const char* const envPROGRAMFILESx86 = std::getenv("PROGRAMFILES(x86)");
    const char* const envCOMMONPROGRAMFILES = std::getenv("COMMONPROGRAMFILES");

    // Small integrity tests
    if (envAPPDATA == nullptr)
    {
        qFatal("APPDATA variable not set, cannot continue");
        std::exit(1);
    }

    if (envPROGRAMFILES == nullptr)
    {
        qFatal("PROGRAMFILES variable not set, cannot continue");
        std::exit(1);
    }

    if (envCOMMONPROGRAMFILES == nullptr)
    {
        qFatal("COMMONPROGRAMFILES variable not set, cannot continue");
        std::exit(1);
    }

    QString APPDATA(envAPPDATA);
    QString PROGRAMFILES(envPROGRAMFILES);
    QString COMMONPROGRAMFILES(envCOMMONPROGRAMFILES);

    DEFAULT_LADSPA_PATH  = APPDATA + "\\LADSPA";
    DEFAULT_LADSPA_PATH += ";" + PROGRAMFILES + "\\LADSPA";

    DEFAULT_DSSI_PATH    = APPDATA + "\\DSSI";
    DEFAULT_DSSI_PATH   += ";" + PROGRAMFILES + "\\DSSI";

    DEFAULT_LV2_PATH     = APPDATA + "\\LV2";
    DEFAULT_LV2_PATH    += ";" + COMMONPROGRAMFILES + "\\LV2";

    DEFAULT_VST_PATH     = PROGRAMFILES + "\\VstPlugins";
    DEFAULT_VST_PATH    += ";" + PROGRAMFILES + "\\Steinberg\\VstPlugins";

    DEFAULT_VST3_PATH    = PROGRAMFILES + "\\Vst3";

    DEFAULT_GIG_PATH     = APPDATA + "\\GIG";
    DEFAULT_SF2_PATH     = APPDATA + "\\SF2";
    DEFAULT_SFZ_PATH     = APPDATA + "\\SFZ";

    if (envPROGRAMFILESx86 != nullptr)
    {
        QString PROGRAMFILESx86(envPROGRAMFILESx86);
        DEFAULT_LADSPA_PATH += ";" + PROGRAMFILESx86 + "\\LADSPA";
        DEFAULT_DSSI_PATH   += ";" + PROGRAMFILESx86 + "\\DSSI";
        DEFAULT_VST_PATH    += ";" + PROGRAMFILESx86 + "\\VstPlugins";
        DEFAULT_VST_PATH    += ";" + PROGRAMFILESx86 + "\\Steinberg\\VstPlugins";
    }
#elif defined(CARLA_OS_HAIKU)
    splitter = ":";

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa";
    DEFAULT_LADSPA_PATH += ":/boot/common/add-ons/ladspa";

    DEFAULT_DSSI_PATH    = HOME + "/.dssi";
    DEFAULT_DSSI_PATH   += ":/boot/common/add-ons/dssi";

    DEFAULT_LV2_PATH     = HOME + "/.lv2";
    DEFAULT_LV2_PATH    += ":/boot/common/add-ons/lv2";

    DEFAULT_VST_PATH     = HOME + "/.vst";
    DEFAULT_VST_PATH    += ":/boot/common/add-ons/vst";

    DEFAULT_VST3_PATH    = HOME + "/.vst3";
    DEFAULT_VST3_PATH   += ":/boot/common/add-ons/vst3";
#elif defined(CARLA_OS_MAC)
    splitter = ":";

    DEFAULT_LADSPA_PATH  = HOME + "/Library/Audio/Plug-Ins/LADSPA";
    DEFAULT_LADSPA_PATH += ":/Library/Audio/Plug-Ins/LADSPA";

    DEFAULT_DSSI_PATH    = HOME + "/Library/Audio/Plug-Ins/DSSI";
    DEFAULT_DSSI_PATH   += ":/Library/Audio/Plug-Ins/DSSI";

    DEFAULT_LV2_PATH     = HOME + "/Library/Audio/Plug-Ins/LV2";
    DEFAULT_LV2_PATH    += ":/Library/Audio/Plug-Ins/LV2";

    DEFAULT_VST_PATH     = HOME + "/Library/Audio/Plug-Ins/VST";
    DEFAULT_VST_PATH    += ":/Library/Audio/Plug-Ins/VST";

    DEFAULT_VST3_PATH    = HOME + "/Library/Audio/Plug-Ins/VST3";
    DEFAULT_VST3_PATH   += ":/Library/Audio/Plug-Ins/VST3";

    DEFAULT_AU_PATH      = HOME + "/Library/Audio/Plug-Ins/Components";
    DEFAULT_AU_PATH     += ":/Library/Audio/Plug-Ins/Components";
#else
    splitter = ":";

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa";
    DEFAULT_LADSPA_PATH += ":/usr/lib/ladspa";
    DEFAULT_LADSPA_PATH += ":/usr/local/lib/ladspa";

    DEFAULT_DSSI_PATH    = HOME + "/.dssi";
    DEFAULT_DSSI_PATH   += ":/usr/lib/dssi";
    DEFAULT_DSSI_PATH   += ":/usr/local/lib/dssi";

    DEFAULT_LV2_PATH     = HOME + "/.lv2";
    DEFAULT_LV2_PATH    += ":/usr/lib/lv2";
    DEFAULT_LV2_PATH    += ":/usr/local/lib/lv2";

    DEFAULT_VST_PATH     = HOME + "/.vst";
    DEFAULT_VST_PATH    += ":/usr/lib/vst";
    DEFAULT_VST_PATH    += ":/usr/local/lib/vst";

    DEFAULT_VST3_PATH    = HOME + "/.vst3";
    DEFAULT_VST3_PATH   += ":/usr/lib/vst3";
    DEFAULT_VST3_PATH   += ":/usr/local/lib/vst3";

    DEFAULT_GIG_PATH     = HOME + "/.sounds/gig";
    DEFAULT_GIG_PATH    += ":/usr/share/sounds/gig";

    DEFAULT_SF2_PATH     = HOME + "/.sounds/sf2";
    DEFAULT_SF2_PATH    += ":/usr/share/sounds/sf2";

    DEFAULT_SFZ_PATH     = HOME + "/.sounds/sfz";
    DEFAULT_SFZ_PATH    += ":/usr/share/sounds/sfz";
#endif

#ifndef CARLA_OS_WIN
    QString winePrefix = std::getenv("WINEPREFIX");

    if (winePrefix.isEmpty())
        winePrefix = HOME + "/.wine";

    if (QDir(winePrefix).exists())
    {
        DEFAULT_VST_PATH  += ":" + winePrefix + "/drive_c/Program Files/VstPlugins";
        DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST3";

# if defined (__LP64__) || defined (_LP64)
        if (QDir(winePrefix + "/drive_c/Program Files (x86)").exists())
        {
            DEFAULT_VST_PATH  += ":" + winePrefix + "/drive_c/Program Files (x86)/VstPlugins";
            DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST3";
        }
# endif
    }
#endif

    // --------------------------------------------------------------------------------------------------------
    // Default Plugin Folders (set)

    bool readEnvVars = true;

#if 0 //def CARLA_OS_WIN
    // Check if running Wine. If yes, ignore env vars
    from winreg import ConnectRegistry, OpenKey, CloseKey, HKEY_CURRENT_USER;
    reg = ConnectRegistry(None, HKEY_CURRENT_USER);


    key = OpenKey(reg, r"SOFTWARE\Wine");
    CloseKey(key);
    readEnvVars = False;

    CloseKey(reg);
#endif

    if (readEnvVars)
    {
        gCarla.DEFAULT_LADSPA_PATH = getenvWithFallback("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter);
        gCarla.DEFAULT_DSSI_PATH   = getenvWithFallback("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter);
        gCarla.DEFAULT_LV2_PATH    = getenvWithFallback("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter);
        gCarla.DEFAULT_VST_PATH    = getenvWithFallback("VST_PATH",    DEFAULT_VST_PATH).split(splitter);
        gCarla.DEFAULT_VST3_PATH   = getenvWithFallback("VST3_PATH",   DEFAULT_VST3_PATH).split(splitter);
        gCarla.DEFAULT_AU_PATH     = getenvWithFallback("AU_PATH",     DEFAULT_AU_PATH).split(splitter);
        gCarla.DEFAULT_GIG_PATH    = getenvWithFallback("GIG_PATH",    DEFAULT_GIG_PATH).split(splitter);
        gCarla.DEFAULT_SF2_PATH    = getenvWithFallback("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter);
        gCarla.DEFAULT_SFZ_PATH    = getenvWithFallback("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter);
    }
    else
    {
        gCarla.DEFAULT_LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter);
        gCarla.DEFAULT_DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter);
        gCarla.DEFAULT_LV2_PATH    = DEFAULT_LV2_PATH.split(splitter);
        gCarla.DEFAULT_VST_PATH    = DEFAULT_VST_PATH.split(splitter);
        gCarla.DEFAULT_VST3_PATH   = DEFAULT_VST3_PATH.split(splitter);
        gCarla.DEFAULT_AU_PATH     = DEFAULT_AU_PATH.split(splitter);
        gCarla.DEFAULT_GIG_PATH    = DEFAULT_GIG_PATH.split(splitter);
        gCarla.DEFAULT_SF2_PATH    = DEFAULT_SF2_PATH.split(splitter);
        gCarla.DEFAULT_SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter);
    }
}

// ------------------------------------------------------------------------------------------------------------
// find tool

QString findTool(const QString& toolName)
{
    QString path;

    path = QDir::current().filePath(toolName);
    if (QFile(path).exists())
        return path;

    if (! gCarla.pathBinaries.isEmpty())
    {
        path = QDir(gCarla.pathBinaries).filePath(toolName);
        if (QFile(path).exists())
            return path;
    }

    foreach (const QString& p, PATH)
    {
        path = QDir(p).filePath(toolName);
        if (QFile(path).exists())
            return path;
    }

    return "";
}

// ------------------------------------------------------------------------------------------------------------
// Init host

void initHost(const char* const initName, const char* const libPrefix, bool failError)
{
    init();

    // -------------------------------------------------------------
    // Set Carla library name

    QString libname = "libcarla_";

    if (gCarla.isControl)
        libname += "control2";
    else
        libname += "standalone2";

#if defined(CARLA_OS_WIN)
    libname += ".dll";
#elif defined(CARLA_OS_MAC)
    libname += ".dylib";
#else
    libname += ".so";
#endif

    // -------------------------------------------------------------
    // Set binary dir

    QString CWD = QDir::current().absolutePath();

    if (libPrefix != nullptr && libPrefix[0] != '\0')
    {
        QDir tmp(libPrefix);
        tmp.cd("lib");
        tmp.cd("carla");

        gCarla.pathBinaries = tmp.absolutePath();
    }
    else if (CWD.endsWith("resources", Qt::CaseInsensitive))
    {
        QDir tmp(CWD);
        tmp.cdUp();

        gCarla.pathBinaries = tmp.absolutePath();
    }
    else if (CWD.endsWith("source", Qt::CaseInsensitive))
    {
        QDir tmp(CWD);
        tmp.cdUp();
        tmp.cd("bin");

        gCarla.pathBinaries = tmp.absolutePath();
    }
    else if (CWD.endsWith("bin", Qt::CaseInsensitive))
    {
        gCarla.pathBinaries = CWD;
    }

    // -------------------------------------------------------------
    // Fail if binary dir is not found

    if (gCarla.pathBinaries.isEmpty() && ! gCarla.isPlugin)
    {
        if (failError)
        {
            QMessageBox::critical(nullptr, "Error", "Failed to find the carla library, cannot continue");
            std::exit(1);
        }
        return;
    }

    // -------------------------------------------------------------
    // Set resources dir

    if (libPrefix != nullptr && libPrefix[0] != '\0')
    {
        QDir tmp(libPrefix);
        tmp.cd("share");
        tmp.cd("carla");
        tmp.cd("resources");

        gCarla.pathResources = tmp.absolutePath();
    }
    else
    {
        QDir tmp(gCarla.pathBinaries);
        tmp.cd("resources");

        gCarla.pathResources = tmp.absolutePath();
    }

    // -------------------------------------------------------------
    // Print info

    carla_stdout("Carla %s started, status:", VERSION);
    carla_stdout("  binary dir:    %s", gCarla.pathBinaries.toUtf8().constData());
    carla_stdout("  resources dir: %s", gCarla.pathResources.toUtf8().constData());

    // -------------------------------------------------------------
    // Init host

    if (! (gCarla.isControl or gCarla.isPlugin))
        carla_set_engine_option(ENGINE_OPTION_NSM_INIT, getpid(), initName);

    carla_set_engine_option(ENGINE_OPTION_PATH_BINARIES,  0, gCarla.pathBinaries.toUtf8().constData());
    carla_set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, gCarla.pathResources.toUtf8().constData());
}

// ------------------------------------------------------------------------------------------------------------
// Get Icon from user theme, using our own as backup (Oxygen)

QIcon getIcon(const QString& icon, const int size)
{
    return QIcon::fromTheme(icon, QIcon(QString(":/%ix%i/%s.png").arg(size).arg(size).arg(icon)));
}

// ------------------------------------------------------------------------------------------------------------
// Signal handler

static inline // TODO - remove inline
void signalHandler(/*sig, frame*/)
{
    if (gCarla.gui == nullptr)
        return;

//    if (sig == SIGINT || sig == SIGTERM)
//        emit(gCarla.gui.SIGTERM);
#ifdef HAVE_SIGUSR1
//    else if (sig == SIGUSR1)
//        emit(gCarla.gui.SIGUSR1);
#endif
}

void setUpSignals()
{
    //signal(SIGINT,  signalHandler);
    //signal(SIGTERM, signalHandler);

#ifdef HAVE_SIGUSR1
    //signal(SIGUSR1, signalHandler);
#endif
}

// ------------------------------------------------------------------------------------------------------------
// QLineEdit and QPushButton combo

QString getAndSetPath(QWidget* const parent, const QString& currentPath, QLineEdit* const lineEdit)
{
    QString newPath = QFileDialog::getExistingDirectory(parent, parent->tr("Set Path"), currentPath, QFileDialog::ShowDirsOnly);

    if (! newPath.isEmpty())
        lineEdit->setText(newPath);

    return newPath;
}

// ------------------------------------------------------------------------------------------------------------
// Custom MessageBox

int CustomMessageBox(QWidget* const parent, const QMessageBox::Icon icon, const QString& title, const QString& text, const QString& extraText, const QMessageBox::StandardButtons buttons, const QMessageBox::StandardButton defButton)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setInformativeText(extraText);
    msgBox.setStandardButtons(buttons);
    msgBox.setDefaultButton(defButton);
    return msgBox.exec();
}

// ------------------------------------------------------------------------------------------------------------
