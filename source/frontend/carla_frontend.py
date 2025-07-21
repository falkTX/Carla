#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

# ------------------------------------------------------------------------------------------------------------
# Imports (ctypes)

from ctypes import (
    c_bool, c_char, c_char_p, c_int, c_uint, c_uint64, c_void_p, cast,
    cdll, Structure,
    POINTER
)

try:
    from sip import unwrapinstance
except ImportError:
    def unwrapinstance(_):
        return None

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

class HostSettings(Structure):
    _fields_ = [
        ("showPluginBridges", c_bool),
        ("showWineBridges", c_bool),
        ("useSystemIcons", c_bool),
        ("wineAutoPrefix", c_bool),
        ("wineExecutable", c_char_p),
        ("wineFallbackPrefix", c_char_p),
    ]

class PluginListDialogResults(Structure):
    _fields_ = [
        ("build", c_uint),
        ("type", c_uint),
        ("hints", c_uint),
        ("category", c_char_p),
        ("filename", c_char_p),
        ("name", c_char_p),
        ("label", c_char_p),
        ("maker", c_char_p),
        ("uniqueId", c_uint64),
        ("audioIns", c_uint),
        ("audioOuts", c_uint),
        ("cvIns", c_uint),
        ("cvOuts", c_uint),
        ("midiIns", c_uint),
        ("midiOuts", c_uint),
        ("parametersIns", c_uint),
        ("parametersOuts", c_uint),
    ]

# ------------------------------------------------------------------------------------------------------------
# Carla Frontend object using a DLL

class CarlaFrontendLib():
    def __init__(self, filename):
        self.lib = cdll.LoadLibrary(filename)

        self.lib.carla_frontend_createAndExecAboutDialog.argtypes = (c_void_p, c_void_p, c_bool, c_bool)
        self.lib.carla_frontend_createAndExecAboutDialog.restype = None

        self.lib.carla_frontend_createAndExecJackAppDialog.argtypes = (c_void_p, c_char_p)
        self.lib.carla_frontend_createAndExecJackAppDialog.restype = POINTER(JackApplicationDialogResults)

        self.lib.carla_frontend_createPluginListDialog.argtypes = (c_void_p, POINTER(HostSettings))
        self.lib.carla_frontend_createPluginListDialog.restype = c_void_p

        self.lib.carla_frontend_destroyPluginListDialog.argtypes = (c_void_p,)
        self.lib.carla_frontend_destroyPluginListDialog.restype = None

        self.lib.carla_frontend_setPluginListDialogPath.argtypes = (c_void_p, c_int, c_char_p)
        self.lib.carla_frontend_setPluginListDialogPath.restype = None

        self.lib.carla_frontend_execPluginListDialog.argtypes = (c_void_p,)
        self.lib.carla_frontend_execPluginListDialog.restype = POINTER(PluginListDialogResults)

    # --------------------------------------------------------------------------------------------------------

    def createAndExecAboutDialog(self, parent, hostHandle, isControl, isPlugin):
        return structToDictOrNull(self.lib.carla_frontend_createAndExecAboutDialog(unwrapinstance(parent),
                                                                                   hostHandle,
                                                                                   isControl,
                                                                                   isPlugin))

    def createAndExecJackAppDialog(self, parent, projectFilename):
        return structToDictOrNull(
            self.lib.carla_frontend_createAndExecJackAppDialog(unwrapinstance(parent),
                                                               projectFilename.encode("utf-8")))

    def createPluginListDialog(self, parent, hostSettings):
        hostSettingsC = HostSettings()
        hostSettingsC.showPluginBridges = hostSettings['showPluginBridges']
        hostSettingsC.showWineBridges = hostSettings['showWineBridges']
        hostSettingsC.useSystemIcons = hostSettings['useSystemIcons']
        hostSettingsC.wineAutoPrefix = hostSettings['wineAutoPrefix']
        hostSettingsC.wineExecutable = hostSettings['wineExecutable'].encode("utf-8")
        hostSettingsC.wineFallbackPrefix = hostSettings['wineFallbackPrefix'].encode("utf-8")
        return self.lib.carla_frontend_createPluginListDialog(unwrapinstance(parent), hostSettingsC)

    def destroyPluginListDialog(self, dialog):
        self.lib.carla_frontend_destroyPluginListDialog(dialog)

    def setPluginListDialogPath(self, dialog, ptype, path):
        self.lib.carla_frontend_setPluginListDialogPath(dialog, ptype, path.encode("utf-8"))

    def execPluginListDialog(self, dialog):
        return structToDictOrNull(self.lib.carla_frontend_execPluginListDialog(dialog))

# ------------------------------------------------------------------------------------------------------------
