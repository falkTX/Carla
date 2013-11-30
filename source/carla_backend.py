#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
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

from ctypes import *
from platform import architecture
from sys import platform, maxsize

# ------------------------------------------------------------------------------------------------------------
# 64bit check

kIs64bit = bool(architecture()[0] == "64bit" and maxsize > 2**32)

# ------------------------------------------------------------------------------------------------------------
# Set Platform

if platform == "darwin":
    HAIKU   = False
    LINUX   = False
    MACOS   = True
    WINDOWS = False
elif "haiku" in platform:
    HAIKU   = True
    LINUX   = False
    MACOS   = False
    WINDOWS = False
elif "linux" in platform:
    HAIKU   = False
    LINUX   = True
    MACOS   = False
    WINDOWS = False
elif platform in ("win32", "win64", "cygwin"):
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
# Convert a ctypes c_char_p into a python string

def charPtrToString(value):
    if not value:
        return ""
    if isinstance(value, str):
        return value
    return value.decode("utf-8", errors="ignore")

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes POINTER(c_char_p) into a python string list

def charPtrPtrToStringList(charPtrPtr):
    if not charPtrPtr:
        return []

    i       = 0
    charPtr = charPtrPtr[0]
    strList = []

    while charPtr:
        strList.append(charPtr.decode("utf-8", errors="ignore"))

        i += 1
        charPtr = charPtrPtr[i]

    return strList

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes POINTER(c_<num>) into a python number list

def numPtrToList(numPtr):
    if not numPtr:
        return []

    i       = 0
    num     = numPtr[0].value
    numList = []

    while num not in (0, 0.0):
        numList.append(num)

        i += 1
        num = numPtr[i].value

    return numList

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict

def structToDict(struct):
    return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------------------
# Carla Backend API

# Maximum default number of loadable plugins.
MAX_DEFAULT_PLUGINS = 99

# Maximum number of loadable plugins in rack mode.
MAX_RACK_PLUGINS = 16

# Maximum number of loadable plugins in patchbay mode.
MAX_PATCHBAY_PLUGINS = 255

# Maximum default number of parameters allowed.
# @see ENGINE_OPTION_MAX_PARAMETERS
MAX_DEFAULT_PARAMETERS = 200

# ------------------------------------------------------------------------------------------------------------
# Engine Driver Device Hints

# Engine driver device has custom control-panel.
# @see ENGINE_OPTION_AUDIO_SHOW_CTRL_PANEL
ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL = 0x1

# Engine driver device can change buffer-size on the fly.
ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE = 0x2

# Engine driver device can change sample-rate on the fly.
ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE = 0x4

# ------------------------------------------------------------------------------------------------------------
# Plugin Hints

# Plugin is a bridge.
# This hint is required because "bridge" itself is not a plugin type.
PLUGIN_IS_BRIDGE = 0x01

# Plugin is hard real-time safe.
PLUGIN_IS_RTSAFE = 0x02

# Plugin is a synth (produces sound).
PLUGIN_IS_SYNTH = 0x04

# Plugin has its own custom UI.
# @see carla_show_custom_ui()
PLUGIN_HAS_CUSTOM_UI = 0x08

# Plugin can use internal Dry/Wet control.
PLUGIN_CAN_DRYWET = 0x10

# Plugin can use internal Volume control.
PLUGIN_CAN_VOLUME = 0x20

# Plugin can use internal (Stereo) Balance controls.
PLUGIN_CAN_BALANCE = 0x40

# Plugin can use internal (Mono) Panning control.
PLUGIN_CAN_PANNING = 0x80

# ------------------------------------------------------------------------------------------------------------
# Plugin Options

# Use constant/fixed-size audio buffers.
PLUGIN_OPTION_FIXED_BUFFERS = 0x001

# Force mono plugin as stereo.
PLUGIN_OPTION_FORCE_STEREO = 0x002

# Map MIDI programs to plugin programs.
PLUGIN_OPTION_MAP_PROGRAM_CHANGES = 0x004

# Use chunks to save and restore data.
PLUGIN_OPTION_USE_CHUNKS = 0x008

# Send MIDI control change events.
PLUGIN_OPTION_SEND_CONTROL_CHANGES = 0x010

# Send MIDI channel pressure events.
PLUGIN_OPTION_SEND_CHANNEL_PRESSURE = 0x020

# Send MIDI note after-touch events.
PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH = 0x040

# Send MIDI pitch-bend events.
PLUGIN_OPTION_SEND_PITCHBEND = 0x080

# Send MIDI all-sounds/notes-off events, single note-offs otherwise.
PLUGIN_OPTION_SEND_ALL_SOUND_OFF = 0x100

# ------------------------------------------------------------------------------------------------------------
# Parameter Hints

# Parameter value is boolean.
PARAMETER_IS_BOOLEAN = 0x001

# Parameter value is integer.
PARAMETER_IS_INTEGER = 0x002

# Parameter value is logarithmic.
PARAMETER_IS_LOGARITHMIC = 0x004

# Parameter is enabled.
# It can be viewed, changed and stored.
PARAMETER_IS_ENABLED = 0x010

# Parameter is automable (real-time safe).
PARAMETER_IS_AUTOMABLE = 0x020

# Parameter is read-only.
# It cannot be changed.
PARAMETER_IS_READ_ONLY = 0x040

# Parameter needs sample rate to work.
# Value and ranges are multiplied by sample rate on usage and divided by sample rate on save.
PARAMETER_USES_SAMPLERATE = 0x100

# Parameter uses scale points to define internal values in a meaningful way.
PARAMETER_USES_SCALEPOINTS = 0x200

# Parameter uses custom text for displaying its value.
# @see carla_get_parameter_text()
PARAMETER_USES_CUSTOM_TEXT = 0x400

