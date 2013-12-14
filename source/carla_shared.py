#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
# Imports (Global)

import os
import sys

try:
    from PyQt5.QtCore import qFatal, qWarning
    from PyQt5.QtGui import QIcon
    from PyQt5.QtWidgets import QFileDialog, QMessageBox
except:
    from PyQt4.QtCore import qFatal, qWarning
    from PyQt4.QtGui import QIcon
    from PyQt4.QtGui import QFileDialog, QMessageBox

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
    from PyQt5.QtGui import qt_mac_set_menubar_icons
    qt_mac_set_menubar_icons(False)
elif WINDOWS:
    WINDIR = os.getenv("WINDIR")

# ------------------------------------------------------------------------------------------------------------
# Set Version

VERSION = "1.9.0"

# ------------------------------------------------------------------------------------------------------------
# Set TMP

TMP = os.getenv("TMP")

if TMP is None:
    if WINDOWS:
        qWarning("TMP variable not set")
        TMP = os.path.join(WINDIR, "temp")
    else:
        TMP = "/tmp"

elif not os.path.exists(TMP):
    qWarning("TMP does not exist")
    TMP = "/tmp"

# ------------------------------------------------------------------------------------------------------------
# Set HOME

HOME = os.getenv("HOME")

if HOME is None:
    HOME = os.path.expanduser("~")

    if LINUX or MACOS:
        qWarning("HOME variable not set")

if not os.path.exists(HOME):
    qWarning("HOME does not exist")
    HOME = TMP

# ------------------------------------------------------------------------------------------------------------
# Set PATH

PATH = os.getenv("PATH")

if PATH is None:
    qWarning("PATH variable not set")

    if MACOS:
        PATH = ("/opt/local/bin", "/usr/local/bin", "/usr/bin", "/bin")
    elif WINDOWS:
        PATH = (os.path.join(WINDIR, "system32"), WINDIR)
    else:
        PATH = ("/usr/local/bin", "/usr/bin", "/bin")

else:
    PATH = PATH.split(os.pathsep)

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
# Default Plugin Folders (get)

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

    DEFAULT_LADSPA_PATH = ";".join((os.path.join(APPDATA, "LADSPA"),
                                    os.path.join(PROGRAMFILES, "LADSPA")))

    DEFAULT_DSSI_PATH   = ";".join((os.path.join(APPDATA, "DSSI"),
                                    os.path.join(PROGRAMFILES, "DSSI")))

    DEFAULT_LV2_PATH    = ";".join((os.path.join(APPDATA, "LV2"),
                                    os.path.join(COMMONPROGRAMFILES, "LV2")))

    DEFAULT_VST_PATH    = ";".join((os.path.join(PROGRAMFILES, "VstPlugins"),
                                    os.path.join(PROGRAMFILES, "Steinberg", "VstPlugins")))

    DEFAULT_AU_PATH     = ""

    # TODO
    DEFAULT_CSOUND_PATH = ""

    DEFAULT_GIG_PATH    = ";".join((os.path.join(APPDATA, "GIG"),))
    DEFAULT_SF2_PATH    = ";".join((os.path.join(APPDATA, "SF2"),))
    DEFAULT_SFZ_PATH    = ";".join((os.path.join(APPDATA, "SFZ"),))

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH += ";"+os.path.join(PROGRAMFILESx86, "LADSPA")
        DEFAULT_DSSI_PATH   += ";"+os.path.join(PROGRAMFILESx86, "DSSI")
        DEFAULT_VST_PATH    += ";"+os.path.join(PROGRAMFILESx86, "VstPlugins")
        DEFAULT_VST_PATH    += ";"+os.path.join(PROGRAMFILESx86, "Steinberg", "VstPlugins")

elif HAIKU:
    splitter = ":"

    DEFAULT_LADSPA_PATH = ":".join((os.path.join(HOME, ".ladspa"),
                                    os.path.join("/", "boot", "common", "add-ons", "ladspa")))

    DEFAULT_DSSI_PATH   = ":".join((os.path.join(HOME, ".dssi"),
                                    os.path.join("/", "boot", "common", "add-ons", "dssi")))

    DEFAULT_LV2_PATH    = ":".join((os.path.join(HOME, ".lv2"),
                                    os.path.join("/", "boot", "common", "add-ons", "lv2")))

    DEFAULT_VST_PATH    = ":".join((os.path.join(HOME, ".vst"),
                                    os.path.join("/", "boot", "common", "add-ons", "vst")))

    DEFAULT_AU_PATH     = ""

    # TODO
    DEFAULT_CSOUND_PATH = ""

    # TODO
    DEFAULT_GIG_PATH    = ""
    DEFAULT_SF2_PATH    = ""
    DEFAULT_SFZ_PATH    = ""

