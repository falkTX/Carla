#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend utils
# Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

# ------------------------------------------------------------------------------------------------------------
# Imports (ctypes)

from ctypes import (
    c_bool, c_char_p, c_int, c_void_p, cast,
    cdll, Structure,
    POINTER
)

from sip import unwrapinstance

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import (
    structToDict
)

# ---------------------------------------------------------------------------------------------------------------------
# Convert a ctypes struct into a python dict, or None if null

def structToDictOrNull(struct):
    return structToDict(struct.contents) if struct else None

# ------------------------------------------------------------------------------------------------------------
# Carla Frontend API (C stuff)

class JackApplicationDialogResults(Structure):
    _fields_ = [
        ("command", c_char_p),
        ("name", c_char_p),
        ("labelSetup", c_char_p)
    ]

class PluginListDialogResults(Structure):
    _fields_ = [
        ("btype", c_int),
        ("ptype", c_int),
        ("binary", c_char_p),
        ("label", c_char_p)
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Frontend object using a DLL

class CarlaFrontendLib():
    def __init__(self, filename):
        self.lib = cdll.LoadLibrary(filename)

        self.lib.carla_frontend_createAndExecAboutJuceDialog.argtypes = (c_void_p,)
        self.lib.carla_frontend_createAndExecAboutJuceDialog.restype = None

        self.lib.carla_frontend_createAndExecJackAppDialog.argtypes = (c_void_p, c_char_p)
        self.lib.carla_frontend_createAndExecJackAppDialog.restype = POINTER(JackApplicationDialogResults)

        self.lib.carla_frontend_createAndExecPluginListDialog.argtypes = (c_void_p, c_bool)
        self.lib.carla_frontend_createAndExecPluginListDialog.restype = POINTER(JackApplicationDialogResults)

    # --------------------------------------------------------------------------------------------------------

    def createAndExecAboutJuceDialog(self, parent):
        self.lib.carla_frontend_createAndExecAboutJuceDialog(unwrapinstance(parent))

    def createAndExecJackAppDialog(self, parent, projectFilename):
        return structToDictOrNull(self.lib.carla_frontend_createAndExecJackAppDialog(unwrapinstance(parent), projectFilename.encode("utf-8")))

    def createAndExecPluginListDialog(self, parent, useSystemIcons):
        self.lib.carla_frontend_createAndExecPluginListDialog(unwrapinstance(parent), useSystemIcons)

# ------------------------------------------------------------------------------------------------------------