# ------------------------------------------------------------------------------------------------------------
# Patchbay Port Hints

# Patchbay port is input.
# When this hint is not set, port is assumed to be output.
PATCHBAY_PORT_IS_INPUT = 0x1

# Patchbay port is of Audio type.
PATCHBAY_PORT_TYPE_AUDIO = 0x2

# Patchbay port is of CV type (Control Voltage).
PATCHBAY_PORT_TYPE_CV = 0x4

# Patchbay port is of MIDI type.
PATCHBAY_PORT_TYPE_MIDI = 0x8

# ------------------------------------------------------------------------------------------------------------
# Custom Data Types

# Boolean string type URI.
# Only "true" and "false" are valid values.
CUSTOM_DATA_TYPE_BOOLEAN = "http://kxstudio.sf.net/ns/carla/boolean"

# Chunk type URI.
CUSTOM_DATA_TYPE_CHUNK = "http://kxstudio.sf.net/ns/carla/chunk"

# String type URI.
CUSTOM_DATA_TYPE_STRING = "http://kxstudio.sf.net/ns/carla/string"

# ------------------------------------------------------------------------------------------------------------
# Custom Data Keys

# Plugin options key.
CUSTOM_DATA_KEY_PLUGIN_OPTIONS = "CarlaPluginOptions"

# UI position key.
CUSTOM_DATA_KEY_UI_POSITION = "CarlaUiPosition"

# UI size key.
CUSTOM_DATA_KEY_UI_SIZE = "CarlaUiSize"

# UI visible key.
CUSTOM_DATA_KEY_UI_VISIBLE = "CarlaUiVisible"

# ------------------------------------------------------------------------------------------------------------
# Binary Type

# Null binary type.
BINARY_NONE = 0

# POSIX 32bit binary.
BINARY_POSIX32 = 1

# POSIX 64bit binary.
BINARY_POSIX64 = 2

# Windows 32bit binary.
BINARY_WIN32 = 3

# Windows 64bit binary.
BINARY_WIN64 = 4

# Other binary type.
BINARY_OTHER = 5

# ------------------------------------------------------------------------------------------------------------
# Plugin Type

# Null plugin type.
PLUGIN_NONE = 0

# Internal plugin.
PLUGIN_INTERNAL = 1

# LADSPA plugin.
PLUGIN_LADSPA = 2

# DSSI plugin.
PLUGIN_DSSI = 3

# LV2 plugin.
PLUGIN_LV2 = 4

# VST plugin.
PLUGIN_VST = 5

# AU plugin.
# @note MacOS only
PLUGIN_AU = 6

# Csound file.
PLUGIN_CSOUND = 7

# GIG file.
PLUGIN_GIG = 8

# SF2 file (also known as SoundFont).
PLUGIN_SF2 = 9

# SFZ file.
PLUGIN_SFZ = 10

# ------------------------------------------------------------------------------------------------------------
# Plugin Category

# Null plugin category.
PLUGIN_CATEGORY_NONE = 0

# A synthesizer or generator.
PLUGIN_CATEGORY_SYNTH = 1

# A delay or reverb.
PLUGIN_CATEGORY_DELAY = 2

# An equalizer.
PLUGIN_CATEGORY_EQ = 3

# A filter.
PLUGIN_CATEGORY_FILTER = 4

# A distortion plugin.
PLUGIN_CATEGORY_DISTORTION = 5

# A 'dynamic' plugin (amplifier, compressor, gate, etc).
PLUGIN_CATEGORY_DYNAMICS = 6

# A 'modulator' plugin (chorus, flanger, phaser, etc).
PLUGIN_CATEGORY_MODULATOR = 7

# An 'utility' plugin (analyzer, converter, mixer, etc).
PLUGIN_CATEGORY_UTILITY = 8

# Miscellaneous plugin (used to check if the plugin has a category).
PLUGIN_CATEGORY_OTHER = 9

# ------------------------------------------------------------------------------------------------------------
# Parameter Type

# Unknown parameter type.
PARAMETER_UNKNOWN = 0

# Input parameter.
PARAMETER_INPUT = 1

# Output parameter.
PARAMETER_OUTPUT = 2

# Special parameter.
# Used to report specific information to plugins.
PARAMETER_SPECIAL = 3

# ------------------------------------------------------------------------------------------------------------
# Internal Parameters Index

# Null parameter.
PARAMETER_NULL = -1

# Active parameter, boolean type.
# Default is 'false'.
PARAMETER_ACTIVE = -2

# Dry/Wet parameter.
# Range 0.0...1.0; default is 1.0.
PARAMETER_DRYWET = -3

# Volume parameter.
# Range 0.0...1.27; default is 1.0.
PARAMETER_VOLUME = -4

# Stereo Balance-Left parameter.
# Range -1.0...1.0; default is -1.0.
PARAMETER_BALANCE_LEFT = -5

# Stereo Balance-Right parameter.
# Range -1.0...1.0; default is 1.0.
PARAMETER_BALANCE_RIGHT = -6

# Mono Panning parameter.
# Range -1.0...1.0; default is 0.0.
PARAMETER_PANNING = -7

# MIDI Control channel, integer type.
# Range -1...15 (-1 = off).
PARAMETER_CTRL_CHANNEL = -8

# Max value, defined only for convenience.
PARAMETER_MAX = -9

# ------------------------------------------------------------------------------------------------------------
# Engine Callback Opcode

# Debug.
# This opcode is undefined and used only for testing purposes.
ENGINE_CALLBACK_DEBUG = 0

# A plugin has been added.
# @param pluginId Plugin Id
# @param valueStr Plugin name
ENGINE_CALLBACK_PLUGIN_ADDED = 1

