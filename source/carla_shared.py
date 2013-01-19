#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Common Carla code
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
#import platform
import sys
from codecs import open as codecopen
from copy import deepcopy
#from decimal import Decimal
from PyQt4.QtCore import qWarning
#pyqtSlot, qFatal, Qt, QSettings, QTimer
#from PyQt4.QtGui import QColor, QCursor, QDialog, QFontMetrics, QFrame, QGraphicsScene, QInputDialog, QLinearGradient, QMenu, QPainter, QPainterPath, QVBoxLayout, QWidget
#from PyQt4.QtXml import QDomDocument

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

#import ui_carla_about
#import ui_carla_edit
#import ui_carla_parameter
#import ui_carla_plugin

# ------------------------------------------------------------------------------------------------------------
# Try Import Signal

try:
    from signal import signal, SIGINT, SIGTERM, SIGUSR1, SIGUSR2
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

is64bit = False #bool(platform.architecture()[0] == "64bit" and sys.maxsize > 2**32)

# ------------------------------------------------------------------------------------------------------------
# Check if a value is a number (float support)

def isNumber(value):
    try:
        float(value)
        return True
    except:
        return False

# ------------------------------------------------------------------------------------------------------------
# Unicode open

def uopen(filename, mode="r"):
    return codecopen(filename, encoding="utf-8", mode=mode)

# ------------------------------------------------------------------------------------------------
# Backend defines

MAX_DEFAULT_PLUGINS    = 99
MAX_RACK_PLUGINS       = 16
MAX_PATCHBAY_PLUGINS   = 999
MAX_DEFAULT_PARAMETERS = 200

# Plugin Hints
PLUGIN_IS_BRIDGE          = 0x001
PLUGIN_IS_RTSAFE          = 0x002
PLUGIN_IS_SYNTH           = 0x004
PLUGIN_HAS_GUI            = 0x010
PLUGIN_USES_CHUNKS        = 0x020
PLUGIN_USES_SINGLE_THREAD = 0x040
PLUGIN_CAN_DRYWET         = 0x100
PLUGIN_CAN_VOLUME         = 0x200
PLUGIN_CAN_BALANCE        = 0x400
PLUGIN_CAN_FORCE_STEREO   = 0x800

# Parameter Hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# FIXME
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

# Options Type
OPTION_PROCESS_NAME            = 0
OPTION_PROCESS_MODE            = 1
OPTION_PROCESS_HIGH_PRECISION  = 2
OPTION_FORCE_STEREO            = 3
OPTION_PREFER_PLUGIN_BRIDGES   = 4
OPTION_PREFER_UI_BRIDGES       = 5
OPTION_USE_DSSI_VST_CHUNKS     = 6
OPTION_MAX_PARAMETERS          = 7
OPTION_OSC_UI_TIMEOUT          = 8
OPTION_PREFERRED_BUFFER_SIZE   = 9
OPTION_PREFERRED_SAMPLE_RATE   = 10
OPTION_PATH_BRIDGE_NATIVE      = 11
OPTION_PATH_BRIDGE_POSIX32     = 12
OPTION_PATH_BRIDGE_POSIX64     = 13
OPTION_PATH_BRIDGE_WIN32       = 14
OPTION_PATH_BRIDGE_WIN64       = 15
OPTION_PATH_BRIDGE_LV2_GTK2    = 16
OPTION_PATH_BRIDGE_LV2_GTK3    = 17
OPTION_PATH_BRIDGE_LV2_QT4     = 18
OPTION_PATH_BRIDGE_LV2_QT5     = 19
OPTION_PATH_BRIDGE_LV2_COCOA   = 20
OPTION_PATH_BRIDGE_LV2_WINDOWS = 21
OPTION_PATH_BRIDGE_LV2_X11     = 22
OPTION_PATH_BRIDGE_VST_COCOA   = 23
OPTION_PATH_BRIDGE_VST_HWND    = 24
OPTION_PATH_BRIDGE_VST_X11     = 25

# Callback Type
CALLBACK_DEBUG                          = 0
CALLBACK_PARAMETER_VALUE_CHANGED        = 1
CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 2
CALLBACK_PARAMETER_MIDI_CC_CHANGED = 3
CALLBACK_PROGRAM_CHANGED      = 4
CALLBACK_MIDI_PROGRAM_CHANGED = 5
CALLBACK_NOTE_ON              = 6
CALLBACK_NOTE_OFF             = 7
CALLBACK_SHOW_GUI             = 8
CALLBACK_UPDATE               = 9
CALLBACK_RELOAD_INFO          = 10
CALLBACK_RELOAD_PARAMETERS    = 11
CALLBACK_RELOAD_PROGRAMS      = 12
CALLBACK_RELOAD_ALL           = 13
CALLBACK_NSM_ANNOUNCE         = 14
CALLBACK_NSM_OPEN1            = 15
CALLBACK_NSM_OPEN2            = 16
CALLBACK_NSM_SAVE             = 17
CALLBACK_ERROR                = 18
CALLBACK_QUIT                 = 19

# Process Mode Type
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2
PROCESS_MODE_PATCHBAY         = 3

# Set BINARY_NATIVE
if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if is64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if is64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Carla Host object

class CarlaHostObject(object):
    __slots__ = [
        'host',
        'gui',
        'isControl',
        'processMode',
        'maxParameters'
    ]

Carla = CarlaHostObject()
Carla.host = None
Carla.gui  = None
Carla.isControl = False
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
    'type': CUSTOM_DATA_INVALID,
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
# Carla XML helpers

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

def xmlSafeString(string, toXml):
    if toXml:
        return string.replace("&", "&amp;").replace("<","&lt;").replace(">","&gt;").replace("'","&apos;").replace("\"","&quot;")
    else:
        return string.replace("&amp;", "&").replace("&lt;","<").replace("&gt;",">").replace("&apos;","'").replace("&quot;","\"")