elif MACOS:
    splitter = ":"

    DEFAULT_LADSPA_PATH = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LADSPA"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "LADSPA")))

    DEFAULT_DSSI_PATH   = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "DSSI"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "DSSI")))

    DEFAULT_LV2_PATH    = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LV2"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "LV2")))

    DEFAULT_VST_PATH    = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "VST"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "VST")))

    DEFAULT_AU_PATH     = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "Components"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "Components")))

    # TODO
    DEFAULT_CSOUND_PATH = ""

    # TODO
    DEFAULT_GIG_PATH    = ""
    DEFAULT_SF2_PATH    = ""
    DEFAULT_SFZ_PATH    = ""

else:
    splitter = ":"

    DEFAULT_LADSPA_PATH = ":".join((os.path.join(HOME, ".ladspa"),
                                    os.path.join("/", "usr", "lib", "ladspa"),
                                    os.path.join("/", "usr", "local", "lib", "ladspa")))

    DEFAULT_DSSI_PATH   = ":".join((os.path.join(HOME, ".dssi"),
                                    os.path.join("/", "usr", "lib", "dssi"),
                                    os.path.join("/", "usr", "local", "lib", "dssi")))

    DEFAULT_LV2_PATH    = ":".join((os.path.join(HOME, ".lv2"),
                                    os.path.join("/", "usr", "lib", "lv2"),
                                    os.path.join("/", "usr", "local", "lib", "lv2")))

    DEFAULT_VST_PATH    = ":".join((os.path.join(HOME, ".vst"),
                                    os.path.join("/", "usr", "lib", "vst"),
                                    os.path.join("/", "usr", "local", "lib", "vst")))

    DEFAULT_AU_PATH     = ""

    # TODO
    DEFAULT_CSOUND_PATH = ""

    DEFAULT_GIG_PATH    = ":".join((os.path.join(HOME, ".sounds", "gig"),
                                    os.path.join("/", "usr", "share", "sounds", "gig")))

    DEFAULT_SF2_PATH    = ":".join((os.path.join(HOME, ".sounds", "sf2"),
                                    os.path.join("/", "usr", "share", "sounds", "sf2")))

    DEFAULT_SFZ_PATH    = ":".join((os.path.join(HOME, ".sounds", "sfz"),
                                    os.path.join("/", "usr", "share", "sounds", "sfz")))

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

CARLA_KEY_ENGINE_DRIVER_PREFIX         = "Engine/Driver-"
CARLA_KEY_ENGINE_AUDIO_DRIVER          = "Engine/AudioDriver"         # str
CARLA_KEY_ENGINE_PROCESS_MODE          = "Engine/ProcessMode"         # enum
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
CARLA_KEY_PATHS_AU     = "Paths/AU"
CARLA_KEY_PATHS_CSOUND = "Paths/CSOUND"
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
        'DEFAULT_AU_PATH',
        'DEFAULT_CSOUND_PATH',
        'DEFAULT_GIG_PATH',
        'DEFAULT_SF2_PATH',
        'DEFAULT_SFZ_PATH'
    ]

Carla = CarlaObject()
Carla.host = None
Carla.gui  = None
Carla.isControl = False
Carla.isLocal   = True
Carla.isPlugin  = False
Carla.processMode   = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS if LINUX else ENGINE_PROCESS_MODE_CONTINUOUS_RACK
Carla.transportMode = ENGINE_TRANSPORT_MODE_JACK if LINUX else ENGINE_TRANSPORT_MODE_INTERNAL
Carla.maxParameters = MAX_DEFAULT_PARAMETERS

Carla.discovery_native  = ""
Carla.discovery_posix32 = ""
Carla.discovery_posix64 = ""
Carla.discovery_win32   = ""
Carla.discovery_win64   = ""

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
    Carla.DEFAULT_LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
    Carla.DEFAULT_DSSI_PATH   = os.getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter)
    Carla.DEFAULT_LV2_PATH    = os.getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter)
    Carla.DEFAULT_VST_PATH    = os.getenv("VST_PATH",    DEFAULT_VST_PATH).split(splitter)
    Carla.DEFAULT_AU_PATH     = os.getenv("AU_PATH",     DEFAULT_AU_PATH).split(splitter)
    Carla.DEFAULT_CSOUND_PATH = os.getenv("CSOUND_PATH", DEFAULT_CSOUND_PATH).split(splitter)
    Carla.DEFAULT_GIG_PATH    = os.getenv("GIG_PATH",    DEFAULT_GIG_PATH).split(splitter)
    Carla.DEFAULT_SF2_PATH    = os.getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter)
    Carla.DEFAULT_SFZ_PATH    = os.getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter)