# A plugin has been removed.
# @param pluginId Plugin Id
ENGINE_CALLBACK_PLUGIN_REMOVED = 2

# A plugin has been renamed.
# @param pluginId Plugin Id
# @param valueStr New plugin name
ENGINE_CALLBACK_PLUGIN_RENAMED = 3

# A plugin has become unavailable.
# @param pluginId Plugin Id
# @param valueStr Related error string
ENGINE_CALLBACK_PLUGIN_UNAVAILABLE = 4

# A parameter value has changed.
# @param pluginId Plugin Id
# @param value1   Parameter index
# @param value3   New parameter value
ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED = 5

# A parameter default has changed.
# @param pluginId Plugin Id
# @param value1   Parameter index
# @param value3   New default value
ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED = 6

# A parameter's MIDI CC has changed.
# @param pluginId Plugin Id
# @param value1   Parameter index
# @param value2   New MIDI CC
ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED = 7

# A parameter's MIDI channel has changed.
# @param pluginId Plugin Id
# @param value1   Parameter index
# @param value2   New MIDI channel
ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 8

# The current program of a plugin has changed.
# @param pluginId Plugin Id
# @param value1   New program index
ENGINE_CALLBACK_PROGRAM_CHANGED = 9

# The current MIDI program of a plugin has changed.
# @param pluginId Plugin Id
# @param value1   New MIDI bank
# @param value2   New MIDI program
ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED = 10

# A plugin's custom UI state has changed.
# @param pluginId Plugin Id
# @param value1   New state, as follows:\n
#                  0: UI is now hidden\n
#                  1: UI is now visible\n
#                 -1: UI has crashed and should not be shown again
ENGINE_CALLBACK_UI_STATE_CHANGED = 11

# A note has been pressed.
# @param pluginId Plugin Id
# @param value1   Channel
# @param value2   Note
# @param value3   Velocity
ENGINE_CALLBACK_NOTE_ON = 12

# A note has been released.
# @param pluginId Plugin Id
# @param value1   Channel
# @param value2   Note
ENGINE_CALLBACK_NOTE_OFF = 13

# A plugin needs update.
# @param pluginId Plugin Id
ENGINE_CALLBACK_UPDATE = 14

# A plugin's data/information has changed.
# @param pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_INFO = 15

# A plugin's parameters have changed.
# @param pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_PARAMETERS = 16

# A plugin's programs have changed.
# @param pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_PROGRAMS = 17

# A plugin state has changed.
# @param pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_ALL = 18

# A patchbay client has been added.
# @param pluginId Client Id
# @param valueStr Client name and icon, as "name:icon"
ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED = 19

# A patchbay client has been removed.
# @param pluginId Client Id
ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED = 20

# A patchbay client has been renamed.
# @param pluginId Client Id
# @param valueStr New client name
ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED = 21

# A patchbay client icon has changed.
# @param pluginId Client Id
# @param valueStr New icon name
ENGINE_CALLBACK_PATCHBAY_CLIENT_ICON_CHANGED = 22

# A patchbay port has been added.
# @param pluginId Client Id
# @param value1   Port Id
# @param value2   Port hints
# @param valueStr Port name
# @see PatchbayPortHints
ENGINE_CALLBACK_PATCHBAY_PORT_ADDED = 23

# A patchbay port has been removed.
# @param pluginId Client Id
# @param value1   Port Id
ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED = 24

# A patchbay port has been renamed.
# @param pluginId Client Id
# @param value1   Port Id
# @param valueStr New port name
ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED = 25

# A patchbay connection has been added.
# @param value1 Output port Id
# @param value2 Input port Id
ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED = 26

# A patchbay connection has been removed.
# @param value1 Output port Id
# @param value2 Input port Id
ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED = 27

# Engine started.
# @param value1   Process mode
# @param value2   Transport mode
# @param valuestr Engine driver
# @see EngineProcessMode
# @see EngineTransportMode
ENGINE_CALLBACK_ENGINE_STARTED = 28

# Engine stopped.
ENGINE_CALLBACK_ENGINE_STOPPED = 29

# Engine process mode has changed.
# @param value1 New process mode
# @see EngineProcessMode
ENGINE_CALLBACK_PROCESS_MODE_CHANGED = 30

# Engine transport mode has changed.
# @param value1 New transport mode
# @see EngineTransportMode
ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED = 31

# Engine buffer-size changed.
# @param value1 New buffer size
ENGINE_CALLBACK_BUFFER_SIZE_CHANGED = 32

# Engine sample-rate changed.
# @param value3 New sample rate
ENGINE_CALLBACK_SAMPLE_RATE_CHANGED = 33

# Show a message as information.
# @param valueStr The message
ENGINE_CALLBACK_INFO = 34

# Show a message as an error.
# @param valueStr The message
ENGINE_CALLBACK_ERROR = 35

# The engine has crashed or malfunctioned and will no longer work.
ENGINE_CALLBACK_QUIT = 36

# ------------------------------------------------------------------------------------------------------------
# Engine Options Type

