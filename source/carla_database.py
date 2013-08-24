#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin database code
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
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from copy import deepcopy
from subprocess import Popen, PIPE

try:
    from PyQt5.QtCore import Qt, QThread, QSettings
    from PyQt5.QtWidgets import QDialog, QTableWidgetItem
except:
    from PyQt4.QtCore import Qt, QThread, QSettings
    from PyQt4.QtGui import QDialog, QTableWidgetItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_database
import ui_carla_refresh
from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Try Import LADSPA-RDF

try:
    import ladspa_rdf
    import json
    haveLRDF = True
except:
    qWarning("LRDF Support not available (LADSPA-RDF will be disabled)")
    haveLRDF = False

# ------------------------------------------------------------------------------------------------------------
# Set LADSPA-RDF Path

if haveLRDF and readEnvVars:
    LADSPA_RDF_PATH_env = os.getenv("LADSPA_RDF_PATH")
    if LADSPA_RDF_PATH_env:
        ladspa_rdf.set_rdf_path(LADSPA_RDF_PATH_env.split(splitter))
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

def findLV2Bundles(bundlePath):
    bundles = []

    for root, dirs, files in os.walk(bundlePath):
        if os.path.exists(os.path.join(root, "manifest.ttl")):
            bundles.append(root)

    return bundles

def findMacVSTBundles(bundlePath):
    bundles = []

    for root, dirs, files in os.walk(bundlePath):
        for name in [name for name in dirs if name.lower().endswith(".vst")]:
            bundles.append(os.path.join(root, name))

    return bundles

def findFilenames(filePath, stype):
    filenames = []

    if stype == "csound":
        extensions = (".csd",)
    elif stype == "gig":
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

# FIXME - put this into c++ discovery
def findDSSIGUI(filename, name, label):
    pluginDir = filename.rsplit(".", 1)[0]
    shortName = os.path.basename(pluginDir)
    guiFilename = ""

    checkName  = name.replace(" ", "_")
    checkLabel = label
    checkSName = shortName

    if checkName[-1]  != "_": checkName  += "_"
    if checkLabel[-1] != "_": checkLabel += "_"
    if checkSName[-1] != "_": checkSName += "_"

    for root, dirs, files in os.walk(pluginDir):
        guiFiles = files
        break
    else:
        guiFiles = []

    for guiFile in guiFiles:
        if guiFile.startswith((checkName, checkLabel, checkSName)):
            guiFilename = os.path.join(pluginDir, guiFile)
            break

    return guiFilename

# ------------------------------------------------------------------------------------------------------------
# Plugin Query

PLUGIN_QUERY_API_VERSION = 1

PyPluginInfo = {
    'API': PLUGIN_QUERY_API_VERSION,
    'build': BINARY_NONE,
    'type': PLUGIN_NONE,
    'hints': 0x0,
    'binary': "",
    'name': "",
    'label': "",
    'maker': "",
    'copyright': "",
    'uniqueId': 0,
    'audio.ins': 0,
    'audio.outs': 0,
    'audio.total': 0,
    'midi.ins': 0,
    'midi.outs': 0,
    'midi.total': 0,
    'parameters.ins': 0,
    'parameters.outs': 0,
    'parameters.total': 0,
    'programs.total': 0
}

global discoveryProcess
discoveryProcess = None

