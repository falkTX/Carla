#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
import sys

if config_UseQt5:
    from PyQt5.QtCore import qFatal, qWarning, QDir
    from PyQt5.QtGui import QIcon
    from PyQt5.QtWidgets import QFileDialog, QMessageBox
else:
    from PyQt4.QtCore import qFatal, qWarning, QDir
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

from carla_backend import *

# ------------------------------------------------------------------------------------------------------------
# Platform specific stuff

if MACOS:
    if config_UseQt5:
        from PyQt5.QtGui import qt_mac_set_menubar_icons
    else:
        from PyQt4.QtGui import qt_mac_set_menubar_icons
    qt_mac_set_menubar_icons(False)

elif WINDOWS:
    WINDIR = os.getenv("WINDIR")

# ------------------------------------------------------------------------------------------------------------
# Set Version

VERSION = "1.9.3 (2.0-beta1)"

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
    HOME = QDir.homePath()
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
# Carla Settings keys

CARLA_KEY_MAIN_PROJECT_FOLDER   = "Main/ProjectFolder"   # str
CARLA_KEY_MAIN_USE_PRO_THEME    = "Main/UseProTheme"     # bool
CARLA_KEY_MAIN_PRO_THEME_COLOR  = "Main/ProThemeColor"   # str
CARLA_KEY_MAIN_REFRESH_INTERVAL = "Main/RefreshInterval" # int

CARLA_KEY_CANVAS_THEME            = "Canvas/Theme"           # str
CARLA_KEY_CANVAS_SIZE             = "Canvas/Size"            # str "NxN"
CARLA_KEY_CANVAS_USE_BEZIER_LINES = "Canvas/UseBezierLines"  # bool
CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS = "Canvas/AutoHideGroups"  # bool
CARLA_KEY_CANVAS_EYE_CANDY        = "Canvas/EyeCandy"        # enum
CARLA_KEY_CANVAS_USE_OPENGL       = "Canvas/UseOpenGL"       # bool
CARLA_KEY_CANVAS_ANTIALIASING     = "Canvas/Antialiasing"    # enum
CARLA_KEY_CANVAS_HQ_ANTIALIASING  = "Canvas/HQAntialiasing"  # bool
CARLA_KEY_CUSTOM_PAINTING         = "UseCustomPainting"      # bool

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
CARLA_KEY_PATHS_VST    = "Paths/VST"
CARLA_KEY_PATHS_VST3   = "Paths/VST3"
CARLA_KEY_PATHS_AU     = "Paths/AU"
CARLA_KEY_PATHS_CSD    = "Paths/CSD"
CARLA_KEY_PATHS_GIG    = "Paths/GIG"
CARLA_KEY_PATHS_SF2    = "Paths/SF2"
CARLA_KEY_PATHS_SFZ    = "Paths/SFZ"

# ------------------------------------------------------------------------------------------------------------
# Global Carla object

class CarlaObject(object):
    __slots__ = [
        # Host library object
        'host',
        # Host Window
        'gui',
        # bool, is controller
        'isControl',
        # bool, is running local
        'isLocal',
        # bool, is plugin
        'isPlugin',
        # current buffer size
        'bufferSize',
        # current sample rate
        'sampleRate',
        # current process mode
        'processMode',
        # current transport mode
        'transportMode',
        # current max parameters
        'maxParameters',
        # discovery tools
        'discovery_native',
        'discovery_posix32',
        'discovery_posix64',
        'discovery_win32',
        'discovery_win64',
        # default paths
        'DEFAULT_LADSPA_PATH',
        'DEFAULT_DSSI_PATH',
        'DEFAULT_LV2_PATH',
        'DEFAULT_VST_PATH',
        'DEFAULT_VST3_PATH',
        'DEFAULT_AU_PATH',
        'DEFAULT_CSOUND_PATH',
        'DEFAULT_GIG_PATH',
        'DEFAULT_SF2_PATH',
        'DEFAULT_SFZ_PATH'
    ]

