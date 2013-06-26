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
# For a full copy of the GNU General Public License see the GPL.txt file

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
# Convert a ctypes char** into a python string list

def charStringList(charPtr):
    i = 0
    retList = []

    if not charPtr:
        return retList

    while True:
        char_p = charPtr[i]

        if not char_p:
            break

        retList.append(char_p.decode("utf-8", errors="ignore"))
        i += 1

    return retList

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict

def structToDict(struct):
    return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------
# Backend defines

MAX_DEFAULT_PLUGINS    = 99
MAX_RACK_PLUGINS       = 16
MAX_PATCHBAY_PLUGINS   = 999
MAX_DEFAULT_PARAMETERS = 200

# Plugin Hints
PLUGIN_IS_BRIDGE         = 0x001
PLUGIN_IS_RTSAFE         = 0x002
PLUGIN_IS_SYNTH          = 0x004
PLUGIN_HAS_GUI           = 0x010
PLUGIN_HAS_GUI_AS_FILE   = 0x020
PLUGIN_HAS_SINGLE_THREAD = 0x040
PLUGIN_CAN_DRYWET        = 0x100
PLUGIN_CAN_VOLUME        = 0x200
PLUGIN_CAN_BALANCE       = 0x400
PLUGIN_CAN_PANNING       = 0x800

# Plugin Options
PLUGIN_OPTION_FIXED_BUFFER          = 0x001
PLUGIN_OPTION_FORCE_STEREO          = 0x002
PLUGIN_OPTION_MAP_PROGRAM_CHANGES   = 0x004
PLUGIN_OPTION_USE_CHUNKS            = 0x008
PLUGIN_OPTION_SEND_CONTROL_CHANGES  = 0x010
PLUGIN_OPTION_SEND_CHANNEL_PRESSURE = 0x020
PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH  = 0x040
PLUGIN_OPTION_SEND_PITCHBEND        = 0x080
PLUGIN_OPTION_SEND_ALL_SOUND_OFF    = 0x100

# Parameter Hints
PARAMETER_IS_BOOLEAN       = 0x01
PARAMETER_IS_INTEGER       = 0x02
PARAMETER_IS_LOGARITHMIC   = 0x04
PARAMETER_IS_ENABLED       = 0x08
PARAMETER_IS_AUTOMABLE     = 0x10
PARAMETER_USES_SAMPLERATE  = 0x20
PARAMETER_USES_SCALEPOINTS = 0x40
PARAMETER_USES_CUSTOM_TEXT = 0x80

# Patchbay Port Hints
PATCHBAY_PORT_IS_INPUT  = 0x01
PATCHBAY_PORT_IS_OUTPUT = 0x02
PATCHBAY_PORT_IS_AUDIO  = 0x04
PATCHBAY_PORT_IS_CV     = 0x08
PATCHBAY_PORT_IS_MIDI   = 0x10

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
PLUGIN_VST3     = 6
PLUGIN_GIG      = 7
PLUGIN_SF2      = 8
PLUGIN_SFZ      = 9

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
PARAMETER_CTRL_CHANNEL  = -8
PARAMETER_MAX           = -9

# Patchbay Icon Type
PATCHBAY_ICON_APPLICATION = 0
PATCHBAY_ICON_HARDWARE    = 1
PATCHBAY_ICON_CARLA       = 2
PATCHBAY_ICON_DISTRHO     = 3
PATCHBAY_ICON_FILE        = 4
PATCHBAY_ICON_PLUGIN      = 5

# Options Type
OPTION_PROCESS_NAME            = 0
OPTION_PROCESS_MODE            = 1
OPTION_TRANSPORT_MODE          = 2
OPTION_FORCE_STEREO            = 3
OPTION_PREFER_PLUGIN_BRIDGES   = 4
OPTION_PREFER_UI_BRIDGES       = 5
OPTION_USE_DSSI_VST_CHUNKS     = 6
OPTION_MAX_PARAMETERS          = 7
OPTION_OSC_UI_TIMEOUT          = 8
OPTION_JACK_AUTOCONNECT        = 9
OPTION_JACK_TIMEMASTER         = 10
OPTION_RTAUDIO_BUFFER_SIZE     = 11
OPTION_RTAUDIO_SAMPLE_RATE     = 12
OPTION_RTAUDIO_DEVICE          = 13
OPTION_PATH_RESOURCES          = 14
OPTION_PATH_BRIDGE_NATIVE      = 15
OPTION_PATH_BRIDGE_POSIX32     = 16
OPTION_PATH_BRIDGE_POSIX64     = 17
OPTION_PATH_BRIDGE_WIN32       = 18
OPTION_PATH_BRIDGE_WIN64       = 19
OPTION_PATH_BRIDGE_LV2_GTK2    = 20
OPTION_PATH_BRIDGE_LV2_GTK3    = 21
OPTION_PATH_BRIDGE_LV2_QT4     = 22
OPTION_PATH_BRIDGE_LV2_QT5     = 23
OPTION_PATH_BRIDGE_LV2_COCOA   = 24
OPTION_PATH_BRIDGE_LV2_WINDOWS = 25
OPTION_PATH_BRIDGE_LV2_X11     = 26
OPTION_PATH_BRIDGE_VST_COCOA   = 27
OPTION_PATH_BRIDGE_VST_HWND    = 28
OPTION_PATH_BRIDGE_VST_X11     = 29

