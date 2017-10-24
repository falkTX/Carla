#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend utils
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

from os import environ
from sys import argv

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import *

# ------------------------------------------------------------------------------------------------------------

def getPluginTypeAsString(ptype):
    if ptype == PLUGIN_NONE:
        return "NONE"
    if ptype == PLUGIN_INTERNAL:
        return "Internal"
    if ptype == PLUGIN_LADSPA:
        return "LADSPA"
    if ptype == PLUGIN_DSSI:
        return "DSSI"
    if ptype == PLUGIN_LV2:
        return "LV2"
    if ptype == PLUGIN_VST2:
        return "VST2"
    if ptype == PLUGIN_GIG:
        return "GIG"
    if ptype == PLUGIN_SF2:
        return "SF2"
    if ptype == PLUGIN_SFZ:
        return "SFZ"
    if ptype == PLUGIN_JACK:
        return "JACK"

    print("getPluginTypeAsString(%i) - invalid type" % ptype);
    return "Unknown"

def getPluginTypeFromString(stype):
    if not stype:
        return PLUGIN_NONE

    stype = stype.lower()

    if stype == "none":
        return PLUGIN_NONE
    if stype in ("internal", "native"):
        return PLUGIN_INTERNAL
    if stype == "ladspa":
        return PLUGIN_LADSPA
    if stype == "dssi":
        return PLUGIN_DSSI
    if stype == "lv2":
        return PLUGIN_LV2
    if stype in ("vst2", "vst"):
        return PLUGIN_VST2
    if stype == "gig":
        return PLUGIN_GIG
    if stype == "sf2":
        return PLUGIN_SF2
    if stype == "sfz":
        return PLUGIN_SFZ
    if stype == "jack":
        return PLUGIN_JACK

    print("getPluginTypeFromString(\"%s\") - invalid string type" % stype)
    return PLUGIN_NONE

# ------------------------------------------------------------------------------------------------------------
# Carla Utils API (C stuff)

CarlaPipeClientHandle = c_void_p
CarlaPipeCallbackFunc = CFUNCTYPE(None, c_void_p, c_char_p)