gCarla = CarlaObject()
gCarla.host = None
gCarla.gui  = None
gCarla.isControl = False
gCarla.isLocal   = True
gCarla.isPlugin  = False
gCarla.bufferSize = 0
gCarla.sampleRate = 0.0
gCarla.processMode   = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS if LINUX else ENGINE_PROCESS_MODE_CONTINUOUS_RACK
gCarla.transportMode = ENGINE_TRANSPORT_MODE_JACK if LINUX else ENGINE_TRANSPORT_MODE_INTERNAL
gCarla.maxParameters = MAX_DEFAULT_PARAMETERS
gCarla.discovery_native  = ""
gCarla.discovery_posix32 = ""
gCarla.discovery_posix64 = ""
gCarla.discovery_win32   = ""
gCarla.discovery_win64   = ""

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (get)

DEFAULT_LADSPA_PATH = ""
DEFAULT_DSSI_PATH   = ""
DEFAULT_LV2_PATH    = ""
DEFAULT_VST_PATH    = ""
DEFAULT_VST3_PATH   = ""
DEFAULT_AU_PATH     = ""
DEFAULT_CSOUND_PATH = ""
DEFAULT_GIG_PATH    = ""
DEFAULT_SF2_PATH    = ""
DEFAULT_SFZ_PATH    = ""

if WINDOWS:
    splitter = ";"

    APPDATA = os.getenv("APPDATA")
    PROGRAMFILES = os.getenv("PROGRAMFILES")
    PROGRAMFILESx86 = os.getenv("PROGRAMFILES(x86)")
    COMMONPROGRAMFILES = os.getenv("COMMONPROGRAMFILES")

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

    DEFAULT_VST_PATH     = PROGRAMFILES + "\\VstPlugins"
    DEFAULT_VST_PATH    += ";" + PROGRAMFILES + "\\Steinberg\\VstPlugins"

    DEFAULT_VST3_PATH    = PROGRAMFILES + "\\Vst3"

    DEFAULT_GIG_PATH     = APPDATA + "\\GIG"
    DEFAULT_SF2_PATH     = APPDATA + "\\SF2"
    DEFAULT_SFZ_PATH     = APPDATA + "\\SFZ"

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH += ";" + PROGRAMFILESx86 + "\\LADSPA"
        DEFAULT_DSSI_PATH   += ";" + PROGRAMFILESx86 + "\\DSSI"
        DEFAULT_VST_PATH    += ";" + PROGRAMFILESx86 + "\\VstPlugins"
        DEFAULT_VST_PATH    += ";" + PROGRAMFILESx86 + "\\Steinberg\\VstPlugins"

elif HAIKU:
    splitter = ":"

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa"
    DEFAULT_LADSPA_PATH += ":/boot/common/add-ons/ladspa"

    DEFAULT_DSSI_PATH    = HOME + "/.dssi"
    DEFAULT_DSSI_PATH   += ":/boot/common/add-ons/dssi"

    DEFAULT_LV2_PATH     = HOME + "/.lv2"
    DEFAULT_LV2_PATH    += ":/boot/common/add-ons/lv2"

    DEFAULT_VST_PATH     = HOME + "/.vst"
    DEFAULT_VST_PATH    += ":/boot/common/add-ons/vst"

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

    DEFAULT_VST_PATH     = HOME + "/Library/Audio/Plug-Ins/VST"
    DEFAULT_VST_PATH    += ":/Library/Audio/Plug-Ins/VST"

    DEFAULT_VST3_PATH    = HOME + "/Library/Audio/Plug-Ins/VST3"
    DEFAULT_VST3_PATH   += ":/Library/Audio/Plug-Ins/VST3"

    DEFAULT_AU_PATH      = HOME + "/Library/Audio/Plug-Ins/Components"
    DEFAULT_AU_PATH     += ":/Library/Audio/Plug-Ins/Components"

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

    DEFAULT_VST_PATH     = HOME + "/.vst"
    DEFAULT_VST_PATH    += ":/usr/lib/vst"
    DEFAULT_VST_PATH    += ":/usr/local/lib/vst"

    DEFAULT_VST3_PATH    = HOME + "/.vst3"
    DEFAULT_VST3_PATH   += ":/usr/lib/vst3"
    DEFAULT_VST3_PATH   += ":/usr/local/lib/vst3"

    DEFAULT_GIG_PATH     = HOME + "/.sounds/gig"
    DEFAULT_GIG_PATH    += ":/usr/share/sounds/gig"

    DEFAULT_SF2_PATH     = HOME + "/.sounds/sf2"
    DEFAULT_SF2_PATH    += ":/usr/share/sounds/sf2"

    DEFAULT_SFZ_PATH     = HOME + "/.sounds/sfz"
    DEFAULT_SFZ_PATH    += ":/usr/share/sounds/sfz"

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
        readEnvVars = False
    except:
        pass

    CloseKey(reg)
    del reg