# Callback Type
CALLBACK_DEBUG          = 0
CALLBACK_PLUGIN_ADDED   = 1
CALLBACK_PLUGIN_REMOVED = 2
CALLBACK_PLUGIN_RENAMED = 3
CALLBACK_PARAMETER_VALUE_CHANGED        = 4
CALLBACK_PARAMETER_DEFAULT_CHANGED      = 5
CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED = 6
CALLBACK_PARAMETER_MIDI_CC_CHANGED      = 7
CALLBACK_PROGRAM_CHANGED         = 8
CALLBACK_MIDI_PROGRAM_CHANGED    = 9
CALLBACK_NOTE_ON                 = 10
CALLBACK_NOTE_OFF                = 11
CALLBACK_SHOW_GUI                = 12
CALLBACK_UPDATE                  = 13
CALLBACK_RELOAD_INFO             = 14
CALLBACK_RELOAD_PARAMETERS       = 15
CALLBACK_RELOAD_PROGRAMS         = 16
CALLBACK_RELOAD_ALL              = 17
CALLBACK_PATCHBAY_CLIENT_ADDED   = 18
CALLBACK_PATCHBAY_CLIENT_REMOVED = 19
CALLBACK_PATCHBAY_CLIENT_RENAMED = 20
CALLBACK_PATCHBAY_PORT_ADDED         = 21
CALLBACK_PATCHBAY_PORT_REMOVED       = 22
CALLBACK_PATCHBAY_PORT_RENAMED       = 23
CALLBACK_PATCHBAY_CONNECTION_ADDED   = 24
CALLBACK_PATCHBAY_CONNECTION_REMOVED = 25
CALLBACK_PATCHBAY_ICON_CHANGED = 26
CALLBACK_BUFFER_SIZE_CHANGED  = 27
CALLBACK_SAMPLE_RATE_CHANGED  = 28
CALLBACK_PROCESS_MODE_CHANGED = 29
CALLBACK_NSM_ANNOUNCE = 30
CALLBACK_NSM_OPEN     = 31
CALLBACK_NSM_SAVE     = 32
CALLBACK_ERROR = 33
CALLBACK_INFO  = 34
CALLBACK_QUIT  = 35

# Process Mode
PROCESS_MODE_SINGLE_CLIENT    = 0
PROCESS_MODE_MULTIPLE_CLIENTS = 1
PROCESS_MODE_CONTINUOUS_RACK  = 2
PROCESS_MODE_PATCHBAY         = 3
PROCESS_MODE_BRIDGE           = 4

# Transport Mode
TRANSPORT_MODE_INTERNAL = 0
TRANSPORT_MODE_JACK     = 1
TRANSPORT_MODE_PLUGIN   = 2
TRANSPORT_MODE_BRIDGE   = 3

# Set BINARY_NATIVE
if HAIKU or LINUX or MACOS:
    BINARY_NATIVE = BINARY_POSIX64 if kIs64bit else BINARY_POSIX32
elif WINDOWS:
    BINARY_NATIVE = BINARY_WIN64 if kIs64bit else BINARY_WIN32
else:
    BINARY_NATIVE = BINARY_OTHER

# ------------------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int
c_nullptr = None

CallbackFunc = CFUNCTYPE(None, c_void_p, c_enum, c_uint, c_int, c_int, c_float, c_char_p)

class ParameterData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("index", c_int32),
        ("rindex", c_int32),
        ("hints", c_uint32),
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

# ------------------------------------------------------------------------------------------------------------
# Standalone C++ -> Python variables

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
        ("uniqueId", c_long),
        ("latency", c_uint32)
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
        ("frame", c_uint32),
        ("bar", c_int32),
        ("beat", c_int32),
        ("tick", c_int32),
        ("bpm", c_double)
    ]

# ------------------------------------------------------------------------------------------------------------
# Standalone Python object

class Host(object):
    def __init__(self, libName):
        object.__init__(self)

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

        self.lib.carla_set_engine_callback.argtypes = [CallbackFunc, c_void_p]
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
        self.lib.carla_patchbay_refresh.restype = None

        self.lib.carla_transport_play.argtypes = None
        self.lib.carla_transport_play.restype = None

        self.lib.carla_transport_pause.argtypes = None
        self.lib.carla_transport_pause.restype = None

        self.lib.carla_transport_relocate.argtypes = [c_uint32]
        self.lib.carla_transport_relocate.restype = None

        self.lib.carla_get_current_transport_frame.argtypes = None
        self.lib.carla_get_current_transport_frame.restype = c_uint32

        self.lib.carla_get_transport_info.argtypes = None
        self.lib.carla_get_transport_info.restype = POINTER(CarlaTransportInfo)

        self.lib.carla_add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_char_p, c_void_p]
        self.lib.carla_add_plugin.restype = c_bool

        self.lib.carla_remove_plugin.argtypes = [c_uint]
        self.lib.carla_remove_plugin.restype = c_bool

        self.lib.carla_remove_all_plugins.argtypes = None
        self.lib.carla_remove_all_plugins.restype = None

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

    def get_extended_license_text(self):
        return self.lib.carla_get_extended_license_text()

    def get_supported_file_types(self):
        return self.lib.carla_get_supported_file_types()

    def get_engine_driver_count(self):
        return self.lib.carla_get_engine_driver_count()

    def get_engine_driver_name(self, index):
        return self.lib.carla_get_engine_driver_name(index)

    def get_engine_driver_device_names(self, index):
        return charStringList(self.lib.carla_get_engine_driver_device_names(index))

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
        self._callback = CallbackFunc(func)
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
        self.lib.carla_patchbay_refresh()

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
        self.lib.carla_remove_all_plugins()

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