# Information about an internal Carla plugin.
# @see carla_get_cached_plugin_info()
class CarlaCachedPluginInfo(Structure):
    _fields_ = [
        # Plugin category.
        ("category", c_enum),

        # Plugin hints.
        # @see PluginHints
        ("hints", c_uint),

        # Number of audio inputs.
        ("audioIns", c_uint32),

        # Number of audio outputs.
        ("audioOuts", c_uint32),

        # Number of MIDI inputs.
        ("midiIns", c_uint32),

        # Number of MIDI outputs.
        ("midiOuts", c_uint32),

        # Number of input parameters.
        ("parameterIns", c_uint32),

        # Number of output parameters.
        ("parameterOuts", c_uint32),

        # Plugin name.
        ("name", c_char_p),

        # Plugin label.
        ("label", c_char_p),

        # Plugin author/maker.
        ("maker", c_char_p),

        # Plugin copyright/license.
        ("copyright", c_char_p)
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Utils API (Python compatible stuff)

# @see CarlaCachedPluginInfo
PyCarlaCachedPluginInfo = {
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'audioIns': 0,
    'audioOuts': 0,
    'midiIns': 0,
    'midiOuts': 0,
    'parameterIns': 0,
    'parameterOuts': 0,
    'name':  "",
    'label': "",
    'maker': "",
    'copyright': ""
}

# ------------------------------------------------------------------------------------------------------------
# Carla Utils object using a DLL

class CarlaUtils(object):
    def __init__(self, filename):
        object.__init__(self)

        self.lib = cdll.LoadLibrary(filename)
        #self.lib = CDLL(filename, RTLD_GLOBAL)

        self.lib.carla_get_complete_license_text.argtypes = None
        self.lib.carla_get_complete_license_text.restype = c_char_p

        self.lib.carla_get_supported_file_extensions.argtypes = None
        self.lib.carla_get_supported_file_extensions.restype = c_char_p

        self.lib.carla_get_cached_plugin_count.argtypes = [c_enum, c_char_p]
        self.lib.carla_get_cached_plugin_count.restype = c_uint

        self.lib.carla_get_cached_plugin_info.argtypes = [c_enum, c_uint]
        self.lib.carla_get_cached_plugin_info.restype = POINTER(CarlaCachedPluginInfo)

        self.lib.carla_fflush.argtypes = [c_bool]
        self.lib.carla_fflush.restype = None

        self.lib.carla_fputs.argtypes = [c_bool, c_char_p]
        self.lib.carla_fputs.restype = None

        self.lib.carla_set_process_name.argtypes = [c_char_p]
        self.lib.carla_set_process_name.restype = None

        self.lib.carla_pipe_client_new.argtypes = [POINTER(c_char_p), CarlaPipeCallbackFunc, c_void_p]
        self.lib.carla_pipe_client_new.restype = CarlaPipeClientHandle

        self.lib.carla_pipe_client_idle.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_idle.restype = None

        self.lib.carla_pipe_client_is_running.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_is_running.restype = c_bool

        self.lib.carla_pipe_client_lock.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_lock.restype = None

        self.lib.carla_pipe_client_unlock.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_unlock.restype = None

        self.lib.carla_pipe_client_readlineblock.argtypes = [CarlaPipeClientHandle, c_uint]
        self.lib.carla_pipe_client_readlineblock.restype = c_char_p

        self.lib.carla_pipe_client_write_msg.argtypes = [CarlaPipeClientHandle, c_char_p]
        self.lib.carla_pipe_client_write_msg.restype = c_bool

        self.lib.carla_pipe_client_write_and_fix_msg.argtypes = [CarlaPipeClientHandle, c_char_p]
        self.lib.carla_pipe_client_write_and_fix_msg.restype = c_bool

        self.lib.carla_pipe_client_flush.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_flush.restype = c_bool

        self.lib.carla_pipe_client_flush_and_unlock.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_flush_and_unlock.restype = c_bool

        self.lib.carla_pipe_client_destroy.argtypes = [CarlaPipeClientHandle]
        self.lib.carla_pipe_client_destroy.restype = None

        self.lib.carla_x11_reparent_window.argtypes = [c_uintptr, c_uintptr]
        self.lib.carla_x11_reparent_window.restype = None

        self.lib.carla_x11_move_window.argtypes = [c_uintptr, c_int, c_int]
        self.lib.carla_x11_move_window.restype = None

        self.lib.carla_x11_get_window_pos.argtypes = [c_uintptr]
        self.lib.carla_x11_get_window_pos.restype = POINTER(c_int)

        # use _putenv on windows
        if not WINDOWS:
            self.msvcrt = None
            return

        self.msvcrt = cdll.msvcrt
        self.msvcrt._putenv.argtypes = [c_char_p]
        self.msvcrt._putenv.restype = None

    # --------------------------------------------------------------------------------------------------------

    # set environment variable
    def setenv(self, key, value):
        environ[key] = value

        if WINDOWS:
            keyvalue = "%s=%s" % (key, value)
            self.msvcrt._putenv(keyvalue.encode("utf-8"))

    # unset environment variable
    def unsetenv(self, key):
        if environ.get(key) is not None:
            environ.pop(key)

        if WINDOWS:
            keyrm = "%s=" % key
            self.msvcrt._putenv(keyrm.encode("utf-8"))

    # --------------------------------------------------------------------------------------------------------

    # Get the complete license text of used third-party code and features.
    # Returned string is in basic html format.
    def get_complete_license_text(self):
        return charPtrToString(self.lib.carla_get_complete_license_text())

    # Get all the supported file extensions in carla_load_file().
    # Returned string uses this syntax:
    # @code
    # "*.ext1;*.ext2;*.ext3"
    # @endcode
    def get_supported_file_extensions(self):
        return charPtrToString(self.lib.carla_get_supported_file_extensions())

    # Get how many internal plugins are available.
    def get_cached_plugin_count(self, ptype, pluginPath):
        return int(self.lib.carla_get_cached_plugin_count(ptype, pluginPath.encode("utf-8")))

    # Get information about a cached plugin.
    def get_cached_plugin_info(self, ptype, index):
        return structToDict(self.lib.carla_get_cached_plugin_info(ptype, index).contents)

    def fflush(self, err):
        self.lib.carla_fflush(err)

    def fputs(self, err, string):
        self.lib.carla_fputs(err, string.encode("utf-8"))

    def set_process_name(self, name):
        self.lib.carla_set_process_name(name.encode("utf-8"))

    def pipe_client_new(self, func):
        argc      = len(argv)
        cagrvtype = c_char_p * argc
        cargv     = cagrvtype()

        for i in range(argc):
            cargv[i] = c_char_p(argv[i].encode("utf-8"))

        self._pipeClientCallback = CarlaPipeCallbackFunc(func)
        return self.lib.carla_pipe_client_new(cargv, self._pipeClientCallback, None)

    def pipe_client_idle(self, handle):
        try:
            self.lib.carla_pipe_client_idle(handle)
        except OSError as e:
            print("pipe_client_idle", e)

    def pipe_client_is_running(self, handle):
        return bool(self.lib.carla_pipe_client_is_running(handle))

    def pipe_client_lock(self, handle):
        self.lib.carla_pipe_client_lock(handle)

    def pipe_client_unlock(self, handle):
        self.lib.carla_pipe_client_unlock(handle)

    def pipe_client_readlineblock(self, handle, timeout):
        return charPtrToString(self.lib.carla_pipe_client_readlineblock(handle, timeout))

    def pipe_client_write_msg(self, handle, msg):
        return bool(self.lib.carla_pipe_client_write_msg(handle, msg.encode("utf-8")))

    def pipe_client_write_and_fix_msg(self, handle, msg):
        return bool(self.lib.carla_pipe_client_write_and_fix_msg(handle, msg.encode("utf-8")))

    def pipe_client_flush(self, handle):
        return bool(self.lib.carla_pipe_client_flush(handle))

    def pipe_client_flush_and_unlock(self, handle):
        return bool(self.lib.carla_pipe_client_flush_and_unlock(handle))

    def pipe_client_destroy(self, handle):
        self.lib.carla_pipe_client_destroy(handle)

    def x11_reparent_window(self, winId1, winId2):
        self.lib.carla_x11_reparent_window(winId1, winId2)

    def x11_move_window(self, winId, x, y):
        self.lib.carla_x11_move_window(winId, x, y)

    def x11_get_window_pos(self, winId):
        data = self.lib.carla_x11_get_window_pos(winId)
        return (int(data[0]), int(data[1]))

# ------------------------------------------------------------------------------------------------------------
