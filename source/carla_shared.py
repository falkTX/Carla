#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Config)

from carla_config import *

# These will be modified during install
X_LIBDIR_X = None
X_DATADIR_X = None

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
import sys

if config_UseQt5:
    from PyQt5.Qt import PYQT_VERSION_STR
    from PyQt5.QtCore import qFatal, qVersion, qWarning, QDir
    from PyQt5.QtGui import QIcon
    from PyQt5.QtWidgets import QFileDialog, QMessageBox
else:
    from PyQt4.Qt import PYQT_VERSION_STR
    from PyQt4.QtCore import qFatal, qVersion, qWarning, QDir
    from PyQt4.QtGui import QFileDialog, QIcon, QMessageBox

# ------------------------------------------------------------------------------------------------------------
# Import Signal

from signal import signal, SIGINT, SIGTERM

try:
    from signal import SIGUSR1
    haveSIGUSR1 = True
except:
    haveSIGUSR1 = False

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend_qt import *

# ------------------------------------------------------------------------------------------------------------
# Platform specific stuff

if WINDOWS:
    WINDIR = os.getenv("WINDIR")

# ------------------------------------------------------------------------------------------------------------
# Set Version

VERSION = "1.9.6 (2.0-beta4)"

# ------------------------------------------------------------------------------------------------------------
# Set TMP

envTMP = os.getenv("TMP")

if envTMP is None:
    if WINDOWS:
        qWarning("TMP variable not set")
    TMP = QDir.tempPath()
else:
    TMP = envTMP

if not os.path.exists(TMP):
    qWarning("TMP does not exist")
    TMP = "/"

del envTMP

# ------------------------------------------------------------------------------------------------------------
# Set HOME

envHOME = os.getenv("HOME")

if envHOME is None:
    if LINUX or MACOS:
        qWarning("HOME variable not set")
    HOME = QDir.toNativeSeparators(QDir.homePath())
else:
    HOME = envHOME

if not os.path.exists(HOME):
    qWarning("HOME does not exist")
    HOME = TMP

del envHOME

# ------------------------------------------------------------------------------------------------------------
# Set PATH

envPATH = os.getenv("PATH")

if envPATH is None:
    qWarning("PATH variable not set")
    if MACOS:
        PATH = ("/opt/local/bin", "/usr/local/bin", "/usr/bin", "/bin")
    elif WINDOWS:
        PATH = (os.path.join(WINDIR, "system32"), WINDIR)
    else:
        PATH = ("/usr/local/bin", "/usr/bin", "/bin")
else:
    PATH = envPATH.split(os.pathsep)

del envPATH

# ------------------------------------------------------------------------------------------------------------
# Static MIDI CC list

MIDI_CC_LIST = (
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
    "0x5F FX 5 Depth [Phaser]"
)

# ------------------------------------------------------------------------------------------------------------
# PatchCanvas defines

CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# ------------------------------------------------------------------------------------------------------------
# Carla Settings keys

CARLA_KEY_MAIN_PROJECT_FOLDER   = "Main/ProjectFolder"   # str
CARLA_KEY_MAIN_USE_PRO_THEME    = "Main/UseProTheme"     # bool
CARLA_KEY_MAIN_PRO_THEME_COLOR  = "Main/ProThemeColor"   # str
CARLA_KEY_MAIN_REFRESH_INTERVAL = "Main/RefreshInterval" # int
CARLA_KEY_MAIN_USE_CUSTOM_SKINS = "Main/UseCustomSkins"  # bool

CARLA_KEY_CANVAS_THEME             = "Canvas/Theme"           # str
CARLA_KEY_CANVAS_SIZE              = "Canvas/Size"            # str "NxN"
CARLA_KEY_CANVAS_USE_BEZIER_LINES  = "Canvas/UseBezierLines"  # bool
CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS  = "Canvas/AutoHideGroups"  # bool
CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS = "Canvas/AutoSelectItems" # bool
CARLA_KEY_CANVAS_EYE_CANDY         = "Canvas/EyeCandy"        # enum
CARLA_KEY_CANVAS_USE_OPENGL        = "Canvas/UseOpenGL"       # bool
CARLA_KEY_CANVAS_ANTIALIASING      = "Canvas/Antialiasing"    # enum
CARLA_KEY_CANVAS_HQ_ANTIALIASING   = "Canvas/HQAntialiasing"  # bool

