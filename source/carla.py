#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
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
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from time import sleep
from PyQt4.QtCore import Qt, QModelIndex, QPointF, QSize
from PyQt4.QtGui import QApplication, QDialogButtonBox, QFileSystemModel, QMainWindow, QResizeEvent

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas
import ui_carla
import ui_carla_settings
from carla_backend import * # FIXME, remove later
from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Static Variables

DEFAULT_CANVAS_WIDTH  = 3100
DEFAULT_CANVAS_HEIGHT = 2400

# Tab indexes
TAB_INDEX_MAIN         = 0
TAB_INDEX_CANVAS       = 1
TAB_INDEX_CARLA_ENGINE = 2
TAB_INDEX_CARLA_PATHS  = 3
TAB_INDEX_NONE         = 4

# Single and Multiple client mode is only for JACK,
# but we still want to match QComboBox index to defines,
# so add +2 pos padding if driverName != "JACK".
PROCESS_MODE_NON_JACK_PADDING = 2

# Carla defaults
CARLA_DEFAULT_PROCESS_HIGH_PRECISION = False
CARLA_DEFAULT_MAX_PARAMETERS         = 200
CARLA_DEFAULT_FORCE_STEREO           = False
CARLA_DEFAULT_USE_DSSI_VST_CHUNKS    = False
CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES  = False
CARLA_DEFAULT_PREFER_UI_BRIDGES      = True
CARLA_DEFAULT_OSC_UI_TIMEOUT         = 4000
CARLA_DEFAULT_DISABLE_CHECKS         = False

# PatchCanvas defines
CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# ------------------------------------------------------------------------------------------------------------
# Settings Dialog

