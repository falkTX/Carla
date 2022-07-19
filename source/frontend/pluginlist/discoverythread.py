#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
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

from PyQt5.QtCore import pyqtSignal, QThread
from PyQt5.QtWidgets import QWidget

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import (
    PLUGIN_AU,
    PLUGIN_DSSI,
    PLUGIN_JSFX,
    PLUGIN_LADSPA,
    PLUGIN_LV2,
    PLUGIN_SFZ,
    PLUGIN_VST2,
)

from carla_shared import (
    CARLA_DEFAULT_DSSI_PATH,
    CARLA_DEFAULT_JSFX_PATH,
    CARLA_DEFAULT_LADSPA_PATH,
    CARLA_DEFAULT_LV2_PATH,
    CARLA_DEFAULT_SF2_PATH,
    CARLA_DEFAULT_SFZ_PATH,
    CARLA_DEFAULT_VST2_PATH,
    CARLA_DEFAULT_VST3_PATH,
    CARLA_DEFAULT_WINE_AUTO_PREFIX,
    CARLA_DEFAULT_WINE_EXECUTABLE,
    CARLA_DEFAULT_WINE_FALLBACK_PREFIX,
    CARLA_KEY_PATHS_DSSI,
    CARLA_KEY_PATHS_JSFX,
    CARLA_KEY_PATHS_LADSPA,
    CARLA_KEY_PATHS_LV2,
    CARLA_KEY_PATHS_SF2,
    CARLA_KEY_PATHS_SFZ,
    CARLA_KEY_PATHS_VST2,
    CARLA_KEY_PATHS_VST3,
    CARLA_KEY_WINE_AUTO_PREFIX,
    CARLA_KEY_WINE_EXECUTABLE,
    CARLA_KEY_WINE_FALLBACK_PREFIX,
    HAIKU,
    LINUX,
    MACOS,
    WINDOWS,
    gCarla,
    splitter,
)

from utils import QSafeSettings

from .discovery import (
    checkAllPluginsAU,
    checkFileSF2,
    checkPluginCached,
    checkPluginDSSI,
    checkPluginLADSPA,
    checkPluginVST2,
    checkPluginVST3,
    findBinaries,
    findFilenames,
    findMacVSTBundles,
    findVST3Binaries
)

# ---------------------------------------------------------------------------------------------------------------------
# Separate Thread for Plugin Search

