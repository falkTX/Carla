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
from copy import deepcopy
from subprocess import Popen, PIPE

try:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, qCritical, qFatal, qWarning
    from PyQt5.QtGui import QIcon
    from PyQt5.QtWidgets import QFileDialog, QMessageBox
except:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, qCritical, qFatal, qWarning
    from PyQt4.QtGui import QIcon
    from PyQt4.QtGui import QFileDialog, QMessageBox

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import *

# ------------------------------------------------------------------------------------------------------------
# Try Import LADSPA-RDF

try:
    import ladspa_rdf
    haveLRDF = True
except:
    qWarning("LRDF Support not available (LADSPA-RDF will be disabled)")
    haveLRDF = False

# ------------------------------------------------------------------------------------------------------------
# Import Signal

from signal import signal, SIGINT, SIGTERM

try:
    from signal import SIGUSR1
    haveSIGUSR1 = True
except:
    haveSIGUSR1 = False

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
# Global Carla object

class CarlaObject(object):
    __slots__ = [
        'host',
        'gui',
        'isControl',
        'isLocal',
        'processMode',
        'maxParameters',
        'LADSPA_PATH',
        'DSSI_PATH',
        'LV2_PATH',
        'VST_PATH',
        'AU_PATH',
        'CSOUND_PATH',
        'GIG_PATH',
        'SF2_PATH',
        'SFZ_PATH'
    ]

Carla = CarlaObject()
Carla.host = None
Carla.gui  = None
Carla.isControl = False
Carla.isLocal   = False
Carla.processMode   = PROCESS_MODE_MULTIPLE_CLIENTS if LINUX else PROCESS_MODE_CONTINUOUS_RACK
Carla.maxParameters = MAX_DEFAULT_PARAMETERS

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
# Default Plugin Folders

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

    DEFAULT_AU_PATH     = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "AudioUnits"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "AudioUnits")))

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

    DEFAULT_GIG_PATH    = ":".join((os.path.join(HOME, ".sounds"),
                                    os.path.join("/", "usr", "share", "sounds", "gig")))

    DEFAULT_SF2_PATH    = ":".join((os.path.join(HOME, ".sounds"),
                                    os.path.join("/", "usr", "share", "sounds", "sf2")))

    DEFAULT_SFZ_PATH    = ":".join((os.path.join(HOME, ".sounds"),
                                    os.path.join("/", "usr", "share", "sounds", "sfz")))

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
    Carla.LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
    Carla.DSSI_PATH   = os.getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter)
    Carla.LV2_PATH    = os.getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter)
    Carla.VST_PATH    = os.getenv("VST_PATH",    DEFAULT_VST_PATH).split(splitter)
    Carla.AU_PATH     = os.getenv("AU_PATH",     DEFAULT_AU_PATH).split(splitter)
    Carla.CSOUND_PATH = os.getenv("CSOUND_PATH", DEFAULT_CSOUND_PATH).split(splitter)
    Carla.GIG_PATH    = os.getenv("GIG_PATH",    DEFAULT_GIG_PATH).split(splitter)
    Carla.SF2_PATH    = os.getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter)
    Carla.SFZ_PATH    = os.getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter)

    if haveLRDF:
        LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
        if LADSPA_RDF_PATH_env:
            ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))
        del LADSPA_RDF_PATH_env

else:
    Carla.LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter)
    Carla.DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter)
    Carla.LV2_PATH    = DEFAULT_LV2_PATH.split(splitter)
    Carla.VST_PATH    = DEFAULT_VST_PATH.split(splitter)
    Carla.AU_PATH     = DEFAULT_AU_PATH.split(splitter)
    Carla.CSOUND_PATH = DEFAULT_CSOUND_PATH.split(splitter)
    Carla.GIG_PATH    = DEFAULT_GIG_PATH.split(splitter)
    Carla.SF2_PATH    = DEFAULT_SF2_PATH.split(splitter)
    Carla.SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter)

del readEnvVars

# ------------------------------------------------------------------------------------------------------------
# Search for Carla library and tools

carla_library_filename  = ""

carla_discovery_native  = ""
carla_discovery_posix32 = ""
carla_discovery_posix64 = ""
carla_discovery_win32   = ""
carla_discovery_win64   = ""

carla_bridge_native  = ""
carla_bridge_posix32 = ""
carla_bridge_posix64 = ""
carla_bridge_win32   = ""
carla_bridge_win64   = ""

