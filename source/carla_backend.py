#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
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
# Imports (Global)

from abc import ABCMeta, abstractmethod
from copy import deepcopy
from ctypes import *
from platform import architecture
from sys import platform, maxsize

# ------------------------------------------------------------------------------------------------------------
# 64bit check

kIs64bit = bool(architecture()[0] == "64bit" and maxsize > 2**32)

# ------------------------------------------------------------------------------------------------------------
# Define custom types

c_enum = c_int
c_uintptr = c_uint64 if kIs64bit else c_uint32

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

def charPtrToString(charPtr):
    if not charPtr:
        return ""
    if isinstance(charPtr, str):
        return charPtr
    return charPtr.decode("utf-8", errors="ignore")

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
    num     = numPtr[0] #.value
    numList = []

    while num not in (0, 0.0):
        numList.append(num)

        i += 1
        num = numPtr[i] #.value

    return numList

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes value into a python one

c_int_types    = (c_int, c_int8, c_int16, c_int32, c_int64, c_uint, c_uint8, c_uint16, c_uint32, c_uint64, c_long, c_longlong)
c_float_types  = (c_float, c_double, c_longdouble)
c_intp_types   = tuple(POINTER(i) for i in c_int_types)
c_floatp_types = tuple(POINTER(i) for i in c_float_types)

def toPythonType(value, attr):
    if isinstance(value, (bool, int, float)):
        return value
    if isinstance(value, bytes):
        return charPtrToString(value)
    if isinstance(value, c_intp_types) or isinstance(value, c_floatp_types):
        return numPtrToList(value)
    if isinstance(value, POINTER(c_char_p)):
        return charPtrPtrToStringList(value)
    print("..............", attr, ".....................", value, ":", type(value))
    return value

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict

def structToDict(struct):
    return dict((attr, toPythonType(getattr(struct, attr), attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------------------
# Carla Backend API (base definitions)

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
# Various engine driver device hints.
# @see carla_get_engine_driver_device_info()

# Engine driver device has custom control-panel.
ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL = 0x1

# Engine driver device can use a triple-buffer (3 number of periods instead of the usual 2).
# @see ENGINE_OPTION_AUDIO_NUM_PERIODS
ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER = 0x2

# Engine driver device can change buffer-size on the fly.
# @see ENGINE_OPTION_AUDIO_BUFFER_SIZE
ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE = 0x4

# Engine driver device can change sample-rate on the fly.
# @see ENGINE_OPTION_AUDIO_SAMPLE_RATE
ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE = 0x8

# ------------------------------------------------------------------------------------------------------------
# Plugin Hints
# Various plugin hints.
# @see carla_get_plugin_info()

# Plugin is a bridge.
# This hint is required because "bridge" itself is not a plugin type.
PLUGIN_IS_BRIDGE = 0x001

# Plugin is hard real-time safe.
PLUGIN_IS_RTSAFE = 0x002

# Plugin is a synth (produces sound).
PLUGIN_IS_SYNTH = 0x004

# Plugin has its own custom UI.
# @see carla_show_custom_ui()
PLUGIN_HAS_CUSTOM_UI = 0x008

# Plugin can use internal Dry/Wet control.
PLUGIN_CAN_DRYWET = 0x010

# Plugin can use internal Volume control.
PLUGIN_CAN_VOLUME = 0x020

# Plugin can use internal (Stereo) Balance controls.
PLUGIN_CAN_BALANCE = 0x040

# Plugin can use internal (Mono) Panning control.
PLUGIN_CAN_PANNING = 0x080

# Plugin needs a constant, fixed-size audio buffer.
PLUGIN_NEEDS_FIXED_BUFFERS = 0x100

# Plugin needs to receive all UI events in the main thread.
PLUGIN_NEEDS_UI_MAIN_THREAD = 0x200

# Plugin uses 1 program per MIDI channel.
# @note: Only used in some internal plugins and gig+sf2 files.
PLUGIN_USES_MULTI_PROGS = 0x400

# ------------------------------------------------------------------------------------------------------------
# Plugin Options
# Various plugin options.
# @see carla_get_plugin_info() and carla_set_option()

# Use constant/fixed-size audio buffers.
PLUGIN_OPTION_FIXED_BUFFERS = 0x001

# Force mono plugin as stereo.
PLUGIN_OPTION_FORCE_STEREO = 0x002

# Map MIDI programs to plugin programs.
PLUGIN_OPTION_MAP_PROGRAM_CHANGES = 0x004

# Use chunks to save and restore data instead of parameter values.
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

# Send MIDI bank/program changes.
# @note: This option conflicts with PLUGIN_OPTION_MAP_PROGRAM_CHANGES and cannot be used at the same time.
PLUGIN_OPTION_SEND_PROGRAM_CHANGES = 0x200

# ------------------------------------------------------------------------------------------------------------
# Parameter Hints
# Various parameter hints.
# @see CarlaPlugin::getParameterData() and carla_get_parameter_data()

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
# Various patchbay port hints.

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
# These types define how the value in the CustomData struct is stored.
# @see CustomData.type

# Boolean string type URI.
# Only "true" and "false" are valid values.
CUSTOM_DATA_TYPE_BOOLEAN = "http://kxstudio.sf.net/ns/carla/boolean"

# Chunk type URI.
CUSTOM_DATA_TYPE_CHUNK = "http://kxstudio.sf.net/ns/carla/chunk"

# Property type URI.
CUSTOM_DATA_TYPE_PROPERTY = "http://kxstudio.sf.net/ns/carla/property"

# String type URI.
CUSTOM_DATA_TYPE_STRING = "http://kxstudio.sf.net/ns/carla/string"

# ------------------------------------------------------------------------------------------------------------
# Custom Data Keys
# Pre-defined keys used internally in Carla.
# @see CustomData.key

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
# The binary type of a plugin.

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
# Plugin type.
# Some files are handled as if they were plugins.

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

# VST2 plugin.
PLUGIN_VST2 = 5

# VST3 plugin.
PLUGIN_VST3 = 6

# AU plugin.
# @note MacOS only
PLUGIN_AU = 7

# GIG file.
PLUGIN_GIG = 8

# SF2 file (SoundFont).
PLUGIN_SF2 = 9

# SFZ file.
PLUGIN_SFZ = 10

# ------------------------------------------------------------------------------------------------------------
# Plugin Category
# Plugin category, which describes the functionality of a plugin.

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
# Plugin parameter type.

# Null parameter type.
PARAMETER_UNKNOWN = 0

# Input parameter.
PARAMETER_INPUT = 1

# Ouput parameter.
PARAMETER_OUTPUT = 2

# ------------------------------------------------------------------------------------------------------------
# Internal Parameter Index
# Special parameters used internally in Carla.
# Plugins do not know about their existence.

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
# Engine callback opcodes.
# Front-ends must never block indefinitely during a callback.
# @see EngineCallbackFunc and carla_set_engine_callback()

# Debug.
# This opcode is undefined and used only for testing purposes.
ENGINE_CALLBACK_DEBUG = 0

# A plugin has been added.
# @a pluginId Plugin Id
# @a valueStr Plugin name
ENGINE_CALLBACK_PLUGIN_ADDED = 1

# A plugin has been removed.
# @a pluginId Plugin Id
ENGINE_CALLBACK_PLUGIN_REMOVED = 2

# A plugin has been renamed.
# @a pluginId Plugin Id
# @a valueStr New plugin name
ENGINE_CALLBACK_PLUGIN_RENAMED = 3

# A plugin has become unavailable.
# @a pluginId Plugin Id
# @a valueStr Related error string
ENGINE_CALLBACK_PLUGIN_UNAVAILABLE = 4

# A parameter value has changed.
# @a pluginId Plugin Id
# @a value1   Parameter index
# @a value3   New parameter value
ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED = 5

# A parameter default has changed.
# @a pluginId Plugin Id
# @a value1   Parameter index
# @a value3   New default value
ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED = 6

# A parameter's MIDI CC has changed.
# @a pluginId Plugin Id
# @a value1   Parameter index
# @a value2   New MIDI CC
ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED = 7

# A parameter's MIDI channel has changed.
# @a pluginId Plugin Id
# @a value1   Parameter index
# @a value2   New MIDI channel
ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 8

# A plugin option has changed.
# @a pluginId Plugin Id
# @a value1   Option
# @a value2   New on/off state (1 for on, 0 for off)
# @see PluginOptions
ENGINE_CALLBACK_OPTION_CHANGED = 9

# The current program of a plugin has changed.
# @a pluginId Plugin Id
# @a value1   New program index
ENGINE_CALLBACK_PROGRAM_CHANGED = 10

# The current MIDI program of a plugin has changed.
# @a pluginId Plugin Id
# @a value1   New MIDI program index
ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED = 11

# A plugin's custom UI state has changed.
# @a pluginId Plugin Id
# @a value1   New state, as follows:
#                  0: UI is now hidden
#                  1: UI is now visible
#                 -1: UI has crashed and should not be shown again
ENGINE_CALLBACK_UI_STATE_CHANGED = 12

# A note has been pressed.
# @a pluginId Plugin Id
# @a value1   Channel
# @a value2   Note
# @a value3   Velocity
ENGINE_CALLBACK_NOTE_ON = 13

# A note has been released.
# @a pluginId Plugin Id
# @a value1   Channel
# @a value2   Note
ENGINE_CALLBACK_NOTE_OFF = 14

# A plugin needs update.
# @a pluginId Plugin Id
ENGINE_CALLBACK_UPDATE = 15

# A plugin's data/information has changed.
# @a pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_INFO = 16

# A plugin's parameters have changed.
# @a pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_PARAMETERS = 17

# A plugin's programs have changed.
# @a pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_PROGRAMS = 18

# A plugin state has changed.
# @a pluginId Plugin Id
ENGINE_CALLBACK_RELOAD_ALL = 19

# A patchbay client has been added.
# @a pluginId Client Id
# @a value1   Client icon
# @a value2   Plugin Id (-1 if not a plugin)
# @a valueStr Client name
# @see PatchbayIcon
ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED = 20

# A patchbay client has been removed.
# @a pluginId Client Id
ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED = 21

# A patchbay client has been renamed.
# @a pluginId Client Id
# @a valueStr New client name
ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED = 22

# A patchbay client data has changed.
# @a pluginId Client Id
# @a value1   New icon
# @a value2   New plugin Id (-1 if not a plugin)
# @see PatchbayIcon
ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED = 23

# A patchbay port has been added.
# @a pluginId Client Id
# @a value1   Port Id
# @a value2   Port hints
# @a valueStr Port name
# @see PatchbayPortHints
ENGINE_CALLBACK_PATCHBAY_PORT_ADDED = 24

# A patchbay port has been removed.
# @a pluginId Client Id
# @a value1   Port Id
ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED = 25

# A patchbay port has been renamed.
# @a pluginId Client Id
# @a value1   Port Id
# @a valueStr New port name
ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED = 26

# A patchbay connection has been added.
# @a pluginId Connection Id
# @a valueStr Out group, port plus in group and port, in "og:op:ig:ip" syntax.
ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED = 27

# A patchbay connection has been removed.
# @a pluginId Connection Id
ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED = 28

# Engine started.
# @a value1   Process mode
# @a value2   Transport mode
# @a valuestr Engine driver
# @see EngineProcessMode
# @see EngineTransportMode
ENGINE_CALLBACK_ENGINE_STARTED = 29

# Engine stopped.
ENGINE_CALLBACK_ENGINE_STOPPED = 30

# Engine process mode has changed.
# @a value1 New process mode
# @see EngineProcessMode
ENGINE_CALLBACK_PROCESS_MODE_CHANGED = 31

# Engine transport mode has changed.
# @a value1 New transport mode
# @see EngineTransportMode
ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED = 32

# Engine buffer-size changed.
# @a value1 New buffer size
ENGINE_CALLBACK_BUFFER_SIZE_CHANGED = 33

# Engine sample-rate changed.
# @a value3 New sample rate
ENGINE_CALLBACK_SAMPLE_RATE_CHANGED = 34

# NSM callback.
# (Work in progress, values are not defined yet)
ENGINE_CALLBACK_NSM = 35

# Idle frontend.
# This is used by the engine during long operations that might block the frontend,
# giving it the possibility to idle while the operation is still in place.
ENGINE_CALLBACK_IDLE = 36

# Show a message as information.
# @a valueStr The message
ENGINE_CALLBACK_INFO = 37

# Show a message as an error.
# @a valueStr The message
ENGINE_CALLBACK_ERROR = 38

# The engine has crashed or malfunctioned and will no longer work.
ENGINE_CALLBACK_QUIT = 39

# ------------------------------------------------------------------------------------------------------------
# Engine Option
# Engine options.
# @see carla_set_engine_option()

# Debug.
# This option is undefined and used only for testing purposes.
ENGINE_OPTION_DEBUG = 0

# Set the engine processing mode.
# Default is ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS on Linux and ENGINE_PROCESS_MODE_PATCHBAY for all other OSes.
# @see EngineProcessMode
ENGINE_OPTION_PROCESS_MODE = 1

# Set the engine transport mode.
# Default is ENGINE_TRANSPORT_MODE_JACK on Linux and ENGINE_TRANSPORT_MODE_INTERNAL for all other OSes.
# @see EngineTransportMode
ENGINE_OPTION_TRANSPORT_MODE = 2

# Force mono plugins as stereo, by running 2 instances at the same time.
# Default is false, but always true when process mode is ENGINE_PROCESS_MODE_CONTINUOUS_RACK.
# @note Not supported by all plugins
# @see PLUGIN_OPTION_FORCE_STEREO
ENGINE_OPTION_FORCE_STEREO = 3

# Use plugin bridges whenever possible.
# Default is no, EXPERIMENTAL.
ENGINE_OPTION_PREFER_PLUGIN_BRIDGES = 4

# Use UI bridges whenever possible, otherwise UIs will be directly handled in the main backend thread.
# Default is yes.
ENGINE_OPTION_PREFER_UI_BRIDGES = 5

# Make custom plugin UIs always-on-top.
# Default is yes.
ENGINE_OPTION_UIS_ALWAYS_ON_TOP = 6

# Maximum number of parameters allowed.
# Default is MAX_DEFAULT_PARAMETERS.
ENGINE_OPTION_MAX_PARAMETERS = 7

# Timeout value for how much to wait for UI bridges to respond, in milliseconds.
# Default is 4000 (4 seconds).
ENGINE_OPTION_UI_BRIDGES_TIMEOUT = 8

# Number of audio periods.
# Default is 2.
ENGINE_OPTION_AUDIO_NUM_PERIODS = 9

# Audio buffer size.
# Default is 512.
ENGINE_OPTION_AUDIO_BUFFER_SIZE = 10

# Audio sample rate.
# Default is 44100.
ENGINE_OPTION_AUDIO_SAMPLE_RATE = 11

# Audio device (within a driver).
# Default unset.
ENGINE_OPTION_AUDIO_DEVICE = 12

# Set path used for a specific plugin type.
# Uses value as the plugin format, valueStr as actual path.
# @see PluginType
ENGINE_OPTION_PLUGIN_PATH = 13

# Set path to the binary files.
# Default unset.
# @note Must be set for plugin and UI bridges to work
ENGINE_OPTION_PATH_BINARIES = 14

# Set path to the resource files.
# Default unset.
# @note Must be set for some internal plugins to work
ENGINE_OPTION_PATH_RESOURCES = 15

# Prevent bad plugin and UI behaviour.
# @note: Linux only
ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR = 16

# Set frontend winId, used to define as parent window for plugin UIs.
ENGINE_OPTION_FRONTEND_WIN_ID = 17

# ------------------------------------------------------------------------------------------------------------
# Engine Process Mode
# Engine process mode.
# @see ENGINE_OPTION_PROCESS_MODE

# Single client mode.
# Inputs and outputs are added dynamically as needed by plugins.
ENGINE_PROCESS_MODE_SINGLE_CLIENT = 0

# Multiple client mode.
# It has 1 master client + 1 client per plugin.
ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS = 1

# Single client, 'rack' mode.
# Processes plugins in order of Id, with forced stereo always on.
ENGINE_PROCESS_MODE_CONTINUOUS_RACK = 2

# Single client, 'patchbay' mode.
ENGINE_PROCESS_MODE_PATCHBAY = 3

# Special mode, used in plugin-bridges only.
ENGINE_PROCESS_MODE_BRIDGE = 4

# ------------------------------------------------------------------------------------------------------------
# Engine Transport Mode
# Engine transport mode.
# @see ENGINE_OPTION_TRANSPORT_MODE

# Internal transport mode.
ENGINE_TRANSPORT_MODE_INTERNAL = 0

# Transport from JACK.
# Only available if driver name is "JACK".
ENGINE_TRANSPORT_MODE_JACK = 1

# Transport from host, used when Carla is a plugin.
ENGINE_TRANSPORT_MODE_PLUGIN = 2

# Special mode, used in plugin-bridges only.
ENGINE_TRANSPORT_MODE_BRIDGE = 3

# ------------------------------------------------------------------------------------------------------------
# File Callback Opcode
# File callback opcodes.
# Front-ends must always block-wait for user input.
# @see FileCallbackFunc and carla_set_file_callback()

# Debug.
# This opcode is undefined and used only for testing purposes.
FILE_CALLBACK_DEBUG = 0

# Open file or folder.
FILE_CALLBACK_OPEN = 1

# Save file or folder.
FILE_CALLBACK_SAVE = 2

# ------------------------------------------------------------------------------------------------------------
# Patchbay Icon
# The icon of a patchbay client/group.

# Generic application icon.
# Used for all non-plugin clients that don't have a specific icon.
PATCHBAY_ICON_APPLICATION = 0

# Plugin icon.
# Used for all plugin clients that don't have a specific icon.
PATCHBAY_ICON_PLUGIN = 1

# Hardware icon.
# Used for hardware (audio or MIDI) clients.
PATCHBAY_ICON_HARDWARE = 2

# Carla icon.
# Used for the main app.
PATCHBAY_ICON_CARLA = 3

# DISTRHO icon.
# Used for DISTRHO based plugins.
PATCHBAY_ICON_DISTRHO = 4

# File icon.
# Used for file type plugins (like GIG and SF2).
PATCHBAY_ICON_FILE = 5

# ------------------------------------------------------------------------------------------------------------
# Carla Backend API (C stuff)

# Engine callback function.
# Front-ends must never block indefinitely during a callback.
# @see EngineCallbackOpcode and carla_set_engine_callback()
EngineCallbackFunc = CFUNCTYPE(None, c_void_p, c_enum, c_uint, c_int, c_int, c_float, c_char_p)

# File callback function.
# @see FileCallbackOpcode
FileCallbackFunc = CFUNCTYPE(c_char_p, c_void_p, c_enum, c_bool, c_char_p, c_char_p)

# Parameter data.
class ParameterData(Structure):
    _fields_ = [
        # This parameter type.
        ("type", c_enum),

        # This parameter hints.
        # @see ParameterHints
        ("hints", c_uint),

        # Index as seen by Carla.
        ("index", c_int32),

        # Real index as seen by plugins.
        ("rindex", c_int32),

        # Currently mapped MIDI CC.
        # A value lower than 0 means invalid or unused.
        # Maximum allowed value is 119 (0x77).
        ("midiCC", c_int16),

        # Currently mapped MIDI channel.
        # Counts from 0 to 15.
        ("midiChannel", c_uint8)
    ]

# Parameter ranges.
class ParameterRanges(Structure):
    _fields_ = [
        # Default value.
        ("def", c_float),

        # Minimum value.
        ("min", c_float),

        # Maximum value.
        ("max", c_float),

        # Regular, single step value.
        ("step", c_float),

        # Small step value.
        ("stepSmall", c_float),

        # Large step value.
        ("stepLarge", c_float)
    ]

# MIDI Program data.
class MidiProgramData(Structure):
    _fields_ = [
        # MIDI bank.
        ("bank", c_uint32),

        # MIDI program.
        ("program", c_uint32),

        # MIDI program name.
        ("name", c_char_p)
    ]

# Custom data, used for saving key:value 'dictionaries'.
class CustomData(Structure):
    _fields_ = [
        # Value type, in URI form.
        # @see CustomDataTypes
        ("type", c_char_p),

        # Key.
        # @see CustomDataKeys
        ("key", c_char_p),

        # Value.
        ("value", c_char_p)
    ]

# Engine driver device information.
class EngineDriverDeviceInfo(Structure):
    _fields_ = [
        # This driver device hints.
        # @see EngineDriverHints
        ("hints", c_uint),

        # Available buffer sizes.
        # Terminated with 0.
        ("bufferSizes", POINTER(c_uint32)),

        # Available sample rates.
        # Terminated with 0.0.
        ("sampleRates", POINTER(c_double))
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Backend API (Python compatible stuff)

# @see ParameterData
PyParameterData = {
    'type': PARAMETER_UNKNOWN,
    'hints': 0x0,
    'index': PARAMETER_NULL,
    'rindex': -1,
    'midiCC': -1,
    'midiChannel': 0
}

# @see ParameterRanges
PyParameterRanges = {
    'def': 0.0,
    'min': 0.0,
    'max': 1.0,
    'step': 0.01,
    'stepSmall': 0.0001,
    'stepLarge': 0.1
}

# @see MidiProgramData
PyMidiProgramData = {
    'bank': 0,
    'program': 0,
    'name': None
}

# @see CustomData
PyCustomData = {
    'type': None,
    'key': None,
    'value': None
}

# @see EngineDriverDeviceInfo
PyEngineDriverDeviceInfo = {
    'hints': 0x0,
    'bufferSizes': [],
    'sampleRates': []
}

# ------------------------------------------------------------------------------------------------------------
# Carla Host API (C stuff)

# Information about a loaded plugin.
# @see carla_get_plugin_info()
class CarlaPluginInfo(Structure):
    _fields_ = [
        # Plugin type.
        ("type", c_enum),

        # Plugin category.
        ("category", c_enum),

        # Plugin hints.
        # @see PluginHints
        ("hints", c_uint),

        # Plugin options available for the user to change.
        # @see PluginOptions
        ("optionsAvailable", c_uint),

        # Plugin options currently enabled.
        # Some options are enabled but not available, which means they will always be on.
        # @see PluginOptions
        ("optionsEnabled", c_uint),

        # Plugin filename.
        # This can be the plugin binary or resource file.
        ("filename", c_char_p),

        # Plugin name.
        # This name is unique within a Carla instance.
        # @see carla_get_real_plugin_name()
        ("name", c_char_p),

        # Plugin label or URI.
        ("label", c_char_p),

        # Plugin author/maker.
        ("maker", c_char_p),

        # Plugin copyright/license.
        ("copyright", c_char_p),

        # Icon name for this plugin, in lowercase.
        # Default is "plugin".
        ("iconName", c_char_p),

        # Plugin unique Id.
        # This Id is dependant on the plugin type and may sometimes be 0.
        ("uniqueId", c_int64)
    ]

# Port count information, used for Audio and MIDI ports and parameters.
# @see carla_get_audio_port_count_info()
# @see carla_get_midi_port_count_info()
# @see carla_get_parameter_count_info()
class CarlaPortCountInfo(Structure):
    _fields_ = [
        # Number of inputs.
        ("ins", c_uint32),

        # Number of outputs.
        ("outs", c_uint32)
    ]

# Parameter information.
# @see carla_get_parameter_info()
class CarlaParameterInfo(Structure):
    _fields_ = [
        # Parameter name.
        ("name", c_char_p),

        # Parameter symbol.
        ("symbol", c_char_p),

        # Parameter unit.
        ("unit", c_char_p),

        # Number of scale points.
        # @see CarlaScalePointInfo
        ("scalePointCount", c_uint32)
    ]

# Parameter scale point information.
# @see carla_get_parameter_scalepoint_info()
class CarlaScalePointInfo(Structure):
    _fields_ = [
        # Scale point value.
        ("value", c_float),

        # Scale point label.
        ("label", c_char_p)
    ]

# Transport information.
# @see carla_get_transport_info()
class CarlaTransportInfo(Structure):
    _fields_ = [
        # Wherever transport is playing.
        ("playing", c_bool),

        # Current transport frame.
        ("frame", c_uint64),

        # Bar
        ("bar", c_int32),

        # Beat
        ("beat", c_int32),

        # Tick
        ("tick", c_int32),

        # Beats per minute.
        ("bpm", c_double)
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Host API (Python compatible stuff)

# @see CarlaPluginInfo
PyCarlaPluginInfo = {
    'type': PLUGIN_NONE,
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'optionsAvailable': 0x0,
    'optionsEnabled': 0x0,
    'filename': "",
    'name':  "",
    'label': "",
    'maker': "",
    'copyright': "",
    'iconName': "",
    'uniqueId': 0
}

# @see CarlaPortCountInfo
PyCarlaPortCountInfo = {
    'ins': 0,
    'outs': 0
}

# @see CarlaParameterInfo
PyCarlaParameterInfo = {
    'name': "",
    'symbol': "",
    'unit': "",
    'scalePointCount': 0,
}

# @see CarlaScalePointInfo
PyCarlaScalePointInfo = {
    'value': 0.0,
    'label': ""
}

# @see CarlaTransportInfo
PyCarlaTransportInfo = {
    "playing": False,
    "frame": 0,
    "bar": 0,
    "beat": 0,
    "tick": 0,
    "bpm": 0.0
}

# ------------------------------------------------------------------------------------------------------------
# Set BINARY_NATIVE

if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if kIs64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if kIs64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Carla Host object (Meta)

class CarlaHostMeta(object):
#class CarlaHostMeta(object, metaclass=ABCMeta):
    def __init__(self):
        object.__init__(self)

        # info about this host object
        self.isControl = False
        self.isPlugin  = False
        self.nsmOK     = False

        # settings
        self.processMode       = ENGINE_PROCESS_MODE_PATCHBAY
        self.transportMode     = ENGINE_TRANSPORT_MODE_INTERNAL
        self.nextProcessMode   = ENGINE_PROCESS_MODE_PATCHBAY
        self.processModeForced = False

        # settings
        self.forceStereo         = False
        self.preferPluginBridges = False
        self.preferUIBridges     = False
        self.preventBadBehaviour = False
        self.uisAlwaysOnTop      = False
        self.maxParameters       = 0
        self.uiBridgesTimeout    = 0

        # settings
        self.pathBinaries  = ""
        self.pathResources = ""

    # Get how many engine drivers are available.
    @abstractmethod
    def get_engine_driver_count(self):
        raise NotImplementedError

    # Get an engine driver name.
    # @param index Driver index
    @abstractmethod
    def get_engine_driver_name(self, index):
        raise NotImplementedError

    # Get the device names of an engine driver.
    # @param index Driver index
    @abstractmethod
    def get_engine_driver_device_names(self, index):
        raise NotImplementedError

    # Get information about a device driver.
    # @param index Driver index
    # @param name  Device name
    @abstractmethod
    def get_engine_driver_device_info(self, index, name):
        raise NotImplementedError

    # Initialize the engine.
    # Make sure to call carla_engine_idle() at regular intervals afterwards.
    # @param driverName Driver to use
    # @param clientName Engine master client name
    @abstractmethod
    def engine_init(self, driverName, clientName):
        raise NotImplementedError

    # Close the engine.
    # This function always closes the engine even if it returns false.
    # In other words, even when something goes wrong when closing the engine it still be closed nonetheless.
    @abstractmethod
    def engine_close(self):
        raise NotImplementedError

    # Idle the engine.
    # Do not call this if the engine is not running.
    @abstractmethod
    def engine_idle(self):
        raise NotImplementedError

    # Check if the engine is running.
    @abstractmethod
    def is_engine_running(self):
        raise NotImplementedError

    # Tell the engine it's about to close.
    # This is used to prevent the engine thread(s) from reactivating.
    @abstractmethod
    def set_engine_about_to_close(self):
        raise NotImplementedError

    # Set the engine callback function.
    # @param func Callback function
    @abstractmethod
    def set_engine_callback(self, func):
        raise NotImplementedError

    # Set an engine option.
    # @param option   Option
    # @param value    Value as number
    # @param valueStr Value as string
    @abstractmethod
    def set_engine_option(self, option, value, valueStr):
        raise NotImplementedError

    # Set the file callback function.
    # @param func Callback function
    # @param ptr  Callback pointer
    @abstractmethod
    def set_file_callback(self, func):
        raise NotImplementedError

    # Load a file of any type.
    # This will try to load a generic file as a plugin,
    # either by direct handling (GIG, SF2 and SFZ) or by using an internal plugin (like Audio and MIDI).
    # @see carla_get_supported_file_extensions()
    @abstractmethod
    def load_file(self, filename):
        raise NotImplementedError

    # Load a Carla project file.
    # @note Currently loaded plugins are not removed; call carla_remove_all_plugins() first if needed.
    @abstractmethod
    def load_project(self, filename):
        raise NotImplementedError

    # Save current project to a file.
    @abstractmethod
    def save_project(self, filename):
        raise NotImplementedError

    # Connect two patchbay ports.
    # @param groupIdA Output group
    # @param portIdA  Output port
    # @param groupIdB Input group
    # @param portIdB  Input port
    # @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED
    @abstractmethod
    def patchbay_connect(self, groupIdA, portIdA, groupIdB, portIdB):
        raise NotImplementedError

    # Disconnect two patchbay ports.
    # @param connectionId Connection Id
    # @see ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED
    @abstractmethod
    def patchbay_disconnect(self, connectionId):
        raise NotImplementedError

    # Force the engine to resend all patchbay clients, ports and connections again.
    # @param external Wherever to show external/hardware ports instead of internal ones.
    #                 Only valid in patchbay engine mode, other modes will ignore this.
    @abstractmethod
    def patchbay_refresh(self, external):
        raise NotImplementedError

    # Start playback of the engine transport.
    @abstractmethod
    def transport_play(self):
        raise NotImplementedError

    # Pause the engine transport.
    @abstractmethod
    def transport_pause(self):
        raise NotImplementedError

    # Relocate the engine transport to a specific frame.
    @abstractmethod
    def transport_relocate(self, frame):
        raise NotImplementedError

    # Get the current transport frame.
    @abstractmethod
    def get_current_transport_frame(self):
        raise NotImplementedError

    # Get the engine transport information.
    @abstractmethod
    def get_transport_info(self):
        raise NotImplementedError

    # Current number of plugins loaded.
    @abstractmethod
    def get_current_plugin_count(self):
        raise NotImplementedError

    # Maximum number of loadable plugins allowed.
    # Returns 0 if engine is not started.
    @abstractmethod
    def get_max_plugin_number(self):
        raise NotImplementedError

    # Add a new plugin.
    # If you don't know the binary type use the BINARY_NATIVE macro.
    # @param btype    Binary type
    # @param ptype    Plugin type
    # @param filename Filename, if applicable
    # @param name     Name of the plugin, can be NULL
    # @param label    Plugin label, if applicable
    # @param uniqueId Plugin unique Id, if applicable
    # @param extraPtr Extra pointer, defined per plugin type
    # @param options  Initial plugin options
    @abstractmethod
    def add_plugin(self, btype, ptype, filename, name, label, uniqueId, extraPtr, options):
        raise NotImplementedError

    # Remove a plugin.
    # @param pluginId Plugin to remove.
    @abstractmethod
    def remove_plugin(self, pluginId):
        raise NotImplementedError

    # Remove all plugins.
    @abstractmethod
    def remove_all_plugins(self):
        raise NotImplementedError

    # Rename a plugin.
    # Returns the new name, or NULL if the operation failed.
    # @param pluginId Plugin to rename
    # @param newName  New plugin name
    @abstractmethod
    def rename_plugin(self, pluginId, newName):
        raise NotImplementedError

    # Clone a plugin.
    # @param pluginId Plugin to clone
    @abstractmethod
    def clone_plugin(self, pluginId):
        raise NotImplementedError

    # Prepare replace of a plugin.
    # The next call to carla_add_plugin() will use this id, replacing the current plugin.
    # @param pluginId Plugin to replace
    # @note This function requires carla_add_plugin() to be called afterwards *as soon as possible*.
    @abstractmethod
    def replace_plugin(self, pluginId):
        raise NotImplementedError

    # Switch two plugins positions.
    # @param pluginIdA Plugin A
    # @param pluginIdB Plugin B
    @abstractmethod
    def switch_plugins(self, pluginIdA, pluginIdB):
        raise NotImplementedError

    # Load a plugin state.
    # @param pluginId Plugin
    # @param filename Path to plugin state
    # @see carla_save_plugin_state()
    @abstractmethod
    def load_plugin_state(self, pluginId, filename):
        raise NotImplementedError

    # Save a plugin state.
    # @param pluginId Plugin
    # @param filename Path to plugin state
    # @see carla_load_plugin_state()
    @abstractmethod
    def save_plugin_state(self, pluginId, filename):
        raise NotImplementedError

    # Get information from a plugin.
    # @param pluginId Plugin
    @abstractmethod
    def get_plugin_info(self, pluginId):
        raise NotImplementedError

    # Get audio port count information from a plugin.
    # @param pluginId Plugin
    @abstractmethod
    def get_audio_port_count_info(self, pluginId):
        raise NotImplementedError

    # Get MIDI port count information from a plugin.
    # @param pluginId Plugin
    @abstractmethod
    def get_midi_port_count_info(self, pluginId):
        raise NotImplementedError

    # Get parameter count information from a plugin.
    # @param pluginId Plugin
    @abstractmethod
    def get_parameter_count_info(self, pluginId):
        raise NotImplementedError

    # Get parameter information from a plugin.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @see carla_get_parameter_count()
    @abstractmethod
    def get_parameter_info(self, pluginId, parameterId):
        raise NotImplementedError

    # Get parameter scale point information from a plugin.
    # @param pluginId     Plugin
    # @param parameterId  Parameter index
    # @param scalePointId Parameter scale-point index
    # @see CarlaParameterInfo::scalePointCount
    @abstractmethod
    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        raise NotImplementedError

    # Get a plugin's parameter data.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @see carla_get_parameter_count()
    @abstractmethod
    def get_parameter_data(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's parameter ranges.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @see carla_get_parameter_count()
    @abstractmethod
    def get_parameter_ranges(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's MIDI program data.
    # @param pluginId      Plugin
    # @param midiProgramId MIDI Program index
    # @see carla_get_midi_program_count()
    @abstractmethod
    def get_midi_program_data(self, pluginId, midiProgramId):
        raise NotImplementedError

    # Get a plugin's custom data.
    # @param pluginId     Plugin
    # @param customDataId Custom data index
    # @see carla_get_custom_data_count()
    @abstractmethod
    def get_custom_data(self, pluginId, customDataId):
        raise NotImplementedError

    # Get a plugin's chunk data.
    # @param pluginId Plugin
    # @see PLUGIN_OPTION_USE_CHUNKS
    @abstractmethod
    def get_chunk_data(self, pluginId):
        raise NotImplementedError

    # Get how many parameters a plugin has.
    # @param pluginId Plugin
    @abstractmethod
    def get_parameter_count(self, pluginId):
        raise NotImplementedError

    # Get how many programs a plugin has.
    # @param pluginId Plugin
    # @see carla_get_program_name()
    @abstractmethod
    def get_program_count(self, pluginId):
        raise NotImplementedError

    # Get how many MIDI programs a plugin has.
    # @param pluginId Plugin
    # @see carla_get_midi_program_name() and carla_get_midi_program_data()
    @abstractmethod
    def get_midi_program_count(self, pluginId):
        raise NotImplementedError

    # Get how many custom data sets a plugin has.
    # @param pluginId Plugin
    # @see carla_get_custom_data()
    @abstractmethod
    def get_custom_data_count(self, pluginId):
        raise NotImplementedError

    # Get a plugin's parameter text (custom display of internal values).
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @see PARAMETER_USES_CUSTOM_TEXT
    @abstractmethod
    def get_parameter_text(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's program name.
    # @param pluginId  Plugin
    # @param programId Program index
    # @see carla_get_program_count()
    @abstractmethod
    def get_program_name(self, pluginId, programId):
        raise NotImplementedError

    # Get a plugin's MIDI program name.
    # @param pluginId      Plugin
    # @param midiProgramId MIDI Program index
    # @see carla_get_midi_program_count()
    @abstractmethod
    def get_midi_program_name(self, pluginId, midiProgramId):
        raise NotImplementedError

    # Get a plugin's real name.
    # This is the name the plugin uses to identify itself; may not be unique.
    # @param pluginId Plugin
    @abstractmethod
    def get_real_plugin_name(self, pluginId):
        raise NotImplementedError

    # Get a plugin's program index.
    # @param pluginId Plugin
    @abstractmethod
    def get_current_program_index(self, pluginId):
        raise NotImplementedError

    # Get a plugin's midi program index.
    # @param pluginId Plugin
    @abstractmethod
    def get_current_midi_program_index(self, pluginId):
        raise NotImplementedError

    # Get a plugin's default parameter value.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    @abstractmethod
    def get_default_parameter_value(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's current parameter value.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    @abstractmethod
    def get_current_parameter_value(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's internal parameter value.
    # @param pluginId    Plugin
    # @param parameterId Parameter index, maybe be negative
    # @see InternalParameterIndex
    @abstractmethod
    def get_internal_parameter_value(self, pluginId, parameterId):
        raise NotImplementedError

    # Get a plugin's input peak value.
    # @param pluginId Plugin
    # @param isLeft   Wherever to get the left/mono value, otherwise right.
    @abstractmethod
    def get_input_peak_value(self, pluginId, isLeft):
        raise NotImplementedError

    # Get a plugin's output peak value.
    # @param pluginId Plugin
    # @param isLeft   Wherever to get the left/mono value, otherwise right.
    @abstractmethod
    def get_output_peak_value(self, pluginId, isLeft):
        raise NotImplementedError

    # Enable a plugin's option.
    # @param pluginId Plugin
    # @param option   An option from PluginOptions
    # @param yesNo    New enabled state
    @abstractmethod
    def set_option(self, pluginId, option, yesNo):
        raise NotImplementedError

    # Enable or disable a plugin.
    # @param pluginId Plugin
    # @param onOff    New active state
    @abstractmethod
    def set_active(self, pluginId, onOff):
        raise NotImplementedError

    # Change a plugin's internal dry/wet.
    # @param pluginId Plugin
    # @param value    New dry/wet value
    @abstractmethod
    def set_drywet(self, pluginId, value):
        raise NotImplementedError

    # Change a plugin's internal volume.
    # @param pluginId Plugin
    # @param value    New volume
    @abstractmethod
    def set_volume(self, pluginId, value):
        raise NotImplementedError

    # Change a plugin's internal stereo balance, left channel.
    # @param pluginId Plugin
    # @param value    New value
    @abstractmethod
    def set_balance_left(self, pluginId, value):
        raise NotImplementedError

    # Change a plugin's internal stereo balance, right channel.
    # @param pluginId Plugin
    # @param value    New value
    @abstractmethod
    def set_balance_right(self, pluginId, value):
        raise NotImplementedError

    # Change a plugin's internal mono panning value.
    # @param pluginId Plugin
    # @param value    New value
    @abstractmethod
    def set_panning(self, pluginId, value):
        raise NotImplementedError

    # Change a plugin's internal control channel.
    # @param pluginId Plugin
    # @param channel  New channel
    @abstractmethod
    def set_ctrl_channel(self, pluginId, channel):
        raise NotImplementedError

    # Change a plugin's parameter value.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @param value       New value
    @abstractmethod
    def set_parameter_value(self, pluginId, parameterId, value):
        raise NotImplementedError

    # Change a plugin's parameter MIDI cc.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @param cc          New MIDI cc
    @abstractmethod
    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        raise NotImplementedError

    # Change a plugin's parameter MIDI channel.
    # @param pluginId    Plugin
    # @param parameterId Parameter index
    # @param channel     New MIDI channel
    @abstractmethod
    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        raise NotImplementedError

    # Change a plugin's current program.
    # @param pluginId  Plugin
    # @param programId New program
    @abstractmethod
    def set_program(self, pluginId, programId):
        raise NotImplementedError

    # Change a plugin's current MIDI program.
    # @param pluginId      Plugin
    # @param midiProgramId New value
    @abstractmethod
    def set_midi_program(self, pluginId, midiProgramId):
        raise NotImplementedError

    # Set a plugin's custom data set.
    # @param pluginId Plugin
    # @param type     Type
    # @param key      Key
    # @param value    New value
    # @see CustomDataTypes and CustomDataKeys
    @abstractmethod
    def set_custom_data(self, pluginId, type_, key, value):
        raise NotImplementedError

    # Set a plugin's chunk data.
    # @param pluginId  Plugin
    # @param chunkData New chunk data
    # @see PLUGIN_OPTION_USE_CHUNKS and carla_get_chunk_data()
    @abstractmethod
    def set_chunk_data(self, pluginId, chunkData):
        raise NotImplementedError

    # Tell a plugin to prepare for save.
    # This should be called before saving custom data sets.
    # @param pluginId Plugin
    @abstractmethod
    def prepare_for_save(self, pluginId):
        raise NotImplementedError

    # Reset all plugin's parameters.
    # @param pluginId Plugin
    @abstractmethod
    def reset_parameters(self, pluginId):
        raise NotImplementedError

    # Randomize all plugin's parameters.
    # @param pluginId Plugin
    @abstractmethod
    def randomize_parameters(self, pluginId):
        raise NotImplementedError

    # Send a single note of a plugin.
    # If velocity is 0, note-off is sent; note-on otherwise.
    # @param pluginId Plugin
    # @param channel  Note channel
    # @param note     Note pitch
    # @param velocity Note velocity
    @abstractmethod
    def send_midi_note(self, pluginId, channel, note, velocity):
        raise NotImplementedError

    # Tell a plugin to show its own custom UI.
    # @param pluginId Plugin
    # @param yesNo    New UI state, visible or not
    # @see PLUGIN_HAS_CUSTOM_UI
    @abstractmethod
    def show_custom_ui(self, pluginId, yesNo):
        raise NotImplementedError

    # Get the current engine buffer size.
    @abstractmethod
    def get_buffer_size(self):
        raise NotImplementedError

    # Get the current engine sample rate.
    @abstractmethod
    def get_sample_rate(self):
        raise NotImplementedError

    # Get the last error.
    @abstractmethod
    def get_last_error(self):
        raise NotImplementedError

    # Get the current engine OSC URL (TCP).
    @abstractmethod
    def get_host_osc_url_tcp(self):
        raise NotImplementedError

    # Get the current engine OSC URL (UDP).
    @abstractmethod
    def get_host_osc_url_udp(self):
        raise NotImplementedError

# ------------------------------------------------------------------------------------------------------------
# Carla Host object (dummy/null, does nothing)

class CarlaHostNull(CarlaHostMeta):
    def __init__(self):
        CarlaHostMeta.__init__(self)

        self.fEngineCallback = None
        self.fEngineRunning  = False

    def get_engine_driver_count(self):
        return 0

    def get_engine_driver_name(self, index):
        return ""

    def get_engine_driver_device_names(self, index):
        return []

    def get_engine_driver_device_info(self, index, name):
        return PyEngineDriverDeviceInfo

    def engine_init(self, driverName, clientName):
        self.fEngineRunning = True
        if self.fEngineCallback is not None:
            self.fEngineCallback(None, ENGINE_CALLBACK_ENGINE_STARTED, 0, self.processMode, self.transportMode, 0.0, driverName)
        return True

    def engine_close(self):
        self.fEngineRunning = False
        if self.fEngineCallback is not None:
            self.fEngineCallback(None, ENGINE_CALLBACK_ENGINE_STOPPED, 0, 0, 0, 0.0, "")
        return True

    def engine_idle(self):
        return

    def is_engine_running(self):
        return False

    def set_engine_about_to_close(self):
        return

    def set_engine_callback(self, func):
        self.fEngineCallback = func

    def set_engine_option(self, option, value, valueStr):
        return

    def set_file_callback(self, func):
        return

    def load_file(self, filename):
        return False

    def load_project(self, filename):
        return False

    def save_project(self, filename):
        return False

    def patchbay_connect(self, groupIdA, portIdA, groupIdB, portIdB):
        return False

    def patchbay_disconnect(self, connectionId):
        return False

    def patchbay_refresh(self, external):
        return False

    def transport_play(self):
        return

    def transport_pause(self):
        return

    def transport_relocate(self, frame):
        return

    def get_current_transport_frame(self):
        return 0

    def get_transport_info(self):
        return PyCarlaTransportInfo

    def get_current_plugin_count(self):
        return 0

    def get_max_plugin_number(self):
        return 0

    def add_plugin(self, btype, ptype, filename, name, label, uniqueId, extraPtr, options):
        return False

    def remove_plugin(self, pluginId):
        return False

    def remove_all_plugins(self):
        return False

    def rename_plugin(self, pluginId, newName):
        return ""

    def clone_plugin(self, pluginId):
        return False

    def replace_plugin(self, pluginId):
        return False

    def switch_plugins(self, pluginIdA, pluginIdB):
        return False

    def load_plugin_state(self, pluginId, filename):
        return False

    def save_plugin_state(self, pluginId, filename):
        return False

    def get_plugin_info(self, pluginId):
        return PyCarlaPluginInfo

    def get_audio_port_count_info(self, pluginId):
        return PyCarlaPortCountInfo

    def get_midi_port_count_info(self, pluginId):
        return PyCarlaPortCountInfo

    def get_parameter_count_info(self, pluginId):
        return PyCarlaPortCountInfo

    def get_parameter_info(self, pluginId, parameterId):
        return PyCarlaParameterInfo

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        return PyCarlaScalePointInfo

    def get_parameter_data(self, pluginId, parameterId):
        return PyParameterData

    def get_parameter_ranges(self, pluginId, parameterId):
        return PyParameterRanges

    def get_midi_program_data(self, pluginId, midiProgramId):
        return PyMidiProgramData

    def get_custom_data(self, pluginId, customDataId):
        return PyCustomData

    def get_chunk_data(self, pluginId):
        return ""

    def get_parameter_count(self, pluginId):
        return 0

    def get_program_count(self, pluginId):
        return 0

    def get_midi_program_count(self, pluginId):
        return 0

    def get_custom_data_count(self, pluginId):
        return 0

    def get_parameter_text(self, pluginId, parameterId):
        return ""

    def get_program_name(self, pluginId, programId):
        return ""

    def get_midi_program_name(self, pluginId, midiProgramId):
        return ""

    def get_real_plugin_name(self, pluginId):
        return ""

    def get_current_program_index(self, pluginId):
        return 0

    def get_current_midi_program_index(self, pluginId):
        return 0

    def get_default_parameter_value(self, pluginId, parameterId):
        return 0.0

    def get_current_parameter_value(self, pluginId, parameterId):
        return 0.0

    def get_internal_parameter_value(self, pluginId, parameterId):
        return 0.0

    def get_input_peak_value(self, pluginId, isLeft):
        return 0.0

    def get_output_peak_value(self, pluginId, isLeft):
        return 0.0

    def set_option(self, pluginId, option, yesNo):
        return

    def set_active(self, pluginId, onOff):
        return

    def set_drywet(self, pluginId, value):
        return

    def set_volume(self, pluginId, value):
        return

    def set_balance_left(self, pluginId, value):
        return

    def set_balance_right(self, pluginId, value):
        return

    def set_panning(self, pluginId, value):
        return

    def set_ctrl_channel(self, pluginId, channel):
        return

    def set_parameter_value(self, pluginId, parameterId, value):
        return

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        return

    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        return

    def set_program(self, pluginId, programId):
        return

    def set_midi_program(self, pluginId, midiProgramId):
        return

    def set_custom_data(self, pluginId, type_, key, value):
        return

    def set_chunk_data(self, pluginId, chunkData):
        return

    def prepare_for_save(self, pluginId):
        return

    def reset_parameters(self, pluginId):
        return

    def randomize_parameters(self, pluginId):
        return

    def send_midi_note(self, pluginId, channel, note, velocity):
        return

    def show_custom_ui(self, pluginId, yesNo):
        return

    def get_buffer_size(self):
        return 0

    def get_sample_rate(self):
        return 0.0

    def get_last_error(self):
        return ""

    def get_host_osc_url_tcp(self):
        return ""

    def get_host_osc_url_udp(self):
        return ""

# ------------------------------------------------------------------------------------------------------------
# Carla Host object using a DLL

class CarlaHostDLL(CarlaHostMeta):
    def __init__(self, libName):
        CarlaHostMeta.__init__(self)

        # info about this host object
        self.isPlugin = False

        self.lib = cdll.LoadLibrary(libName)

        self.lib.carla_get_engine_driver_count.argtypes = None
        self.lib.carla_get_engine_driver_count.restype = c_uint

        self.lib.carla_get_engine_driver_name.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_name.restype = c_char_p

        self.lib.carla_get_engine_driver_device_names.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_device_names.restype = POINTER(c_char_p)

        self.lib.carla_get_engine_driver_device_info.argtypes = [c_uint, c_char_p]
        self.lib.carla_get_engine_driver_device_info.restype = POINTER(EngineDriverDeviceInfo)

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

        self.lib.carla_set_file_callback.argtypes = [FileCallbackFunc, c_void_p]
        self.lib.carla_set_file_callback.restype = None

        self.lib.carla_load_file.argtypes = [c_char_p]
        self.lib.carla_load_file.restype = c_bool

        self.lib.carla_load_project.argtypes = [c_char_p]
        self.lib.carla_load_project.restype = c_bool

        self.lib.carla_save_project.argtypes = [c_char_p]
        self.lib.carla_save_project.restype = c_bool

        self.lib.carla_patchbay_connect.argtypes = [c_uint, c_uint, c_uint, c_uint]
        self.lib.carla_patchbay_connect.restype = c_bool

        self.lib.carla_patchbay_disconnect.argtypes = [c_uint]
        self.lib.carla_patchbay_disconnect.restype = c_bool

        self.lib.carla_patchbay_refresh.argtypes = [c_bool]
        self.lib.carla_patchbay_refresh.restype = c_bool

        self.lib.carla_transport_play.argtypes = None
        self.lib.carla_transport_play.restype = None

        self.lib.carla_transport_pause.argtypes = None
        self.lib.carla_transport_pause.restype = None

        self.lib.carla_transport_relocate.argtypes = [c_uint64]
        self.lib.carla_transport_relocate.restype = None

        self.lib.carla_get_current_transport_frame.argtypes = None
        self.lib.carla_get_current_transport_frame.restype = c_uint64

        self.lib.carla_get_transport_info.argtypes = None
        self.lib.carla_get_transport_info.restype = POINTER(CarlaTransportInfo)

        self.lib.carla_get_current_plugin_count.argtypes = None
        self.lib.carla_get_current_plugin_count.restype = c_uint32

        self.lib.carla_get_max_plugin_number.argtypes = None
        self.lib.carla_get_max_plugin_number.restype = c_uint32

        self.lib.carla_add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_char_p, c_int64, c_void_p, c_uint]
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

        self.lib.carla_get_internal_parameter_value.argtypes = [c_uint, c_int32]
        self.lib.carla_get_internal_parameter_value.restype = c_float

        self.lib.carla_get_input_peak_value.argtypes = [c_uint, c_bool]
        self.lib.carla_get_input_peak_value.restype = c_float

        self.lib.carla_get_output_peak_value.argtypes = [c_uint, c_bool]
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

        self.lib.carla_set_parameter_midi_channel.argtypes = [c_uint, c_uint32, c_uint8]
        self.lib.carla_set_parameter_midi_channel.restype = None

        self.lib.carla_set_parameter_midi_cc.argtypes = [c_uint, c_uint32, c_int16]
        self.lib.carla_set_parameter_midi_cc.restype = None

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

        self.lib.carla_reset_parameters.argtypes = [c_uint]
        self.lib.carla_reset_parameters.restype = None

        self.lib.carla_randomize_parameters.argtypes = [c_uint]
        self.lib.carla_randomize_parameters.restype = None

        self.lib.carla_send_midi_note.argtypes = [c_uint, c_uint8, c_uint8, c_uint8]
        self.lib.carla_send_midi_note.restype = None

        self.lib.carla_show_custom_ui.argtypes = [c_uint, c_bool]
        self.lib.carla_show_custom_ui.restype = None

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

        self.lib.carla_nsm_init.argtypes = [c_int, c_char_p]
        self.lib.carla_nsm_init.restype = c_bool

        self.lib.carla_nsm_ready.argtypes = [c_int]
        self.lib.carla_nsm_ready.restype = None

    # --------------------------------------------------------------------------------------------------------

    def get_engine_driver_count(self):
        return int(self.lib.carla_get_engine_driver_count())

    def get_engine_driver_name(self, index):
        return charPtrToString(self.lib.carla_get_engine_driver_name(index))

    def get_engine_driver_device_names(self, index):
        return charPtrPtrToStringList(self.lib.carla_get_engine_driver_device_names(index))

    def get_engine_driver_device_info(self, index, name):
        return structToDict(self.lib.carla_get_engine_driver_device_info(index, name.encode("utf-8")).contents)

    def engine_init(self, driverName, clientName):
        return bool(self.lib.carla_engine_init(driverName.encode("utf-8"), clientName.encode("utf-8")))

    def engine_close(self):
        return bool(self.lib.carla_engine_close())

    def engine_idle(self):
        self.lib.carla_engine_idle()

    def is_engine_running(self):
        return bool(self.lib.carla_is_engine_running())

    def set_engine_about_to_close(self):
        self.lib.carla_set_engine_about_to_close()

    def set_engine_callback(self, func):
        self._engineCallback = EngineCallbackFunc(func)
        self.lib.carla_set_engine_callback(self._engineCallback, None)

    def set_engine_option(self, option, value, valueStr):
        self.lib.carla_set_engine_option(option, value, valueStr.encode("utf-8"))

    def set_file_callback(self, func):
        self._fileCallback = FileCallbackFunc(func)
        self.lib.carla_set_file_callback(self._fileCallback, None)

    def load_file(self, filename):
        return bool(self.lib.carla_load_file(filename.encode("utf-8")))

    def load_project(self, filename):
        return bool(self.lib.carla_load_project(filename.encode("utf-8")))

    def save_project(self, filename):
        return bool(self.lib.carla_save_project(filename.encode("utf-8")))

    def patchbay_connect(self, groupIdA, portIdA, groupIdB, portIdB):
        return bool(self.lib.carla_patchbay_connect(groupIdA, portIdA, groupIdB, portIdB))

    def patchbay_disconnect(self, connectionId):
        return bool(self.lib.carla_patchbay_disconnect(connectionId))

    def patchbay_refresh(self, external):
        return bool(self.lib.carla_patchbay_refresh(external))

    def transport_play(self):
        self.lib.carla_transport_play()

    def transport_pause(self):
        self.lib.carla_transport_pause()

    def transport_relocate(self, frame):
        self.lib.carla_transport_relocate(frame)

    def get_current_transport_frame(self):
        return int(self.lib.carla_get_current_transport_frame())

    def get_transport_info(self):
        return structToDict(self.lib.carla_get_transport_info().contents)

    def get_current_plugin_count(self):
        return int(self.lib.carla_get_current_plugin_count())

    def get_max_plugin_number(self):
        return int(self.lib.carla_get_max_plugin_number())

    def add_plugin(self, btype, ptype, filename, name, label, uniqueId, extraPtr, options):
        cfilename = filename.encode("utf-8") if filename else None
        cname     = name.encode("utf-8") if name else None
        clabel    = label.encode("utf-8") if label else None
        return bool(self.lib.carla_add_plugin(btype, ptype, cfilename, cname, clabel, uniqueId, cast(extraPtr, c_void_p), options))

    def remove_plugin(self, pluginId):
        return bool(self.lib.carla_remove_plugin(pluginId))

    def remove_all_plugins(self):
        return bool(self.lib.carla_remove_all_plugins())

    def rename_plugin(self, pluginId, newName):
        return charPtrToString(self.lib.carla_rename_plugin(pluginId, newName.encode("utf-8")))

    def clone_plugin(self, pluginId):
        return bool(self.lib.carla_clone_plugin(pluginId))

    def replace_plugin(self, pluginId):
        return bool(self.lib.carla_replace_plugin(pluginId))

    def switch_plugins(self, pluginIdA, pluginIdB):
        return bool(self.lib.carla_switch_plugins(pluginIdA, pluginIdB))

    def load_plugin_state(self, pluginId, filename):
        return bool(self.lib.carla_load_plugin_state(pluginId, filename.encode("utf-8")))

    def save_plugin_state(self, pluginId, filename):
        return bool(self.lib.carla_save_plugin_state(pluginId, filename.encode("utf-8")))

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
        return charPtrToString(self.lib.carla_get_chunk_data(pluginId))

    def get_parameter_count(self, pluginId):
        return int(self.lib.carla_get_parameter_count(pluginId))

    def get_program_count(self, pluginId):
        return int(self.lib.carla_get_program_count(pluginId))

    def get_midi_program_count(self, pluginId):
        return int(self.lib.carla_get_midi_program_count(pluginId))

    def get_custom_data_count(self, pluginId):
        return int(self.lib.carla_get_custom_data_count(pluginId))

    def get_parameter_text(self, pluginId, parameterId):
        return charPtrToString(self.lib.carla_get_parameter_text(pluginId, parameterId))

    def get_program_name(self, pluginId, programId):
        return charPtrToString(self.lib.carla_get_program_name(pluginId, programId))

    def get_midi_program_name(self, pluginId, midiProgramId):
        return charPtrToString(self.lib.carla_get_midi_program_name(pluginId, midiProgramId))

    def get_real_plugin_name(self, pluginId):
        return charPtrToString(self.lib.carla_get_real_plugin_name(pluginId))

    def get_current_program_index(self, pluginId):
        return int(self.lib.carla_get_current_program_index(pluginId))

    def get_current_midi_program_index(self, pluginId):
        return int(self.lib.carla_get_current_midi_program_index(pluginId))

    def get_default_parameter_value(self, pluginId, parameterId):
        return float(self.lib.carla_get_default_parameter_value(pluginId, parameterId))

    def get_current_parameter_value(self, pluginId, parameterId):
        return float(self.lib.carla_get_current_parameter_value(pluginId, parameterId))

    def get_internal_parameter_value(self, pluginId, parameterId):
        return float(self.lib.carla_get_internal_parameter_value(pluginId, parameterId))

    def get_input_peak_value(self, pluginId, isLeft):
        return float(self.lib.carla_get_input_peak_value(pluginId, isLeft))

    def get_output_peak_value(self, pluginId, isLeft):
        return float(self.lib.carla_get_output_peak_value(pluginId, isLeft))

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

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        self.lib.carla_set_parameter_midi_channel(pluginId, parameterId, channel)

    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        self.lib.carla_set_parameter_midi_cc(pluginId, parameterId, cc)

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

    def reset_parameters(self, pluginId):
        self.lib.carla_reset_parameters(pluginId)

    def randomize_parameters(self, pluginId):
        self.lib.carla_randomize_parameters(pluginId)

    def send_midi_note(self, pluginId, channel, note, velocity):
        self.lib.carla_send_midi_note(pluginId, channel, note, velocity)

    def show_custom_ui(self, pluginId, yesNo):
        self.lib.carla_show_custom_ui(pluginId, yesNo)

    def get_buffer_size(self):
        return int(self.lib.carla_get_buffer_size())

    def get_sample_rate(self):
        return float(self.lib.carla_get_sample_rate())

    def get_last_error(self):
        return charPtrToString(self.lib.carla_get_last_error())

    def get_host_osc_url_tcp(self):
        return charPtrToString(self.lib.carla_get_host_osc_url_tcp())

    def get_host_osc_url_udp(self):
        return charPtrToString(self.lib.carla_get_host_osc_url_udp())

    def nsm_init(self, pid, executableName):
        return bool(self.lib.carla_nsm_init(pid, executableName.encode("utf-8")))

    def nsm_ready(self, action):
        self.lib.carla_nsm_ready(action)

# ------------------------------------------------------------------------------------------------------------
# Helper object for CarlaHostPlugin

class PluginStoreInfo(object):
    __slots__ = [
        'pluginInfo',
        'pluginRealName',
        'internalValues',
        'audioCountInfo',
        'midiCountInfo',
        'parameterCount',
        'parameterCountInfo',
        'parameterInfo',
        'parameterData',
        'parameterRanges',
        'parameterValues',
        'programCount',
        'programCurrent',
        'programNames',
        'midiProgramCount',
        'midiProgramCurrent',
        'midiProgramData',
        'customDataCount',
        'customData',
        'peaks'
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Host object for plugins (using pipes)

class CarlaHostPlugin(CarlaHostMeta):
#class CarlaHostPlugin(CarlaHostMeta, metaclass=ABCMeta):
    def __init__(self):
        CarlaHostMeta.__init__(self)

        # info about this host object
        self.isPlugin          = True
        self.processModeForced = True

        # text data to return when requested
        self.fMaxPluginNumber = 0
        self.fLastError       = ""

        # plugin info
        self.fPluginsInfo = []

        # transport info
        self.fTransportInfo = {
            "playing": False,
            "frame": 0,
            "bar": 0,
            "beat": 0,
            "tick": 0,
            "bpm": 0.0
        }

        # some other vars
        self.fBufferSize = 0
        self.fSampleRate = 0.0
        self.fOscTCP = ""
        self.fOscUDP = ""

    # --------------------------------------------------------------------------------------------------------

    # Needs to be reimplemented
    @abstractmethod
    def sendMsg(self, lines):
        raise NotImplementedError

    # internal, sets error if sendMsg failed
    def sendMsgAndSetError(self, lines):
        if self.sendMsg(lines):
            return True

        self.fLastError = "Communication error with backend"
        return False

    # --------------------------------------------------------------------------------------------------------

    def get_engine_driver_count(self):
        return 1

    def get_engine_driver_name(self, index):
        return "Plugin"

    def get_engine_driver_device_names(self, index):
        return []

    def get_engine_driver_device_info(self, index, name):
        return PyEngineDriverDeviceInfo

    def set_engine_callback(self, func):
        return # TODO

    def set_engine_option(self, option, value, valueStr):
        self.sendMsg(["set_engine_option", option, int(value), valueStr])

    def set_file_callback(self, func):
        return # TODO

    def load_file(self, filename):
        return self.sendMsgAndSetError(["load_file", filename])

    def load_project(self, filename):
        return self.sendMsgAndSetError(["load_project", filename])

    def save_project(self, filename):
        return self.sendMsgAndSetError(["save_project", filename])

    def patchbay_connect(self, groupIdA, portIdA, groupIdB, portIdB):
        return self.sendMsgAndSetError(["patchbay_connect", groupIdA, portIdA, groupIdB, portIdB])

    def patchbay_disconnect(self, connectionId):
        return self.sendMsgAndSetError(["patchbay_disconnect", connectionId])

    def patchbay_refresh(self, external):
        # don't send external param, never used in plugins
        return self.sendMsgAndSetError(["patchbay_refresh"])

    def transport_play(self):
        self.sendMsg(["transport_play"])

    def transport_pause(self):
        self.sendMsg(["transport_pause"])

    def transport_relocate(self, frame):
        self.sendMsg(["transport_relocate"])

    def get_current_transport_frame(self):
        return self.fTransportInfo['frame']

    def get_transport_info(self):
        return self.fTransportInfo

    def get_current_plugin_count(self):
        return len(self.fPluginsInfo)

    def get_max_plugin_number(self):
        return self.fMaxPluginNumber

    def add_plugin(self, btype, ptype, filename, name, label, uniqueId, extraPtr, options):
        return self.sendMsgAndSetError(["add_plugin", btype, ptype, filename, name, label, uniqueId, options])

    def remove_plugin(self, pluginId):
        return self.sendMsgAndSetError(["remove_plugin", pluginId])

    def remove_all_plugins(self):
        return self.sendMsgAndSetError(["remove_all_plugins"])

    def rename_plugin(self, pluginId, newName):
        if self.sendMsg(["rename_plugin", pluginId, newName]):
            return newName

        self.fLastError = "Communication error with backend"
        return ""

    def clone_plugin(self, pluginId):
        return self.sendMsgAndSetError(["clone_plugin", pluginId])

    def replace_plugin(self, pluginId):
        return self.sendMsgAndSetError(["replace_plugin", pluginId])

    def switch_plugins(self, pluginIdA, pluginIdB):
        return self.sendMsgAndSetError(["switch_plugins", pluginIdA, pluginIdB])

    def load_plugin_state(self, pluginId, filename):
        return self.sendMsgAndSetError(["load_plugin_state", pluginId, filename])

    def save_plugin_state(self, pluginId, filename):
        return self.sendMsgAndSetError(["save_plugin_state", pluginId, filename])

    def get_plugin_info(self, pluginId):
        return self.fPluginsInfo[pluginId].pluginInfo

    def get_audio_port_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].audioCountInfo

    def get_midi_port_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].midiCountInfo

    def get_parameter_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].parameterCountInfo

    def get_parameter_info(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterInfo[parameterId]

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalePointId):
        return PyCarlaScalePointInfo

    def get_parameter_data(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterData[parameterId]

    def get_parameter_ranges(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterRanges[parameterId]

    def get_midi_program_data(self, pluginId, midiProgramId):
        return self.fPluginsInfo[pluginId].midiProgramData[midiProgramId]

    def get_custom_data(self, pluginId, customDataId):
        return self.fPluginsInfo[pluginId].customData[customDataId]

    def get_chunk_data(self, pluginId):
        return ""

    def get_parameter_count(self, pluginId):
        return self.fPluginsInfo[pluginId].parameterCount

    def get_program_count(self, pluginId):
        return self.fPluginsInfo[pluginId].programCount

    def get_midi_program_count(self, pluginId):
        return self.fPluginsInfo[pluginId].midiProgramCount

    def get_custom_data_count(self, pluginId):
        return self.fPluginsInfo[pluginId].customDataCount

    def get_parameter_text(self, pluginId, parameterId):
        return ""

    def get_program_name(self, pluginId, programId):
        return self.fPluginsInfo[pluginId].programNames[programId]

    def get_midi_program_name(self, pluginId, midiProgramId):
        return self.fPluginsInfo[pluginId].midiProgramData[midiProgramId]['label']

    def get_real_plugin_name(self, pluginId):
        return self.fPluginsInfo[pluginId].pluginRealName

    def get_current_program_index(self, pluginId):
        return self.fPluginsInfo[pluginId].programCurrent

    def get_current_midi_program_index(self, pluginId):
        return self.fPluginsInfo[pluginId].midiProgramCurrent

    def get_default_parameter_value(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterRanges[parameterId]['def']

    def get_current_parameter_value(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterValues[parameterId]

    def get_internal_parameter_value(self, pluginId, parameterId):
        if parameterId == PARAMETER_NULL or parameterId <= PARAMETER_MAX:
            return 0.0
        if parameterId < 0:
            return self.fPluginsInfo[pluginId].internalValues[abs(parameterId)-2]

        return self.fPluginsInfo[pluginId].parameterValues[parameterId]

    def get_input_peak_value(self, pluginId, isLeft):
        return self.fPluginsInfo[pluginId].peaks[0 if isLeft else 1]

    def get_output_peak_value(self, pluginId, isLeft):
        return self.fPluginsInfo[pluginId].peaks[2 if isLeft else 3]

    def set_option(self, pluginId, option, yesNo):
        self.sendMsg(["set_option", pluginId, option, yesNo])

    def set_active(self, pluginId, onOff):
        self.sendMsg(["set_active", pluginId, onOff])
        self.fPluginsInfo[pluginId].internalValues[0] = 1.0 if onOff else 0.0

    def set_drywet(self, pluginId, value):
        self.sendMsg(["set_drywet", pluginId, value])
        self.fPluginsInfo[pluginId].internalValues[1] = value

    def set_volume(self, pluginId, value):
        self.sendMsg(["set_volume", pluginId, value])
        self.fPluginsInfo[pluginId].internalValues[2] = value

    def set_balance_left(self, pluginId, value):
        self.sendMsg(["set_balance_left", pluginId, value])
        self.fPluginsInfo[pluginId].internalValues[3] = value

    def set_balance_right(self, pluginId, value):
        self.sendMsg(["set_balance_right", pluginId, value])
        self.fPluginsInfo[pluginId].internalValues[4] = value

    def set_panning(self, pluginId, value):
        self.sendMsg(["set_panning", pluginId, value])
        self.fPluginsInfo[pluginId].internalValues[5] = value

    def set_ctrl_channel(self, pluginId, channel):
        self.sendMsg(["set_ctrl_channel", pluginId, channel])
        self.fPluginsInfo[pluginId].internalValues[6] = float(channel)

    def set_parameter_value(self, pluginId, parameterId, value):
        self.sendMsg(["set_parameter_value", pluginId, parameterId, value])
        self.fPluginsInfo[pluginId].parameterValues[parameterId] = value

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        self.sendMsg(["set_parameter_midi_channel", pluginId, parameterId, channel])
        self.fPluginsInfo[pluginId].parameterData[parameterId]['midiCC'] = channel

    def set_parameter_midi_cc(self, pluginId, parameterId, cc):
        self.sendMsg(["set_parameter_midi_cc", pluginId, parameterId, cc])
        self.fPluginsInfo[pluginId].parameterData[parameterId]['midiCC'] = cc

    def set_program(self, pluginId, programId):
        self.sendMsg(["set_program", pluginId, programId])
        self.fPluginsInfo[pluginId].programCurrent = programId

    def set_midi_program(self, pluginId, midiProgramId):
        self.sendMsg(["set_midi_program", pluginId, midiProgramId])
        self.fPluginsInfo[pluginId].midiProgramCurrent = midiProgramId

    def set_custom_data(self, pluginId, type_, key, value):
        self.sendMsg(["set_custom_data", pluginId, type_, key, value])

        for cdata in self.fPluginsInfo[pluginId].customData:
            if cdata['type'] != type_:
                continue
            if cdata['key'] != key:
                continue
            cdata['value'] = value
            break

    def set_chunk_data(self, pluginId, chunkData):
        self.sendMsg(["set_chunk_data", pluginId, chunkData])

    def prepare_for_save(self, pluginId):
        self.sendMsg(["prepare_for_save", pluginId])

    def reset_parameters(self, pluginId):
        self.sendMsg(["reset_parameters", pluginId])

    def randomize_parameters(self, pluginId):
        self.sendMsg(["randomize_parameters", pluginId])

    def send_midi_note(self, pluginId, channel, note, velocity):
        self.sendMsg(["send_midi_note", pluginId, channel, note, velocity])

    def show_custom_ui(self, pluginId, yesNo):
        self.sendMsg(["show_custom_ui", pluginId, yesNo])

    def get_buffer_size(self):
        return self.fBufferSize

    def get_sample_rate(self):
        return self.fSampleRate

    def get_last_error(self):
        return self.fLastError

    def get_host_osc_url_tcp(self):
        return self.fOscTCP

    def get_host_osc_url_udp(self):
        return self.fOscUDP

    # --------------------------------------------------------------------------------------------------------

    def _set_transport(self, playing, frame, bar, beat, tick, bpm):
        self.fTransportInfo = {
            "playing": playing,
            "frame": frame,
            "bar": bar,
            "beat": beat,
            "tick": tick,
            "bpm": bpm
        }

    def _add(self, pluginId):
        if len(self.fPluginsInfo) != pluginId:
            return

        info = PluginStoreInfo()
        info.pluginInfo     = deepcopy(PyCarlaPluginInfo)
        info.pluginRealName = ""
        info.internalValues = [0.0, 1.0, 1.0, -1.0, 1.0, 0.0, -1.0]
        info.audioCountInfo = deepcopy(PyCarlaPortCountInfo)
        info.midiCountInfo  = deepcopy(PyCarlaPortCountInfo)
        info.parameterCount = 0
        info.parameterCountInfo = deepcopy(PyCarlaPortCountInfo)
        info.parameterInfo   = []
        info.parameterData   = []
        info.parameterRanges = []
        info.parameterValues = []
        info.programCount   = 0
        info.programCurrent = -1
        info.programNames   = []
        info.midiProgramCount   = 0
        info.midiProgramCurrent = -1
        info.midiProgramData    = []
        info.customDataCount = 0
        info.customData      = []
        info.peaks = [0.0, 0.0, 0.0, 0.0]
        self.fPluginsInfo.append(info)

    def _set_pluginInfo(self, pluginId, info):
        self.fPluginsInfo[pluginId].pluginInfo = info

    def _set_pluginInfoUpdate(self, pluginId, info):
        self.fPluginsInfo[pluginId].pluginInfo.update(info)

    def _set_pluginName(self, pluginId, name):
        self.fPluginsInfo[pluginId].pluginInfo['name'] = name

    def _set_pluginRealName(self, pluginId, realName):
        self.fPluginsInfo[pluginId].pluginRealName = realName

    def _set_internalValue(self, pluginId, paramIndex, value):
        if pluginId < len(self.fPluginsInfo) and PARAMETER_NULL > paramIndex > PARAMETER_MAX:
            self.fPluginsInfo[pluginId].internalValues[abs(paramIndex)-2] = float(value)

    def _set_audioCountInfo(self, pluginId, info):
        self.fPluginsInfo[pluginId].audioCountInfo = info

    def _set_midiCountInfo(self, pluginId, info):
        self.fPluginsInfo[pluginId].midiCountInfo = info

    def _set_parameterCountInfo(self, pluginId, count, info):
        self.fPluginsInfo[pluginId].parameterCount = count
        self.fPluginsInfo[pluginId].parameterCountInfo = info

        # clear
        self.fPluginsInfo[pluginId].parameterInfo  = []
        self.fPluginsInfo[pluginId].parameterData  = []
        self.fPluginsInfo[pluginId].parameterRanges = []
        self.fPluginsInfo[pluginId].parameterValues = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[pluginId].parameterInfo.append(deepcopy(PyCarlaParameterInfo))
            self.fPluginsInfo[pluginId].parameterData.append(deepcopy(PyParameterData))
            self.fPluginsInfo[pluginId].parameterRanges.append(deepcopy(PyParameterRanges))
            self.fPluginsInfo[pluginId].parameterValues.append(0.0)

    def _set_programCount(self, pluginId, count):
        self.fPluginsInfo[pluginId].programCount = count

        # clear
        self.fPluginsInfo[pluginId].programNames = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[pluginId].programNames.append("")

    def _set_midiProgramCount(self, pluginId, count):
        self.fPluginsInfo[pluginId].midiProgramCount = count

        # clear
        self.fPluginsInfo[pluginId].midiProgramData = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[pluginId].midiProgramData.append(deepcopy(PyMidiProgramData))

    def _set_customDataCount(self, pluginId, count):
        self.fPluginsInfo[pluginId].customDataCount = count

        # clear
        self.fPluginsInfo[pluginId].customData = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[pluginId].customData.append(PyCustomData)

    def _set_parameterInfo(self, pluginId, paramIndex, info):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterInfo[paramIndex] = info

    def _set_parameterData(self, pluginId, paramIndex, data):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterData[paramIndex] = data

    def _set_parameterRanges(self, pluginId, paramIndex, ranges):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterRanges[paramIndex] = ranges

    def _set_parameterRangesUpdate(self, pluginId, paramIndex, ranges):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterRanges[paramIndex].update(ranges)

    def _set_parameterValue(self, pluginId, paramIndex, value):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterValues[paramIndex] = value

    def _set_parameterDefault(self, pluginId, paramIndex, value):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterRanges[paramIndex]['def'] = value

    def _set_parameterMidiChannel(self, pluginId, paramIndex, channel):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterData[paramIndex]['midiChannel'] = channel

    def _set_parameterMidiCC(self, pluginId, paramIndex, cc):
        if pluginId < len(self.fPluginsInfo) and paramIndex < self.fPluginsInfo[pluginId].parameterCount:
            self.fPluginsInfo[pluginId].parameterData[paramIndex]['midiCC'] = cc

    def _set_currentProgram(self, pluginId, pIndex):
        self.fPluginsInfo[pluginId].programCurrent = pIndex

    def _set_currentMidiProgram(self, pluginId, mpIndex):
        self.fPluginsInfo[pluginId].midiProgramCurrent = mpIndex

    def _set_programName(self, pluginId, pIndex, name):
        if pIndex < self.fPluginsInfo[pluginId].programCount:
            self.fPluginsInfo[pluginId].programNames[pIndex] = name

    def _set_midiProgramData(self, pluginId, mpIndex, data):
        if mpIndex < self.fPluginsInfo[pluginId].midiProgramCount:
            self.fPluginsInfo[pluginId].midiProgramData[mpIndex] = data

    def _set_customData(self, pluginId, cdIndex, data):
        if cdIndex < self.fPluginsInfo[pluginId].customDataCount:
            self.fPluginsInfo[pluginId].customData[cdIndex] = data

    def _set_peaks(self, pluginId, in1, in2, out1, out2):
        self.fPluginsInfo[pluginId].peaks = [in1, in2, out1, out2]

# ------------------------------------------------------------------------------------------------------------