else:
    Carla.DEFAULT_LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter)
    Carla.DEFAULT_DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter)
    Carla.DEFAULT_LV2_PATH    = DEFAULT_LV2_PATH.split(splitter)
    Carla.DEFAULT_VST_PATH    = DEFAULT_VST_PATH.split(splitter)
    Carla.DEFAULT_AU_PATH     = DEFAULT_AU_PATH.split(splitter)
    Carla.DEFAULT_CSOUND_PATH = DEFAULT_CSOUND_PATH.split(splitter)
    Carla.DEFAULT_GIG_PATH    = DEFAULT_GIG_PATH.split(splitter)
    Carla.DEFAULT_SF2_PATH    = DEFAULT_SF2_PATH.split(splitter)
    Carla.DEFAULT_SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter)

# ------------------------------------------------------------------------------------------------------------
# Search for Carla tools

CWD = sys.path[0]

# make it work with cxfreeze
if CWD.endswith("/carla"):
    CWD = CWD.rsplit("/carla", 1)[0]
elif CWD.endswith("\\carla.exe"):
    CWD = CWD.rsplit("\\carla.exe", 1)[0]

# find tool
def findTool(toolDir, toolName):
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

    if Carla.isControl:
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

    Carla.discovery_win32 = findTool("discovery", "carla-discovery-win32.exe")
    Carla.discovery_win64 = findTool("discovery", "carla-discovery-win64.exe")

    # -------------------------------------------------------------
    # find native and posix tools

    if not WINDOWS:
        Carla.discovery_native  = findTool("discovery", "carla-discovery-native")
        Carla.discovery_posix32 = findTool("discovery", "carla-discovery-posix32")
        Carla.discovery_posix64 = findTool("discovery", "carla-discovery-posix64")

    if not libfilename:
        if failError:
            QMessageBox.critical(None, "Error", "Failed to find the carla library, cannot continue")
            sys.exit(1)
        return

    # -------------------------------------------------------------
    # Init host

    Carla.host = Host(libfilename)

    # -------------------------------------------------------------
    # Set binary path

    libfolder      = libfilename.replace(libname, "")
    localBinaries  = os.path.join(libfolder, "..", "bridges")
    systemBinaries = os.path.join(libfolder, "bridges")

    if os.path.exists(libfolder):
        Carla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, libfolder)
    elif os.path.exists(localBinaries):
        Carla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, localBinaries)
    elif os.path.exists(systemBinaries):
        Carla.host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, systemBinaries)

    # -------------------------------------------------------------
    # Set resource path

    localResources  = os.path.join(libfolder, "..", "modules", "carla_native", "resources")
    systemResources = os.path.join(libfolder, "resources")

    if os.path.exists(localResources):
        Carla.host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, localResources)
    elif os.path.exists(systemResources):
        Carla.host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, systemResources)

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
    if Carla.gui is None:
        return

    if sig in (SIGINT, SIGTERM):
        Carla.gui.SIGTERM.emit()
    elif haveSIGUSR1 and sig == SIGUSR1:
        Carla.gui.SIGUSR1.emit()

def setUpSignals():
    signal(SIGINT,  signalHandler)
    signal(SIGTERM, signalHandler)

    if not haveSIGUSR1:
        return

    signal(SIGUSR1, signalHandler)

# ------------------------------------------------------------------------------------------------------------
# QLineEdit and QPushButton combo

def getAndSetPath(self_, currentPath, lineEdit):
    newPath = QFileDialog.getExistingDirectory(self_, self_.tr("Set Path"), currentPath, QFileDialog.ShowDirsOnly)
    if newPath:
        lineEdit.setText(newPath)
    return newPath

# ------------------------------------------------------------------------------------------------------------
# Custom MessageBox

def CustomMessageBox(self_, icon, title, text, extraText="", buttons=QMessageBox.Yes|QMessageBox.No, defButton=QMessageBox.No):
    msgBox = QMessageBox(self_)
    msgBox.setIcon(icon)
    msgBox.setWindowTitle(title)
    msgBox.setText(text)
    msgBox.setInformativeText(extraText)
    msgBox.setStandardButtons(buttons)
    msgBox.setDefaultButton(defButton)
    return msgBox.exec_()