CARLA_KEY_ENGINE_DRIVER_PREFIX         = "Engine/Driver-"
CARLA_KEY_ENGINE_AUDIO_DRIVER          = "Engine/AudioDriver"         # str
CARLA_KEY_ENGINE_PROCESS_MODE          = "Engine/ProcessMode"         # enum
CARLA_KEY_ENGINE_TRANSPORT_MODE        = "Engine/TransportMode"       # enum
CARLA_KEY_ENGINE_FORCE_STEREO          = "Engine/ForceStereo"         # bool
CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES = "Engine/PreferPluginBridges" # bool
CARLA_KEY_ENGINE_PREFER_UI_BRIDGES     = "Engine/PreferUiBridges"     # bool
CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP     = "Engine/UIsAlwaysOnTop"      # bool
CARLA_KEY_ENGINE_MAX_PARAMETERS        = "Engine/MaxParameters"       # int
CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT    = "Engine/UiBridgesTimeout"    # int

CARLA_KEY_PATHS_LADSPA = "Paths/LADSPA"
CARLA_KEY_PATHS_DSSI   = "Paths/DSSI"
CARLA_KEY_PATHS_LV2    = "Paths/LV2"
CARLA_KEY_PATHS_VST2   = "Paths/VST2"
CARLA_KEY_PATHS_VST3   = "Paths/VST3"
CARLA_KEY_PATHS_GIG    = "Paths/GIG"
CARLA_KEY_PATHS_SF2    = "Paths/SF2"
CARLA_KEY_PATHS_SFZ    = "Paths/SFZ"

# if pro theme is on and color is black
CARLA_KEY_CUSTOM_PAINTING = "UseCustomPainting" # bool

# ------------------------------------------------------------------------------------------------------------
# Carla Settings defaults

# Main
CARLA_DEFAULT_MAIN_PROJECT_FOLDER   = HOME
CARLA_DEFAULT_MAIN_USE_PRO_THEME    = True
CARLA_DEFAULT_MAIN_PRO_THEME_COLOR  = "Black"
CARLA_DEFAULT_MAIN_REFRESH_INTERVAL = 20
CARLA_DEFAULT_MAIN_USE_CUSTOM_SKINS = True

# Canvas
CARLA_DEFAULT_CANVAS_THEME             = "Modern Dark"
CARLA_DEFAULT_CANVAS_SIZE              = "3100x2400"
CARLA_DEFAULT_CANVAS_SIZE_WIDTH        = 3100
CARLA_DEFAULT_CANVAS_SIZE_HEIGHT       = 2400
CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES  = True
CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS  = True
CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS = False
CARLA_DEFAULT_CANVAS_EYE_CANDY         = CANVAS_EYECANDY_SMALL
CARLA_DEFAULT_CANVAS_USE_OPENGL        = False
CARLA_DEFAULT_CANVAS_ANTIALIASING      = CANVAS_ANTIALIASING_SMALL
CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING   = False

# Engine
CARLA_DEFAULT_FORCE_STEREO          = False
CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES = False
CARLA_DEFAULT_PREFER_UI_BRIDGES     = bool(not WINDOWS)
CARLA_DEFAULT_UIS_ALWAYS_ON_TOP     = False
CARLA_DEFAULT_MAX_PARAMETERS        = MAX_DEFAULT_PARAMETERS
CARLA_DEFAULT_UI_BRIDGES_TIMEOUT    = 4000

CARLA_DEFAULT_AUDIO_NUM_PERIODS     = 2
CARLA_DEFAULT_AUDIO_BUFFER_SIZE     = 512
CARLA_DEFAULT_AUDIO_SAMPLE_RATE     = 44100

if WINDOWS:
    CARLA_DEFAULT_AUDIO_DRIVER = "DirectSound"
