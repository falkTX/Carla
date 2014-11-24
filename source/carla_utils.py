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
    if ptype == PLUGIN_VST:
        return "VST"
    if ptype == PLUGIN_VST3:
        return "VST3"
    if ptype == PLUGIN_AU:
        return "AU"
    if ptype == PLUGIN_GIG:
        return "GIG"
    if ptype == PLUGIN_SF2:
        return "SF2"
    if ptype == PLUGIN_SFZ:
        return "SFZ"

    print("getPluginTypeAsString(%i) - invalid type" % ptype);
    return "Unknown"

def getPluginTypeFromString(stype):
    if not stype:
        return PLUGIN_NONE

    stype = stype.lower()

    if stype == "none":
        return PLUGIN_NONE
    if stype == "internal":
        return PLUGIN_INTERNAL
    if stype == "ladspa":
        return PLUGIN_LADSPA
    if stype == "dssi":
        return PLUGIN_DSSI
    if stype == "lv2":
        return PLUGIN_LV2
    if stype == "vst":
        return PLUGIN_VST
    if stype == "vst3":
        return PLUGIN_VST3
    if stype == "au":
        return PLUGIN_AU
    if stype == "gig":
        return PLUGIN_GIG
    if stype == "sf2":
        return PLUGIN_SF2
    if stype == "sfz":
        return PLUGIN_SFZ

    print("getPluginTypeFromString(\"%s\") - invalid string type" % stype)
    return PLUGIN_NONE

# ------------------------------------------------------------------------------------------------------------
# Carla Pipe stuff

CarlaPipeClientHandle = c_void_p
CarlaPipeCallbackFunc = CFUNCTYPE(None, c_void_p, c_char_p)

# ------------------------------------------------------------------------------------------------------------
# Carla Utils object using a DLL

class CarlaUtils(object):
    def __init__(self, filename):
        object.__init__(self)

        self.lib = cdll.LoadLibrary(filename)

        self.lib.carla_get_library_filename.argtypes = None
        self.lib.carla_get_library_filename.restype = c_char_p

        self.lib.carla_get_library_folder.argtypes = None
        self.lib.carla_get_library_folder.restype = c_char_p

        self.lib.carla_set_locale_C.argtypes = None
        self.lib.carla_set_locale_C.restype = None

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

    # --------------------------------------------------------------------------------------------------------

    def get_library_filename(self):
        return charPtrToString(self.lib.carla_get_library_filename())

    def get_library_folder(self):
        return charPtrToString(self.lib.carla_get_library_folder())

    def set_locale_C(self):
        self.lib.carla_set_locale_C()

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
        self.lib.carla_pipe_client_idle(handle)

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

# ------------------------------------------------------------------------------------------------------------
