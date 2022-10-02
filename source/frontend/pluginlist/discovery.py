#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin list code
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

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os

from copy import deepcopy
from subprocess import Popen, PIPE
from PyQt5.QtCore import qWarning

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Carla)

from carla_backend import (
    BINARY_NATIVE,
    BINARY_NONE,
    PLUGIN_AU,
    PLUGIN_DSSI,
    PLUGIN_LADSPA,
    PLUGIN_LV2,
    PLUGIN_NONE,
    PLUGIN_SF2,
    PLUGIN_SFZ,
    PLUGIN_VST2,
    PLUGIN_VST3,
    PLUGIN_CLAP,
)

from carla_shared import (
    LINUX,
    MACOS,
    WINDOWS,
)

from carla_utils import getPluginCategoryAsString

# ---------------------------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(binPath, pluginType, OS):
    binaries = []

    if OS == "HAIKU":
        extensions = ("") if pluginType == PLUGIN_VST2 else (".so",)
    elif OS == "MACOS":
        extensions = (".dylib", ".so")
    elif OS == "WINDOWS":
        extensions = (".dll",)
    else:
        extensions = (".so",)

    for root, _, files in os.walk(binPath):
        for name in tuple(name for name in files if name.lower().endswith(extensions)):
            binaries.append(os.path.join(root, name))

    return binaries

def findVST3Binaries(binPath):
    binaries = []

    for root, dirs, files in os.walk(binPath):
        for name in tuple(name for name in (files+dirs) if name.lower().endswith(".vst3")):
            binaries.append(os.path.join(root, name))

    return binaries

def findCLAPBinaries(binPath):
    binaries = []

    for root, dirs, files in os.walk(binPath):
        for name in tuple(name for name in (files+dirs) if name.lower().endswith(".clap")):
            binaries.append(os.path.join(root, name))

    return binaries

def findLV2Bundles(bundlePath):
    bundles = []

    for root, _, _2 in os.walk(bundlePath, followlinks=True):
        if root == bundlePath:
            continue
        if os.path.exists(os.path.join(root, "manifest.ttl")):
            bundles.append(root)

    return bundles

def findMacVSTBundles(bundlePath, isVST3):
    bundles = []
    extension = ".vst3" if isVST3 else ".vst"

    for root, dirs, _ in os.walk(bundlePath, followlinks=True):
        for name in tuple(name for name in dirs if name.lower().endswith(extension)):
            bundles.append(os.path.join(root, name))

    return bundles

def findFilenames(filePath, stype):
    filenames = []

    if stype == "sf2":
        extensions = (".sf2",".sf3",)
    else:
        return []

    for root, _, files in os.walk(filePath):
        for name in tuple(name for name in files if name.lower().endswith(extensions)):
            filenames.append(os.path.join(root, name))

    return filenames

# ---------------------------------------------------------------------------------------------------------------------
# Plugin Query

# NOTE: this code is ugly, it is meant to be replaced, so let it be as-is for now

PLUGIN_QUERY_API_VERSION = 12

PyPluginInfo = {
    'API': PLUGIN_QUERY_API_VERSION,
    'valid': False,
    'build': BINARY_NONE,
    'type': PLUGIN_NONE,
    'hints': 0x0,
    'category': "",
    'filename': "",
    'name': "",
    'label': "",
    'maker': "",
    'uniqueId': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'cv.ins': 0,
    'cv.outs': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'parameters.ins': 0,
    'parameters.outs': 0
}

gDiscoveryProcess = None

def findWinePrefix(filename, recursionLimit = 10):
    if recursionLimit == 0 or len(filename) < 5 or "/" not in filename:
        return ""

    path = filename[:filename.rfind("/")]

    if os.path.isdir(path + "/dosdevices"):
        return path

    return findWinePrefix(path, recursionLimit-1)