elif MACOS:
    CARLA_DEFAULT_AUDIO_DRIVER = "CoreAudio"
else:
    CARLA_DEFAULT_AUDIO_DRIVER = "JACK"

if LINUX:
    CARLA_DEFAULT_PROCESS_MODE   = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS
    CARLA_DEFAULT_TRANSPORT_MODE = ENGINE_TRANSPORT_MODE_JACK
else:
    CARLA_DEFAULT_PROCESS_MODE   = ENGINE_PROCESS_MODE_PATCHBAY
    CARLA_DEFAULT_TRANSPORT_MODE = ENGINE_TRANSPORT_MODE_INTERNAL

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (get)

DEFAULT_LADSPA_PATH = ""
DEFAULT_DSSI_PATH   = ""
DEFAULT_LV2_PATH    = ""
DEFAULT_VST2_PATH   = ""
DEFAULT_VST3_PATH   = ""
DEFAULT_GIG_PATH    = ""
DEFAULT_SF2_PATH    = ""
DEFAULT_SFZ_PATH    = ""

if WINDOWS:
    splitter = ";"

    APPDATA = os.getenv("APPDATA")
    PROGRAMFILES = os.getenv("PROGRAMFILES")
    PROGRAMFILESx86 = os.getenv("PROGRAMFILES(x86)")
    COMMONPROGRAMFILES = os.getenv("COMMONPROGRAMFILES")
    COMMONPROGRAMFILESx86 = os.getenv("COMMONPROGRAMFILES(x86)")

    # Small integrity tests
    if not APPDATA:
        qFatal("APPDATA variable not set, cannot continue")
        sys.exit(1)

    if not PROGRAMFILES:
        qFatal("PROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    if not COMMONPROGRAMFILES:
        qFatal("COMMONPROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    DEFAULT_LADSPA_PATH  = APPDATA + "\\LADSPA"
    DEFAULT_LADSPA_PATH += ";" + PROGRAMFILES + "\\LADSPA"

    DEFAULT_DSSI_PATH    = APPDATA + "\\DSSI"
    DEFAULT_DSSI_PATH   += ";" + PROGRAMFILES + "\\DSSI"

    DEFAULT_LV2_PATH     = APPDATA + "\\LV2"
    DEFAULT_LV2_PATH    += ";" + COMMONPROGRAMFILES + "\\LV2"

    DEFAULT_VST2_PATH    = PROGRAMFILES + "\\VstPlugins"
    DEFAULT_VST2_PATH   += ";" + PROGRAMFILES + "\\Steinberg\\VstPlugins"

    if kIs64bit:
        DEFAULT_VST2_PATH  += ";" + COMMONPROGRAMFILES + "\\VST2"

    DEFAULT_VST3_PATH    = COMMONPROGRAMFILES + "\\VST3"

    DEFAULT_GIG_PATH     = APPDATA + "\\GIG"
    DEFAULT_SF2_PATH     = APPDATA + "\\SF2"
    DEFAULT_SFZ_PATH     = APPDATA + "\\SFZ"

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH += ";" + PROGRAMFILESx86 + "\\LADSPA"
        DEFAULT_DSSI_PATH   += ";" + PROGRAMFILESx86 + "\\DSSI"
        DEFAULT_VST2_PATH    += ";" + PROGRAMFILESx86 + "\\VstPlugins"
        DEFAULT_VST2_PATH    += ";" + PROGRAMFILESx86 + "\\Steinberg\\VstPlugins"

    if COMMONPROGRAMFILESx86:
        DEFAULT_VST3_PATH   += COMMONPROGRAMFILESx86 + "\\VST3"

elif HAIKU:
    splitter = ":"

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa"
    DEFAULT_LADSPA_PATH += ":/boot/common/add-ons/ladspa"

    DEFAULT_DSSI_PATH    = HOME + "/.dssi"
    DEFAULT_DSSI_PATH   += ":/boot/common/add-ons/dssi"

    DEFAULT_LV2_PATH     = HOME + "/.lv2"
    DEFAULT_LV2_PATH    += ":/boot/common/add-ons/lv2"

    DEFAULT_VST2_PATH    = HOME + "/.vst"
    DEFAULT_VST2_PATH   += ":/boot/common/add-ons/vst"

    DEFAULT_VST3_PATH    = HOME + "/.vst3"
    DEFAULT_VST3_PATH   += ":/boot/common/add-ons/vst3"

elif MACOS:
    splitter = ":"

    DEFAULT_LADSPA_PATH  = HOME + "/Library/Audio/Plug-Ins/LADSPA"
    DEFAULT_LADSPA_PATH += ":/Library/Audio/Plug-Ins/LADSPA"

    DEFAULT_DSSI_PATH    = HOME + "/Library/Audio/Plug-Ins/DSSI"
    DEFAULT_DSSI_PATH   += ":/Library/Audio/Plug-Ins/DSSI"

    DEFAULT_LV2_PATH     = HOME + "/Library/Audio/Plug-Ins/LV2"
    DEFAULT_LV2_PATH    += ":/Library/Audio/Plug-Ins/LV2"

    DEFAULT_VST2_PATH    = HOME + "/Library/Audio/Plug-Ins/VST"
    DEFAULT_VST2_PATH   += ":/Library/Audio/Plug-Ins/VST"

    DEFAULT_VST3_PATH    = HOME + "/Library/Audio/Plug-Ins/VST3"
    DEFAULT_VST3_PATH   += ":/Library/Audio/Plug-Ins/VST3"

else:
    splitter = ":"

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa"
    DEFAULT_LADSPA_PATH += ":/usr/lib/ladspa"
    DEFAULT_LADSPA_PATH += ":/usr/local/lib/ladspa"

    DEFAULT_DSSI_PATH    = HOME + "/.dssi"
    DEFAULT_DSSI_PATH   += ":/usr/lib/dssi"
    DEFAULT_DSSI_PATH   += ":/usr/local/lib/dssi"

    DEFAULT_LV2_PATH     = HOME + "/.lv2"
    DEFAULT_LV2_PATH    += ":/usr/lib/lv2"
    DEFAULT_LV2_PATH    += ":/usr/local/lib/lv2"

    DEFAULT_VST2_PATH    = HOME + "/.vst"
    DEFAULT_VST2_PATH   += ":/usr/lib/vst"
    DEFAULT_VST2_PATH   += ":/usr/local/lib/vst"

    DEFAULT_VST3_PATH    = HOME + "/.vst3"
    DEFAULT_VST3_PATH   += ":/usr/lib/vst3"
    DEFAULT_VST3_PATH   += ":/usr/local/lib/vst3"

    DEFAULT_GIG_PATH     = HOME + "/.sounds/gig"
    DEFAULT_GIG_PATH    += ":/usr/share/sounds/gig"

    DEFAULT_SF2_PATH     = HOME + "/.sounds/sf2"
    DEFAULT_SF2_PATH    += ":/usr/share/sounds/sf2"

    DEFAULT_SFZ_PATH     = HOME + "/.sounds/sfz"
    DEFAULT_SFZ_PATH    += ":/usr/share/sounds/sfz"

if not WINDOWS:
    winePrefix = os.getenv("WINEPREFIX")

    if not winePrefix:
        winePrefix = HOME + "/.wine"

    if os.path.exists(winePrefix):
        DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/VstPlugins"
        DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST3"

        if kIs64bit and os.path.exists(winePrefix + "/drive_c/Program Files (x86)"):
            DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/VstPlugins"
            DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST3"

    del winePrefix

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (set)

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

if readEnvVars:
    CARLA_DEFAULT_LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
    CARLA_DEFAULT_DSSI_PATH   = os.getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter)
    CARLA_DEFAULT_LV2_PATH    = os.getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter)
    CARLA_DEFAULT_VST2_PATH   = os.getenv("VST_PATH",    DEFAULT_VST2_PATH).split(splitter)
    CARLA_DEFAULT_VST3_PATH   = os.getenv("VST3_PATH",   DEFAULT_VST3_PATH).split(splitter)
    CARLA_DEFAULT_GIG_PATH    = os.getenv("GIG_PATH",    DEFAULT_GIG_PATH).split(splitter)
    CARLA_DEFAULT_SF2_PATH    = os.getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter)
    CARLA_DEFAULT_SFZ_PATH    = os.getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter)

