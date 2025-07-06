#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
import sys

from math import fmod

# ------------------------------------------------------------------------------------------------------------
# Imports (Signal)

from signal import signal, SIGINT, SIGTERM

try:
    from signal import SIGUSR1
    haveSIGUSR1 = True
except:
    haveSIGUSR1 = False

# ------------------------------------------------------------------------------------------------------------
# Imports (PyQt)

from qt_compat import qt_config

# pylint: disable=import-error

if qt_config == 5:
    # import changed in PyQt 5.15.8, so try both
    try:
        from PyQt5.Qt import PYQT_VERSION_STR
    except ImportError:
        from PyQt5.QtCore import PYQT_VERSION_STR

    from PyQt5.QtCore import qFatal, QT_VERSION, QT_VERSION_STR, qWarning, QDir, QSettings
    from PyQt5.QtGui import QIcon
    from PyQt5.QtWidgets import QFileDialog, QMessageBox

elif qt_config == 6:
    from PyQt6.QtCore import qFatal, PYQT_VERSION_STR, QT_VERSION, QT_VERSION_STR, qWarning, QDir, QSettings
    from PyQt6.QtGui import QIcon
    from PyQt6.QtWidgets import QFileDialog, QMessageBox

# pylint: enable=import-error
# pylint: disable=possibly-used-before-assignment

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import (
    CARLA_OS_64BIT,
    CARLA_OS_HAIKU,
    CARLA_OS_MAC,
    CARLA_OS_WIN,
    CARLA_VERSION_STRING,
    MAX_DEFAULT_PARAMETERS,
    ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS,
    ENGINE_PROCESS_MODE_PATCHBAY,
    ENGINE_TRANSPORT_MODE_INTERNAL,
    ENGINE_TRANSPORT_MODE_JACK,
)

# ------------------------------------------------------------------------------------------------------------
# Config

# These will be modified during install
X_LIBDIR_X = None
X_DATADIR_X = None

# ------------------------------------------------------------------------------------------------------------
# Platform specific stuff

if CARLA_OS_WIN:
    WINDIR = os.getenv("WINDIR")

# ------------------------------------------------------------------------------------------------------------
# Set TMP

envTMP = os.getenv("TMP")

if envTMP is None:
    if CARLA_OS_WIN:
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
    if not CARLA_OS_WIN:
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
    if CARLA_OS_MAC:
        PATH = ("/opt/local/bin", "/usr/local/bin", "/usr/bin", "/bin")
    elif CARLA_OS_WIN:
        PATH = (os.path.join(WINDIR, "system32"), WINDIR)
    else:
        PATH = ("/usr/local/bin", "/usr/bin", "/bin")
else:
    PATH = envPATH.split(os.pathsep)

del envPATH

# ------------------------------------------------------------------------------------------------------------
# Static MIDI CC list

MIDI_CC_LIST = (
    "01 [0x01] Modulation",
    "02 [0x02] Breath",
    "04 [0x04] Foot",
    "05 [0x05] Portamento",
    "07 [0x07] Volume",
    "08 [0x08] Balance",
    "10 [0x0A] Pan",
    "11 [0x0B] Expression",
    "12 [0x0C] FX Control 1",
    "13 [0x0D] FX Control 2",
    "16 [0x10] General Purpose 1",
    "17 [0x11] General Purpose 2",
    "18 [0x12] General Purpose 3",
    "19 [0x13] General Purpose 4",
    "70 [0x46] Control 1 [Variation]",
    "71 [0x47] Control 2 [Timbre]",
    "72 [0x48] Control 3 [Release]",
    "73 [0x49] Control 4 [Attack]",
    "74 [0x4A] Control 5 [Brightness]",
    "75 [0x4B] Control 6 [Decay]",
    "76 [0x4C] Control 7 [Vib Rate]",
    "77 [0x4D] Control 8 [Vib Depth]",
    "78 [0x4E] Control 9 [Vib Delay]",
    "79 [0x4F] Control 10 [Undefined]",
    "80 [0x50] General Purpose 5",
    "81 [0x51] General Purpose 6",
    "82 [0x52] General Purpose 7",
    "83 [0x53] General Purpose 8",
    "84 [0x54] Portamento Control",
    "91 [0x5B] FX 1 Depth [Reverb]",
    "92 [0x5C] FX 2 Depth [Tremolo]",
    "93 [0x5D] FX 3 Depth [Chorus]",
    "94 [0x5E] FX 4 Depth [Detune]",
    "95 [0x5F] FX 5 Depth [Phaser]"
)

