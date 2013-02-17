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
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
import json
import platform
import sys
from codecs import open as codecopen
from copy import deepcopy
#from decimal import Decimal
from subprocess import Popen, PIPE
from PyQt4.QtCore import pyqtSlot, qWarning, Qt, QByteArray, QSettings, QThread, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QColor, QCursor, QDialog, QIcon, QFileDialog, QFontMetrics, QFrame, QMenu
from PyQt4.QtGui import QMessageBox, QPainter, QPainterPath, QTableWidgetItem, QVBoxLayout, QWidget
#from PyQt4.QtGui import QGraphicsScene, QInputDialog, QLinearGradient,
#from PyQt4.QtXml import QDomDocument

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_about
import ui_carla_database
import ui_carla_edit
import ui_carla_parameter
import ui_carla_plugin
import ui_carla_refresh

# ------------------------------------------------------------------------------------------------------------
# Try Import LADSPA-RDF

try:
    import ladspa_rdf
    haveLRDF = True
except:
    print("LRDF Support not available (LADSPA-RDF will be disabled)")
    haveLRDF = False

# ------------------------------------------------------------------------------------------------------------
# Try Import Signal

try:
    from signal import signal, SIGINT, SIGTERM, SIGUSR1
    haveSignal = True
except:
    haveSignal = False

# ------------------------------------------------------------------------------------------------------------
# Set Platform

if sys.platform == "darwin":
    from PyQt4.QtGui import qt_mac_set_menubar_icons
    qt_mac_set_menubar_icons(False)
    HAIKU   = False
    LINUX   = False
    MACOS   = True
    WINDOWS = False
elif "haiku" in sys.platform:
    HAIKU   = True
    LINUX   = False
    MACOS   = False
    WINDOWS = False
elif "linux" in sys.platform:
    HAIKU   = False
    LINUX   = True
    MACOS   = False
    WINDOWS = False
elif sys.platform in ("win32", "win64", "cygwin"):
    WINDIR  = os.getenv("WINDIR")
    HAIKU   = False
    LINUX   = False
    MACOS   = False
    WINDOWS = True
else:
    HAIKU   = False
    LINUX   = False
    MACOS   = False
    WINDOWS = False

# ------------------------------------------------------------------------------------------------------------
# Set Version

VERSION = "0.5.0"

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
# 64bit check

kIs64bit = bool(platform.architecture()[0] == "64bit" and sys.maxsize > 2**32)

# ------------------------------------------------------------------------------------------------
# Backend defines

MAX_DEFAULT_PLUGINS    = 99
MAX_RACK_PLUGINS       = 16
MAX_PATCHBAY_PLUGINS   = 999
MAX_DEFAULT_PARAMETERS = 200

# Plugin Hints
PLUGIN_IS_BRIDGE          = 0x0001
PLUGIN_IS_RTSAFE          = 0x0002
PLUGIN_IS_SYNTH           = 0x0004
PLUGIN_HAS_GUI            = 0x0010
PLUGIN_USES_CHUNKS        = 0x0020
PLUGIN_USES_SINGLE_THREAD = 0x0040
PLUGIN_CAN_DRYWET         = 0x0100
PLUGIN_CAN_VOLUME         = 0x0200
PLUGIN_CAN_BALANCE        = 0x0400
PLUGIN_CAN_PANNING        = 0x0800
PLUGIN_CAN_FORCE_STEREO   = 0x1000

# Plugin Options
PLUGIN_OPTION_FIXED_BUFFER         = 0x001
PLUGIN_OPTION_FORCE_STEREO         = 0x002
PLUGIN_OPTION_SELF_AUTOMATION      = 0x004
PLUGIN_OPTION_USE_CHUNKS           = 0x008
PLUGIN_OPTION_SEND_ALL_SOUND_OFF   = 0x010
PLUGIN_OPTION_SEND_NOTE_OFF_VELO   = 0x020
PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH = 0x040
PLUGIN_OPTION_SEND_PITCHBEND       = 0x080
PLUGIN_OPTION_VST_SUPPLY_IDLE      = 0x100
PLUGIN_OPTION_VST_UPDATE_DISPLAY   = 0x200

# Parameter Hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# Custom Data types
CUSTOM_DATA_INVALID = None
CUSTOM_DATA_CHUNK   = "http://kxstudio.sf.net/ns/carla/chunk"
CUSTOM_DATA_STRING  = "http://kxstudio.sf.net/ns/carla/string"

# Binary Type
BINARY_NONE    = 0
BINARY_POSIX32 = 1
BINARY_POSIX64 = 2
BINARY_WIN32   = 3
BINARY_WIN64   = 4
BINARY_OTHER   = 5

# Plugin Type
PLUGIN_NONE     = 0
PLUGIN_INTERNAL = 1
PLUGIN_LADSPA   = 2
PLUGIN_DSSI     = 3
PLUGIN_LV2      = 4
PLUGIN_VST      = 5
PLUGIN_GIG      = 6
PLUGIN_SF2      = 7
PLUGIN_SFZ      = 8

# Plugin Category
PLUGIN_CATEGORY_NONE      = 0
PLUGIN_CATEGORY_SYNTH     = 1
PLUGIN_CATEGORY_DELAY     = 2 # also Reverb
PLUGIN_CATEGORY_EQ        = 3
PLUGIN_CATEGORY_FILTER    = 4
PLUGIN_CATEGORY_DYNAMICS  = 5 # Amplifier, Compressor, Gate
PLUGIN_CATEGORY_MODULATOR = 6 # Chorus, Flanger, Phaser
PLUGIN_CATEGORY_UTILITY   = 7 # Analyzer, Converter, Mixer
PLUGIN_CATEGORY_OTHER     = 8 # used to check if a plugin has a category

# Parameter Type
PARAMETER_UNKNOWN       = 0
PARAMETER_INPUT         = 1
PARAMETER_OUTPUT        = 2
PARAMETER_LATENCY       = 3
PARAMETER_SAMPLE_RATE   = 4
PARAMETER_LV2_FREEWHEEL = 5
PARAMETER_LV2_TIME      = 6

# Internal Parameters Index
PARAMETER_NULL          = -1
PARAMETER_ACTIVE        = -2
PARAMETER_DRYWET        = -3
PARAMETER_VOLUME        = -4
PARAMETER_BALANCE_LEFT  = -5
PARAMETER_BALANCE_RIGHT = -6
PARAMETER_PANNING       = -7
PARAMETER_MAX           = -8

# Options Type
OPTION_PROCESS_NAME            = 0
OPTION_PROCESS_MODE            = 1
OPTION_FORCE_STEREO            = 2
OPTION_PREFER_PLUGIN_BRIDGES   = 3
OPTION_PREFER_UI_BRIDGES       = 4
OPTION_USE_DSSI_VST_CHUNKS     = 5
OPTION_MAX_PARAMETERS          = 6
OPTION_OSC_UI_TIMEOUT          = 7
OPTION_PREFERRED_BUFFER_SIZE   = 8
OPTION_PREFERRED_SAMPLE_RATE   = 9
OPTION_PATH_BRIDGE_NATIVE      = 10
OPTION_PATH_BRIDGE_POSIX32     = 11
OPTION_PATH_BRIDGE_POSIX64     = 12
OPTION_PATH_BRIDGE_WIN32       = 13
OPTION_PATH_BRIDGE_WIN64       = 14
OPTION_PATH_BRIDGE_LV2_GTK2    = 15
OPTION_PATH_BRIDGE_LV2_GTK3    = 16
OPTION_PATH_BRIDGE_LV2_QT4     = 17
OPTION_PATH_BRIDGE_LV2_QT5     = 18
OPTION_PATH_BRIDGE_LV2_COCOA   = 19
OPTION_PATH_BRIDGE_LV2_WINDOWS = 20
OPTION_PATH_BRIDGE_LV2_X11     = 21
OPTION_PATH_BRIDGE_VST_COCOA   = 22
OPTION_PATH_BRIDGE_VST_HWND    = 23
OPTION_PATH_BRIDGE_VST_X11     = 24

# Callback Type
CALLBACK_DEBUG          = 0
CALLBACK_PLUGIN_ADDED   = 1
CALLBACK_PLUGIN_REMOVED = 2
CALLBACK_PARAMETER_VALUE_CHANGED        = 3
CALLBACK_PARAMETER_DEFAULT_CHANGED      = 4
CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 5
CALLBACK_PARAMETER_MIDI_CC_CHANGED      = 6
CALLBACK_PROGRAM_CHANGED      = 7
CALLBACK_MIDI_PROGRAM_CHANGED = 8
CALLBACK_NOTE_ON              = 9
CALLBACK_NOTE_OFF             = 10
CALLBACK_SHOW_GUI             = 11
CALLBACK_UPDATE               = 12
CALLBACK_RELOAD_INFO          = 13
CALLBACK_RELOAD_PARAMETERS    = 14
CALLBACK_RELOAD_PROGRAMS      = 15
CALLBACK_RELOAD_ALL           = 16
CALLBACK_NSM_ANNOUNCE         = 17
CALLBACK_NSM_OPEN1            = 18
CALLBACK_NSM_OPEN2            = 19
CALLBACK_NSM_SAVE             = 20
CALLBACK_ERROR                = 21
CALLBACK_QUIT                 = 22

# Process Mode Type
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2
PROCESS_MODE_PATCHBAY         = 3

# Set BINARY_NATIVE
if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if kIs64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if kIs64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Carla Host object

class CarlaHostObject(object):
    __slots__ = [
        'host',
        'gui',
        'isControl',
        'isLocal',
        'processMode',
        'maxParameters'
    ]

Carla = CarlaHostObject()
Carla.host = None
Carla.gui  = None
Carla.isControl = False
Carla.isLocal   = True
Carla.processMode   = PROCESS_MODE_CONTINUOUS_RACK
Carla.maxParameters = MAX_RACK_PLUGINS

# ------------------------------------------------------------------------------------------------------------
# Carla GUI stuff

ICON_STATE_NULL = 0
ICON_STATE_WAIT = 1
ICON_STATE_OFF  = 2
ICON_STATE_ON   = 3

PALETTE_COLOR_NONE   = 0
PALETTE_COLOR_WHITE  = 1
PALETTE_COLOR_RED    = 2
PALETTE_COLOR_GREEN  = 3
PALETTE_COLOR_BLUE   = 4
PALETTE_COLOR_YELLOW = 5
PALETTE_COLOR_ORANGE = 6
PALETTE_COLOR_BROWN  = 7
PALETTE_COLOR_PINK   = 8

CarlaStateParameter = {
    'index': 0,
    'name': "",
    'symbol': "",
    'value': 0.0,
    'midiChannel': 1,
    'midiCC': -1
}

CarlaStateCustomData = {
    'type': "",
    'key': "",
    'value': ""
}

CarlaSaveState = {
    'type': "",
    'name': "",
    'label': "",
    'binary': "",
    'uniqueId': 0,
    'active': False,
    'dryWet': 1.0,
    'volume': 1.0,
    'balanceLeft': -1.0,
    'balanceRight': 1.0,
    'pannning': 0.0,
    'parameterList': [],
    'currentProgramIndex': -1,
    'currentProgramName': "",
    'currentMidiBank': -1,
    'currentMidiProgram': -1,
    'customDataList': [],
    'chunk': None
}

# ------------------------------------------------------------------------------------------------------------
# Static MIDI CC list