def runCarlaDiscovery(itype, stype, filename, tool, wineSettings=None):
    if not os.path.exists(tool):
        qWarning(f"runCarlaDiscovery() - tool '{tool}' does not exist")
        return []

    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        command.append("LD_PRELOAD=")
        if wineSettings is not None:
            command.append("WINEDEBUG=-all")

            if wineSettings['autoPrefix']:
                winePrefix = findWinePrefix(filename)
            else:
                winePrefix = ""

            if not winePrefix:
                envWinePrefix = os.getenv("WINEPREFIX")

                if envWinePrefix:
                    winePrefix = envWinePrefix
                elif wineSettings['fallbackPrefix']:
                    winePrefix = os.path.expanduser(wineSettings['fallbackPrefix'])
                else:
                    winePrefix = os.path.expanduser("~/.wine")

            wineCMD = wineSettings['executable'] if wineSettings['executable'] else "wine"

            if tool.endswith("64.exe") and os.path.exists(wineCMD + "64"):
                wineCMD += "64"

            command.append("WINEPREFIX=" + winePrefix)
            command.append(wineCMD)

    command.append(tool)
    command.append(stype)
    command.append(filename)

    # pylint: disable=global-statement
    global gDiscoveryProcess
    # pylint: enable=global-statement

    # pylint: disable=consider-using-with
    gDiscoveryProcess = Popen(command, stdout=PIPE)
    # pylint: enable=consider-using-with

    pinfo = None
    plugins = []
    fakeLabel = os.path.basename(filename).rsplit(".", 1)[0]

    while True:
        try:
            line = gDiscoveryProcess.stdout.readline().decode("utf-8", errors="ignore")
        except:
            print("ERROR: discovery readline failed")
            break

        # line is valid, strip it
        if line:
            line = line.strip()

        # line is invalid, try poll() again
        elif gDiscoveryProcess.poll() is None:
            continue

        # line is invalid and poll() failed, stop here
        else:
            break

        if line == "carla-discovery::init::-----------":
            pinfo = deepcopy(PyPluginInfo)
            pinfo['type']     = itype
            pinfo['filename'] = filename if filename != ":all" else ""

        elif line == "carla-discovery::end::------------":
            if pinfo is not None:
                plugins.append(pinfo)
                del pinfo
                pinfo = None

        elif line == "Segmentation fault":
            print(f"carla-discovery::crash::{filename} crashed during discovery")

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::info::"):
            print(f"{line} - {filename}")

        elif line.startswith("carla-discovery::warning::"):
            print(f"{line} - {filename}")

        elif line.startswith("carla-discovery::error::"):
            print(f"{line} - {filename}")

        elif line.startswith("carla-discovery::"):
            if pinfo is None:
                continue

            try:
                prop, value = line.replace("carla-discovery::", "").split("::", 1)
            except:
                continue

            # pylint: disable=unsupported-assignment-operation
            if prop == "build":
                if value.isdigit():
                    pinfo['build'] = int(value)
            elif prop == "name":
                pinfo['name'] = value if value else fakeLabel
            elif prop == "label":
                pinfo['label'] = value if value else fakeLabel
            elif prop == "filename":
                pinfo['filename'] = value
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "category":
                pinfo['category'] = value
            elif prop == "uniqueId":
                if value.isdigit():
                    pinfo['uniqueId'] = int(value)
            elif prop == "hints":
                if value.isdigit():
                    pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit():
                    pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit():
                    pinfo['audio.outs'] = int(value)
            elif prop == "cv.ins":
                if value.isdigit():
                    pinfo['cv.ins'] = int(value)
            elif prop == "cv.outs":
                if value.isdigit():
                    pinfo['cv.outs'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit():
                    pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit():
                    pinfo['midi.outs'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit():
                    pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit():
                    pinfo['parameters.outs'] = int(value)
            elif prop == "uri":
                if value:
                    pinfo['label'] = value
                else:
                    # cannot use empty URIs
                    del pinfo
                    pinfo = None
                    continue
            else:
                print(f"{line} - {filename} (unknown property)")
            # pylint: enable=unsupported-assignment-operation

    tmp = gDiscoveryProcess
    gDiscoveryProcess = None
    del tmp

    return plugins

def killDiscovery():
    # pylint: disable=global-variable-not-assigned
    global gDiscoveryProcess
    # pylint: enable=global-variable-not-assigned

    if gDiscoveryProcess is not None:
        gDiscoveryProcess.kill()

def checkPluginCached(desc, ptype):
    pinfo = deepcopy(PyPluginInfo)
    pinfo['build'] = BINARY_NATIVE
    pinfo['type']  = ptype
    pinfo['hints'] = desc['hints']
    pinfo['name']  = desc['name']
    pinfo['label'] = desc['label']
    pinfo['maker'] = desc['maker']
    pinfo['category'] = getPluginCategoryAsString(desc['category'])

    pinfo['audio.ins']  = desc['audioIns']
    pinfo['audio.outs'] = desc['audioOuts']

    pinfo['cv.ins']  = desc['cvIns']
    pinfo['cv.outs'] = desc['cvOuts']

    pinfo['midi.ins']  = desc['midiIns']
    pinfo['midi.outs'] = desc['midiOuts']

    pinfo['parameters.ins']  = desc['parameterIns']
    pinfo['parameters.outs'] = desc['parameterOuts']

    if ptype == PLUGIN_LV2:
        pinfo['filename'], pinfo['label'] = pinfo['label'].split('\\' if WINDOWS else '/',1)

    elif ptype == PLUGIN_SFZ:
        pinfo['filename'] = pinfo['label']
        pinfo['label']    = pinfo['name']

    return pinfo

def checkPluginLADSPA(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, wineSettings)

def checkPluginDSSI(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, wineSettings)

def checkPluginLV2(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, wineSettings)

def checkPluginVST2(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_VST2, "VST2", filename, tool, wineSettings)

def checkPluginVST3(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_VST3, "VST3", filename, tool, wineSettings)

def checkPluginCLAP(filename, tool, wineSettings=None):
    return runCarlaDiscovery(PLUGIN_CLAP, "CLAP", filename, tool, wineSettings)

def checkFileSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkFileSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

def checkAllPluginsAU(tool):
    return runCarlaDiscovery(PLUGIN_AU, "AU", ":all", tool)