ENGINE_OPTION_PROCESS_NAME             = 0
ENGINE_OPTION_PROCESS_MODE             = 1
ENGINE_OPTION_TRANSPORT_MODE           = 2
ENGINE_OPTION_FORCE_STEREO             = 3
ENGINE_OPTION_PREFER_PLUGIN_BRIDGES    = 4
ENGINE_OPTION_PREFER_UI_BRIDGES        = 5
ENGINE_OPTION_UIS_ALWAYS_ON_TOP        = 6
ENGINE_OPTION_MAX_PARAMETERS           = 7
ENGINE_OPTION_UI_BRIDGES_TIMEOUT       = 8
ENGINE_OPTION_AUDIO_NUM_PERIODS        = 9
ENGINE_OPTION_AUDIO_BUFFER_SIZE        = 10
ENGINE_OPTION_AUDIO_SAMPLE_RATE        = 11
ENGINE_OPTION_AUDIO_DEVICE             = 12
ENGINE_OPTION_PATH_RESOURCES           = 13
ENGINE_OPTION_PATH_BRIDGE_NATIVE       = 14
ENGINE_OPTION_PATH_BRIDGE_POSIX32      = 15
ENGINE_OPTION_PATH_BRIDGE_POSIX64      = 16
ENGINE_OPTION_PATH_BRIDGE_WIN32        = 17
ENGINE_OPTION_PATH_BRIDGE_WIN64        = 18
ENGINE_OPTION_PATH_BRIDGE_LV2_EXTERNAL = 19
ENGINE_OPTION_PATH_BRIDGE_LV2_GTK2     = 20
ENGINE_OPTION_PATH_BRIDGE_LV2_GTK3     = 21
ENGINE_OPTION_PATH_BRIDGE_LV2_NTK      = 22
ENGINE_OPTION_PATH_BRIDGE_LV2_QT4      = 23
ENGINE_OPTION_PATH_BRIDGE_LV2_QT5      = 24
ENGINE_OPTION_PATH_BRIDGE_LV2_COCOA    = 25
ENGINE_OPTION_PATH_BRIDGE_LV2_WINDOWS  = 26
ENGINE_OPTION_PATH_BRIDGE_LV2_X11      = 27
ENGINE_OPTION_PATH_BRIDGE_VST_MAC      = 28
ENGINE_OPTION_PATH_BRIDGE_VST_HWND     = 29
ENGINE_OPTION_PATH_BRIDGE_VST_X11      = 30

# ------------------------------------------------------------------------------------------------------------
# Process Mode

PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2
PROCESS_MODE_PATCHBAY         = 3
PROCESS_MODE_BRIDGE           = 4

# ------------------------------------------------------------------------------------------------------------
# Transport Mode

TRANSPORT_MODE_INTERNAL = 0
TRANSPORT_MODE_JACK     = 1
TRANSPORT_MODE_PLUGIN   = 2
TRANSPORT_MODE_BRIDGE   = 3

# ------------------------------------------------------------------------------------------------------------
# File Callback Type

FILE_CALLBACK_DEBUG = 0
FILE_CALLBACK_OPEN  = 1
FILE_CALLBACK_SAVE  = 2

# ------------------------------------------------------------------------------------------------------------
# Set BINARY_NATIVE

if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if kIs64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if kIs64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int32

EngineCallbackFunc = CFUNCTYPE(None, c_void_p, c_enum, c_uint, c_int, c_int, c_float, c_char_p)
FileCallbackFunc = CFUNCTYPE(c_char_p, c_void_p, c_enum, c_bool, c_char_p, c_char_p)

class ParameterData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("index", c_int32),
        ("rindex", c_int32),
        ("hints", c_uint),
        ("midiChannel", c_uint8),
        ("midiCC", c_int16)
    ]

class ParameterRanges(Structure):
    _fields_ = [
        ("def", c_float),
        ("min", c_float),
        ("max", c_float),
        ("step", c_float),
        ("stepSmall", c_float),
        ("stepLarge", c_float)
    ]

class MidiProgramData(Structure):
    _fields_ = [
        ("bank", c_uint32),
        ("program", c_uint32),
        ("name", c_char_p)
    ]

class CustomData(Structure):
    _fields_ = [
        ("type", c_char_p),
        ("key", c_char_p),
        ("value", c_char_p)
    ]

class EngineDriverDeviceInfo(Structure):
    _fields_ = [
        ("hints", c_uint),
        ("bufferSizes", POINTER(c_uint32)),
        ("sampleRates", POINTER(c_double))
    ]

# ------------------------------------------------------------------------------------------------------------
# Host C++ -> Python variables

class CarlaPluginInfo(Structure):
    _fields_ = [
        ("type", c_enum),
        ("category", c_enum),
        ("hints", c_uint),
        ("optionsAvailable", c_uint),
        ("optionsEnabled", c_uint),
        ("binary", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p),
        ("iconName", c_char_p),
        ("patchbayClientId", c_int),
        ("uniqueId", c_long)
    ]

class CarlaNativePluginInfo(Structure):
    _fields_ = [
        ("category", c_enum),
        ("hints", c_uint),
        ("audioIns", c_uint32),
        ("audioOuts", c_uint32),
        ("midiIns", c_uint32),
        ("midiOuts", c_uint32),
        ("parameterIns", c_uint32),
        ("parameterOuts", c_uint32),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p)
    ]

class CarlaPortCountInfo(Structure):
    _fields_ = [
        ("ins", c_uint32),
        ("outs", c_uint32),
        ("total", c_uint32)
    ]

class CarlaParameterInfo(Structure):
    _fields_ = [
        ("name", c_char_p),
        ("symbol", c_char_p),
        ("unit", c_char_p),
        ("scalePointCount", c_uint32)
    ]

class CarlaScalePointInfo(Structure):
    _fields_ = [
        ("value", c_float),
        ("label", c_char_p)
    ]

class CarlaTransportInfo(Structure):
    _fields_ = [
        ("playing", c_bool),
        ("frame", c_uint64),
        ("bar", c_int32),
        ("beat", c_int32),
        ("tick", c_int32),
        ("bpm", c_double)
    ]

# ------------------------------------------------------------------------------------------------------------
# Python object dicts compatible with ctypes struct

PyParameterData = {
    'type': PARAMETER_NULL,
    'index': PARAMETER_NULL,
    'rindex': -1,
    'hints': 0x0,
    'midiChannel': 0,
    'midiCC': -1
}