if readEnvVars:
    gCarla.DEFAULT_LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
    gCarla.DEFAULT_DSSI_PATH   = os.getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter)
    gCarla.DEFAULT_LV2_PATH    = os.getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter)
    gCarla.DEFAULT_VST_PATH    = os.getenv("VST_PATH",    DEFAULT_VST_PATH).split(splitter)
    gCarla.DEFAULT_VST3_PATH   = os.getenv("VST3_PATH",   DEFAULT_VST3_PATH).split(splitter)
    gCarla.DEFAULT_AU_PATH     = os.getenv("AU_PATH",     DEFAULT_AU_PATH).split(splitter)
    gCarla.DEFAULT_CSOUND_PATH = os.getenv("CSOUND_PATH", DEFAULT_CSOUND_PATH).split(splitter)
    gCarla.DEFAULT_GIG_PATH    = os.getenv("GIG_PATH",    DEFAULT_GIG_PATH).split(splitter)
    gCarla.DEFAULT_SF2_PATH    = os.getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter)
    gCarla.DEFAULT_SFZ_PATH    = os.getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter)
else:
    gCarla.DEFAULT_LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter)
    gCarla.DEFAULT_DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter)
    gCarla.DEFAULT_LV2_PATH    = DEFAULT_LV2_PATH.split(splitter)
    gCarla.DEFAULT_VST_PATH    = DEFAULT_VST_PATH.split(splitter)
    gCarla.DEFAULT_VST3_PATH   = DEFAULT_VST3_PATH.split(splitter)
    gCarla.DEFAULT_AU_PATH     = DEFAULT_AU_PATH.split(splitter)
    gCarla.DEFAULT_CSOUND_PATH = DEFAULT_CSOUND_PATH.split(splitter)
    gCarla.DEFAULT_GIG_PATH    = DEFAULT_GIG_PATH.split(splitter)
    gCarla.DEFAULT_SF2_PATH    = DEFAULT_SF2_PATH.split(splitter)
    gCarla.DEFAULT_SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter)

# ------------------------------------------------------------------------------------------------------------
# Search for Carla tools

CWD = sys.path[0]

# make it work with cxfreeze
if WINDOWS and CWD.endswith(".exe"):
    CWD = CWD.rsplit("\\", 1)[0]
elif CWD.endswith("/carla") or CWD.endswith("/carla-plugin") or CWD.endswith("/carla-patchbay") or CWD.endswith("/carla-rack"):
    CWD = CWD.rsplit("/", 1)[0]

# find tool
def findTool(toolDir, toolName):
    path = os.path.join(CWD, toolName)
    if os.path.exists(path):
        return path

    path = os.path.join(CWD, toolDir, toolName)
    if os.path.exists(path):
        return path

    for p in PATH:
        path = os.path.join(p, toolName)
        if os.path.exists(path):
            return path

    return ""

# ------------------------------------------------------------------------------------------------------------
# Init host

