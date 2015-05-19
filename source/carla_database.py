#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin database code
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
# Imports (Config)

from carla_config import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from copy import deepcopy
from subprocess import Popen, PIPE

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QThread, QSettings
    from PyQt5.QtWidgets import QDialog, QTableWidgetItem
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt, QThread, QSettings
    from PyQt4.QtGui import QDialog, QTableWidgetItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_database
import ui_carla_refresh

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Try Import LADSPA-RDF

if not CXFREEZE:
    try:
        import ladspa_rdf
        import json
        haveLRDF = True
    except:
        qWarning("LRDF Support not available (LADSPA-RDF will be disabled)")
        haveLRDF = False
else:
    qWarning("LRDF Support disabled for static build (LADSPA-RDF will be disabled)")
    haveLRDF = False

# ------------------------------------------------------------------------------------------------------------
# Set LADSPA-RDF Path

if haveLRDF and readEnvVars:
    LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
    if LADSPA_RDF_PATH_env:
        try:
            ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))
        except:
            pass
    del LADSPA_RDF_PATH_env

# ------------------------------------------------------------------------------------------------------------
# Plugin Query (helper functions)

def findBinaries(binPath, OS):
    binaries = []

    if OS == "WINDOWS":
        extensions = (".dll",)
    elif OS == "MACOS":
        extensions = (".dylib", ".so")
    else:
        extensions = (".so",)

    for root, dirs, files in os.walk(binPath):
        for name in [name for name in files if name.lower().endswith(extensions)]:
            binaries.append(os.path.join(root, name))

    return binaries

def findVST3Binaries(binPath):
    binaries = []

    for root, dirs, files in os.walk(binPath):
        for name in [name for name in files if name.lower().endswith(".vst3")]:
            binaries.append(os.path.join(root, name))

    return binaries

def findLV2Bundles(bundlePath):
    bundles = []

    for root, dirs, files in os.walk(bundlePath, followlinks=True):
        if root == bundlePath: continue
        if os.path.exists(os.path.join(root, "manifest.ttl")):
            bundles.append(root)

    return bundles

def findMacVSTBundles(bundlePath, isVST3):
    bundles = []

    for root, dirs, files in os.walk(bundlePath, followlinks=True):
        #if root == bundlePath: continue # FIXME
        for name in [name for name in dirs if name.lower().endswith(".vst3" if isVST3 else ".vst")]:
            bundles.append(os.path.join(root, name))

    return bundles

def findFilenames(filePath, stype):
    filenames = []

    if stype == "gig":
        extensions = (".gig",)
    elif stype == "sf2":
        extensions = (".sf2",)
    elif stype == "sfz":
        extensions = (".sfz",)
    else:
        return []

    for root, dirs, files in os.walk(filePath):
        for name in [name for name in files if name.lower().endswith(extensions)]:
            filenames.append(os.path.join(root, name))

    return filenames

# ------------------------------------------------------------------------------------------------------------
# Plugin Query

PLUGIN_QUERY_API_VERSION = 6

PyPluginInfo = {
    'API': PLUGIN_QUERY_API_VERSION,
    'build': BINARY_NONE,
    'type': PLUGIN_NONE,
    'hints': 0x0,
    'filename': "",
    'name': "",
    'label': "",
    'maker': "",
    'uniqueId': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'parameters.ins': 0,
    'parameters.outs': 0
}

global gDiscoveryProcess
gDiscoveryProcess = None

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
    if not os.path.exists(tool):
        qWarning("runCarlaDiscovery() - tool '%s' does not exist" % tool)
        return

    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        command.append("LD_PRELOAD=")
        if isWine:
            command.append("WINEDEBUG=-all")
            command.append("wine")

    command.append(tool)
    command.append(stype)
    command.append(filename)

    global gDiscoveryProcess
    gDiscoveryProcess = Popen(command, stdout=PIPE)

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
            pinfo['filename'] = filename

        elif line == "carla-discovery::end::------------":
            if pinfo is not None:
                plugins.append(pinfo)
                del pinfo
                pinfo = None

        elif line == "Segmentation fault":
            print("carla-discovery::crash::%s crashed during discovery" % filename)

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::info::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::warning::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::error::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::"):
            if pinfo == None:
                continue

            try:
                prop, value = line.replace("carla-discovery::", "").split("::", 1)
            except:
                continue

            if prop == "build":
                if value.isdigit(): pinfo['build'] = int(value)
            elif prop == "name":
                pinfo['name'] = value if value else fakeLabel
            elif prop == "label":
                pinfo['label'] = value if value else fakeLabel
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "uniqueId":
                if value.isdigit(): pinfo['uniqueId'] = int(value)
            elif prop == "hints":
                if value.isdigit(): pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit(): pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit(): pinfo['audio.outs'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit(): pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit(): pinfo['midi.outs'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit(): pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit(): pinfo['parameters.outs'] = int(value)
            elif prop == "uri":
                if value:
                    pinfo['label'] = value
                else:
                    # cannot use empty URIs
                    del pinfo
                    pinfo = None
                    continue
            else:
                print("%s - %s (unknown property)" % (line, filename))

    # FIXME?
    tmp = gDiscoveryProcess
    gDiscoveryProcess = None
    del gDiscoveryProcess, tmp

    return plugins

def killDiscovery():
    global gDiscoveryProcess

    if gDiscoveryProcess is not None:
        gDiscoveryProcess.kill()

def checkPluginCached(desc, ptype):
    plugins = []

    pinfo = deepcopy(PyPluginInfo)
    pinfo['build'] = BINARY_NATIVE
    pinfo['type']  = ptype
    pinfo['hints'] = desc['hints']
    pinfo['name']  = desc['name']
    pinfo['label'] = desc['label']
    pinfo['maker'] = desc['maker']

    pinfo['audio.ins']  = desc['audioIns']
    pinfo['audio.outs'] = desc['audioOuts']

    pinfo['midi.ins']   = desc['midiIns']
    pinfo['midi.outs']  = desc['midiOuts']

    pinfo['parameters.ins']  = desc['parameterIns']
    pinfo['parameters.outs'] = desc['parameterOuts']

    plugins.append(pinfo)

    return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginLV2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, isWine)

def checkPluginVST2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST2, "VST2", filename, tool, isWine)

def checkPluginVST3(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST3, "VST3", filename, tool, isWine)

def checkPluginAU(filename, tool):
    return runCarlaDiscovery(PLUGIN_AU, "AU", filename, tool)

def checkFileGIG(filename, tool):
    return runCarlaDiscovery(PLUGIN_GIG, "GIG", filename, tool)

def checkFileSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkFileSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

# ------------------------------------------------------------------------------------------------------------
# Separate Thread for Plugin Search

