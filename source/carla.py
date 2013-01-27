#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
import json
from PyQt4.QtCore import QThread
from PyQt4.QtGui import QApplication, QMainWindow, QTableWidgetItem

# Imports (Custom Stuff)
import ui_carla, ui_carla_database, ui_carla_refresh
from carla_backend import *
from shared_settings import *

# set defaults
DEFAULT_PROJECT_FOLDER = HOME
setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)
setDefaultPluginsPaths(LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH)

# Separate Thread for Plugin Search
class SearchPluginsThread(QThread):
    def __init__(self, parent):
        QThread.__init__(self, parent)

        self.settings_db = self.parent().settings_db

        self.check_native  = False
        self.check_posix32 = False
        self.check_posix64 = False
        self.check_win32   = False
        self.check_win64   = False

        self.check_ladspa = False
        self.check_dssi  = False
        self.check_lv2 = False
        self.check_vst = False
        self.check_gig = False
        self.check_sf2 = False
        self.check_sfz = False

        self.tool_native = carla_discovery_native

    def skipPlugin(self):
        # TODO - windows and mac support
        apps  = ""
        apps += " carla-discovery"
        apps += " carla-discovery-native"
        apps += " carla-discovery-posix32"
        apps += " carla-discovery-posix64"
        apps += " carla-discovery-win32.exe"
        apps += " carla-discovery-win64.exe"

        if LINUX:
            os.system("killall -KILL %s" % apps)

    def pluginLook(self, percent, plugin):
        self.emit(SIGNAL("PluginLook(int, QString)"), percent, plugin)

    def setSearchBinaryTypes(self, native, posix32, posix64, win32, win64):
        self.check_native  = native
        self.check_posix32 = posix32
        self.check_posix64 = posix64
        self.check_win32   = win32
        self.check_win64   = win64

    def setSearchPluginTypes(self, ladspa, dssi, lv2, vst, gig, sf2, sfz):
        self.check_ladspa = ladspa
        self.check_dssi = dssi
        self.check_lv2 = lv2
        self.check_vst = vst
        self.check_gig = gig
        self.check_sf2 = sf2
        self.check_sfz = sfz

    def setSearchToolNative(self, tool):
        self.tool_native = tool

    def setLastLoadedBinary(self, binary):
        self.settings_db.setValue("Plugins/LastLoadedBinary", binary)

    def checkLADSPA(self, OS, tool, isWine=False):
        global LADSPA_PATH
        ladspa_binaries = []
        self.ladspa_plugins = []

        for iPATH in LADSPA_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in ladspa_binaries:
                    ladspa_binaries.append(binary)

        ladspa_binaries.sort()

        for i in range(len(ladspa_binaries)):
            ladspa = ladspa_binaries[i]
            if os.path.basename(ladspa) in self.blacklist:
                print("plugin %s is blacklisted, skip it" % ladspa)
                continue
            else:
                percent = ( float(i) / len(ladspa_binaries) ) * self.m_percent_value
                self.pluginLook((self.m_last_value + percent) * 0.9, ladspa)
                self.setLastLoadedBinary(ladspa)

                plugins = checkPluginLADSPA(ladspa, tool, isWine)
                if plugins:
                    self.ladspa_plugins.append(plugins)

        self.m_last_value += self.m_percent_value
        self.setLastLoadedBinary("")

    def checkDSSI(self, OS, tool, isWine=False):
        global DSSI_PATH
        dssi_binaries = []
        self.dssi_plugins = []

        for iPATH in DSSI_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in dssi_binaries:
                    dssi_binaries.append(binary)

        dssi_binaries.sort()

        for i in range(len(dssi_binaries)):
            dssi = dssi_binaries[i]
            if os.path.basename(dssi) in self.blacklist:
                print("plugin %s is blacklisted, skip it" % dssi)
                continue
            else:
                percent = ( float(i) / len(dssi_binaries) ) * self.m_percent_value
                self.pluginLook(self.m_last_value + percent, dssi)
                self.setLastLoadedBinary(dssi)

                plugins = checkPluginDSSI(dssi, tool, isWine)
                if plugins:
                    self.dssi_plugins.append(plugins)

        self.m_last_value += self.m_percent_value
        self.setLastLoadedBinary("")

    def checkLV2(self, tool, isWine=False):
        global LV2_PATH
        lv2_bundles = []
        self.lv2_plugins = []

        self.pluginLook(self.m_last_value, "LV2 bundles...")

        for iPATH in LV2_PATH:
            bundles = findLV2Bundles(iPATH)
            for bundle in bundles:
                if bundle not in lv2_bundles:
                    lv2_bundles.append(bundle)

        lv2_bundles.sort()

        for i in range(len(lv2_bundles)):
            lv2 = lv2_bundles[i]
            if (os.path.basename(lv2) in self.blacklist):
                print("bundle %s is blacklisted, skip it" % lv2)
                continue
            else:
                percent = ( float(i) / len(lv2_bundles) ) * self.m_percent_value
                self.pluginLook(self.m_last_value + percent, lv2)
                self.setLastLoadedBinary(lv2)

                plugins = checkPluginLV2(lv2, tool, isWine)
                if plugins:
                    self.lv2_plugins.append(plugins)

        self.m_last_value += self.m_percent_value
        self.setLastLoadedBinary("")

    def checkVST(self, OS, tool, isWine=False):
        global VST_PATH
        vst_binaries = []
        self.vst_plugins = []

        for iPATH in VST_PATH:
            binaries = findBinaries(iPATH, OS)
            for binary in binaries:
                if binary not in vst_binaries:
                    vst_binaries.append(binary)

        vst_binaries.sort()

        for i in range(len(vst_binaries)):
            vst = vst_binaries[i]
            if os.path.basename(vst) in self.blacklist:
                print("plugin %s is blacklisted, skip it" % vst)
                continue
            else:
                percent = ( float(i) / len(vst_binaries) ) * self.m_percent_value
                self.pluginLook(self.m_last_value + percent, vst)
                self.setLastLoadedBinary(vst)

                plugins = checkPluginVST(vst, tool, isWine)
                if plugins:
                    self.vst_plugins.append(plugins)

        self.m_last_value += self.m_percent_value
        self.setLastLoadedBinary("")

    def checkKIT(self, kPATH, kType):
        kit_files = []
        self.kit_plugins = []

        for iPATH in kPATH:
            files = findSoundKits(iPATH, kType)
            for file_ in files:
                if file_ not in kit_files:
                    kit_files.append(file_)

        kit_files.sort()

        for i in range(len(kit_files)):
            kit = kit_files[i]
            if os.path.basename(kit) in self.blacklist:
                print("plugin %s is blacklisted, skip it" % kit)
                continue
            else:
                percent = ( float(i) / len(kit_files) ) * self.m_percent_value
                self.pluginLook(self.m_last_value + percent, kit)
                self.setLastLoadedBinary(kit)

                if kType == "gig":
                    plugins = checkPluginGIG(kit, self.tool_native)
                elif kType == "sf2":
                    plugins = checkPluginSF2(kit, self.tool_native)
                elif kType == "sfz":
                    plugins = checkPluginSFZ(kit, self.tool_native)
                else:
                    plugins = None

                if plugins:
                    self.kit_plugins.append(plugins)

        self.m_last_value += self.m_percent_value
        self.setLastLoadedBinary("")

    def run(self):
        global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH

        self.blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))

        self.m_count = 0
        plugin_count = 0

        if self.check_ladspa: plugin_count += 1
        if self.check_dssi:   plugin_count += 1
        if self.check_lv2:    plugin_count += 1
        if self.check_vst:    plugin_count += 1

        if self.check_native:
            self.m_count += plugin_count
        if self.check_posix32:
            self.m_count += plugin_count
        if self.check_posix64:
            self.m_count += plugin_count
        if self.check_win32:
            self.m_count += plugin_count
        if self.check_win64:
            self.m_count += plugin_count

        if self.tool_native:
            if self.check_gig: self.m_count += 1
            if self.check_sf2: self.m_count += 1
            if self.check_sfz: self.m_count += 1
        else:
            self.check_gig = False
            self.check_sf2 = False
            self.check_sfz = False

        if self.m_count == 0:
            return

        self.m_last_value = 0
        self.m_percent_value = 100 / self.m_count

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

        if self.check_ladspa:
            m_value = 0
            if haveLRDF:
                if self.check_native:  m_value += 0.1
                if self.check_posix32: m_value += 0.1
                if self.check_posix64: m_value += 0.1
                if self.check_win32:   m_value += 0.1
                if self.check_win64:   m_value += 0.1
            rdf_pad_value = self.m_percent_value * m_value

            if self.check_native:
                self.checkLADSPA(OS, carla_discovery_native)
                self.settings_db.setValue("Plugins/LADSPA_native", self.ladspa_plugins)
                self.settings_db.sync()

            if self.check_posix32:
                self.checkLADSPA(OS, carla_discovery_posix32)
                self.settings_db.setValue("Plugins/LADSPA_posix32", self.ladspa_plugins)
                self.settings_db.sync()

            if self.check_posix64:
                self.checkLADSPA(OS, carla_discovery_posix64)
                self.settings_db.setValue("Plugins/LADSPA_posix64", self.ladspa_plugins)
                self.settings_db.sync()

            if self.check_win32:
                self.checkLADSPA("WINDOWS", carla_discovery_win32, not WINDOWS)
                self.settings_db.setValue("Plugins/LADSPA_win32", self.ladspa_plugins)
                self.settings_db.sync()

            if self.check_win64:
                self.checkLADSPA("WINDOWS", carla_discovery_win64, not WINDOWS)
                self.settings_db.setValue("Plugins/LADSPA_win64", self.ladspa_plugins)
                self.settings_db.sync()

            if haveLRDF:
                if m_value > 0:
                    start_value = self.m_last_value - rdf_pad_value

                    self.pluginLook(start_value, "LADSPA RDFs...")
                    ladspa_rdf_info = ladspa_rdf.recheck_all_plugins(self, start_value, self.m_percent_value, m_value)

                    SettingsDir = os.path.join(HOME, ".config", "Cadence")

                    f_ladspa = open(os.path.join(SettingsDir, "ladspa_rdf.db"), 'w')
                    json.dump(ladspa_rdf_info, f_ladspa)
                    f_ladspa.close()

        if self.check_dssi:
            if self.check_native:
                self.checkDSSI(OS, carla_discovery_native)
                self.settings_db.setValue("Plugins/DSSI_native", self.dssi_plugins)
                self.settings_db.sync()

            if self.check_posix32:
                self.checkDSSI(OS, carla_discovery_posix32)
                self.settings_db.setValue("Plugins/DSSI_posix32", self.dssi_plugins)
                self.settings_db.sync()

            if self.check_posix64:
                self.checkDSSI(OS, carla_discovery_posix64)
                self.settings_db.setValue("Plugins/DSSI_posix64", self.dssi_plugins)
                self.settings_db.sync()

            if self.check_win32:
                self.checkDSSI("WINDOWS", carla_discovery_win32, not WINDOWS)
                self.settings_db.setValue("Plugins/DSSI_win32", self.dssi_plugins)
                self.settings_db.sync()

            if self.check_win64:
                self.checkDSSI("WINDOWS", carla_discovery_win64, not WINDOWS)
                self.settings_db.setValue("Plugins/DSSI_win64", self.dssi_plugins)
                self.settings_db.sync()

        if self.check_lv2:
            if self.check_native:
                self.checkLV2(carla_discovery_native)
                self.settings_db.setValue("Plugins/LV2_native", self.lv2_plugins)
                self.settings_db.sync()

            if self.check_posix32:
                self.checkLV2(carla_discovery_posix32)
                self.settings_db.setValue("Plugins/LV2_posix32", self.lv2_plugins)
                self.settings_db.sync()

            if self.check_posix64:
                self.checkLV2(carla_discovery_posix64)
                self.settings_db.setValue("Plugins/LV2_posix64", self.lv2_plugins)
                self.settings_db.sync()

            if self.check_win32:
                self.checkLV2(carla_discovery_win32, not WINDOWS)
                self.settings_db.setValue("Plugins/LV2_win32", self.lv2_plugins)
                self.settings_db.sync()

            if self.check_win64:
                self.checkLV2(carla_discovery_win64, not WINDOWS)
                self.settings_db.setValue("Plugins/LV2_win64", self.lv2_plugins)
                self.settings_db.sync()

        if self.check_vst:
            if self.check_native:
                self.checkVST(OS, carla_discovery_native)
                self.settings_db.setValue("Plugins/VST_native", self.vst_plugins)
                self.settings_db.sync()

            if self.check_posix32:
                self.checkVST(OS, carla_discovery_posix32)
                self.settings_db.setValue("Plugins/VST_posix32", self.vst_plugins)
                self.settings_db.sync()

            if self.check_posix64:
                self.checkVST(OS, carla_discovery_posix64)
                self.settings_db.setValue("Plugins/VST_posix64", self.vst_plugins)
                self.settings_db.sync()

            if self.check_win32:
                self.checkVST("WINDOWS", carla_discovery_win32, not WINDOWS)
                self.settings_db.setValue("Plugins/VST_win32", self.vst_plugins)
                self.settings_db.sync()

            if self.check_win64:
                self.checkVST("WINDOWS", carla_discovery_win64, not WINDOWS)
                self.settings_db.setValue("Plugins/VST_win64", self.vst_plugins)
                self.settings_db.sync()

        if self.check_gig:
            self.checkKIT(GIG_PATH, "gig")
            self.settings_db.setValue("Plugins/GIG", self.kit_plugins)
            self.settings_db.sync()

        if self.check_sf2:
            self.checkKIT(SF2_PATH, "sf2")
            self.settings_db.setValue("Plugins/SF2", self.kit_plugins)
            self.settings_db.sync()

        if self.check_sfz:
            self.checkKIT(SFZ_PATH, "sfz")
            self.settings_db.setValue("Plugins/SFZ", self.kit_plugins)
            self.settings_db.sync()