MAX_MIDI_CC_LIST_ITEM = 95

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
CARLA_KEY_MAIN_CONFIRM_EXIT     = "Main/ConfirmExit"     # bool
CARLA_KEY_MAIN_CLASSIC_SKIN     = "Main/ClassicSkin"     # bool
CARLA_KEY_MAIN_SHOW_LOGS        = "Main/ShowLogs"        # bool
CARLA_KEY_MAIN_SYSTEM_ICONS     = "Main/SystemIcons"     # bool
CARLA_KEY_MAIN_EXPERIMENTAL     = "Main/Experimental"    # bool

CARLA_KEY_CANVAS_THEME             = "Canvas/Theme"           # str
CARLA_KEY_CANVAS_SIZE              = "Canvas/Size"            # str "NxN"
CARLA_KEY_CANVAS_USE_BEZIER_LINES  = "Canvas/UseBezierLines"  # bool
CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS  = "Canvas/AutoHideGroups"  # bool
CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS = "Canvas/AutoSelectItems" # bool
CARLA_KEY_CANVAS_EYE_CANDY         = "Canvas/EyeCandy2"       # bool
CARLA_KEY_CANVAS_FANCY_EYE_CANDY   = "Canvas/FancyEyeCandy"   # bool
CARLA_KEY_CANVAS_USE_OPENGL        = "Canvas/UseOpenGL"       # bool
CARLA_KEY_CANVAS_ANTIALIASING      = "Canvas/Antialiasing"    # enum
CARLA_KEY_CANVAS_HQ_ANTIALIASING   = "Canvas/HQAntialiasing"  # bool
CARLA_KEY_CANVAS_INLINE_DISPLAYS   = "Canvas/InlineDisplays"  # bool
CARLA_KEY_CANVAS_FULL_REPAINTS     = "Canvas/FullRepaints"    # bool

CARLA_KEY_ENGINE_DRIVER_PREFIX         = "Engine/Driver-"
CARLA_KEY_ENGINE_AUDIO_DRIVER          = "Engine/AudioDriver"         # str
CARLA_KEY_ENGINE_AUDIO_DEVICE          = "Engine/AudioDevice"         # str
CARLA_KEY_ENGINE_BUFFER_SIZE           = "Engine/BufferSize"          # int
CARLA_KEY_ENGINE_SAMPLE_RATE           = "Engine/SampleRate"          # int
CARLA_KEY_ENGINE_TRIPLE_BUFFER         = "Engine/TripleBuffer"        # bool
CARLA_KEY_ENGINE_PROCESS_MODE          = "Engine/ProcessMode"         # enum
CARLA_KEY_ENGINE_TRANSPORT_MODE        = "Engine/TransportMode"       # enum
CARLA_KEY_ENGINE_TRANSPORT_EXTRA       = "Engine/TransportExtra"      # str
CARLA_KEY_ENGINE_FORCE_STEREO          = "Engine/ForceStereo"         # bool
CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES = "Engine/PreferPluginBridges" # bool
CARLA_KEY_ENGINE_PREFER_UI_BRIDGES     = "Engine/PreferUiBridges"     # bool
CARLA_KEY_ENGINE_MANAGE_UIS            = "Engine/ManageUIs"           # bool
CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP     = "Engine/UIsAlwaysOnTop"      # bool
CARLA_KEY_ENGINE_MAX_PARAMETERS        = "Engine/MaxParameters"       # int
CARLA_KEY_ENGINE_RESET_XRUNS           = "Engine/ResetXruns"          # bool
CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT    = "Engine/UiBridgesTimeout"    # int

