#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla settings code
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

try:
    from PyQt5.QtCore import pyqtSlot, QByteArray, QSettings
    from PyQt5.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt5.QtWidgets import QDialog, QFrame, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget
except:
    from PyQt4.QtCore import pyqtSlot, QByteArray, QSettings
    from PyQt4.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt4.QtGui import QDialog, QFrame, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

#import patchcanvas
import ui_carla_settings
import ui_carla_settings_driver

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Carla defaults

# Canvas size
CARLA_DEFAULT_CANVAS_WIDTH  = 3100
CARLA_DEFAULT_CANVAS_HEIGHT = 2400

# Engine settings
CARLA_DEFAULT_RUN_DISCOVERY_CHECKS  = True
CARLA_DEFAULT_FORCE_STEREO          = False
CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES = False
CARLA_DEFAULT_PREFER_UI_BRIDGES     = True
CARLA_DEFAULT_UIS_ALWAYS_ON_TOP     = True
CARLA_DEFAULT_MAX_PARAMETERS        = MAX_DEFAULT_PARAMETERS
CARLA_DEFAULT_UI_BRIDGES_TIMEOUT    = 4000
CARLA_DEFAULT_AUDIO_NUM_PERIODS     = 2
CARLA_DEFAULT_AUDIO_BUFFER_SIZE     = 512
CARLA_DEFAULT_AUDIO_SAMPLE_RATE     = 44100

if WINDOWS:
    CARLA_DEFAULT_AUDIO_DRIVER = "DirectSound"
elif MACOS:
    CARLA_DEFAULT_AUDIO_DRIVER = "CoreAudio"
else:
    CARLA_DEFAULT_AUDIO_DRIVER = "JACK"

if LINUX:
    CARLA_DEFAULT_PROCESS_MODE   = PROCESS_MODE_MULTIPLE_CLIENTS
    CARLA_DEFAULT_TRANSPORT_MODE = TRANSPORT_MODE_JACK
else:
    CARLA_DEFAULT_PROCESS_MODE   = PROCESS_MODE_CONTINUOUS_RACK
    CARLA_DEFAULT_TRANSPORT_MODE = TRANSPORT_MODE_INTERNAL

# ------------------------------------------------------------------------------------------------------------
# ...

BUFFER_SIZE_LIST = (16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192)
SAMPLE_RATE_LIST = (22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000)

# ------------------------------------------------------------------------------------------------------------
# Driver Settings

class DriverSettingsW(QDialog):
    def __init__(self, parent, driverIndex, driverName):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_settings_driver.Ui_DriverSettingsW()
        self.ui.setupUi(self)

        if driverName == "JACK":
            print("This dialog is not compatible with JACK")
            return

        # -------------------------------------------------------------
        # Internal stuff

        self.fDriverIndex = driverIndex
        self.fDriverName  = driverName
        self.fDeviceNames = Carla.host.get_engine_driver_device_names(driverIndex) if Carla.host is not None else []

        # -------------------------------------------------------------
        # Set-up GUI

        for name in self.fDeviceNames:
            self.ui.cb_device.addItem(name)

        for bsize in BUFFER_SIZE_LIST:
            self.ui.cb_buffersize.addItem(str(bsize))

        for srate in SAMPLE_RATE_LIST:
            self.ui.cb_samplerate.addItem(str(srate))

        # -------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------
        # Set-up connections

        self.accepted.connect(self.slot_saveSettings)

        # -------------------------------------------------------------

    def loadSettings(self):
        settings = QSettings()

        audioDevice     = settings.value("Engine/Driver-%s/Device"     % self.fDriverName, "",                              type=str)
        audioNumPeriods = settings.value("Engine/Driver-%s/NumPeriods" % self.fDriverName, CARLA_DEFAULT_AUDIO_NUM_PERIODS, type=int)
        audioBufferSize = settings.value("Engine/Driver-%s/BufferSize" % self.fDriverName, CARLA_DEFAULT_AUDIO_BUFFER_SIZE, type=int)
        audioSampleRate = settings.value("Engine/Driver-%s/SampleRate" % self.fDriverName, CARLA_DEFAULT_AUDIO_SAMPLE_RATE, type=int)

        if audioDevice and audioDevice in self.fDeviceNames:
            self.ui.cb_device.setCurrentIndex(self.fDeviceNames.index(audioDevice))
        else:
            self.ui.cb_device.setCurrentIndex(-1)

        if 2 < audioNumPeriods < 3:
            self.ui.sb_numperiods.setValue(audioNumPeriods)
        else:
            self.ui.sb_numperiods.setValue(CARLA_DEFAULT_AUDIO_NUM_PERIODS)

        if audioBufferSize and audioBufferSize in BUFFER_SIZE_LIST:
            self.ui.cb_buffersize.setCurrentIndex(BUFFER_SIZE_LIST.index(audioBufferSize))
        else:
            self.ui.cb_buffersize.setCurrentIndex(BUFFER_SIZE_LIST.index(CARLA_DEFAULT_AUDIO_BUFFER_SIZE))

        if audioSampleRate and audioSampleRate in SAMPLE_RATE_LIST:
            self.ui.cb_samplerate.setCurrentIndex(SAMPLE_RATE_LIST.index(audioSampleRate))
        else:
            self.ui.cb_samplerate.setCurrentIndex(SAMPLE_RATE_LIST.index(CARLA_DEFAULT_AUDIO_SAMPLE_RATE))

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings()

        settings.setValue("Engine/Driver-%s/Device"     % self.fDriverName, self.ui.cb_device.currentText())
        settings.setValue("Engine/Driver-%s/NumPeriods" % self.fDriverName, self.ui.sb_numperiods.value())
        settings.setValue("Engine/Driver-%s/BufferSize" % self.fDriverName, self.ui.cb_buffersize.currentText())
        settings.setValue("Engine/Driver-%s/SampleRate" % self.fDriverName, self.ui.cb_samplerate.currentText())

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Settings Dialog