def initHost(appName, libPrefix = None, failError = True):
    # -------------------------------------------------------------
    # Set Carla library name

    libname = "libcarla_"

    if gCarla.isControl:
        libname += "control2"
    else:
        libname += "standalone2"

    if WINDOWS:
        libname += ".dll"
    elif MACOS:
        libname += ".dylib"
    else:
        libname += ".so"

    # -------------------------------------------------------------
    # Search for the Carla library

    libfilename = ""

    if libPrefix is not None:
        libfilename = os.path.join(libPrefix, "lib", "carla", libname)
    else:
        path = os.path.join(CWD, "backend", libname)

        if os.path.exists(path):
            libfilename = path
        else:
            path = os.getenv("CARLA_LIB_PATH")

            if path and os.path.exists(path):
                CARLA_LIB_PATH = (path,)
            elif WINDOWS:
                CARLA_LIB_PATH = (os.path.join(PROGRAMFILES, "Carla"),)
            elif MACOS:
                CARLA_LIB_PATH = ("/opt/local/lib", "/usr/local/lib/", "/usr/lib")
            else:
                CARLA_LIB_PATH = ("/usr/local/lib/", "/usr/lib")

            for libpath in CARLA_LIB_PATH:
                path = os.path.join(libpath, "carla", libname)
                if os.path.exists(path):
                    libfilename = path
                    break

    # -------------------------------------------------------------
    # find windows tools

    gCarla.discovery_win32 = findTool("discovery", "carla-discovery-win32.exe")
    gCarla.discovery_win64 = findTool("discovery", "carla-discovery-win64.exe")

    # -------------------------------------------------------------
    # find native and posix tools

    if not WINDOWS:
        gCarla.discovery_native  = findTool("discovery", "carla-discovery-native")
        gCarla.discovery_posix32 = findTool("discovery", "carla-discovery-posix32")
        gCarla.discovery_posix64 = findTool("discovery", "carla-discovery-posix64")

    # -------------------------------------------------------------

    if not (libfilename or gCarla.isPlugin):
        if failError:
            QMessageBox.critical(None, "Error", "Failed to find the carla library, cannot continue")
            sys.exit(1)
        return

    # -------------------------------------------------------------
    # Init host

    if gCarla.host is None:
        gCarla.host = Host(libfilename)

    # -------------------------------------------------------------
    # Set binary path

    libfolder      = libfilename.replace(libname, "")
    localBinaries  = os.path.join(libfolder, "..", "bridges")
    systemBinaries = os.path.join(libfolder, "bridges")

    if os.path.exists(libfolder):
        gCarla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, libfolder)
    elif os.path.exists(localBinaries):
        gCarla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, localBinaries)
    elif os.path.exists(systemBinaries):
        gCarla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, systemBinaries)

    # -------------------------------------------------------------
    # Set resource path

    localResources  = os.path.join(libfolder, "..", "modules", "native-plugins", "resources")
    systemResources = os.path.join(libfolder, "resources")

    if os.path.exists(localResources):
        gCarla.host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, localResources)
    elif os.path.exists(systemResources):
        gCarla.host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, systemResources)

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

def getIcon(icon, size=16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# ------------------------------------------------------------------------------------------------------------
# Signal handler

def signalHandler(sig, frame):
    if gCarla.gui is None:
        return

    if sig in (SIGINT, SIGTERM):
        gCarla.gui.SIGTERM.emit()
    elif haveSIGUSR1 and sig == SIGUSR1:
        gCarla.gui.SIGUSR1.emit()

def setUpSignals():
    signal(SIGINT,  signalHandler)
    signal(SIGTERM, signalHandler)

    if not haveSIGUSR1:
        return

    signal(SIGUSR1, signalHandler)

# ------------------------------------------------------------------------------------------------------------
# QLineEdit and QPushButton combo

def getAndSetPath(parent, currentPath, lineEdit):
    newPath = QFileDialog.getExistingDirectory(parent, parent.tr("Set Path"), currentPath, QFileDialog.ShowDirsOnly)
    if newPath:
        lineEdit.setText(newPath)
    return newPath

# ------------------------------------------------------------------------------------------------------------
# Get plugin type as string

def getPluginTypeAsString(ptype):
    if ptype == PLUGIN_INTERNAL:
        return "Internal"
    if ptype == PLUGIN_LADSPA:
        return "LADSPA"
    if ptype == PLUGIN_DSSI:
        return "DSSI"
    if ptype == PLUGIN_LV2:
        return "LV2"
    if ptype == PLUGIN_VST:
        return "VST"
    if ptype == PLUGIN_VST3:
        return "VST3"
    if ptype == PLUGIN_AU:
        return "AU"
    if ptype == PLUGIN_JACK:
        return "JACK"
    if ptype == PLUGIN_REWIRE:
        return "ReWire"
    if ptype == PLUGIN_FILE_CSD:
        return "CSD"
    if ptype == PLUGIN_FILE_GIG:
        return "GIG"
    if ptype == PLUGIN_FILE_SF2:
        return "SF2"
    if ptype == PLUGIN_FILE_SFZ:
        return "SFZ"
    return "Unknown"

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