CARLA_KEY_OSC_ENABLED          = "OSC/Enabled"
CARLA_KEY_OSC_TCP_PORT_ENABLED = "OSC/TCPEnabled"
CARLA_KEY_OSC_TCP_PORT_NUMBER  = "OSC/TCPNumber"
CARLA_KEY_OSC_TCP_PORT_RANDOM  = "OSC/TCPRandom"
CARLA_KEY_OSC_UDP_PORT_ENABLED = "OSC/UDPEnabled"
CARLA_KEY_OSC_UDP_PORT_NUMBER  = "OSC/UDPNumber"
CARLA_KEY_OSC_UDP_PORT_RANDOM  = "OSC/UDPRandom"

CARLA_KEY_PATHS_AUDIO = "Paths/Audio"
CARLA_KEY_PATHS_MIDI  = "Paths/MIDI"

CARLA_KEY_PATHS_LADSPA = "Paths/LADSPA"
CARLA_KEY_PATHS_DSSI   = "Paths/DSSI"
CARLA_KEY_PATHS_LV2    = "Paths/LV2"
CARLA_KEY_PATHS_VST2   = "Paths/VST2"
CARLA_KEY_PATHS_VST3   = "Paths/VST3"
CARLA_KEY_PATHS_CLAP   = "Paths/CLAP"
CARLA_KEY_PATHS_SF2    = "Paths/SF2"
CARLA_KEY_PATHS_SFZ    = "Paths/SFZ"
CARLA_KEY_PATHS_JSFX   = "Paths/JSFX"

CARLA_KEY_WINE_EXECUTABLE      = "Wine/Executable"     # str
CARLA_KEY_WINE_AUTO_PREFIX     = "Wine/AutoPrefix"     # bool
CARLA_KEY_WINE_FALLBACK_PREFIX = "Wine/FallbackPrefix" # str
CARLA_KEY_WINE_RT_PRIO_ENABLED = "Wine/RtPrioEnabled"  # bool
CARLA_KEY_WINE_BASE_RT_PRIO    = "Wine/BaseRtPrio"     # int
CARLA_KEY_WINE_SERVER_RT_PRIO  = "Wine/ServerRtPrio"   # int

CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES        = "Experimental/PluginBridges"       # bool
CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES          = "Experimental/WineBridges"         # bool
CARLA_KEY_EXPERIMENTAL_JACK_APPS             = "Experimental/JackApplications"    # bool
CARLA_KEY_EXPERIMENTAL_EXPORT_LV2            = "Experimental/ExportLV2"           # bool
CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR = "Experimental/PreventBadBehaviour" # bool
CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL       = "Experimental/LoadLibGlobal"       # bool

# if pro theme is on and color is black
CARLA_KEY_CUSTOM_PAINTING = "UseCustomPainting" # bool

# ------------------------------------------------------------------------------------------------------------
# Carla Settings defaults

# Main
CARLA_DEFAULT_MAIN_PROJECT_FOLDER   = HOME
CARLA_DEFAULT_MAIN_USE_PRO_THEME    = True
CARLA_DEFAULT_MAIN_PRO_THEME_COLOR  = "Black"
CARLA_DEFAULT_MAIN_REFRESH_INTERVAL = 20
CARLA_DEFAULT_MAIN_CONFIRM_EXIT     = True
CARLA_DEFAULT_MAIN_CLASSIC_SKIN     = False
CARLA_DEFAULT_MAIN_SHOW_LOGS        = bool(not CARLA_OS_WIN)
CARLA_DEFAULT_MAIN_SYSTEM_ICONS     = False
CARLA_DEFAULT_MAIN_EXPERIMENTAL     = False

# Canvas
CARLA_DEFAULT_CANVAS_THEME             = "Modern Dark"
CARLA_DEFAULT_CANVAS_SIZE              = "3100x2400"
CARLA_DEFAULT_CANVAS_SIZE_WIDTH        = 3100
CARLA_DEFAULT_CANVAS_SIZE_HEIGHT       = 2400
CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES  = True
CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS  = True
CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS = False
CARLA_DEFAULT_CANVAS_EYE_CANDY         = True
CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY   = False
CARLA_DEFAULT_CANVAS_USE_OPENGL        = False
CARLA_DEFAULT_CANVAS_ANTIALIASING      = CANVAS_ANTIALIASING_SMALL
CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING   = False
CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS   = False
CARLA_DEFAULT_CANVAS_FULL_REPAINTS     = False