PyParameterRanges = {
    'def': 0.0,
    'min': 0.0,
    'max': 1.0,
    'step': 0.01,
    'stepSmall': 0.0001,
    'stepLarge': 0.1
}

PyMidiProgramData = {
    'bank': 0,
    'program': 0,
    'name': None
}

PyCustomData = {
    'type': None,
    'key': None,
    'value': None
}

PyCarlaPluginInfo = {
    'type': PLUGIN_NONE,
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'optionsAvailable': 0x0,
    'optionsEnabled': 0x0,
    'binary': None,
    'name':  None,
    'label': None,
    'maker': None,
    'copyright': None,
    'iconName': None,
    'patchbayClientId': 0,
    'uniqueId': 0
}

PyCarlaPortCountInfo = {
    'ins': 0,
    'outs': 0,
    'total': 0
}

PyCarlaParameterInfo = {
    'name': None,
    'symbol': None,
    'unit': None,
    'scalePointCount': 0,
}

PyCarlaScalePointInfo = {
    'value': 0.0,
    'label': None
}

PyCarlaTransportInfo = {
    "playing": False,
    "frame": 0,
    "bar": 0,
    "beat": 0,
    "tick": 0,
    "bpm": 0.0
}

PyCarlaEngineDriverInfo = {
    "name": None,
    "hints": 0x0
}

# ------------------------------------------------------------------------------------------------------------
# Host Python object (Control/Standalone)