MIDI_CC_LIST = (
    #"0x00 Bank Select",
    "0x01 Modulation",
    "0x02 Breath",
    "0x03 (Undefined)",
    "0x04 Foot",
    "0x05 Portamento",
    #"0x06 (Data Entry MSB)",
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
    #"0x20 *Bank Select",
    #"0x21 *Modulation",
    #"0x22 *Breath",
    #"0x23 *(Undefined)",
    #"0x24 *Foot",
    #"0x25 *Portamento",
    #"0x26 *(Data Entry MSB)",
    #"0x27 *Volume",
    #"0x28 *Balance",
    #"0x29 *(Undefined)",
    #"0x2A *Pan",
    #"0x2B *Expression",
    #"0x2C *FX *Control 1",
    #"0x2D *FX *Control 2",
    #"0x2E *(Undefined)",
    #"0x2F *(Undefined)",
    #"0x30 *General Purpose 1",
    #"0x31 *General Purpose 2",
    #"0x32 *General Purpose 3",
    #"0x33 *General Purpose 4",
    #"0x34 *(Undefined)",
    #"0x35 *(Undefined)",
    #"0x36 *(Undefined)",
    #"0x37 *(Undefined)",
    #"0x38 *(Undefined)",
    #"0x39 *(Undefined)",
    #"0x3A *(Undefined)",
    #"0x3B *(Undefined)",
    #"0x3C *(Undefined)",
    #"0x3D *(Undefined)",
    #"0x3E *(Undefined)",
    #"0x3F *(Undefined)",
    #"0x40 Damper On/Off", # <63 off, >64 on
    #"0x41 Portamento On/Off", # <63 off, >64 on
    #"0x42 Sostenuto On/Off", # <63 off, >64 on
    #"0x43 Soft Pedal On/Off", # <63 off, >64 on
    #"0x44 Legato Footswitch", # <63 Normal, >64 Legato
    #"0x45 Hold 2", # <63 off, >64 on
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
        print("APPDATA variable not set, cannot continue")
        sys.exit(1)

    if not PROGRAMFILES:
        print("PROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    if not COMMONPROGRAMFILES:
        print("COMMONPROGRAMFILES variable not set, cannot continue")
        sys.exit(1)

    DEFAULT_LADSPA_PATH = ";".join((os.path.join(APPDATA, "LADSPA"),
                                    os.path.join(PROGRAMFILES, "LADSPA")))

    DEFAULT_DSSI_PATH = ";".join((os.path.join(APPDATA, "DSSI"),
                                  os.path.join(PROGRAMFILES, "DSSI")))

    DEFAULT_LV2_PATH = ";".join((os.path.join(APPDATA, "LV2"),
                                 os.path.join(COMMONPROGRAMFILES, "LV2")))

    DEFAULT_VST_PATH = ";".join((os.path.join(PROGRAMFILES, "VstPlugins"),
                                 os.path.join(PROGRAMFILES, "Steinberg", "VstPlugins")))

    DEFAULT_GIG_PATH = ";".join((os.path.join(APPDATA, "GIG"),))
    DEFAULT_SF2_PATH = ";".join((os.path.join(APPDATA, "SF2"),))
    DEFAULT_SFZ_PATH = ";".join((os.path.join(APPDATA, "SFZ"),))

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH += ";"+os.path.join(PROGRAMFILESx86, "LADSPA")
        DEFAULT_DSSI_PATH += ";"+os.path.join(PROGRAMFILESx86, "DSSI")
        DEFAULT_VST_PATH += ";"+os.path.join(PROGRAMFILESx86, "VstPlugins")
        DEFAULT_VST_PATH += ";"+os.path.join(PROGRAMFILESx86, "Steinberg", "VstPlugins")

elif HAIKU:
    splitter = ":"

    # TODO
    DEFAULT_LADSPA_PATH = ""
    DEFAULT_DSSI_PATH = ""
    DEFAULT_LV2_PATH = ""
    DEFAULT_VST_PATH = ""
    DEFAULT_GIG_PATH = ""
    DEFAULT_SF2_PATH = ""
    DEFAULT_SFZ_PATH = ""

elif MACOS:
    splitter = ":"

    DEFAULT_LADSPA_PATH = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LADSPA"),
                                    os.path.join("/", "Library", "Audio", "Plug-Ins", "LADSPA")))

    DEFAULT_DSSI_PATH = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "DSSI"),
                                  os.path.join("/", "Library", "Audio", "Plug-Ins", "DSSI")))

    DEFAULT_LV2_PATH = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "LV2"),
                                 os.path.join("/", "Library", "Audio", "Plug-Ins", "LV2")))

    DEFAULT_VST_PATH = ":".join((os.path.join(HOME, "Library", "Audio", "Plug-Ins", "VST"),
                                 os.path.join("/", "Library", "Audio", "Plug-Ins", "VST")))

    # TODO
    DEFAULT_GIG_PATH = ""
    DEFAULT_SF2_PATH = ""
    DEFAULT_SFZ_PATH = ""

else:
    splitter = ":"

    DEFAULT_LADSPA_PATH = ":".join((os.path.join(HOME, ".ladspa"),
                                    os.path.join("/", "usr", "lib", "ladspa"),
                                    os.path.join("/", "usr", "local", "lib", "ladspa")))

    DEFAULT_DSSI_PATH = ":".join((os.path.join(HOME, ".dssi"),
                                  os.path.join("/", "usr", "lib", "dssi"),
                                  os.path.join("/", "usr", "local", "lib", "dssi")))

    DEFAULT_LV2_PATH = ":".join((os.path.join(HOME, ".lv2"),
                                 os.path.join("/", "usr", "lib", "lv2"),
                                 os.path.join("/", "usr", "local", "lib", "lv2")))

    DEFAULT_VST_PATH = ":".join((os.path.join(HOME, ".vst"),
                                 os.path.join("/", "usr", "lib", "vst"),
                                 os.path.join("/", "usr", "local", "lib", "vst")))

    DEFAULT_GIG_PATH = ":".join((os.path.join(HOME, ".sounds"),
                                 os.path.join("/", "usr", "share", "sounds", "gig")))

    DEFAULT_SF2_PATH = ":".join((os.path.join(HOME, ".sounds"),
                                 os.path.join("/", "usr", "share", "sounds", "sf2"),
                                 "/home/falktx/Personal/Muzyks/Kits/SF2/"))

    DEFAULT_SFZ_PATH = ":".join((os.path.join(HOME, ".sounds"),
                                 os.path.join("/", "usr", "share", "sounds", "sfz")))

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (set)

global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH

LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
DSSI_PATH = os.getenv("DSSI_PATH", DEFAULT_DSSI_PATH).split(splitter)
LV2_PATH = os.getenv("LV2_PATH", DEFAULT_LV2_PATH).split(splitter)
VST_PATH = os.getenv("VST_PATH", DEFAULT_VST_PATH).split(splitter)
GIG_PATH = os.getenv("GIG_PATH", DEFAULT_GIG_PATH).split(splitter)
SF2_PATH = os.getenv("SF2_PATH", DEFAULT_SF2_PATH).split(splitter)
SFZ_PATH = os.getenv("SFZ_PATH", DEFAULT_SFZ_PATH).split(splitter)

if haveLRDF:
    LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
    if LADSPA_RDF_PATH_env:
        ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))
    del LADSPA_RDF_PATH_env

# ------------------------------------------------------------------------------------------------------------
# Search for Carla library and tools

global carla_library_path
carla_library_path = ""

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

carla_bridge_lv2_gtk2    = ""
carla_bridge_lv2_gtk3    = ""
carla_bridge_lv2_qt4     = ""
carla_bridge_lv2_qt5     = ""
carla_bridge_lv2_cocoa   = ""
carla_bridge_lv2_windows = ""
carla_bridge_lv2_x11     = ""

carla_bridge_vst_cocoa = ""
carla_bridge_vst_hwnd  = ""
carla_bridge_vst_x11   = ""

if WINDOWS:
    carla_libname = "libcarla_standalone.dll"
elif MACOS:
    carla_libname = "libcarla_standalone.dylib"
else:
    carla_libname = "libcarla_standalone.so"

CWD = sys.path[0]

# make it work with cxfreeze
if CWD.endswith("%scarla" % os.sep):
    CWD = CWD.rsplit("%scarla" % os.sep, 1)[0]

# find carla_library_path
if os.path.exists(os.path.join(CWD, "backend", carla_libname)):
    carla_library_path = os.path.join(CWD, "backend", carla_libname)
else:
    if WINDOWS:
        CARLA_PATH = (os.path.join(PROGRAMFILES, "Carla"),)
    elif MACOS:
        CARLA_PATH = ("/opt/local/lib", "/usr/local/lib/", "/usr/lib")
    else:
        CARLA_PATH = ("/usr/local/lib/", "/usr/lib")

    for path in CARLA_PATH:
        if os.path.exists(os.path.join(path, "carla", carla_libname)):
            carla_library_path = os.path.join(path, "carla", carla_libname)
            break

# find any tool
def findTool(tdir, tname):
    if os.path.exists(os.path.join(CWD, tdir, tname)):
        return os.path.join(CWD, tdir, tname)

    for p in PATH:
        if os.path.exists(os.path.join(p, tname)):
            return os.path.join(p, tname)

    return ""

# find wine/windows tools
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

# find windows only tools
if WINDOWS:
    carla_bridge_lv2_windows = findTool("bridges", "carla-bridge-lv2-windows.exe")
    carla_bridge_vst_hwnd    = findTool("bridges", "carla-bridge-vst-hwnd.exe")

# find mac os only tools
elif MACOS:
    carla_bridge_lv2_cocoa = findTool("bridges", "carla-bridge-lv2-cocoa")
    carla_bridge_vst_cocoa = findTool("bridges", "carla-bridge-vst-cocoa")

# find generic tools
else:
    carla_bridge_lv2_gtk2 = findTool("bridges", "carla-bridge-lv2-gtk2")
    carla_bridge_lv2_gtk3 = findTool("bridges", "carla-bridge-lv2-gtk3")
    carla_bridge_lv2_qt4  = findTool("bridges", "carla-bridge-lv2-qt4")
    carla_bridge_lv2_qt5  = findTool("bridges", "carla-bridge-lv2-qt5")

# find linux only tools
if LINUX:
    carla_bridge_lv2_x11 = os.path.join("bridges", "carla-bridge-lv2-x11")
    carla_bridge_vst_x11 = os.path.join("bridges", "carla-bridge-vst-x11")

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
# Unicode open

def uopen(filename, mode="r"):
    return codecopen(filename, encoding="utf-8", mode=mode)

# ------------------------------------------------------------------------------------------------------------
# Get Icon from user theme, using our own as backup (Oxygen)

def getIcon(icon, size=16):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.png" % (size, size, icon)))

# ------------------------------------------------------------------------------------------------------------
# Signal handler

def setUpSignals():
    if not haveSignal:
        return

    signal(SIGINT,  signalHandler)
    signal(SIGTERM, signalHandler)
    signal(SIGUSR1, signalHandler)

def signalHandler(sig, frame):
    if sig in (SIGINT, SIGTERM):
        Carla.gui.emit(SIGNAL("SIGTERM()"))
    elif sig == SIGUSR1:
        Carla.gui.emit(SIGNAL("SIGUSR1()"))

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

# ------------------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(bPATH, OS):
    binaries = []

    if OS == "WINDOWS":
        extensions = (".dll",)
    elif OS == "MACOS":
        extensions = (".dylib", ".so")
    else:
        extensions = (".so", ".sO", ".SO", ".So")

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            binaries.append(os.path.join(root, name))

    return binaries

def findLV2Bundles(bPATH):
    bundles = []

    for root, dirs, files in os.walk(bPATH):
        if os.path.exists(os.path.join(root, "manifest.ttl")):
            bundles.append(root)

    return bundles

def findSoundKits(bPATH, stype):
    soundfonts = []

    if stype == "gig":
        extensions = (".gig", ".giG", ".gIG", ".GIG", ".GIg", ".Gig") if not WINDOWS else (".gig",)
    elif stype == "sf2":
        extensions = (".sf2", ".sF2", ".SF2", ".Sf2") if not WINDOWS else (".sf2",)
    elif stype == "sfz":
        extensions = (".sfz", ".sfZ", ".sFZ", ".SFZ", ".SFz", ".Sfz") if not WINDOWS else (".sfz",)
    else:
        return []

    for root, dirs, files in os.walk(bPATH):
        for name in [name for name in files if name.endswith(extensions)]:
            soundfonts.append(os.path.join(root, name))

    return soundfonts

def findDSSIGUI(filename, name, label):
    pluginDir = filename.rsplit(".", 1)[0]
    shortName = os.path.basename(pluginDir)
    guiFilename = ""

    checkName  = name.replace(" ", "_")
    checkLabel = label
    checkSName = shortName

    if checkName[-1]  != "_": checkName  += "_"
    if checkLabel[-1] != "_": checkLabel += "_"
    if checkSName[-1] != "_": checkSName += "_"

    for root, dirs, files in os.walk(pluginDir):
        guiFiles = files
        break
    else:
        guiFiles = []

    for guiFile in guiFiles:
        if guiFile.startswith(checkName) or guiFile.startswith(checkLabel) or guiFile.startswith(checkSName):
            guiFilename = os.path.join(pluginDir, guiFile)
            break

    return guiFilename

# ------------------------------------------------------------------------------------------------------------
# Plugin Query

PLUGIN_QUERY_API_VERSION = 1

PyPluginInfo = {
    'API': PLUGIN_QUERY_API_VERSION,
    'build': 0, # BINARY_NONE
    'type': 0, # PLUGIN_NONE
    'hints': 0x0,
    'binary': "",
    'name': "",
    'label': "",
    'maker': "",
    'copyright': "",
    'uniqueId': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'audio.total': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'midi.total': 0,
    'parameters.ins': 0,
    'parameters.outs': 0,
    'parameters.total': 0,
    'programs.total': 0
}

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
    fakeLabel = os.path.basename(filename).rsplit(".", 1)[0]
    plugins = []
    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        if isWine:
            command.append("WINEDEBUG=-all")

    command.append(tool)
    command.append(stype)
    command.append(filename)

    Ps = Popen(command, stdout=PIPE)
    Ps.wait()
    output = Ps.stdout.read().decode("utf-8", errors="ignore").split("\n")

    pinfo = None

    for line in output:
        line = line.strip()
        if line == "carla-discovery::init::-----------":
            pinfo = deepcopy(PyPluginInfo)
            pinfo['type'] = itype
            pinfo['binary'] = filename

        elif line == "carla-discovery::end::------------":
            if pinfo != None:
                plugins.append(pinfo)
                pinfo = None

        elif line == "Segmentation fault":
            print("carla-discovery::crash::%s crashed during discovery" % filename)

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::error::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::"):
            if pinfo == None:
                continue

            prop, value = line.replace("carla-discovery::", "").split("::", 1)

            if prop == "name":
                pinfo['name'] = value if value else fakeLabel
            elif prop == "label":
                pinfo['label'] = value if value else fakeLabel
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "copyright":
                pinfo['copyright'] = value
            elif prop == "uniqueId":
                if value.isdigit(): pinfo['uniqueId'] = int(value)
            elif prop == "hints":
                if value.isdigit(): pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit(): pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit(): pinfo['audio.outs'] = int(value)
            elif prop == "audio.total":
                if value.isdigit(): pinfo['audio.total'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit(): pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit(): pinfo['midi.outs'] = int(value)
            elif prop == "midi.total":
                if value.isdigit(): pinfo['midi.total'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit(): pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit(): pinfo['parameters.outs'] = int(value)
            elif prop == "parameters.total":
                if value.isdigit(): pinfo['parameters.total'] = int(value)
            elif prop == "programs.total":
                if value.isdigit(): pinfo['programs.total'] = int(value)
            elif prop == "build":
                if value.isdigit(): pinfo['build'] = int(value)

    # Additional checks
    for pinfo in plugins:
        if itype == PLUGIN_DSSI:
            if findDSSIGUI(pinfo['binary'], pinfo['name'], pinfo['label']):
                pinfo['hints'] |= PLUGIN_HAS_GUI

    return plugins

def checkPluginInternal(desc):
    plugins = []

    pinfo = deepcopy(PyPluginInfo)
    pinfo['build'] = BINARY_NATIVE
    pinfo['type']  = PLUGIN_INTERNAL
    pinfo['hints'] = int(desc['hints'])
    pinfo['name']  = cString(desc['name'])
    pinfo['label'] = cString(desc['label'])
    pinfo['maker'] = cString(desc['maker'])
    pinfo['copyright'] = cString(desc['copyright'])

    pinfo['audio.ins']   = int(desc['audioIns'])
    pinfo['audio.outs']  = int(desc['audioOuts'])
    pinfo['audio.total'] = pinfo['audio.ins'] + pinfo['audio.outs']

    pinfo['midi.ins']   = int(desc['midiIns'])
    pinfo['midi.outs']  = int(desc['midiOuts'])
    pinfo['midi.total'] = pinfo['midi.ins'] + pinfo['midi.outs']

    pinfo['parameters.ins']   = int(desc['parameterIns'])
    pinfo['parameters.outs']  = int(desc['parameterOuts'])
    pinfo['parameters.total'] = pinfo['parameters.ins'] + pinfo['parameters.outs']

    plugins.append(pinfo)

    return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginLV2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, isWine)

def checkPluginVST(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST, "VST", filename, tool, isWine)

def checkPluginGIG(filename, tool):
    return runCarlaDiscovery(PLUGIN_GIG, "GIG", filename, tool)

def checkPluginSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkPluginSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

# ------------------------------------------------------------------------------------------------------------
# Carla XML helpers

def xmlSafeString(string, toXml):
    if toXml:
        return string.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;")
    else:
        return string.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"")