# Engine
CARLA_DEFAULT_FORCE_STEREO          = False
CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES = False
CARLA_DEFAULT_PREFER_UI_BRIDGES     = True
CARLA_DEFAULT_MANAGE_UIS            = True
CARLA_DEFAULT_UIS_ALWAYS_ON_TOP     = False
CARLA_DEFAULT_MAX_PARAMETERS        = MAX_DEFAULT_PARAMETERS
CARLA_DEFAULT_RESET_XRUNS           = False
CARLA_DEFAULT_UI_BRIDGES_TIMEOUT    = 4000

CARLA_DEFAULT_AUDIO_BUFFER_SIZE     = 512
CARLA_DEFAULT_AUDIO_SAMPLE_RATE     = 44100
CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER   = False

if CARLA_OS_HAIKU:
    CARLA_DEFAULT_AUDIO_DRIVER = "SDL"
elif CARLA_OS_MAC:
    CARLA_DEFAULT_AUDIO_DRIVER = "CoreAudio"
elif CARLA_OS_WIN:
    CARLA_DEFAULT_AUDIO_DRIVER = "Windows Audio"
elif os.path.exists("/usr/bin/jackd") or os.path.exists("/usr/bin/jackdbus") or os.path.exists("/usr/bin/pw-jack"):
    CARLA_DEFAULT_AUDIO_DRIVER = "JACK"
else:
    CARLA_DEFAULT_AUDIO_DRIVER = "PulseAudio"

if CARLA_DEFAULT_AUDIO_DRIVER == "JACK":
    CARLA_DEFAULT_PROCESS_MODE   = ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS
    CARLA_DEFAULT_TRANSPORT_MODE = ENGINE_TRANSPORT_MODE_JACK
else:
    CARLA_DEFAULT_PROCESS_MODE   = ENGINE_PROCESS_MODE_PATCHBAY
    CARLA_DEFAULT_TRANSPORT_MODE = ENGINE_TRANSPORT_MODE_INTERNAL

# OSC
CARLA_DEFAULT_OSC_ENABLED = not (CARLA_OS_MAC or CARLA_OS_WIN)
CARLA_DEFAULT_OSC_TCP_PORT_ENABLED = True
CARLA_DEFAULT_OSC_TCP_PORT_NUMBER  = 22752
CARLA_DEFAULT_OSC_TCP_PORT_RANDOM  = False
CARLA_DEFAULT_OSC_UDP_PORT_ENABLED = True
CARLA_DEFAULT_OSC_UDP_PORT_NUMBER  = 22752
CARLA_DEFAULT_OSC_UDP_PORT_RANDOM  = False

# Wine
CARLA_DEFAULT_WINE_EXECUTABLE      = "wine"
CARLA_DEFAULT_WINE_AUTO_PREFIX     = True
CARLA_DEFAULT_WINE_FALLBACK_PREFIX = os.path.expanduser("~/.wine")
CARLA_DEFAULT_WINE_RT_PRIO_ENABLED = True
CARLA_DEFAULT_WINE_BASE_RT_PRIO    = 15
CARLA_DEFAULT_WINE_SERVER_RT_PRIO  = 10

# Experimental
CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES        = False
CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES          = False
CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS             = False
CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT            = False
CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR = False
CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL       = False

# ------------------------------------------------------------------------------------------------------------
# Default File Folders

CARLA_DEFAULT_FILE_PATH_AUDIO = []
CARLA_DEFAULT_FILE_PATH_MIDI  = []

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (get)

DEFAULT_LADSPA_PATH = ""
DEFAULT_DSSI_PATH   = ""
DEFAULT_LV2_PATH    = ""
DEFAULT_VST2_PATH   = ""
DEFAULT_VST3_PATH   = ""
DEFAULT_CLAP_PATH   = ""
DEFAULT_SF2_PATH    = ""
DEFAULT_SFZ_PATH    = ""
DEFAULT_JSFX_PATH   = ""

