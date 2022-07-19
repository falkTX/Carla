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

from PyQt5.QtCore import pyqtSlot, Qt, QT_VERSION
from PyQt5.QtGui import QPixmap
from PyQt5.QtWidgets import QDialog, QWidget

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Carla)

from carla_shared import (
    HAIKU,
    LINUX,
    MACOS,
    WINDOWS,
    gCarla,
    getIcon,
    kIs64bit,
)

from utils import QSafeSettings

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Local)

from .discovery import killDiscovery
from .discoverythread import SearchPluginsThread
from .pluginlistrefreshdialog_ui import Ui_PluginRefreshW

# ---------------------------------------------------------------------------------------------------------------------
# Plugin Refresh Dialog

class PluginRefreshW(QDialog):
    def __init__(self, parent: QWidget, host, useSystemIcons: bool, hasLoadedLv2Plugins: bool):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = Ui_PluginRefreshW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------------------------------------------------------
        # Internal stuff

        toolNative = "carla-discovery-native.exe" if WINDOWS else "carla-discovery-native"
        hasNative  = os.path.exists(os.path.join(host.pathBinaries, toolNative))
        hasPosix32 = os.path.exists(os.path.join(host.pathBinaries, "carla-discovery-posix32"))
        hasPosix64 = os.path.exists(os.path.join(host.pathBinaries, "carla-discovery-posix64"))
        hasWin32   = os.path.exists(os.path.join(host.pathBinaries, "carla-discovery-win32.exe"))
        hasWin64   = os.path.exists(os.path.join(host.pathBinaries, "carla-discovery-win64.exe"))

        self.fThread = SearchPluginsThread(self, host.pathBinaries)

        # -------------------------------------------------------------------------------------------------------------
        # Set-up Icons

        if useSystemIcons:
            self.ui.b_start.setIcon(getIcon('arrow-right', 16, 'svgz'))
            self.ui.b_close.setIcon(getIcon('window-close', 16, 'svgz'))
            if QT_VERSION >= 0x50600:
                size = int(16 * self.devicePixelRatioF())
            else:
                size = 16
            self.fIconYes = QPixmap(getIcon('dialog-ok-apply', 16, 'svgz').pixmap(size))
            self.fIconNo  = QPixmap(getIcon('dialog-error', 16, 'svgz').pixmap(size))
        else:
            self.fIconYes = QPixmap(":/16x16/dialog-ok-apply.svgz")
            self.fIconNo  = QPixmap(":/16x16/dialog-error.svgz")

        # -------------------------------------------------------------------------------------------------------------
        # Set-up GUI

        # FIXME remove LRDF
        self.ui.ico_rdflib.setPixmap(self.fIconNo)

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

        if WINDOWS:
            if kIs64bit:
                hasNonNative = hasWin32
                self.ui.ch_win64.setEnabled(False)
                self.ui.ch_win64.setVisible(False)
                self.ui.ico_win64.setVisible(False)
                self.ui.label_win64.setVisible(False)
            else:
                hasNonNative = hasWin64
                self.ui.ch_win32.setEnabled(False)
                self.ui.ch_win32.setVisible(False)
                self.ui.ico_win32.setVisible(False)
                self.ui.label_win32.setVisible(False)

            self.ui.ch_posix32.setEnabled(False)
            self.ui.ch_posix32.setVisible(False)
            self.ui.ch_posix64.setEnabled(False)
            self.ui.ch_posix64.setVisible(False)
            self.ui.ico_posix32.hide()
            self.ui.ico_posix64.hide()
            self.ui.label_posix32.hide()
            self.ui.label_posix64.hide()
            self.ui.ico_rdflib.hide()
            self.ui.label_rdflib.hide()

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

                if not (LINUX or hasWin32 or hasWin64):
                    self.ui.ch_vst3.setEnabled(False)
                    self.ui.ch_vst3.setVisible(False)

        if MACOS:
            self.setWindowModality(Qt.WindowModal)
        else:
            self.ui.ch_au.setEnabled(False)
            self.ui.ch_au.setVisible(False)

        if hasNative:
            self.ui.ico_native.setPixmap(self.fIconYes)
        else:
            self.ui.ico_native.setPixmap(self.fIconNo)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_sf2.setEnabled(False)
            if not hasNonNative:
                self.ui.ch_ladspa.setEnabled(False)
                self.ui.ch_dssi.setEnabled(False)
                self.ui.ch_vst.setEnabled(False)
                self.ui.ch_vst3.setEnabled(False)

        if not hasLoadedLv2Plugins:
            self.ui.lv2_restart_notice.hide()

        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        # -------------------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Hide bridges if disabled

        # NOTE: We Assume win32 carla build will not run win64 plugins
        if (WINDOWS and not kIs64bit) or not host.showPluginBridges:
            self.ui.ch_native.setChecked(True)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_native.setVisible(False)
            self.ui.ch_posix32.setChecked(False)
            self.ui.ch_posix32.setEnabled(False)
            self.ui.ch_posix32.setVisible(False)
            self.ui.ch_posix64.setChecked(False)
            self.ui.ch_posix64.setEnabled(False)
            self.ui.ch_posix64.setVisible(False)
            self.ui.ch_win32.setChecked(False)
            self.ui.ch_win32.setEnabled(False)
            self.ui.ch_win32.setVisible(False)
            self.ui.ch_win64.setChecked(False)
            self.ui.ch_win64.setEnabled(False)
            self.ui.ch_win64.setVisible(False)
            self.ui.ico_posix32.hide()
            self.ui.ico_posix64.hide()
            self.ui.ico_win32.hide()
            self.ui.ico_win64.hide()
            self.ui.label_posix32.hide()
            self.ui.label_posix64.hide()
            self.ui.label_win32.hide()
            self.ui.label_win64.hide()
            self.ui.sep_format.hide()

        elif not (WINDOWS or host.showWineBridges):
            self.ui.ch_win32.setChecked(False)
            self.ui.ch_win32.setEnabled(False)
            self.ui.ch_win32.setVisible(False)
            self.ui.ch_win64.setChecked(False)
            self.ui.ch_win64.setEnabled(False)
            self.ui.ch_win64.setVisible(False)
            self.ui.ico_win32.hide()
            self.ui.ico_win64.hide()
            self.ui.label_win32.hide()
            self.ui.label_win64.hide()

        # Disable non-supported features
        features = gCarla.utils.get_supported_features() if gCarla.utils else ()

        if "sf2" not in features:
            self.ui.ch_sf2.setChecked(False)
            self.ui.ch_sf2.setEnabled(False)

        if MACOS and "juce" not in features:
            self.ui.ch_au.setChecked(False)
            self.ui.ch_au.setEnabled(False)

        # -------------------------------------------------------------------------------------------------------------
        # Resize to minimum size, as it's very likely UI stuff was hidden

        self.resize(self.minimumSize())

        # -------------------------------------------------------------------------------------------------------------
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
        self.ui.ch_lv2.clicked.connect(self.slot_checkTools)
        self.ui.ch_vst.clicked.connect(self.slot_checkTools)
        self.ui.ch_vst3.clicked.connect(self.slot_checkTools)
        self.ui.ch_au.clicked.connect(self.slot_checkTools)
        self.ui.ch_sf2.clicked.connect(self.slot_checkTools)
        self.ui.ch_sfz.clicked.connect(self.slot_checkTools)
        self.ui.ch_jsfx.clicked.connect(self.slot_checkTools)
        self.fThread.pluginLook.connect(self.slot_handlePluginLook)
        self.fThread.finished.connect(self.slot_handlePluginThreadFinished)

        # -------------------------------------------------------------------------------------------------------------
        # Post-connect setup

        self.slot_checkTools()

    # -----------------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSafeSettings("falkTX", "CarlaRefresh2")

        check = settings.value("PluginDatabase/SearchLADSPA", True, bool) and self.ui.ch_ladspa.isEnabled()
        self.ui.ch_ladspa.setChecked(check)

        check = settings.value("PluginDatabase/SearchDSSI", True, bool) and self.ui.ch_dssi.isEnabled()
        self.ui.ch_dssi.setChecked(check)

        check = settings.value("PluginDatabase/SearchLV2", True, bool) and self.ui.ch_lv2.isEnabled()
        self.ui.ch_lv2.setChecked(check)

        check = settings.value("PluginDatabase/SearchVST2", True, bool) and self.ui.ch_vst.isEnabled()
        self.ui.ch_vst.setChecked(check)

        check = settings.value("PluginDatabase/SearchVST3", True, bool) and self.ui.ch_vst3.isEnabled()
        self.ui.ch_vst3.setChecked(check)

        if MACOS:
            check = settings.value("PluginDatabase/SearchAU", True, bool) and self.ui.ch_au.isEnabled()
        else:
            check = False
        self.ui.ch_au.setChecked(check)

        check = settings.value("PluginDatabase/SearchSF2", False, bool) and self.ui.ch_sf2.isEnabled()
        self.ui.ch_sf2.setChecked(check)

        check = settings.value("PluginDatabase/SearchSFZ", False, bool) and self.ui.ch_sfz.isEnabled()
        self.ui.ch_sfz.setChecked(check)

        check = settings.value("PluginDatabase/SearchJSFX", True, bool) and self.ui.ch_jsfx.isEnabled()
        self.ui.ch_jsfx.setChecked(check)

        check = settings.value("PluginDatabase/SearchNative", True, bool) and self.ui.ch_native.isEnabled()
        self.ui.ch_native.setChecked(check)

        check = settings.value("PluginDatabase/SearchPOSIX32", False, bool) and self.ui.ch_posix32.isEnabled()
        self.ui.ch_posix32.setChecked(check)

        check = settings.value("PluginDatabase/SearchPOSIX64", False, bool) and self.ui.ch_posix64.isEnabled()
        self.ui.ch_posix64.setChecked(check)

        check = settings.value("PluginDatabase/SearchWin32", False, bool) and self.ui.ch_win32.isEnabled()
        self.ui.ch_win32.setChecked(check)

        check = settings.value("PluginDatabase/SearchWin64", False, bool) and self.ui.ch_win64.isEnabled()
        self.ui.ch_win64.setChecked(check)

        self.ui.ch_do_checks.setChecked(settings.value("PluginDatabase/DoChecks", False, bool))

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSafeSettings("falkTX", "CarlaRefresh2")
        settings.setValue("PluginDatabase/SearchLADSPA", self.ui.ch_ladspa.isChecked())
        settings.setValue("PluginDatabase/SearchDSSI", self.ui.ch_dssi.isChecked())
        settings.setValue("PluginDatabase/SearchLV2", self.ui.ch_lv2.isChecked())
        settings.setValue("PluginDatabase/SearchVST2", self.ui.ch_vst.isChecked())
        settings.setValue("PluginDatabase/SearchVST3", self.ui.ch_vst3.isChecked())
        settings.setValue("PluginDatabase/SearchAU", self.ui.ch_au.isChecked())
        settings.setValue("PluginDatabase/SearchSF2", self.ui.ch_sf2.isChecked())
        settings.setValue("PluginDatabase/SearchSFZ", self.ui.ch_sfz.isChecked())
        settings.setValue("PluginDatabase/SearchJSFX", self.ui.ch_jsfx.isChecked())
        settings.setValue("PluginDatabase/SearchNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX32", self.ui.ch_posix32.isChecked())
        settings.setValue("PluginDatabase/SearchPOSIX64", self.ui.ch_posix64.isChecked())
        settings.setValue("PluginDatabase/SearchWin32", self.ui.ch_win32.isChecked())
        settings.setValue("PluginDatabase/SearchWin64", self.ui.ch_win64.isChecked())
        settings.setValue("PluginDatabase/DoChecks", self.ui.ch_do_checks.isChecked())

    # -----------------------------------------------------------------------------------------------------------------

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

        if gCarla.utils:
            if self.ui.ch_do_checks.isChecked():
                gCarla.utils.unsetenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")
            else:
                gCarla.utils.setenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS", "true")

        native, posix32, posix64, win32, win64 = (self.ui.ch_native.isChecked(),
                                                  self.ui.ch_posix32.isChecked(), self.ui.ch_posix64.isChecked(),
                                                  self.ui.ch_win32.isChecked(), self.ui.ch_win64.isChecked())

        ladspa, dssi, lv2, vst, vst3, au, sf2, sfz, jsfx = (self.ui.ch_ladspa.isChecked(), self.ui.ch_dssi.isChecked(),
                                                            self.ui.ch_lv2.isChecked(), self.ui.ch_vst.isChecked(),
                                                            self.ui.ch_vst3.isChecked(), self.ui.ch_au.isChecked(),
                                                            self.ui.ch_sf2.isChecked(), self.ui.ch_sfz.isChecked(),
                                                            self.ui.ch_jsfx.isChecked())

        self.fThread.setSearchBinaryTypes(native, posix32, posix64, win32, win64)
        self.fThread.setSearchPluginTypes(ladspa, dssi, lv2, vst, vst3, au, sf2, sfz, jsfx)
        self.fThread.start()

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_skip(self):
        killDiscovery()

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_checkTools(self):
        enabled1 = bool(self.ui.ch_native.isChecked() or
                        self.ui.ch_posix32.isChecked() or self.ui.ch_posix64.isChecked() or
                        self.ui.ch_win32.isChecked() or self.ui.ch_win64.isChecked())

        enabled2 = bool(self.ui.ch_ladspa.isChecked() or self.ui.ch_dssi.isChecked() or
                        self.ui.ch_lv2.isChecked() or self.ui.ch_vst.isChecked() or
                        self.ui.ch_vst3.isChecked() or self.ui.ch_au.isChecked() or
                        self.ui.ch_sf2.isChecked() or self.ui.ch_sfz.isChecked() or
                        self.ui.ch_jsfx.isChecked())

        self.ui.b_start.setEnabled(enabled1 and enabled2)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot(float, str)
    def slot_handlePluginLook(self, percent, plugin):
        self.ui.progressBar.setFormat(plugin)
        self.ui.progressBar.setValue(int(percent))

    # -----------------------------------------------------------------------------------------------------------------

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

    # -----------------------------------------------------------------------------------------------------------------

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

# ---------------------------------------------------------------------------------------------------------------------
# Testing

if __name__ == '__main__':
    import sys
    # pylint: disable=ungrouped-imports
    from PyQt5.QtWidgets import QApplication
    # pylint: enable=ungrouped-imports

    class _host:
        pathBinaries = ""
        showPluginBridges = False
        showWineBridges = False
    _host = _host()

    _app = QApplication(sys.argv)
    _gui = PluginRefreshW(None, _host, True, False)
    _app.exec_()

# ---------------------------------------------------------------------------------------------------------------------