carla_bridge_lv2_external = ""
carla_bridge_lv2_gtk2     = ""
carla_bridge_lv2_gtk3     = ""
carla_bridge_lv2_qt4      = ""
carla_bridge_lv2_qt5      = ""
carla_bridge_lv2_cocoa    = ""
carla_bridge_lv2_windows  = ""
carla_bridge_lv2_x11      = ""

carla_bridge_vst_mac  = ""
carla_bridge_vst_hwnd = ""
carla_bridge_vst_x11  = ""

carla_libname = "libcarla_%s" % "control" if Carla.isControl else "standalone"

if WINDOWS:
    carla_libname += ".dll"
elif MACOS:
    carla_libname += ".dylib"
else:
    carla_libname += ".so"

CWD = sys.path[0]

# make it work with cxfreeze
if CWD.endswith("/carla"):
    CWD = CWD.rsplit("/carla", 1)[0]
elif CWD.endswith("\\carla.exe"):
    CWD = CWD.rsplit("\\carla.exe", 1)[0]

# find carla_library_filename
if os.path.exists(os.path.join(CWD, "backend", carla_libname)):
    carla_library_filename = os.path.join(CWD, "backend", carla_libname)
else:
    if WINDOWS:
        CARLA_PATH = (os.path.join(PROGRAMFILES, "Carla"),)
    elif MACOS:
        CARLA_PATH = ("/opt/local/lib", "/usr/local/lib/", "/usr/lib")
    else:
        CARLA_PATH = ("/usr/local/lib/", "/usr/lib")

    for path in CARLA_PATH:
        if os.path.exists(os.path.join(path, "carla", carla_libname)):
            carla_library_filename = os.path.join(path, "carla", carla_libname)
            break

    del CARLA_PATH

# find tool
def findTool(toolDir, toolName):
    if os.path.exists(os.path.join(CWD, toolDir, toolName)):
        return os.path.join(CWD, toolDir, toolName)

    for p in PATH:
        if os.path.exists(os.path.join(p, toolName)):
            return os.path.join(p, toolName)

    return ""

# find windows tools
carla_discovery_win32 = findTool("discovery", "carla-discovery-win32.exe")
carla_discovery_win64 = findTool("discovery", "carla-discovery-win64.exe")
carla_bridge_win32    = findTool("bridges", "carla-bridge-win32.exe")
carla_bridge_win64    = findTool("bridges", "carla-bridge-win64.exe")

# find native and posix tools
if not WINDOWS:
    carla_discovery_native  = findTool("discovery", "carla-discovery-native")
    carla_discovery_posix32 = findTool("discovery", "carla-discovery-posix32")
    carla_discovery_posix64 = findTool("discovery", "carla-discovery-posix64")
    carla_bridge_native     = findTool("bridges", "carla-bridge-native")
    carla_bridge_posix32    = findTool("bridges", "carla-bridge-posix32")
    carla_bridge_posix64    = findTool("bridges", "carla-bridge-posix64")

# find generic tools
carla_bridge_lv2_external = findTool("bridges", "carla-bridge-lv2-external")

# find windows only tools
if WINDOWS:
    carla_bridge_lv2_windows = findTool("bridges", "carla-bridge-lv2-windows.exe")
    carla_bridge_vst_hwnd    = findTool("bridges", "carla-bridge-vst-hwnd.exe")

# find mac os only tools
elif MACOS:
    carla_bridge_lv2_cocoa = findTool("bridges", "carla-bridge-lv2-cocoa")
    carla_bridge_vst_mac   = findTool("bridges", "carla-bridge-vst-mac")

# find other tools
else:
    carla_bridge_lv2_gtk2 = findTool("bridges", "carla-bridge-lv2-gtk2")
    carla_bridge_lv2_gtk3 = findTool("bridges", "carla-bridge-lv2-gtk3")
    carla_bridge_lv2_qt4  = findTool("bridges", "carla-bridge-lv2-qt4")
    carla_bridge_lv2_qt5  = findTool("bridges", "carla-bridge-lv2-qt5")
    carla_bridge_lv2_x11  = findTool("bridges", "carla-bridge-lv2-x11")
    carla_bridge_vst_x11  = findTool("bridges", "carla-bridge-vst-x11")

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes c_char_p into a python string

def cString(value):
    if not value:
        return ""
    if isinstance(value, str):
        return value
    return value.decode("utf-8", errors="ignore")

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

    if haveSIGUSR1:
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