if CARLA_OS_WIN:
    splitter = ";"

    APPDATA = os.getenv("APPDATA")
    LOCALAPPDATA = os.getenv("LOCALAPPDATA", APPDATA)
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

    DEFAULT_JSFX_PATH    = APPDATA + "\\REAPER\\Effects"
    #DEFAULT_JSFX_PATH   += ";" + PROGRAMFILES + "\\REAPER\\InstallData\\Effects"

    if CARLA_OS_64BIT:
        DEFAULT_VST2_PATH  += ";" + COMMONPROGRAMFILES + "\\VST2"

    DEFAULT_VST3_PATH    = COMMONPROGRAMFILES + "\\VST3"
    DEFAULT_VST3_PATH   += ";" + LOCALAPPDATA + "\\Programs\\Common\\VST3"

    DEFAULT_CLAP_PATH    = COMMONPROGRAMFILES + "\\CLAP"
    DEFAULT_CLAP_PATH   += ";" + LOCALAPPDATA + "\\Programs\\Common\\CLAP"

    DEFAULT_SF2_PATH     = APPDATA + "\\SF2"
    DEFAULT_SFZ_PATH     = APPDATA + "\\SFZ"

    if PROGRAMFILESx86:
        DEFAULT_LADSPA_PATH += ";" + PROGRAMFILESx86 + "\\LADSPA"
        DEFAULT_DSSI_PATH   += ";" + PROGRAMFILESx86 + "\\DSSI"
        DEFAULT_VST2_PATH   += ";" + PROGRAMFILESx86 + "\\VstPlugins"
        DEFAULT_VST2_PATH   += ";" + PROGRAMFILESx86 + "\\Steinberg\\VstPlugins"
        #DEFAULT_JSFX_PATH   += ";" + PROGRAMFILESx86 + "\\REAPER\\InstallData\\Effects"

    if COMMONPROGRAMFILESx86:
        DEFAULT_VST3_PATH   += COMMONPROGRAMFILESx86 + "\\VST3"
        DEFAULT_CLAP_PATH   += COMMONPROGRAMFILESx86 + "\\CLAP"

elif CARLA_OS_HAIKU:
    splitter = ":"

    DEFAULT_LADSPA_PATH  = HOME + "/.ladspa"
    DEFAULT_LADSPA_PATH += ":/system/add-ons/media/ladspaplugins"
    DEFAULT_LADSPA_PATH += ":/system/lib/ladspa"

    DEFAULT_DSSI_PATH    = HOME + "/.dssi"
    DEFAULT_DSSI_PATH   += ":/system/add-ons/media/dssiplugins"
    DEFAULT_DSSI_PATH   += ":/system/lib/dssi"

    DEFAULT_LV2_PATH     = HOME + "/.lv2"
    DEFAULT_LV2_PATH    += ":/system/add-ons/media/lv2plugins"

    DEFAULT_VST2_PATH    = HOME + "/.vst"
    DEFAULT_VST2_PATH   += ":/system/add-ons/media/vstplugins"

    DEFAULT_VST3_PATH    = HOME + "/.vst3"
    DEFAULT_VST3_PATH   += ":/system/add-ons/media/vst3plugins"

    DEFAULT_CLAP_PATH    = HOME + "/.clap"
    DEFAULT_CLAP_PATH   += ":/system/add-ons/media/clapplugins"

elif CARLA_OS_MAC:
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

    DEFAULT_CLAP_PATH    = HOME + "/Library/Audio/Plug-Ins/CLAP"
    DEFAULT_CLAP_PATH   += ":/Library/Audio/Plug-Ins/CLAP"

    DEFAULT_JSFX_PATH    = HOME + "/Library/Application Support/REAPER/Effects"
    #DEFAULT_JSFX_PATH   += ":/Applications/REAPER.app/Contents/InstallFiles/Effects"

