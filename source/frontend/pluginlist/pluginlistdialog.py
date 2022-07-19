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

from PyQt5.QtCore import pyqtSlot, Qt, QByteArray, QEventLoop
from PyQt5.QtWidgets import QApplication, QDialog, QHeaderView, QTableWidgetItem, QWidget

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Carla)

from carla_backend import (
    BINARY_NATIVE,
    BINARY_OTHER,
    BINARY_POSIX32,
    BINARY_POSIX64,
    BINARY_WIN32,
    BINARY_WIN64,
    PLUGIN_AU,
    PLUGIN_DSSI,
    PLUGIN_HAS_CUSTOM_UI,
    PLUGIN_HAS_INLINE_DISPLAY,
    PLUGIN_INTERNAL,
    PLUGIN_IS_BRIDGE,
    PLUGIN_IS_RTSAFE,
    PLUGIN_IS_SYNTH,
    PLUGIN_JSFX,
    PLUGIN_LADSPA,
    PLUGIN_LV2,
    PLUGIN_SF2,
    PLUGIN_SFZ,
    PLUGIN_VST2,
    PLUGIN_VST3,
)

from carla_shared import (
    CARLA_DEFAULT_LV2_PATH,
    CARLA_KEY_PATHS_LV2,
    HAIKU,
    LINUX,
    MACOS,
    WINDOWS,
    fontMetricsHorizontalAdvance,
    gCarla,
    getIcon,
    kIs64bit,
    splitter,
)

from carla_utils import getPluginTypeAsString

from utils import QSafeSettings

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Local)

from .discovery import PLUGIN_QUERY_API_VERSION, checkPluginCached
from .pluginlistdialog_ui import Ui_PluginDatabaseW
from .pluginlistrefreshdialog import PluginRefreshW

# ---------------------------------------------------------------------------------------------------------------------
# Plugin Database Dialog

