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

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict

def structToDict(struct):
    return dict((attr, getattr(struct, attr)) for attr, value in struct._fields_)

# ------------------------------------------------------------------------------------------------------------
# Backend C++ -> Python variables

c_enum = c_int
c_nullptr = None

#if kIs64bit:
    #c_uintptr = c_uint64
#else:
    #c_uintptr = c_uint32

CallbackFunc = CFUNCTYPE(None, c_void_p, c_enum, c_int, c_int, c_int, c_double, c_char_p)

class ParameterData(Structure):
    _fields_ = [
        ("type", c_enum),
        ("index", c_int32),
        ("rindex", c_int32),
        ("hints", c_int32),
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
        ("binary", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("copyright", c_char_p),
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

# ------------------------------------------------------------------------------------------------------------
# Standalone Python object

class Host(object):
    def __init__(self, lib_prefix_arg):
        object.__init__(self)

        global carla_library_path

        if lib_prefix_arg:
            carla_library_path = os.path.join(lib_prefix_arg, "lib", "carla", carla_libname)

        if not carla_library_path:
            self.lib = None
            return

        self.lib = cdll.LoadLibrary(carla_library_path)

        self.lib.carla_get_extended_license_text.argtypes = None
        self.lib.carla_get_extended_license_text.restype = c_char_p

        self.lib.carla_get_engine_driver_count.argtypes = None
        self.lib.carla_get_engine_driver_count.restype = c_uint

        self.lib.carla_get_engine_driver_name.argtypes = [c_uint]
        self.lib.carla_get_engine_driver_name.restype = c_char_p

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

        self.lib.carla_add_plugin.argtypes = [c_enum, c_enum, c_char_p, c_char_p, c_char_p, c_void_p]
        self.lib.carla_add_plugin.restype = c_bool

        self.lib.carla_remove_plugin.argtypes = [c_uint]
        self.lib.carla_remove_plugin.restype = c_bool

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

        # TODO - consider removal
        self.lib.carla_get_default_parameter_value.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_default_parameter_value.restype = c_float

        # TODO - consider removal
        self.lib.carla_get_current_parameter_value.argtypes = [c_uint, c_uint32]
        self.lib.carla_get_current_parameter_value.restype = c_float

        self.lib.carla_get_input_peak_value.argtypes = [c_uint, c_ushort]
        self.lib.carla_get_input_peak_value.restype = c_float

        self.lib.carla_get_output_peak_value.argtypes = [c_uint, c_ushort]
        self.lib.carla_get_output_peak_value.restype = c_float

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

        self.lib.carla_get_host_osc_url.argtypes = None
        self.lib.carla_get_host_osc_url.restype = c_char_p

        self.lib.carla_set_callback_function.argtypes = [CallbackFunc]
        self.lib.carla_set_callback_function.restype = None

        self.lib.carla_set_option.argtypes = [c_enum, c_int, c_char_p]
        self.lib.carla_set_option.restype = None

        #self.lib.nsm_announce.argtypes = [c_char_p, c_int]
        #self.lib.nsm_announce.restype = None

        #self.lib.nsm_reply_open.argtypes = None
        #self.lib.nsm_reply_open.restype = None

        #self.lib.nsm_reply_save.argtypes = None
        #self.lib.nsm_reply_save.restype = None

    def get_extended_license_text(self):
        return self.lib.carla_get_extended_license_text()

    def get_engine_driver_count(self):
        return self.lib.carla_get_engine_driver_count()

    def get_engine_driver_name(self, index):
        return self.lib.carla_get_engine_driver_name(index)

    def get_internal_plugin_count(self):
        return self.lib.carla_get_internal_plugin_count()

    def get_internal_plugin_info(self, internalPluginId):
        return structToDict(self.lib.carla_get_internal_plugin_info(internalPluginId).contents)

    def engine_init(self, driverName, clientName):
        return self.lib.carla_engine_init(driverName.encode("utf-8"), clientName.encode("utf-8"))

    def engine_close(self):
        return self.lib.carla_engine_close()

    def engine_idle(self):
        return self.lib.carla_engine_idle()

    def is_engine_running(self):
        return self.lib.carla_is_engine_running()

    def add_plugin(self, btype, ptype, filename, name, label, extraStuff):
        cname = name.encode("utf-8") if name else c_nullptr
        return self.lib.carla_add_plugin(btype, ptype, filename.encode("utf-8"), cname, label.encode("utf-8"), cast(extraStuff, c_void_p))

    def remove_plugin(self, pluginId):
        return self.lib.carla_remove_plugin(pluginId)

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

    def get_host_osc_url(self):
        return self.lib.carla_get_host_osc_url()

    def get_buffer_size(self):
        return self.lib.carla_get_buffer_size()

    def get_sample_rate(self):
        return self.lib.carla_get_sample_rate()

    def set_callback_function(self, func):
        self._callback = CallbackFunc(func)
        self.lib.carla_set_callback_function(self._callback)

    def set_option(self, option, value, valueStr):
        self.lib.carla_set_option(option, value, valueStr.encode("utf-8"))

    #def nsm_announce(self, url, pid):
        #self.lib.nsm_announce(url.encode("utf-8"), pid)

    #def nsm_reply_open(self):
        #self.lib.nsm_reply_open()

    #def nsm_reply_save(self):
        #self.lib.nsm_reply_save()

#Carla.host = Host(None)

## Test available drivers
#driverCount = Carla.host.get_engine_driver_count()
#driverList  = []
#for i in range(driverCount):
    #driver = cString(Carla.host.get_engine_driver_name(i))
    #if driver:
        #driverList.append(driver)
        #print(i, driver)

## Test available internal plugins
#pluginCount = Carla.host.get_internal_plugin_count()
#for i in range(pluginCount):
    #plugin = Carla.host.get_internal_plugin_info(i)
    #print(plugin)