else:
    splitter = ":"

    CONFIG_HOME = os.getenv("XDG_CONFIG_HOME", HOME + "/.config")

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

    DEFAULT_VST2_PATH   += HOME + "/.lxvst"
    DEFAULT_VST2_PATH   += ":/usr/lib/lxvst"
    DEFAULT_VST2_PATH   += ":/usr/local/lib/lxvst"

    DEFAULT_VST3_PATH    = HOME + "/.vst3"
    DEFAULT_VST3_PATH   += ":/usr/lib/vst3"
    DEFAULT_VST3_PATH   += ":/usr/local/lib/vst3"

    DEFAULT_CLAP_PATH    = HOME + "/.clap"
    DEFAULT_CLAP_PATH   += ":/usr/lib/clap"
    DEFAULT_CLAP_PATH   += ":/usr/local/lib/clap"

    DEFAULT_SF2_PATH     = HOME + "/.sounds/sf2"
    DEFAULT_SF2_PATH    += ":" + HOME + "/.sounds/sf3"
    DEFAULT_SF2_PATH    += ":/usr/share/sounds/sf2"
    DEFAULT_SF2_PATH    += ":/usr/share/sounds/sf3"
    DEFAULT_SF2_PATH    += ":/usr/share/soundfonts"

    DEFAULT_SFZ_PATH     = HOME + "/.sounds/sfz"
    DEFAULT_SFZ_PATH    += ":/usr/share/sounds/sfz"

    DEFAULT_JSFX_PATH    = CONFIG_HOME + "/REAPER/Effects"
    #DEFAULT_JSFX_PATH   += ":" + "/opt/REAPER/InstallData/Effects"

if not CARLA_OS_WIN:
    winePrefix = os.getenv("WINEPREFIX")

    if not winePrefix:
        winePrefix = HOME + "/.wine"

    DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/VstPlugins"
    DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/VSTPlugins"
    DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/Steinberg/VstPlugins"
    DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/Steinberg/VSTPlugins"
    DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST2"
    DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST3"
    DEFAULT_CLAP_PATH += ":" + winePrefix + "/drive_c/Program Files/Common Files/CLAP"

    if CARLA_OS_64BIT:
        DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/VstPlugins"
        DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/VSTPlugins"
        DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Steinberg/VstPlugins"
        DEFAULT_VST2_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Steinberg/VSTPlugins"
        DEFAULT_VST3_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST3"
        DEFAULT_CLAP_PATH += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/CLAP"

    del winePrefix

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (set)

readEnvVars = True

if CARLA_OS_WIN:
    # Check if running Wine. If yes, ignore env vars
    # pylint: disable=import-error
    from winreg import ConnectRegistry, OpenKey, CloseKey, HKEY_CURRENT_USER
    # pylint: enable=import-error
    _reg = ConnectRegistry(None, HKEY_CURRENT_USER)

    try:
        _key = OpenKey(_reg, r"SOFTWARE\Wine")
        CloseKey(_key)
        del _key
        readEnvVars = False
    except:
        pass

    CloseKey(_reg)
    del _reg

if readEnvVars:
    CARLA_DEFAULT_LADSPA_PATH = os.getenv("LADSPA_PATH", DEFAULT_LADSPA_PATH).split(splitter)
    CARLA_DEFAULT_DSSI_PATH   = os.getenv("DSSI_PATH",   DEFAULT_DSSI_PATH).split(splitter)
    CARLA_DEFAULT_LV2_PATH    = os.getenv("LV2_PATH",    DEFAULT_LV2_PATH).split(splitter)
    CARLA_DEFAULT_VST2_PATH   = os.getenv("VST_PATH",    DEFAULT_VST2_PATH).split(splitter)
    CARLA_DEFAULT_VST3_PATH   = os.getenv("VST3_PATH",   DEFAULT_VST3_PATH).split(splitter)
    CARLA_DEFAULT_CLAP_PATH   = os.getenv("CLAP_PATH",   DEFAULT_CLAP_PATH).split(splitter)
    CARLA_DEFAULT_SF2_PATH    = os.getenv("SF2_PATH",    DEFAULT_SF2_PATH).split(splitter)
    CARLA_DEFAULT_SFZ_PATH    = os.getenv("SFZ_PATH",    DEFAULT_SFZ_PATH).split(splitter)
    CARLA_DEFAULT_JSFX_PATH   = os.getenv("JSFX_PATH",   DEFAULT_JSFX_PATH).split(splitter)

