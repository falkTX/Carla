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