# Plugin Refresh Dialog
class PluginRefreshW(QDialog, ui_carla_refresh.Ui_PluginRefreshW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.b_skip.setVisible(False)

        if HAIKU:
            self.ch_posix32.setText("Haiku 32bit")
            self.ch_posix64.setText("Haiku 64bit")
        elif LINUX:
            self.ch_posix32.setText("Linux 32bit")
            self.ch_posix64.setText("Linux 64bit")
        elif MACOS:
            self.ch_posix32.setText("MacOS 32bit")
            self.ch_posix64.setText("MacOS 64bit")

        self.settings = self.parent().settings
        self.settings_db = self.parent().settings_db
        self.loadSettings()

        self.pThread = SearchPluginsThread(self)

        if carla_discovery_posix32 and not WINDOWS:
            self.ico_posix32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_posix32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_posix32.setChecked(False)
            self.ch_posix32.setEnabled(False)

        if carla_discovery_posix64 and not WINDOWS:
            self.ico_posix64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_posix64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_posix64.setChecked(False)
            self.ch_posix64.setEnabled(False)

        if carla_discovery_win32:
            self.ico_win32.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_win32.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_win32.setChecked(False)
            self.ch_win32.setEnabled(False)

        if carla_discovery_win64:
            self.ico_win64.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_win64.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_win64.setChecked(False)
            self.ch_win64.setEnabled(False)

        if haveLRDF:
            self.ico_rdflib.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_rdflib.setPixmap(getIcon("dialog-error").pixmap(16, 16))

        hasNative = bool(carla_discovery_native)
        hasNonNative = False

        if WINDOWS:
            if is64bit:
                hasNative = bool(carla_discovery_win64)
                hasNonNative = bool(carla_discovery_win32)
                self.pThread.setSearchToolNative(carla_discovery_win64)
                self.ch_win64.setChecked(False)
                self.ch_win64.setVisible(False)
                self.ico_win64.setVisible(False)
                self.label_win64.setVisible(False)
            else:
                hasNative = bool(carla_discovery_win32)
                hasNonNative = bool(carla_discovery_win64)
                self.pThread.setSearchToolNative(carla_discovery_win32)
                self.ch_win32.setChecked(False)
                self.ch_win32.setVisible(False)
                self.ico_win32.setVisible(False)
                self.label_win32.setVisible(False)
        elif LINUX or MACOS:
            if is64bit:
                hasNonNative = bool(carla_discovery_posix32 or carla_discovery_win32 or carla_discovery_win64)
                self.ch_posix64.setChecked(False)
                self.ch_posix64.setVisible(False)
                self.ico_posix64.setVisible(False)
                self.label_posix64.setVisible(False)
            else:
                hasNonNative = bool(carla_discovery_posix64 or carla_discovery_win32 or carla_discovery_win64)
                self.ch_posix32.setChecked(False)
                self.ch_posix32.setVisible(False)
                self.ico_posix32.setVisible(False)
                self.label_posix32.setVisible(False)

        if hasNative:
            self.ico_native.setPixmap(getIcon("dialog-ok-apply").pixmap(16, 16))
        else:
            self.ico_native.setPixmap(getIcon("dialog-error").pixmap(16, 16))
            self.ch_native.setChecked(False)
            self.ch_native.setEnabled(False)
            self.ch_gig.setChecked(False)
            self.ch_gig.setEnabled(False)
            self.ch_sf2.setChecked(False)
            self.ch_sf2.setEnabled(False)
            self.ch_sfz.setChecked(False)
            self.ch_sfz.setEnabled(False)
            if not hasNonNative:
                self.ch_ladspa.setChecked(False)
                self.ch_ladspa.setEnabled(False)
                self.ch_dssi.setChecked(False)
                self.ch_dssi.setEnabled(False)
                self.ch_vst.setChecked(False)
                self.ch_vst.setEnabled(False)
                self.b_start.setEnabled(False)

        self.connect(self.b_start, SIGNAL("clicked()"), SLOT("slot_start()"))
        self.connect(self.b_skip, SIGNAL("clicked()"), SLOT("slot_skip()"))
        self.connect(self.pThread, SIGNAL("PluginLook(int, QString)"), SLOT("slot_handlePluginLook(int, QString)"))
        self.connect(self.pThread, SIGNAL("finished()"), SLOT("slot_handlePluginThreadFinished()"))

    @pyqtSlot()
    def slot_start(self):
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(100)
        self.progressBar.setValue(0)
        self.b_start.setEnabled(False)
        self.b_skip.setVisible(True)
        self.b_close.setVisible(False)

        native, posix32, posix64, win32, win64 = (self.ch_native.isChecked(), self.ch_posix32.isChecked(), self.ch_posix64.isChecked(), self.ch_win32.isChecked(), self.ch_win64.isChecked())
        ladspa, dssi, lv2, vst, gig, sf2, sfz  = (self.ch_ladspa.isChecked(), self.ch_dssi.isChecked(), self.ch_lv2.isChecked(), self.ch_vst.isChecked(),
                                                  self.ch_gig.isChecked(), self.ch_sf2.isChecked(), self.ch_sfz.isChecked())

        self.pThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
        self.pThread.setSearchPluginTypes(ladspa, dssi, lv2, vst, gig, sf2, sfz)
        self.pThread.start()

    @pyqtSlot()
    def slot_skip(self):
        self.pThread.skipPlugin()

    @pyqtSlot(int, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.progressBar.setFormat("%s" % plugin)
        self.progressBar.setValue(percent)

    @pyqtSlot()
    def slot_handlePluginThreadFinished(self):
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(1)
        self.progressBar.setValue(1)
        self.progressBar.setFormat(self.tr("Done"))
        self.b_start.setEnabled(True)
        self.b_skip.setVisible(False)
        self.b_close.setVisible(True)

    def saveSettings(self):
        self.settings.setValue("PluginDatabase/SearchLADSPA", self.ch_ladspa.isChecked())
        self.settings.setValue("PluginDatabase/SearchDSSI", self.ch_dssi.isChecked())
        self.settings.setValue("PluginDatabase/SearchLV2", self.ch_lv2.isChecked())
        self.settings.setValue("PluginDatabase/SearchVST", self.ch_vst.isChecked())
        self.settings.setValue("PluginDatabase/SearchGIG", self.ch_gig.isChecked())
        self.settings.setValue("PluginDatabase/SearchSF2", self.ch_sf2.isChecked())
        self.settings.setValue("PluginDatabase/SearchSFZ", self.ch_sfz.isChecked())
        self.settings.setValue("PluginDatabase/SearchNative", self.ch_native.isChecked())
        self.settings.setValue("PluginDatabase/SearchPOSIX32", self.ch_posix32.isChecked())
        self.settings.setValue("PluginDatabase/SearchPOSIX64", self.ch_posix64.isChecked())
        self.settings.setValue("PluginDatabase/SearchWin32", self.ch_win32.isChecked())
        self.settings.setValue("PluginDatabase/SearchWin64", self.ch_win64.isChecked())
        self.settings_db.setValue("Plugins/LastLoadedBinary", "")

    def loadSettings(self):
        self.ch_ladspa.setChecked(self.settings.value("PluginDatabase/SearchLADSPA", True, type=bool))
        self.ch_dssi.setChecked(self.settings.value("PluginDatabase/SearchDSSI", True, type=bool))
        self.ch_lv2.setChecked(self.settings.value("PluginDatabase/SearchLV2", True, type=bool))
        self.ch_vst.setChecked(self.settings.value("PluginDatabase/SearchVST", True, type=bool))
        self.ch_gig.setChecked(self.settings.value("PluginDatabase/SearchGIG", True, type=bool))
        self.ch_sf2.setChecked(self.settings.value("PluginDatabase/SearchSF2", True, type=bool))
        self.ch_sfz.setChecked(self.settings.value("PluginDatabase/SearchSFZ", True, type=bool))
        self.ch_native.setChecked(self.settings.value("PluginDatabase/SearchNative", True, type=bool))
        self.ch_posix32.setChecked(self.settings.value("PluginDatabase/SearchPOSIX32", False, type=bool))
        self.ch_posix64.setChecked(self.settings.value("PluginDatabase/SearchPOSIX64", False, type=bool))
        self.ch_win32.setChecked(self.settings.value("PluginDatabase/SearchWin32", False, type=bool))
        self.ch_win64.setChecked(self.settings.value("PluginDatabase/SearchWin64", False, type=bool))

    def closeEvent(self, event):
        if self.pThread.isRunning():
            self.pThread.terminate()
            self.pThread.wait()
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Plugin Database Dialog
class PluginDatabaseW(QDialog, ui_carla_database.Ui_PluginDatabaseW):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.m_showOldWarning = False

        self.settings = self.parent().settings
        self.settings_db = self.parent().settings_db
        self.loadSettings()

        self.b_add.setEnabled(False)

        if BINARY_NATIVE in (BINARY_POSIX32, BINARY_WIN32):
            self.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ch_bridged.setText(self.tr("Bridged (32bit)"))

        if not (LINUX or MACOS):
            self.ch_bridged_wine.setChecked(False)
            self.ch_bridged_wine.setEnabled(False)

        # Blacklist plugins
        if not self.settings_db.contains("Plugins/Blacklisted"):
            blacklist = [] # FIXME
            # Broken or useless plugins
            #blacklist.append("dssi-vst.so")
            blacklist.append("liteon_biquad-vst.so")
            blacklist.append("liteon_biquad-vst_64bit.so")
            blacklist.append("fx_blur-vst.so")
            blacklist.append("fx_blur-vst_64bit.so")
            blacklist.append("fx_tempodelay-vst.so")
            blacklist.append("Scrubby_64bit.so")
            blacklist.append("Skidder_64bit.so")
            blacklist.append("libwormhole2_64bit.so")
            blacklist.append("vexvst.so")
            #blacklist.append("deckadance.dll")
            self.settings_db.setValue("Plugins/Blacklisted", blacklist)

        self.connect(self.b_add, SIGNAL("clicked()"), SLOT("slot_add_plugin()"))
        self.connect(self.b_refresh, SIGNAL("clicked()"), SLOT("slot_refresh_plugins()"))
        self.connect(self.tb_filters, SIGNAL("clicked()"), SLOT("slot_maybe_show_filters()"))
        self.connect(self.tableWidget, SIGNAL("currentCellChanged(int, int, int, int)"), SLOT("slot_checkPlugin(int)"))
        self.connect(self.tableWidget, SIGNAL("cellDoubleClicked(int, int)"), SLOT("slot_add_plugin()"))

        self.connect(self.lineEdit, SIGNAL("textChanged(QString)"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_effects, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_instruments, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_midi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_other, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_kits, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_internal, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_ladspa, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_dssi, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_lv2, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_vst, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_native, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_bridged, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_bridged_wine, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_gui, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))
        self.connect(self.ch_stereo, SIGNAL("clicked()"), SLOT("slot_checkFilters()"))

        self.ret_plugin = None

        if self.m_showOldWarning:
            QTimer.singleShot(0, self, SLOT("slot_showOldWarning()"))

    def showFilters(self, yesno):
        self.tb_filters.setArrowType(Qt.UpArrow if yesno else Qt.DownArrow)
        self.frame.setVisible(yesno)

    def checkInternalPlugins(self):
        internals = toList(self.settings_db.value("Plugins/Internal", []))

        count = 0

        for plugins in internals:
            for x in plugins:
                count += 1

        if count != Carla.host.get_internal_plugin_count():
            internal_plugins = []

            for i in range(Carla.host.get_internal_plugin_count()):
                descInfo = Carla.host.get_internal_plugin_info(i)
                plugins  = checkPluginInternal(descInfo)

                if plugins:
                    internal_plugins.append(plugins)

            self.settings_db.setValue("Plugins/Internal", internal_plugins)
            self.settings_db.sync()

    def reAddPlugins(self):
        row_count = self.tableWidget.rowCount()
        for x in range(row_count):
            self.tableWidget.removeRow(0)

        self.last_table_index = 0
        self.tableWidget.setSortingEnabled(False)

        self.checkInternalPlugins()

        internals = toList(self.settings_db.value("Plugins/Internal", []))

        ladspa_plugins  = []
        ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_native", []))
        ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix32", []))
        ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix64", []))
        ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win32", []))
        ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win64", []))

        dssi_plugins  = []
        dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_native", []))
        dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix32", []))
        dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix64", []))
        dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win32", []))
        dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win64", []))

        lv2_plugins  = []
        lv2_plugins += toList(self.settings_db.value("Plugins/LV2_native", []))
        lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix32", []))
        lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix64", []))
        lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win32", []))
        lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win64", []))

        vst_plugins  = []
        vst_plugins += toList(self.settings_db.value("Plugins/VST_native", []))
        vst_plugins += toList(self.settings_db.value("Plugins/VST_posix32", []))
        vst_plugins += toList(self.settings_db.value("Plugins/VST_posix64", []))
        vst_plugins += toList(self.settings_db.value("Plugins/VST_win32", []))
        vst_plugins += toList(self.settings_db.value("Plugins/VST_win64", []))

        gigs = toList(self.settings_db.value("Plugins/GIG", []))
        sf2s = toList(self.settings_db.value("Plugins/SF2", []))
        sfzs = toList(self.settings_db.value("Plugins/SFZ", []))

        internal_count = 0
        ladspa_count = 0
        dssi_count = 0
        lv2_count = 0
        vst_count = 0
        kit_count = 0

        for plugins in internals:
            for plugin in plugins:
                self.addPluginToTable(plugin, self.tr("Internal"))
                internal_count += 1

        for plugins in ladspa_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "LADSPA")
                ladspa_count += 1

        for plugins in dssi_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "DSSI")
                dssi_count += 1

        for plugins in lv2_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "LV2")
                lv2_count += 1

        for plugins in vst_plugins:
            for plugin in plugins:
                self.addPluginToTable(plugin, "VST")
                vst_count += 1

        for gig in gigs:
            for gig_i in gig:
                self.addPluginToTable(gig_i, "GIG")
                kit_count += 1

        for sf2 in sf2s:
            for sf2_i in sf2:
                self.addPluginToTable(sf2_i, "SF2")
                kit_count += 1

        for sfz in sfzs:
            for sfz_i in sfz:
                self.addPluginToTable(sfz_i, "SFZ")
                kit_count += 1

        self.slot_checkFilters()
        self.tableWidget.setSortingEnabled(True)
        self.tableWidget.sortByColumn(0, Qt.AscendingOrder)

        self.label.setText(self.tr("Have %i %s, %i LADSPA, %i DSSI, %i LV2, %i VST and %i Sound Kits" % (internal_count, self.tr("Internal"), ladspa_count, dssi_count, lv2_count, vst_count, kit_count)))

    def addPluginToTable(self, plugin, ptype):
        index = self.last_table_index

        if self.m_showOldWarning or 'API' not in plugin.keys() or plugin['API'] < PLUGIN_QUERY_API_VERSION:
            self.m_showOldWarning = True
            return

        if plugin['build'] == BINARY_NATIVE:
            bridge_text = self.tr("No")
        else:
            type_text = self.tr("Unknown")
            if LINUX or MACOS:
                if plugin['build'] == BINARY_POSIX32:
                    type_text = "32bit"
                elif plugin['build'] == BINARY_POSIX64:
                    type_text = "64bit"
                elif plugin['build'] == BINARY_WIN32:
                    type_text = "Windows 32bit"
                elif plugin['build'] == BINARY_WIN64:
                    type_text = "Windows 64bit"
            elif WINDOWS:
                if plugin['build'] == BINARY_WIN32:
                    type_text = "32bit"
                elif plugin['build'] == BINARY_WIN64:
                    type_text = "64bit"
            bridge_text = self.tr("Yes (%s)" % type_text)

        self.tableWidget.insertRow(index)
        self.tableWidget.setItem(index, 0, QTableWidgetItem(plugin['name']))
        self.tableWidget.setItem(index, 1, QTableWidgetItem(plugin['label']))
        self.tableWidget.setItem(index, 2, QTableWidgetItem(plugin['maker']))
        self.tableWidget.setItem(index, 3, QTableWidgetItem(str(plugin['unique_id'])))
        self.tableWidget.setItem(index, 4, QTableWidgetItem(str(plugin['audio.ins'])))
        self.tableWidget.setItem(index, 5, QTableWidgetItem(str(plugin['audio.outs'])))
        self.tableWidget.setItem(index, 6, QTableWidgetItem(str(plugin['parameters.ins'])))
        self.tableWidget.setItem(index, 7, QTableWidgetItem(str(plugin['parameters.outs'])))
        self.tableWidget.setItem(index, 8, QTableWidgetItem(str(plugin['programs.total'])))
        self.tableWidget.setItem(index, 9, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_HAS_GUI) else self.tr("No")))
        self.tableWidget.setItem(index, 10, QTableWidgetItem(self.tr("Yes") if (plugin['hints'] & PLUGIN_IS_SYNTH) else self.tr("No")))
        self.tableWidget.setItem(index, 11, QTableWidgetItem(bridge_text))
        self.tableWidget.setItem(index, 12, QTableWidgetItem(ptype))
        self.tableWidget.setItem(index, 13, QTableWidgetItem(plugin['binary']))
        self.tableWidget.item(self.last_table_index, 0).plugin_data = plugin
        self.last_table_index += 1

    @pyqtSlot()
    def slot_add_plugin(self):
        if self.tableWidget.currentRow() >= 0:
            self.ret_plugin = self.tableWidget.item(self.tableWidget.currentRow(), 0).plugin_data
            self.accept()
        else:
            self.reject()

    @pyqtSlot()
    def slot_refresh_plugins(self):
        lastLoadedPlugin = self.settings_db.value("Plugins/LastLoadedBinary", "", type=str)
        if lastLoadedPlugin:
            lastLoadedPlugin = os.path.basename(lastLoadedPlugin)
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There was an error while checking the plugin %s.\n"
                                                                         "Do you want to blacklist it?" % lastLoadedPlugin),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if ask == QMessageBox.Yes:
                blacklist = toList(self.settings_db.value("Plugins/Blacklisted", []))
                blacklist.append(lastLoadedPlugin)
                self.settings_db.setValue("Plugins/Blacklisted", blacklist)

        #self.label.setText(self.tr("Looking for plugins..."))
        PluginRefreshW(self).exec_()

        self.reAddPlugins()
        self.parent().loadRDFs()

    @pyqtSlot()
    def slot_maybe_show_filters(self):
        self.showFilters(not self.frame.isVisible())

    @pyqtSlot(int)
    def slot_checkPlugin(self, row):
        self.b_add.setEnabled(row >= 0)

    @pyqtSlot()
    def slot_checkFilters(self):
        text = self.lineEdit.text().lower()

        hide_effects     = not self.ch_effects.isChecked()
        hide_instruments = not self.ch_instruments.isChecked()
        hide_midi        = not self.ch_midi.isChecked()
        hide_other       = not self.ch_other.isChecked()

        hide_internal = not self.ch_internal.isChecked()
        hide_ladspa   = not self.ch_ladspa.isChecked()
        hide_dssi     = not self.ch_dssi.isChecked()
        hide_lv2      = not self.ch_lv2.isChecked()
        hide_vst      = not self.ch_vst.isChecked()
        hide_kits     = not self.ch_kits.isChecked()

        hide_native  = not self.ch_native.isChecked()
        hide_bridged = not self.ch_bridged.isChecked()
        hide_bridged_wine = not self.ch_bridged_wine.isChecked()

        hide_non_gui    = self.ch_gui.isChecked()
        hide_non_stereo = self.ch_stereo.isChecked()

        if HAIKU or LINUX or MACOS:
            native_bins = [BINARY_POSIX32, BINARY_POSIX64]
            wine_bins = [BINARY_WIN32, BINARY_WIN64]
        elif WINDOWS:
            native_bins = [BINARY_WIN32, BINARY_WIN64]
            wine_bins = []
        else:
            native_bins = []
            wine_bins = []

        row_count = self.tableWidget.rowCount()
        for i in range(row_count):
            self.tableWidget.showRow(i)

            plugin = self.tableWidget.item(i, 0).plugin_data
            ains   = plugin['audio.ins']
            aouts  = plugin['audio.outs']
            mins   = plugin['midi.ins']
            mouts  = plugin['midi.outs']
            ptype  = self.tableWidget.item(i, 12).text()
            is_synth  = bool(plugin['hints'] & PLUGIN_IS_SYNTH)
            is_effect = bool(ains > 0 < aouts and not is_synth)
            is_midi   = bool(ains == 0 and aouts == 0 and mins > 0 < mouts)
            is_kit    = bool(ptype in ("GIG", "SF2", "SFZ"))
            is_other  = bool(not (is_effect or is_synth or is_midi or is_kit))
            is_native = bool(plugin['build'] == BINARY_NATIVE)
            is_stereo = bool(ains == 2 and aouts == 2) or (is_synth and aouts == 2)
            has_gui   = bool(plugin['hints'] & PLUGIN_HAS_GUI)

            is_bridged = bool(not is_native and plugin['build'] in native_bins)
            is_bridged_wine = bool(not is_native and plugin['build'] in wine_bins)

            if (hide_effects and is_effect):
                self.tableWidget.hideRow(i)
            elif (hide_instruments and is_synth):
                self.tableWidget.hideRow(i)
            elif (hide_midi and is_midi):
                self.tableWidget.hideRow(i)
            elif (hide_other and is_other):
                self.tableWidget.hideRow(i)
            elif (hide_kits and is_kit):
                self.tableWidget.hideRow(i)
            elif (hide_internal and ptype == self.tr("Internal")):
                self.tableWidget.hideRow(i)
            elif (hide_ladspa and ptype == "LADSPA"):
                self.tableWidget.hideRow(i)
            elif (hide_dssi and ptype == "DSSI"):
                self.tableWidget.hideRow(i)
            elif (hide_lv2 and ptype == "LV2"):
                self.tableWidget.hideRow(i)
            elif (hide_vst and ptype == "VST"):
                self.tableWidget.hideRow(i)
            elif (hide_native and is_native):
                self.tableWidget.hideRow(i)
            elif (hide_bridged and is_bridged):
                self.tableWidget.hideRow(i)
            elif (hide_bridged_wine and is_bridged_wine):
                self.tableWidget.hideRow(i)
            elif (hide_non_gui and not has_gui):
                self.tableWidget.hideRow(i)
            elif (hide_non_stereo and not is_stereo):
                self.tableWidget.hideRow(i)
            elif (text and not (
                text in self.tableWidget.item(i, 0).text().lower() or
                text in self.tableWidget.item(i, 1).text().lower() or
                text in self.tableWidget.item(i, 2).text().lower() or
                text in self.tableWidget.item(i, 3).text().lower() or
                text in self.tableWidget.item(i, 13).text().lower())):
                self.tableWidget.hideRow(i)

    @pyqtSlot()
    def slot_showOldWarning(self):
        QMessageBox.warning(self, self.tr("Warning"), self.tr("You're using a Carla-Database from an old version of Carla, please update *all* the plugins"))

    def saveSettings(self):
        self.settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        self.settings.setValue("PluginDatabase/TableGeometry", self.tableWidget.horizontalHeader().saveState())
        self.settings.setValue("PluginDatabase/ShowFilters", (self.tb_filters.arrowType() == Qt.UpArrow))
        self.settings.setValue("PluginDatabase/ShowEffects", self.ch_effects.isChecked())
        self.settings.setValue("PluginDatabase/ShowInstruments", self.ch_instruments.isChecked())
        self.settings.setValue("PluginDatabase/ShowMIDI", self.ch_midi.isChecked())
        self.settings.setValue("PluginDatabase/ShowOther", self.ch_other.isChecked())
        self.settings.setValue("PluginDatabase/ShowInternal", self.ch_internal.isChecked())
        self.settings.setValue("PluginDatabase/ShowLADSPA", self.ch_ladspa.isChecked())
        self.settings.setValue("PluginDatabase/ShowDSSI", self.ch_dssi.isChecked())
        self.settings.setValue("PluginDatabase/ShowLV2", self.ch_lv2.isChecked())
        self.settings.setValue("PluginDatabase/ShowVST", self.ch_vst.isChecked())
        self.settings.setValue("PluginDatabase/ShowKits", self.ch_kits.isChecked())
        self.settings.setValue("PluginDatabase/ShowNative", self.ch_native.isChecked())
        self.settings.setValue("PluginDatabase/ShowBridged", self.ch_bridged.isChecked())
        self.settings.setValue("PluginDatabase/ShowBridgedWine", self.ch_bridged_wine.isChecked())
        self.settings.setValue("PluginDatabase/ShowHasGUI", self.ch_gui.isChecked())
        self.settings.setValue("PluginDatabase/ShowStereoOnly", self.ch_stereo.isChecked())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("PluginDatabase/Geometry", ""))
        self.tableWidget.horizontalHeader().restoreState(self.settings.value("PluginDatabase/TableGeometry", ""))
        self.showFilters(self.settings.value("PluginDatabase/ShowFilters", False, type=bool))
        self.ch_effects.setChecked(self.settings.value("PluginDatabase/ShowEffects", True, type=bool))
        self.ch_instruments.setChecked(self.settings.value("PluginDatabase/ShowInstruments", True, type=bool))
        self.ch_midi.setChecked(self.settings.value("PluginDatabase/ShowMIDI", True, type=bool))
        self.ch_other.setChecked(self.settings.value("PluginDatabase/ShowOther", True, type=bool))
        self.ch_internal.setChecked(self.settings.value("PluginDatabase/ShowInternal", True, type=bool))
        self.ch_ladspa.setChecked(self.settings.value("PluginDatabase/ShowLADSPA", True, type=bool))
        self.ch_dssi.setChecked(self.settings.value("PluginDatabase/ShowDSSI", True, type=bool))
        self.ch_lv2.setChecked(self.settings.value("PluginDatabase/ShowLV2", True, type=bool))
        self.ch_vst.setChecked(self.settings.value("PluginDatabase/ShowVST", True, type=bool))
        self.ch_kits.setChecked(self.settings.value("PluginDatabase/ShowKits", True, type=bool))
        self.ch_native.setChecked(self.settings.value("PluginDatabase/ShowNative", True, type=bool))
        self.ch_bridged.setChecked(self.settings.value("PluginDatabase/ShowBridged", True, type=bool))
        self.ch_bridged_wine.setChecked(self.settings.value("PluginDatabase/ShowBridgedWine", True, type=bool))
        self.ch_gui.setChecked(self.settings.value("PluginDatabase/ShowHasGUI", False, type=bool))
        self.ch_stereo.setChecked(self.settings.value("PluginDatabase/ShowStereoOnly", False, type=bool))
        self.reAddPlugins()

    def closeEvent(self, event):
        self.saveSettings()
        QDialog.closeEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# Main Window