else:
    CARLA_DEFAULT_LADSPA_PATH = DEFAULT_LADSPA_PATH.split(splitter)
    CARLA_DEFAULT_DSSI_PATH   = DEFAULT_DSSI_PATH.split(splitter)
    CARLA_DEFAULT_LV2_PATH    = DEFAULT_LV2_PATH.split(splitter)
    CARLA_DEFAULT_VST2_PATH   = DEFAULT_VST2_PATH.split(splitter)
    CARLA_DEFAULT_VST3_PATH   = DEFAULT_VST3_PATH.split(splitter)
    CARLA_DEFAULT_CLAP_PATH   = DEFAULT_CLAP_PATH.split(splitter)
    CARLA_DEFAULT_SF2_PATH    = DEFAULT_SF2_PATH.split(splitter)
    CARLA_DEFAULT_SFZ_PATH    = DEFAULT_SFZ_PATH.split(splitter)
    CARLA_DEFAULT_JSFX_PATH   = DEFAULT_JSFX_PATH.split(splitter)

# ------------------------------------------------------------------------------------------------------------
# Default Plugin Folders (cleanup)

del DEFAULT_LADSPA_PATH
del DEFAULT_DSSI_PATH
del DEFAULT_LV2_PATH
del DEFAULT_VST2_PATH
del DEFAULT_VST3_PATH
del DEFAULT_CLAP_PATH
del DEFAULT_SF2_PATH
del DEFAULT_SFZ_PATH

# ------------------------------------------------------------------------------------------------------------
# Global Carla object

class CarlaObject():
    def __init__(self):
        self.cnprefix = ""    # Client name prefix
        self.gui      = None  # Host Window
        self.nogui    = False # Skip UI
        self.term     = False # Terminated by OS signal
        self.felib    = None  # Frontend lib object
        self.utils    = None  # Utils object

gCarla = CarlaObject()

# ------------------------------------------------------------------------------------------------------------
# Set CWD

CWD = sys.path[0]

if not CWD:
    CWD = os.path.dirname(sys.argv[0])

# make it work with cxfreeze
if os.path.isfile(CWD):
    CWD = os.path.dirname(CWD)
    if CWD.endswith("/lib"):
        CWD = CWD.rsplit("/lib",1)[0]
    CXFREEZE = True
    if not CARLA_OS_WIN:
        os.environ['CARLA_MAGIC_FILE'] = os.path.join(CWD, "magic.mgc")
else:
    CXFREEZE = False

# ------------------------------------------------------------------------------------------------------------
# Set DLL_EXTENSION

if CARLA_OS_WIN:
    DLL_EXTENSION = "dll"
elif CARLA_OS_MAC:
    DLL_EXTENSION = "dylib"
else:
    DLL_EXTENSION = "so"

# ------------------------------------------------------------------------------------------------------------
# Find decimal points for a parameter, using step and stepSmall

def countDecimalPoints(step, stepSmall):
    if stepSmall >= 1.0:
        return 0
    if step >= 1.0:
        return 2

    count = 0
    value = fmod(abs(step), 1)
    while 0.0001 < value < 0.999 and count < 6:
        value = fmod(value*10, 1)
        count += 1

    return count

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
    if not isinstance(value, list):
        return [value]
    return value

# ------------------------------------------------------------------------------------------------------------
# Get Icon from user theme, using our own as backup (Oxygen)

def getIcon(icon, size, qrcformat):
    return QIcon.fromTheme(icon, QIcon(":/%ix%i/%s.%s" % (size, size, icon, qrcformat)))

# ------------------------------------------------------------------------------------------------------------
# Handle some basic command-line arguments shared between all carla variants