class Host(object):
    def __init__(self, libName):
        object.__init__(self)
        self._init(libName)

    # ...
    def get_extended_license_text(self):
        return self.lib.carla_get_extended_license_text()

    def get_supported_file_types(self):
        return self.lib.carla_get_supported_file_types()

    def get_engine_driver_count(self):
        return self.lib.carla_get_engine_driver_count()

    def get_engine_driver_name(self, index):
        return self.lib.carla_get_engine_driver_name(index)

    def get_engine_driver_device_names(self, index):
        return charPtrPtrToStringList(self.lib.carla_get_engine_driver_device_names(index))

    def get_engine_driver_device_info(self, index, deviceName):
        return structToDict(self.lib.carla_get_engine_driver_device_info(index, deviceName))

    def get_internal_plugin_count(self):
        return self.lib.carla_get_internal_plugin_count()

    def get_internal_plugin_info(self, internalPluginId):
        return structToDict(self.lib.carla_get_internal_plugin_info(internalPluginId).contents)

    def engine_init(self, driverName, clientName):
        return self.lib.carla_engine_init(driverName.encode("utf-8"), clientName.encode("utf-8"))

    def engine_close(self):
        return self.lib.carla_engine_close()

    def engine_idle(self):
        self.lib.carla_engine_idle()

    def is_engine_running(self):
        return self.lib.carla_is_engine_running()

    def set_engine_about_to_close(self):
        self.lib.carla_set_engine_about_to_close()

    def set_engine_callback(self, func):
        self._callback = EngineCallbackFunc(func)
        self.lib.carla_set_engine_callback(self._callback, c_nullptr)

    def set_engine_option(self, option, value, valueStr):
        self.lib.carla_set_engine_option(option, value, valueStr.encode("utf-8"))

    def load_filename(self, filename):
        return self.lib.carla_load_filename(filename.encode("utf-8"))

    def load_project(self, filename):
        return self.lib.carla_load_project(filename.encode("utf-8"))

    def save_project(self, filename):
        return self.lib.carla_save_project(filename.encode("utf-8"))

    def patchbay_connect(self, portIdA, portIdB):
        return self.lib.carla_patchbay_connect(portIdA, portIdB)

    def patchbay_disconnect(self, connectionId):
        return self.lib.carla_patchbay_disconnect(connectionId)

    def patchbay_refresh(self):
        return self.lib.carla_patchbay_refresh()

    def transport_play(self):
        self.lib.carla_transport_play()

    def transport_pause(self):
        self.lib.carla_transport_pause()

    def transport_relocate(self, frames):
        self.lib.carla_transport_relocate(frames)

    def get_current_transport_frame(self):
        return self.lib.carla_get_current_transport_frame()

    def get_transport_info(self):
        return structToDict(self.lib.carla_get_transport_info().contents)

    def add_plugin(self, btype, ptype, filename, name, label, extraStuff):
        cfilename = filename.encode("utf-8") if filename else c_nullptr
        cname     = name.encode("utf-8") if name else c_nullptr
        clabel    = label.encode("utf-8") if label else c_nullptr
        return self.lib.carla_add_plugin(btype, ptype, cfilename, cname, clabel, cast(extraStuff, c_void_p))

    def remove_plugin(self, pluginId):
        return self.lib.carla_remove_plugin(pluginId)

    def remove_all_plugins(self):
        return self.lib.carla_remove_all_plugins()

    def rename_plugin(self, pluginId, newName):
        return self.lib.carla_rename_plugin(pluginId, newName.encode("utf-8"))

    def clone_plugin(self, pluginId):
        return self.lib.carla_clone_plugin(pluginId)

    def replace_plugin(self, pluginId):
        return self.lib.carla_replace_plugin(pluginId)

    def switch_plugins(self, pluginIdA, pluginIdB):
        return self.lib.carla_switch_plugins(pluginIdA, pluginIdB)

    def load_plugin_state(self, pluginId, filename):
        return self.lib.carla_load_plugin_state(pluginId, filename.encode("utf-8"))

    def save_plugin_state(self, pluginId, filename):
        return self.lib.carla_save_plugin_state(pluginId, filename.encode("utf-8"))

    def get_plugin_info(self, pluginId):
        return structToDict(self.lib.carla_get_plugin_info(pluginId).contents)

    def get_audio_port_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_audio_port_count_info(pluginId).contents)

    def get_midi_port_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_midi_port_count_info(pluginId).contents)

    def get_parameter_count_info(self, pluginId):
        return structToDict(self.lib.carla_get_parameter_count_info(pluginId).contents)

    def get_parameter_info(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_info(pluginId, parameterId).contents)

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        return structToDict(self.lib.carla_get_parameter_scalepoint_info(pluginId, parameterId, scalePointId).contents)

    def get_parameter_data(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_data(pluginId, parameterId).contents)

    def get_parameter_ranges(self, pluginId, parameterId):
        return structToDict(self.lib.carla_get_parameter_ranges(pluginId, parameterId).contents)

    def get_midi_program_data(self, pluginId, midiProgramId):
        return structToDict(self.lib.carla_get_midi_program_data(pluginId, midiProgramId).contents)

    def get_custom_data(self, pluginId, customDataId):
        return structToDict(self.lib.carla_get_custom_data(pluginId, customDataId).contents)

    def get_chunk_data(self, pluginId):
        return self.lib.carla_get_chunk_data(pluginId)

    def get_parameter_count(self, pluginId):
        return self.lib.carla_get_parameter_count(pluginId)

    def get_program_count(self, pluginId):
        return self.lib.carla_get_program_count(pluginId)

    def get_midi_program_count(self, pluginId):
        return self.lib.carla_get_midi_program_count(pluginId)

    def get_custom_data_count(self, pluginId):
        return self.lib.carla_get_custom_data_count(pluginId)

    def get_parameter_text(self, pluginId, parameterId):
        return self.lib.carla_get_parameter_text(pluginId, parameterId)

    def get_program_name(self, pluginId, programId):
        return self.lib.carla_get_program_name(pluginId, programId)

    def get_midi_program_name(self, pluginId, midiProgramId):
        return self.lib.carla_get_midi_program_name(pluginId, midiProgramId)

    def get_real_plugin_name(self, pluginId):
        return self.lib.carla_get_real_plugin_name(pluginId)

    def get_current_program_index(self, pluginId):
        return self.lib.carla_get_current_program_index(pluginId)

    def get_current_midi_program_index(self, pluginId):
        return self.lib.carla_get_current_midi_program_index(pluginId)

    def get_default_parameter_value(self, pluginId, parameterId):
        return self.lib.carla_get_default_parameter_value(pluginId, parameterId)

    def get_current_parameter_value(self, pluginId, parameterId):
        return self.lib.carla_get_current_parameter_value(pluginId, parameterId)

    def get_input_peak_value(self, pluginId, portId):
        return self.lib.carla_get_input_peak_value(pluginId, portId)

    def get_output_peak_value(self, pluginId, portId):
        return self.lib.carla_get_output_peak_value(pluginId, portId)

    def set_option(self, pluginId, option, yesNo):
        self.lib.carla_set_option(pluginId, option, yesNo)

    def set_active(self, pluginId, onOff):
        self.lib.carla_set_active(pluginId, onOff)

    def set_drywet(self, pluginId, value):
        self.lib.carla_set_drywet(pluginId, value)

    def set_volume(self, pluginId, value):
        self.lib.carla_set_volume(pluginId, value)

    def set_balance_left(self, pluginId, value):
        self.lib.carla_set_balance_left(pluginId, value)

    def set_balance_right(self, pluginId, value):
        self.lib.carla_set_balance_right(pluginId, value)

    def set_panning(self, pluginId, value):
        self.lib.carla_set_panning(pluginId, value)

    def set_ctrl_channel(self, pluginId, channel):
        self.lib.carla_set_ctrl_channel(pluginId, channel)

    def set_parameter_value(self, pluginId, parameterId, value):
        self.lib.carla_set_parameter_value(pluginId, parameterId, value)

    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        self.lib.carla_set_parameter_midi_cc(pluginId, parameterId, cc)

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        self.lib.carla_set_parameter_midi_channel(pluginId, parameterId, channel)

    def set_program(self, pluginId, programId):
        self.lib.carla_set_program(pluginId, programId)

    def set_midi_program(self, pluginId, midiProgramId):
        self.lib.carla_set_midi_program(pluginId, midiProgramId)

    def set_custom_data(self, pluginId, type_, key, value):
        self.lib.carla_set_custom_data(pluginId, type_.encode("utf-8"), key.encode("utf-8"), value.encode("utf-8"))

    def set_chunk_data(self, pluginId, chunkData):
        self.lib.carla_set_chunk_data(pluginId, chunkData.encode("utf-8"))

    def prepare_for_save(self, pluginId):
        self.lib.carla_prepare_for_save(pluginId)

    def send_midi_note(self, pluginId, channel, note, velocity):
        self.lib.carla_send_midi_note(pluginId, channel, note, velocity)

    def show_gui(self, pluginId, yesNo):
        self.lib.carla_show_gui(pluginId, yesNo)

    def get_last_error(self):
        return self.lib.carla_get_last_error()

    def get_host_osc_url_tcp(self):
        return self.lib.carla_get_host_osc_url_tcp()

    def get_host_osc_url_udp(self):
        return self.lib.carla_get_host_osc_url_udp()

    def get_buffer_size(self):
        return self.lib.carla_get_buffer_size()

    def get_sample_rate(self):
        return self.lib.carla_get_sample_rate()

    def nsm_announce(self, url, appName_, pid):
        self.lib.carla_nsm_announce(url.encode("utf-8"), appName_.encode("utf-8"), pid)

    def nsm_ready(self):
        self.lib.carla_nsm_ready()

    def nsm_reply_open(self):
        self.lib.carla_nsm_reply_open()

    def nsm_reply_save(self):
        self.lib.carla_nsm_reply_save()

    def _init(self, libName):
        self.lib = cdll.LoadLibrary(libName)

        self.lib.carla_get_extended_license_text.argtypes = None
        self.lib.carla_get_extended_license_text.restype = c_char_p

        self.lib.carla_get_supported_file_types.argtypes = None
        self.lib.carla_get_supported_file_types.restype = c_char_p

        self.lib.carla_get_engine_driver_count.argtypes = None
        self.lib.carla_get_engine_driver_count.restype = c_uint

        self.lib.carla_get_engine_driver_name.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_name.restype = c_char_p

        self.lib.carla_get_engine_driver_device_names.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_device_names.restype = POINTER(c_char_p)

        self.lib.carla_get_engine_driver_device_info.argtypes = [c_uint, c_char_p]
        self.lib.carla_get_engine_driver_device_info.restype = POINTER(EngineDriverDeviceInfo)

        self.lib.carla_get_internal_plugin_count.argtypes = None
        self.lib.carla_get_internal_plugin_count.restype = c_uint

        self.lib.carla_get_internal_plugin_info.argtypes = [c_uint]
        self.lib.carla_get_internal_plugin_info.restype = POINTER(CarlaNativePluginInfo)

        self.lib.carla_engine_init.argtypes = [c_char_p, c_char_p]
        self.lib.carla_engine_init.restype = c_bool

        self.lib.carla_engine_close.argtypes = None
        self.lib.carla_engine_close.restype = c_bool

        self.lib.carla_engine_idle.argtypes = None
        self.lib.carla_engine_idle.restype = None

        self.lib.carla_is_engine_running.argtypes = None
        self.lib.carla_is_engine_running.restype = c_bool

        self.lib.carla_set_engine_about_to_close.argtypes = None
        self.lib.carla_set_engine_about_to_close.restype = None

        self.lib.carla_set_engine_callback.argtypes = [EngineCallbackFunc, c_void_p]
        self.lib.carla_set_engine_callback.restype = None

        self.lib.carla_set_engine_option.argtypes = [c_enum, c_int, c_char_p]
        self.lib.carla_set_engine_option.restype = None

        self.lib.carla_load_filename.argtypes = [c_char_p]
        self.lib.carla_load_filename.restype = c_bool

        self.lib.carla_load_project.argtypes = [c_char_p]
        self.lib.carla_load_project.restype = c_bool

        self.lib.carla_save_project.argtypes = [c_char_p]
        self.lib.carla_save_project.restype = c_bool

        self.lib.carla_patchbay_connect.argtypes = [c_int, c_int]
        self.lib.carla_patchbay_connect.restype = c_bool

        self.lib.carla_patchbay_disconnect.argtypes = [c_int]
        self.lib.carla_patchbay_disconnect.restype = c_bool

        self.lib.carla_patchbay_refresh.argtypes = None
        self.lib.carla_patchbay_refresh.restype = c_bool

        self.lib.carla_transport_play.argtypes = None
        self.lib.carla_transport_play.restype = None

        self.lib.carla_transport_pause.argtypes = None
        self.lib.carla_transport_pause.restype = None

        self.lib.carla_transport_relocate.argtypes = [c_uint32]
        self.lib.carla_transport_relocate.restype = None

        self.lib.carla_get_current_transport_frame.argtypes = None
        self.lib.carla_get_current_transport_frame.restype = c_uint64

        self.lib.carla_get_transport_info.argtypes = None
        self.lib.carla_get_transport_info.restype = POINTER(CarlaTransportInfo)

        self.lib.carla_add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_char_p, c_void_p]
        self.lib.carla_add_plugin.restype = c_bool

        self.lib.carla_remove_plugin.argtypes = [c_uint]
        self.lib.carla_remove_plugin.restype = c_bool

        self.lib.carla_remove_all_plugins.argtypes = None
        self.lib.carla_remove_all_plugins.restype = c_bool

        self.lib.carla_rename_plugin.argtypes = [c_uint, c_char_p]
        self.lib.carla_rename_plugin.restype = c_char_p

        self.lib.carla_clone_plugin.argtypes = [c_uint]
        self.lib.carla_clone_plugin.restype = c_bool

        self.lib.carla_replace_plugin.argtypes = [c_uint]
        self.lib.carla_replace_plugin.restype = c_bool

        self.lib.carla_switch_plugins.argtypes = [c_uint, c_uint]
        self.lib.carla_switch_plugins.restype = c_bool

        self.lib.carla_load_plugin_state.argtypes = [c_uint, c_char_p]
        self.lib.carla_load_plugin_state.restype = c_bool

        self.lib.carla_save_plugin_state.argtypes = [c_uint, c_char_p]
        self.lib.carla_save_plugin_state.restype = c_bool

        self.lib.carla_get_plugin_info.argtypes = [c_uint]
        self.lib.carla_get_plugin_info.restype = POINTER(CarlaPluginInfo)

        self.lib.carla_get_audio_port_count_info.argtypes = [c_uint]
        self.lib.carla_get_audio_port_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_midi_port_count_info.argtypes = [c_uint]
        self.lib.carla_get_midi_port_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_parameter_count_info.argtypes = [c_uint]
        self.lib.carla_get_parameter_count_info.restype = POINTER(CarlaPortCountInfo)

        self.lib.carla_get_parameter_info.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_info.restype = POINTER(CarlaParameterInfo)

        self.lib.carla_get_parameter_scalepoint_info.argtypes = [c_uint, c_uint32, c_uint32]
        self.lib.carla_get_parameter_scalepoint_info.restype = POINTER(CarlaScalePointInfo)

        self.lib.carla_get_parameter_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_data.restype = POINTER(ParameterData)

        self.lib.carla_get_parameter_ranges.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_ranges.restype = POINTER(ParameterRanges)

        self.lib.carla_get_midi_program_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_midi_program_data.restype = POINTER(MidiProgramData)

        self.lib.carla_get_custom_data.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_custom_data.restype = POINTER(CustomData)

        self.lib.carla_get_chunk_data.argtypes = [c_uint]
        self.lib.carla_get_chunk_data.restype = c_char_p

        self.lib.carla_get_parameter_count.argtypes = [c_uint]
        self.lib.carla_get_parameter_count.restype = c_uint32

        self.lib.carla_get_program_count.argtypes = [c_uint]
        self.lib.carla_get_program_count.restype = c_uint32

        self.lib.carla_get_midi_program_count.argtypes = [c_uint]
        self.lib.carla_get_midi_program_count.restype = c_uint32

        self.lib.carla_get_custom_data_count.argtypes = [c_uint]
        self.lib.carla_get_custom_data_count.restype = c_uint32

        self.lib.carla_get_parameter_text.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_parameter_text.restype = c_char_p

        self.lib.carla_get_program_name.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_program_name.restype = c_char_p

        self.lib.carla_get_midi_program_name.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_midi_program_name.restype = c_char_p

        self.lib.carla_get_real_plugin_name.argtypes = [c_uint]
        self.lib.carla_get_real_plugin_name.restype = c_char_p

        self.lib.carla_get_current_program_index.argtypes = [c_uint]
        self.lib.carla_get_current_program_index.restype = c_int32

        self.lib.carla_get_current_midi_program_index.argtypes = [c_uint]
        self.lib.carla_get_current_midi_program_index.restype = c_int32

        self.lib.carla_get_default_parameter_value.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_default_parameter_value.restype = c_float

        self.lib.carla_get_current_parameter_value.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_current_parameter_value.restype = c_float

        self.lib.carla_get_input_peak_value.argtypes = [c_uint, c_ushort]
        self.lib.carla_get_input_peak_value.restype = c_float

        self.lib.carla_get_output_peak_value.argtypes = [c_uint, c_ushort]
        self.lib.carla_get_output_peak_value.restype = c_float

        self.lib.carla_set_option.argtypes = [c_uint, c_uint, c_bool]
        self.lib.carla_set_option.restype = None

        self.lib.carla_set_active.argtypes = [c_uint, c_bool]
        self.lib.carla_set_active.restype = None

        self.lib.carla_set_drywet.argtypes = [c_uint, c_float]
        self.lib.carla_set_drywet.restype = None

        self.lib.carla_set_volume.argtypes = [c_uint, c_float]
        self.lib.carla_set_volume.restype = None

        self.lib.carla_set_balance_left.argtypes = [c_uint, c_float]
        self.lib.carla_set_balance_left.restype = None

        self.lib.carla_set_balance_right.argtypes = [c_uint, c_float]
        self.lib.carla_set_balance_right.restype = None

        self.lib.carla_set_panning.argtypes = [c_uint, c_float]
        self.lib.carla_set_panning.restype = None

        self.lib.carla_set_ctrl_channel.argtypes = [c_uint, c_int8]
        self.lib.carla_set_ctrl_channel.restype = None

        self.lib.carla_set_parameter_value.argtypes = [c_uint, c_uint32, c_float]
        self.lib.carla_set_parameter_value.restype = None

        self.lib.carla_set_parameter_midi_cc.argtypes = [c_uint, c_uint32, c_int16]
        self.lib.carla_set_parameter_midi_cc.restype = None

        self.lib.carla_set_parameter_midi_channel.argtypes = [c_uint, c_uint32, c_uint8]
        self.lib.carla_set_parameter_midi_channel.restype = None

        self.lib.carla_set_program.argtypes = [c_uint, c_uint32]
        self.lib.carla_set_program.restype = None

        self.lib.carla_set_midi_program.argtypes = [c_uint, c_uint32]
        self.lib.carla_set_midi_program.restype = None

        self.lib.carla_set_custom_data.argtypes = [c_uint, c_char_p, c_char_p, c_char_p]
        self.lib.carla_set_custom_data.restype = None

        self.lib.carla_set_chunk_data.argtypes = [c_uint, c_char_p]
        self.lib.carla_set_chunk_data.restype = None

        self.lib.carla_prepare_for_save.argtypes = [c_uint]
        self.lib.carla_prepare_for_save.restype = None

        self.lib.carla_send_midi_note.argtypes = [c_uint, c_uint8, c_uint8, c_uint8]
        self.lib.carla_send_midi_note.restype = None

        self.lib.carla_show_gui.argtypes = [c_uint, c_bool]
        self.lib.carla_show_gui.restype = None

        self.lib.carla_get_buffer_size.argtypes = None
        self.lib.carla_get_buffer_size.restype = c_uint32

        self.lib.carla_get_sample_rate.argtypes = None
        self.lib.carla_get_sample_rate.restype = c_double

        self.lib.carla_get_last_error.argtypes = None
        self.lib.carla_get_last_error.restype = c_char_p

        self.lib.carla_get_host_osc_url_tcp.argtypes = None
        self.lib.carla_get_host_osc_url_tcp.restype = c_char_p

        self.lib.carla_get_host_osc_url_udp.argtypes = None
        self.lib.carla_get_host_osc_url_udp.restype = c_char_p

        self.lib.carla_nsm_announce.argtypes = [c_char_p, c_char_p, c_int]
        self.lib.carla_nsm_announce.restype = None

        self.lib.carla_nsm_ready.argtypes = None
        self.lib.carla_nsm_ready.restype = None

        self.lib.carla_nsm_reply_open.argtypes = None
        self.lib.carla_nsm_reply_open.restype = None

        self.lib.carla_nsm_reply_save.argtypes = None
        self.lib.carla_nsm_reply_save.restype = None
