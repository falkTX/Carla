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
    c_bool, c_char_p, c_double, c_int, c_uint, c_uint32, c_void_p, cast,
    cdll, Structure,
    CFUNCTYPE, POINTER
)

from sip import voidptr

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

# ------------------------------------------------------------------------------------------------------------
# Carla Frontend object using a DLL

class CarlaFrontendLib():
    def __init__(self, filename):
        self.lib = cdll.LoadLibrary(filename)

        self.lib.carla_frontend_createAndExecAboutJuceW.argtypes = (c_void_p,)
        self.lib.carla_frontend_createAndExecAboutJuceW.restype = None

        self.lib.carla_frontend_createAndExecJackApplicationW.argtypes = (c_void_p, c_char_p)
        self.lib.carla_frontend_createAndExecJackApplicationW.restype = POINTER(JackApplicationDialogResults)

    # --------------------------------------------------------------------------------------------------------

    def createAndExecAboutJuceW(self, parent):
        # FIXME cast(c_void_p, voidptr(parent))
        self.lib.carla_frontend_createAndExecAboutJuceW(None)

    def createAndExecJackApplicationW(self, parent, projectFilename):
        # FIXME cast(c_void_p, voidptr(parent))
        return structToDictOrNull(self.lib.carla_frontend_createAndExecJackApplicationW(None, projectFilename.encode("utf-8")))

# ------------------------------------------------------------------------------------------------------------