else:
    CARLA_DEFAULT_LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter)
    CARLA_DEFAULT_DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter)
    CARLA_DEFAULT_LV2_PATH    = DEFAULT_LV2_PATH.split(splitter)
    CARLA_DEFAULT_VST2_PATH   = DEFAULT_VST2_PATH.split(splitter)
    CARLA_DEFAULT_VST3_PATH   = DEFAULT_VST3_PATH.split(splitter)
    CARLA_DEFAULT_GIG_PATH    = DEFAULT_GIG_PATH.split(splitter)
    CARLA_DEFAULT_SF2_PATH    = DEFAULT_SF2_PATH.split(splitter)
    CARLA_DEFAULT_SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter)

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (cleanup)

del DEFAULT_LADSPA_PATH
del DEFAULT_DSSI_PATH
del DEFAULT_LV2_PATH
del DEFAULT_VST2_PATH
del DEFAULT_VST3_PATH
del DEFAULT_GIG_PATH
del DEFAULT_SF2_PATH
del DEFAULT_SFZ_PATH

# ------------------------------------------------------------------------------------------------------------
# Global Carla object

class CarlaObject(object):
    __slots__ = [
        'gui',   # Host Window
        'nogui', # Skip UI
        'term',  # Terminated by OS signal
        'utils'  # Utils object
    ]