class CarlaMainW(QMainWindow, ui_carla.Ui_CarlaMainW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Load Settings

        self.settings = QSettings("Cadence", "Carla")
        self.settings_db = QSettings("Cadence", "Carla-Database")
        self.loadSettings(True)

        self.loadRDFs()

        self.setStyleSheet("""
          QWidget#centralwidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                stop:0 rgb( 7,  7,  7),
                stop:1 rgb(28, 28, 28)
            );
          }
        """)

        # -------------------------------------------------------------
        # Internal stuff

        self.m_engine_started = False
        self.m_project_filename = None

        self.m_firstEngineInit = True
        self.m_pluginCount = 0

        self._nsmAnnounce2str = ""
        self._nsmOpen1str = ""
        self._nsmOpen2str = ""
        self.nsm_server = None
        self.nsm_url = None

        self.m_plugin_list = []
        for x in range(MAX_PLUGINS):
            self.m_plugin_list.append(None)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.act_engine_start.setEnabled(False)
        self.act_engine_stop.setEnabled(False)
        self.act_plugin_remove_all.setEnabled(False)

        #self.m_scene = CarlaScene(self, self.graphicsView)
        #self.graphicsView.setScene(self.m_scene)

        self.resize(self.width(), 0)

        #self.m_fakeEdit = PluginEdit(self, -1)
        #self.m_curEdit  = self.m_fakeEdit
        #self.w_edit.layout().addWidget(self.m_curEdit)
        #self.w_edit.layout().addStretch()

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.act_file_new, SIGNAL("triggered()"), SLOT("slot_file_new()"))
        self.connect(self.act_file_open, SIGNAL("triggered()"), SLOT("slot_file_open()"))
        self.connect(self.act_file_save, SIGNAL("triggered()"), SLOT("slot_file_save()"))
        self.connect(self.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_file_save_as()"))

        self.connect(self.act_engine_start, SIGNAL("triggered()"), SLOT("slot_engine_start()"))
        self.connect(self.act_engine_stop, SIGNAL("triggered()"), SLOT("slot_engine_stop()"))

        self.connect(self.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_plugin_add()"))
        self.connect(self.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_remove_all()"))

        self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))
        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        self.connect(self, SIGNAL("DebugCallback(int, int, int, double, QString)"), SLOT("slot_handleDebugCallback(int, int, int, double, QString)"))
        self.connect(self, SIGNAL("ParameterValueCallback(int, int, double)"), SLOT("slot_handleParameterValueCallback(int, int, double)"))
        self.connect(self, SIGNAL("ParameterMidiChannelCallback(int, int, int)"), SLOT("slot_handleParameterMidiChannelCallback(int, int, int)"))
        self.connect(self, SIGNAL("ParameterMidiCcCallback(int, int, int)"), SLOT("slot_handleParameterMidiCcCallback(int, int, int)"))
        self.connect(self, SIGNAL("ProgramCallback(int, int)"), SLOT("slot_handleProgramCallback(int, int)"))
        self.connect(self, SIGNAL("MidiProgramCallback(int, int)"), SLOT("slot_handleMidiProgramCallback(int, int)"))
        self.connect(self, SIGNAL("NoteOnCallback(int, int, int, int)"), SLOT("slot_handleNoteOnCallback(int, int, int, int)"))
        self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), SLOT("slot_handleNoteOffCallback(int, int, int)"))
        self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), SLOT("slot_handleShowGuiCallback(int, int)"))
        self.connect(self, SIGNAL("ResizeGuiCallback(int, int, int)"), SLOT("slot_handleResizeGuiCallback(int, int, int)"))
        self.connect(self, SIGNAL("UpdateCallback(int)"), SLOT("slot_handleUpdateCallback(int)"))
        self.connect(self, SIGNAL("ReloadInfoCallback(int)"), SLOT("slot_handleReloadInfoCallback(int)"))
        self.connect(self, SIGNAL("ReloadParametersCallback(int)"), SLOT("slot_handleReloadParametersCallback(int)"))
        self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), SLOT("slot_handleReloadProgramsCallback(int)"))
        self.connect(self, SIGNAL("ReloadAllCallback(int)"), SLOT("slot_handleReloadAllCallback(int)"))
        self.connect(self, SIGNAL("NSM_AnnounceCallback()"), SLOT("slot_handleNSM_AnnounceCallback()"))
        self.connect(self, SIGNAL("NSM_Open1Callback()"), SLOT("slot_handleNSM_Open1Callback()"))
        self.connect(self, SIGNAL("NSM_Open2Callback()"), SLOT("slot_handleNSM_Open2Callback()"))
        self.connect(self, SIGNAL("NSM_SaveCallback()"), SLOT("slot_handleNSM_SaveCallback()"))
        self.connect(self, SIGNAL("ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        self.TIMER_GUI_STUFF  = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])     # Peaks
        self.TIMER_GUI_STUFF2 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 2) # LEDs and edit dialog

        NSM_URL = os.getenv("NSM_URL")

        if NSM_URL:
            Carla.host.nsm_announce(NSM_URL, os.getpid())
        else:
            QTimer.singleShot(0, self, SLOT("slot_engine_start()"))

        QTimer.singleShot(0, self, SLOT("slot_showInitialWarning()"))

    def loadProjectLater(self):
        QTimer.singleShot(0, self.load_project)

    def startEngine(self, clientName = "Carla"):
        # ---------------------------------------------
        # engine

        Carla.processMode    = self.settings.value("Engine/ProcessMode", PROCESS_MODE_MULTIPLE_CLIENTS, type=int)
        Carla.maxParameters  = self.settings.value("Engine/MaxParameters", MAX_PARAMETERS, type=int)

        processHighPrecision = self.settings.value("Engine/ProcessHighPrecision", False, type=bool)
        preferredBufferSize  = self.settings.value("Engine/PreferredBufferSize", 512, type=int)
        preferredSampleRate  = self.settings.value("Engine/PreferredSampleRate", 44100, type=int)

        forceStereo          = self.settings.value("Engine/ForceStereo", False, type=bool)
        useDssiVstChunks     = self.settings.value("Engine/UseDssiVstChunks", False, type=bool)
        preferPluginBridges  = self.settings.value("Engine/PreferPluginBridges", False, type=bool)

        preferUiBridges = self.settings.value("Engine/PreferUiBridges", True, type=bool)
        oscUiTimeout    = self.settings.value("Engine/OscUiTimeout", 40, type=int)

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif Carla.processMode == PROCESS_MODE_MULTIPLE_CLIENTS and os.getenv("LADISH_APP_NAME"):
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            Carla.processMode = PROCESS_MODE_SINGLE_CLIENT

        Carla.host.set_option(OPTION_PROCESS_MODE, Carla.processMode, "")
        Carla.host.set_option(OPTION_PROCESS_HIGH_PRECISION, processHighPrecision, "")

        Carla.host.set_option(OPTION_MAX_PARAMETERS, Carla.maxParameters, "")
        Carla.host.set_option(OPTION_PREFERRED_BUFFER_SIZE, preferredBufferSize, "")
        Carla.host.set_option(OPTION_PREFERRED_SAMPLE_RATE, preferredSampleRate, "")

        Carla.host.set_option(OPTION_FORCE_STEREO, forceStereo, "")
        Carla.host.set_option(OPTION_USE_DSSI_VST_CHUNKS, useDssiVstChunks, "")
        Carla.host.set_option(OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")

        Carla.host.set_option(OPTION_PREFER_UI_BRIDGES, preferUiBridges, "")
        Carla.host.set_option(OPTION_OSC_UI_TIMEOUT, oscUiTimeout, "")

        # ---------------------------------------------
        # start

        audioDriver = self.settings.value("Engine/AudioDriver", "JACK", type=str)

        if not Carla.host.engine_init(audioDriver, clientName):
            if self.m_firstEngineInit:
                self.m_firstEngineInit = False
                return

            self.act_engine_start.setEnabled(True)
            self.act_engine_stop.setEnabled(False)

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons: %s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        if self.m_firstEngineInit:
            self.m_firstEngineInit = False

        self.m_engine_started = True

    def stopEngine(self):
        if self.m_pluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask == QMessageBox.Yes:
                self.slot_remove_all()
            else:
                return

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(cString(Carla.host.get_last_error()))

        self.m_engine_started = False

    #def pluginWidgetActivated(self, widget):
        #if self.m_curEdit == widget:
            #self.keyboard.allNotesOff()

    #def pluginWidgetClicked(self, widget):
        #if self.m_curEdit == widget:
            #return

        #self.w_edit.layout().removeWidget(self.m_curEdit)
        #self.w_edit.layout().insertWidget(0, widget)

        #widget.show()
        #self.m_curEdit.hide()

        #self.m_curEdit = widget

    @pyqtSlot()
    def slot_showInitialWarning(self):
        QMessageBox.warning(self, self.tr("Carla is incomplete"), self.tr(""
            "The version of Carla you're currently running is incomplete.\n"
            "Although most things work fine, Carla is not yet in a stable state.\n"
            "\n"
            "It will be fully functional for the next Cadence beta release."
            ""))

    @pyqtSlot()
    def slot_engine_start(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.act_file_open.setEnabled(check)
        self.act_engine_start.setEnabled(not check)
        self.act_engine_stop.setEnabled(check)

    @pyqtSlot()
    def slot_engine_stop(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.act_file_open.setEnabled(check)
        self.act_engine_start.setEnabled(not check)
        self.act_engine_stop.setEnabled(check)

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_file_save()"))

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, plugin_id, value1, value2, value3, valueStr):
        print("DEBUG :: %i, %i, %i, %f, \"%s\")" % (plugin_id, value1, value2, value3, valueStr))

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueCallback(self, pluginId, parameterId, value):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.m_parameterIconTimer = ICON_STATE_ON

            if parameterId == PARAMETER_ACTIVE:
                pwidget.set_active((value > 0.0), True, False)
            elif parameterId == PARAMETER_DRYWET:
                pwidget.set_drywet(value * 1000, True, False)
            elif parameterId == PARAMETER_VOLUME:
                pwidget.set_volume(value * 1000, True, False)
            elif parameterId == PARAMETER_BALANCE_LEFT:
                pwidget.set_balance_left(value * 1000, True, False)
            elif parameterId == PARAMETER_BALANCE_RIGHT:
                pwidget.set_balance_right(value * 1000, True, False)
            elif parameterId >= 0:
                pwidget.edit_dialog.set_parameter_to_update(parameterId)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelCallback(self, pluginId, parameterId, channel):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_parameter_midi_channel(parameterId, channel, True)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcCallback(self, pluginId, parameterId, cc):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_parameter_midi_cc(parameterId, cc, True)

    @pyqtSlot(int, int)
    def slot_handleProgramCallback(self, plugin_id, program_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.set_program(program_id)
            pwidget.m_parameterIconTimer = ICON_STATE_ON

    @pyqtSlot(int, int)
    def slot_handleMidiProgramCallback(self, plugin_id, midi_program_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.set_midi_program(midi_program_id)
            pwidget.m_parameterIconTimer = ICON_STATE_ON

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, plugin_id, channel, note, velo):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, plugin_id, channel, note):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.keyboard.sendNoteOff(note, False)

    @pyqtSlot(int, int)
    def slot_handleShowGuiCallback(self, plugin_id, show):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            if show == 0:
                pwidget.b_gui.setChecked(False)
                pwidget.b_gui.setEnabled(True)
            elif show == 1:
                pwidget.b_gui.setChecked(True)
                pwidget.b_gui.setEnabled(True)
            elif show == -1:
                pwidget.b_gui.setChecked(False)
                pwidget.b_gui.setEnabled(False)

    @pyqtSlot(int, int, int)
    def slot_handleResizeGuiCallback(self, plugin_id, width, height):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            gui_dialog = pwidget.gui_dialog
            if gui_dialog:
                gui_dialog.setNewSize(width, height)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.do_update()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.do_reload_info()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.do_reload_parameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.do_reload_programs()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, plugin_id):
        pwidget = self.m_plugin_list[plugin_id]
        if pwidget:
            pwidget.edit_dialog.do_reload_all()

    @pyqtSlot()
    def slot_handleNSM_AnnounceCallback(self):
        smName = self._nsmAnnounce2str

        self.act_file_new.setEnabled(False)
        self.act_file_open.setEnabled(False)
        self.act_file_save_as.setEnabled(False)
        self.setWindowTitle("Carla (%s)" % smName)

    @pyqtSlot()
    def slot_handleNSM_Open1Callback(self):
        clientId = self._nsmOpen1str

        # remove all previous plugins
        self.slot_remove_all()

        # restart engine
        if Carla.host.is_engine_running():
            self.stopEngine()

        self.startEngine(clientId)

    @pyqtSlot()
    def slot_handleNSM_Open2Callback(self):
        projectPath = self._nsmOpen2str

        self.m_project_filename = projectPath

        if os.path.exists(self.m_project_filename):
            self.load_project()
        else:
            self.save_project()

        self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))
        Carla.host.nsm_reply_open()

    @pyqtSlot()
    def slot_handleNSM_SaveCallback(self):
        self.save_project()
        Carla.host.nsm_reply_save()

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, self.tr("Error"), error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("JACK has been stopped or crashed.\nPlease start JACK and restart Carla"),
            self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    def add_plugin(self, btype, ptype, filename, name, label, extra_stuff, activate):
        if not self.m_engine_started:
            if activate:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return -1

        new_plugin_id = Carla.host.add_plugin(btype, ptype, filename, name, label, extra_stuff)

        if new_plugin_id < 0:
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
            return -1
        else:
            pwidget = PluginWidget(self, new_plugin_id)
            self.w_plugins.layout().addWidget(pwidget)
            #self.m_scene.addWidget(new_plugin_id, pwidget)
            self.m_plugin_list[new_plugin_id] = pwidget
            self.act_plugin_remove_all.setEnabled(True)

            pwidget.peak_in.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])
            pwidget.peak_out.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])

            if activate:
                pwidget.set_active(True, True, True)

        self.m_pluginCount += 1
        return new_plugin_id

    def remove_plugin(self, plugin_id, showError):
        pwidget = self.m_plugin_list[plugin_id]

        #if pwidget.edit_dialog == self.m_curEdit:
            #self.w_edit.layout().removeWidget(self.m_curEdit)
            #self.w_edit.layout().insertWidget(0, self.m_fakeEdit)

            #self.m_fakeEdit.show()
            #self.m_curEdit.hide()

            #self.m_curEdit = self.m_fakeEdit

        pwidget.edit_dialog.close()

        if pwidget.gui_dialog:
            pwidget.gui_dialog.close()

        if Carla.host.remove_plugin(plugin_id):
            pwidget.close()
            pwidget.deleteLater()
            self.w_plugins.layout().removeWidget(pwidget)
            self.m_plugin_list[plugin_id] = None
            self.m_pluginCount -= 1

        elif showError:
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to remove plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        # push all plugins 1 slot if rack mode
        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            for i in range(MAX_PLUGINS-1):
                if i < plugin_id: continue
                self.m_plugin_list[i] = self.m_plugin_list[i+1]

                if self.m_plugin_list[i]:
                    self.m_plugin_list[i].setId(i)

            self.m_plugin_list[MAX_PLUGINS-1] = None

        # check if there are still plugins
        for i in range(MAX_PLUGINS):
            if self.m_plugin_list[i]: break
        else:
            self.act_plugin_remove_all.setEnabled(False)

    def get_extra_stuff(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            unique_id = plugin['unique_id']
            for rdf_item in self.ladspa_rdf_list:
                if rdf_item.UniqueID == unique_id:
                    return pointer(rdf_item)

        elif ptype == PLUGIN_DSSI:
            if plugin['hints'] & PLUGIN_HAS_GUI:
                gui = findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
                if gui:
                    return gui.encode("utf-8")

        return c_nullptr

    def save_project(self):
        content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   "<!DOCTYPE CARLA-PROJECT>\n"
                   "<CARLA-PROJECT VERSION='%s'>\n") % (VERSION)

        first_plugin = True

        for pwidget in self.m_plugin_list:
            if pwidget:
                if not first_plugin:
                    content += "\n"

                real_plugin_name = cString(Carla.host.get_real_plugin_name(pwidget.m_pluginId))
                if real_plugin_name:
                    content += " <!-- %s -->\n" % xmlSafeString(real_plugin_name, True)

                content += " <Plugin>\n"
                content += pwidget.getSaveXMLContent()
                content += " </Plugin>\n"

                first_plugin = False

        content += "</CARLA-PROJECT>\n"

        try:
            fd = uopen(self.m_project_filename, "w")
            fd.write(content)
            fd.close()
        except:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save project file"))

    def load_project(self):
        try:
            fd = uopen(self.m_project_filename, "r")
            projectRead = fd.read()
            fd.close()
        except:
            projectRead = None

        if not projectRead:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load project file"))
            return

        xml = QDomDocument()
        xml.setContent(projectRead.encode("utf-8"))

        xml_node = xml.documentElement()
        if xml_node.tagName() != "CARLA-PROJECT":
            QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla project file"))
            return

        x_internal_plugins = None
        x_ladspa_plugins = None
        x_dssi_plugins = None
        x_lv2_plugins = None
        x_vst_plugins = None
        x_gig_plugins = None
        x_sf2_plugins = None
        x_sfz_plugins = None

        x_failedPlugins = []
        x_saveStates = []

        node = xml_node.firstChild()
        while not node.isNull():
            if node.toElement().tagName() == "Plugin":
                x_saveState = getSaveStateDictFromXML(node)
                x_saveStates.append(x_saveState)
            node = node.nextSibling()

        for x_saveState in x_saveStates:
            ptype = x_saveState['Type']
            label = x_saveState['Label']
            binary = x_saveState['Binary']
            binaryS = os.path.basename(binary)
            unique_id = x_saveState['UniqueID']

            if ptype == "Internal":
                if not x_internal_plugins: x_internal_plugins = toList(self.settings_db.value("Plugins/Internal", []))
                x_plugins = x_internal_plugins

            elif ptype == "LADSPA":
                if not x_ladspa_plugins:
                    x_ladspa_plugins  = []
                    x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_native", []))
                    x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix32", []))
                    x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix64", []))
                    x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win32", []))
                    x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win64", []))
                x_plugins = x_ladspa_plugins

            elif ptype == "DSSI":
                if not x_dssi_plugins:
                    x_dssi_plugins  = []
                    x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_native", []))
                    x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix32", []))
                    x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix64", []))
                    x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win32", []))
                    x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win64", []))
                x_plugins = x_dssi_plugins

            elif ptype == "LV2":
                if not x_lv2_plugins:
                    x_lv2_plugins  = []
                    x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_native", []))
                    x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix32", []))
                    x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix64", []))
                    x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win32", []))
                    x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win64", []))
                x_plugins = x_lv2_plugins

            elif ptype == "VST":
                if not x_vst_plugins:
                    x_vst_plugins  = []
                    x_vst_plugins += toList(self.settings_db.value("Plugins/VST_native", []))
                    x_vst_plugins += toList(self.settings_db.value("Plugins/VST_posix32", []))
                    x_vst_plugins += toList(self.settings_db.value("Plugins/VST_posix64", []))
                    x_vst_plugins += toList(self.settings_db.value("Plugins/VST_win32", []))
                    x_vst_plugins += toList(self.settings_db.value("Plugins/VST_win64", []))
                x_plugins = x_vst_plugins

            elif ptype == "GIG":
                if not x_gig_plugins: x_gig_plugins = toList(self.settings_db.value("Plugins/GIG", []))
                x_plugins = x_gig_plugins

            elif ptype == "SF2":
                if not x_sf2_plugins: x_sf2_plugins = toList(self.settings_db.value("Plugins/SF2", []))
                x_plugins = x_sf2_plugins

            elif ptype == "SFZ":
                if not x_sfz_plugins: x_sfz_plugins = toList(self.settings_db.value("Plugins/SFZ", []))
                x_plugins = x_sfz_plugins

            else:
                print("load_project() - ptype '%s' not recognized" % ptype)
                x_failedPlugins.append(x_saveState['Name'])
                continue

            # Try UniqueID -> Label -> Binary (full) -> Binary (short)
            plugin_ulB = None
            plugin_ulb = None
            plugin_ul = None
            plugin_uB = None
            plugin_ub = None
            plugin_lB = None
            plugin_lb = None
            plugin_u = None
            plugin_l = None
            plugin_B = None

            for _plugins in x_plugins:
                for x_plugin in _plugins:
                    if unique_id == x_plugin['unique_id'] and label == x_plugin['label'] and binary == x_plugin['binary']:
                        plugin_ulB = x_plugin
                        break
                    elif unique_id == x_plugin['unique_id'] and label == x_plugin['label'] and binaryS == os.path.basename(x_plugin['binary']):
                        plugin_ulb = x_plugin
                    elif unique_id == x_plugin['unique_id'] and label == x_plugin['label']:
                        plugin_ul = x_plugin
                    elif unique_id == x_plugin['unique_id'] and binary == x_plugin['binary']:
                        plugin_uB = x_plugin
                    elif unique_id == x_plugin['unique_id'] and binaryS == os.path.basename(x_plugin['binary']):
                        plugin_ub = x_plugin
                    elif label == x_plugin['label'] and binary == x_plugin['binary']:
                        plugin_lB = x_plugin
                    elif label == x_plugin['label'] and binaryS == os.path.basename(x_plugin['binary']):
                        plugin_lb = x_plugin
                    elif unique_id == x_plugin['unique_id']:
                        plugin_u = x_plugin
                    elif label == x_plugin['label']:
                        plugin_l = x_plugin
                    elif binary == x_plugin['binary']:
                        plugin_B = x_plugin

            # LADSPA uses UniqueID or binary+label
            if ptype == "LADSPA":
                plugin_l = None
                plugin_B = None

            # DSSI uses binary+label (UniqueID ignored)
            elif ptype == "DSSI":
                plugin_ul = None
                plugin_uB = None
                plugin_ub = None
                plugin_u = None
                plugin_l = None
                plugin_B = None

            # LV2 uses URIs (label in this case)
            elif ptype == "LV2":
                plugin_uB = None
                plugin_ub = None
                plugin_u = None
                plugin_B = None

            # VST uses UniqueID
            elif ptype == "VST":
                plugin_lB = None
                plugin_lb = None
                plugin_l = None
                plugin_B = None

            # Sound Kits use binaries
            elif ptype in ("GIG", "SF2", "SFZ"):
                plugin_ul = None
                plugin_u = None
                plugin_l = None
                plugin_B = binary

            if plugin_ulB:
                plugin = plugin_ulB
            elif plugin_ulb:
                plugin = plugin_ulb
            elif plugin_ul:
                plugin = plugin_ul
            elif plugin_uB:
                plugin = plugin_uB
            elif plugin_ub:
                plugin = plugin_ub
            elif plugin_lB:
                plugin = plugin_lB
            elif plugin_lb:
                plugin = plugin_lb
            elif plugin_u:
                plugin = plugin_u
            elif plugin_l:
                plugin = plugin_l
            elif plugin_B:
                plugin = plugin_B
            else:
                plugin = None

            if plugin:
                btype    = plugin['build']
                ptype    = plugin['type']
                filename = plugin['binary']
                name     = x_saveState['Name']
                label    = plugin['label']
                extra_stuff   = self.get_extra_stuff(plugin)
                new_plugin_id = self.add_plugin(btype, ptype, filename, name, label, extra_stuff, False)

                if new_plugin_id >= 0:
                    pwidget = self.m_plugin_list[new_plugin_id]
                    pwidget.loadStateDict(x_saveState)

                else:
                    x_failedPlugins.append(x_saveState['Name'])

            else:
                x_failedPlugins.append(x_saveState['Name'])

        if len(x_failedPlugins) > 0:
            text = self.tr("The following plugins were not found or failed to initialize:\n")
            for plugin in x_failedPlugins:
                text += " - %s\n" % plugin

            self.statusBar().showMessage("State file loaded with errors")
            QMessageBox.critical(self, self.tr("Error"), text)

        else:
            self.statusBar().showMessage("State file loaded sucessfully!")

    def loadRDFs(self):
        # Save RDF info for later
        if haveLRDF:
            SettingsDir = os.path.join(HOME, ".config", "Cadence")
            fr_ladspa_file = os.path.join(SettingsDir, "ladspa_rdf.db")
            if os.path.exists(fr_ladspa_file):
                fr_ladspa = open(fr_ladspa_file, 'r')
                try:
                    self.ladspa_rdf_list = ladspa_rdf.get_c_ladspa_rdfs(json.load(fr_ladspa))
                except:
                    self.ladspa_rdf_list = []
                fr_ladspa.close()
                return

        self.ladspa_rdf_list = []

    @pyqtSlot()
    def slot_file_new(self):
        self.slot_remove_all()
        self.m_project_filename = None
        self.setWindowTitle("Carla")

    @pyqtSlot()
    def slot_file_open(self):
        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.m_savedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            self.m_project_filename = filenameTry
            self.slot_remove_all()
            self.load_project()
            self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))

    @pyqtSlot()
    def slot_file_save(self, saveAs=False):
        if self.m_project_filename == None or saveAs:
            fileFilter  = self.tr("Carla Project File (*.carxp)")
            filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.m_savedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

            if filenameTry:
                if not filenameTry.endswith(".carxp"):
                    filenameTry += ".carxp"

                self.m_project_filename = filenameTry
                self.save_project()
                self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))

        else:
            self.save_project()

    @pyqtSlot()
    def slot_file_save_as(self):
        self.slot_file_save(True)

    @pyqtSlot()
    def slot_plugin_add(self):
        dialog = PluginDatabaseW(self)
        if dialog.exec_():
            btype = dialog.ret_plugin['build']
            ptype = dialog.ret_plugin['type']
            filename = dialog.ret_plugin['binary']
            label = dialog.ret_plugin['label']
            extra_stuff = self.get_extra_stuff(dialog.ret_plugin)
            self.add_plugin(btype, ptype, filename, None, label, extra_stuff, True)

    @pyqtSlot()
    def slot_remove_all(self):
        h = 0
        for i in range(MAX_PLUGINS):
            pwidget = self.m_plugin_list[i]

            if not pwidget:
                continue

            pwidget.setId(i-h)
            pwidget.edit_dialog.close()

            if pwidget.gui_dialog:
                pwidget.gui_dialog.close()

            if Carla.host.remove_plugin(i-h):
                pwidget.close()
                pwidget.deleteLater()
                self.w_plugins.layout().removeWidget(pwidget)
                self.m_plugin_list[i] = None

            if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
                h += 1

        self.m_pluginCount = 0
        self.act_plugin_remove_all.setEnabled(False)

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = SettingsW(self, "carla")
        if dialog.exec_():
            self.loadSettings(False)

            for pwidget in self.m_plugin_list:
                if pwidget:
                    pwidget.peak_in.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])
                    pwidget.peak_out.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        self.settings.setValue("ShowToolbar", self.toolBar.isVisible())

    def loadSettings(self, geometry):
        if geometry:
            self.restoreGeometry(self.settings.value("Geometry", ""))

            show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
            self.act_settings_show_toolbar.setChecked(show_toolbar)
            self.toolBar.setVisible(show_toolbar)

        self.m_savedSettings = {
            "Main/DefaultProjectFolder": self.settings.value("Main/DefaultProjectFolder", DEFAULT_PROJECT_FOLDER, type=str),
            "Main/RefreshInterval": self.settings.value("Main/RefreshInterval", 120, type=int)
        }

        # ---------------------------------------------
        # plugin checks

        disableChecks = self.settings.value("Engine/DisableChecks", False, type=bool)

        if disableChecks:
            os.environ["CARLA_DISCOVERY_NO_PROCESSING_CHECKS"] = "true"

        elif os.getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"):
            os.environ.pop("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")

        # ---------------------------------------------
        # plugin paths

        global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH
        LADSPA_PATH = toList(self.settings.value("Paths/LADSPA", LADSPA_PATH))
        DSSI_PATH = toList(self.settings.value("Paths/DSSI", DSSI_PATH))
        LV2_PATH = toList(self.settings.value("Paths/LV2", LV2_PATH))
        VST_PATH = toList(self.settings.value("Paths/VST", VST_PATH))
        GIG_PATH = toList(self.settings.value("Paths/GIG", GIG_PATH))
        SF2_PATH = toList(self.settings.value("Paths/SF2", SF2_PATH))
        SFZ_PATH = toList(self.settings.value("Paths/SFZ", SFZ_PATH))

        os.environ["LADSPA_PATH"] = splitter.join(LADSPA_PATH)
        os.environ["DSSI_PATH"] = splitter.join(DSSI_PATH)
        os.environ["LV2_PATH"] = splitter.join(LV2_PATH)
        os.environ["VST_PATH"] = splitter.join(VST_PATH)
        os.environ["GIG_PATH"] = splitter.join(GIG_PATH)
        os.environ["SF2_PATH"] = splitter.join(SF2_PATH)
        os.environ["SFZ_PATH"] = splitter.join(SFZ_PATH)

    #def resizeEvent(self, event):
        #self.m_scene.resize()
        #QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.TIMER_GUI_STUFF:
            for pwidget in self.m_plugin_list:
                if pwidget: pwidget.check_gui_stuff()
            if self.m_engine_started and self.m_pluginCount > 0:
                Carla.host.idle_guis()

        elif event.timerId() == self.TIMER_GUI_STUFF2:
            for pwidget in self.m_plugin_list:
                if pwidget: pwidget.check_gui_stuff2()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        if self.nsm_server:
            self.nsm_server.stop()

        self.saveSettings()
        self.slot_remove_all()
        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------