def runCarlaDiscovery(itype, stype, filename, tool, isWine=False):
    if not os.path.exists(tool):
        qCritical("runCarlaDiscovery() - tool '%s' does not exist" % tool)
        return

    command = []

    if LINUX or MACOS:
        command.append("env")
        command.append("LANG=C")
        if isWine:
            command.append("WINEDEBUG=-all")

    command.append(tool)
    command.append(stype)
    command.append(filename)

    global discoveryProcess
    discoveryProcess = Popen(command, stdout=PIPE)

    try:
        discoveryProcess.wait()
        output = discoveryProcess.stdout.read().decode("utf-8", errors="ignore").split("\n")
    except:
        output = ()

    pinfo = None
    plugins = []
    fakeLabel = os.path.basename(filename).rsplit(".", 1)[0]

    for line in output:
        line = line.strip()
        if line == "carla-discovery::init::-----------":
            pinfo = deepcopy(PyPluginInfo)
            pinfo['type']   = itype
            pinfo['binary'] = filename

        elif line == "carla-discovery::end::------------":
            if pinfo != None:
                plugins.append(pinfo)
                del pinfo
                pinfo = None

        elif line == "Segmentation fault":
            print("carla-discovery::crash::%s crashed during discovery" % filename)

        elif line.startswith("err:module:import_dll Library"):
            print(line)

        elif line.startswith("carla-discovery::error::"):
            print("%s - %s" % (line, filename))

        elif line.startswith("carla-discovery::"):
            if pinfo == None:
                continue

            try:
                prop, value = line.replace("carla-discovery::", "").split("::", 1)
            except:
               continue

            if prop == "name":
                pinfo['name'] = value if value else fakeLabel
            elif prop == "label":
                pinfo['label'] = value if value else fakeLabel
            elif prop == "maker":
                pinfo['maker'] = value
            elif prop == "copyright":
                pinfo['copyright'] = value
            elif prop == "uniqueId":
                if value.isdigit(): pinfo['uniqueId'] = int(value)
            elif prop == "hints":
                if value.isdigit(): pinfo['hints'] = int(value)
            elif prop == "audio.ins":
                if value.isdigit(): pinfo['audio.ins'] = int(value)
            elif prop == "audio.outs":
                if value.isdigit(): pinfo['audio.outs'] = int(value)
            elif prop == "audio.total":
                if value.isdigit(): pinfo['audio.total'] = int(value)
            elif prop == "midi.ins":
                if value.isdigit(): pinfo['midi.ins'] = int(value)
            elif prop == "midi.outs":
                if value.isdigit(): pinfo['midi.outs'] = int(value)
            elif prop == "midi.total":
                if value.isdigit(): pinfo['midi.total'] = int(value)
            elif prop == "parameters.ins":
                if value.isdigit(): pinfo['parameters.ins'] = int(value)
            elif prop == "parameters.outs":
                if value.isdigit(): pinfo['parameters.outs'] = int(value)
            elif prop == "parameters.total":
                if value.isdigit(): pinfo['parameters.total'] = int(value)
            elif prop == "programs.total":
                if value.isdigit(): pinfo['programs.total'] = int(value)
            elif prop == "build":
                if value.isdigit(): pinfo['build'] = int(value)
            elif prop == "uri":
                if value: pinfo['label'] = value
                else:
                    # cannot use empty URIs
                    del pinfo
                    pinfo = None
                    continue

    # FIXME - put this into c++ discovery
    for pinfo in plugins:
        if itype == PLUGIN_DSSI:
            if findDSSIGUI(pinfo['binary'], pinfo['name'], pinfo['label']):
                pinfo['hints'] |= PLUGIN_HAS_GUI

    # FIXME?
    tmp = discoveryProcess
    discoveryProcess = None
    del discoveryProcess, tmp

    return plugins

def killDiscovery():
    global discoveryProcess

    if discoveryProcess is not None:
        discoveryProcess.kill()

def checkPluginInternal(desc):
    plugins = []

    pinfo = deepcopy(PyPluginInfo)
    pinfo['build'] = BINARY_NATIVE
    pinfo['type']  = PLUGIN_INTERNAL
    pinfo['hints'] = int(desc['hints'])
    pinfo['name']  = cString(desc['name'])
    pinfo['label'] = cString(desc['label'])
    pinfo['maker'] = cString(desc['maker'])
    pinfo['copyright'] = cString(desc['copyright'])

    pinfo['audio.ins']   = int(desc['audioIns'])
    pinfo['audio.outs']  = int(desc['audioOuts'])
    pinfo['audio.total'] = pinfo['audio.ins'] + pinfo['audio.outs']

    pinfo['midi.ins']   = int(desc['midiIns'])
    pinfo['midi.outs']  = int(desc['midiOuts'])
    pinfo['midi.total'] = pinfo['midi.ins'] + pinfo['midi.outs']

    pinfo['parameters.ins']   = int(desc['parameterIns'])
    pinfo['parameters.outs']  = int(desc['parameterOuts'])
    pinfo['parameters.total'] = pinfo['parameters.ins'] + pinfo['parameters.outs']

    plugins.append(pinfo)

    return plugins

def checkPluginLADSPA(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LADSPA, "LADSPA", filename, tool, isWine)

def checkPluginDSSI(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_DSSI, "DSSI", filename, tool, isWine)

def checkPluginLV2(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_LV2, "LV2", filename, tool, isWine)

def checkPluginVST(filename, tool, isWine=False):
    return runCarlaDiscovery(PLUGIN_VST, "VST", filename, tool, isWine)

def checkPluginAU(filename, tool):
    return runCarlaDiscovery(PLUGIN_AU, "AU", filename, tool)

def checkPluginCSOUND(filename, tool):
    return runCarlaDiscovery(PLUGIN_CSOUND, "CSOUND", filename, tool)

def checkPluginGIG(filename, tool):
    return runCarlaDiscovery(PLUGIN_GIG, "GIG", filename, tool)

def checkPluginSF2(filename, tool):
    return runCarlaDiscovery(PLUGIN_SF2, "SF2", filename, tool)

def checkPluginSFZ(filename, tool):
    return runCarlaDiscovery(PLUGIN_SFZ, "SFZ", filename, tool)

# ------------------------------------------------------------------------------------------------------------
# Separate Thread for Plugin Search