class CarlaSettingsW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_settings.Ui_CarlaSettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        driverCount = Carla.host.get_engine_driver_count()

        for i in range(driverCount):
            driverName = cString(Carla.host.get_engine_driver_name(i))
            self.ui.cb_engine_audio_driver.addItem(driverName)

        # -------------------------------------------------------------
        # Load settings

        self.loadSettings()

        if not hasGL:
            self.ui.cb_canvas_use_opengl.setChecked(False)
            self.ui.cb_canvas_use_opengl.setEnabled(False)

        if WINDOWS:
            self.ui.ch_engine_dssi_chunks.setChecked(False)
            self.ui.ch_engine_dssi_chunks.setEnabled(False)

        # -------------------------------------------------------------
        # Set-up connections

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        self.connect(self.ui.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

        self.connect(self.ui.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetProjectPath()"))
        self.connect(self.ui.cb_engine_audio_driver, SIGNAL("currentIndexChanged(int)"), SLOT("slot_engineAudioDriverChanged()"))
        self.connect(self.ui.b_paths_add, SIGNAL("clicked()"), SLOT("slot_addPluginPath()"))
        self.connect(self.ui.b_paths_remove, SIGNAL("clicked()"), SLOT("slot_removePluginPath()"))
        self.connect(self.ui.b_paths_change, SIGNAL("clicked()"), SLOT("slot_changePluginPath()"))
        self.connect(self.ui.tw_paths, SIGNAL("currentChanged(int)"), SLOT("slot_pluginPathTabChanged(int)"))
        self.connect(self.ui.lw_ladspa, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.ui.lw_dssi, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.ui.lw_lv2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.ui.lw_vst, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        self.connect(self.ui.lw_sf2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))

        # -------------------------------------------------------------
        # Post-connect setup

        self.ui.lw_ladspa.setCurrentRow(0)
        self.ui.lw_dssi.setCurrentRow(0)
        self.ui.lw_lv2.setCurrentRow(0)
        self.ui.lw_vst.setCurrentRow(0)
        self.ui.lw_gig.setCurrentRow(0)
        self.ui.lw_sf2.setCurrentRow(0)
        self.ui.lw_sfz.setCurrentRow(0)

        self.ui.lw_page.setCurrentCell(0, 0)

    def loadSettings(self):
        settings = QSettings()

        # ---------------------------------------

        self.ui.le_main_def_folder.setText(settings.value("Main/DefaultProjectFolder", HOME, type=str))
        self.ui.sb_gui_refresh.setValue(settings.value("Main/RefreshInterval", 50, type=int))

        # ---------------------------------------

        self.ui.cb_canvas_hide_groups.setChecked(settings.value("Canvas/AutoHideGroups", False, type=bool))
        self.ui.cb_canvas_bezier_lines.setChecked(settings.value("Canvas/UseBezierLines", True, type=bool))
        self.ui.cb_canvas_eyecandy.setCheckState(settings.value("Canvas/EyeCandy", CANVAS_EYECANDY_SMALL, type=int))
        self.ui.cb_canvas_use_opengl.setChecked(settings.value("Canvas/UseOpenGL", False, type=bool))
        self.ui.cb_canvas_render_aa.setCheckState(settings.value("Canvas/Antialiasing", CANVAS_ANTIALIASING_SMALL, type=int))
        self.ui.cb_canvas_render_hq_aa.setChecked(settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

        themeName = settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str)

        for i in range(patchcanvas.Theme.THEME_MAX):
            thisThemeName = patchcanvas.getThemeName(i)
            self.ui.cb_canvas_theme.addItem(thisThemeName)
            if thisThemeName == themeName:
                self.ui.cb_canvas_theme.setCurrentIndex(i)

        # --------------------------------------------

        if WINDOWS:
            defaultDriver = "DirectSound"
        elif MACOS:
            defaultDriver = "CoreAudio"
        else:
            defaultDriver = "JACK"

        audioDriver = settings.value("Engine/AudioDriver", defaultDriver, type=str)
        for i in range(self.ui.cb_engine_audio_driver.count()):
            if self.ui.cb_engine_audio_driver.itemText(i) == audioDriver:
                self.ui.cb_engine_audio_driver.setCurrentIndex(i)
                break
        else:
            self.ui.cb_engine_audio_driver.setCurrentIndex(-1)

        if audioDriver == "JACK":
            processModeIndex = settings.value("Engine/ProcessMode", PROCESS_MODE_MULTIPLE_CLIENTS, type=int)
            self.ui.cb_engine_process_mode_jack.setCurrentIndex(processModeIndex)
            self.ui.sw_engine_process_mode.setCurrentIndex(0)
        else:
            processModeIndex  = settings.value("Engine/ProcessMode", PROCESS_MODE_CONTINUOUS_RACK, type=int)
            processModeIndex -= PROCESS_MODE_NON_JACK_PADDING
            self.ui.cb_engine_process_mode_other.setCurrentIndex(processModeIndex)
            self.ui.sw_engine_process_mode.setCurrentIndex(1)

        self.ui.sb_engine_max_params.setValue(settings.value("Engine/MaxParameters", CARLA_DEFAULT_MAX_PARAMETERS, type=int))
        self.ui.ch_engine_prefer_ui_bridges.setChecked(settings.value("Engine/PreferUiBridges", CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool))
        self.ui.sb_engine_oscgui_timeout.setValue(settings.value("Engine/OscUiTimeout", CARLA_DEFAULT_OSC_UI_TIMEOUT, type=int))
        self.ui.ch_engine_disable_checks.setChecked(settings.value("Engine/DisableChecks", CARLA_DEFAULT_DISABLE_CHECKS, type=bool))
        self.ui.ch_engine_dssi_chunks.setChecked(settings.value("Engine/UseDssiVstChunks", CARLA_DEFAULT_USE_DSSI_VST_CHUNKS, type=bool))
        self.ui.ch_engine_prefer_plugin_bridges.setChecked(settings.value("Engine/PreferPluginBridges", CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool))
        self.ui.ch_engine_force_stereo.setChecked(settings.value("Engine/ForceStereo", CARLA_DEFAULT_FORCE_STEREO, type=bool))

        # --------------------------------------------

        ladspas = toList(settings.value("Paths/LADSPA", Carla.LADSPA_PATH))
        dssis = toList(settings.value("Paths/DSSI", Carla.DSSI_PATH))
        lv2s = toList(settings.value("Paths/LV2", Carla.LV2_PATH))
        vsts = toList(settings.value("Paths/VST", Carla.VST_PATH))
        gigs = toList(settings.value("Paths/GIG", Carla.GIG_PATH))
        sf2s = toList(settings.value("Paths/SF2", Carla.SF2_PATH))
        sfzs = toList(settings.value("Paths/SFZ", Carla.SFZ_PATH))

        ladspas.sort()
        dssis.sort()
        lv2s.sort()
        vsts.sort()
        gigs.sort()
        sf2s.sort()
        sfzs.sort()

        for ladspa in ladspas:
            self.ui.lw_ladspa.addItem(ladspa)

        for dssi in dssis:
            self.ui.lw_dssi.addItem(dssi)

        for lv2 in lv2s:
            self.ui.lw_lv2.addItem(lv2)

        for vst in vsts:
            self.ui.lw_vst.addItem(vst)

        for gig in gigs:
            self.ui.lw_gig.addItem(gig)

        for sf2 in sf2s:
            self.ui.lw_sf2.addItem(sf2)

        for sfz in sfzs:
            self.ui.lw_sfz.addItem(sfz)

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings()

        # ---------------------------------------

        settings.setValue("Main/RefreshInterval", self.ui.sb_gui_refresh.value())
        settings.setValue("Main/DefaultProjectFolder", self.ui.le_main_def_folder.text())

        # ---------------------------------------

        settings.setValue("Canvas/Theme", self.ui.cb_canvas_theme.currentText())
        settings.setValue("Canvas/AutoHideGroups", self.ui.cb_canvas_hide_groups.isChecked())
        settings.setValue("Canvas/UseBezierLines", self.ui.cb_canvas_bezier_lines.isChecked())
        settings.setValue("Canvas/UseOpenGL", self.ui.cb_canvas_use_opengl.isChecked())
        settings.setValue("Canvas/HighQualityAntialiasing", self.ui.cb_canvas_render_hq_aa.isChecked())

        # 0, 1, 2 match their enum variants
        settings.setValue("Canvas/EyeCandy", self.ui.cb_canvas_eyecandy.checkState())
        settings.setValue("Canvas/Antialiasing", self.ui.cb_canvas_render_aa.checkState())

        # --------------------------------------------

        audioDriver = self.ui.cb_engine_audio_driver.currentText()
        settings.setValue("Engine/AudioDriver", audioDriver)

        if audioDriver == "JACK":
            settings.setValue("Engine/ProcessMode", self.ui.cb_engine_process_mode_jack.currentIndex())
        else:
            settings.setValue("Engine/ProcessMode", self.ui.cb_engine_process_mode_other.currentIndex()+PROCESS_MODE_NON_JACK_PADDING)

        settings.setValue("Engine/MaxParameters", self.ui.sb_engine_max_params.value())
        settings.setValue("Engine/PreferUiBridges", self.ui.ch_engine_prefer_ui_bridges.isChecked())
        settings.setValue("Engine/OscUiTimeout", self.ui.sb_engine_oscgui_timeout.value())
        settings.setValue("Engine/DisableChecks", self.ui.ch_engine_disable_checks.isChecked())
        settings.setValue("Engine/UseDssiVstChunks", self.ui.ch_engine_dssi_chunks.isChecked())
        settings.setValue("Engine/PreferPluginBridges", self.ui.ch_engine_prefer_plugin_bridges.isChecked())
        settings.setValue("Engine/ForceStereo", self.ui.ch_engine_force_stereo.isChecked())

        # --------------------------------------------

        # FIXME - find a cleaner way to do this, *.items() ?
        ladspas = []
        dssis = []
        lv2s = []
        vsts = []
        gigs = []
        sf2s = []
        sfzs = []

        for i in range(self.ui.lw_ladspa.count()):
            ladspas.append(self.ui.lw_ladspa.item(i).text())

        for i in range(self.ui.lw_dssi.count()):
            dssis.append(self.ui.lw_dssi.item(i).text())

        for i in range(self.ui.lw_lv2.count()):
            lv2s.append(self.ui.lw_lv2.item(i).text())

        for i in range(self.ui.lw_vst.count()):
            vsts.append(self.ui.lw_vst.item(i).text())

        for i in range(self.ui.lw_gig.count()):
            gigs.append(self.ui.lw_gig.item(i).text())

        for i in range(self.ui.lw_sf2.count()):
            sf2s.append(self.ui.lw_sf2.item(i).text())

        for i in range(self.ui.lw_sfz.count()):
            sfzs.append(self.ui.lw_sfz.item(i).text())

        settings.setValue("Paths/LADSPA", ladspas)
        settings.setValue("Paths/DSSI", dssis)
        settings.setValue("Paths/LV2", lv2s)
        settings.setValue("Paths/VST", vsts)
        settings.setValue("Paths/GIG", gigs)
        settings.setValue("Paths/SF2", sf2s)
        settings.setValue("Paths/SFZ", sfzs)

    @pyqtSlot()
    def slot_resetSettings(self):
        if self.ui.lw_page.currentRow() == TAB_INDEX_MAIN:
            self.ui.le_main_def_folder.setText(HOME)
            self.ui.sb_gui_refresh.setValue(50)

        elif self.ui.lw_page.currentRow() == TAB_INDEX_CANVAS:
            self.ui.cb_canvas_theme.setCurrentIndex(0)
            self.ui.cb_canvas_hide_groups.setChecked(False)
            self.ui.cb_canvas_bezier_lines.setChecked(True)
            self.ui.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_use_opengl.setChecked(False)
            self.ui.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_render_hq_aa.setChecked(False)

        elif self.ui.lw_page.currentRow() == TAB_INDEX_CARLA_ENGINE:
            self.ui.cb_engine_audio_driver.setCurrentIndex(0)
            self.ui.sb_engine_max_params.setValue(CARLA_DEFAULT_MAX_PARAMETERS)
            self.ui.ch_engine_prefer_ui_bridges.setChecked(CARLA_DEFAULT_PREFER_UI_BRIDGES)
            self.ui.sb_engine_oscgui_timeout.setValue(CARLA_DEFAULT_OSC_UI_TIMEOUT)
            self.ui.ch_engine_disable_checks.setChecked(CARLA_DEFAULT_DISABLE_CHECKS)
            self.ui.ch_engine_dssi_chunks.setChecked(CARLA_DEFAULT_USE_DSSI_VST_CHUNKS)
            self.ui.ch_engine_prefer_plugin_bridges.setChecked(CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES)
            self.ui.ch_engine_force_stereo.setChecked(CARLA_DEFAULT_FORCE_STEREO)

            if self.ui.cb_engine_audio_driver.currentText() == "JACK":
                self.ui.cb_engine_process_mode_jack.setCurrentIndex(PROCESS_MODE_MULTIPLE_CLIENTS)
                self.ui.sw_engine_process_mode.setCurrentIndex(0)
            else:
                self.ui.cb_engine_process_mode_other.setCurrentIndex(PROCESS_MODE_CONTINUOUS_RACK-PROCESS_MODE_NON_JACK_PADDING)
                self.ui.sw_engine_process_mode.setCurrentIndex(1)

        elif self.ui.lw_page.currentRow() == TAB_INDEX_CARLA_PATHS:
            if self.ui.tw_paths.currentIndex() == 0:
                Carla.LADSPA_PATH.sort()
                self.ui.lw_ladspa.clear()

                for ladspa in Carla.LADSPA_PATH:
                    self.ui.lw_ladspa.addItem(ladspa)

            elif self.ui.tw_paths.currentIndex() == 1:
                Carla.DSSI_PATH.sort()
                self.ui.lw_dssi.clear()

                for dssi in Carla.DSSI_PATH:
                    self.ui.lw_dssi.addItem(dssi)

            elif self.ui.tw_paths.currentIndex() == 2:
                Carla.LV2_PATH.sort()
                self.ui.lw_lv2.clear()

                for lv2 in Carla.LV2_PATH:
                    self.ui.lw_lv2.addItem(lv2)

            elif self.ui.tw_paths.currentIndex() == 3:
                Carla.VST_PATH.sort()
                self.ui.lw_vst.clear()

                for vst in Carla.VST_PATH:
                    self.ui.lw_vst.addItem(vst)

            elif self.ui.tw_paths.currentIndex() == 4:
                Carla.GIG_PATH.sort()
                self.ui.lw_gig.clear()

                for gig in Carla.GIG_PATH:
                    self.ui.lw_gig.addItem(gig)

            elif self.ui.tw_paths.currentIndex() == 5:
                Carla.SF2_PATH.sort()
                self.ui.lw_sf2.clear()

                for sf2 in Carla.SF2_PATH:
                    self.ui.lw_sf2.addItem(sf2)

            elif self.ui.tw_paths.currentIndex() == 6:
                Carla.SFZ_PATH.sort()
                self.ui.lw_sfz.clear()

                for sfz in Carla.SFZ_PATH:
                    self.ui.lw_sfz.addItem(sfz)

    @pyqtSlot()
    def slot_getAndSetProjectPath(self):
        getAndSetPath(self, self.ui.le_main_def_folder.text(), self.ui.le_main_def_folder)

    @pyqtSlot()
    def slot_engineAudioDriverChanged(self):
        if self.ui.cb_engine_audio_driver.currentText() == "JACK":
            self.ui.sw_engine_process_mode.setCurrentIndex(0)
        else:
            self.ui.sw_engine_process_mode.setCurrentIndex(1)

    @pyqtSlot()
    def slot_addPluginPath(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)
        if newPath:
            if self.ui.tw_paths.currentIndex() == 0:
                self.ui.lw_ladspa.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 1:
                self.ui.lw_dssi.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 2:
                self.ui.lw_lv2.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 3:
                self.ui.lw_vst.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 4:
                self.ui.lw_gig.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 5:
                self.ui.lw_sf2.addItem(newPath)
            elif self.ui.tw_paths.currentIndex() == 6:
                self.ui.lw_sfz.addItem(newPath)

    @pyqtSlot()
    def slot_removePluginPath(self):
        if self.ui.tw_paths.currentIndex() == 0:
            self.ui.lw_ladspa.takeItem(self.ui.lw_ladspa.currentRow())
        elif self.ui.tw_paths.currentIndex() == 1:
            self.ui.lw_dssi.takeItem(self.ui.lw_dssi.currentRow())
        elif self.ui.tw_paths.currentIndex() == 2:
            self.ui.lw_lv2.takeItem(self.ui.lw_lv2.currentRow())
        elif self.ui.tw_paths.currentIndex() == 3:
            self.ui.lw_vst.takeItem(self.ui.lw_vst.currentRow())
        elif self.ui.tw_paths.currentIndex() == 4:
            self.ui.lw_gig.takeItem(self.ui.lw_gig.currentRow())
        elif self.ui.tw_paths.currentIndex() == 5:
            self.ui.lw_sf2.takeItem(self.ui.lw_sf2.currentRow())
        elif self.ui.tw_paths.currentIndex() == 6:
            self.ui.lw_sfz.takeItem(self.ui.lw_sfz.currentRow())

    @pyqtSlot()
    def slot_changePluginPath(self):
        if self.ui.tw_paths.currentIndex() == 0:
            currentPath = self.ui.lw_ladspa.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 1:
            currentPath = self.ui.lw_dssi.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 2:
            currentPath = self.ui.lw_lv2.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 3:
            currentPath = self.ui.lw_vst.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 4:
            currentPath = self.ui.lw_gig.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 5:
            currentPath = self.ui.lw_sf2.currentItem().text()
        elif self.ui.tw_paths.currentIndex() == 6:
            currentPath = self.ui.lw_sfz.currentItem().text()
        else:
            currentPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), currentPath, QFileDialog.ShowDirsOnly)
        if newPath:
            if self.ui.tw_paths.currentIndex() == 0:
                self.ui.lw_ladspa.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 1:
                self.ui.lw_dssi.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 2:
                self.ui.lw_lv2.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 3:
                self.ui.lw_vst.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 4:
                self.ui.lw_gig.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 5:
                self.ui.lw_sf2.currentItem().setText(newPath)
            elif self.ui.tw_paths.currentIndex() == 6:
                self.ui.lw_sfz.currentItem().setText(newPath)

    @pyqtSlot(int)
    def slot_pluginPathTabChanged(self, index):
        if index == 0:
            row = self.ui.lw_ladspa.currentRow()
        elif index == 1:
            row = self.ui.lw_dssi.currentRow()
        elif index == 2:
            row = self.ui.lw_lv2.currentRow()
        elif index == 3:
            row = self.ui.lw_vst.currentRow()
        elif index == 4:
            row = self.ui.lw_gig.currentRow()
        elif index == 5:
            row = self.ui.lw_sf2.currentRow()
        elif index == 6:
            row = self.ui.lw_sfz.currentRow()
        else:
            row = -1

        check = bool(row >= 0)
        self.ui.b_paths_remove.setEnabled(check)
        self.ui.b_paths_change.setEnabled(check)

    @pyqtSlot(int)
    def slot_pluginPathRowChanged(self, row):
        check = bool(row >= 0)
        self.ui.b_paths_remove.setEnabled(check)
        self.ui.b_paths_change.setEnabled(check)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CarlaMainW(QMainWindow):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.ui =  ui_carla.Ui_CarlaMainW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)
        self.loadRDFs()

        self.setStyleSheet("""
          QWidget#w_plugins {
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

        self.fEngineStarted   = False
        self.fFirstEngineInit = False

        self.fProjectFilename = None
        self.fProjectLoading  = False

        self.fPluginCount = 0
        self.fPluginList  = []

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLastLoadedPluginId = -1

        #self._nsmAnnounce2str = ""
        #self._nsmOpen1str = ""
        #self._nsmOpen2str = ""
        #self.nsm_server = None
        #self.nsm_url = None

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setNameFilters(cString(Carla.host.get_supported_file_types()).split(";"))
        self.fDirModel.setRootPath(HOME)

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        self.ui.act_engine_start.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        # FIXME: Qt4 needs this so it properly create & resize the canvas
        self.ui.tabMain.setCurrentIndex(1)
        self.ui.tabMain.setCurrentIndex(0)

        # -------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self.ui.graphicsView)
        self.ui.graphicsView.setScene(self.scene)
        #self.ui.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.fSavedSettings["Canvas/Antialiasing"] == patchcanvas.ANTIALIASING_FULL))
        #if self.fSavedSettings["Canvas/UseOpenGL"] and hasGL:
            #self.ui.graphicsView.setViewport(QGLWidget(self.ui.graphicsView))
            #self.ui.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.fSavedSettings["Canvas/HighQualityAntialiasing"])

        pOptions = patchcanvas.options_t()
        pOptions.theme_name       = self.fSavedSettings["Canvas/Theme"]
        pOptions.auto_hide_groups = self.fSavedSettings["Canvas/AutoHideGroups"]
        pOptions.use_bezier_lines = self.fSavedSettings["Canvas/UseBezierLines"]
        pOptions.antialiasing     = self.fSavedSettings["Canvas/Antialiasing"]
        pOptions.eyecandy         = self.fSavedSettings["Canvas/EyeCandy"]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = False
        pFeatures.port_rename  = False
        pFeatures.handle_group_pos = True

        patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Carla", self.scene, canvasCallback, False)

        patchcanvas.setCanvasSize(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        patchcanvas.setInitialPos(DEFAULT_CANVAS_WIDTH / 2, DEFAULT_CANVAS_HEIGHT / 2)
        self.ui.graphicsView.setSceneRect(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

        # -------------------------------------------------------------
        # Set-up Canvas Preview

        self.ui.miniCanvasPreview.setRealParent(self)
        self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.ui.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        QTimer.singleShot(100, self, SLOT("slot_miniCanvasInit()"))

        #self.m_fakeEdit = PluginEdit(self, -1)
        #self.m_curEdit  = self.m_fakeEdit
        #self.w_edit.layout().addWidget(self.m_curEdit)
        #self.w_edit.layout().addStretch()

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.ui.act_file_new, SIGNAL("triggered()"), SLOT("slot_fileNew()"))
        self.connect(self.ui.act_file_open, SIGNAL("triggered()"), SLOT("slot_fileOpen()"))
        self.connect(self.ui.act_file_save, SIGNAL("triggered()"), SLOT("slot_fileSave()"))
        self.connect(self.ui.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_fileSaveAs()"))

        self.connect(self.ui.act_engine_start, SIGNAL("triggered()"), SLOT("slot_engineStart()"))
        self.connect(self.ui.act_engine_stop, SIGNAL("triggered()"), SLOT("slot_engineStop()"))

        self.connect(self.ui.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        self.connect(self.ui.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_pluginRemoveAll()"))

        self.connect(self.ui.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))
        self.connect(self.ui.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.ui.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self.ui.ch_transport, SIGNAL("currentIndexChanged(int)"), SLOT("slot_transportModeChanged(int)"))
        self.connect(self.ui.b_transport_play, SIGNAL("clicked(bool)"), SLOT("slot_transportPlayPause(bool)"))
        self.connect(self.ui.b_transport_stop, SIGNAL("clicked()"), SLOT("slot_transportStop()"))

        self.connect(self.ui.fileTreeView, SIGNAL("doubleClicked(QModelIndex)"), SLOT("slot_fileTreeDoubleClicked(QModelIndex)"))
        self.connect(self.ui.miniCanvasPreview, SIGNAL("miniCanvasMoved(double, double)"), SLOT("slot_miniCanvasMoved(double, double)"))

        self.connect(self.ui.graphicsView.horizontalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_horizontalScrollBarChanged(int)"))
        self.connect(self.ui.graphicsView.verticalScrollBar(), SIGNAL("valueChanged(int)"), SLOT("slot_verticalScrollBarChanged(int)"))

        self.connect(self.scene, SIGNAL("sceneGroupMoved(int, int, QPointF)"), SLOT("slot_canvasItemMoved(int, int, QPointF)"))
        self.connect(self.scene, SIGNAL("scaleChanged(double)"), SLOT("slot_canvasScaleChanged(double)"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        self.connect(self, SIGNAL("SIGTERM()"), SLOT("slot_handleSIGTERM()"))

        self.connect(self, SIGNAL("DebugCallback(int, int, int, double, QString)"), SLOT("slot_handleDebugCallback(int, int, int, double, QString)"))
        self.connect(self, SIGNAL("PluginAddedCallback(int)"), SLOT("slot_handlePluginAddedCallback(int)"))
        self.connect(self, SIGNAL("PluginRemovedCallback(int)"), SLOT("slot_handlePluginRemovedCallback(int)"))
        self.connect(self, SIGNAL("PluginRenamedCallback(int, QString)"), SLOT("slot_handlePluginRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("ParameterValueChangedCallback(int, int, double)"), SLOT("slot_handleParameterValueChangedCallback(int, int, double)"))
        self.connect(self, SIGNAL("ParameterDefaultChangedCallback(int, int, double)"), SLOT("slot_handleParameterDefaultChangedCallback(int, int, double)"))
        self.connect(self, SIGNAL("ParameterMidiChannelChangedCallback(int, int, int)"), SLOT("slot_handleParameterMidiChannelChangedCallback(int, int, int)"))
        self.connect(self, SIGNAL("ParameterMidiCcChangedCallback(int, int, int)"), SLOT("slot_handleParameterMidiCcChangedCallback(int, int, int)"))
        self.connect(self, SIGNAL("ProgramChangedCallback(int, int)"), SLOT("slot_handleProgramChangedCallback(int, int)"))
        self.connect(self, SIGNAL("MidiProgramChangedCallback(int, int)"), SLOT("slot_handleMidiProgramChangedCallback(int, int)"))
        self.connect(self, SIGNAL("NoteOnCallback(int, int, int, int)"), SLOT("slot_handleNoteOnCallback(int, int, int, int)"))
        self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), SLOT("slot_handleNoteOffCallback(int, int, int)"))
        self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), SLOT("slot_handleShowGuiCallback(int, int)"))
        self.connect(self, SIGNAL("UpdateCallback(int)"), SLOT("slot_handleUpdateCallback(int)"))
        self.connect(self, SIGNAL("ReloadInfoCallback(int)"), SLOT("slot_handleReloadInfoCallback(int)"))
        self.connect(self, SIGNAL("ReloadParametersCallback(int)"), SLOT("slot_handleReloadParametersCallback(int)"))
        self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), SLOT("slot_handleReloadProgramsCallback(int)"))
        self.connect(self, SIGNAL("ReloadAllCallback(int)"), SLOT("slot_handleReloadAllCallback(int)"))
        self.connect(self, SIGNAL("PatchbayClientAddedCallback(int, QString)"), SLOT("slot_handlePatchbayClientAddedCallback(int, QString)"))
        self.connect(self, SIGNAL("PatchbayClientRemovedCallback(int)"), SLOT("slot_handlePatchbayClientRemovedCallback(int)"))
        self.connect(self, SIGNAL("PatchbayClientRenamedCallback(int, QString)"), SLOT("slot_handlePatchbayClientRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("PatchbayPortAddedCallback(int, int, int, QString)"), SLOT("slot_handlePatchbayPortAddedCallback(int, int, int, QString)"))
        self.connect(self, SIGNAL("PatchbayPortRemovedCallback(int)"), SLOT("slot_handlePatchbayPortRemovedCallback(int)"))
        self.connect(self, SIGNAL("PatchbayPortRenamedCallback(int, QString)"), SLOT("slot_handlePatchbayPortRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("PatchbayConnectionAddedCallback(int, int, int)"), SLOT("slot_handlePatchbayConnectionAddedCallback(int, int, int)"))
        self.connect(self, SIGNAL("PatchbayConnectionRemovedCallback(int)"), SLOT("slot_handlePatchbayConnectionRemovedCallback(int)"))
        #self.connect(self, SIGNAL("NSM_AnnounceCallback()"), SLOT("slot_handleNSM_AnnounceCallback()"))
        #self.connect(self, SIGNAL("NSM_Open1Callback()"), SLOT("slot_handleNSM_Open1Callback()"))
        #self.connect(self, SIGNAL("NSM_Open2Callback()"), SLOT("slot_handleNSM_Open2Callback()"))
        #self.connect(self, SIGNAL("NSM_SaveCallback()"), SLOT("slot_handleNSM_SaveCallback()"))
        self.connect(self, SIGNAL("ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        #NSM_URL = os.getenv("NSM_URL")

        #if NSM_URL:
            #Carla.host.nsm_announce(NSM_URL, os.getpid())
        #else:
        QTimer.singleShot(0, self, SLOT("slot_engineStart()"))

    @pyqtSlot(QModelIndex)
    def slot_fileTreeDoubleClicked(self, modelIndex):
        filename = self.fDirModel.filePath(modelIndex)
        print(filename)

        if not os.path.exists(filename):
            return
        if not os.path.isfile(filename):
            return

        basename  = os.path.basename(filename)
        extension = filename.rsplit(".", 1)[-1].lower()

        if extension == "carxp":
            Carla.host.load_project(filename)

        elif extension == "gig":
            Carla.host.add_plugin(BINARY_NATIVE, PLUGIN_GIG, filename, None, basename, None)

        elif extension == "sf2":
            Carla.host.add_plugin(BINARY_NATIVE, PLUGIN_SF2, filename, None, basename, None)

        elif extension == "sfz":
            Carla.host.add_plugin(BINARY_NATIVE, PLUGIN_SFZ, filename, None, basename, None)

        elif extension in ("aac", "flac", "oga", "ogg", "mp3", "wav"):
            self.fLastLoadedPluginId = -2
            if Carla.host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, None, basename, "audiofile", None):
                while (self.fLastLoadedPluginId == -2): sleep(0.2)
                idx = self.fLastLoadedPluginId
                self.fLastLoadedPluginId = -1
                Carla.host.set_custom_data(idx, CUSTOM_DATA_STRING, "file00", filename)
            else:
                self.fLastLoadedPluginId = -1
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                                 cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        elif extension in ("mid", "midi"):
            self.fLastLoadedPluginId = -2
            if Carla.host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, None, basename, "midifile", None):
                while (self.fLastLoadedPluginId == -2): sleep(0.2)
                idx = self.fLastLoadedPluginId
                self.fLastLoadedPluginId = -1
                Carla.host.set_custom_data(idx, CUSTOM_DATA_STRING, "file", filename)
            else:
                self.fLastLoadedPluginId = -1
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                                 cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.ui.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        self.ui.miniCanvasPreview.update()

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosX(xp)

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosY(yp)

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        settings = QSettings()
        self.ui.graphicsView.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH / 3, type=int))
        self.ui.graphicsView.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT * 3 / 8, type=int))

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.ui.graphicsView.horizontalScrollBar().setValue(xp * DEFAULT_CANVAS_WIDTH)
        self.ui.graphicsView.verticalScrollBar().setValue(yp * DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.ui.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.ui.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.ui.miniCanvasPreview.setViewSize(float(self.ui.graphicsView.width()) / DEFAULT_CANVAS_WIDTH, float(self.ui.graphicsView.height()) / DEFAULT_CANVAS_HEIGHT)

    def startEngine(self, clientName = "Carla"):
        # ---------------------------------------------
        # Engine settings

        settings = QSettings()

        if LINUX:
            defaultMode = PROCESS_MODE_MULTIPLE_CLIENTS
        else:
            defaultMode = PROCESS_MODE_CONTINUOUS_RACK

        Carla.processMode    = settings.value("Engine/ProcessMode", defaultMode, type=int)
        Carla.maxParameters  = settings.value("Engine/MaxParameters", MAX_DEFAULT_PARAMETERS, type=int)

        transportMode        = settings.value("Engine/TransportMode", TRANSPORT_MODE_JACK, type=int)
        forceStereo          = settings.value("Engine/ForceStereo", False, type=bool)
        preferPluginBridges  = settings.value("Engine/PreferPluginBridges", False, type=bool)
        preferUiBridges      = settings.value("Engine/PreferUiBridges", True, type=bool)
        useDssiVstChunks     = settings.value("Engine/UseDssiVstChunks", False, type=bool)

        oscUiTimeout         = settings.value("Engine/OscUiTimeout", 40, type=int)
        preferredBufferSize  = settings.value("Engine/PreferredBufferSize", 512, type=int)
        preferredSampleRate  = settings.value("Engine/PreferredSampleRate", 44100, type=int)

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif Carla.processMode == PROCESS_MODE_MULTIPLE_CLIENTS and os.getenv("LADISH_APP_NAME"):
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            Carla.processMode = PROCESS_MODE_SINGLE_CLIENT

        Carla.host.set_engine_option(OPTION_PROCESS_MODE, Carla.processMode, "")
        Carla.host.set_engine_option(OPTION_MAX_PARAMETERS, Carla.maxParameters, "")
        Carla.host.set_engine_option(OPTION_FORCE_STEREO, forceStereo, "")
        Carla.host.set_engine_option(OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")
        Carla.host.set_engine_option(OPTION_PREFER_UI_BRIDGES, preferUiBridges, "")
        Carla.host.set_engine_option(OPTION_USE_DSSI_VST_CHUNKS, useDssiVstChunks, "")
        Carla.host.set_engine_option(OPTION_OSC_UI_TIMEOUT, oscUiTimeout, "")
        Carla.host.set_engine_option(OPTION_PREFERRED_BUFFER_SIZE, preferredBufferSize, "")
        Carla.host.set_engine_option(OPTION_PREFERRED_SAMPLE_RATE, preferredSampleRate, "")

        # ---------------------------------------------
        # Start

        if WINDOWS:
            defaultDriver = "DirectSound"
        elif MACOS:
            defaultDriver = "CoreAudio"
        else:
            defaultDriver = "JACK"

        audioDriver = settings.value("Engine/AudioDriver", defaultDriver, type=str)

        if not Carla.host.engine_init(audioDriver, clientName):
            if self.fFirstEngineInit:
                self.fFirstEngineInit = False
                return

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fEngineStarted   = True
        self.fFirstEngineInit = False

        self.fPluginCount = 0
        self.fPluginList  = []

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            maxCount = MAX_RACK_PLUGINS
        elif Carla.processMode == PROCESS_MODE_PATCHBAY:
            maxCount = MAX_PATCHBAY_PLUGINS
        else:
            maxCount = MAX_DEFAULT_PLUGINS

        self.ui.ch_transport.blockSignals(True)
        self.ui.ch_transport.addItem(self.tr("Internal"))

        if audioDriver == "JACK":
            self.ui.ch_transport.addItem(self.tr("JACK"))
        elif transportMode == TRANSPORT_MODE_JACK:
            transportMode = TRANSPORT_MODE_INTERNAL

        self.ui.ch_transport.setCurrentIndex(transportMode)
        self.ui.ch_transport.blockSignals(False)

        for x in range(maxCount):
            self.fPluginList.append(None)

        Carla.host.set_engine_option(OPTION_TRANSPORT_MODE, transportMode, "")

        # Peaks
        self.fIdleTimerFast = self.startTimer(self.fSavedSettings["Main/RefreshInterval"])
        # LEDs and edit dialog parameters
        self.fIdleTimerSlow = self.startTimer(self.fSavedSettings["Main/RefreshInterval"]*2)

    def stopEngine(self):
        if self.fPluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask != QMessageBox.Yes:
                return

            self.removeAllPlugins()

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(cString(Carla.host.get_last_error()))

        self.fEngineStarted = False
        self.fPluginCount = 0
        self.fPluginList  = []

        self.killTimer(self.fIdleTimerFast)
        self.killTimer(self.fIdleTimerSlow)

        settings = QSettings()
        settings.setValue("Engine/TransportMode", self.ui.ch_transport.currentIndex())
        self.ui.ch_transport.blockSignals(True)
        self.ui.ch_transport.clear()
        self.ui.ch_transport.blockSignals(False)

        patchcanvas.clear()

    def loadProject(self, filename):
        self.fProjectFilename = filename
        self.setWindowTitle("Carla - %s" % os.path.basename(filename))

        self.fProjectLoading = True
        Carla.host.load_project(filename)
        self.fProjectLoading = False

    def loadProjectLater(self, filename):
        self.fProjectFilename = filename
        self.setWindowTitle("Carla - %s" % os.path.basename(filename))
        QTimer.singleShot(0, self, SLOT("slot_loadProjectLater()"))

    def saveProject(self, filename):
        self.fProjectFilename = filename
        self.setWindowTitle("Carla - %s" % os.path.basename(filename))
        Carla.host.save_project(filename)

    def addPlugin(self, btype, ptype, filename, name, label, extraStuff):
        if not self.fEngineStarted:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return False

        if not Carla.host.add_plugin(btype, ptype, filename, name, label, extraStuff):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
            return False

        return True

    def removeAllPlugins(self):
        while (self.ui.w_plugins.layout().takeAt(0)):
            pass

        self.ui.act_plugin_remove_all.setEnabled(False)

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            self.fPluginList[i] = None

            pwidget.ui.edit_dialog.close()
            pwidget.close()
            pwidget.deleteLater()
            del pwidget

        self.fPluginCount = 0

        Carla.host.remove_all_plugins()

    @pyqtSlot()
    def slot_fileNew(self):
        self.removeAllPlugins()
        self.fProjectFilename = None
        self.fProjectLoading  = False
        self.setWindowTitle("Carla")

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            # FIXME - show dialog to user
            self.removeAllPlugins()
            self.loadProject(filenameTry)

    @pyqtSlot()
    def slot_fileSave(self, saveAs=False):
        if self.fProjectFilename and not saveAs:
            return self.saveProject(self.fProjectFilename)

        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            if not filenameTry.endswith(".carxp"):
                filenameTry += ".carxp"

            self.saveProject(filenameTry)

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    @pyqtSlot()
    def slot_loadProjectLater(self):
        self.fProjectLoading = True
        Carla.host.load_project(self.fProjectFilename)
        self.fProjectLoading = False

    @pyqtSlot()
    def slot_engineStart(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)
        self.ui.act_engine_configure.setEnabled(not check)
        self.ui.frame_transport.setEnabled(check)

    @pyqtSlot()
    def slot_engineStop(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)
        self.ui.frame_transport.setEnabled(check)

    @pyqtSlot()
    def slot_pluginAdd(self):
        dialog = PluginDatabaseW(self)
        if dialog.exec_():
            btype    = dialog.fRetPlugin['build']
            ptype    = dialog.fRetPlugin['type']
            filename = dialog.fRetPlugin['binary']
            label    = dialog.fRetPlugin['label']
            extraStuff = self.getExtraStuff(dialog.fRetPlugin)
            self.addPlugin(btype, ptype, filename, None, label, extraStuff)

    @pyqtSlot()
    def slot_pluginRemoveAll(self):
        self.removeAllPlugins()

        self.connect(self.ui.ch_transport, SIGNAL("currentIndexChanged(int)"), SLOT("(int)"))
        self.connect(self.ui.b_transport_play, SIGNAL("clicked(bool)"), SLOT("(bool)"))
        self.connect(self.ui.b_transport_stop, SIGNAL("clicked()"), SLOT("()"))

    @pyqtSlot(int)
    def slot_transportModeChanged(self, newMode):
        Carla.host.set_engine_option(OPTION_TRANSPORT_MODE, newMode, "")

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if toggled:
            Carla.host.transport_play()
        else:
            Carla.host.transport_pause()

    @pyqtSlot()
    def slot_transportStop(self):
        Carla.host.transport_pause()
        Carla.host.transport_relocate(0)

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self)
        if dialog.exec_():
            self.loadSettings(False)

            for pwidget in self.fPluginList:
                if pwidget is None:
                    return
                pwidget.setRefreshRate(self.fSavedSettings["Main/RefreshInterval"])

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_fileSave()"))

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG :: %i, %i, %i, %f, \"%s\")" % (pluginId, value1, value2, value3, valueStr))

    @pyqtSlot(int)
    def slot_handlePluginAddedCallback(self, pluginId):
        pwidget = PluginWidget(self, pluginId)
        pwidget.setRefreshRate(self.fSavedSettings["Main/RefreshInterval"])

        self.ui.w_plugins.layout().addWidget(pwidget)

        self.fPluginCount += 1
        self.fPluginList[pluginId] = pwidget

        if not self.fProjectLoading:
            pwidget.setActive(True, True, True)

        if self.fPluginCount == 1:
            self.ui.act_plugin_remove_all.setEnabled(True)

        if self.fLastLoadedPluginId == -2:
            self.fLastLoadedPluginId = pluginId

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        self.fPluginList[pluginId] = None
        self.fPluginCount -= 1

        self.ui.w_plugins.layout().removeWidget(pwidget)

        pwidget.ui.edit_dialog.close()
        pwidget.close()
        pwidget.deleteLater()
        del pwidget

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            self.fPluginList[i] = self.fPluginList[i+1]
            self.fPluginList[i].setId(i)

        if self.fPluginCount == 0:
            self.ui.act_plugin_remove_all.setEnabled(False)

    @pyqtSlot(int, str)
    def slot_handlePluginRenamedCallback(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.label_name.setText(newName)

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, parameterId, value):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterValue(parameterId, value)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, parameterId, value):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterDefault(parameterId, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, parameterId, channel):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiChannel(parameterId, channel)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, parameterId, cc):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiControl(parameterId, cc)

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, programId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setProgram(programId)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, midiProgramId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setMidiProgram(midiProgramId)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.sendNoteOn(channel, note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.sendNoteOff(channel, note)

    @pyqtSlot(int, int)
    def slot_handleShowGuiCallback(self, pluginId, show):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        if show == 0:
            pwidget.ui.b_gui.setChecked(False)
            pwidget.ui.b_gui.setEnabled(True)
        elif show == 1:
            pwidget.ui.b_gui.setChecked(True)
            pwidget.ui.b_gui.setEnabled(True)
        elif show == -1:
            pwidget.ui.b_gui.setChecked(False)
            pwidget.ui.b_gui.setEnabled(False)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadAll()

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientAddedCallback(self, clientId, clientName):
        patchcanvas.addGroup(clientId, clientName)

    @pyqtSlot(int)
    def slot_handlePatchbayClientRemovedCallback(self, clientId):
        patchcanvas.removeGroup(clientId)

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortAddedCallback(self, clientId, portId, portFlags, portName):
        if (portFlags & PATCHBAY_PORT_IS_INPUT):
            portMode = patchcanvas.PORT_MODE_INPUT
        elif (portFlags & PATCHBAY_PORT_IS_OUTPUT):
            portMode = patchcanvas.PORT_MODE_OUTPUT
        else:
            portMode = patchcanvas.PORT_MODE_NULL

        if (portFlags & PATCHBAY_PORT_IS_AUDIO):
            portType = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif (portFlags & PATCHBAY_PORT_IS_MIDI):
            portType = patchcanvas.PORT_TYPE_MIDI_JACK
        else:
            portType = patchcanvas.PORT_TYPE_NULL

        patchcanvas.addPort(clientId, portId, portName, portMode, portType)

    @pyqtSlot(int)
    def slot_handlePatchbayPortRemovedCallback(self, portId):
        patchcanvas.removePort(portId)

    @pyqtSlot(int, str)
    def slot_handlePatchbayPortRenamedCallback(self, portId, newPortName):
        patchcanvas.renamePort(portId, newPortName)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, portOutId, portInId):
        patchcanvas.connectPorts(connectionId, portOutId, portInId)

    @pyqtSlot(int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId):
        patchcanvas.disconnectPorts(connectionId)

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, self.tr("Error"), error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("Engine has been stopped or crashed.\nPlease restart Carla"),
            self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    def getExtraStuff(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']
            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype == PLUGIN_DSSI:
            if (plugin['hints'] & PLUGIN_HAS_GUI):
                gui = findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
                if gui:
                    return gui.encode("utf-8")

        elif ptype == PLUGIN_SF2:
            if plugin['name'].endswith(" (16 outputs)"):
                # return a dummy non-null pointer
                INTPOINTER = POINTER(c_int)
                ptr  = c_int(0x1)
                addr = addressof(ptr)
                return cast(addr, INTPOINTER)

        return c_nullptr

    def loadRDFs(self):
        # Save RDF info for later
        self.fLadspaRdfList = []

        if not haveLRDF:
            return

        settingsDir  = os.path.join(HOME, ".config", "falkTX")
        frLadspaFile = os.path.join(settingsDir, "ladspa_rdf.db")

        if os.path.exists(frLadspaFile):
            frLadspa = open(frLadspaFile, 'r')

            try:
                self.fLadspaRdfList = ladspa_rdf.get_c_ladspa_rdfs(json.load(frLadspa))
            except:
                pass

            frLadspa.close()

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("SplitterState", self.ui.splitter.saveState())
        settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())
        #settings.setValue("ShowTransport", self.ui.frame_transport.isVisible())
        settings.setValue("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().value())

    def loadSettings(self, geometry):
        settings = QSettings()

        if geometry:
            self.restoreGeometry(settings.value("Geometry", ""))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

            #showTransport = settings.value("ShowTransport", True, type=bool)
            #self.ui.act_settings_show_transport.setChecked(showTransport)
            #self.ui.frame_transport.setVisible(showTransport)
            self.ui.frame_transport.setVisible(False)

            if settings.contains("SplitterState"):
                self.ui.splitter.restoreState(settings.value("SplitterState", ""))
            else:
                self.ui.splitter.setSizes([99999, 210])

        self.fSavedSettings = {
            "Main/DefaultProjectFolder": settings.value("Main/DefaultProjectFolder", HOME, type=str),
            "Main/RefreshInterval": settings.value("Main/RefreshInterval", 50, type=int),
            "Canvas/Theme": settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/HighQualityAntialiasing": settings.value("Canvas/HighQualityAntialiasing", False, type=bool)
        }

        # ---------------------------------------------
        # plugin checks

        if settings.value("Engine/DisableChecks", False, type=bool):
            os.environ["CARLA_DISCOVERY_NO_PROCESSING_CHECKS"] = "true"

        elif os.getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"):
            os.environ.pop("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")

        # ---------------------------------------------
        # plugin paths

        Carla.LADSPA_PATH = toList(settings.value("Paths/LADSPA", Carla.LADSPA_PATH))
        Carla.DSSI_PATH = toList(settings.value("Paths/DSSI", Carla.DSSI_PATH))
        Carla.LV2_PATH = toList(settings.value("Paths/LV2", Carla.LV2_PATH))
        Carla.VST_PATH = toList(settings.value("Paths/VST", Carla.VST_PATH))
        Carla.GIG_PATH = toList(settings.value("Paths/GIG", Carla.GIG_PATH))
        Carla.SF2_PATH = toList(settings.value("Paths/SF2", Carla.SF2_PATH))
        Carla.SFZ_PATH = toList(settings.value("Paths/SFZ", Carla.SFZ_PATH))

        os.environ["LADSPA_PATH"] = splitter.join(Carla.LADSPA_PATH)
        os.environ["DSSI_PATH"] = splitter.join(Carla.DSSI_PATH)
        os.environ["LV2_PATH"] = splitter.join(Carla.LV2_PATH)
        os.environ["VST_PATH"] = splitter.join(Carla.VST_PATH)
        os.environ["GIG_PATH"] = splitter.join(Carla.GIG_PATH)
        os.environ["SF2_PATH"] = splitter.join(Carla.SF2_PATH)
        os.environ["SFZ_PATH"] = splitter.join(Carla.SFZ_PATH)

    def resizeEvent(self, event):
        if self.ui.tabMain.currentIndex() == 0:
            # Force update of 2nd tab
            width  = self.ui.tab_plugins.width()-4
            height = self.ui.tab_plugins.height()-4
            self.ui.miniCanvasPreview.setViewSize(float(width) / DEFAULT_CANVAS_WIDTH, float(height) / DEFAULT_CANVAS_HEIGHT)
        else:
            QTimer.singleShot(0, self, SLOT("slot_miniCanvasCheckSize()"))

        QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            Carla.host.engine_idle()

            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        #if self.nsm_server:
            #self.nsm_server.stop()

        self.saveSettings()

        if Carla.host.is_engine_running():
            Carla.host.set_engine_about_to_close()
            self.removeAllPlugins()

        self.stopEngine()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------

def canvasCallback(action, value1, value2, valueStr):
    if action == patchcanvas.ACTION_GROUP_INFO:
        pass

    elif action == patchcanvas.ACTION_GROUP_RENAME:
        pass

    elif action == patchcanvas.ACTION_GROUP_SPLIT:
        groupId = value1
        patchcanvas.splitGroup(groupId)

    elif action == patchcanvas.ACTION_GROUP_JOIN:
        groupId = value1
        patchcanvas.joinGroup(groupId)

    elif action == patchcanvas.ACTION_PORT_INFO:
        pass

    elif action == patchcanvas.ACTION_PORT_RENAME:
        pass

    elif action == patchcanvas.ACTION_PORTS_CONNECT:
        portIdA = value1
        portIdB = value2

        Carla.host.patchbay_connect(portIdA, portIdB)

    elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
        connectionId = value1

        Carla.host.patchbay_disconnect(connectionId)

def engineCallback(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId < 0 or not Carla.gui:
        return

    if action == CALLBACK_DEBUG:
        Carla.gui.emit(SIGNAL("DebugCallback(int, int, int, double, QString)"), pluginId, value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PLUGIN_ADDED:
        Carla.gui.emit(SIGNAL("PluginAddedCallback(int)"), pluginId)
    elif action == CALLBACK_PLUGIN_REMOVED:
        Carla.gui.emit(SIGNAL("PluginRemovedCallback(int)"), pluginId)
    elif action == CALLBACK_PLUGIN_RENAMED:
        Carla.gui.emit(SIGNAL("PluginRenamedCallback(int, QString)"), pluginId, valueStr)
    elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterValueChangedCallback(int, int, double)"), pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_DEFAULT_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterDefaultChangedCallback(int, int, double)"), pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterMidiChannelChangedCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        Carla.gui.emit(SIGNAL("ParameterMidiCcChangedCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PROGRAM_CHANGED:
        Carla.gui.emit(SIGNAL("ProgramChangedCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        Carla.gui.emit(SIGNAL("MidiProgramChangedCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_NOTE_ON:
        Carla.gui.emit(SIGNAL("NoteOnCallback(int, int, int, int)"), pluginId, value1, value2, value3)
    elif action == CALLBACK_NOTE_OFF:
        Carla.gui.emit(SIGNAL("NoteOffCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_SHOW_GUI:
        Carla.gui.emit(SIGNAL("ShowGuiCallback(int, int)"), pluginId, value1)
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
    elif action == CALLBACK_PATCHBAY_CLIENT_ADDED:
        Carla.gui.emit(SIGNAL("PatchbayClientAddedCallback(int, QString)"), value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayClientRemovedCallback(int)"), value1)
    elif action == CALLBACK_PATCHBAY_CLIENT_RENAMED:
        Carla.gui.emit(SIGNAL("PatchbayClientRenamedCallback(int, QString)"), value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_ADDED:
        Carla.gui.emit(SIGNAL("PatchbayPortAddedCallback(int, int, int, QString)"), value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayPortRemovedCallback(int)"), value1)
    elif action == CALLBACK_PATCHBAY_PORT_RENAMED:
        Carla.gui.emit(SIGNAL("PatchbayPortRenamedCallback(int, QString)"), value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CONNECTION_ADDED:
        Carla.gui.emit(SIGNAL("PatchbayConnectionAddedCallback(int, int, int)"), value1, value2, value3)
    elif action == CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayConnectionRemovedCallback(int)"), value1)
    #elif action == CALLBACK_NSM_ANNOUNCE:
        #Carla.gui._nsmAnnounce2str = cString(Carla.host.get_last_error())
        #Carla.gui.emit(SIGNAL("NSM_AnnounceCallback()"))
    #elif action == CALLBACK_NSM_OPEN1:
        #Carla.gui._nsmOpen1str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open1Callback()"))
    #elif action == CALLBACK_NSM_OPEN2:
        #Carla.gui._nsmOpen2str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open2Callback()"))
    #elif action == CALLBACK_NSM_SAVE:
        #Carla.gui.emit(SIGNAL("NSM_SaveCallback()"))
    elif action == CALLBACK_ERROR:
        Carla.gui.emit(SIGNAL("ErrorCallback(QString)"), cString(valueStr))
    elif action == CALLBACK_QUIT:
        Carla.gui.emit(SIGNAL("QuitCallback()"))

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("falkTX")
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
    Carla.host.set_engine_callback(engineCallback)
    Carla.host.set_engine_option(OPTION_PROCESS_NAME, 0, "carla")

    # Set bridge paths
    if carla_bridge_native:
        Carla.host.set_engine_option(OPTION_PATH_BRIDGE_NATIVE, 0, carla_bridge_native)

    if carla_bridge_posix32:
        Carla.host.set_engine_option(OPTION_PATH_BRIDGE_POSIX32, 0, carla_bridge_posix32)

    if carla_bridge_posix64:
        Carla.host.set_engine_option(OPTION_PATH_BRIDGE_POSIX64, 0, carla_bridge_posix64)

    if carla_bridge_win32:
        Carla.host.set_engine_option(OPTION_PATH_BRIDGE_WIN32, 0, carla_bridge_win32)

    if carla_bridge_win64:
        Carla.host.set_engine_option(OPTION_PATH_BRIDGE_WIN64, 0, carla_bridge_win64)

    if WINDOWS:
        if carla_bridge_lv2_windows:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, carla_bridge_lv2_windows)

        if carla_bridge_vst_hwnd:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_HWND, 0, carla_bridge_vst_hwnd)

    elif MACOS:
        if carla_bridge_lv2_cocoa:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_COCOA, 0, carla_bridge_lv2_cocoa)

        if carla_bridge_vst_cocoa:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_COCOA, 0, carla_bridge_vst_cocoa)

    else:
        if carla_bridge_lv2_gtk2:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_GTK2, 0, carla_bridge_lv2_gtk2)

        if carla_bridge_lv2_gtk3:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_GTK3, 0, carla_bridge_lv2_gtk3)

        if carla_bridge_lv2_qt4:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_QT4, 0, carla_bridge_lv2_qt4)

        if carla_bridge_lv2_qt5:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_QT5, 0, carla_bridge_lv2_qt5)

        if carla_bridge_lv2_x11:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_X11, 0, carla_bridge_lv2_x11)

        if carla_bridge_vst_x11:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_X11, 0, carla_bridge_vst_x11)

    # Create GUI and start engine
    Carla.gui = CarlaMainW()

    # Set-up custom signal handling
    setUpSignals()

    # Show GUI
    Carla.gui.show()

    # Load project file if set
    if projectFilename:
        Carla.gui.loadProjectLater(projectFilename)

    # App-Loop
    ret = app.exec_()

    # Exit properly
    sys.exit(ret)