def getSaveStateDictFromXML(xmlNode):
    saveState = deepcopy(CarlaSaveState)

    node = xmlNode.firstChild()

    while not node.isNull():
        # ------------------------------------------------------
        # Info

        if node.toElement().tagName() == "Info":
            xmlInfo = node.toElement().firstChild()

            while not xmlInfo.isNull():
                tag  = xmlInfo.toElement().tagName()
                text = xmlInfo.toElement().text().strip()

                if tag == "Type":
                    saveState["type"] = text
                elif tag == "Name":
                    saveState["name"] = xmlSafeString(text, False)
                elif tag in ("Label", "URI"):
                    saveState["label"] = xmlSafeString(text, False)
                elif tag == "Binary":
                    saveState["binary"] = xmlSafeString(text, False)
                elif tag == "UniqueID":
                    if text.isdigit(): saveState["uniqueId"] = int(text)

                xmlInfo = xmlInfo.nextSibling()

        # ------------------------------------------------------
        # Data

        elif node.toElement().tagName() == "Data":
            xmlData = node.toElement().firstChild()

            while not xmlData.isNull():
                tag  = xmlData.toElement().tagName()
                text = xmlData.toElement().text().strip()

                # ----------------------------------------------
                # Internal Data

                if tag == "Active":
                    saveState['active'] = bool(text == "Yes")
                elif tag == "DryWet":
                    if isNumber(text): saveState["dryWet"] = float(text)
                elif tag == "Volume":
                    if isNumber(text): saveState["volume"] = float(text)
                elif tag == "Balance-Left":
                    if isNumber(text): saveState["balanceLeft"] = float(text)
                elif tag == "Balance-Right":
                    if isNumber(text): saveState["balanceRight"] = float(text)
                elif tag == "Panning":
                    if isNumber(text): saveState["pannning"] = float(text)

                # ----------------------------------------------
                # Program (current)

                elif tag == "CurrentProgramIndex":
                    if text.isdigit(): saveState["currentProgramIndex"] = int(text)
                elif tag == "CurrentProgramName":
                    saveState["currentProgramName"] = xmlSafeString(text, False)

                # ----------------------------------------------
                # Midi Program (current)

                elif tag == "CurrentMidiBank":
                    if text.isdigit(): saveState["currentMidiBank"] = int(text)
                elif tag == "CurrentMidiProgram":
                    if text.isdigit(): saveState["currentMidiProgram"] = int(text)

                # ----------------------------------------------
                # Parameters

                elif tag == "Parameter":
                    stateParameter = deepcopy(CarlaStateParameter)

                    xmlSubData = xmlData.toElement().firstChild()

                    while not xmlSubData.isNull():
                        pTag  = xmlSubData.toElement().tagName()
                        pText = xmlSubData.toElement().text().strip()

                        if pTag == "Index":
                            if pText.isdigit(): stateParameter["index"] = int(pText)
                        elif pTag == "Name":
                            stateParameter["name"] = xmlSafeString(pText, False)
                        elif pTag == "Symbol":
                            stateParameter["symbol"] = xmlSafeString(pText, False)
                        elif pTag == "Value":
                            if isNumber(pText): stateParameter["value"] = float(pText)
                        elif pTag == "MidiChannel":
                            if pText.isdigit(): stateParameter["midiChannel"] = int(pText)
                        elif pTag == "MidiCC":
                            if pText.isdigit(): stateParameter["midiCC"] = int(pText)

                        xmlSubData = xmlSubData.nextSibling()

                    saveState["parameterList"].append(stateParameter)

                # ----------------------------------------------
                # Custom Data

                elif tag == "CustomData":
                    stateCustomData = deepcopy(CarlaStateCustomData)

                    xmlSubData = xmlData.toElement().firstChild()

                    while not xmlSubData.isNull():
                        cTag  = xmlSubData.toElement().tagName()
                        cText = xmlSubData.toElement().text().strip()

                        if cTag == "Type":
                            stateCustomData["type"] = xmlSafeString(cText, False)
                        elif cTag == "Key":
                            stateCustomData["key"] = xmlSafeString(cText, False)
                        elif cTag == "Value":
                            stateCustomData["value"] = xmlSafeString(cText, False)

                        xmlSubData = xmlSubData.nextSibling()

                    saveState["customDataList"].append(stateCustomData)

                # ----------------------------------------------
                # Chunk

                elif tag == "Chunk":
                    saveState["chunk"] = xmlSafeString(text, False)

                # ----------------------------------------------

                xmlData = xmlData.nextSibling()

        # ------------------------------------------------------

        node = node.nextSibling()

    return saveState

# ------------------------------------------------------------------------------------------------------------
# Carla About dialog