class PluginDatabaseW(QDialog):
    TABLEWIDGET_ITEM_FAVORITE = 0
    TABLEWIDGET_ITEM_NAME     = 1
    TABLEWIDGET_ITEM_LABEL    = 2
    TABLEWIDGET_ITEM_MAKER    = 3
    TABLEWIDGET_ITEM_BINARY   = 4

    def __init__(self, parent: QWidget, host, useSystemIcons: bool):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = Ui_PluginDatabaseW()
        self.ui.setupUi(self)

        # To be changed by parent
        self.hasLoadedLv2Plugins = False

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fLastTableIndex = 0
        self.fRetPlugin  = None
        self.fRealParent = parent
        self.fFavoritePlugins = []
        self.fFavoritePluginsChanged = False
        self.fUseSystemIcons = useSystemIcons

        self.fTrYes    = self.tr("Yes")
        self.fTrNo     = self.tr("No")
        self.fTrNative = self.tr("Native")

        # ----------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.ui.b_add.setEnabled(False)
        self.addAction(self.ui.act_focus_search)
        self.ui.act_focus_search.triggered.connect(self.slot_focusSearchFieldAndSelectAll)

        if BINARY_NATIVE in (BINARY_POSIX32, BINARY_WIN32):
            self.ui.ch_bridged.setText(self.tr("Bridged (64bit)"))
        else:
            self.ui.ch_bridged.setText(self.tr("Bridged (32bit)"))

        if not (LINUX or MACOS):
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)

        if MACOS:
            self.setWindowModality(Qt.WindowModal)
        else:
            self.ui.ch_au.setChecked(False)
            self.ui.ch_au.setEnabled(False)
            self.ui.ch_au.setVisible(False)

        self.ui.tab_info.tabBar().hide()
        self.ui.tab_reqs.tabBar().hide()
        # FIXME, why /2 needed?
        self.ui.tab_info.setMinimumWidth(int(self.ui.la_id.width()/2) +
                                         fontMetricsHorizontalAdvance(self.ui.l_id.fontMetrics(), "9999999999") + 6*3)
        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        # ----------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # ----------------------------------------------------------------------------------------------------
        # Disable bridges if not enabled in settings

        # NOTE: We Assume win32 carla build will not run win64 plugins
        if (WINDOWS and not kIs64bit) or not host.showPluginBridges:
            self.ui.ch_native.setChecked(True)
            self.ui.ch_native.setEnabled(False)
            self.ui.ch_native.setVisible(True)
            self.ui.ch_bridged.setChecked(False)
            self.ui.ch_bridged.setEnabled(False)
            self.ui.ch_bridged.setVisible(False)
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)
            self.ui.ch_bridged_wine.setVisible(False)
            self.ui.l_arch.setVisible(False)

        elif not host.showWineBridges:
            self.ui.ch_bridged_wine.setChecked(False)
            self.ui.ch_bridged_wine.setEnabled(False)
            self.ui.ch_bridged_wine.setVisible(False)

        # ----------------------------------------------------------------------------------------------------
        # Set-up Icons

        if useSystemIcons:
            self.ui.b_add.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.b_cancel.setIcon(getIcon('dialog-cancel', 16, 'svgz'))
            self.ui.b_clear_filters.setIcon(getIcon('edit-clear', 16, 'svgz'))
            self.ui.b_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
            hhi = self.ui.tableWidget.horizontalHeaderItem(self.TABLEWIDGET_ITEM_FAVORITE)
            hhi.setIcon(getIcon('bookmarks', 16, 'svgz'))

        # ----------------------------------------------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_saveSettings)
        self.ui.b_add.clicked.connect(self.slot_addPlugin)
        self.ui.b_cancel.clicked.connect(self.reject)
        self.ui.b_refresh.clicked.connect(self.slot_refreshPlugins)
        self.ui.b_clear_filters.clicked.connect(self.slot_clearFilters)
        self.ui.lineEdit.textChanged.connect(self.slot_checkFilters)
        self.ui.tableWidget.currentCellChanged.connect(self.slot_checkPlugin)
        self.ui.tableWidget.cellClicked.connect(self.slot_cellClicked)
        self.ui.tableWidget.cellDoubleClicked.connect(self.slot_cellDoubleClicked)

        self.ui.ch_internal.clicked.connect(self.slot_checkFilters)
        self.ui.ch_ladspa.clicked.connect(self.slot_checkFilters)
        self.ui.ch_dssi.clicked.connect(self.slot_checkFilters)
        self.ui.ch_lv2.clicked.connect(self.slot_checkFilters)
        self.ui.ch_vst.clicked.connect(self.slot_checkFilters)
        self.ui.ch_vst3.clicked.connect(self.slot_checkFilters)
        self.ui.ch_au.clicked.connect(self.slot_checkFilters)
        self.ui.ch_jsfx.clicked.connect(self.slot_checkFilters)
        self.ui.ch_kits.clicked.connect(self.slot_checkFilters)
        self.ui.ch_effects.clicked.connect(self.slot_checkFilters)
        self.ui.ch_instruments.clicked.connect(self.slot_checkFilters)
        self.ui.ch_midi.clicked.connect(self.slot_checkFilters)
        self.ui.ch_other.clicked.connect(self.slot_checkFilters)
        self.ui.ch_native.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged.clicked.connect(self.slot_checkFilters)
        self.ui.ch_bridged_wine.clicked.connect(self.slot_checkFilters)
        self.ui.ch_favorites.clicked.connect(self.slot_checkFilters)
        self.ui.ch_rtsafe.clicked.connect(self.slot_checkFilters)
        self.ui.ch_cv.clicked.connect(self.slot_checkFilters)
        self.ui.ch_gui.clicked.connect(self.slot_checkFilters)
        self.ui.ch_inline_display.clicked.connect(self.slot_checkFilters)
        self.ui.ch_stereo.clicked.connect(self.slot_checkFilters)
        self.ui.ch_cat_all.clicked.connect(self.slot_checkFiltersCategoryAll)
        self.ui.ch_cat_delay.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_distortion.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_dynamics.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_eq.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_filter.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_modulator.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_synth.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_utility.clicked.connect(self.slot_checkFiltersCategorySpecific)
        self.ui.ch_cat_other.clicked.connect(self.slot_checkFiltersCategorySpecific)

        # ----------------------------------------------------------------------------------------------------
        # Post-connect setup

        self._reAddPlugins()
        self.slot_focusSearchFieldAndSelectAll()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_cellClicked(self, row, column):
        if column == self.TABLEWIDGET_ITEM_FAVORITE:
            widget = self.ui.tableWidget.item(row, self.TABLEWIDGET_ITEM_FAVORITE)
            plugin = self.ui.tableWidget.item(row, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1)
            plugin = self._createFavoritePluginDict(plugin)

            if widget.checkState() == Qt.Checked:
                if not plugin in self.fFavoritePlugins:
                    self.fFavoritePlugins.append(plugin)
                    self.fFavoritePluginsChanged = True
            else:
                try:
                    self.fFavoritePlugins.remove(plugin)
                    self.fFavoritePluginsChanged = True
                except ValueError:
                    pass

    @pyqtSlot(int, int)
    def slot_cellDoubleClicked(self, _, column):
        if column != self.TABLEWIDGET_ITEM_FAVORITE:
            self.slot_addPlugin()

    @pyqtSlot()
    def slot_focusSearchFieldAndSelectAll(self):
        self.ui.lineEdit.setFocus()
        self.ui.lineEdit.selectAll()

    @pyqtSlot()
    def slot_addPlugin(self):
        if self.ui.tableWidget.currentRow() >= 0:
            self.fRetPlugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(),
                                                       self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1)
            self.accept()
        else:
            self.reject()

    @pyqtSlot(int)
    def slot_checkPlugin(self, row):
        if row >= 0:
            self.ui.b_add.setEnabled(True)
            plugin = self.ui.tableWidget.item(self.ui.tableWidget.currentRow(),
                                              self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1)

            isSynth  = bool(plugin['hints'] & PLUGIN_IS_SYNTH)
            isEffect = bool(plugin['audio.ins'] > 0 < plugin['audio.outs'] and not isSynth)
            isMidi   = bool(plugin['audio.ins'] == 0 and
                            plugin['audio.outs'] == 0 and
                            plugin['midi.ins'] > 0 < plugin['midi.outs'])
            # isKit    = bool(plugin['type'] in (PLUGIN_SF2, PLUGIN_SFZ))
            # isOther  = bool(not (isEffect or isSynth or isMidi or isKit))

            if isSynth:
                ptype = "Instrument"
            elif isEffect:
                ptype = "Effect"
            elif isMidi:
                ptype = "MIDI Plugin"
            else:
                ptype = "Other"

            if plugin['build'] == BINARY_NATIVE:
                parch = self.fTrNative
            elif plugin['build'] == BINARY_POSIX32:
                parch = "posix32"
            elif plugin['build'] == BINARY_POSIX64:
                parch = "posix64"
            elif plugin['build'] == BINARY_WIN32:
                parch = "win32"
            elif plugin['build'] == BINARY_WIN64:
                parch = "win64"
            elif plugin['build'] == BINARY_OTHER:
                parch = self.tr("Other")
            elif plugin['build'] == BINARY_WIN32:
                parch = self.tr("Unknown")

            self.ui.l_format.setText(getPluginTypeAsString(plugin['type']))
            self.ui.l_type.setText(ptype)
            self.ui.l_arch.setText(parch)
            self.ui.l_id.setText(str(plugin['uniqueId']))
            self.ui.l_ains.setText(str(plugin['audio.ins']))
            self.ui.l_aouts.setText(str(plugin['audio.outs']))
            self.ui.l_cvins.setText(str(plugin['cv.ins']))
            self.ui.l_cvouts.setText(str(plugin['cv.outs']))
            self.ui.l_mins.setText(str(plugin['midi.ins']))
            self.ui.l_mouts.setText(str(plugin['midi.outs']))
            self.ui.l_pins.setText(str(plugin['parameters.ins']))
            self.ui.l_pouts.setText(str(plugin['parameters.outs']))
            self.ui.l_gui.setText(self.fTrYes if plugin['hints'] & PLUGIN_HAS_CUSTOM_UI else self.fTrNo)
            self.ui.l_idisp.setText(self.fTrYes if plugin['hints'] & PLUGIN_HAS_INLINE_DISPLAY else self.fTrNo)
            self.ui.l_bridged.setText(self.fTrYes if plugin['hints'] & PLUGIN_IS_BRIDGE else self.fTrNo)
            self.ui.l_synth.setText(self.fTrYes if isSynth else self.fTrNo)
        else:
            self.ui.b_add.setEnabled(False)
            self.ui.l_format.setText("---")
            self.ui.l_type.setText("---")
            self.ui.l_arch.setText("---")
            self.ui.l_id.setText("---")
            self.ui.l_ains.setText("---")
            self.ui.l_aouts.setText("---")
            self.ui.l_cvins.setText("---")
            self.ui.l_cvouts.setText("---")
            self.ui.l_mins.setText("---")
            self.ui.l_mouts.setText("---")
            self.ui.l_pins.setText("---")
            self.ui.l_pouts.setText("---")
            self.ui.l_gui.setText("---")
            self.ui.l_idisp.setText("---")
            self.ui.l_bridged.setText("---")
            self.ui.l_synth.setText("---")

    @pyqtSlot()
    def slot_checkFilters(self):
        self._checkFilters()

    @pyqtSlot(bool)
    def slot_checkFiltersCategoryAll(self, clicked):
        self.ui.ch_cat_delay.setChecked(not clicked)
        self.ui.ch_cat_distortion.setChecked(not clicked)
        self.ui.ch_cat_dynamics.setChecked(not clicked)
        self.ui.ch_cat_eq.setChecked(not clicked)
        self.ui.ch_cat_filter.setChecked(not clicked)
        self.ui.ch_cat_modulator.setChecked(not clicked)
        self.ui.ch_cat_synth.setChecked(not clicked)
        self.ui.ch_cat_utility.setChecked(not clicked)
        self.ui.ch_cat_other.setChecked(not clicked)
        self._checkFilters()

    @pyqtSlot(bool)
    def slot_checkFiltersCategorySpecific(self, clicked):
        if clicked:
            self.ui.ch_cat_all.setChecked(False)
        elif not (self.ui.ch_cat_delay.isChecked() or
                  self.ui.ch_cat_distortion.isChecked() or
                  self.ui.ch_cat_dynamics.isChecked() or
                  self.ui.ch_cat_eq.isChecked() or
                  self.ui.ch_cat_filter.isChecked() or
                  self.ui.ch_cat_modulator.isChecked() or
                  self.ui.ch_cat_synth.isChecked() or
                  self.ui.ch_cat_utility.isChecked() or
                  self.ui.ch_cat_other.isChecked()):
            self.ui.ch_cat_all.setChecked(True)
        self._checkFilters()

    @pyqtSlot()
    def slot_refreshPlugins(self):
        if PluginRefreshW(self, self.host, self.fUseSystemIcons, self.hasLoadedLv2Plugins).exec_():
            self._reAddPlugins()

            if self.fRealParent:
                self.fRealParent.setLoadRDFsNeeded()

    @pyqtSlot()
    def slot_clearFilters(self):
        self.blockSignals(True)

        self.ui.ch_internal.setChecked(True)
        self.ui.ch_ladspa.setChecked(True)
        self.ui.ch_dssi.setChecked(True)
        self.ui.ch_lv2.setChecked(True)
        self.ui.ch_vst.setChecked(True)
        self.ui.ch_jsfx.setChecked(True)
        self.ui.ch_kits.setChecked(True)

        self.ui.ch_instruments.setChecked(True)
        self.ui.ch_effects.setChecked(True)
        self.ui.ch_midi.setChecked(True)
        self.ui.ch_other.setChecked(True)

        self.ui.ch_native.setChecked(True)
        self.ui.ch_bridged.setChecked(False)
        self.ui.ch_bridged_wine.setChecked(False)

        self.ui.ch_favorites.setChecked(False)
        self.ui.ch_rtsafe.setChecked(False)
        self.ui.ch_stereo.setChecked(False)
        self.ui.ch_cv.setChecked(False)
        self.ui.ch_gui.setChecked(False)
        self.ui.ch_inline_display.setChecked(False)

        if self.ui.ch_vst3.isEnabled():
            self.ui.ch_vst3.setChecked(True)
        if self.ui.ch_au.isEnabled():
            self.ui.ch_au.setChecked(True)

        self.ui.ch_cat_all.setChecked(True)
        self.ui.ch_cat_delay.setChecked(False)
        self.ui.ch_cat_distortion.setChecked(False)
        self.ui.ch_cat_dynamics.setChecked(False)
        self.ui.ch_cat_eq.setChecked(False)
        self.ui.ch_cat_filter.setChecked(False)
        self.ui.ch_cat_modulator.setChecked(False)
        self.ui.ch_cat_synth.setChecked(False)
        self.ui.ch_cat_utility.setChecked(False)
        self.ui.ch_cat_other.setChecked(False)

        self.ui.lineEdit.clear()

        self.blockSignals(False)

        self._checkFilters()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSafeSettings("falkTX", "CarlaDatabase2")
        settings.setValue("PluginDatabase/Geometry", self.saveGeometry())
        settings.setValue("PluginDatabase/TableGeometry_6", self.ui.tableWidget.horizontalHeader().saveState())
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
        settings.setValue("PluginDatabase/ShowJSFX", self.ui.ch_jsfx.isChecked())
        settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits.isChecked())
        settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native.isChecked())
        settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged.isChecked())
        settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine.isChecked())
        settings.setValue("PluginDatabase/ShowFavorites", self.ui.ch_favorites.isChecked())
        settings.setValue("PluginDatabase/ShowRtSafe", self.ui.ch_rtsafe.isChecked())
        settings.setValue("PluginDatabase/ShowHasCV", self.ui.ch_cv.isChecked())
        settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui.isChecked())
        settings.setValue("PluginDatabase/ShowHasInlineDisplay", self.ui.ch_inline_display.isChecked())
        settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo.isChecked())
        settings.setValue("PluginDatabase/SearchText", self.ui.lineEdit.text())

        if self.ui.ch_cat_all.isChecked():
            settings.setValue("PluginDatabase/ShowCategory", "all")
        else:
            categoryhash = ""
            if self.ui.ch_cat_delay.isChecked():
                categoryhash += ":delay"
            if self.ui.ch_cat_distortion.isChecked():
                categoryhash += ":distortion"
            if self.ui.ch_cat_dynamics.isChecked():
                categoryhash += ":dynamics"
            if self.ui.ch_cat_eq.isChecked():
                categoryhash += ":eq"
            if self.ui.ch_cat_filter.isChecked():
                categoryhash += ":filter"
            if self.ui.ch_cat_modulator.isChecked():
                categoryhash += ":modulator"
            if self.ui.ch_cat_synth.isChecked():
                categoryhash += ":synth"
            if self.ui.ch_cat_utility.isChecked():
                categoryhash += ":utility"
            if self.ui.ch_cat_other.isChecked():
                categoryhash += ":other"
            if categoryhash:
                categoryhash += ":"
            settings.setValue("PluginDatabase/ShowCategory", categoryhash)

        if self.fFavoritePluginsChanged:
            settings.setValue("PluginDatabase/Favorites", self.fFavoritePlugins)

    # --------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSafeSettings("falkTX", "CarlaDatabase2")
        self.fFavoritePlugins = settings.value("PluginDatabase/Favorites", [], list)
        self.fFavoritePluginsChanged = False

        self.restoreGeometry(settings.value("PluginDatabase/Geometry", QByteArray(), QByteArray))
        self.ui.ch_effects.setChecked(settings.value("PluginDatabase/ShowEffects", True, bool))
        self.ui.ch_instruments.setChecked(settings.value("PluginDatabase/ShowInstruments", True, bool))
        self.ui.ch_midi.setChecked(settings.value("PluginDatabase/ShowMIDI", True, bool))
        self.ui.ch_other.setChecked(settings.value("PluginDatabase/ShowOther", True, bool))
        self.ui.ch_internal.setChecked(settings.value("PluginDatabase/ShowInternal", True, bool))
        self.ui.ch_ladspa.setChecked(settings.value("PluginDatabase/ShowLADSPA", True, bool))
        self.ui.ch_dssi.setChecked(settings.value("PluginDatabase/ShowDSSI", True, bool))
        self.ui.ch_lv2.setChecked(settings.value("PluginDatabase/ShowLV2", True, bool))
        self.ui.ch_vst.setChecked(settings.value("PluginDatabase/ShowVST2", True, bool))
        self.ui.ch_vst3.setChecked(settings.value("PluginDatabase/ShowVST3", (MACOS or WINDOWS), bool))
        self.ui.ch_au.setChecked(settings.value("PluginDatabase/ShowAU", MACOS, bool))
        self.ui.ch_jsfx.setChecked(settings.value("PluginDatabase/ShowJSFX", True, bool))
        self.ui.ch_kits.setChecked(settings.value("PluginDatabase/ShowKits", True, bool))
        self.ui.ch_native.setChecked(settings.value("PluginDatabase/ShowNative", True, bool))
        self.ui.ch_bridged.setChecked(settings.value("PluginDatabase/ShowBridged", True, bool))
        self.ui.ch_bridged_wine.setChecked(settings.value("PluginDatabase/ShowBridgedWine", True, bool))
        self.ui.ch_favorites.setChecked(settings.value("PluginDatabase/ShowFavorites", False, bool))
        self.ui.ch_rtsafe.setChecked(settings.value("PluginDatabase/ShowRtSafe", False, bool))
        self.ui.ch_cv.setChecked(settings.value("PluginDatabase/ShowHasCV", False, bool))
        self.ui.ch_gui.setChecked(settings.value("PluginDatabase/ShowHasGUI", False, bool))
        self.ui.ch_inline_display.setChecked(settings.value("PluginDatabase/ShowHasInlineDisplay", False, bool))
        self.ui.ch_stereo.setChecked(settings.value("PluginDatabase/ShowStereoOnly", False, bool))
        self.ui.lineEdit.setText(settings.value("PluginDatabase/SearchText", "", str))

        categoryhash = settings.value("PluginDatabase/ShowCategory", "all", str)
        if categoryhash == "all" or len(categoryhash) < 2:
            self.ui.ch_cat_all.setChecked(True)
            self.ui.ch_cat_delay.setChecked(False)
            self.ui.ch_cat_distortion.setChecked(False)
            self.ui.ch_cat_dynamics.setChecked(False)
            self.ui.ch_cat_eq.setChecked(False)
            self.ui.ch_cat_filter.setChecked(False)
            self.ui.ch_cat_modulator.setChecked(False)
            self.ui.ch_cat_synth.setChecked(False)
            self.ui.ch_cat_utility.setChecked(False)
            self.ui.ch_cat_other.setChecked(False)
        else:
            self.ui.ch_cat_all.setChecked(False)
            self.ui.ch_cat_delay.setChecked(":delay:" in categoryhash)
            self.ui.ch_cat_distortion.setChecked(":distortion:" in categoryhash)
            self.ui.ch_cat_dynamics.setChecked(":dynamics:" in categoryhash)
            self.ui.ch_cat_eq.setChecked(":eq:" in categoryhash)
            self.ui.ch_cat_filter.setChecked(":filter:" in categoryhash)
            self.ui.ch_cat_modulator.setChecked(":modulator:" in categoryhash)
            self.ui.ch_cat_synth.setChecked(":synth:" in categoryhash)
            self.ui.ch_cat_utility.setChecked(":utility:" in categoryhash)
            self.ui.ch_cat_other.setChecked(":other:" in categoryhash)

        tableGeometry = settings.value("PluginDatabase/TableGeometry_6", QByteArray(), QByteArray)
        horizontalHeader = self.ui.tableWidget.horizontalHeader()
        if not tableGeometry.isNull():
            horizontalHeader.restoreState(tableGeometry)
        else:
            horizontalHeader.setSectionResizeMode(self.TABLEWIDGET_ITEM_FAVORITE, QHeaderView.Fixed)
            self.ui.tableWidget.setColumnWidth(self.TABLEWIDGET_ITEM_FAVORITE, 24)
            self.ui.tableWidget.setColumnWidth(self.TABLEWIDGET_ITEM_NAME, 250)
            self.ui.tableWidget.setColumnWidth(self.TABLEWIDGET_ITEM_LABEL, 200)
            self.ui.tableWidget.setColumnWidth(self.TABLEWIDGET_ITEM_MAKER, 150)
            self.ui.tableWidget.sortByColumn(self.TABLEWIDGET_ITEM_NAME, Qt.AscendingOrder)

    # --------------------------------------------------------------------------------------------------------

    def _createFavoritePluginDict(self, plugin):
        return {
            'name'    : plugin['name'],
            'build'   : plugin['build'],
            'type'    : plugin['type'],
            'filename': plugin['filename'],
            'label'   : plugin['label'],
            'uniqueId': plugin['uniqueId'],
        }

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
        hideJSFX     = not self.ui.ch_jsfx.isChecked()
        hideKits     = not self.ui.ch_kits.isChecked()

        hideNative  = not self.ui.ch_native.isChecked()
        hideBridged = not self.ui.ch_bridged.isChecked()
        hideBridgedWine = not self.ui.ch_bridged_wine.isChecked()

        hideNonFavs   = self.ui.ch_favorites.isChecked()
        hideNonRtSafe = self.ui.ch_rtsafe.isChecked()
        hideNonCV     = self.ui.ch_cv.isChecked()
        hideNonGui    = self.ui.ch_gui.isChecked()
        hideNonIDisp  = self.ui.ch_inline_display.isChecked()
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

        self.ui.tableWidget.setRowCount(self.fLastTableIndex)

        for i in range(self.fLastTableIndex):
            plugin = self.ui.tableWidget.item(i, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+1)
            ptext  = self.ui.tableWidget.item(i, self.TABLEWIDGET_ITEM_NAME).data(Qt.UserRole+2)
            aIns   = plugin['audio.ins']
            aOuts  = plugin['audio.outs']
            cvIns  = plugin['cv.ins']
            cvOuts = plugin['cv.outs']
            mIns   = plugin['midi.ins']
            mOuts  = plugin['midi.outs']
            phints = plugin['hints']
            ptype  = plugin['type']
            categ  = plugin['category']
            isSynth  = bool(phints & PLUGIN_IS_SYNTH)
            isEffect = bool(aIns > 0 < aOuts and not isSynth)
            isMidi   = bool(aIns == 0 and aOuts == 0 and mIns > 0 < mOuts)
            isKit    = bool(ptype in (PLUGIN_SF2, PLUGIN_SFZ))
            isOther  = bool(not (isEffect or isSynth or isMidi or isKit))
            isNative = bool(plugin['build'] == BINARY_NATIVE)
            isRtSafe = bool(phints & PLUGIN_IS_RTSAFE)
            isStereo = bool(aIns == 2 and aOuts == 2) or (isSynth and aOuts == 2)
            hasCV    = bool(cvIns + cvOuts > 0)
            hasGui   = bool(phints & PLUGIN_HAS_CUSTOM_UI)
            hasIDisp = bool(phints & PLUGIN_HAS_INLINE_DISPLAY)

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
            elif hideInternal and ptype == PLUGIN_INTERNAL:
                self.ui.tableWidget.hideRow(i)
            elif hideLadspa and ptype == PLUGIN_LADSPA:
                self.ui.tableWidget.hideRow(i)
            elif hideDssi and ptype == PLUGIN_DSSI:
                self.ui.tableWidget.hideRow(i)
            elif hideLV2 and ptype == PLUGIN_LV2:
                self.ui.tableWidget.hideRow(i)
            elif hideVST2 and ptype == PLUGIN_VST2:
                self.ui.tableWidget.hideRow(i)
            elif hideVST3 and ptype == PLUGIN_VST3:
                self.ui.tableWidget.hideRow(i)
            elif hideAU and ptype == PLUGIN_AU:
                self.ui.tableWidget.hideRow(i)
            elif hideJSFX and ptype == PLUGIN_JSFX:
                self.ui.tableWidget.hideRow(i)
            elif hideNative and isNative:
                self.ui.tableWidget.hideRow(i)
            elif hideBridged and isBridged:
                self.ui.tableWidget.hideRow(i)
            elif hideBridgedWine and isBridgedWine:
                self.ui.tableWidget.hideRow(i)
            elif hideNonRtSafe and not isRtSafe:
                self.ui.tableWidget.hideRow(i)
            elif hideNonCV and not hasCV:
                self.ui.tableWidget.hideRow(i)
            elif hideNonGui and not hasGui:
                self.ui.tableWidget.hideRow(i)
            elif hideNonIDisp and not hasIDisp:
                self.ui.tableWidget.hideRow(i)
            elif hideNonStereo and not isStereo:
                self.ui.tableWidget.hideRow(i)
            elif text and not all(t in ptext for t in text.strip().split(' ')):
                self.ui.tableWidget.hideRow(i)
            elif hideNonFavs and self._createFavoritePluginDict(plugin) not in self.fFavoritePlugins:
                self.ui.tableWidget.hideRow(i)
            elif (self.ui.ch_cat_all.isChecked() or
                  (self.ui.ch_cat_delay.isChecked() and categ == "delay") or
                  (self.ui.ch_cat_distortion.isChecked() and categ == "distortion") or
                  (self.ui.ch_cat_dynamics.isChecked() and categ == "dynamics") or
                  (self.ui.ch_cat_eq.isChecked() and categ == "eq") or
                  (self.ui.ch_cat_filter.isChecked() and categ == "filter") or
                  (self.ui.ch_cat_modulator.isChecked() and categ == "modulator") or
                  (self.ui.ch_cat_synth.isChecked() and categ == "synth") or
                  (self.ui.ch_cat_utility.isChecked() and categ == "utility") or
                  (self.ui.ch_cat_other.isChecked() and categ == "other")):
                self.ui.tableWidget.showRow(i)
            else:
                self.ui.tableWidget.hideRow(i)

    # --------------------------------------------------------------------------------------------------------

    def _addPluginToTable(self, plugin, ptype):
        if plugin['API'] != PLUGIN_QUERY_API_VERSION:
            return
        if ptype in (self.tr("Internal"), "LV2", "SF2", "SFZ", "JSFX"):
            plugin['build'] = BINARY_NATIVE

        index = self.fLastTableIndex

        isFav = bool(self._createFavoritePluginDict(plugin) in self.fFavoritePlugins)
        favItem = QTableWidgetItem()
        favItem.setCheckState(Qt.Checked if isFav else Qt.Unchecked)
        favItem.setText(" " if isFav else "  ")

        pluginText = (plugin['name']+plugin['label']+plugin['maker']+plugin['filename']).lower()
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_FAVORITE, favItem)
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_NAME, QTableWidgetItem(plugin['name']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_LABEL, QTableWidgetItem(plugin['label']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_MAKER, QTableWidgetItem(plugin['maker']))
        self.ui.tableWidget.setItem(index, self.TABLEWIDGET_ITEM_BINARY, QTableWidgetItem(os.path.basename(plugin['filename'])))
        self.ui.tableWidget.item(index, self.TABLEWIDGET_ITEM_NAME).setData(Qt.UserRole+1, plugin)
        self.ui.tableWidget.item(index, self.TABLEWIDGET_ITEM_NAME).setData(Qt.UserRole+2, pluginText)

        self.fLastTableIndex += 1

    # --------------------------------------------------------------------------------------------------------

    def _reAddInternalHelper(self, settingsDB, ptype, path):
        if ptype == PLUGIN_INTERNAL:
            ptypeStr   = "Internal"
            ptypeStrTr = self.tr("Internal")
        elif ptype == PLUGIN_LV2:
            ptypeStr   = "LV2"
            ptypeStrTr = ptypeStr
        elif ptype == PLUGIN_AU:
            ptypeStr   = "AU"
            ptypeStrTr = ptypeStr
        #elif ptype == PLUGIN_SFZ:
            #ptypeStr   = "SFZ"
            #ptypeStrTr = ptypeStr
        # TODO(jsfx) what to do here?
        else:
            return 0

        plugins     = settingsDB.value("Plugins/" + ptypeStr, [], list)
        pluginCount = settingsDB.value("PluginCount/" + ptypeStr, 0, int)

        if ptype == PLUGIN_AU:
            gCarla.utils.juce_init()

        pluginCountNew = gCarla.utils.get_cached_plugin_count(ptype, path)

        if pluginCountNew != pluginCount or len(plugins) != pluginCount or (len(plugins) > 0 and plugins[0]['API'] != PLUGIN_QUERY_API_VERSION):
            plugins     = []
            pluginCount = pluginCountNew

            QApplication.processEvents(QEventLoop.ExcludeUserInputEvents, 50)
            if ptype == PLUGIN_AU:
                gCarla.utils.juce_idle()

            for i in range(pluginCountNew):
                descInfo = gCarla.utils.get_cached_plugin_info(ptype, i)

                if not descInfo['valid']:
                    continue

                info = checkPluginCached(descInfo, ptype)

                plugins.append(info)

                if i % 50 == 0:
                    QApplication.processEvents(QEventLoop.ExcludeUserInputEvents, 50)
                    if ptype == PLUGIN_AU:
                        gCarla.utils.juce_idle()

            settingsDB.setValue("Plugins/" + ptypeStr, plugins)
            settingsDB.setValue("PluginCount/" + ptypeStr, pluginCount)

        if ptype == PLUGIN_AU:
            gCarla.utils.juce_cleanup()

        # prepare rows in advance
        self.ui.tableWidget.setRowCount(self.fLastTableIndex + len(plugins))

        for plugin in plugins:
            self._addPluginToTable(plugin, ptypeStrTr)

        return pluginCount

    def _reAddPlugins(self):
        settingsDB = QSafeSettings("falkTX", "CarlaPlugins5")

        self.fLastTableIndex = 0
        self.ui.tableWidget.setSortingEnabled(False)
        self.ui.tableWidget.clearContents()

        settings = QSafeSettings("falkTX", "Carla2")
        LV2_PATH = splitter.join(settings.value(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH, list))
        del settings

        # ----------------------------------------------------------------------------------------------------
        # plugins handled through backend

        internalCount = self._reAddInternalHelper(settingsDB, PLUGIN_INTERNAL, "")
        lv2Count      = self._reAddInternalHelper(settingsDB, PLUGIN_LV2, LV2_PATH)
        auCount       = self._reAddInternalHelper(settingsDB, PLUGIN_AU, "") if MACOS else 0

        # ----------------------------------------------------------------------------------------------------
        # LADSPA

        ladspaPlugins  = []
        ladspaPlugins += settingsDB.value("Plugins/LADSPA_native", [], list)
        ladspaPlugins += settingsDB.value("Plugins/LADSPA_posix32", [], list)
        ladspaPlugins += settingsDB.value("Plugins/LADSPA_posix64", [], list)
        ladspaPlugins += settingsDB.value("Plugins/LADSPA_win32", [], list)
        ladspaPlugins += settingsDB.value("Plugins/LADSPA_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # DSSI

        dssiPlugins  = []
        dssiPlugins += settingsDB.value("Plugins/DSSI_native", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_posix32", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_posix64", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_win32", [], list)
        dssiPlugins += settingsDB.value("Plugins/DSSI_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # VST2

        vst2Plugins  = []
        vst2Plugins += settingsDB.value("Plugins/VST2_native", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_posix32", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_posix64", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_win32", [], list)
        vst2Plugins += settingsDB.value("Plugins/VST2_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # VST3

        vst3Plugins  = []
        vst3Plugins += settingsDB.value("Plugins/VST3_native", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_posix32", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_posix64", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_win32", [], list)
        vst3Plugins += settingsDB.value("Plugins/VST3_win64", [], list)

        # ----------------------------------------------------------------------------------------------------
        # AU (extra non-cached)

        auPlugins32 = settingsDB.value("Plugins/AU_posix32", [], list) if MACOS else []

        # ----------------------------------------------------------------------------------------------------
        # JSFX

        jsfxPlugins = settingsDB.value("Plugins/JSFX", [], list)

        # ----------------------------------------------------------------------------------------------------
        # Kits

        sf2s = settingsDB.value("Plugins/SF2", [], list)
        sfzs = settingsDB.value("Plugins/SFZ", [], list)

        # ----------------------------------------------------------------------------------------------------
        # count plugins first, so we can create rows in advance

        ladspaCount = 0
        dssiCount   = 0
        vstCount    = 0
        vst3Count   = 0
        au32Count   = 0
        jsfxCount   = len(jsfxPlugins)
        sf2Count    = 0
        sfzCount    = len(sfzs)

        for plugins in ladspaPlugins:
            ladspaCount += len(plugins)

        for plugins in dssiPlugins:
            dssiCount += len(plugins)

        for plugins in vst2Plugins:
            vstCount += len(plugins)

        for plugins in vst3Plugins:
            vst3Count += len(plugins)

        for plugins in auPlugins32:
            au32Count += len(plugins)

        for plugins in sf2s:
            sf2Count += len(plugins)

        self.ui.tableWidget.setRowCount(self.fLastTableIndex +
                                        ladspaCount + dssiCount + vstCount + vst3Count + au32Count + jsfxCount +
                                        sf2Count + sfzCount)

        if MACOS:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST2, %i VST3, %i AudioUnit plugins and %i JSFX plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, auCount+au32Count, jsfxCount, sf2Count+sfzCount)))
        else:
            self.ui.label.setText(self.tr("Have %i Internal, %i LADSPA, %i DSSI, %i LV2, %i VST2, %i VST3 plugins and %i JSFX plugins, plus %i Sound Kits" % (
                                          internalCount, ladspaCount, dssiCount, lv2Count, vstCount, vst3Count, jsfxCount, sf2Count+sfzCount)))

        # ----------------------------------------------------------------------------------------------------
        # now add all plugins to the table

        for plugins in ladspaPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "LADSPA")

        for plugins in dssiPlugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "DSSI")

        for plugins in vst2Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST2")

        for plugins in vst3Plugins:
            for plugin in plugins:
                self._addPluginToTable(plugin, "VST3")

        for plugins in auPlugins32:
            for plugin in plugins:
                self._addPluginToTable(plugin, "AU")

        for plugin in jsfxPlugins:
            self._addPluginToTable(plugin, "JSFX")

        for sf2 in sf2s:
            for sf2_i in sf2:
                self._addPluginToTable(sf2_i, "SF2")

        for sfz in sfzs:
            self._addPluginToTable(sfz, "SFZ")

        # ----------------------------------------------------------------------------------------------------

        self.ui.tableWidget.setSortingEnabled(True)
        self._checkFilters()
        self.slot_checkPlugin(self.ui.tableWidget.currentRow())

    # --------------------------------------------------------------------------------------------------------

    def showEvent(self, event):
        self.slot_focusSearchFieldAndSelectAll()
        QDialog.showEvent(self, event)

# ---------------------------------------------------------------------------------------------------------------------
# Testing

if __name__ == '__main__':
    import sys

    class _host:
        pathBinaries = ""
        showPluginBridges = False
        showWineBridges = False
    _host = _host()

    _app = QApplication(sys.argv)
    _gui = PluginDatabaseW(None, _host, True)

    if _gui.exec_():
        print(f"Result: {_gui.fRetPlugin}")

# ---------------------------------------------------------------------------------------------------------------------