def handleInitialCommandLineArguments(file):
    initName  = os.path.basename(file) if (file is not None and os.path.dirname(file) in PATH) else sys.argv[0]
    libPrefix = None
    readPrefixNext = False

    for arg in sys.argv[1:]:
        if arg.startswith("--with-appname="):
            initName = os.path.basename(arg.replace("--with-appname=", ""))

        elif arg.startswith("--with-libprefix="):
            libPrefix = arg.replace("--with-libprefix=", "")

        elif arg.startswith("--osc-gui="):
            gCarla.nogui = int(arg.replace("--osc-gui=", ""))

        elif arg.startswith("--cnprefix="):
            gCarla.cnprefix = arg.replace("--cnprefix=", "")

        elif arg == "--cnprefix":
            readPrefixNext = True

        elif arg == "--gdb":
            pass

        elif arg in ("-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui"):
            gCarla.nogui = True

        elif CARLA_OS_MAC and arg.startswith("-psn_"):
            pass

        elif arg in ("-h", "--h", "-help", "--help"):
            print("Usage: %s [OPTION]... [FILE|URL]" % initName)
            print("")
            print(" where FILE can be a Carla project or preset file to be loaded, or URL if using Carla-Control")
            print("")
            print(" and OPTION can be one or more of the following:")
            print("")
            print("    --cnprefix\t Set a prefix for client names in multi-client mode.")
            if isinstance(gCarla.nogui, bool):
                if X_LIBDIR_X is not None:
                    print("    --gdb     \t Run Carla inside gdb.")
                print(" -n,--no-gui  \t Run Carla headless, don't show UI.")
                print("")
            print(" -h,--help    \t Print this help text and exit.")
            print(" -v,--version \t Print version information and exit.")
            print("")

            if not isinstance(gCarla.nogui, bool):
                print("NOTE: when using %s the FILE is only valid the first time the backend is started" % initName)
                sys.exit(1)

            sys.exit(0)

        elif arg in ("-v", "--v", "-version", "--version"):
            pathBinaries, pathResources = getPaths(libPrefix)

            print("Using Carla version %s" % CARLA_VERSION_STRING)
            print("  Python version: %s" % sys.version.split(" ",1)[0])
            print("  Qt version:     %s" % QT_VERSION_STR)
            print("  PyQt version:   %s" % PYQT_VERSION_STR)
            print("  Binary dir:     %s" % pathBinaries)
            print("  Resources dir:  %s" % pathResources)

            sys.exit(1 if gCarla.nogui else 0)

        elif readPrefixNext:
            readPrefixNext = False
            gCarla.cnprefix = arg

    if gCarla.nogui and not isinstance(gCarla.nogui, bool):
        if os.fork():
            # pylint: disable=protected-access
            os._exit(0)
            # pylint: enable=protected-access
        else:
            os.setsid()

    return (initName, libPrefix)

# ------------------------------------------------------------------------------------------------------------
# Get initial project file (as passed in the command-line parameters)

def getInitialProjectFile(skipExistCheck = False):
    # NOTE: PyQt mishandles unicode characters, we directly use sys.argv instead of qApp->arguments()
    # see https://riverbankcomputing.com/pipermail/pyqt/2015-January/035395.html
    args = sys.argv[1:]
    readPrefixNext = False
    for arg in args:
        if readPrefixNext:
            readPrefixNext = False
            continue
        if arg.startswith("--cnprefix="):
            continue
        if arg.startswith("--osc-gui="):
            continue
        if arg.startswith("--with-appname="):
            continue
        if arg.startswith("--with-libprefix="):
            continue
        if arg == "--cnprefix":
            readPrefixNext = True
            continue
        if arg in ("-n", "--n", "-no-gui", "--no-gui", "-nogui", "--nogui", "--gdb"):
            continue
        if CARLA_OS_MAC and arg.startswith("-psn_"):
            continue
        arg = os.path.expanduser(arg)
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
    elif CWDl.endswith("frontend"):
        pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "..", "bin"))
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
# Backwards-compatible horizontalAdvance/width call, depending on Qt version

def fontMetricsHorizontalAdvance(fontMetrics, string):
    if QT_VERSION >= 0x50b00:
        return fontMetrics.horizontalAdvance(string)
    return fontMetrics.width(string)

# ------------------------------------------------------------------------------------------------------------
# Custom QMessageBox which resizes itself to fit text

class QMessageBoxWithBetterWidth(QMessageBox):
    def __init__(self, parent):
        QMessageBox.__init__(self, parent)

    def showEvent(self, event):
        fontMetrics = self.fontMetrics()

        lines = self.text().strip().split("\n") + self.informativeText().strip().split("\n")

        if lines:
            width = 0

            for line in lines:
                width = max(fontMetricsHorizontalAdvance(fontMetrics, line), width)

            self.layout().setColumnMinimumWidth(2, width + 12)

        QMessageBox.showEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Custom MessageBox

# pylint: disable=too-many-arguments
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
    # pylint: disable=no-value-for-parameter
    return msgBox.exec_()
    # pylint: enable=no-value-for-parameter
# pylint: enable=too-many-arguments

# ------------------------------------------------------------------------------------------------------------
# pylint: enable=possibly-used-before-assignment