class CarlaSettingsW(QDialog):
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

    def __init__(self, parent, hasGL):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_settings.Ui_CarlaSettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        driverCount = Carla.host.get_engine_driver_count() if Carla.host is not None else 0

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
            self.ui.group_theme.setEnabled(False)
            self.ui.ch_theme_pro.setChecked(False)

        # -------------------------------------------------------------
        # Set-up connections

        #self.connect(self, SIGNAL("accepted()"), SLOT("slot_saveSettings()"))
        #self.connect(self.ui.buttonBox.button(QDialogButtonBox.Reset), SIGNAL("clicked()"), SLOT("slot_resetSettings()"))

        #self.connect(self.ui.b_main_def_folder_open, SIGNAL("clicked()"), SLOT("slot_getAndSetProjectPath()"))
        #self.connect(self.ui.cb_engine_audio_driver, SIGNAL("currentIndexChanged(int)"), SLOT("slot_engineAudioDriverChanged()"))
        #self.connect(self.ui.tb_engine_driver_config, SIGNAL("clicked()"), SLOT("slot_showAudioDriverSettings()"))
        #self.connect(self.ui.b_paths_add, SIGNAL("clicked()"), SLOT("slot_addPluginPath()"))
        #self.connect(self.ui.b_paths_remove, SIGNAL("clicked()"), SLOT("slot_removePluginPath()"))
        #self.connect(self.ui.b_paths_change, SIGNAL("clicked()"), SLOT("slot_changePluginPath()"))
        #self.connect(self.ui.tw_paths, SIGNAL("currentChanged(int)"), SLOT("slot_pluginPathTabChanged(int)"))
        #self.connect(self.ui.lw_ladspa, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        #self.connect(self.ui.lw_dssi, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        #self.connect(self.ui.lw_lv2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        #self.connect(self.ui.lw_vst, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))
        #self.connect(self.ui.lw_sf2, SIGNAL("currentRowChanged(int)"), SLOT("slot_pluginPathRowChanged(int)"))

        # -------------------------------------------------------------
        # Post-connect setup

        # TODO - add AU and csound, hide AU on non-mac

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
        self.ui.ch_theme_pro.setChecked(settings.value("Main/UseProTheme", True, type=bool))
        self.ui.sb_gui_refresh.setValue(settings.value("Main/RefreshInterval", 50, type=int))

        themeColor = settings.value("Main/ProThemeColor", "Black", type=str)

        if themeColor == "System":
            self.ui.cb_theme_color.setCurrentIndex(1)
        else:
            self.ui.cb_theme_color.setCurrentIndex(0)

        # ---------------------------------------

        #self.ui.cb_canvas_hide_groups.setChecked(settings.value("Canvas/AutoHideGroups", False, type=bool))
        #self.ui.cb_canvas_bezier_lines.setChecked(settings.value("Canvas/UseBezierLines", True, type=bool))
        #self.ui.cb_canvas_eyecandy.setCheckState(settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int))
        #self.ui.cb_canvas_use_opengl.setChecked(settings.value("Canvas/UseOpenGL", False, type=bool))
        #self.ui.cb_canvas_render_aa.setCheckState(settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int))
        #self.ui.cb_canvas_render_hq_aa.setChecked(settings.value("Canvas/HighQualityAntialiasing", False, type=bool))

        #canvasThemeName = settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str)

        #for i in range(patchcanvas.Theme.THEME_MAX):
            #thisThemeName = patchcanvas.getThemeName(i)
            #self.ui.cb_canvas_theme.addItem(thisThemeName)
            #if thisThemeName == canvasThemeName:
                #self.ui.cb_canvas_theme.setCurrentIndex(i)

        # --------------------------------------------

        audioDriver = settings.value("Engine/AudioDriver", CARLA_DEFAULT_AUDIO_DRIVER, type=str)

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
            processModeIndex -= self.PROCESS_MODE_NON_JACK_PADDING
            self.ui.cb_engine_process_mode_other.setCurrentIndex(processModeIndex)
            self.ui.sw_engine_process_mode.setCurrentIndex(1)

        self.ui.sb_engine_max_params.setValue(settings.value("Engine/MaxParameters", CARLA_DEFAULT_MAX_PARAMETERS, type=int))
        self.ui.ch_engine_uis_always_on_top.setChecked(settings.value("Engine/UIsAlwaysOnTop", CARLA_DEFAULT_UIS_ALWAYS_ON_TOP, type=bool))
        self.ui.ch_engine_prefer_ui_bridges.setChecked(settings.value("Engine/PreferUiBridges", CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool))
        #self.ui.sb_engine_oscgui_timeout.setValue(settings.value("Engine/OscUiTimeout", CARLA_DEFAULT_OSC_UI_TIMEOUT, type=int))
        #self.ui.ch_engine_disable_checks.setChecked(settings.value("Engine/DisableChecks", CARLA_DEFAULT_DISABLE_CHECKS, type=bool))
        self.ui.ch_engine_prefer_plugin_bridges.setChecked(settings.value("Engine/PreferPluginBridges", CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool))
        self.ui.ch_engine_force_stereo.setChecked(settings.value("Engine/ForceStereo", CARLA_DEFAULT_FORCE_STEREO, type=bool))

        # --------------------------------------------

        ladspas = toList(settings.value("Paths/LADSPA", Carla.DEFAULT_LADSPA_PATH))
        dssis = toList(settings.value("Paths/DSSI", Carla.DEFAULT_DSSI_PATH))
        lv2s = toList(settings.value("Paths/LV2", Carla.DEFAULT_LV2_PATH))
        vsts = toList(settings.value("Paths/VST", Carla.DEFAULT_VST_PATH))
        gigs = toList(settings.value("Paths/GIG", Carla.DEFAULT_GIG_PATH))
        sf2s = toList(settings.value("Paths/SF2", Carla.DEFAULT_SF2_PATH))
        sfzs = toList(settings.value("Paths/SFZ", Carla.DEFAULT_SFZ_PATH))

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

        settings.setValue("Main/DefaultProjectFolder", self.ui.le_main_def_folder.text())
        settings.setValue("Main/UseProTheme", self.ui.ch_theme_pro.isChecked())
        settings.setValue("Main/ProThemeColor", self.ui.cb_theme_color.currentText())
        settings.setValue("Main/RefreshInterval", self.ui.sb_gui_refresh.value())

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

        if audioDriver:
            settings.setValue("Engine/AudioDriver", audioDriver)

            if audioDriver == "JACK":
                settings.setValue("Engine/ProcessMode", self.ui.cb_engine_process_mode_jack.currentIndex())
            else:
                settings.setValue("Engine/ProcessMode", self.ui.cb_engine_process_mode_other.currentIndex()+self.PROCESS_MODE_NON_JACK_PADDING)

        settings.setValue("Engine/MaxParameters", self.ui.sb_engine_max_params.value())
        settings.setValue("Engine/UIsAlwaysOnTop", self.ui.ch_engine_uis_always_on_top.isChecked())
        settings.setValue("Engine/PreferUiBridges", self.ui.ch_engine_prefer_ui_bridges.isChecked())
        #settings.setValue("Engine/OscUiTimeout", self.ui.sb_engine_oscgui_timeout.value())
        settings.setValue("Engine/DisableChecks", self.ui.ch_engine_disable_checks.isChecked())
        settings.setValue("Engine/UseDssiVstChunks", self.ui.ch_engine_dssi_chunks.isChecked())
        settings.setValue("Engine/PreferPluginBridges", self.ui.ch_engine_prefer_plugin_bridges.isChecked())
        settings.setValue("Engine/ForceStereo", self.ui.ch_engine_force_stereo.isChecked())

        # --------------------------------------------

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
        if self.ui.lw_page.currentRow() == self.TAB_INDEX_MAIN:
            self.ui.le_main_def_folder.setText(HOME)
            self.ui.ch_theme_pro.setChecked(True)
            self.ui.cb_theme_color.setCurrentIndex(0)
            self.ui.sb_gui_refresh.setValue(50)

        elif self.ui.lw_page.currentRow() == self.TAB_INDEX_CANVAS:
            self.ui.cb_canvas_theme.setCurrentIndex(0)
            self.ui.cb_canvas_hide_groups.setChecked(False)
            self.ui.cb_canvas_bezier_lines.setChecked(True)
            self.ui.cb_canvas_eyecandy.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_use_opengl.setChecked(False)
            self.ui.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked)
            self.ui.cb_canvas_render_hq_aa.setChecked(False)

        elif self.ui.lw_page.currentRow() == self.TAB_INDEX_CARLA_ENGINE:
            self.ui.cb_engine_audio_driver.setCurrentIndex(0)
            self.ui.sb_engine_max_params.setValue(CARLA_DEFAULT_MAX_PARAMETERS)
            self.ui.ch_engine_uis_always_on_top.setChecked(CARLA_DEFAULT_UIS_ALWAYS_ON_TOP)
            self.ui.ch_engine_prefer_ui_bridges.setChecked(CARLA_DEFAULT_PREFER_UI_BRIDGES)
            #self.ui.sb_engine_oscgui_timeout.setValue(CARLA_DEFAULT_OSC_UI_TIMEOUT)
            #self.ui.ch_engine_disable_checks.setChecked(CARLA_DEFAULT_DISABLE_CHECKS)
            self.ui.ch_engine_prefer_plugin_bridges.setChecked(CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES)
            self.ui.ch_engine_force_stereo.setChecked(CARLA_DEFAULT_FORCE_STEREO)

            if self.ui.cb_engine_audio_driver.currentText() == "JACK":
                self.ui.cb_engine_process_mode_jack.setCurrentIndex(PROCESS_MODE_MULTIPLE_CLIENTS)
                self.ui.sw_engine_process_mode.setCurrentIndex(0)
            else:
                self.ui.cb_engine_process_mode_other.setCurrentIndex(PROCESS_MODE_CONTINUOUS_RACK-self.PROCESS_MODE_NON_JACK_PADDING)
                self.ui.sw_engine_process_mode.setCurrentIndex(1)

        elif self.ui.lw_page.currentRow() == self.TAB_INDEX_CARLA_PATHS:
            if self.ui.tw_paths.currentIndex() == 0:
                paths = DEFAULT_LADSPA_PATH.split(splitter)
                paths.sort()
                self.ui.lw_ladspa.clear()

                for ladspa in paths:
                    self.ui.lw_ladspa.addItem(ladspa)

            elif self.ui.tw_paths.currentIndex() == 1:
                paths = DEFAULT_DSSI_PATH.split(splitter)
                paths.sort()
                self.ui.lw_dssi.clear()

                for dssi in paths:
                    self.ui.lw_dssi.addItem(dssi)

            elif self.ui.tw_paths.currentIndex() == 2:
                paths = DEFAULT_LV2_PATH.split(splitter)
                paths.sort()
                self.ui.lw_lv2.clear()

                for lv2 in paths:
                    self.ui.lw_lv2.addItem(lv2)

            elif self.ui.tw_paths.currentIndex() == 3:
                paths = DEFAULT_VST_PATH.split(splitter)
                paths.sort()
                self.ui.lw_vst.clear()

                for vst in paths:
                    self.ui.lw_vst.addItem(vst)

            elif self.ui.tw_paths.currentIndex() == 4:
                paths = DEFAULT_GIG_PATH.split(splitter)
                paths.sort()
                self.ui.lw_gig.clear()

                for gig in paths:
                    self.ui.lw_gig.addItem(gig)

            elif self.ui.tw_paths.currentIndex() == 5:
                paths = DEFAULT_SF2_PATH.split(splitter)
                paths.sort()
                self.ui.lw_sf2.clear()

                for sf2 in paths:
                    self.ui.lw_sf2.addItem(sf2)

            elif self.ui.tw_paths.currentIndex() == 6:
                paths = DEFAULT_SFZ_PATH.split(splitter)
                paths.sort()
                self.ui.lw_sfz.clear()

                for sfz in paths:
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
    def slot_showAudioDriverSettings(self):
        driverIndex = self.ui.cb_engine_audio_driver.currentIndex()
        driverName  = self.ui.cb_engine_audio_driver.currentText()
        DriverSettingsW(self, driverIndex, driverName).exec_()

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
# TESTING

#from PyQt5.QtWidgets import QApplication
#app = QApplication(sys.argv)
#gui = CarlaSettingsW(None, True)
#gui.show()
#app.exec_()