class SearchPluginsThread(QThread):
    pluginLook = pyqtSignal(int, str)

    def __init__(self, parent, pathBinaries):
        QThread.__init__(self, parent)

        self.fContinueChecking = False
        self.fPathBinaries     = pathBinaries

        self.fCheckNative  = False
        self.fCheckPosix32 = False
        self.fCheckPosix64 = False
        self.fCheckWin32   = False
        self.fCheckWin64   = False

        self.fCheckLADSPA = False
        self.fCheckDSSI   = False
        self.fCheckVST2   = False
        self.fCheckVST3   = False
        self.fCheckGIG    = False
        self.fCheckSF2    = False
        self.fCheckSFZ    = False

        if WINDOWS:
            toolNative = "carla-discovery-win64.exe" if kIs64bit else "carla-discovery-win32.exe"
        else:
            toolNative = "carla-discovery-native"

        self.fToolNative = os.path.join(pathBinaries, toolNative)

        if not os.path.exists(self.fToolNative):
            self.fToolNative = ""

        self.fCurCount = 0
        self.fCurPercentValue = 0
        self.fLastCheckValue  = 0
        self.fSomethingChanged = False

        self.fLadspaPlugins = []
        self.fDssiPlugins   = []
        self.fVst2Plugins   = []
        self.fVst3Plugins   = []
        self.fKitPlugins    = []

        # -------------------------------------------------------------

    def hasSomethingChanged(self):
        return self.fSomethingChanged

    def setSearchBinaryTypes(self, native, posix32, posix64, win32, win64):
        self.fCheckNative  = native
        self.fCheckPosix32 = posix32
        self.fCheckPosix64 = posix64
        self.fCheckWin32   = win32
        self.fCheckWin64   = win64

    def setSearchPluginTypes(self, ladspa, dssi, vst2, vst3, gig, sf2, sfz):
        self.fCheckLADSPA = ladspa
        self.fCheckDSSI   = dssi
        self.fCheckVST2   = vst2
        self.fCheckVST3   = vst3
        self.fCheckGIG    = gig
        self.fCheckSF2    = sf2
        self.fCheckSFZ    = sfz

    def stop(self):
        self.fContinueChecking = False

    def run(self):
        pluginCount = 0
        settingsDB  = QSettings("falkTX", "CarlaPlugins2")

        self.fContinueChecking = True
        self.fCurCount = 0

        if self.fCheckLADSPA: pluginCount += 1
        if self.fCheckDSSI:   pluginCount += 1
        if self.fCheckVST2:   pluginCount += 1
        if self.fCheckVST3:   pluginCount += 1

        if self.fCheckNative:
            self.fCurCount += pluginCount

        if self.fCheckPosix32:
            self.fCurCount += pluginCount
            if self.fCheckVST3 and not MACOS:
                self.fCurCount -= 1

        if self.fCheckPosix64:
            self.fCurCount += pluginCount
            if self.fCheckVST3 and not MACOS:
                self.fCurCount -= 1

        if self.fCheckWin32:
            self.fCurCount += pluginCount

        if self.fCheckWin64:
            self.fCurCount += pluginCount

        if self.fCheckNative and self.fToolNative:
            if self.fCheckGIG: self.fCurCount += 1
            if self.fCheckSF2: self.fCurCount += 1
            if self.fCheckSFZ: self.fCurCount += 1
        else:
            self.fCheckGIG = False
            self.fCheckSF2 = False
            self.fCheckSFZ = False

        if self.fCurCount == 0:
            return

        self.fCurPercentValue = 100.0 / self.fCurCount
        self.fLastCheckValue  = 0.0

        if HAIKU:
            OS = "HAIKU"
        elif LINUX:
            OS = "LINUX"
        elif MACOS:
            OS = "MACOS"
        elif WINDOWS:
            OS = "WINDOWS"
        else:
            OS = "UNKNOWN"

        if not self.fContinueChecking: return

        if self.fCheckLADSPA:
            checkValue = 0.0
            if haveLRDF:
                if self.fCheckNative:  checkValue += 0.1
                if self.fCheckPosix32: checkValue += 0.1
                if self.fCheckPosix64: checkValue += 0.1
                if self.fCheckWin32:   checkValue += 0.1
                if self.fCheckWin64:   checkValue += 0.1
            rdfPadValue = self.fCurPercentValue * checkValue

            if self.fCheckNative:
                self._checkLADSPA(OS, self.fToolNative)
                settingsDB.setValue("Plugins/LADSPA_native", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkLADSPA(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/LADSPA_posix32", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkLADSPA(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/LADSPA_posix64", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkLADSPA("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win32.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win32", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkLADSPA("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win64.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win64", self.fLadspaPlugins)

            settingsDB.sync()

            if not self.fContinueChecking: return

            if haveLRDF and checkValue > 0:
                startValue = self.fLastCheckValue - rdfPadValue

                self._pluginLook(startValue, "LADSPA RDFs...")
                try:
                    ladspaRdfInfo = ladspa_rdf.recheck_all_plugins(self, startValue, self.fCurPercentValue, checkValue)
                except:
                    ladspaRdfInfo = None

                if ladspaRdfInfo is not None:
                    settingsDir = os.path.join(HOME, ".config", "falkTX")
                    fdLadspa    = open(os.path.join(settingsDir, "ladspa_rdf.db"), 'w')
                    json.dump(ladspaRdfInfo, fdLadspa)
                    fdLadspa.close()

            if not self.fContinueChecking: return

        if self.fCheckDSSI:
            if self.fCheckNative:
                self._checkDSSI(OS, self.fToolNative)
                settingsDB.setValue("Plugins/DSSI_native", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkDSSI(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/DSSI_posix32", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkDSSI(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/DSSI_posix64", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkDSSI("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win32.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win32", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkDSSI("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win64.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win64", self.fDssiPlugins)

            settingsDB.sync()

            if not self.fContinueChecking: return

        if self.fCheckVST2:
            if self.fCheckNative:
                self._checkVST2(OS, self.fToolNative)
                settingsDB.setValue("Plugins/VST2_native", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkVST2(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/VST2_posix32", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkVST2(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/VST2_posix64", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkVST2("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win32.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/VST2_win32", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkVST2("WINDOWS", os.path.join(self.fPathBinaries, "carla-discovery-win64.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/VST2_win64", self.fVstPlugins)

            settingsDB.sync()

            if not self.fContinueChecking: return

        if self.fCheckVST3:
            if self.fCheckNative:
                self._checkVST3(self.fToolNative)
                settingsDB.setValue("Plugins/VST3_native", self.fVst3Plugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32 and MACOS:
                self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/VST3_posix32", self.fVst3Plugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64 and MACOS:
                self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/VST3_posix64", self.fVst3Plugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-win32.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/VST3_win32", self.fVst3Plugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-win64.exe"), not WINDOWS)
                settingsDB.setValue("Plugins/VST3_win64", self.fVst3Plugins)

            settingsDB.sync()

            if not self.fContinueChecking: return

        if self.fCheckGIG:
            settings = QSettings("falkTX", "Carla2")
            GIG_PATH = toList(settings.value(CARLA_KEY_PATHS_GIG, CARLA_DEFAULT_GIG_PATH))
            del settings

            self._checkKIT(GIG_PATH, "gig")
            settingsDB.setValue("Plugins/GIG", self.fKitPlugins)

            if not self.fContinueChecking: return

        if self.fCheckSF2:
            settings = QSettings("falkTX", "Carla2")
            SF2_PATH = toList(settings.value(CARLA_KEY_PATHS_SF2, CARLA_DEFAULT_SF2_PATH))
            del settings

            self._checkKIT(SF2_PATH, "sf2")
            settingsDB.setValue("Plugins/SF2", self.fKitPlugins)

            if not self.fContinueChecking: return

        if self.fCheckSFZ:
            settings = QSettings("falkTX", "Carla2")
            SFZ_PATH = toList(settings.value(CARLA_KEY_PATHS_SFZ, CARLA_DEFAULT_SFZ_PATH))
            del settings

            self._checkKIT(SFZ_PATH, "sfz")
            settingsDB.setValue("Plugins/SFZ", self.fKitPlugins)

        settingsDB.sync()

    def _checkLADSPA(self, OS, tool, isWine=False):
        ladspaBinaries = []
        self.fLadspaPlugins = []

        self._pluginLook(self.fLastCheckValue, "LADSPA plugins...")

        settings = QSettings("falkTX", "Carla2")
        LADSPA_PATH = toList(settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH))
        del settings

        for iPATH in LADSPA_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in ladspaBinaries:
                    ladspaBinaries.append(binary)

        ladspaBinaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(ladspaBinaries)):
            ladspa  = ladspaBinaries[i]
            percent = ( float(i) / len(ladspaBinaries) ) * self.fCurPercentValue
            self._pluginLook((self.fLastCheckValue + percent) * 0.9, ladspa)

            plugins = checkPluginLADSPA(ladspa, tool, isWine)
            if plugins:
                self.fLadspaPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkDSSI(self, OS, tool, isWine=False):
        dssiBinaries = []
        self.fDssiPlugins = []

        self._pluginLook(self.fLastCheckValue, "DSSI plugins...")

        settings = QSettings("falkTX", "Carla2")
        DSSI_PATH = toList(settings.value(CARLA_KEY_PATHS_DSSI, CARLA_DEFAULT_DSSI_PATH))
        del settings

        for iPATH in DSSI_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in dssiBinaries:
                    dssiBinaries.append(binary)

        dssiBinaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(dssiBinaries)):
            dssi    = dssiBinaries[i]
            percent = ( float(i) / len(dssiBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, dssi)

            plugins = checkPluginDSSI(dssi, tool, isWine)
            if plugins:
                self.fDssiPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkVST2(self, OS, tool, isWine=False):
        vst2Binaries = []
        self.fVstPlugins = []

        if MACOS and not isWine:
            self._pluginLook(self.fLastCheckValue, "VST2 bundles...")
        else:
            self._pluginLook(self.fLastCheckValue, "VST2 plugins...")

        settings = QSettings("falkTX", "Carla2")
        VST2_PATH = toList(settings.value(CARLA_KEY_PATHS_VST2, CARLA_DEFAULT_VST2_PATH))
        del settings

        for iPATH in VST2_PATH:
            if MACOS and not isWine:
                binaries = findMacVSTBundles(iPATH, False)
            else:
                binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in vst2Binaries:
                    vst2Binaries.append(binary)

        vst2Binaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(vst2Binaries)):
            vst2    = vst2Binaries[i]
            percent = ( float(i) / len(vst2Binaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst2)

            plugins = checkPluginVST2(vst2, tool, isWine)
            if plugins:
                self.fVstPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkVST3(self, tool, isWine=False):
        vst3Binaries = []
        self.fVst3Plugins = []

        if MACOS and not isWine:
            self._pluginLook(self.fLastCheckValue, "VST3 bundles...")
        else:
            self._pluginLook(self.fLastCheckValue, "VST3 plugins...")

        settings = QSettings("falkTX", "Carla2")
        VST3_PATH = toList(settings.value(CARLA_KEY_PATHS_VST3, CARLA_DEFAULT_VST3_PATH))
        del settings

        for iPATH in VST3_PATH:
            if MACOS and not isWine:
                binaries = findMacVSTBundles(iPATH, True)
            else:
                binaries = findVST3Binaries(iPATH)
            for binary in binaries:
                if binary not in vst3Binaries:
                    vst3Binaries.append(binary)

        vst3Binaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(vst3Binaries)):
            vst3    = vst3Binaries[i]
            percent = ( float(i) / len(vst3Binaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst3)

            plugins = checkPluginVST3(vst3, tool, isWine)
            if plugins:
                self.fVst3Plugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkKIT(self, kitPATH, kitExtension):
        kitFiles = []
        self.fKitPlugins = []

        for iPATH in kitPATH:
            files = findFilenames(iPATH, kitExtension)
            for file_ in files:
                if file_ not in kitFiles:
                    kitFiles.append(file_)

        kitFiles.sort()

        if not self.fContinueChecking: return

        for i in range(len(kitFiles)):
            kit     = kitFiles[i]
            percent = ( float(i) / len(kitFiles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, kit)

            if kitExtension == "gig":
                plugins = checkFileGIG(kit, self.fToolNative)
            elif kitExtension == "sf2":
                plugins = checkFileSF2(kit, self.fToolNative)
            elif kitExtension == "sfz":
                plugins = checkFileSFZ(kit, self.fToolNative)
            else:
                plugins = None

            if plugins:
                self.fKitPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _pluginLook(self, percent, plugin):
        self.pluginLook.emit(percent, plugin)

# ------------------------------------------------------------------------------------------------------------
# Plugin Refresh Dialog

class PluginRefreshW(QDialog):
    def __init__(self, parent, host):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_refresh.Ui_PluginRefreshW()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        hasNative  = os.path.exists(os.path.join(self.host.pathBinaries, "carla-discovery-native"))
        hasPosix32 = os.path.exists(os.path.join(self.host.pathBinaries, "carla-discovery-posix32"))
        hasPosix64 = os.path.exists(os.path.join(self.host.pathBinaries, "carla-discovery-posix64"))
        hasWin32   = os.path.exists(os.path.join(self.host.pathBinaries, "carla-discovery-win32.exe"))
        hasWin64   = os.path.exists(os.path.join(self.host.pathBinaries, "carla-discovery-win64.exe"))

        self.fThread  = SearchPluginsThread(self, host.pathBinaries)
        self.fIconYes = getIcon("dialog-ok-apply").pixmap(16, 16)
        self.fIconNo  = getIcon("dialog-error").pixmap(16, 16)

        # ----------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.ui.b_skip.setVisible(False)

        if HAIKU:
            self.ui.ch_posix32.setText("Haiku 32bit")
            self.ui.ch_posix64.setText("Haiku 64bit")
        elif LINUX:
            self.ui.ch_posix32.setText("Linux 32bit")
            self.ui.ch_posix64.setText("Linux 64bit")
        elif MACOS:
            self.ui.ch_posix32.setText("MacOS 32bit")
            self.ui.ch_posix64.setText("MacOS 64bit")

        if hasPosix32 and not WINDOWS:
            self.ui.ico_posix32.setPixmap(self.fIconYes)
        else:
            self.ui.ico_posix32.setPixmap(self.fIconNo)
            self.ui.ch_posix32.setEnabled(False)

        if hasPosix64 and not WINDOWS:
            self.ui.ico_posix64.setPixmap(self.fIconYes)
        else:
            self.ui.ico_posix64.setPixmap(self.fIconNo)
            self.ui.ch_posix64.setEnabled(False)

        if hasWin32:
            self.ui.ico_win32.setPixmap(self.fIconYes)
        else:
            self.ui.ico_win32.setPixmap(self.fIconNo)
            self.ui.ch_win32.setEnabled(False)

        if hasWin64:
            self.ui.ico_win64.setPixmap(self.fIconYes)
        else:
            self.ui.ico_win64.setPixmap(self.fIconNo)
            self.ui.ch_win64.setEnabled(False)

        if haveLRDF:
            self.ui.ico_rdflib.setPixmap(self.fIconYes)
        else:
            self.ui.ico_rdflib.setPixmap(self.fIconNo)

        if WINDOWS:
            if kIs64bit:
                hasNative = hasWin64
                hasNonNative = hasWin32
                self.ui.ch_win64.setEnabled(False)
                self.ui.ch_win64.setVisible(False)
                self.ui.ico_win64.setVisible(False)
                self.ui.label_win64.setVisible(False)
            else:
                hasNative = hasWin32
                hasNonNative = hasWin64
                self.ui.ch_win32.setEnabled(False)
                self.ui.ch_win32.setVisible(False)
                self.ui.ico_win32.setVisible(False)
                self.ui.label_win32.setVisible(False)
        else:
            if kIs64bit:
                hasNonNative = bool(hasPosix32 or hasWin32 or hasWin64)
                self.ui.ch_posix64.setEnabled(False)
                self.ui.ch_posix64.setVisible(False)
                self.ui.ico_posix64.setVisible(False)
                self.ui.label_posix64.setVisible(False)
            else:
                hasNonNative = bool(hasPosix64 or hasWin32 or hasWin64)
                self.ui.ch_posix32.setEnabled(False)
                self.ui.ch_posix32.setVisible(False)
                self.ui.ico_posix32.setVisible(False)
                self.ui.label_posix32.setVisible(False)

        if hasNative:
            self.ui.ico_native.setPixmap(self.fIconYes)
        else:
            self.ui.ico_native.setPixmap(self.fIconNo)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_gig.setEnabled(False)
            self.ui.ch_sf2.setEnabled(False)
            self.ui.ch_sfz.setEnabled(False)
            if not hasNonNative:
                self.ui.ch_ladspa.setEnabled(False)
                self.ui.ch_dssi.setEnabled(False)
                self.ui.ch_vst.setEnabled(False)
                self.ui.ch_vst3.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # ----------------------------------------------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_saveSettings)
        self.ui.b_start.clicked.connect(self.slot_start)
        self.ui.b_skip.clicked.connect(self.slot_skip)
        self.ui.ch_native.clicked.connect(self.slot_checkTools)
        self.ui.ch_posix32.clicked.connect(self.slot_checkTools)
        self.ui.ch_posix64.clicked.connect(self.slot_checkTools)
        self.ui.ch_win32.clicked.connect(self.slot_checkTools)
        self.ui.ch_win64.clicked.connect(self.slot_checkTools)
        self.ui.ch_ladspa.clicked.connect(self.slot_checkTools)
        self.ui.ch_dssi.clicked.connect(self.slot_checkTools)
        self.ui.ch_vst.clicked.connect(self.slot_checkTools)
        self.ui.ch_vst3.clicked.connect(self.slot_checkTools)
        self.ui.ch_gig.clicked.connect(self.slot_checkTools)
        self.ui.ch_sf2.clicked.connect(self.slot_checkTools)
        self.ui.ch_sfz.clicked.connect(self.slot_checkTools)
        self.fThread.pluginLook.connect(self.slot_handlePluginLook)
        self.fThread.finished.connect(self.slot_handlePluginThreadFinished)

        # ----------------------------------------------------------------------------------------------------
        # Post-connect setup

        self.slot_checkTools()

    # --------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSettings("falkTX", "CarlaRefresh2")
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/SearchLADSPA", True, type=bool) and self.ui.ch_ladspa.isEnabled())
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/SearchDSSI", True, type=bool) and self.ui.ch_dssi.isEnabled())
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/SearchVST2", True, type=bool) and self.ui.ch_vst.isEnabled())
        self.ui.ch_vst3.setChecked(settings.value("PluginDatabase/SearchVST3", (MACOS or WINDOWS), type=bool) and self.ui.ch_vst3.isEnabled())
        self.ui.ch_gig.setChecked(settings.value("PluginDatabase/SearchGIG", False, type=bool) and self.ui.ch_gig.isEnabled())
        self.ui.ch_sf2.setChecked(settings.value("PluginDatabase/SearchSF2", False, type=bool) and self.ui.ch_sf2.isEnabled())
        self.ui.ch_sfz.setChecked(settings.value("PluginDatabase/SearchSFZ", False, type=bool) and self.ui.ch_sfz.isEnabled())
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/SearchNative", True, type=bool) and self.ui.ch_native.isEnabled())
        self.ui.ch_posix32.setChecked(settings.value("PluginDatabase/SearchPOSIX32", False, type=bool) and self.ui.ch_posix32.isEnabled())
        self.ui.ch_posix64.setChecked(settings.value("PluginDatabase/SearchPOSIX64", False, type=bool) and self.ui.ch_posix64.isEnabled())
        self.ui.ch_win32.setChecked(settings.value("PluginDatabase/SearchWin32", False, type=bool) and self.ui.ch_win32.isEnabled())
        self.ui.ch_win64.setChecked(settings.value("PluginDatabase/SearchWin64", False, type=bool) and self.ui.ch_win64.isEnabled())
        self.ui.ch_do_checks.setChecked(settings.value("PluginDatabase/DoChecks", False, type=bool))

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings("falkTX", "CarlaRefresh2")
        settings.setValue("PluginDatabase/SearchLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/SearchDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/SearchVST2", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/SearchVST3", self.ui.ch_vst3.isChecked())
        settings.setValue("PluginDatabase/SearchGIG", self.ui.ch_gig.isChecked())
        settings.setValue("PluginDatabase/SearchSF2", self.ui.ch_sf2.isChecked())
        settings.setValue("PluginDatabase/SearchSFZ", self.ui.ch_sfz.isChecked())
        settings.setValue("PluginDatabase/SearchNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX32", self.ui.ch_posix32.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX64", self.ui.ch_posix64.isChecked())
        settings.setValue("PluginDatabase/SearchWin32", self.ui.ch_win32.isChecked())
        settings.setValue("PluginDatabase/SearchWin64", self.ui.ch_win64.isChecked())
        settings.setValue("PluginDatabase/DoChecks", self.ui.ch_do_checks.isChecked())

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_start(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(100)
        self.ui.progressBar.setValue(0)
        self.ui.b_start.setEnabled(False)
        self.ui.b_skip.setVisible(True)
        self.ui.b_close.setVisible(False)
        self.ui.group_types.setEnabled(False)
        self.ui.group_options.setEnabled(False)

        if self.ui.ch_do_checks.isChecked():
            gCarla.utils.unsetenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")
        else:
            gCarla.utils.setenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS", "true")

        native, posix32, posix64, win32, win64 = (self.ui.ch_native.isChecked(),
                                                  self.ui.ch_posix32.isChecked(), self.ui.ch_posix64.isChecked(),
                                                  self.ui.ch_win32.isChecked(), self.ui.ch_win64.isChecked())

        ladspa, dssi, vst, vst3, gig, sf2, sfz = (self.ui.ch_ladspa.isChecked(), self.ui.ch_dssi.isChecked(),
                                                  self.ui.ch_vst.isChecked(), self.ui.ch_vst3.isChecked(),
                                                  self.ui.ch_gig.isChecked(), self.ui.ch_sf2.isChecked(), self.ui.ch_sfz.isChecked())

        self.fThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
        self.fThread.setSearchPluginTypes(ladspa, dssi, vst, vst3, gig, sf2, sfz)
        self.fThread.start()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_skip(self):
        killDiscovery()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_checkTools(self):
        enabled1 = bool(self.ui.ch_native.isChecked() or self.ui.ch_posix32.isChecked() or self.ui.ch_posix64.isChecked() or self.ui.ch_win32.isChecked() or self.ui.ch_win64.isChecked())
        enabled2 = bool(self.ui.ch_ladspa.isChecked() or self.ui.ch_dssi.isChecked() or self.ui.ch_vst.isChecked() or self.ui.ch_vst3.isChecked() or
                        self.ui.ch_gig.isChecked() or self.ui.ch_sf2.isChecked() or self.ui.ch_sfz.isChecked())
        self.ui.b_start.setEnabled(enabled1 and enabled2)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.ui.progressBar.setFormat("%s" % plugin)
        self.ui.progressBar.setValue(percent)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_handlePluginThreadFinished(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(1)
        self.ui.progressBar.setValue(1)
        self.ui.progressBar.setFormat(self.tr("Done"))
        self.ui.b_start.setEnabled(True)
        self.ui.b_skip.setVisible(False)
        self.ui.b_close.setVisible(True)
        self.ui.group_types.setEnabled(True)
        self.ui.group_options.setEnabled(True)

    # --------------------------------------------------------------------------------------------------------

    def closeEvent(self, event):
        if self.fThread.isRunning():
            self.fThread.stop()
            killDiscovery()
            #self.fThread.terminate()
            self.fThread.wait()

        if self.fThread.hasSomethingChanged():
            self.accept()
        else:
            self.reject()

        QDialog.closeEvent(self, event)

    # --------------------------------------------------------------------------------------------------------

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Database Dialog

class PluginDatabaseW(QDialog):
    def __init__(self, parent, host):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_database.Ui_PluginDatabaseW()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fLastTableIndex = 0
        self.fRetPlugin  = None
        self.fRealParent = parent

        # ----------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.ui.b_add.setEnabled(False)

        if BINARY_NATIVE in (BINARY_POSIX32, BINARY_WIN32):
            self.ui.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ui.ch_bridged.setText(self.tr("Bridged (32bit)"))

        if not (LINUX or MACOS):
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # ----------------------------------------------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_saveSettings)
        self.ui.b_add.clicked.connect(self.slot_addPlugin)
        self.ui.b_refresh.clicked.connect(self.slot_refreshPlugins)
        self.ui.tb_filters.clicked.connect(self.slot_maybeShowFilters)
        self.ui.lineEdit.textChanged.connect(self.slot_checkFilters)
        self.ui.tableWidget.currentCellChanged.connect(self.slot_checkPlugin)
        self.ui.tableWidget.cellDoubleClicked.connect(self.slot_addPlugin)

        self.ui.ch_effects.clicked.connect(self.slot_checkFilters)
        self.ui.ch_instruments.clicked.connect(self.slot_checkFilters)
        self.ui.ch_midi.clicked.connect(self.slot_checkFilters)
        self.ui.ch_other.clicked.connect(self.slot_checkFilters)
        self.ui.ch_internal.clicked.connect(self.slot_checkFilters)
        self.ui.ch_ladspa.clicked.connect(self.slot_checkFilters)
        self.ui.ch_dssi.clicked.connect(self.slot_checkFilters)
        self.ui.ch_lv2.clicked.connect(self.slot_checkFilters)
        self.ui.ch_vst.clicked.connect(self.slot_checkFilters)
        self.ui.ch_vst3.clicked.connect(self.slot_checkFilters)
        self.ui.ch_au.clicked.connect(self.slot_checkFilters)
        self.ui.ch_kits.clicked.connect(self.slot_checkFilters)
        self.ui.ch_native.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged_wine.clicked.connect(self.slot_checkFilters)
        self.ui.ch_rtsafe.clicked.connect(self.slot_checkFilters)
        self.ui.ch_gui.clicked.connect(self.slot_checkFilters)
        self.ui.ch_stereo.clicked.connect(self.slot_checkFilters)

        # ----------------------------------------------------------------------------------------------------
        # Post-connect setup

        self._reAddPlugins()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_addPlugin(self):
        if self.ui.tableWidget.currentRow() >= 0:
            self.fRetPlugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(), 0).data(Qt.UserRole)
            self.accept()
        else:
            self.reject()

    @pyqtSlot(int)
    def slot_checkPlugin(self, row):
        self.ui.b_add.setEnabled(row >= 0)

    @pyqtSlot()
    def slot_checkFilters(self):
        self._checkFilters()

    @pyqtSlot()
    def slot_maybeShowFilters(self):
        self._showFilters(not self.ui.frame.isVisible())

    @pyqtSlot()
    def slot_refreshPlugins(self):
        if PluginRefreshW(self, self.host).exec_():
            self._reAddPlugins()

            if self.fRealParent:
                self.fRealParent.setLoadRDFsNeeded()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings("falkTX", "CarlaDatabase2")
        settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        settings.setValue("PluginDatabase/TableGeometry%s" % ("_5" if config_UseQt5 else "_4"), self.ui.tableWidget.horizontalHeader().saveState())
        settings.setValue("PluginDatabase/ShowFilters", (self.ui.tb_filters.arrowType() == Qt.UpArrow))
        settings.setValue("PluginDatabase/ShowEffects", self.ui.ch_effects.isChecked())
        settings.setValue("PluginDatabase/ShowInstruments", self.ui.ch_instruments.isChecked())
        settings.setValue("PluginDatabase/ShowMIDI", self.ui.ch_midi.isChecked())
        settings.setValue("PluginDatabase/ShowOther", self.ui.ch_other.isChecked())
        settings.setValue("PluginDatabase/ShowInternal", self.ui.ch_internal.isChecked())
        settings.setValue("PluginDatabase/ShowLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/ShowDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/ShowLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/ShowVST2", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/ShowVST3", self.ui.ch_vst3.isChecked())
        settings.setValue("PluginDatabase/ShowAU", self.ui.ch_au.isChecked())
        settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits.isChecked())
        settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged.isChecked())
        settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine.isChecked())
        settings.setValue("PluginDatabase/ShowRtSafe", self.ui.ch_rtsafe.isChecked())
        settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui.isChecked())
        settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo.isChecked())
        settings.setValue("PluginDatabase/SearchText", self.ui.lineEdit.text())

    # --------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSettings("falkTX", "CarlaDatabase2")
        self.restoreGeometry(settings.value("PluginDatabase/Geometry", ""))
        self.ui.tableWidget.horizontalHeader().restoreState(settings.value("PluginDatabase/TableGeometry%s" % ("_5" if config_UseQt5 else "_4"), ""))
        self.ui.ch_effects.setChecked(settings.value("PluginDatabase/ShowEffects", True, type=bool))
        self.ui.ch_instruments.setChecked(settings.value("PluginDatabase/ShowInstruments", True, type=bool))
        self.ui.ch_midi.setChecked(settings.value("PluginDatabase/ShowMIDI", True, type=bool))
        self.ui.ch_other.setChecked(settings.value("PluginDatabase/ShowOther", True, type=bool))
        self.ui.ch_internal.setChecked(settings.value("PluginDatabase/ShowInternal", True, type=bool))
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/ShowLADSPA", True, type=bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/ShowDSSI", True, type=bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/ShowLV2", True, type=bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/ShowVST2", True, type=bool))
        self.ui.ch_vst3.setChecked(settings.value("PluginDatabase/ShowVST3", (MACOS or WINDOWS), type=bool))
        self.ui.ch_au.setChecked(settings.value("PluginDatabase/ShowAU", True, type=bool))
        self.ui.ch_kits.setChecked(settings.value("PluginDatabase/ShowKits", True, type=bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/ShowNative", True, type=bool))
        self.ui.ch_bridged.setChecked(settings.value("PluginDatabase/ShowBridged", True, type=bool))
        self.ui.ch_bridged_wine.setChecked(settings.value("PluginDatabase/ShowBridgedWine", True, type=bool))
        self.ui.ch_rtsafe.setChecked(settings.value("PluginDatabase/ShowRtSafe", False, type=bool))
        self.ui.ch_gui.setChecked(settings.value("PluginDatabase/ShowHasGUI", False, type=bool))
        self.ui.ch_stereo.setChecked(settings.value("PluginDatabase/ShowStereoOnly", False, type=bool))
        self.ui.lineEdit.setText(settings.value("PluginDatabase/SearchText", "", type=str))

        self._showFilters(settings.value("PluginDatabase/ShowFilters", False, type=bool))

    # --------------------------------------------------------------------------------------------------------

    def _checkFilters(self):
        text = self.ui.lineEdit.text().lower()

        hideEffects     = not self.ui.ch_effects.isChecked()
        hideInstruments = not self.ui.ch_instruments.isChecked()
        hideMidi        = not self.ui.ch_midi.isChecked()
        hideOther       = not self.ui.ch_other.isChecked()

        hideInternal = not self.ui.ch_internal.isChecked()
        hideLadspa   = not self.ui.ch_ladspa.isChecked()
        hideDssi     = not self.ui.ch_dssi.isChecked()
        hideLV2      = not self.ui.ch_lv2.isChecked()
        hideVST2     = not self.ui.ch_vst.isChecked()
        hideVST3     = not self.ui.ch_vst3.isChecked()
        hideAU       = not self.ui.ch_au.isChecked()
        hideKits     = not self.ui.ch_kits.isChecked()

        hideNative  = not self.ui.ch_native.isChecked()
        hideBridged = not self.ui.ch_bridged.isChecked()
        hideBridgedWine = not self.ui.ch_bridged_wine.isChecked()

        hideNonRtSafe = self.ui.ch_rtsafe.isChecked()
        hideNonGui    = self.ui.ch_gui.isChecked()
        hideNonStereo = self.ui.ch_stereo.isChecked()

        if HAIKU or LINUX or MACOS:
            nativeBins = [BINARY_POSIX32, BINARY_POSIX64]
            wineBins   = [BINARY_WIN32, BINARY_WIN64]
        elif WINDOWS:
            nativeBins = [BINARY_WIN32, BINARY_WIN64]
            wineBins   = []
        else:
            nativeBins = []
            wineBins   = []

        rowCount = self.ui.tableWidget.rowCount()

        for i in range(rowCount):
            self.ui.tableWidget.showRow(i)

            plugin = self.ui.tableWidget.item(i, 0).data(Qt.UserRole)
            aIns   = plugin['audio.ins']
            aOuts  = plugin['audio.outs']
            mIns   = plugin['midi.ins']
            mOuts  = plugin['midi.outs']
            ptype  = self.ui.tableWidget.item(i, 12).text()
            isSynth  = bool(plugin['hints'] & PLUGIN_IS_SYNTH)
            isEffect = bool(aIns > 0 < aOuts and not isSynth)
            isMidi   = bool(aIns == 0 and aOuts == 0 and mIns > 0 < mOuts)
            isKit    = bool(ptype in ("GIG", "SF2", "SFZ"))
            isOther  = bool(not (isEffect or isSynth or isMidi or isKit))
            isNative = bool(plugin['build'] == BINARY_NATIVE)
            isRtSafe = bool(plugin['hints'] & PLUGIN_IS_RTSAFE)
            isStereo = bool(aIns == 2 and aOuts == 2) or (isSynth and aOuts == 2)
            hasGui   = bool(plugin['hints'] & PLUGIN_HAS_CUSTOM_UI)

            isBridged = bool(not isNative and plugin['build'] in nativeBins)
            isBridgedWine = bool(not isNative and plugin['build'] in wineBins)

            if hideEffects and isEffect:
                self.ui.tableWidget.hideRow(i)
            elif hideInstruments and isSynth:
                self.ui.tableWidget.hideRow(i)
            elif hideMidi and isMidi:
                self.ui.tableWidget.hideRow(i)
            elif hideOther and isOther:
                self.ui.tableWidget.hideRow(i)
            elif hideKits and isKit:
                self.ui.tableWidget.hideRow(i)
            elif hideInternal and ptype == self.tr("Internal"):
                self.ui.tableWidget.hideRow(i)
            elif hideLadspa and ptype == "LADSPA":
                self.ui.tableWidget.hideRow(i)
            elif hideDssi and ptype == "DSSI":
                self.ui.tableWidget.hideRow(i)
            elif hideLV2 and ptype == "LV2":
                self.ui.tableWidget.hideRow(i)
            elif hideVST2 and ptype == "VST2":
                self.ui.tableWidget.hideRow(i)
            elif hideVST3 and ptype == "VST3":
                self.ui.tableWidget.hideRow(i)
            elif hideAU and ptype == "AU":
                self.ui.tableWidget.hideRow(i)
            elif hideNative and isNative:
                self.ui.tableWidget.hideRow(i)
            elif hideBridged and isBridged:
                self.ui.tableWidget.hideRow(i)
            elif hideBridgedWine and isBridgedWine:
                self.ui.tableWidget.hideRow(i)
            elif hideNonRtSafe and not isRtSafe:
                self.ui.tableWidget.hideRow(i)
            elif hideNonGui and not hasGui:
                self.ui.tableWidget.hideRow(i)
            elif hideNonStereo and not isStereo:
                self.ui.tableWidget.hideRow(i)
            elif (text and not (
                  text in self.ui.tableWidget.item(i, 0).text().lower() or
                  text in self.ui.tableWidget.item(i, 1).text().lower() or
                  text in self.ui.tableWidget.item(i, 2).text().lower() or
                  text in self.ui.tableWidget.item(i, 3).text().lower() or
                  text in self.ui.tableWidget.item(i, 13).text().lower())):
                self.ui.tableWidget.hideRow(i)

    # --------------------------------------------------------------------------------------------------------

    def _showFilters(self, yesNo):
        self.ui.tb_filters.setArrowType(Qt.UpArrow if yesNo else Qt.DownArrow)
        self.ui.frame.setVisible(yesNo)

    # --------------------------------------------------------------------------------------------------------

    def _addPluginToTable(self, plugin, ptype):
        if plugin['API'] != PLUGIN_QUERY_API_VERSION and ptype == self.tr("Internal"):
            return

        if ptype in (self.tr("Internal"), "LV2", "AU", "GIG", "SF2", "SFZ"):
            plugin['build'] = BINARY_NATIVE

        index = self.fLastTableIndex

        if plugin['build'] == BINARY_NATIVE:
            bridgeText = self.tr("No")

        else:
            if LINUX or MACOS:
                if plugin['build'] == BINARY_WIN32:
                    typeText = "32bit"
                elif plugin['build'] == BINARY_WIN64:
                    typeText = "64bit"
                else:
                    typeText = self.tr("Unknown")
            else:
                if plugin['build'] == BINARY_POSIX32:
                    typeText = "32bit"
                elif plugin['build'] == BINARY_POSIX64:
                    typeText = "64bit"
                elif plugin['build'] == BINARY_WIN32:
                    typeText = "Windows 32bit"
                elif plugin['build'] == BINARY_WIN64:
                    typeText = "Windows 64bit"
                else:
                    typeText = self.tr("Unknown")

            bridgeText = self.tr("Yes (%s)" % typeText)

        self.ui.tableWidget.insertRow(index)
        self.ui.tableWidget.setItem(index, 0, QTableWidgetItem(str(plugin['name'])))
        self.ui.tableWidget.setItem(index, 1, QTableWidgetItem(str(plugin['label'])))
        self.ui.tableWidget.setItem(index, 2, QTableWidgetItem(str(plugin['maker'])))
        self.ui.tableWidget.setItem(index, 3, QTableWidgetItem(str(plugin['uniqueId'])))
        self.ui.tableWidget.setItem(index, 4, QTableWidgetItem(str(plugin['audio.ins'])))
        self.ui.tableWidget.setItem(index, 5, QTableWidgetItem(str(plugin['audio.outs'])))
        self.ui.tableWidget.setItem(index, 6, QTableWidgetItem(str(plugin['parameters.ins'])))
        self.ui.tableWidget.setItem(index, 7, QTableWidgetItem(str(plugin['parameters.outs'])))
        self.ui.tableWidget.setItem(index, 9, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_HAS_CUSTOM_UI) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 10, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_IS_SYNTH) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 11, QTableWidgetItem(bridgeText))
        self.ui.tableWidget.setItem(index, 12, QTableWidgetItem(ptype))
        self.ui.tableWidget.setItem(index, 13, QTableWidgetItem(str(plugin['filename'])))
        self.ui.tableWidget.item(index, 0).setData(Qt.UserRole, plugin)

        self.fLastTableIndex += 1

    # --------------------------------------------------------------------------------------------------------

    def _reAddPlugins(self):
        settingsDB = QSettings("falkTX", "CarlaPlugins2")

        for x in range(self.ui.tableWidget.rowCount()):
            self.ui.tableWidget.removeRow(0)

        self.fLastTableIndex = 0
        self.ui.tableWidget.setSortingEnabled(False)

        internalCount = 0
        ladspaCount = 0
        dssiCount   = 0
        lv2Count    = 0
        vstCount    = 0
        vst3Count   = 0
        auCount     = 0
        kitCount    = 0

        settings = QSettings("falkTX", "Carla2")
        LV2_PATH = splitter.join(toList(settings.value(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH)))
        del settings

        # ----------------------------------------------------------------------------------------------------
        # Cached plugins (Internal)

        internalPlugins = toList(settingsDB.value("Plugins/Internal", []))

        for plugins in internalPlugins:
            internalCount += len(plugins)

        internalCountNew = gCarla.utils.get_cached_plugin_count(PLUGIN_INTERNAL, "")

        if internalCountNew != internalCount or (len(internalPlugins) > 0 and
                                                 len(internalPlugins[0]) > 0 and
                                                 internalPlugins[0][0]['API'] != PLUGIN_QUERY_API_VERSION):
            internalCount   = internalCountNew
            internalPlugins = []

            for i in range(internalCountNew):
                descInfo = gCarla.utils.get_cached_plugin_info(PLUGIN_INTERNAL, i)
                plugins  = checkPluginCached(descInfo, PLUGIN_INTERNAL)

                if plugins:
                    internalPlugins.append(plugins)

            settingsDB.setValue("Plugins/Internal", internalPlugins)

        for plugins in internalPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, self.tr("Internal"))

        del internalCountNew
        del internalPlugins

        # ----------------------------------------------------------------------------------------------------
        # Cached plugins (LV2)

        lv2Plugins = toList(settingsDB.value("Plugins/LV2", []))

        for plugins in lv2Plugins:
            lv2Count += len(plugins)

        lv2CountNew = gCarla.utils.get_cached_plugin_count(PLUGIN_LV2, LV2_PATH)

        if lv2CountNew != lv2Count or (len(lv2Plugins) > 0 and
                                       len(lv2Plugins[0]) > 0 and
                                       lv2Plugins[0][0]['API'] != PLUGIN_QUERY_API_VERSION):
            lv2Count   = lv2CountNew
            lv2Plugins = []

            for i in range(lv2CountNew):
                descInfo = gCarla.utils.get_cached_plugin_info(PLUGIN_LV2, i)
                plugins  = checkPluginCached(descInfo, PLUGIN_LV2)

                if plugins:
                    lv2Plugins.append(plugins)

            settingsDB.setValue("Plugins/LV2", lv2Plugins)

        for plugins in lv2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LV2")

        del lv2CountNew
        del lv2Plugins

        # ----------------------------------------------------------------------------------------------------
        # Cached plugins (AU)

        auPlugins = toList(settingsDB.value("Plugins/AU", []))

        for plugins in auPlugins:
            auCount += len(plugins)

        auCountNew = gCarla.utils.get_cached_plugin_count(PLUGIN_AU, "")

        if auCountNew != auCount or (len(auPlugins) > 0 and
                                     len(auPlugins[0]) > 0 and
                                     auPlugins[0][0]['API'] != PLUGIN_QUERY_API_VERSION):
            auCount   = auCountNew
            auPlugins = []

            for i in range(auCountNew):
                descInfo = gCarla.utils.get_cached_plugin_info(PLUGIN_AU, i)
                plugins  = checkPluginCached(descInfo, PLUGIN_AU)

                if plugins:
                    auPlugins.append(plugins)

            settingsDB.setValue("Plugins/AU", auPlugins)

        for plugins in auPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "AU")

        del auCountNew
        del auPlugins

        # ----------------------------------------------------------------------------------------------------
        # LADSPA

        ladspaPlugins  = []
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_native", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_posix32", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_posix64", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_win32", []))
        ladspaPlugins += toList(settingsDB.value("Plugins/LADSPA_win64", []))

        for plugins in ladspaPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LADSPA")
                ladspaCount += 1

        del ladspaPlugins

        # ----------------------------------------------------------------------------------------------------
        # DSSI

        dssiPlugins  = []
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_native", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_posix32", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_posix64", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_win32", []))
        dssiPlugins += toList(settingsDB.value("Plugins/DSSI_win64", []))

        for plugins in dssiPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "DSSI")
                dssiCount += 1

        del dssiPlugins

        # ----------------------------------------------------------------------------------------------------
        # VST2

        vst2Plugins  = []
        vst2Plugins += toList(settingsDB.value("Plugins/VST2_native", []))
        vst2Plugins += toList(settingsDB.value("Plugins/VST2_posix32", []))
        vst2Plugins += toList(settingsDB.value("Plugins/VST2_posix64", []))
        vst2Plugins += toList(settingsDB.value("Plugins/VST2_win32", []))
        vst2Plugins += toList(settingsDB.value("Plugins/VST2_win64", []))

        for plugins in vst2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST2")
                vstCount += 1

        del vst2Plugins

        # ----------------------------------------------------------------------------------------------------
        # VST3

        vst3Plugins  = []
        vst3Plugins += toList(settingsDB.value("Plugins/VST3_native", []))
        vst3Plugins += toList(settingsDB.value("Plugins/VST3_posix32", []))
        vst3Plugins += toList(settingsDB.value("Plugins/VST3_posix64", []))
        vst3Plugins += toList(settingsDB.value("Plugins/VST3_win32", []))
        vst3Plugins += toList(settingsDB.value("Plugins/VST3_win64", []))

        for plugins in vst3Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST3")
                vst3Count += 1

        del vst3Plugins

        # ----------------------------------------------------------------------------------------------------
        # AU

        if MACOS:
            auPlugins  = []
            auPlugins += toList(settingsDB.value("Plugins/AU_native", []))
            auPlugins += toList(settingsDB.value("Plugins/AU_posix32", []))
            auPlugins += toList(settingsDB.value("Plugins/AU_posix64", []))

            for plugins in auPlugins:
                for plugin in plugins:
                    self._addPluginToTable(plugin, "AU")
                    auCount += 1

            del auPlugins

        # ----------------------------------------------------------------------------------------------------
        # Kits

        gigs = toList(settingsDB.value("Plugins/GIG", []))

        for gig in gigs:
            for gig_i in gig:
                self._addPluginToTable(gig_i, "GIG")
                kitCount += 1

        del gigs

        # ----------------------------------------------------------------------------------------------------

        sf2s = toList(settingsDB.value("Plugins/SF2", []))

        for sf2 in sf2s:
            for sf2_i in sf2:
                self._addPluginToTable(sf2_i, "SF2")
                kitCount += 1

        del sf2s

        # ----------------------------------------------------------------------------------------------------

        sfzs = toList(settingsDB.value("Plugins/SFZ", []))

        for sfz in sfzs:
            for sfz_i in sfz:
                self._addPluginToTable(sfz_i, "SFZ")
                kitCount += 1

        del sfzs

        # ----------------------------------------------------------------------------------------------------

        self.ui.tableWidget.setSortingEnabled(True)
        self.ui.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        if MACOS:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST, %i VST3 and %i AudioUnit plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, auCount, kitCount)))
        else:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST and %i VST3 plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, kitCount)))

        self._checkFilters()

    # --------------------------------------------------------------------------------------------------------

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    from carla_app import CarlaApplication
    from carla_host import initHost, loadHostSettings

    initName, libPrefix = handleInitialCommandLineArguments(__file__ if "__file__" in dir() else None)

    app  = CarlaApplication("Carla2-Database", libPrefix)
    host = initHost("Carla2-Database", libPrefix, False, False, False)
    loadHostSettings(host)

    gui = PluginDatabaseW(None, host)
    gui.show()

    app.exit_exec()

# ------------------------------------------------------------------------------------------------------------
