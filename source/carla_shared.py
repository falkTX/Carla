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
import platform
import sys
from codecs import open as codecopen
from copy import deepcopy
#from decimal import Decimal
from PyQt4.QtCore import pyqtSlot, qWarning, Qt, QByteArray, QSettings, QTimer, SIGNAL, SLOT
#pyqtSlot, qFatal,
from PyQt4.QtGui import QColor, QDialog, QIcon, QFontMetrics, QFrame, QMessageBox, QPainter, QPainterPath, QVBoxLayout, QWidget
#from PyQt4.QtGui import QCursor, QGraphicsScene, QInputDialog, QLinearGradient, QMenu,
#from PyQt4.QtXml import QDomDocument

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_about
import ui_carla_edit
import ui_carla_parameter
import ui_carla_plugin

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

kIs64bit = bool(platform.architecture()[0] == "64bit" and sys.maxsize > 2**32)

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

# FIXME
# Custom Data types
#CUSTOM_DATA_INVALID = None
#CUSTOM_DATA_CHUNK   = "http://kxstudio.sf.net/ns/carla/chunk"
#CUSTOM_DATA_STRING  = "http://kxstudio.sf.net/ns/carla/string"

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
            #self.ui.le_osc_url.setText(cString(Carla.host.get_host_osc_url()) if Carla.host.is_engine_running() else self.tr("(Engine not running)"))

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

        self.fMidiCC      = -1
        self.fMidiChannel = 1
        self.fParameterId = pInfo['index']
        self.fPluginId    = pluginId
        self.fTabIndex    = tabIndex

        self.ui.label.setText(pInfo['name'])

        for MIDI_CC in MIDI_CC_LIST:
            self.ui.combo.addItem(MIDI_CC)

        if pType == PARAMETER_INPUT:
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setDefault(pInfo['default'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setStep(pInfo['step'])
            self.ui.widget.setStepSmall(pInfo['stepSmall'])
            self.ui.widget.setStepLarge(pInfo['stepLarge'])
            self.ui.widget.setScalePoints(pInfo['scalepoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

            if not pHints & PARAMETER_IS_ENABLED:
                self.ui.widget.setReadOnly(True)
                self.ui.combo.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

            elif not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.combo.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

        elif pType == PARAMETER_OUTPUT:
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setReadOnly(True)

            if not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.combo.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

        else:
            self.ui.widget.setVisible(False)
            self.ui.combo.setVisible(False)
            self.ui.sb_channel.setVisible(False)

        if pHints & PARAMETER_USES_CUSTOM_TEXT:
            self.ui.widget.setTextCallback(self._textCallBack)

        self.setMidiCC(pInfo['midiCC'])
        self.setMidiChannel(pInfo['midiChannel'])

        self.connect(self.ui.widget, SIGNAL("valueChanged(double)"), SLOT("slot_valueChanged(double)"))
        self.connect(self.ui.sb_channel, SIGNAL("valueChanged(int)"), SLOT("slot_midiChannelChanged(int)"))
        self.connect(self.ui.combo, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiCcChanged(int)"))

        #if force_parameters_style:
        #self.widget.force_plastique_style()

        self.ui.widget.updateAll()

    def setDefault(self, value):
        self.ui.widget.setDefault(value)

    def setValue(self, value, send=True):
        self.ui.widget.setValue(value, send)

    def setMidiCC(self, cc):
        self.fMidiCC = cc
        self._setMidiCcInComboBox(cc)

    def setMidiChannel(self, channel):
        self.fMidiChannel = channel
        self.ui.sb_channel.setValue(channel)

    def setLabelWidth(self, width):
        self.ui.label.setMinimumWidth(width)
        self.ui.label.setMaximumWidth(width)

    def tabIndex(self):
        return self.fTabIndex

    @pyqtSlot(float)
    def slot_valueChanged(self, value):
        self.emit(SIGNAL("valueChanged(int, double)"), self.fParameterId, value)

    @pyqtSlot(int)
    def slot_midiCcChanged(self, ccIndex):
        if ccIndex <= 0:
            cc = -1
        else:
            ccStr = MIDI_CC_LIST[ccIndex - 1].split(" ")[0]
            cc    = int(ccStr, 16)

        if self.fMidiCC != cc:
            self.emit(SIGNAL("midiCcChanged(int, int)"), self.fParameterId, cc)

        self.fMidiCC = cc

    @pyqtSlot(int)
    def slot_midiChannelChanged(self, channel):
        if self.fMidiChannel != channel:
            self.emit(SIGNAL("midiChannelChanged(int, int)"), self.fParameterId, channel)

        self.fMidiChannel = channel

    def _setMidiCcInComboBox(self, cc):
        for i in range(len(MIDI_CC_LIST)):
            ccStr = MIDI_CC_LIST[i].split(" ")[0]
            if int(ccStr, 16) == cc:
                ccIndex = i+1
                break
        else:
            ccIndex = 0

        self.ui.combo.setCurrentIndex(ccIndex)

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
        self.fParameterList  = [] # (type, id, widget)
        self.fParameterIdsToUpdate = [] # id

        self.fTabIconOff = QIcon(":/bitmaps/led_off.png")
        self.fTabIconOn  = QIcon(":/bitmaps/led_yellow.png")
        self.fTabIconCount  = 0
        self.fTabIconTimers = []

        self.ui.keyboard.setMode(self.ui.keyboard.HORIZONTAL)
        self.ui.keyboard.setOctaves(6)
        self.ui.scrollArea.ensureVisible(self.ui.keyboard.width() * 1 / 5, 0)
        self.ui.scrollArea.setVisible(False)

        # TODO - not implemented yet
        self.ui.b_reload_program.setEnabled(False)
        self.ui.b_reload_midi_program.setEnabled(False)

        # Not available for carla-control
        if Carla.isControl:
            self.ui.b_load_state.setEnabled(False)
            self.ui.b_save_state.setEnabled(False)
        else:
            self.connect(self.ui.b_save_state, SIGNAL("clicked()"), SLOT("slot_saveState()"))
            self.connect(self.ui.b_load_state, SIGNAL("clicked()"), SLOT("slot_loadState()"))

        self.connect(self.ui.keyboard, SIGNAL("noteOn(int)"), SLOT("slot_noteOn(int)"))
        self.connect(self.ui.keyboard, SIGNAL("noteOff(int)"), SLOT("slot_noteOff(int)"))

        self.connect(self.ui.cb_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_programIndexChanged(int)"))
        self.connect(self.ui.cb_midi_programs, SIGNAL("currentIndexChanged(int)"), SLOT("slot_midiProgramIndexChanged(int)"))

        self.connect(self, SIGNAL("finished(int)"), SLOT("slot_finished()"))

        #self.reloadAll()

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

    def reloadInfo(self):
        pluginName  = cString(Carla.host.get_real_plugin_name(self.fPluginId))
        pluginType  = self.fPluginInfo['type']
        pluginHints = self.fPluginInfo['hints']

        # Automatically change to MidiProgram tab
        if pluginType != PLUGIN_VST and not self.ui.le_name.text():
            self.ui.tab_programs.setCurrentIndex(1)

        # Set Meta-Data
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
        self.ui.label_plugin.setText("\n%s\n" % self.fPluginInfo['name'])
        self.setWindowTitle(self.fPluginInfo['name'])

        # Set Processing Data
        audioCountInfo = Carla.host.get_audio_port_count_info(self.fPluginId)
        midiCountInfo  = Carla.host.get_midi_port_count_info(self.fPluginId)
        paramCountInfo = Carla.host.get_parameter_count_info(self.fPluginId)

        self.ui.le_ains.setText(str(audioCountInfo['ins']))
        self.ui.le_aouts.setText(str(audioCountInfo['outs']))
        self.ui.le_params.setText(str(paramCountInfo['ins']))
        self.ui.le_couts.setText(str(paramCountInfo['outs']))

        self.ui.le_is_synth.setText(self.tr("Yes") if (pluginHints & PLUGIN_IS_SYNTH) else self.tr("No"))
        self.ui.le_has_gui.setText(self.tr("Yes") if (pluginHints & PLUGIN_HAS_GUI) else self.tr("No"))

        # Show/hide keyboard
        self.ui.scrollArea.setVisible((pluginHints & PLUGIN_IS_SYNTH) != 0 or (midiCountInfo['ins'] > 0 < midiCountInfo['outs']))

        # Force-Update parent for new hints (knobs)
        if self.fRealParent:
            self.fRealParent.recheckPluginHints(pluginHints)

    def reloadParameters(self):
        parameterCount = Carla.host.get_parameter_count(self.m_pluginId)

        # Reset
        self.fParameterCount = 0
        self.fParameterList  = []
        self.fParameterIdsToUpdate = []

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

                    parameter['scalepoints'].append(
                        {
                          'value': scalePointInfo['value'],
                          'label': cString(scalePointInfo['label'])
                        })

                paramInputList.append(parameter)

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if parameter['type'] == PARAMETER_INPUT:
                    paramInputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramInputWidthTMP > paramInputWidth:
                        paramInputWidth = paramInputWidthTMP

                    if len(paramInputList) == 10:
                        paramInputListFull.append((paramInputList, paramInputWidth))
                        paramInputList  = []
                        paramInputWidth = 0

                else:
                    paramOutputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramOutputWidthTMP > paramOutputWidth:
                        paramOutputWidth = paramOutputWidthTMP

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
                'scalepoints': [],

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

            self.createParameterWidgets(PARAMETER_UNKNOWN, paramFakeListFull, self.tr("Information"))

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
                mpLabel = cString(mpData['label'])
                self.ui.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank, mpProg, mpLabel))

            self.fCurrentMidiProgram = Carla.host.get_current_midi_program_index(self.fPluginId)
            self.ui.cb_midi_programs.setCurrentIndex(self.fCurrentMidiProgram)

        else:
            self.fCurrentMidiProgram = -1
            self.ui.cb_midi_programs.setEnabled(False)

        self.ui.cb_midi_programs.blockSignals(False)

    def setVisible(self, yesNo):
        if yesNo:
            if not self.fGeometry.isNull():
                self.restoreGeometry(self.fGeometry)
        else:
            self.fGeometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameterId, value):
        Carla.host.set_parameter_value(self.fPluginId, parameterId, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameterId, channel):
        Carla.host.set_parameter_midi_channel(self.fPluginId, parameterId, channel-1)

    @pyqtSlot(int, int)
    def slot_parameterMidiCcChanged(self, parameterId, cc):
        Carla.host.set_parameter_midi_cc(self.fPluginId, parameterId, cc)

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
            self.fRealParent.led_midi.setChecked(True)

    @pyqtSlot()
    def slot_notesOff(self):
        if self.fRealParent:
            self.fRealParent.led_midi.setChecked(False)

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

                self.connect(paramWidget, SIGNAL("midiChannelChanged(int, int)"), SLOT("slot_parameterMidiChannelChanged(int, int)"))
                self.connect(paramWidget, SIGNAL("midiCcChanged(int, int)"), SLOT("slot_parameterMidiCcChanged(int, int)"))

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

        self.fPluginId   = pluginId
        #self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId)
        #self.fPluginInfo["binary"]    = cString(self.fPluginInfo["binary"])
        #self.fPluginInfo["name"]      = cString(self.fPluginInfo["name"])
        #self.fPluginInfo["label"]     = cString(self.fPluginInfo["label"])
        #self.fPluginInfo["maker"]     = cString(self.fPluginInfo["maker"])
        #self.fPluginInfo["copyright"] = cString(self.fPluginInfo["copyright"])

        self.fParameterIconTimer = ICON_STATE_NULL

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

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

        self.ui.dial_drywet.setPixmap(3)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_b_left.setPixmap(4)
        self.ui.dial_b_right.setPixmap(4)

        self.ui.dial_drywet.setCustomPaint(self.ui.dial_drywet.CUSTOM_PAINT_CARLA_WET)
        self.ui.dial_vol.setCustomPaint(self.ui.dial_vol.CUSTOM_PAINT_CARLA_VOL)
        self.ui.dial_b_left.setCustomPaint(self.ui.dial_b_left.CUSTOM_PAINT_CARLA_L)
        self.ui.dial_b_right.setCustomPaint(self.ui.dial_b_right.CUSTOM_PAINT_CARLA_R)

        self.ui.peak_in.setColor(self.ui.peak_in.GREEN)
        self.ui.peak_in.setOrientation(self.ui.peak_in.HORIZONTAL)

        self.ui.peak_out.setColor(self.ui.peak_in.BLUE)
        self.ui.peak_out.setOrientation(self.ui.peak_out.HORIZONTAL)

        self.ui.peak_in.setChannels(self.fPeaksInputCount)
        self.ui.peak_out.setChannels(self.fPeaksOutputCount)

        #self.ui.label_name.setText(self.fPluginInfo['name'])

        self.ui.edit_dialog = PluginEdit(self, self.fPluginId)
        self.ui.edit_dialog.hide()

        #self.connect(self.ui.led_enable, SIGNAL("clicked(bool)"), SLOT("slot_setActive(bool)"))
        #self.connect(self.ui.dial_drywet, SIGNAL("sliderMoved(int)"), SLOT("slot_setDryWet(int)"))
        #self.connect(self.ui.dial_vol, SIGNAL("sliderMoved(int)"), SLOT("slot_setVolume(int)"))
        #self.connect(self.ui.dial_b_left, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceLeft(int)"))
        #self.connect(self.ui.dial_b_right, SIGNAL("sliderMoved(int)"), SLOT("slot_setBalanceRight(int)"))
        #self.connect(self.ui.b_gui, SIGNAL("clicked(bool)"), SLOT("slot_guiClicked(bool)"))
        self.connect(self.ui.b_edit, SIGNAL("clicked(bool)"), SLOT("slot_editClicked(bool)"))
        #self.connect(self.ui.b_remove, SIGNAL("clicked()"), SLOT("slot_removeClicked()"))

        #self.connect(self.ui.dial_drywet, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_vol, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_b_left, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))
        #self.connect(self.ui.dial_b_right, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomDialMenu()"))

        # FIXME
        self.ui.frame_controls.setVisible(False)
        self.ui.pushButton.setVisible(False)
        self.ui.pushButton_2.setVisible(False)
        self.ui.pushButton_3.setVisible(False)
        self.setMaximumHeight(30)

    def editClosed(self):
        self.ui.b_edit.setChecked(False)

    def recheckPluginHints(self, hints):
        self.fPluginInfo['hints'] = hints
        self.ui.dial_drywet.setEnabled(hints & PLUGIN_CAN_DRYWET)
        self.ui.dial_vol.setEnabled(hints & PLUGIN_CAN_VOLUME)
        self.ui.dial_b_left.setEnabled(hints & PLUGIN_CAN_BALANCE)
        self.ui.dial_b_right.setEnabled(hints & PLUGIN_CAN_BALANCE)
        self.ui.b_gui.setEnabled(hints & PLUGIN_HAS_GUI)

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
    def slot_editClicked(self, show):
        self.ui.edit_dialog.setVisible(show)

# ------------------------------------------------------------------------------------------------------------
# TESTING

#from PyQt4.QtGui import QApplication

#Carla.isControl = True

#ptest = {
    #'index': 0,
    #'name': "",
    #'symbol': "",
    #'current': 0.1,
    #'default': 0.3,
    #'minimum': 0.0,
    #'maximum': 1.0,
    #'midiChannel': 7,
    #'midiCC': 2,
    #'type': PARAMETER_INPUT,
    #'hints': PARAMETER_IS_ENABLED | PARAMETER_IS_AUTOMABLE,
    #'scalepoints': [],
    #'step': 0.01,
    #'stepSmall': 0.001,
    #'stepLarge': 0.1,
    #'unit': "un",
#}

#app  = QApplication(sys.argv)
#gui1 = CarlaAboutW(None)
#gui2 = PluginParameter(None, ptest, 0, 0)
#gui3 = PluginEdit(None, 0)
#gui4 = PluginWidget(None, 0)
#gui1.show()
#gui2.show()
#gui3.show()
#gui4.show()
#app.exec_()