gCarla = CarlaObject()
gCarla.gui   = None
gCarla.nogui = False
gCarla.term  = False
gCarla.utils = None

# ------------------------------------------------------------------------------------------------------------
# Set CWD

CWD = sys.path[0]

if not CWD:
    CWD = os.path.dirname(sys.argv[0])

# make it work with cxfreeze
if os.path.isfile(CWD):
    CWD = os.path.dirname(CWD)
    CXFREEZE = True
else:
    CXFREEZE = False

# ------------------------------------------------------------------------------------------------------------
# Set DLL_EXTENSION

if WINDOWS:
    DLL_EXTENSION = "dll"
elif MACOS:
    DLL_EXTENSION = "dylib"
else:
    DLL_EXTENSION = "so"

# ------------------------------------------------------------------------------------------------------------
# Check if a value is a number (float support)

def isNumber(value):
    try:
        float(value)
        return True
    except:
        return False

# ------------------------------------------------------------------------------------------------------------
# Convert a value to a list

def toList(value):
    if value is None:
        return []
    elif not isinstance(value, list):
        return [value]
    else:
        return value

# ------------------------------------------------------------------------------------------------------------
# Get Icon from user theme, using our own as backup (Oxygen)

def getIcon(icon, size = 16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# ------------------------------------------------------------------------------------------------------------
# Handle some basic command-line arguments shared between all carla variants

def handleInitialCommandLineArguments(file):
    initName  = os.path.basename(file) if (file is not None and os.path.dirname(file) in PATH) else sys.argv[0]
    libPrefix = None

    for arg in sys.argv[1:]:
        if arg.startswith("--with-appname="):
            initName = os.path.basename(arg.replace("--with-initname=", ""))

        elif arg.startswith("--with-libprefix="):
            libPrefix = arg.replace("--with-libprefix=", "")

        elif arg == "--gdb":
            pass

        elif arg in ("-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui"):
            gCarla.nogui = True

        elif arg in ("-h", "--h", "-help", "--help"):
            print("Usage: %s [OPTION]... [FILE|URL]" % initName)
            print("")
            print(" where FILE can be a Carla project or preset file to be loaded, or URL if using Carla-Control")
            print("")
            print(" and OPTION can be one or more of the following:")
            print("")
            print("    --gdb    \t Run Carla inside gdb.")
            print(" -n,--no-gui \t Run Carla headless, don't show UI.")
            print("")
            print(" -h,--help   \t Print this help text and exit.")
            print(" -v,--version\t Print version information and exit.")
            print("")

            sys.exit(0)

        elif arg in ("-v", "--v", "-version", "--version"):
            pathBinaries, pathResources = getPaths(libPrefix)

            print("Using Carla version %s" % VERSION)
            print("  Python version: %s" % sys.version.split(" ",1)[0])
            print("  Qt version:     %s" % qVersion())
            print("  PyQt version:   %s" % PYQT_VERSION_STR)
            print("  Binary dir:     %s" % pathBinaries)
            print("  Resources dir:  %s" % pathResources)

            sys.exit(0)

    return (initName, libPrefix)

# ------------------------------------------------------------------------------------------------------------
# Get initial project file (as passed in the command-line parameters)

def getInitialProjectFile(app, skipExistCheck = False):
    for arg in app.arguments()[1:]:
        if arg.startswith("--with-appname=") or arg.startswith("--with-libprefix=") or arg == "--gdb":
            continue
        if arg in ("-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui"):
            continue
        if skipExistCheck or os.path.exists(arg):
            return arg

    return None

# ------------------------------------------------------------------------------------------------------------
# Get paths (binaries, resources)

def getPaths(libPrefix = None):
    CWDl = CWD.lower()

    # adjust for special distros
    libdir  = os.path.basename(os.path.normpath(X_LIBDIR_X))  if X_LIBDIR_X  else "lib"
    datadir = os.path.basename(os.path.normpath(X_DATADIR_X)) if X_DATADIR_X else "share"

    # standalone, installed system-wide linux
    if libPrefix is not None:
        pathBinaries  = os.path.join(libPrefix, libdir, "carla")
        pathResources = os.path.join(libPrefix, datadir, "carla", "resources")

    # standalone, local source
    elif CWDl.endswith("source"):
        pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "bin"))
        pathResources = os.path.join(pathBinaries, "resources")

    # plugin
    elif CWDl.endswith("resources"):
        # installed system-wide linux
        if CWDl.endswith("/share/carla/resources"):
            pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "..", "..", libdir, "carla"))
            pathResources = CWD

        # local source
        elif CWDl.endswith("native-plugins%sresources" % os.sep):
            pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "..", "..", "bin"))
            pathResources = CWD

        # other
        else:
            pathBinaries  = os.path.abspath(os.path.join(CWD, ".."))
            pathResources = CWD

    # everything else
    else:
        pathBinaries  = CWD
        pathResources = os.path.join(pathBinaries, "resources")

    return (pathBinaries, pathResources)