class SearchPluginsThread(QThread):
    pluginLook = pyqtSignal(float, str)

    def __init__(self, parent: QWidget, pathBinaries: str):
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
        self.fCheckLV2    = False
        self.fCheckVST2   = False
        self.fCheckVST3   = False
        self.fCheckAU     = False
        self.fCheckSF2    = False
        self.fCheckSFZ    = False
        self.fCheckJSFX   = False

        if WINDOWS:
            toolNative = "carla-discovery-native.exe"
            self.fWineSettings = None

        else:
            toolNative = "carla-discovery-native"
            settings = QSafeSettings("falkTX", "Carla2")
            self.fWineSettings = {
                'executable'    : settings.value(CARLA_KEY_WINE_EXECUTABLE,
                                                 CARLA_DEFAULT_WINE_EXECUTABLE, str),
                'autoPrefix'    : settings.value(CARLA_KEY_WINE_AUTO_PREFIX,
                                                 CARLA_DEFAULT_WINE_AUTO_PREFIX, bool),
                'fallbackPrefix': settings.value(CARLA_KEY_WINE_FALLBACK_PREFIX,
                                                 CARLA_DEFAULT_WINE_FALLBACK_PREFIX, str),
            }
            del settings

        self.fToolNative = os.path.join(pathBinaries, toolNative)

        if not os.path.exists(self.fToolNative):
            self.fToolNative = ""

        self.fCurCount = 0
        self.fCurPercentValue = 0
        self.fLastCheckValue  = 0
        self.fSomethingChanged = False

    # -----------------------------------------------------------------------------------------------------------------
    # public methods

    def hasSomethingChanged(self):
        return self.fSomethingChanged

    def setSearchBinaryTypes(self, native: bool, posix32: bool, posix64: bool, win32: bool, win64: bool):
        self.fCheckNative  = native
        self.fCheckPosix32 = posix32
        self.fCheckPosix64 = posix64
        self.fCheckWin32   = win32
        self.fCheckWin64   = win64

    def setSearchPluginTypes(self, ladspa: bool, dssi: bool, lv2: bool, vst2: bool, vst3: bool, au: bool,
                                   sf2: bool, sfz: bool, jsfx: bool):
        self.fCheckLADSPA = ladspa
        self.fCheckDSSI   = dssi
        self.fCheckLV2    = lv2
        self.fCheckVST2   = vst2
        self.fCheckVST3   = vst3 and (LINUX or MACOS or WINDOWS)
        self.fCheckAU     = au and MACOS
        self.fCheckSF2    = sf2
        self.fCheckSFZ    = sfz
        self.fCheckJSFX   = jsfx

    def stop(self):
        self.fContinueChecking = False

    # -----------------------------------------------------------------------------------------------------------------
    # protected reimplemented methods

    def run(self):
        settingsDB = QSafeSettings("falkTX", "CarlaPlugins5")

        self.fContinueChecking = True
        self.fCurCount = 0

        if self.fCheckNative and not self.fToolNative:
            self.fCheckNative = False

        # looking for plugins via external discovery
        pluginCount = 0

        if self.fCheckLADSPA:
            pluginCount += 1
        if self.fCheckDSSI:
            pluginCount += 1
        if self.fCheckVST2:
            pluginCount += 1
        if self.fCheckVST3:
            pluginCount += 1

        # Increase count by the number of externally discoverable plugin types
        if self.fCheckNative:
            self.fCurCount += pluginCount
            # Linux, MacOS and Windows are the only VST3 supported OSes
            if self.fCheckVST3 and not (LINUX or MACOS or WINDOWS):
                self.fCurCount -= 1

        if self.fCheckPosix32:
            self.fCurCount += pluginCount
            if self.fCheckVST3 and not (LINUX or MACOS):
                self.fCurCount -= 1

        if self.fCheckPosix64:
            self.fCurCount += pluginCount
            if self.fCheckVST3 and not (LINUX or MACOS):
                self.fCurCount -= 1

        if self.fCheckWin32:
            self.fCurCount += pluginCount

        if self.fCheckWin64:
            self.fCurCount += pluginCount

        if self.fCheckLV2:
            if self.fCheckNative:
                self.fCurCount += 1
            else:
                self.fCheckLV2 = False

        if self.fCheckAU:
            if self.fCheckNative or self.fCheckPosix32:
                self.fCurCount += int(self.fCheckNative) + int(self.fCheckPosix32)
            else:
                self.fCheckAU = False

        if self.fCheckSF2:
            if self.fCheckNative:
                self.fCurCount += 1
            else:
                self.fCheckSF2 = False

        if self.fCheckSFZ:
            if self.fCheckNative:
                self.fCurCount += 1
            else:
                self.fCheckSFZ = False

        if self.fCheckJSFX:
            if self.fCheckNative:
                self.fCurCount += 1
            else:
                self.fCheckJSFX = False

        if self.fCurCount == 0:
            return

        self.fCurPercentValue = 100.0 / self.fCurCount
        self.fLastCheckValue  = 0.0

        del pluginCount

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

        if not self.fContinueChecking:
            return

        self.fSomethingChanged = True

        if self.fCheckLADSPA:
            if self.fCheckNative:
                plugins = self._checkLADSPA(OS, self.fToolNative)
                settingsDB.setValue("Plugins/LADSPA_native", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix32:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-posix32")
                plugins = self._checkLADSPA(OS, tool)
                settingsDB.setValue("Plugins/LADSPA_posix32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix64:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-posix64")
                plugins = self._checkLADSPA(OS, tool)
                settingsDB.setValue("Plugins/LADSPA_posix64", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin32:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win32.exe")
                plugins = self._checkLADSPA("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin64:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win64.exe")
                plugins = self._checkLADSPA("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/LADSPA_win64", plugins)

            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckDSSI:
            if self.fCheckNative:
                plugins = self._checkDSSI(OS, self.fToolNative)
                settingsDB.setValue("Plugins/DSSI_native", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix32:
                plugins = self._checkDSSI(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/DSSI_posix32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix64:
                plugins = self._checkDSSI(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/DSSI_posix64", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin32:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win32.exe")
                plugins = self._checkDSSI("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin64:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win64.exe")
                plugins = self._checkDSSI("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/DSSI_win64", plugins)

            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckLV2:
            plugins = self._checkCached(True)
            settingsDB.setValue("Plugins/LV2", plugins)
            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckVST2:
            if self.fCheckNative:
                plugins = self._checkVST2(OS, self.fToolNative)
                settingsDB.setValue("Plugins/VST2_native", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix32:
                plugins = self._checkVST2(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/VST2_posix32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix64:
                plugins = self._checkVST2(OS, os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/VST2_posix64", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin32:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win32.exe")
                plugins = self._checkVST2("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/VST2_win32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin64:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win64.exe")
                plugins = self._checkVST2("WINDOWS", tool, not WINDOWS)
                settingsDB.setValue("Plugins/VST2_win64", plugins)
                if not self.fContinueChecking:
                    return

            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckVST3:
            if self.fCheckNative and (LINUX or MACOS or WINDOWS):
                plugins = self._checkVST3(self.fToolNative)
                settingsDB.setValue("Plugins/VST3_native", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix32:
                plugins = self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/VST3_posix32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix64:
                plugins = self._checkVST3(os.path.join(self.fPathBinaries, "carla-discovery-posix64"))
                settingsDB.setValue("Plugins/VST3_posix64", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin32:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win32.exe")
                plugins = self._checkVST3(tool, not WINDOWS)
                settingsDB.setValue("Plugins/VST3_win32", plugins)
                if not self.fContinueChecking:
                    return

            if self.fCheckWin64:
                tool = os.path.join(self.fPathBinaries, "carla-discovery-win64.exe")
                plugins = self._checkVST3(tool, not WINDOWS)
                settingsDB.setValue("Plugins/VST3_win64", plugins)
                if not self.fContinueChecking:
                    return

            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckAU:
            if self.fCheckNative:
                plugins = self._checkCached(False)
                settingsDB.setValue("Plugins/AU", plugins)
                settingsDB.sync()
                if not self.fContinueChecking:
                    return

            if self.fCheckPosix32:
                plugins = self._checkAU(os.path.join(self.fPathBinaries, "carla-discovery-posix32"))
                settingsDB.setValue("Plugins/AU_posix32", self.fAuPlugins)
                if not self.fContinueChecking:
                    return

            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckSF2:
            settings = QSafeSettings("falkTX", "Carla2")
            SF2_PATH = settings.value(CARLA_KEY_PATHS_SF2, CARLA_DEFAULT_SF2_PATH, list)
            del settings

            kits = self._checkKIT(SF2_PATH, "sf2")
            settingsDB.setValue("Plugins/SF2", kits)
            settingsDB.sync()
            if not self.fContinueChecking:
                return

        if self.fCheckSFZ:
            kits = self._checkSfzCached()
            settingsDB.setValue("Plugins/SFZ", kits)
            settingsDB.sync()

        if self.fCheckJSFX:
            kits = self._checkJsfxCached()
            settingsDB.setValue("Plugins/JSFX", kits)
            settingsDB.sync()

    # -----------------------------------------------------------------------------------------------------------------
    # private methods

    def _checkLADSPA(self, OS, tool, isWine=False):
        ladspaBinaries = []
        ladspaPlugins = []

        self._pluginLook(self.fLastCheckValue, "LADSPA plugins...")

        settings = QSafeSettings("falkTX", "Carla2")
        LADSPA_PATH = settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH, list)
        del settings

        for iPATH in LADSPA_PATH:
            binaries = findBinaries(iPATH, PLUGIN_LADSPA, OS)
            for binary in binaries:
                if binary not in ladspaBinaries:
                    ladspaBinaries.append(binary)

        ladspaBinaries.sort()

        if not self.fContinueChecking:
            return ladspaPlugins

        for i in range(len(ladspaBinaries)):
            ladspa  = ladspaBinaries[i]
            percent = ( float(i) / len(ladspaBinaries) ) * self.fCurPercentValue
            self._pluginLook((self.fLastCheckValue + percent) * 0.9, ladspa)

            plugins = checkPluginLADSPA(ladspa, tool, self.fWineSettings if isWine else None)
            if plugins:
                ladspaPlugins.append(plugins)

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return ladspaPlugins

    def _checkDSSI(self, OS, tool, isWine=False):
        dssiBinaries = []
        dssiPlugins = []

        self._pluginLook(self.fLastCheckValue, "DSSI plugins...")

        settings = QSafeSettings("falkTX", "Carla2")
        DSSI_PATH = settings.value(CARLA_KEY_PATHS_DSSI, CARLA_DEFAULT_DSSI_PATH, list)
        del settings

        for iPATH in DSSI_PATH:
            binaries = findBinaries(iPATH, PLUGIN_DSSI, OS)
            for binary in binaries:
                if binary not in dssiBinaries:
                    dssiBinaries.append(binary)

        dssiBinaries.sort()

        if not self.fContinueChecking:
            return dssiPlugins

        for i in range(len(dssiBinaries)):
            dssi    = dssiBinaries[i]
            percent = ( float(i) / len(dssiBinaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, dssi)

            plugins = checkPluginDSSI(dssi, tool, self.fWineSettings if isWine else None)
            if plugins:
                dssiPlugins.append(plugins)

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return dssiPlugins

    def _checkVST2(self, OS, tool, isWine=False):
        vst2Binaries = []
        vst2Plugins = []

        if MACOS and not isWine:
            self._pluginLook(self.fLastCheckValue, "VST2 bundles...")
        else:
            self._pluginLook(self.fLastCheckValue, "VST2 plugins...")

        settings = QSafeSettings("falkTX", "Carla2")
        VST2_PATH = settings.value(CARLA_KEY_PATHS_VST2, CARLA_DEFAULT_VST2_PATH, list)
        del settings

        for iPATH in VST2_PATH:
            if MACOS and not isWine:
                binaries = findMacVSTBundles(iPATH, False)
            else:
                binaries = findBinaries(iPATH, PLUGIN_VST2, OS)
            for binary in binaries:
                if binary not in vst2Binaries:
                    vst2Binaries.append(binary)

        vst2Binaries.sort()

        if not self.fContinueChecking:
            return vst2Plugins

        for i in range(len(vst2Binaries)):
            vst2    = vst2Binaries[i]
            percent = ( float(i) / len(vst2Binaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst2)

            plugins = checkPluginVST2(vst2, tool, self.fWineSettings if isWine else None)
            if plugins:
                vst2Plugins.append(plugins)

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return vst2Plugins

    def _checkVST3(self, tool, isWine=False):
        vst3Binaries = []
        vst3Plugins = []

        if MACOS and not isWine:
            self._pluginLook(self.fLastCheckValue, "VST3 bundles...")
        else:
            self._pluginLook(self.fLastCheckValue, "VST3 plugins...")

        settings = QSafeSettings("falkTX", "Carla2")
        VST3_PATH = settings.value(CARLA_KEY_PATHS_VST3, CARLA_DEFAULT_VST3_PATH, list)
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

        if not self.fContinueChecking:
            return vst3Plugins

        for i in range(len(vst3Binaries)):
            vst3    = vst3Binaries[i]
            percent = ( float(i) / len(vst3Binaries) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, vst3)

            plugins = checkPluginVST3(vst3, tool, self.fWineSettings if isWine else None)
            if plugins:
                vst3Plugins.append(plugins)

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return vst3Plugins

    def _checkAU(self, tool):
        auPlugins = []

        plugins = checkAllPluginsAU(tool)
        if plugins:
            auPlugins.append(plugins)

        self.fLastCheckValue += self.fCurPercentValue
        return auPlugins

    def _checkKIT(self, kitPATH, kitExtension):
        kitFiles = []
        kitPlugins = []

        for iPATH in kitPATH:
            files = findFilenames(iPATH, kitExtension)
            for file_ in files:
                if file_ not in kitFiles:
                    kitFiles.append(file_)

        kitFiles.sort()

        if not self.fContinueChecking:
            return kitPlugins

        for i in range(len(kitFiles)):
            kit     = kitFiles[i]
            percent = ( float(i) / len(kitFiles) ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, kit)

            if kitExtension == "sf2":
                plugins = checkFileSF2(kit, self.fToolNative)
            else:
                plugins = None

            if plugins:
                kitPlugins.append(plugins)

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return kitPlugins

    def _checkCached(self, isLV2):
        if isLV2:
            settings  = QSafeSettings("falkTX", "Carla2")
            PLUG_PATH = splitter.join(settings.value(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH, list))
            del settings
            PLUG_TEXT = "LV2"
            PLUG_TYPE = PLUGIN_LV2
        else: # AU
            PLUG_PATH = ""
            PLUG_TEXT = "AU"
            PLUG_TYPE = PLUGIN_AU

        plugins = []
        self._pluginLook(self.fLastCheckValue, f"{PLUG_TEXT} plugins...")

        if not isLV2:
            gCarla.utils.juce_init()

        count = gCarla.utils.get_cached_plugin_count(PLUG_TYPE, PLUG_PATH)

        if not self.fContinueChecking:
            return plugins

        for i in range(count):
            descInfo = gCarla.utils.get_cached_plugin_info(PLUG_TYPE, i)

            percent = ( float(i) / count ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, descInfo['label'])

            if not descInfo['valid']:
                continue

            plugins.append(checkPluginCached(descInfo, PLUG_TYPE))

            if not self.fContinueChecking:
                break

        if not isLV2:
            gCarla.utils.juce_cleanup()

        self.fLastCheckValue += self.fCurPercentValue
        return plugins

    def _checkSfzCached(self):
        settings  = QSafeSettings("falkTX", "Carla2")
        PLUG_PATH = splitter.join(settings.value(CARLA_KEY_PATHS_SFZ, CARLA_DEFAULT_SFZ_PATH, list))
        del settings

        sfzKits = []
        self._pluginLook(self.fLastCheckValue, "SFZ kits...")

        count = gCarla.utils.get_cached_plugin_count(PLUGIN_SFZ, PLUG_PATH)

        if not self.fContinueChecking:
            return sfzKits

        for i in range(count):
            descInfo = gCarla.utils.get_cached_plugin_info(PLUGIN_SFZ, i)

            percent = ( float(i) / count ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, descInfo['label'])

            if not descInfo['valid']:
                continue

            sfzKits.append(checkPluginCached(descInfo, PLUGIN_SFZ))

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return sfzKits

    def _checkJsfxCached(self):
        settings  = QSafeSettings("falkTX", "Carla2")
        PLUG_PATH = splitter.join(settings.value(CARLA_KEY_PATHS_JSFX, CARLA_DEFAULT_JSFX_PATH, list))
        del settings

        jsfxPlugins = []
        self._pluginLook(self.fLastCheckValue, "JSFX plugins...")

        count = gCarla.utils.get_cached_plugin_count(PLUGIN_JSFX, PLUG_PATH)

        if not self.fContinueChecking:
            return jsfxPlugins

        for i in range(count):
            descInfo = gCarla.utils.get_cached_plugin_info(PLUGIN_JSFX, i)

            percent = ( float(i) / count ) * self.fCurPercentValue
            self._pluginLook(self.fLastCheckValue + percent, descInfo['label'])

            if not descInfo['valid']:
                continue

            jsfxPlugins.append(checkPluginCached(descInfo, PLUGIN_JSFX))

            if not self.fContinueChecking:
                break

        self.fLastCheckValue += self.fCurPercentValue
        return jsfxPlugins

    def _pluginLook(self, percent, plugin):
        self.pluginLook.emit(percent, plugin)

# ---------------------------------------------------------------------------------------------------------------------