def callback_function(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId< 0 or pluginId >= MAX_PLUGINS or not Carla.gui:
        return

    if action == CALLBACK_DEBUG:
        Carla.gui.emit(SIGNAL("DebugCallback(int, int, int, double, QString)"), pluginId, value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterValueCallback(int, int, double)"), pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterMidiChannelCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterMidiCcCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PROGRAM_CHANGED:
        Carla.gui.emit(SIGNAL("ProgramCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        Carla.gui.emit(SIGNAL("MidiProgramCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_NOTE_ON:
        Carla.gui.emit(SIGNAL("NoteOnCallback(int, int, int, int)"), pluginId, value1, value2, value3)
    elif action == CALLBACK_NOTE_OFF:
        Carla.gui.emit(SIGNAL("NoteOffCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_SHOW_GUI:
        Carla.gui.emit(SIGNAL("ShowGuiCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_RESIZE_GUI:
        Carla.gui.emit(SIGNAL("ResizeGuiCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_UPDATE:
        Carla.gui.emit(SIGNAL("UpdateCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_INFO:
        Carla.gui.emit(SIGNAL("ReloadInfoCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_PARAMETERS:
        Carla.gui.emit(SIGNAL("ReloadParametersCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_PROGRAMS:
        Carla.gui.emit(SIGNAL("ReloadProgramsCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_ALL:
        Carla.gui.emit(SIGNAL("ReloadAllCallback(int)"), pluginId)
    elif action == CALLBACK_NSM_ANNOUNCE:
        Carla.gui._nsmAnnounce2str = cString(Carla.host.get_last_error())
        Carla.gui.emit(SIGNAL("NSM_AnnounceCallback()"))
    elif action == CALLBACK_NSM_OPEN1:
        Carla.gui._nsmOpen1str = cString(valueStr)
        Carla.gui.emit(SIGNAL("NSM_Open1Callback()"))
    elif action == CALLBACK_NSM_OPEN2:
        Carla.gui._nsmOpen2str = cString(valueStr)
        Carla.gui.emit(SIGNAL("NSM_Open2Callback()"))
    elif action == CALLBACK_NSM_SAVE:
        Carla.gui.emit(SIGNAL("NSM_SaveCallback()"))
    elif action == CALLBACK_ERROR:
        Carla.gui.emit(SIGNAL("ErrorCallback(QString)"), cString(Carla.host.get_last_error()))
    elif action == CALLBACK_QUIT:
        Carla.gui.emit(SIGNAL("QuitCallback()"))

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/carla.svg"))

    libPrefix = None
    projectFilename = None

    for i in range(len(app.arguments())):
        if i == 0: continue
        argument = app.arguments()[i]

        if argument.startswith("--with-libprefix="):
            libPrefix = argument.replace("--with-libprefix=", "")

        elif os.path.exists(argument):
            projectFilename = argument

    # Init backend
    Carla.host = Host(libPrefix)
    Carla.host.set_callback_function(callback_function)
    Carla.host.set_option(OPTION_PROCESS_NAME, 0, "carla")

    # Set bridge paths
    if carla_bridge_posix32:
        Carla.host.set_option(OPTION_PATH_BRIDGE_POSIX32, 0, carla_bridge_posix32)

    if carla_bridge_posix64:
        Carla.host.set_option(OPTION_PATH_BRIDGE_POSIX64, 0, carla_bridge_posix64)

    if carla_bridge_win32:
        Carla.host.set_option(OPTION_PATH_BRIDGE_WIN32, 0, carla_bridge_win32)

    if carla_bridge_win64:
        Carla.host.set_option(OPTION_PATH_BRIDGE_WIN64, 0, carla_bridge_win64)

    if WINDOWS:
        if carla_bridge_lv2_windows:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, carla_bridge_lv2_windows)

        if carla_bridge_vst_hwnd:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_HWND, 0, carla_bridge_vst_hwnd)

    elif MACOS:
        if carla_bridge_lv2_cocoa:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_COCOA, 0, carla_bridge_lv2_cocoa)

        if carla_bridge_vst_cocoa:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_COCOA, 0, carla_bridge_vst_cocoa)

    else:
        if carla_bridge_lv2_gtk2:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_GTK2, 0, carla_bridge_lv2_gtk2)

        if carla_bridge_lv2_gtk3:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_GTK3, 0, carla_bridge_lv2_gtk3)

        if carla_bridge_lv2_qt4:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_QT4, 0, carla_bridge_lv2_qt4)

        if carla_bridge_lv2_qt5:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_QT5, 0, carla_bridge_lv2_qt5)

        if carla_bridge_lv2_x11:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_X11, 0, carla_bridge_lv2_x11)

        if carla_bridge_vst_x11:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_X11, 0, carla_bridge_vst_x11)

    # Set available drivers
    driverCount = Carla.host.get_engine_driver_count()
    driverList  = []
    for i in range(driverCount):
        driver = cString(Carla.host.get_engine_driver_name(i))
        if driver:
            driverList.append(driver)
    setAvailableEngineDrivers(driverList)

    # Create GUI and start engine
    Carla.gui = CarlaMainW()

    # Set-up custom signal handling
    setUpSignals(Carla.gui)

    # Show GUI
    Carla.gui.show()

    # Load project file if set
    if projectFilename:
        Carla.gui.m_project_filename = projectFilename
        Carla.gui.loadProjectLater()
        Carla.gui.setWindowTitle("Carla - %s" % os.path.basename(projectFilename)) # FIXME - put in loadProject

    # App-Loop
    ret = app.exec_()

    # Close Host
    Carla.gui.stopEngine()

    # Exit properly
    sys.exit(ret)