# ------------------------------------------------------------------------------------------------------------
# Signal handler
# TODO move to carla_host.py or something

def signalHandler(sig, frame):
    if sig in (SIGINT, SIGTERM):
        gCarla.term = True
        if gCarla.gui is not None:
            gCarla.gui.SIGTERM.emit()

    elif haveSIGUSR1 and sig == SIGUSR1:
        if gCarla.gui is not None:
            gCarla.gui.SIGUSR1.emit()

def setUpSignals():
    signal(SIGINT,  signalHandler)
    signal(SIGTERM, signalHandler)

    if not haveSIGUSR1:
        return

    signal(SIGUSR1, signalHandler)

# ------------------------------------------------------------------------------------------------------------
# QLineEdit and QPushButton combo

def getAndSetPath(parent, lineEdit):
    newPath = QFileDialog.getExistingDirectory(parent, parent.tr("Set Path"), lineEdit.text(), QFileDialog.ShowDirsOnly)
    if newPath:
        lineEdit.setText(newPath)
    return newPath

# ------------------------------------------------------------------------------------------------------------
# Custom MessageBox

def CustomMessageBox(parent, icon, title, text, extraText="", buttons=QMessageBox.Yes|QMessageBox.No, defButton=QMessageBox.No):
    msgBox = QMessageBox(parent)
    msgBox.setIcon(icon)
    msgBox.setWindowTitle(title)
    msgBox.setText(text)
    msgBox.setInformativeText(extraText)
    msgBox.setStandardButtons(buttons)
    msgBox.setDefaultButton(defButton)
    return msgBox.exec_()

# ------------------------------------------------------------------------------------------------------------