class SearchPluginsThread(QThread):
    pluginLook = pyqtSignal(int, str)

    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.fContinueChecking = False

        self.fCheckNative  = False
        self.fCheckPosix32 = False
        self.fCheckPosix64 = False
        self.fCheckWin32   = False
        self.fCheckWin64   = False

        self.fCheckLADSPA = False
        self.fCheckDSSI   = False
        self.fCheckLV2    = False
        self.fCheckVST    = False
        self.fCheckAU     = False
        self.fCheckCSOUND = False
        self.fCheckGIG    = False
        self.fCheckSF2    = False
        self.fCheckSFZ    = False

        self.fToolNative = carla_discovery_native

        self.fCurCount = 0
        self.fCurPercentValue = 0
        self.fLastCheckValue  = 0
        self.fSomethingChanged = False

        self.fLadspaPlugins = []
        self.fDssiPlugins   = []
        self.fLv2Plugins    = []
        self.fVstPlugins    = []
        self.fAuPlugins     = []
        self.fCsoundPlugins = []
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

    def setSearchPluginTypes(self, ladspa, dssi, lv2, vst, au, csound, gig, sf2, sfz):
        self.fCheckLADSPA = ladspa
        self.fCheckDSSI   = dssi
        self.fCheckLV2    = lv2
        self.fCheckVST    = vst
        self.fCheckAU     = au and MACOS
        self.fCheckCSOUND = csound
        self.fCheckGIG    = gig
        self.fCheckSF2    = sf2
        self.fCheckSFZ    = sfz

    def setSearchToolNative(self, tool):
        self.fToolNative = tool

    def stop(self):
        self.fContinueChecking = False

    def run(self):
        self.fContinueChecking = True
        self.fCurCount = 0
        pluginCount    = 0
        settingsDB     = QSettings("falkTX", "CarlaPlugins")

        if self.fCheckLADSPA: pluginCount += 1
        if self.fCheckDSSI:   pluginCount += 1
        if self.fCheckLV2:    pluginCount += 1
        if self.fCheckVST:    pluginCount += 1
        if self.fCheckAU:     pluginCount += 1

        if self.fCheckNative:
            self.fCurCount += pluginCount
        if self.fCheckPosix32:
            self.fCurCount += pluginCount
        if self.fCheckPosix64:
            self.fCurCount += pluginCount
        if self.fCheckWin32:
            self.fCurCount += pluginCount
            if self.fCheckAU: self.fCurCount -= 1
        if self.fCheckWin64:
            self.fCurCount += pluginCount
            if self.fCheckAU: self.fCurCount -= 1

        if self.fCheckNative and self.fToolNative:
            if self.fCheckCSOUND: self.fCurCount += 1
            if self.fCheckGIG:    self.fCurCount += 1
            if self.fCheckSF2:    self.fCurCount += 1
            if self.fCheckSFZ:    self.fCurCount += 1
        else:
            self.fCheckCSOUND = False
            self.fCheckGIG    = False
            self.fCheckSF2    = False
            self.fCheckSFZ    = False

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
                self._checkLADSPA(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/LADSPA_posix32", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkLADSPA(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/LADSPA_posix64", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkLADSPA("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win32", self.fLadspaPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkLADSPA("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win64", self.fLadspaPlugins)

            settingsDB.sync()

            if not self.fContinueChecking: return

            if haveLRDF and checkValue > 0:
                startValue = self.fLastCheckValue - rdfPadValue

                self._pluginLook(startValue, "LADSPA RDFs...")
                ladspaRdfInfo = ladspa_rdf.recheck_all_plugins(self, startValue, self.fCurPercentValue, checkValue)

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
                self._checkDSSI(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/DSSI_posix32", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkDSSI(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/DSSI_posix64", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkDSSI("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win32", self.fDssiPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkDSSI("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win64", self.fDssiPlugins)

            settingsDB.sync()

        if not self.fContinueChecking: return

        if self.fCheckLV2:
            if self.fCheckNative:
                self._checkLV2(self.fToolNative)
                settingsDB.setValue("Plugins/LV2_native", self.fLv2Plugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkLV2(carla_discovery_posix32)
                settingsDB.setValue("Plugins/LV2_posix32", self.fLv2Plugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkLV2(carla_discovery_posix64)
                settingsDB.setValue("Plugins/LV2_posix64", self.fLv2Plugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkLV2(carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/LV2_win32", self.fLv2Plugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkLV2(carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/LV2_win64", self.fLv2Plugins)

            settingsDB.sync()

        if not self.fContinueChecking: return

        if self.fCheckVST:
            if self.fCheckNative:
                self._checkVST(OS, self.fToolNative)
                settingsDB.setValue("Plugins/VST_native", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkVST(OS, carla_discovery_posix32)
                settingsDB.setValue("Plugins/VST_posix32", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkVST(OS, carla_discovery_posix64)
                settingsDB.setValue("Plugins/VST_posix64", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin32:
                self._checkVST("WINDOWS", carla_discovery_win32, not WINDOWS)
                settingsDB.setValue("Plugins/VST_win32", self.fVstPlugins)

            if not self.fContinueChecking: return

            if self.fCheckWin64:
                self._checkVST("WINDOWS", carla_discovery_win64, not WINDOWS)
                settingsDB.setValue("Plugins/VST_win64", self.fVstPlugins)

            settingsDB.sync()

        if not self.fContinueChecking: return

        if self.fCheckAU:
            if self.fCheckNative:
                self._checkAU(self.fToolNative)
                settingsDB.setValue("Plugins/AU_native", self.fAuPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix32:
                self._checkAU(carla_discovery_posix32)
                settingsDB.setValue("Plugins/AU_posix32", self.fAuPlugins)

            if not self.fContinueChecking: return

            if self.fCheckPosix64:
                self._checkAU(carla_discovery_posix64)
                settingsDB.setValue("Plugins/AU_posix64", self.fAuPlugins)

            settingsDB.sync()

        if not self.fContinueChecking: return

        if self.fCheckCSOUND:
            self._checkCSOUND()
            settingsDB.setValue("Plugins/CSOUND", self.fCsoundPlugins)

        if not self.fContinueChecking: return

        if self.fCheckGIG:
            self._checkKIT(Carla.GIG_PATH, "gig")
            settingsDB.setValue("Plugins/GIG", self.fKitPlugins)

        if not self.fContinueChecking: return

        if self.fCheckSF2:
            self._checkKIT(Carla.SF2_PATH, "sf2")
            settingsDB.setValue("Plugins/SF2", self.fKitPlugins)

        if not self.fContinueChecking: return

        if self.fCheckSFZ:
            self._checkKIT(Carla.SFZ_PATH, "sfz")
            settingsDB.setValue("Plugins/SFZ", self.fKitPlugins)

        settingsDB.sync()

    def _checkLADSPA(self, OS, tool, isWine=False):
        ladspaBinaries = []
        self.fLadspaPlugins = []

        self._pluginLook(self.fLastCheckValue, "LADSPA plugins...")

        for iPATH in Carla.LADSPA_PATH:
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

        for iPATH in Carla.DSSI_PATH:
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

    def _checkLV2(self, tool, isWine=False):
        lv2Bundles = []
        self.fLv2Plugins = []

        self._pluginLook(self.fLastCheckValue, "LV2 bundles...")

        for iPATH in Carla.LV2_PATH:
            bundles = findLV2Bundles(iPATH)
            for bundle in bundles:
                if bundle not in lv2Bundles:
                    lv2Bundles.append(bundle)

        lv2Bundles.sort()

        if not self.fContinueChecking: return

        for i in range(len(lv2Bundles)):
            lv2     = lv2Bundles[i]
            percent = ( float(i) / len(lv2Bundles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, lv2)

            plugins = checkPluginLV2(lv2, tool, isWine)
            if plugins:
                self.fLv2Plugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkVST(self, OS, tool, isWine=False):
        vstBinaries = []
        self.fVstPlugins = []

        if MACOS and not isWine:
            self._pluginLook(self.fLastCheckValue, "VST bundles...")
        else:
            self._pluginLook(self.fLastCheckValue, "VST plugins...")

        for iPATH in Carla.VST_PATH:
            if MACOS and not isWine:
                binaries = findMacVSTBundles(iPATH)
            else:
                binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in vstBinaries:
                    vstBinaries.append(binary)

        vstBinaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(vstBinaries)):
            vst     = vstBinaries[i]
            percent = ( float(i) / len(vstBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst)

            plugins = checkPluginVST(vst, tool, isWine)
            if plugins:
                self.fVstPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkAU(self, tool):
        auBinaries = []
        self.fAuPlugins = []

        # FIXME - this probably uses bundles
        self._pluginLook(self.fLastCheckValue, "AU plugins...")

        for iPATH in Carla.AU_PATH:
            binaries = findBinaries(iPATH, "MACOS")
            for binary in binaries:
                if binary not in auBinaries:
                    auBinaries.append(binary)

        auBinaries.sort()

        if not self.fContinueChecking: return

        for i in range(len(auBinaries)):
            au      = auBinaries[i]
            percent = ( float(i) / len(auBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, au)

            plugins = checkPluginAU(au, tool)
            if plugins:
                self.fAuPlugins.append(plugins)
                self.fSomethingChanged = True

            if not self.fContinueChecking: break

        self.fLastCheckValue += self.fCurPercentValue

    def _checkCSOUND(self):
        csoundFiles = []
        self.fCsoundPlugins = []

        for iPATH in Carla.CSOUND_PATH:
            files = findFilenames(iPATH, "csd")
            for file_ in files:
                if file_ not in csoundFiles:
                    csoundFiles.append(file_)

        csoundFiles.sort()

        if not self.fContinueChecking: return

        for i in range(len(csoundFiles)):
            csd     = csoundFiles[i]
            percent = ( float(i) / len(csoundFiles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, csd)

            plugins = checkPluginCSOUND(csd, self.fToolNative)
            if plugins:
                self.fCsoundPlugins.append(plugins)
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
                plugins = checkPluginGIG(kit, self.fToolNative)
            elif kitExtension == "sf2":
                plugins = checkPluginSF2(kit, self.fToolNative)
            elif kitExtension == "sfz":
                plugins = checkPluginSFZ(kit, self.fToolNative)
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
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_refresh.Ui_PluginRefreshW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fThread = SearchPluginsThread(self)

        # -------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------
        # Set-up GUI

        self.fIconYes = getIcon("dialog-ok-apply").pixmap(16, 16)
        self.fIconNo  = getIcon("dialog-error").pixmap(16, 16)

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

        if carla_discovery_posix32 and not WINDOWS:
            self.ui.ico_posix32.setPixmap(self.fIconYes)
        else:
            self.ui.ico_posix32.setPixmap(self.fIconNo)
            self.ui.ch_posix32.setChecked(False)
            self.ui.ch_posix32.setEnabled(False)

        if carla_discovery_posix64 and not WINDOWS:
            self.ui.ico_posix64.setPixmap(self.fIconYes)
        else:
            self.ui.ico_posix64.setPixmap(self.fIconNo)
            self.ui.ch_posix64.setChecked(False)
            self.ui.ch_posix64.setEnabled(False)

        if carla_discovery_win32:
            self.ui.ico_win32.setPixmap(self.fIconYes)
        else:
            self.ui.ico_win32.setPixmap(self.fIconNo)
            self.ui.ch_win32.setChecked(False)
            self.ui.ch_win32.setEnabled(False)

        if carla_discovery_win64:
            self.ui.ico_win64.setPixmap(self.fIconYes)
        else:
            self.ui.ico_win64.setPixmap(self.fIconNo)
            self.ui.ch_win64.setChecked(False)
            self.ui.ch_win64.setEnabled(False)

        if haveLRDF:
            self.ui.ico_rdflib.setPixmap(self.fIconYes)
        else:
            self.ui.ico_rdflib.setPixmap(self.fIconNo)

        hasNative = bool(carla_discovery_native)
        hasNonNative = False

        if WINDOWS:
            if kIs64bit:
                hasNative = bool(carla_discovery_win64)
                hasNonNative = bool(carla_discovery_win32)
                self.fThread.setSearchToolNative(carla_discovery_win64)
                self.ui.ch_win64.setChecked(False)
                self.ui.ch_win64.setVisible(False)
                self.ui.ico_win64.setVisible(False)
                self.ui.label_win64.setVisible(False)
            else:
                hasNative = bool(carla_discovery_win32)
                hasNonNative = bool(carla_discovery_win64)
                self.fThread.setSearchToolNative(carla_discovery_win32)
                self.ui.ch_win32.setChecked(False)
                self.ui.ch_win32.setVisible(False)
                self.ui.ico_win32.setVisible(False)
                self.ui.label_win32.setVisible(False)
        elif LINUX or MACOS:
            if kIs64bit:
                hasNonNative = bool(carla_discovery_posix32 or carla_discovery_win32 or carla_discovery_win64)
                self.ui.ch_posix64.setChecked(False)
                self.ui.ch_posix64.setVisible(False)
                self.ui.ico_posix64.setVisible(False)
                self.ui.label_posix64.setVisible(False)
            else:
                hasNonNative = bool(carla_discovery_posix64 or carla_discovery_win32 or carla_discovery_win64)
                self.ui.ch_posix32.setChecked(False)
                self.ui.ch_posix32.setVisible(False)
                self.ui.ico_posix32.setVisible(False)
                self.ui.label_posix32.setVisible(False)

        if hasNative:
            self.ui.ico_native.setPixmap(self.fIconYes)
        else:
            self.ui.ico_native.setPixmap(self.fIconNo)
            self.ui.ch_native.setChecked(False)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_csound.setChecked(False)
            self.ui.ch_csound.setEnabled(False)
            self.ui.ch_gig.setChecked(False)
            self.ui.ch_gig.setEnabled(False)
            self.ui.ch_sf2.setChecked(False)
            self.ui.ch_sf2.setEnabled(False)
            self.ui.ch_sfz.setChecked(False)
            self.ui.ch_sfz.setEnabled(False)
            if not hasNonNative:
                self.ui.ch_ladspa.setChecked(False)
                self.ui.ch_ladspa.setEnabled(False)
                self.ui.ch_dssi.setChecked(False)
                self.ui.ch_dssi.setEnabled(False)
                self.ui.ch_lv2.setChecked(False)
                self.ui.ch_lv2.setEnabled(False)
                self.ui.ch_vst.setChecked(False)
                self.ui.ch_vst.setEnabled(False)
                self.ui.ch_au.setChecked(False)
                self.ui.ch_au.setEnabled(False)
                self.ui.b_start.setEnabled(False)

        if not MACOS:
            self.ui.ch_au.setChecked(False)
            self.ui.ch_au.setEnabled(False)
            self.ui.ch_au.setVisible(False)

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.b_start.clicked.connect(self.slot_start)
        self.ui.b_skip.clicked.connect(self.slot_skip)
        self.ui.ch_native.clicked.connect(self.slot_checkTools)
        self.ui.ch_posix32.clicked.connect(self.slot_checkTools)
        self.ui.ch_posix64.clicked.connect(self.slot_checkTools)
        self.ui.ch_win32.clicked.connect(self.slot_checkTools)
        self.ui.ch_win64.clicked.connect(self.slot_checkTools)
        self.ui.ch_ladspa.clicked.connect(self.slot_checkTools)
        self.ui.ch_dssi.clicked.connect(self.slot_checkTools)
        self.ui.ch_lv2.clicked.connect(self.slot_checkTools)
        self.ui.ch_vst.clicked.connect(self.slot_checkTools)
        self.ui.ch_au.clicked.connect(self.slot_checkTools)
        self.ui.ch_csound.clicked.connect(self.slot_checkTools)
        self.ui.ch_gig.clicked.connect(self.slot_checkTools)
        self.ui.ch_sf2.clicked.connect(self.slot_checkTools)
        self.ui.ch_sfz.clicked.connect(self.slot_checkTools)
        self.fThread.pluginLook.connect(self.slot_handlePluginLook)
        self.fThread.finished.connect(self.slot_handlePluginThreadFinished)

    @pyqtSlot()
    def slot_start(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(100)
        self.ui.progressBar.setValue(0)
        self.ui.b_start.setEnabled(False)
        self.ui.b_skip.setVisible(True)
        self.ui.b_close.setVisible(False)

        native, posix32, posix64, win32, win64 = (self.ui.ch_native.isChecked(),
                                                  self.ui.ch_posix32.isChecked(), self.ui.ch_posix64.isChecked(),
                                                  self.ui.ch_win32.isChecked(), self.ui.ch_win64.isChecked())

        ladspa, dssi, lv2, vst, au, csound, gig, sf2, sfz = (self.ui.ch_ladspa.isChecked(), self.ui.ch_dssi.isChecked(),
                                                             self.ui.ch_lv2.isChecked(), self.ui.ch_vst.isChecked(),
                                                             self.ui.ch_au.isChecked(), self.ui.ch_csound.isChecked(),
                                                             self.ui.ch_gig.isChecked(), self.ui.ch_sf2.isChecked(), self.ui.ch_sfz.isChecked())

        self.fThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
        self.fThread.setSearchPluginTypes(ladspa, dssi, lv2, vst, au, csound, gig, sf2, sfz)
        self.fThread.start()

    @pyqtSlot()
    def slot_skip(self):
        killDiscovery()

    @pyqtSlot()
    def slot_checkTools(self):
        enabled1 = bool(self.ui.ch_native.isChecked() or self.ui.ch_posix32.isChecked() or self.ui.ch_posix64.isChecked() or self.ui.ch_win32.isChecked() or self.ui.ch_win64.isChecked())
        enabled2 = bool(self.ui.ch_ladspa.isChecked() or self.ui.ch_dssi.isChecked() or self.ui.ch_lv2.isChecked() or self.ui.ch_vst.isChecked() or self.ui.ch_au.isChecked() or
                        self.ui.ch_csound.isChecked() or self.ui.ch_gig.isChecked() or self.ui.ch_sf2.isChecked() or self.ui.ch_sfz.isChecked())
        self.ui.b_start.setEnabled(enabled1 and enabled2)

    @pyqtSlot(int, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.ui.progressBar.setFormat("%s" % plugin)
        self.ui.progressBar.setValue(percent)

    @pyqtSlot()
    def slot_handlePluginThreadFinished(self):
        self.ui.progressBar.setMinimum(0)
        self.ui.progressBar.setMaximum(1)
        self.ui.progressBar.setValue(1)
        self.ui.progressBar.setFormat(self.tr("Done"))
        self.ui.b_start.setEnabled(True)
        self.ui.b_skip.setVisible(False)
        self.ui.b_close.setVisible(True)

    def loadSettings(self):
        settings = QSettings()
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/SearchLADSPA", True, type=bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/SearchDSSI", True, type=bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/SearchLV2", True, type=bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/SearchVST", True, type=bool))
        self.ui.ch_au.setChecked(settings.value("PluginDatabase/SearchAU", True, type=bool))
        self.ui.ch_csound.setChecked(settings.value("PluginDatabase/SearchCSOUND", False, type=bool))
        self.ui.ch_gig.setChecked(settings.value("PluginDatabase/SearchGIG", False, type=bool))
        self.ui.ch_sf2.setChecked(settings.value("PluginDatabase/SearchSF2", False, type=bool))
        self.ui.ch_sfz.setChecked(settings.value("PluginDatabase/SearchSFZ", False, type=bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/SearchNative", True, type=bool))
        self.ui.ch_posix32.setChecked(settings.value("PluginDatabase/SearchPOSIX32", False, type=bool))
        self.ui.ch_posix64.setChecked(settings.value("PluginDatabase/SearchPOSIX64", False, type=bool))
        self.ui.ch_win32.setChecked(settings.value("PluginDatabase/SearchWin32", False, type=bool))
        self.ui.ch_win64.setChecked(settings.value("PluginDatabase/SearchWin64", False, type=bool))

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("PluginDatabase/SearchLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/SearchDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/SearchLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/SearchVST", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/SearchAU", self.ui.ch_au.isChecked())
        settings.setValue("PluginDatabase/SearchCSOUND", self.ui.ch_csound.isChecked())
        settings.setValue("PluginDatabase/SearchGIG", self.ui.ch_gig.isChecked())
        settings.setValue("PluginDatabase/SearchSF2", self.ui.ch_sf2.isChecked())
        settings.setValue("PluginDatabase/SearchSFZ", self.ui.ch_sfz.isChecked())
        settings.setValue("PluginDatabase/SearchNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX32", self.ui.ch_posix32.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX64", self.ui.ch_posix64.isChecked())
        settings.setValue("PluginDatabase/SearchWin32", self.ui.ch_win32.isChecked())
        settings.setValue("PluginDatabase/SearchWin64", self.ui.ch_win64.isChecked())

    def closeEvent(self, event):
        self.saveSettings()

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

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Database Dialog

class PluginDatabaseW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_database.Ui_PluginDatabaseW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fLastTableIndex = 0
        self.fRetPlugin  = None
        self.fRealParent = parent

        # -------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------
        # Set-up GUI

        self.ui.b_add.setEnabled(False)

        if BINARY_NATIVE in (BINARY_POSIX32, BINARY_WIN32):
            self.ui.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ui.ch_bridged.setText(self.tr("Bridged (32bit)"))

        if not (LINUX or MACOS):
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)

        # -------------------------------------------------------------
        # Set-up connections

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
        self.ui.ch_au.clicked.connect(self.slot_checkFilters)
        self.ui.ch_csound.clicked.connect(self.slot_checkFilters)
        self.ui.ch_kits.clicked.connect(self.slot_checkFilters)
        self.ui.ch_native.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged_wine.clicked.connect(self.slot_checkFilters)
        self.ui.ch_rtsafe.clicked.connect(self.slot_checkFilters)
        self.ui.ch_gui.clicked.connect(self.slot_checkFilters)
        self.ui.ch_stereo.clicked.connect(self.slot_checkFilters)

        # -------------------------------------------------------------

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
        if PluginRefreshW(self).exec_():
            self._reAddPlugins()

            if self.fRealParent:
                self.fRealParent.loadRDFsNeeded()

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
        hideVST      = not self.ui.ch_vst.isChecked()
        hideAU       = not self.ui.ch_au.isChecked()
        hideCsound   = not self.ui.ch_csound.isChecked()
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
            hasGui   = bool(plugin['hints'] & PLUGIN_HAS_GUI)

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
            elif hideVST and ptype == "VST":
                self.ui.tableWidget.hideRow(i)
            elif hideAU and ptype == "AU":
                self.ui.tableWidget.hideRow(i)
            elif hideCsound and ptype == "CSOUND":
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

    def _showFilters(self, yesNo):
        self.ui.tb_filters.setArrowType(Qt.UpArrow if yesNo else Qt.DownArrow)
        self.ui.frame.setVisible(yesNo)

    def _addPluginToTable(self, plugin, ptype):
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
        self.ui.tableWidget.setItem(index, 8, QTableWidgetItem(str(plugin['programs.total'])))
        self.ui.tableWidget.setItem(index, 9, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_HAS_GUI) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 10, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_IS_SYNTH) else self.tr("No")))
        self.ui.tableWidget.setItem(index, 11, QTableWidgetItem(bridgeText))
        self.ui.tableWidget.setItem(index, 12, QTableWidgetItem(ptype))
        self.ui.tableWidget.setItem(index, 13, QTableWidgetItem(str(plugin['binary'])))
        self.ui.tableWidget.item(index, 0).setData(Qt.UserRole, plugin)

        self.fLastTableIndex += 1

    def _reAddPlugins(self):
        settingsDB = QSettings("falkTX", "CarlaPlugins")

        for x in range(self.ui.tableWidget.rowCount()):
            self.ui.tableWidget.removeRow(0)

        self.fLastTableIndex = 0
        self.ui.tableWidget.setSortingEnabled(False)

        internalCount = 0
        ladspaCount = 0
        dssiCount   = 0
        lv2Count    = 0
        vstCount    = 0
        auCount     = 0
        csoundCount = 0
        kitCount    = 0

        # ---------------------------------------------------------------------------
        # Internal

        internalPlugins = toList(settingsDB.value("Plugins/Internal", []))

        for plugins in internalPlugins:
            for plugin in plugins:
                internalCount += 1

        if Carla.host is not None and internalCount != Carla.host.get_internal_plugin_count():
            internalCount   = Carla.host.get_internal_plugin_count()
            internalPlugins = []

            for i in range(Carla.host.get_internal_plugin_count()):
                descInfo = Carla.host.get_internal_plugin_info(i)
                plugins  = checkPluginInternal(descInfo)

                if plugins:
                    internalPlugins.append(plugins)

            settingsDB.setValue("Plugins/Internal", internalPlugins)

        for plugins in internalPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, self.tr("Internal"))

        # ---------------------------------------------------------------------------
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

        # ---------------------------------------------------------------------------
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

        # ---------------------------------------------------------------------------
        # LV2

        lv2Plugins  = []
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_native", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_posix32", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_posix64", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_win32", []))
        lv2Plugins += toList(settingsDB.value("Plugins/LV2_win64", []))

        for plugins in lv2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LV2")
                lv2Count += 1

        # ---------------------------------------------------------------------------
        # VST

        vstPlugins  = []
        vstPlugins += toList(settingsDB.value("Plugins/VST_native", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_posix32", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_posix64", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_win32", []))
        vstPlugins += toList(settingsDB.value("Plugins/VST_win64", []))

        for plugins in vstPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST")
                vstCount += 1

        # ---------------------------------------------------------------------------
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

        # ---------------------------------------------------------------------------
        # Csound

        csds = toList(settingsDB.value("Plugins/CSOUND", []))

        for csd in csds:
            for csd_i in csd:
                self._addPluginToTable(csd_i, "CSOUND")
                csoundCount += 1

        # ---------------------------------------------------------------------------
        # Kits

        gigs = toList(settingsDB.value("Plugins/GIG", []))
        sf2s = toList(settingsDB.value("Plugins/SF2", []))
        sfzs = toList(settingsDB.value("Plugins/SFZ", []))

        for gig in gigs:
            for gig_i in gig:
                self._addPluginToTable(gig_i, "GIG")
                kitCount += 1

        for sf2 in sf2s:
            for sf2_i in sf2:
                self._addPluginToTable(sf2_i, "SF2")
                kitCount += 1

        for sfz in sfzs:
            for sfz_i in sfz:
                self._addPluginToTable(sfz_i, "SFZ")
                kitCount += 1

        # ---------------------------------------------------------------------------

        self.ui.tableWidget.setSortingEnabled(True)
        self.ui.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        if MACOS:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST and %i AudioUnit plugins, plus %i CSound modules and %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, auCount, csoundCount, kitCount)))
        else:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2 and %i VST plugins, plus %i CSound modules and %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, csoundCount, kitCount)))

        self._checkFilters()

    def loadSettings(self):
        settings = QSettings()
        self.restoreGeometry(settings.value("PluginDatabase/Geometry", ""))
        self.ui.tableWidget.horizontalHeader().restoreState(settings.value("PluginDatabase/TableGeometry", ""))
        self.ui.ch_effects.setChecked(settings.value("PluginDatabase/ShowEffects", True, type=bool))
        self.ui.ch_instruments.setChecked(settings.value("PluginDatabase/ShowInstruments", True, type=bool))
        self.ui.ch_midi.setChecked(settings.value("PluginDatabase/ShowMIDI", True, type=bool))
        self.ui.ch_other.setChecked(settings.value("PluginDatabase/ShowOther", True, type=bool))
        self.ui.ch_internal.setChecked(settings.value("PluginDatabase/ShowInternal", True, type=bool))
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/ShowLADSPA", True, type=bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/ShowDSSI", True, type=bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/ShowLV2", True, type=bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/ShowVST", True, type=bool))
        self.ui.ch_au.setChecked(settings.value("PluginDatabase/ShowAU", True, type=bool))
        self.ui.ch_csound.setChecked(settings.value("PluginDatabase/ShowCSOUND", True, type=bool))
        self.ui.ch_kits.setChecked(settings.value("PluginDatabase/ShowKits", True, type=bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/ShowNative", True, type=bool))
        self.ui.ch_bridged.setChecked(settings.value("PluginDatabase/ShowBridged", True, type=bool))
        self.ui.ch_bridged_wine.setChecked(settings.value("PluginDatabase/ShowBridgedWine", True, type=bool))
        self.ui.ch_rtsafe.setChecked(settings.value("PluginDatabase/ShowRtSafe", False, type=bool))
        self.ui.ch_gui.setChecked(settings.value("PluginDatabase/ShowHasGUI", False, type=bool))
        self.ui.ch_stereo.setChecked(settings.value("PluginDatabase/ShowStereoOnly", False, type=bool))

        self._showFilters(settings.value("PluginDatabase/ShowFilters", False, type=bool))
        self._reAddPlugins()

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        settings.setValue("PluginDatabase/TableGeometry", self.ui.tableWidget.horizontalHeader().saveState())
        settings.setValue("PluginDatabase/ShowFilters", (self.ui.tb_filters.arrowType() == Qt.UpArrow))
        settings.setValue("PluginDatabase/ShowEffects", self.ui.ch_effects.isChecked())
        settings.setValue("PluginDatabase/ShowInstruments", self.ui.ch_instruments.isChecked())
        settings.setValue("PluginDatabase/ShowMIDI", self.ui.ch_midi.isChecked())
        settings.setValue("PluginDatabase/ShowOther", self.ui.ch_other.isChecked())
        settings.setValue("PluginDatabase/ShowInternal", self.ui.ch_internal.isChecked())
        settings.setValue("PluginDatabase/ShowLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/ShowDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/ShowLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/ShowVST", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/ShowAU", self.ui.ch_au.isChecked())
        settings.setValue("PluginDatabase/ShowCSOUND", self.ui.ch_csound.isChecked())
        settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits.isChecked())
        settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged.isChecked())
        settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine.isChecked())
        settings.setValue("PluginDatabase/ShowRtSafe", self.ui.ch_rtsafe.isChecked())
        settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui.isChecked())
        settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo.isChecked())

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# TESTING

#Carla.isControl = True

#from PyQt5.QtWidgets import QApplication
#app = QApplication(sys.argv)
#gui = PluginDatabaseW(None)
#gui.show()
#app.exec_()