class CarlaAboutW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_about.Ui_CarlaAboutW()
        self.ui.setupUi(self)

        if Carla.isControl:
            extraInfo = " - <b>%s</b>" % self.tr("OSC Bridge Version")
        else:
            extraInfo = ""

        self.ui.l_about.setText(self.tr(""
                                     "<br>Version %s"
                                     "<br>Carla is a Multi-Plugin Host for JACK%s.<br>"
                                     "<br>Copyright (C) 2011-2013 falkTX<br>"
                                     "" % (VERSION, extraInfo)))

        if Carla.isControl:
            self.ui.l_extended.hide()
            self.ui.tabWidget.removeTab(1)
            self.ui.tabWidget.removeTab(1)

        else:
            self.ui.l_extended.setText(cString(Carla.host.get_extended_license_text()))
            self.ui.le_osc_url.setText(cString(Carla.host.get_host_osc_url()) if Carla.host.is_engine_running() else self.tr("(Engine not running)"))

            self.ui.l_osc_cmds.setText(
                                    " /set_active                 <i-value>\n"
                                    " /set_drywet                 <f-value>\n"
                                    " /set_volume                 <f-value>\n"
                                    " /set_balance_left           <f-value>\n"
                                    " /set_balance_right          <f-value>\n"
                                    " /set_panning                <f-value>\n"
                                    " /set_parameter_value        <i-index> <f-value>\n"
                                    " /set_parameter_midi_cc      <i-index> <i-cc>\n"
                                    " /set_parameter_midi_channel <i-index> <i-channel>\n"
                                    " /set_program                <i-index>\n"
                                    " /set_midi_program           <i-index>\n"
                                    " /note_on                    <i-note> <i-velo>\n"
                                    " /note_off                   <i-note>\n"
                                  )

            self.ui.l_example.setText("/Carla/2/set_parameter_value 5 1.0")
            self.ui.l_example_help.setText("<i>(as in this example, \"2\" is the plugin number and \"5\" the parameter)</i>")

            self.ui.l_ladspa.setText(self.tr("Everything! (Including LRDF)"))
            self.ui.l_dssi.setText(self.tr("Everything! (Including CustomData/Chunks)"))
            self.ui.l_lv2.setText(self.tr("About 95&#37; complete (using custom extensions).<br/>"
                                      "Implemented Feature/Extensions:"
                                      "<ul>"
                                      "<li>http://lv2plug.in/ns/ext/atom</li>"
                                      "<li>http://lv2plug.in/ns/ext/buf-size</li>"
                                      "<li>http://lv2plug.in/ns/ext/data-access</li>"
                                      #"<li>http://lv2plug.in/ns/ext/dynmanifest</li>"
                                      "<li>http://lv2plug.in/ns/ext/event</li>"
                                      "<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                      "<li>http://lv2plug.in/ns/ext/log</li>"
                                      "<li>http://lv2plug.in/ns/ext/midi</li>"
                                      "<li>http://lv2plug.in/ns/ext/options</li>"
                                      #"<li>http://lv2plug.in/ns/ext/parameters</li>"
                                      "<li>http://lv2plug.in/ns/ext/patch</li>"
                                      #"<li>http://lv2plug.in/ns/ext/port-groups</li>"
                                      "<li>http://lv2plug.in/ns/ext/port-props</li>"
                                      #"<li>http://lv2plug.in/ns/ext/presets</li>"
                                      "<li>http://lv2plug.in/ns/ext/state</li>"
                                      "<li>http://lv2plug.in/ns/ext/time</li>"
                                      "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                      "<li>http://lv2plug.in/ns/ext/urid</li>"
                                      "<li>http://lv2plug.in/ns/ext/worker</li>"
                                      "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                      "<li>http://lv2plug.in/ns/extensions/units</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/external-ui</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/rtmempool</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                      "</ul>"))
            self.ui.l_vst.setText(self.tr("<p>About 85&#37; complete (missing vst bank/presets and some minor stuff)</p>"))

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Parameter

class PluginParameter(QWidget):
    def __init__(self, parent, pInfo, pluginId, tabIndex):
        QWidget.__init__(self, parent)
        self.ui = ui_carla_parameter.Ui_PluginParameter()
        self.ui.setupUi(self)

        pType  = pInfo['type']
        pHints = pInfo['hints']

        self.fMidiControl = -1
        self.fMidiChannel = 1
        self.fParameterId = pInfo['index']
        self.fPluginId    = pluginId
        self.fTabIndex    = tabIndex

        self.ui.label.setText(pInfo['name'])
        self.ui.widget.setName(pInfo['name'])

        if pType == PARAMETER_INPUT:
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setDefault(pInfo['default'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setStep(pInfo['step'])
            self.ui.widget.setStepSmall(pInfo['stepSmall'])
            self.ui.widget.setStepLarge(pInfo['stepLarge'])
            self.ui.widget.setScalePoints(pInfo['scalePoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

            if not pHints & PARAMETER_IS_ENABLED:
                self.ui.widget.setReadOnly(True)
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

            elif not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

        elif pType == PARAMETER_OUTPUT:
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setReadOnly(True)

            if not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

        else:
            self.ui.widget.setVisible(False)
            self.ui.sb_control.setVisible(False)
            self.ui.sb_channel.setVisible(False)

        if pHints & PARAMETER_USES_CUSTOM_TEXT:
            self.ui.widget.setTextCallback(self._textCallBack)

        self.setMidiControl(pInfo['midiCC'])
        self.setMidiChannel(pInfo['midiChannel'])

        self.connect(self.ui.sb_control, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_controlSpinboxCustomMenu()"))
        self.connect(self.ui.sb_control, SIGNAL("valueChanged(int)"), SLOT("slot_controlSpinboxChanged(int)"))
        self.connect(self.ui.sb_channel, SIGNAL("valueChanged(int)"), SLOT("slot_channelSpinboxChanged(int)"))
        self.connect(self.ui.widget, SIGNAL("valueChanged(double)"), SLOT("slot_widgetValueChanged(double)"))

        self.ui.widget.updateAll()

    def setDefault(self, value):
        self.ui.widget.setDefault(value)

    def setValue(self, value, send=True):
        self.ui.widget.setValue(value, send)

    def setMidiControl(self, control):
        self.fMidiControl = control
        self.ui.sb_control.blockSignals(True)
        self.ui.sb_control.setValue(control)
        self.ui.sb_control.blockSignals(False)

    def setMidiChannel(self, channel):
        self.fMidiChannel = channel
        self.ui.sb_channel.blockSignals(True)
        self.ui.sb_channel.setValue(channel)
        self.ui.sb_channel.blockSignals(False)

    def setLabelWidth(self, width):
        self.ui.label.setMinimumWidth(width)
        self.ui.label.setMaximumWidth(width)

    def tabIndex(self):
        return self.fTabIndex

    @pyqtSlot()
    def slot_controlSpinboxCustomMenu(self):
        menu = QMenu(self)

        for cc in MIDI_CC_LIST:
            action = menu.addAction(cc)

            if int(cc.split(" ")[0], 16) == self.fMidiControl:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if actSel:
            selControlStr = actSel.text()
            selControl    = int(selControlStr.split(" ")[0], 16)
            self.ui.sb_control.setValue(selControl)

    @pyqtSlot(int)
    def slot_controlSpinboxChanged(self, control):
        if self.fMidiControl != control:
            self.emit(SIGNAL("midiControlChanged(int, int)"), self.fParameterId, control)

        self.fMidiControl = control

    @pyqtSlot(int)
    def slot_channelSpinboxChanged(self, channel):
        if self.fMidiChannel != channel:
            self.emit(SIGNAL("midiChannelChanged(int, int)"), self.fParameterId, channel)

        self.fMidiChannel = channel

    @pyqtSlot(float)
    def slot_widgetValueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, double)"), self.fParameterId, value)

    def _textCallBack(self):
        return cString(Carla.host.get_parameter_text(self.fPluginId, self.fParameterId))

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor (Built-in)

class PluginEdit(QDialog):
    def __init__(self, parent, pluginId):
        QDialog.__init__(self, Carla.gui)
        self.ui = ui_carla_edit.Ui_PluginEdit()
        self.ui.setupUi(self)

        self.fGeometry   = QByteArray()
        self.fPluginId   = pluginId
        self.fPuginInfo  = None
        self.fRealParent = parent

        self.fCurrentProgram = -1
        self.fCurrentMidiProgram = -1
        self.fCurrentStateFilename = None

        self.fParameterCount = 0
        self.fParameterList  = []     # (type, id, widget)
        self.fParametersToUpdate = [] # (id, value)

        self.fTabIconOff = QIcon(":/bitmaps/led_off.png")
        self.fTabIconOn  = QIcon(":/bitmaps/led_yellow.png")
        self.fTabIconCount  = 0
        self.fTabIconTimers = []

        self.ui.dial_drywet.setPixmap(3)
        self.ui.dial_drywet.setLabel("Dry/Wet")
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_b_left.setPixmap(4)
        self.ui.dial_b_left.setLabel("L")
        self.ui.dial_b_right.setPixmap(4)
        self.ui.dial_b_right.setLabel("R")

        self.ui.dial_drywet.setCustomPaint(self.ui.dial_drywet.CUSTOM_PAINT_CARLA_WET)
        self.ui.dial_vol.setCustomPaint(self.ui.dial_vol.CUSTOM_PAINT_CARLA_VOL)
        self.ui.dial_b_left.setCustomPaint(self.ui.dial_b_left.CUSTOM_PAINT_CARLA_L)
        self.ui.dial_b_right.setCustomPaint(self.ui.dial_b_right.CUSTOM_PAINT_CARLA_R)

        self.ui.keyboard.setMode(self.ui.keyboard.HORIZONTAL)
        self.ui.keyboard.setOctaves(6)

        self.ui.scrollArea.ensureVisible(self.ui.keyboard.width() / 5, 0)
        self.ui.scrollArea.setEnabled(False)
        self.ui.scrollArea.setVisible(False)

        if Carla.isLocal:
            self.connect(self.ui.b_save_state, SIGNAL("clicked()"), SLOT("slot_saveState()"))
            self.connect(self.ui.b_load_state, SIGNAL("clicked()"), SLOT("slot_loadState()"))
        else:
            self.ui.b_load_state.setEnabled(False)
            self.ui.b_save_state.setEnabled(False)

        self.connect(self.ui.dial_drywet, SIGNAL("sliderMoved(int)"), SLOT("slot_dryWetChanged(int)"))
        self.connect(self.ui.dial_vol, SIGNAL("sliderMoved(int)"), SLOT("slot_volumeChanged(int)"))
        self.connect(self.ui.dial_b_left, SIGNAL("sliderMoved(int)"), SLOT("slot_balanceLeftChanged(int)"))
        self.connect(self.ui.dial_b_right, SIGNAL("sliderMoved(int)"), SLOT("slot_balanceRightChanged(int)"))

        #self.connect(self.ui.dial_drywet, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_vol, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_b_left, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_b_right, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))

        self.connect(self.ui.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.ui.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))
        self.connect(self.ui.keyboard, SIGNAL("notesOn()"), SLOT("slot_notesOn()"))
        self.connect(self.ui.keyboard, SIGNAL("notesOff()"), SLOT("slot_notesOff()"))

        self.connect(self.ui.cb_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_programIndexChanged(int)"))
        self.connect(self.ui.cb_midi_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiProgramIndexChanged(int)"))

        self.connect(self, SIGNAL("finished(int)"), SLOT("slot_finished()"))

        self.reloadAll()

    def reloadAll(self):
        self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId)
        self.fPluginInfo["binary"]    = cString(self.fPluginInfo["binary"])
        self.fPluginInfo["name"]      = cString(self.fPluginInfo["name"])
        self.fPluginInfo["label"]     = cString(self.fPluginInfo["label"])
        self.fPluginInfo["maker"]     = cString(self.fPluginInfo["maker"])
        self.fPluginInfo["copyright"] = cString(self.fPluginInfo["copyright"])

        self.reloadInfo()
        self.reloadParameters()
        self.reloadPrograms()

        if not self.ui.scrollArea.isEnabled():
            self.resize(self.width(), self.height()-self.ui.keyboard.height())

    def reloadInfo(self):
        pluginName  = cString(Carla.host.get_real_plugin_name(self.fPluginId))
        pluginType  = self.fPluginInfo['type']
        pluginHints = self.fPluginInfo['hints']

        audioCountInfo = Carla.host.get_audio_port_count_info(self.fPluginId)
        midiCountInfo  = Carla.host.get_midi_port_count_info(self.fPluginId)
        paramCountInfo = Carla.host.get_parameter_count_info(self.fPluginId)

        if pluginType == PLUGIN_INTERNAL:
            self.ui.le_type.setText(self.tr("Internal"))
        elif pluginType == PLUGIN_LADSPA:
            self.ui.le_type.setText("LADSPA")
        elif pluginType == PLUGIN_DSSI:
            self.ui.le_type.setText("DSSI")
        elif pluginType == PLUGIN_LV2:
            self.ui.le_type.setText("LV2")
        elif pluginType == PLUGIN_VST:
            self.ui.le_type.setText("VST")
        elif pluginType == PLUGIN_GIG:
            self.ui.le_type.setText("GIG")
        elif pluginType == PLUGIN_SF2:
            self.ui.le_type.setText("SF2")
        elif pluginType == PLUGIN_SFZ:
            self.ui.le_type.setText("SFZ")
        else:
            self.ui.le_type.setText(self.tr("Unknown"))

        self.ui.le_name.setText(pluginName)
        self.ui.le_name.setToolTip(pluginName)
        self.ui.le_label.setText(self.fPluginInfo['label'])
        self.ui.le_label.setToolTip(self.fPluginInfo['label'])
        self.ui.le_maker.setText(self.fPluginInfo['maker'])
        self.ui.le_maker.setToolTip(self.fPluginInfo['maker'])
        self.ui.le_copyright.setText(self.fPluginInfo['copyright'])
        self.ui.le_copyright.setToolTip(self.fPluginInfo['copyright'])
        self.ui.le_unique_id.setText(str(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setToolTip(str(self.fPluginInfo['uniqueId']))
        self.ui.le_ains.setText(str(audioCountInfo['ins']))
        self.ui.le_aouts.setText(str(audioCountInfo['outs']))
        self.ui.le_params.setText(str(paramCountInfo['ins']))
        self.ui.label_plugin.setText("\n%s\n" % self.fPluginInfo['name'])
        self.setWindowTitle(self.fPluginInfo['name'])

        if self.fPluginInfo['latency'] > 0:
            self.ui.le_latency.setText("%i samples" % self.fPluginInfo['latency'])
        else:
            self.ui.le_latency.setText(self.tr("None"))

        self.ui.dial_drywet.setEnabled(pluginHints & PLUGIN_CAN_DRYWET)
        self.ui.dial_vol.setEnabled(pluginHints & PLUGIN_CAN_VOLUME)
        self.ui.dial_b_left.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_b_right.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)

        # Show/hide keyboard
        showKeyboard = (pluginHints & PLUGIN_IS_SYNTH) != 0 or (midiCountInfo['ins'] > 0 < midiCountInfo['outs'])
        self.ui.scrollArea.setEnabled(showKeyboard)
        self.ui.scrollArea.setVisible(showKeyboard)

        # Force-Update parent for new hints (knobs)
        if self.fRealParent:
            self.fRealParent.recheckPluginHints(pluginHints)

    def reloadParameters(self):
        parameterCount = Carla.host.get_parameter_count(self.fPluginId)

        # Reset
        self.fParameterCount = 0
        self.fParameterList  = []
        self.fParametersToUpdate = []

        self.fTabIconCount  = 0
        self.fTabIconTimers = []

        # Remove all previous parameters
        for i in range(self.ui.tabWidget.count()-1):
            self.ui.tabWidget.widget(1).deleteLater()
            self.ui.tabWidget.removeTab(1)

        if parameterCount <= 0:
            pass

        elif parameterCount <= Carla.maxParameters:
            paramInputListFull  = []
            paramOutputListFull = []

            paramInputList   = [] # ([params], width)
            paramInputWidth  = 0
            paramOutputList  = [] # ([params], width)
            paramOutputWidth = 0

            for i in range(parameterCount):
                paramInfo   = Carla.host.get_parameter_info(self.fPluginId, i)
                paramData   = Carla.host.get_parameter_data(self.fPluginId, i)
                paramRanges = Carla.host.get_parameter_ranges(self.fPluginId, i)

                if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                    continue

                parameter = {
                    'type':  paramData['type'],
                    'hints': paramData['hints'],
                    'name':  cString(paramInfo['name']),
                    'unit':  cString(paramInfo['unit']),
                    'scalePoints': [],

                    'index':   paramData['index'],
                    'default': paramRanges['def'],
                    'minimum': paramRanges['min'],
                    'maximum': paramRanges['max'],
                    'step':    paramRanges['step'],
                    'stepSmall': paramRanges['stepSmall'],
                    'stepLarge': paramRanges['stepLarge'],
                    'midiCC':    paramData['midiCC'],
                    'midiChannel': paramData['midiChannel'],

                    'current': Carla.host.get_current_parameter_value(self.fPluginId, i)
                }

                for j in range(paramInfo['scalePointCount']):
                    scalePointInfo = Carla.host.get_parameter_scalepoint_info(self.fPluginId, i, j)

                    parameter['scalePoints'].append({
                        'value': scalePointInfo['value'],
                        'label': cString(scalePointInfo['label'])
                    })

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if parameter['type'] == PARAMETER_INPUT:
                    paramInputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramInputWidthTMP > paramInputWidth:
                        paramInputWidth = paramInputWidthTMP

                    paramInputList.append(parameter)

                    if len(paramInputList) == 10:
                        paramInputListFull.append((paramInputList, paramInputWidth))
                        paramInputList  = []
                        paramInputWidth = 0

                else:
                    paramOutputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramOutputWidthTMP > paramOutputWidth:
                        paramOutputWidth = paramOutputWidthTMP

                    paramOutputList.append(parameter)

                    if len(paramOutputList) == 10:
                        paramOutputListFull.append((paramOutputList, paramOutputWidth))
                        paramOutputList  = []
                        paramOutputWidth = 0

            # for i in range(parameterCount)
            else:
                # Final page width values
                if 0 < len(paramInputList) < 10:
                    paramInputListFull.append((paramInputList, paramInputWidth))

                if 0 < len(paramOutputList) < 10:
                    paramOutputListFull.append((paramOutputList, paramOutputWidth))

            # -----------------------------------------------------------------
            # Create parameter widgets

            self._createParameterWidgets(PARAMETER_INPUT,  paramInputListFull,  self.tr("Parameters"))
            self._createParameterWidgets(PARAMETER_OUTPUT, paramOutputListFull, self.tr("Outputs"))

        else: # > Carla.maxParameters
            fakeName = self.tr("This plugin has too many parameters to display here!")

            paramFakeListFull = []
            paramFakeList  = []
            paramFakeWidth = QFontMetrics(self.font()).width(fakeName)

            parameter = {
                'type':  PARAMETER_UNKNOWN,
                'hints': 0,
                'name':  fakeName,
                'unit':  "",
                'scalePoints': [],

                'index':   0,
                'default': 0,
                'minimum': 0,
                'maximum': 0,
                'step':     0,
                'stepSmall': 0,
                'stepLarge': 0,
                'midiCC':   -1,
                'midiChannel': 0,

                'current': 0.0
            }

            paramFakeList.append(parameter)
            paramFakeListFull.append((paramFakeList, paramFakeWidth))

            self._createParameterWidgets(PARAMETER_UNKNOWN, paramFakeListFull, self.tr("Information"))

    def reloadPrograms(self):
        # Programs
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.clear()

        programCount = Carla.host.get_program_count(self.fPluginId)

        if programCount > 0:
            self.ui.cb_programs.setEnabled(True)

            for i in range(programCount):
                pName = cString(Carla.host.get_program_name(self.fPluginId, i))
                self.ui.cb_programs.addItem(pName)

            self.fCurrentProgram = Carla.host.get_current_program_index(self.fPluginId)
            self.ui.cb_programs.setCurrentIndex(self.fCurrentProgram)

        else:
            self.fCurrentProgram = -1
            self.ui.cb_programs.setEnabled(False)

        self.ui.cb_programs.blockSignals(False)

        # MIDI Programs
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.clear()

        midiProgramCount = Carla.host.get_midi_program_count(self.fPluginId)

        if midiProgramCount > 0:
            self.ui.cb_midi_programs.setEnabled(True)

            for i in range(midiProgramCount):
                mpData  = Carla.host.get_midi_program_data(self.fPluginId, i)
                mpBank  = int(mpData['bank'])
                mpProg  = int(mpData['program'])
                mpLabel = cString(mpData['name'])
                self.ui.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank, mpProg, mpLabel))

            self.fCurrentMidiProgram = Carla.host.get_current_midi_program_index(self.fPluginId)
            self.ui.cb_midi_programs.setCurrentIndex(self.fCurrentMidiProgram)

        else:
            self.fCurrentMidiProgram = -1
            self.ui.cb_midi_programs.setEnabled(False)

        self.ui.cb_midi_programs.blockSignals(False)

        # Automatically change to Midi Programs tab
        if midiProgramCount > 0 and programCount == 0:
            self.ui.tab_programs.setCurrentIndex(1)

    def updateInfo(self):
        # Update current program text
        if self.ui.cb_programs.count() > 0:
            pIndex = self.ui.cb_programs.currentIndex()
            pName  = cString(Carla.host.get_program_name(self.fPluginId, pIndex))
            self.ui.cb_programs.setItemText(pIndex, pName)

        # Update current midi program text
        if self.ui.cb_midi_programs.count() > 0:
            mpIndex = self.ui.cb_midi_programs.currentIndex()
            mpData  = Carla.host.get_midi_program_data(self.fPluginId, mpIndex)
            mpBank  = int(mpData['bank'])
            mpProg  = int(mpData['program'])
            mpLabel = cString(mpData['name'])
            self.ui.cb_midi_programs.setItemText(mpIndex, "%03i:%03i - %s" % (mpBank, mpProg, mpLabel))

        # Update all parameter values
        for paramType, paramId, paramWidget in self.fParameterList:
            paramWidget.setValue(Carla.host.get_current_parameter_value(self.fPluginId, paramId), False)
            paramWidget.update()

    def setParameterValue(self, parameterId, value):
        for paramItem in self.fParametersToUpdate:
            if paramItem[0] == parameterId:
                paramItem[1] = value
                break
        else:
            self.fParametersToUpdate.append([parameterId, value])

    def setParameterDefault(self, parameterId, value):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setDefault(value)
                break

    def setParameterMidiControl(self, parameterId, control):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMidiControl(control)
                break

    def setParameterMidiChannel(self, parameterId, channel):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMidiChannel(channel)
                break

    def setProgram(self, index):
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.setCurrentIndex(index)
        self.ui.cb_programs.blockSignals(False)

    def setMidiProgram(self, index):
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.setCurrentIndex(index)
        self.ui.cb_midi_programs.blockSignals(False)

    def setVisible(self, yesNo):
        if yesNo:
            if not self.fGeometry.isNull():
                self.restoreGeometry(self.fGeometry)
        else:
            self.fGeometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

    def idleSlow(self):
        # Check Tab icons
        for i in range(len(self.fTabIconTimers)):
            if self.fTabIconTimers[i] == ICON_STATE_ON:
                self.fTabIconTimers[i] = ICON_STATE_WAIT
            elif self.fTabIconTimers[i] == ICON_STATE_WAIT:
                self.fTabIconTimers[i] = ICON_STATE_OFF
            elif self.fTabIconTimers[i] == ICON_STATE_OFF:
                self.fTabIconTimers[i] = ICON_STATE_NULL
                self.ui.tabWidget.setTabIcon(i+1, self.fTabIconOff)

        # Check parameters needing update
        for index, value in self.fParametersToUpdate:
            if index == PARAMETER_DRYWET:
                self.ui.dial_drywet.blockSignals(True)
                self.ui.dial_drywet.setValue(value * 1000)
                self.ui.dial_drywet.blockSignals(False)
            elif index == PARAMETER_VOLUME:
                self.ui.dial_vol.blockSignals(True)
                self.ui.dial_vol.setValue(value * 1000)
                self.ui.dial_vol.blockSignals(False)
            elif index == PARAMETER_BALANCE_LEFT:
                self.ui.dial_b_left.blockSignals(True)
                self.ui.dial_b_left.setValue(value * 1000)
                self.ui.dial_b_left.blockSignals(False)
            elif index == PARAMETER_BALANCE_RIGHT:
                self.ui.dial_b_right.blockSignals(True)
                self.ui.dial_b_right.setValue(value * 1000)
                self.ui.dial_b_right.blockSignals(False)
            elif index == PARAMETER_PANNING:
                pass
                #self.ui.dial_pan.blockSignals(True)
                #self.ui.dial_pan.setValue(value * 1000, True, False)
                #self.ui.dial_pan.blockSignals(False)
            elif index >= 0:
                for paramType, paramId, paramWidget in self.fParameterList:
                    if paramId == index:
                        paramWidget.setValue(value, False)

                        if paramType == PARAMETER_INPUT:
                            tabIndex = paramWidget.tabIndex()

                            if self.fTabIconTimers[tabIndex-1] == ICON_STATE_NULL:
                                self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOn)

                            self.fTabIconTimers[tabIndex-1] = ICON_STATE_ON

                        break

        # Clear all parameters
        self.fParametersToUpdate = []

        # Update parameter outputs
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramType == PARAMETER_OUTPUT:
                value = Carla.host.get_current_parameter_value(self.fPluginId, paramId)
                paramWidget.setValue(value, False)

    @pyqtSlot()
    def slot_saveState(self):
        if self.fCurrentStateFilename:
            askTry = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if askTry == QMessageBox.Ok:
                return #self.saveState()

            self.fCurrentStateFilename = None

        fileFilter  = self.tr("Carla State File (*.carxs)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Plugin State File"), filter=fileFilter)

        if filenameTry:
            if not filenameTry.endswith(".carxs"):
                filenameTry += ".carxs"

            self.fCurrentStateFilename = filenameTry
            #self.saveState()

    @pyqtSlot()
    def slot_loadState(self):
        fileFilter  = self.tr("Carla State File (*.carxs)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Plugin State File"), filter=fileFilter)

        if filenameTry:
            self.fCurrentStateFilename = filenameTry
            #self.loadState()

    @pyqtSlot(int)
    def slot_dryWetChanged(self, value):
        Carla.host.set_drywet(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_volumeChanged(self, value):
        Carla.host.set_volume(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_balanceLeftChanged(self, value):
        Carla.host.set_balance_left(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_balanceRightChanged(self, value):
        Carla.host.set_balance_right(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_panningChanged(self, value):
        Carla.host.set_panning(self.fPluginId, float(value)/1000)

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameterId, value):
        Carla.host.set_parameter_value(self.fPluginId, parameterId, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiControlChanged(self, parameterId, control):
        Carla.host.set_parameter_midi_cc(self.fPluginId, parameterId, control)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameterId, channel):
        Carla.host.set_parameter_midi_channel(self.fPluginId, parameterId, channel-1)

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        if self.fCurrentProgram != index:
            self.fCurrentProgram = index
            Carla.host.set_program(self.fPluginId, index)

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        if self.fCurrentMidiProgram != index:
            self.fCurrentMidiProgram = index
            Carla.host.set_midi_program(self.fPluginId, index)

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        Carla.host.send_midi_note(self.fPluginId, 0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        Carla.host.send_midi_note(self.fPluginId, 0, note, 0)

    @pyqtSlot()
    def slot_notesOn(self):
        if self.fRealParent:
            self.fRealParent.ui.led_midi.setChecked(True)

    @pyqtSlot()
    def slot_notesOff(self):
        if self.fRealParent:
            self.fRealParent.ui.led_midi.setChecked(False)

    @pyqtSlot()
    def slot_finished(self):
        if self.fRealParent:
            self.fRealParent.editClosed()

    def _createParameterWidgets(self, paramType, paramListFull, tabPageName):
        i = 1
        for paramList, width in paramListFull:
            if len(paramList) == 0:
                break

            tabIndex         = self.ui.tabWidget.count()
            tabPageContainer = QWidget(self.ui.tabWidget)
            tabPageLayout    = QVBoxLayout(tabPageContainer)
            tabPageContainer.setLayout(tabPageLayout)

            for paramInfo in paramList:
                paramWidget = PluginParameter(tabPageContainer, paramInfo, self.fPluginId, tabIndex)
                paramWidget.setLabelWidth(width)
                tabPageLayout.addWidget(paramWidget)

                self.fParameterList.append((paramType, paramInfo['index'], paramWidget))

                if paramType == PARAMETER_INPUT:
                    self.connect(paramWidget, SIGNAL("valueChanged(int, double)"), SLOT("slot_parameterValueChanged(int, double)"))

                self.connect(paramWidget, SIGNAL("midiControlChanged(int, int)"), SLOT("slot_parameterMidiControlChanged(int, int)"))
                self.connect(paramWidget, SIGNAL("midiChannelChanged(int, int)"), SLOT("slot_parameterMidiChannelChanged(int, int)"))

            tabPageLayout.addStretch()

            self.ui.tabWidget.addTab(tabPageContainer, "%s (%i)" % (tabPageName, i))
            i += 1

            if paramType == PARAMETER_INPUT:
                self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOff)

            self.fTabIconTimers.append(ICON_STATE_NULL)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Widget

class PluginWidget(QFrame):
    def __init__(self, parent, pluginId):
        QFrame.__init__(self, parent)
        self.ui = ui_carla_plugin.Ui_PluginWidget()
        self.ui.setupUi(self)

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        self.fParameterIconTimer = ICON_STATE_NULL

        self.fPluginId   = pluginId
        self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId)
        self.fPluginInfo["binary"]    = cString(self.fPluginInfo["binary"])
        self.fPluginInfo["name"]      = cString(self.fPluginInfo["name"])
        self.fPluginInfo["label"]     = cString(self.fPluginInfo["label"])
        self.fPluginInfo["maker"]     = cString(self.fPluginInfo["maker"])
        self.fPluginInfo["copyright"] = cString(self.fPluginInfo["copyright"])

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            self.fPeaksInputCount  = 2
            self.fPeaksOutputCount = 2
            #self.ui.stackedWidget.setCurrentIndex(0)
        else:
            audioCountInfo = Carla.host.get_audio_port_count_info(self.fPluginId)

            self.fPeaksInputCount  = int(audioCountInfo['ins'])
            self.fPeaksOutputCount = int(audioCountInfo['outs'])

            if self.fPeaksInputCount > 2:
                self.fPeaksInputCount = 2

            if self.fPeaksOutputCount > 2:
                self.fPeaksOutputCount = 2

            #if audioCountInfo['total'] == 0:
                #self.ui.stackedWidget.setCurrentIndex(1)
            #else:
                #self.ui.stackedWidget.setCurrentIndex(0)

        # Background
        self.fColorTop    = QColor(60, 60, 60)
        self.fColorBottom = QColor(47, 47, 47)

        # Colorify
        #if self.m_pluginInfo['category'] == PLUGIN_CATEGORY_SYNTH:
            #self.setWidgetColor(PALETTE_COLOR_WHITE)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_DELAY:
            #self.setWidgetColor(PALETTE_COLOR_ORANGE)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_EQ:
            #self.setWidgetColor(PALETTE_COLOR_GREEN)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_FILTER:
            #self.setWidgetColor(PALETTE_COLOR_BLUE)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_DYNAMICS:
            #self.setWidgetColor(PALETTE_COLOR_PINK)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_MODULATOR:
            #self.setWidgetColor(PALETTE_COLOR_RED)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_UTILITY:
            #self.setWidgetColor(PALETTE_COLOR_YELLOW)
        #elif self.m_pluginInfo['category'] == PLUGIN_CATEGORY_OTHER:
            #self.setWidgetColor(PALETTE_COLOR_BROWN)
        #else:
            #self.setWidgetColor(PALETTE_COLOR_NONE)

        self.ui.led_enable.setColor(self.ui.led_enable.BIG_RED)
        self.ui.led_enable.setChecked(False)

        self.ui.led_control.setColor(self.ui.led_control.YELLOW)
        self.ui.led_control.setEnabled(False)

        self.ui.led_midi.setColor(self.ui.led_midi.RED)
        self.ui.led_midi.setEnabled(False)

        self.ui.led_audio_in.setColor(self.ui.led_audio_in.GREEN)
        self.ui.led_audio_in.setEnabled(False)

        self.ui.led_audio_out.setColor(self.ui.led_audio_out.BLUE)
        self.ui.led_audio_out.setEnabled(False)

        self.ui.peak_in.setColor(self.ui.peak_in.GREEN)
        self.ui.peak_in.setOrientation(self.ui.peak_in.HORIZONTAL)

        self.ui.peak_out.setColor(self.ui.peak_in.BLUE)
        self.ui.peak_out.setOrientation(self.ui.peak_out.HORIZONTAL)

        self.ui.peak_in.setChannels(self.fPeaksInputCount)
        self.ui.peak_out.setChannels(self.fPeaksOutputCount)

        self.ui.label_name.setText(self.fPluginInfo['name'])

        self.ui.edit_dialog = PluginEdit(self, self.fPluginId)
        self.ui.edit_dialog.hide()

        self.connect(self.ui.led_enable, SIGNAL("clicked(bool)"), SLOT("slot_setActive(bool)"))
        self.connect(self.ui.b_gui, SIGNAL("clicked(bool)"), SLOT("slot_guiClicked(bool)"))
        self.connect(self.ui.b_edit, SIGNAL("clicked(bool)"), SLOT("slot_editClicked(bool)"))
        self.connect(self.ui.b_remove, SIGNAL("clicked()"), SLOT("slot_removeClicked()"))

        # FIXME
        self.setMaximumHeight(48)

    def idleFast(self):
        # Input peaks
        if self.fPeaksInputCount > 0:
            if self.fPeaksInputCount > 1:
                peak1 = Carla.host.get_input_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_input_peak_value(self.fPluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.ui.peak_in.displayMeter(1, peak1)
                self.ui.peak_in.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_input_peak_value(self.fPluginId, 1)
                ledState = bool(peak != 0.0)

                self.ui.peak_in.displayMeter(1, peak)

            if self.fLastGreenLedState != ledState:
                self.fLastGreenLedState = ledState
                self.ui.led_audio_in.setChecked(ledState)

        # Output peaks
        if self.fPeaksOutputCount > 0:
            if self.fPeaksOutputCount > 1:
                peak1 = Carla.host.get_output_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_output_peak_value(self.fPluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.ui.peak_out.displayMeter(1, peak1)
                self.ui.peak_out.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_output_peak_value(self.fPluginId, 1)
                ledState = bool(peak != 0.0)

                self.ui.peak_out.displayMeter(1, peak)

            if self.fLastBlueLedState != ledState:
                self.fLastBlueLedState = ledState
                self.ui.led_audio_out.setChecked(ledState)

    def idleSlow(self):
        # Parameter Activity LED
        if self.fParameterIconTimer == ICON_STATE_ON:
            self.fParameterIconTimer = ICON_STATE_WAIT
            self.ui.led_control.setChecked(True)
        elif self.fParameterIconTimer == ICON_STATE_WAIT:
            self.fParameterIconTimer = ICON_STATE_OFF
        elif self.fParameterIconTimer == ICON_STATE_OFF:
            self.fParameterIconTimer = ICON_STATE_NULL
            self.ui.led_control.setChecked(False)

        # Update edit dialog
        self.ui.edit_dialog.idleSlow()

    def editClosed(self):
        self.ui.b_edit.setChecked(False)

    def recheckPluginHints(self, hints):
        self.fPluginInfo['hints'] = hints
        self.ui.b_gui.setEnabled(hints & PLUGIN_HAS_GUI)

    def setActive(self, active, sendGui=False, sendCallback=True):
        if sendGui:      self.ui.led_enable.setChecked(active)
        if sendCallback: Carla.host.set_active(self.fPluginId, active)

        if active:
            self.ui.edit_dialog.ui.keyboard.allNotesOff()

    def setParameterDefault(self, parameterId, value):
        self.ui.edit_dialog.setParameterDefault(parameterId, value)

    def setParameterValue(self, parameterId, value):
        self.fParameterIconTimer = ICON_STATE_ON

        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, False)

        self.ui.edit_dialog.setParameterValue(parameterId, value)

    def setParameterMidiControl(self, parameterId, control):
        self.ui.edit_dialog.setParameterMidiControl(parameterId, control)

    def setParameterMidiChannel(self, parameterId, channel):
        self.ui.edit_dialog.setParameterMidiChannel(parameterId, channel)

    def setProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.ui.edit_dialog.setProgram(index)

    def setMidiProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.ui.edit_dialog.setMidiProgram(index)

    def sendNoteOn(self, note):
        self.ui.edit_dialog.ui.keyboard.sendNoteOn(note, False)

    def sendNoteOff(self, note):
        self.ui.edit_dialog.ui.keyboard.sendNoteOff(note, False)

    def setId(self, idx):
        self.fPluginId = idx
        self.ui.edit_dialog.fPluginId = idx

    def setRefreshRate(self, rate):
        self.ui.peak_in.setRefreshRate(rate)
        self.ui.peak_out.setRefreshRate(rate)

    def paintEvent(self, event):
        painter = QPainter(self)

        areaX = self.ui.area_right.x()

        # background
        #painter.setPen(self.m_colorTop)
        #painter.setBrush(self.m_colorTop)
        #painter.drawRect(0, 0, areaX+40, self.height())

        # bottom line
        painter.setPen(self.fColorBottom)
        painter.setBrush(self.fColorBottom)
        painter.drawRect(0, self.height()-5, areaX, 5)

        # top line
        painter.drawLine(0, 0, areaX+40, 0)

        # name -> leds arc
        path = QPainterPath()
        path.moveTo(areaX-80, self.height())
        path.cubicTo(areaX+40, self.height()-5, areaX-40, 30, areaX+20, 0)
        path.lineTo(areaX+20, self.height())
        painter.drawPath(path)

        # fill the rest
        painter.drawRect(areaX+20, 0, self.width(), self.height())

        #painter.drawLine(0, 3, self.width(), 3)
        #painter.drawLine(0, self.height() - 4, self.width(), self.height() - 4)
        #painter.setPen(self.m_color2)
        #painter.drawLine(0, 2, self.width(), 2)
        #painter.drawLine(0, self.height() - 3, self.width(), self.height() - 3)
        #painter.setPen(self.m_color3)
        #painter.drawLine(0, 1, self.width(), 1)
        #painter.drawLine(0, self.height() - 2, self.width(), self.height() - 2)
        #painter.setPen(self.m_color4)
        #painter.drawLine(0, 0, self.width(), 0)
        #painter.drawLine(0, self.height() - 1, self.width(), self.height() - 1)
        QFrame.paintEvent(self, event)

    @pyqtSlot(bool)
    def slot_setActive(self, yesNo):
        self.setActive(yesNo, False, True)

    @pyqtSlot(bool)
    def slot_guiClicked(self, show):
        Carla.host.show_gui(self.fPluginId, show)

    @pyqtSlot(bool)
    def slot_editClicked(self, show):
        self.ui.edit_dialog.setVisible(show)

    @pyqtSlot()
    def slot_removeClicked(self):
        Carla.host.remove_plugin(self.fPluginId)

# ------------------------------------------------------------------------------------------------------------
# Separate Thread for Plugin Search

class SearchPluginsThread(QThread):
    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.fCheckNative  = False
        self.fCheckPosix32 = False
        self.fCheckPosix64 = False
        self.fCheckWin32   = False
        self.fCheckWin64   = False

        self.fCheckLADSPA = False
        self.fCheckDSSI = False
        self.fCheckLV2 = False
        self.fCheckVST = False
        self.fCheckGIG = False
        self.fCheckSF2 = False
        self.fCheckSFZ = False

        self.fToolNative = carla_discovery_native

        self.fCurCount = 0
        self.fCurPercentValue = 0
        self.fLastCheckValue  = 0
        self.fSomethingChanged = False

    def somethingChanged(self):
        return self.fSomethingChanged

    def skipPlugin(self):
        # TODO - windows and mac support
        apps  = " carla-discovery"
        apps += " carla-discovery-native"
        apps += " carla-discovery-posix32"
        apps += " carla-discovery-posix64"
        apps += " carla-discovery-win32.exe"
        apps += " carla-discovery-win64.exe"

        if LINUX:
            os.system("killall -KILL %s" % apps)

    def setSearchBinaryTypes(self, native, posix32, posix64, win32, win64):
        self.fCheckNative  = native
        self.fCheckPosix32 = posix32
        self.fCheckPosix64 = posix64
        self.fCheckWin32   = win32
        self.fCheckWin64   = win64

    def setSearchPluginTypes(self, ladspa, dssi, lv2, vst, gig, sf2, sfz):
        self.fCheckLADSPA = ladspa
        self.fCheckDSSI = dssi
        self.fCheckLV2 = lv2
        self.fCheckVST = vst
        self.fCheckGIG = gig
        self.fCheckSF2 = sf2
        self.fCheckSFZ = sfz

    def setSearchToolNative(self, tool):
        self.fToolNative = tool

    def run(self):
        self.fCurCount = 0
        pluginCount    = 0
        settingsDB     = QSettings("falkTX", "CarlaPlugins")

        if self.fCheckLADSPA: pluginCount += 1
        if self.fCheckDSSI:   pluginCount += 1
        if self.fCheckLV2:    pluginCount += 1
        if self.fCheckVST:    pluginCount += 1

        if self.fCheckNative:
            self.fCurCount += pluginCount
        if self.fCheckPosix32:
            self.fCurCount += pluginCount
        if self.fCheckPosix64:
            self.fCurCount += pluginCount
        if self.fCheckWin32:
            self.fCurCount += pluginCount
        if self.fCheckWin64:
            self.fCurCount += pluginCount

        if self.fToolNative:
            if self.fCheckGIG: self.fCurCount += 1
            if self.fCheckSF2: self.fCurCount += 1
            if self.fCheckSFZ: self.fCurCount += 1
        else:
            self.fCheckGIG = False
            self.fCheckSF2 = False
            self.fCheckSFZ = False

        if self.fCurCount == 0:
            return

        self.fCurPercentValue = 100 / self.fCurCount
        self.fLastCheckValue  = 0

        if HAIKU:
            OS = "HAIKU"
        elif LINUX:
            OS = "LINUX"
        elif MACOS:
            OS = "MACOS"
        elif WINDOWS:
            OS = "WINDOWS"
        else:
            OS = "UNKNOWN"

        if self.fCheckLADSPA:
            checkValue = 0
            if haveLRDF:
                if self.fCheckNative:  checkValue += 0.1
                if self.fCheckPosix32: checkValue += 0.1
                if self.fCheckPosix64: checkValue += 0.1
                if self.fCheckWin32:   checkValue += 0.1
                if self.fCheckWin64:   checkValue += 0.1
            rdfPadValue = self.fCurPercentValue * checkValue

            if self.fCheckNative:
                self._checkLADSPA(OS, carla_discovery_native)
                settingsDB.setValue("Plugins/LADSPA_native", self.fLadspaPlugins)
                settingsDB.sync()

            if self.fCheckPosix32:
                self._checkLADSPA(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/LADSPA_posix32", self.fLadspaPlugins)
                settingsDB.sync()

            if self.fCheckPosix64:
                self._checkLADSPA(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/LADSPA_posix64", self.fLadspaPlugins)
                settingsDB.sync()

            if self.fCheckWin32:
                self._checkLADSPA("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win32", self.fLadspaPlugins)
                settingsDB.sync()

            if self.fCheckWin64:
                self._checkLADSPA("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win64", self.fLadspaPlugins)
                settingsDB.sync()

            if haveLRDF and checkValue > 0:
                startValue = self.fLastCheckValue - rdfPadValue

                self._pluginLook(startValue, "LADSPA RDFs...")
                ladspaRdfInfo = ladspa_rdf.recheck_all_plugins(self, startValue, self.fCurPercentValue, checkValue)

                SettingsDir = os.path.join(HOME, ".config", "falkTX")

                fLadspa = open(os.path.join(SettingsDir, "ladspa_rdf.db"), 'w')
                json.dump(ladspaRdfInfo, fLadspa)
                fLadspa.close()

        if self.fCheckDSSI:
            if self.fCheckNative:
                self._checkDSSI(OS, carla_discovery_native)
                settingsDB.setValue("Plugins/DSSI_native", self.fDssiPlugins)
                settingsDB.sync()

            if self.fCheckPosix32:
                self._checkDSSI(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/DSSI_posix32", self.fDssiPlugins)
                settingsDB.sync()

            if self.fCheckPosix64:
                self._checkDSSI(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/DSSI_posix64", self.fDssiPlugins)
                settingsDB.sync()

            if self.fCheckWin32:
                self._checkDSSI("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win32", self.fDssiPlugins)
                settingsDB.sync()

            if self.fCheckWin64:
                self._checkDSSI("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win64", self.fDssiPlugins)
                settingsDB.sync()

        if self.fCheckLV2:
            if self.fCheckNative:
                self._checkLV2(carla_discovery_native)
                settingsDB.setValue("Plugins/LV2_native", self.fLv2Plugins)
                settingsDB.sync()

            if self.fCheckPosix32:
                self._checkLV2(carla_discovery_posix32)
                settingsDB.setValue("Plugins/LV2_posix32", self.fLv2Plugins)
                settingsDB.sync()

            if self.fCheckPosix64:
                self._checkLV2(carla_discovery_posix64)
                settingsDB.setValue("Plugins/LV2_posix64", self.fLv2Plugins)
                settingsDB.sync()

            if self.fCheckWin32:
                self._checkLV2(carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/LV2_win32", self.fLv2Plugins)
                settingsDB.sync()

            if self.fCheckWin64:
                self._checkLV2(carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/LV2_win64", self.fLv2Plugins)
                settingsDB.sync()

        if self.fCheckVST:
            if self.fCheckNative:
                self._checkVST(OS, carla_discovery_native)
                settingsDB.setValue("Plugins/VST_native", self.fVstPlugins)
                settingsDB.sync()

            if self.fCheckPosix32:
                self._checkVST(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/VST_posix32", self.fVstPlugins)
                settingsDB.sync()

            if self.fCheckPosix64:
                self._checkVST(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/VST_posix64", self.fVstPlugins)
                settingsDB.sync()

            if self.fCheckWin32:
                self._checkVST("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/VST_win32", self.fVstPlugins)
                settingsDB.sync()

            if self.fCheckWin64:
                self._checkVST("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/VST_win64", self.fVstPlugins)
                settingsDB.sync()

        if self.fCheckGIG:
            global GIG_PATH
            self._checkKIT(GIG_PATH, "gig")
            settingsDB.setValue("Plugins/GIG", self.fKitPlugins)
            settingsDB.sync()

        if self.fCheckSF2:
            global SF2_PATH
            self._checkKIT(SF2_PATH, "sf2")
            settingsDB.setValue("Plugins/SF2", self.fKitPlugins)
            settingsDB.sync()

        if self.fCheckSFZ:
            global SFZ_PATH
            self._checkKIT(SFZ_PATH, "sfz")
            settingsDB.setValue("Plugins/SFZ", self.fKitPlugins)
            settingsDB.sync()

    def _checkLADSPA(self, OS, tool, isWine=False):
        global LADSPA_PATH
        ladspaBinaries = []
        self.fLadspaPlugins = []

        for iPATH in LADSPA_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in ladspaBinaries:
                    ladspaBinaries.append(binary)

        ladspaBinaries.sort()

        for i in range(len(ladspaBinaries)):
            ladspa  = ladspaBinaries[i]
            percent = ( float(i) / len(ladspaBinaries) ) * self.fCurPercentValue
            self._pluginLook((self.fLastCheckValue + percent) * 0.9, ladspa)

            plugins = checkPluginLADSPA(ladspa, tool, isWine)
            if plugins:
                self.fLadspaPlugins.append(plugins)
                self.fSomethingChanged = True

        self.fLastCheckValue += self.fCurPercentValue

    def _checkDSSI(self, OS, tool, isWine=False):
        global DSSI_PATH
        dssiBinaries = []
        self.fDssiPlugins = []

        for iPATH in DSSI_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in dssiBinaries:
                    dssiBinaries.append(binary)

        dssiBinaries.sort()

        for i in range(len(dssiBinaries)):
            dssi    = dssiBinaries[i]
            percent = ( float(i) / len(dssiBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, dssi)

            plugins = checkPluginDSSI(dssi, tool, isWine)
            if plugins:
                self.fDssiPlugins.append(plugins)
                self.fSomethingChanged = True

        self.fLastCheckValue += self.fCurPercentValue

    def _checkLV2(self, tool, isWine=False):
        global LV2_PATH
        lv2Bundles = []
        self.fLv2Plugins = []

        self._pluginLook(self.fLastCheckValue, "LV2 bundles...")

        for iPATH in LV2_PATH:
            bundles = findLV2Bundles(iPATH)
            for bundle in bundles:
                if bundle not in lv2Bundles:
                    lv2Bundles.append(bundle)

        lv2Bundles.sort()

        for i in range(len(lv2Bundles)):
            lv2     = lv2Bundles[i]
            percent = ( float(i) / len(lv2Bundles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, lv2)

            plugins = checkPluginLV2(lv2, tool, isWine)
            if plugins:
                self.fLv2Plugins.append(plugins)
                self.fSomethingChanged = True

        self.fLastCheckValue += self.fCurPercentValue

    def _checkVST(self, OS, tool, isWine=False):
        global VST_PATH
        vstBinaries = []
        self.fVstPlugins = []

        for iPATH in VST_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in vstBinaries:
                    vstBinaries.append(binary)

        vstBinaries.sort()

        for i in range(len(vstBinaries)):
            vst     = vstBinaries[i]
            percent = ( float(i) / len(vstBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst)

            plugins = checkPluginVST(vst, tool, isWine)
            if plugins:
                self.fVstPlugins.append(plugins)
                self.fSomethingChanged = True

        self.fLastCheckValue += self.fCurPercentValue

    def _checkKIT(self, kPATH, kType):
        kitFiles = []
        self.fKitPlugins = []

        for iPATH in kPATH:
            files = findSoundKits(iPATH, kType)
            for file_ in files:
                if file_ not in kitFiles:
                    kitFiles.append(file_)

        kitFiles.sort()

        for i in range(len(kitFiles)):
            kit     = kitFiles[i]
            percent = ( float(i) / len(kitFiles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, kit)

            if kType == "gig":
                plugins = checkPluginGIG(kit, self.fToolNative)
            elif kType == "sf2":
                plugins = checkPluginSF2(kit, self.fToolNative)
            elif kType == "sfz":
                plugins = checkPluginSFZ(kit, self.fToolNative)
            else:
                plugins = None

            if plugins:
                self.fKitPlugins.append(plugins)
                self.fSomethingChanged = True

        self.fLastCheckValue += self.fCurPercentValue

    def _pluginLook(self, percent, plugin):
        self.emit(SIGNAL("PluginLook(int, QString)"), percent, plugin)

# ------------------------------------------------------------------------------------------------------------
# Plugin Refresh Dialog

class PluginRefreshW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_refresh.Ui_PluginRefreshW()
        self.ui.setupUi(self)

        self._loadSettings()

        self.ui.b_skip.setVisible(False)

        if HAIKU:
            self.ui.ch_posix32.setText("Haiku 32bit")
            self.ui.ch_posix64.setText("Haiku 64bit")
        elif LINUX:
            self.ui.ch_posix32.setText("Linux 32bit")
            self.ui.ch_posix64.setText("Linux 64bit")
        elif MACOS:
            self.ui.ch_posix32.setText("MacOS 32bit")
            self.ui.ch_posix64.setText("MacOS 64bit")

        self.fThread = SearchPluginsThread(self)

        if carla_discovery_posix32 and not WINDOWS:
            self.ui.ico_posix32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_posix32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ui.ch_posix32.setChecked(False)
            self.ui.ch_posix32.setEnabled(False)

        if carla_discovery_posix64 and not WINDOWS:
            self.ui.ico_posix64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_posix64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ui.ch_posix64.setChecked(False)
            self.ui.ch_posix64.setEnabled(False)

        if carla_discovery_win32:
            self.ui.ico_win32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_win32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ui.ch_win32.setChecked(False)
            self.ui.ch_win32.setEnabled(False)

        if carla_discovery_win64:
            self.ui.ico_win64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_win64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ui.ch_win64.setChecked(False)
            self.ui.ch_win64.setEnabled(False)

        if haveLRDF:
            self.ui.ico_rdflib.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_rdflib.setPixmap(getIcon("dialog-error").pixmap(16, 16))

        hasNative = bool(carla_discovery_native)
        hasNonNative = False

        if WINDOWS:
            if kIs64bit:
                hasNative = bool(carla_discovery_win64)
                hasNonNative = bool(carla_discovery_win32)
                self.fThread.setSearchToolNative(carla_discovery_win64)
                self.ui.ch_win64.setChecked(False)
                self.ui.ch_win64.setVisible(False)
                self.ui.ico_win64.setVisible(False)
                self.ui.label_win64.setVisible(False)
            else:
                hasNative = bool(carla_discovery_win32)
                hasNonNative = bool(carla_discovery_win64)
                self.fThread.setSearchToolNative(carla_discovery_win32)
                self.ui.ch_win32.setChecked(False)
                self.ui.ch_win32.setVisible(False)
                self.ui.ico_win32.setVisible(False)
                self.ui.label_win32.setVisible(False)
        elif LINUX or MACOS:
            if kIs64bit:
                hasNonNative = bool(carla_discovery_posix32 or carla_discovery_win32 or carla_discovery_win64)
                self.ui.ch_posix64.setChecked(False)
                self.ui.ch_posix64.setVisible(False)
                self.ui.ico_posix64.setVisible(False)
                self.ui.label_posix64.setVisible(False)
            else:
                hasNonNative = bool(carla_discovery_posix64 or carla_discovery_win32 or carla_discovery_win64)
                self.ui.ch_posix32.setChecked(False)
                self.ui.ch_posix32.setVisible(False)
                self.ui.ico_posix32.setVisible(False)
                self.ui.label_posix32.setVisible(False)

        if hasNative:
            self.ui.ico_native.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ui.ico_native.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ui.ch_native.setChecked(False)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_gig.setChecked(False)
            self.ui.ch_gig.setEnabled(False)
            self.ui.ch_sf2.setChecked(False)
            self.ui.ch_sf2.setEnabled(False)
            self.ui.ch_sfz.setChecked(False)
            self.ui.ch_sfz.setEnabled(False)
            if not hasNonNative:
                self.ui.ch_ladspa.setChecked(False)
                self.ui.ch_ladspa.setEnabled(False)
                self.ui.ch_dssi.setChecked(False)
                self.ui.ch_dssi.setEnabled(False)
                self.ui.ch_lv2.setChecked(False)
                self.ui.ch_lv2.setEnabled(False)
                self.ui.ch_vst.setChecked(False)
                self.ui.ch_vst.setEnabled(False)
                self.ui.b_start.setEnabled(False)

        self.connect(self.ui.b_start, SIGNAL("clicked()"), SLOT("slot_start()"))
        self.connect(self.ui.b_skip, SIGNAL("clicked()"), SLOT("slot_skip()"))
        self.connect(self.fThread, SIGNAL("PluginLook(int, QString)"), SLOT("slot_handlePluginLook(int, QString)"))
        self.connect(self.fThread, SIGNAL("finished()"), SLOT("slot_handlePluginThreadFinished()"))

    @pyqtSlot()
    def slot_start(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(100)
        self.ui.progressBar.setValue(0)
        self.ui.b_start.setEnabled(False)
        self.ui.b_skip.setVisible(True)
        self.ui.b_close.setVisible(False)

        native, posix32, posix64, win32, win64 = (self.ui.ch_native.isChecked(), self.ui.ch_posix32.isChecked(), self.ui.ch_posix64.isChecked(), self.ui.ch_win32.isChecked(), self.ui.ch_win64.isChecked())
        ladspa, dssi, lv2, vst, gig, sf2, sfz  = (self.ui.ch_ladspa.isChecked(), self.ui.ch_dssi.isChecked(), self.ui.ch_lv2.isChecked(), self.ui.ch_vst.isChecked(),
                                                  self.ui.ch_gig.isChecked(), self.ui.ch_sf2.isChecked(), self.ui.ch_sfz.isChecked())

        self.fThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
        self.fThread.setSearchPluginTypes(ladspa, dssi, lv2, vst, gig, sf2, sfz)
        self.fThread.start()

    @pyqtSlot()
    def slot_skip(self):
        self.fThread.skipPlugin()

    @pyqtSlot(int, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.ui.progressBar.setFormat("%s" % plugin)
        self.ui.progressBar.setValue(percent)

    @pyqtSlot()
    def slot_handlePluginThreadFinished(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(1)
        self.ui.progressBar.setValue(1)
        self.ui.progressBar.setFormat(self.tr("Done"))
        self.ui.b_start.setEnabled(True)
        self.ui.b_skip.setVisible(False)
        self.ui.b_close.setVisible(True)

    def _saveSettings(self):
        settings = QSettings()
        settings.setValue("PluginDatabase/SearchLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/SearchDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/SearchLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/SearchVST", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/SearchGIG", self.ui.ch_gig.isChecked())
        settings.setValue("PluginDatabase/SearchSF2", self.ui.ch_sf2.isChecked())
        settings.setValue("PluginDatabase/SearchSFZ", self.ui.ch_sfz.isChecked())
        settings.setValue("PluginDatabase/SearchNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX32", self.ui.ch_posix32.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX64", self.ui.ch_posix64.isChecked())
        settings.setValue("PluginDatabase/SearchWin32", self.ui.ch_win32.isChecked())
        settings.setValue("PluginDatabase/SearchWin64", self.ui.ch_win64.isChecked())

    def _loadSettings(self):
        settings = QSettings()
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/SearchLADSPA", True, type=bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/SearchDSSI", True, type=bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/SearchLV2", True, type=bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/SearchVST", True, type=bool))
        self.ui.ch_gig.setChecked(settings.value("PluginDatabase/SearchGIG", False, type=bool))
        self.ui.ch_sf2.setChecked(settings.value("PluginDatabase/SearchSF2", False, type=bool))
        self.ui.ch_sfz.setChecked(settings.value("PluginDatabase/SearchSFZ", False, type=bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/SearchNative", True, type=bool))
        self.ui.ch_posix32.setChecked(settings.value("PluginDatabase/SearchPOSIX32", False, type=bool))
        self.ui.ch_posix64.setChecked(settings.value("PluginDatabase/SearchPOSIX64", False, type=bool))
        self.ui.ch_win32.setChecked(settings.value("PluginDatabase/SearchWin32", False, type=bool))
        self.ui.ch_win64.setChecked(settings.value("PluginDatabase/SearchWin64", False, type=bool))

    def closeEvent(self, event):
        if self.fThread.isRunning():
            self.fThread.terminate()
            self.fThread.wait()
        self._saveSettings()

        if self.fThread.somethingChanged():
            self.accept()
        else:
            self.reject()

        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Database Dialog

class PluginDatabaseW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_database.Ui_PluginDatabaseW()
        self.ui.setupUi(self)

        self.fLastTableIndex = 0
        self.fRetPlugin  = None
        self.fRealParent = parent

        self.ui.b_add.setEnabled(False)

        if BINARY_NATIVE in (BINARY_POSIX32, BINARY_WIN32):
            self.ui.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ui.ch_bridged.setText(self.tr("Bridged (32bit)"))

        if not (LINUX or MACOS):
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)

        self._loadSettings()

        self.connect(self.ui.b_add, SIGNAL("clicked()"), SLOT("slot_addPlugin()"))
        self.connect(self.ui.b_refresh, SIGNAL("clicked()"), SLOT("slot_refreshPlugins()"))
        self.connect(self.ui.tb_filters, SIGNAL("clicked()"), SLOT("slot_maybeShowFilters()"))
        self.connect(self.ui.tableWidget, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkPlugin(int)"))
        self.connect(self.ui.tableWidget, SIGNAL("cellDoubleClicked(int, int)"), SLOT("slot_addPlugin()"))

        self.connect(self.ui.lineEdit, SIGNAL("textChanged(QString)"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_effects, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_instruments, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_midi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_other, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_kits, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_internal, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_ladspa, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_dssi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_lv2, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_vst, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_native, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_bridged, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_bridged_wine, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_gui, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ui.ch_stereo, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))

    @pyqtSlot()
    def slot_addPlugin(self):
        if self.ui.tableWidget.currentRow() >= 0:
            self.fRetPlugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(), 0).pluginData
            self.accept()
        else:
            self.reject()

    @pyqtSlot(int)
    def slot_checkPlugin(self, row):
        self.ui.b_add.setEnabled(row >= 0)

    @pyqtSlot()
    def slot_checkFilters(self):
        self._checkFilters()

    @pyqtSlot()
    def slot_maybeShowFilters(self):
        self._showFilters(not self.ui.frame.isVisible())

    @pyqtSlot()
    def slot_refreshPlugins(self):
        if PluginRefreshW(self).exec_():
            self._reAddPlugins()

            if self.fRealParent:
                self.fRealParent.loadRDFs()

    def _checkFilters(self):
        text = self.ui.lineEdit.text().lower()

        hideEffects     = not self.ui.ch_effects.isChecked()
        hideInstruments = not self.ui.ch_instruments.isChecked()
        hideMidi        = not self.ui.ch_midi.isChecked()
        hideOther       = not self.ui.ch_other.isChecked()

        hideInternal = not self.ui.ch_internal.isChecked()
        hideLadspa   = not self.ui.ch_ladspa.isChecked()
        hideDssi     = not self.ui.ch_dssi.isChecked()
        hideLV2      = not self.ui.ch_lv2.isChecked()
        hideVST      = not self.ui.ch_vst.isChecked()
        hideKits     = not self.ui.ch_kits.isChecked()

        hideNative  = not self.ui.ch_native.isChecked()
        hideBridged = not self.ui.ch_bridged.isChecked()
        hideBridgedWine = not self.ui.ch_bridged_wine.isChecked()

        hideNonGui    = self.ui.ch_gui.isChecked()
        hideNonStereo = self.ui.ch_stereo.isChecked()

        if HAIKU or LINUX or MACOS:
            nativeBins = [BINARY_POSIX32, BINARY_POSIX64]
            wineBins   = [BINARY_WIN32, BINARY_WIN64]
        elif WINDOWS:
            nativeBins = [BINARY_WIN32, BINARY_WIN64]
            wineBins   = []
        else:
            nativeBins = []
            wineBins   = []

        rowCount = self.ui.tableWidget.rowCount()

        for i in range(rowCount):
            self.ui.tableWidget.showRow(i)

            plugin = self.ui.tableWidget.item(i, 0).pluginData
            aIns   = plugin['audio.ins']
            aOuts  = plugin['audio.outs']
            mIns   = plugin['midi.ins']
            mOuts  = plugin['midi.outs']
            ptype  = self.ui.tableWidget.item(i, 12).text()
            isSynth  = bool(plugin['hints'] & PLUGIN_IS_SYNTH)
            isEffect = bool(aIns > 0 < aOuts and not isSynth)
            isMidi   = bool(aIns == 0 and aOuts == 0 and mIns > 0 < mOuts)
            isKit    = bool(ptype in ("GIG", "SF2", "SFZ"))
            isOther  = bool(not (isEffect or isSynth or isMidi or isKit))
            isNative = bool(plugin['build'] == BINARY_NATIVE)
            isStereo = bool(aIns == 2 and aOuts == 2) or (isSynth and aOuts == 2)
            hasGui   = bool(plugin['hints'] & PLUGIN_HAS_GUI)

            isBridged = bool(not isNative and plugin['build'] in nativeBins)
            isBridgedWine = bool(not isNative and plugin['build'] in wineBins)

            if (hideEffects and isEffect):
                self.ui.tableWidget.hideRow(i)
            elif (hideInstruments and isSynth):
                self.ui.tableWidget.hideRow(i)
            elif (hideMidi and isMidi):
                self.ui.tableWidget.hideRow(i)
            elif (hideOther and isOther):
                self.ui.tableWidget.hideRow(i)
            elif (hideKits and isKit):
                self.ui.tableWidget.hideRow(i)
            elif (hideInternal and ptype == self.tr("Internal")):
                self.ui.tableWidget.hideRow(i)
            elif (hideLadspa and ptype == "LADSPA"):
                self.ui.tableWidget.hideRow(i)
            elif (hideDssi and ptype == "DSSI"):
                self.ui.tableWidget.hideRow(i)
            elif (hideLV2 and ptype == "LV2"):
                self.ui.tableWidget.hideRow(i)
            elif (hideVST and ptype == "VST"):
                self.ui.tableWidget.hideRow(i)
            elif (hideNative and isNative):
                self.ui.tableWidget.hideRow(i)
            elif (hideBridged and isBridged):
                self.ui.tableWidget.hideRow(i)
            elif (hideBridgedWine and isBridgedWine):
                self.ui.tableWidget.hideRow(i)
            elif (hideNonGui and not hasGui):
                self.ui.tableWidget.hideRow(i)
            elif (hideNonStereo and not isStereo):
                self.ui.tableWidget.hideRow(i)
            elif (text and not (
                text in self.ui.tableWidget.item(i, 0).text().lower() or
                text in self.ui.tableWidget.item(i, 1).text().lower() or
                text in self.ui.tableWidget.item(i, 2).text().lower() or
                text in self.ui.tableWidget.item(i, 3).text().lower() or
                text in self.ui.tableWidget.item(i, 13).text().lower())):
                self.ui.tableWidget.hideRow(i)

    def _showFilters(self, yesNo):
        self.ui.tb_filters.setArrowType(Qt.UpArrow if yesNo else Qt.DownArrow)
        self.ui.frame.setVisible(yesNo)

    def _reAddPlugins(self):
        settingsDB = QSettings("falkTX", "CarlaPlugins")

        rowCount = self.ui.tableWidget.rowCount()
        for x in range(rowCount):
            self.ui.tableWidget.removeRow(0)

        self.fLastTableIndex = 0
        self.ui.tableWidget.setSortingEnabled(False)

        internalCount = 0
        ladspaCount = 0
        dssiCount = 0
        lv2Count = 0
        vstCount = 0
        kitCount = 0

        # ---------------------------------------------------------------------------
        # Internal

        internalPlugins = toList(settingsDB.value("Plugins/Internal", []))

        for plugins in internalPlugins:
            for plugin in plugins:
                internalCount += 1

        if (not Carla.isControl) and internalCount != Carla.host.get_internal_plugin_count():
            internalCount   = Carla.host.get_internal_plugin_count()
            internalPlugins = []

            for i in range(Carla.host.get_internal_plugin_count()):
                descInfo = Carla.host.get_internal_plugin_info(i)
                plugins  = checkPluginInternal(descInfo)

                if plugins:
                    internalPlugins.append(plugins)

            settingsDB.setValue("Plugins/Internal", internalPlugins)

        for plugins in internalPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, self.tr("Internal"))

        # ---------------------------------------------------------------------------
        # LADSPA

        ladspaPlugins  = []
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_native", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_posix32", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_posix64", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_win32", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_win64", []))

        for plugins in ladspaPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LADSPA")
                ladspaCount += 1

        # ---------------------------------------------------------------------------
        # DSSI

        dssiPlugins  = []
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_native", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_posix32", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_posix64", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_win32", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_win64", []))

        for plugins in dssiPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "DSSI")
                dssiCount += 1

        # ---------------------------------------------------------------------------
        # LV2

        lv2Plugins  = []
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_native", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_posix32", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_posix64", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_win32", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_win64", []))

        for plugins in lv2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LV2")
                lv2Count += 1

        # ---------------------------------------------------------------------------
        # VST

        vstPlugins  = []
        vstPlugins += toList(settingsDB.value("Plugins/VST_native", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_posix32", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_posix64", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_win32", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_win64", []))

        for plugins in vstPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST")
                vstCount += 1

        # ---------------------------------------------------------------------------
        # Kits

        gigs = toList(settingsDB.value("Plugins/GIG", []))
        sf2s = toList(settingsDB.value("Plugins/SF2", []))
        sfzs = toList(settingsDB.value("Plugins/SFZ", []))

        for gig in gigs:
            for gig_i in gig:
                self._addPluginToTable(gig_i, "GIG")
                kitCount += 1

        for sf2 in sf2s:
            for sf2_i in sf2:
                self._addPluginToTable(sf2_i, "SF2")
                kitCount += 1

        for sfz in sfzs:
            for sfz_i in sfz:
                self._addPluginToTable(sfz_i, "SFZ")
                kitCount += 1

        # ---------------------------------------------------------------------------

        self.ui.tableWidget.setSortingEnabled(True)
        self.ui.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST and %i Sound Kits" % (internalCount, ladspaCount, dssiCount, lv2Count, vstCount, kitCount)))

        self._checkFilters()

    def _addPluginToTable(self, plugin, ptype):
        index = self.fLastTableIndex

        if plugin['build'] == BINARY_NATIVE:
            bridgeText = self.tr("No")
        else:
            typeText = self.tr("Unknown")
            if LINUX or MACOS:
                if plugin['build'] == BINARY_POSIX32:
                    typeText = "32bit"
                elif plugin['build'] == BINARY_POSIX64:
                    typeText = "64bit"
                elif plugin['build'] == BINARY_WIN32:
                    typeText = "Windows 32bit"
                elif plugin['build'] == BINARY_WIN64:
                    typeText = "Windows 64bit"
            elif WINDOWS:
                if plugin['build'] == BINARY_WIN32:
                    typeText = "32bit"
                elif plugin['build'] == BINARY_WIN64:
                    typeText = "64bit"
            bridgeText = self.tr("Yes (%s)" % typeText)

        self.ui.tableWidget.insertRow(index)
        self.ui.tableWidget.setItem(index, 0, QTableWidgetItem(plugin['name']))
        self.ui.tableWidget.setItem(index, 1, QTableWidgetItem(plugin['label']))
        self.ui.tableWidget.setItem(index, 2, QTableWidgetItem(plugin['maker']))
        self.ui.tableWidget.setItem(index, 3, QTableWidgetItem(str(plugin['uniqueId'])))
        self.ui.tableWidget.setItem(index, 4, QTableWidgetItem(str(plugin['audio.ins'])))
        self.ui.tableWidget.setItem(index, 5, QTableWidgetItem(str(plugin['audio.outs'])))
        self.ui.tableWidget.setItem(index, 6, QTableWidgetItem(str(plugin['parameters.ins'])))
        self.ui.tableWidget.setItem(index, 7, QTableWidgetItem(str(plugin['parameters.outs'])))
        self.ui.tableWidget.setItem(index, 8, QTableWidgetItem(str(plugin['programs.total'])))
        self.ui.tableWidget.setItem(index, 9, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_HAS_GUI) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 10, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_IS_SYNTH) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 11, QTableWidgetItem(bridgeText))
        self.ui.tableWidget.setItem(index, 12, QTableWidgetItem(ptype))
        self.ui.tableWidget.setItem(index, 13, QTableWidgetItem(plugin['binary']))
        self.ui.tableWidget.item(self.fLastTableIndex, 0).pluginData = plugin
        self.fLastTableIndex += 1

    def _saveSettings(self):
        settings = QSettings()
        settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        settings.setValue("PluginDatabase/TableGeometry", self.ui.tableWidget.horizontalHeader().saveState())
        settings.setValue("PluginDatabase/ShowFilters", (self.ui.tb_filters.arrowType() == Qt.UpArrow))
        settings.setValue("PluginDatabase/ShowEffects", self.ui.ch_effects.isChecked())
        settings.setValue("PluginDatabase/ShowInstruments", self.ui.ch_instruments.isChecked())
        settings.setValue("PluginDatabase/ShowMIDI", self.ui.ch_midi.isChecked())
        settings.setValue("PluginDatabase/ShowOther", self.ui.ch_other.isChecked())
        settings.setValue("PluginDatabase/ShowInternal", self.ui.ch_internal.isChecked())
        settings.setValue("PluginDatabase/ShowLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/ShowDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/ShowLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/ShowVST", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits.isChecked())
        settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged.isChecked())
        settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine.isChecked())
        settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui.isChecked())
        settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo.isChecked())

    def _loadSettings(self):
        settings = QSettings()
        self.restoreGeometry(settings.value("PluginDatabase/Geometry", ""))
        self.ui.tableWidget.horizontalHeader().restoreState(settings.value("PluginDatabase/TableGeometry", ""))
        self.ui.ch_effects.setChecked(settings.value("PluginDatabase/ShowEffects", True, type=bool))
        self.ui.ch_instruments.setChecked(settings.value("PluginDatabase/ShowInstruments", True, type=bool))
        self.ui.ch_midi.setChecked(settings.value("PluginDatabase/ShowMIDI", True, type=bool))
        self.ui.ch_other.setChecked(settings.value("PluginDatabase/ShowOther", True, type=bool))
        self.ui.ch_internal.setChecked(settings.value("PluginDatabase/ShowInternal", True, type=bool))
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/ShowLADSPA", True, type=bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/ShowDSSI", True, type=bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/ShowLV2", True, type=bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/ShowVST", True, type=bool))
        self.ui.ch_kits.setChecked(settings.value("PluginDatabase/ShowKits", True, type=bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/ShowNative", True, type=bool))
        self.ui.ch_bridged.setChecked(settings.value("PluginDatabase/ShowBridged", True, type=bool))
        self.ui.ch_bridged_wine.setChecked(settings.value("PluginDatabase/ShowBridgedWine", True, type=bool))
        self.ui.ch_gui.setChecked(settings.value("PluginDatabase/ShowHasGUI", False, type=bool))
        self.ui.ch_stereo.setChecked(settings.value("PluginDatabase/ShowStereoOnly", False, type=bool))

        self._showFilters(settings.value("PluginDatabase/ShowFilters", False, type=bool))
        self._reAddPlugins()

    def closeEvent(self, event):
        self._saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# TESTING

#from PyQt4.QtGui import QApplication

#Carla.isControl = True

#ptest = {
    #'index': 0,
    #'name': "Parameter",
    #'symbol': "param",
    #'current': 0.1,
    #'default': 0.3,
    #'minimum': 0.0,
    #'maximum': 1.0,
    #'midiChannel': 7,
    #'midiCC': 2,
    #'type': PARAMETER_INPUT,
    #'hints': PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE,
    #'scalePoints': [],
    #'step': 0.01,
    #'stepSmall': 0.001,
    #'stepLarge': 0.1,
    #'unit': "un",
#}

#app  = QApplication(sys.argv)
#app.setApplicationName("Carla")
#app.setApplicationVersion(VERSION)
#app.setOrganizationName("falkTX")
##gui = CarlaAboutW(None)
##gui = PluginParameter(None, ptest, 0, 0)
#gui = PluginEdit(None, 0)
##gui = PluginWidget(None, 0)
##gui = PluginDatabaseW(None)
#gui.show()
##if gui.exec_():
    ##print(gui.fRetPlugin)
##gui = PluginRefreshW(None)
#app.exec_()
