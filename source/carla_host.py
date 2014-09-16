#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla host code
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

if config_UseQt5:
    from PyQt5.QtCore import qCritical, QFileInfo, QModelIndex, QTimer
    from PyQt5.QtGui import QPalette
    from PyQt5.QtWidgets import QAction, QApplication, QFileSystemModel, QListWidgetItem, QMainWindow
else:
    from PyQt4.QtCore import qCritical, QFileInfo, QModelIndex, QTimer
    from PyQt4.QtGui import QApplication, QFileSystemModel, QListWidgetItem, QMainWindow, QPalette

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_host

from carla_app import *
from carla_database import *
from carla_settings import *
from carla_widgets import *

# ------------------------------------------------------------------------------------------------------------
# PatchCanvas defines

CANVAS_ANTIALIASING_SMALL = 1
CANVAS_EYECANDY_SMALL     = 1

# ------------------------------------------------------------------------------------------------------------
# Session Management support

LADISH_APP_NAME = os.getenv("LADISH_APP_NAME")
NSM_URL         = os.getenv("NSM_URL")

# ------------------------------------------------------------------------------------------------------------
# Dummy widget

class HostWidgetMeta(object):
#class HostWidgetMeta(metaclass=ABCMeta):

    # --------------------------------------------------------------------------------------------------------

    def removeAllPlugins(self):
        raise NotImplementedError

    def engineStarted(self):
        raise NotImplementedError

    def engineStopped(self):
        raise NotImplementedError

    def idleFast(self):
        raise NotImplementedError

    def idleSlow(self):
        raise NotImplementedError

    def projectLoadingStarted(self):
        raise NotImplementedError

    def projectLoadingFinished(self):
        raise NotImplementedError

    def saveSettings(self, settings):
        raise NotImplementedError

    def showEditDialog(self, pluginId):
        raise NotImplementedError

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    # --------------------------------------------------------------------------------------------------------

    def __init__(self, host, parent=None):
        QMainWindow.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_host.Ui_CarlaHostW()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostMeta()
            self.host = host
            self.fContainer = HostWidgetMeta(self, host)
            gCarla.gui = self

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fSampleRate = 0.0

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fLastTransportFrame = 0
        self.fLastTransportState = False
        self.fTransportText = ""

        self.fProjectFilename = ""
        self.fIsProjectLoading = False

        # first attempt of auto-start engine doesn't show an error
        self.fFirstEngineInit = True

        # to be filled with key-value pairs of current settings
        self.fSavedSettings = {}

        if host.isPlugin:
            self.fClientName         = "Carla-Plugin"
            self.fSessionManagerName = "Plugin"
        elif LADISH_APP_NAME:
            self.fClientName         = LADISH_APP_NAME
            self.fSessionManagerName = "LADISH"
        elif NSM_URL:
            self.fClientName         = "Carla" # "Carla.tmp"
            self.fSessionManagerName = "Non Session Manager"
        else:
            self.fClientName         = "Carla"
            self.fSessionManagerName = ""

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (engine stopped)

        if self.host.isPlugin:
            self.ui.act_engine_start.setEnabled(False)
            self.ui.menu_Engine.setEnabled(False)
        else:
            self.ui.act_engine_start.setEnabled(True)

        if self.fSessionManagerName:
            self.ui.act_file_new.setEnabled(False)

        self.ui.act_file_open.setEnabled(False)
        self.ui.act_file_save.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        self.ui.act_canvas_show_internal.setChecked(True)
        self.ui.act_canvas_show_external.setChecked(False)

        self.ui.menu_PluginMacros.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)

        self.ui.dockWidgetTitleBar = QWidget(self)
        self.ui.dockWidget.setTitleBarWidget(self.ui.dockWidgetTitleBar)

        self.setTransportMenuEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (disk)

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setRootPath(HOME)
        self.fDirModel.setNameFilters(host.get_supported_file_extensions().split(";"))

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (special stuff for Mac OS)

        if MACOS and config_UseQt5:
            self.ui.act_file_quit.setMenuRole(QAction.QuitRole)
            self.ui.act_settings_configure.setMenuRole(QAction.PreferencesRole)
            self.ui.act_help_about.setMenuRole(QAction.AboutRole)
            self.ui.act_help_about_juce.setMenuRole(QAction.AboutRole)
            self.ui.act_help_about_qt.setMenuRole(QAction.AboutQtRole)
            self.ui.menu_Settings.setTitle("Panels")
            #self.ui.menu_Help.hide()

        # ----------------------------------------------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_new.triggered.connect(self.slot_fileNew)
        self.ui.act_file_open.triggered.connect(self.slot_fileOpen)
        self.ui.act_file_save.triggered.connect(self.slot_fileSave)
        self.ui.act_file_save_as.triggered.connect(self.slot_fileSaveAs)

        self.ui.act_engine_start.triggered.connect(self.slot_engineStart)
        self.ui.act_engine_stop.triggered.connect(self.slot_engineStop)

        self.ui.act_plugin_add.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_add2.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_remove_all.triggered.connect(self.slot_pluginRemoveAll)

        self.ui.act_transport_play.triggered.connect(self.slot_transportPlayPause)
        self.ui.act_transport_stop.triggered.connect(self.slot_transportStop)
        self.ui.act_transport_backwards.triggered.connect(self.slot_transportBackwards)
        self.ui.act_transport_forwards.triggered.connect(self.slot_transportForwards)

        self.ui.act_help_about.triggered.connect(self.slot_aboutCarla)
        self.ui.act_help_about_juce.triggered.connect(self.slot_aboutJuce)
        self.ui.act_help_about_qt.triggered.connect(self.slot_aboutQt)

        self.ui.cb_disk.currentIndexChanged.connect(self.slot_diskFolderChanged)
        self.ui.b_disk_add.clicked.connect(self.slot_diskFolderAdd)
        self.ui.b_disk_remove.clicked.connect(self.slot_diskFolderRemove)
        self.ui.fileTreeView.doubleClicked.connect(self.slot_fileTreeDoubleClicked)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

        host.EngineStartedCallback.connect(self.slot_handleEngineStartedCallback)
        host.EngineStoppedCallback.connect(self.slot_handleEngineStoppedCallback)
        host.SampleRateChangedCallback.connect(self.slot_handleSampleRateChangedCallback)

        host.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        host.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)

        host.DebugCallback.connect(self.slot_handleDebugCallback)
        host.InfoCallback.connect(self.slot_handleInfoCallback)
        host.ErrorCallback.connect(self.slot_handleErrorCallback)
        host.QuitCallback.connect(self.slot_handleQuitCallback)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        self.setProperWindowTitle()

        QTimer.singleShot(0, self.slot_engineStart)

    # --------------------------------------------------------------------------------------------------------

    def setProperWindowTitle(self):
        title = self.fClientName

        if self.fProjectFilename:
            title += " - %s" % os.path.basename(self.fProjectFilename)
        if self.fSessionManagerName:
            title += " (%s)" % self.fSessionManagerName

        self.setWindowTitle(title)

    # --------------------------------------------------------------------------------------------------------
    # Called by containers

    def isProjectLoading(self):
        return self.fIsProjectLoading

    def getSavedSettings(self):
        return self.fSavedSettings

    def setLoadRDFsNeeded(self):
        self.fLadspaRdfNeedsUpdate = True

    def setupContainer(self, showCanvas, canvasThemeData = []):
        if showCanvas:
            canvasWidth, canvasHeight, canvasBg, canvasBrush, canvasPen = canvasThemeData
            self.ui.miniCanvasPreview.setViewTheme(canvasBg, canvasBrush, canvasPen)
            self.ui.miniCanvasPreview.init(self.fContainer.scene, canvasWidth, canvasHeight, self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING])
        else:
            self.ui.act_canvas_show_internal.setEnabled(False)
            self.ui.act_canvas_show_external.setEnabled(False)
            self.ui.act_canvas_arrange.setVisible(False)
            self.ui.act_canvas_print.setVisible(False)
            self.ui.act_canvas_refresh.setVisible(False)
            self.ui.act_canvas_save_image.setVisible(False)
            self.ui.act_canvas_zoom_100.setVisible(False)
            self.ui.act_canvas_zoom_fit.setVisible(False)
            self.ui.act_canvas_zoom_in.setVisible(False)
            self.ui.act_canvas_zoom_out.setVisible(False)
            self.ui.act_settings_show_meters.setVisible(False)
            self.ui.act_settings_show_keyboard.setVisible(False)
            self.ui.menu_Canvas.setEnabled(False)
            self.ui.menu_Canvas.setVisible(False)
            self.ui.menu_Canvas_Zoom.setEnabled(False)
            self.ui.menu_Canvas_Zoom.setVisible(False)
            self.ui.miniCanvasPreview.hide()

        self.setCentralWidget(self.fContainer)
        self.ui.centralwidget = self.fContainer

    def updateContainer(self, canvasThemeData):
        canvasWidth, canvasHeight, canvasBg, canvasBrush, canvasPen = canvasThemeData
        self.ui.miniCanvasPreview.setViewTheme(canvasBg, canvasBrush, canvasPen)
        self.ui.miniCanvasPreview.init(self.fContainer.scene, canvasWidth, canvasHeight, self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING])

    # --------------------------------------------------------------------------------------------------------
    # Called by the signal handler

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        self.slot_fileSave()

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (files)

    def loadProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: loading project without filename set")

        self.fContainer.projectLoadingStarted()
        self.fIsProjectLoading = True

        self.host.load_project(self.fProjectFilename)

        self.fIsProjectLoading = False
        self.fContainer.projectLoadingFinished()

    def loadProjectLater(self, filename):
        self.fProjectFilename = QFileInfo(filename).absoluteFilePath()
        self.setProperWindowTitle()
        QTimer.singleShot(0, self.slot_loadProjectNow)

    def saveProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: saving project without filename set")

        self.host.save_project(self.fProjectFilename)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_fileNew(self):
        self.fContainer.removeAllPlugins()
        self.fProjectFilename = ""
        self.setProperWindowTitle()

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter = self.tr("Carla Project File (*.carxp)")
        filename   = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        newFile = True

        if self.host.get_current_plugin_count() > 0:
            ask = QMessageBox.question(self, self.tr("Question"), self.tr("There are some plugins loaded, do you want to remove them now?"),
                                                                          QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            newFile = (ask == QMessageBox.Yes)

        if newFile:
            self.fContainer.removeAllPlugins()
            self.fProjectFilename = filename
            self.setProperWindowTitle()
            self.loadProjectNow()
        else:
            filenameOld = self.fProjectFilename
            self.fProjectFilename = filename
            self.loadProjectNow()
            self.fProjectFilename = filenameOld

    @pyqtSlot()
    def slot_fileSave(self, saveAs=False):
        if self.fProjectFilename and not saveAs:
            return self.saveProjectNow()

        fileFilter = self.tr("Carla Project File (*.carxp)")
        filename   = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        if not filename.lower().endswith(".carxp"):
            filename += ".carxp"

        if self.fProjectFilename != filename:
            self.fProjectFilename = filename
            self.setProperWindowTitle()

        self.saveProjectNow()

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    @pyqtSlot()
    def slot_loadProjectNow(self):
        self.loadProjectNow()

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (engine)

    def setEngineSettings(self):
        settings = QSettings("falkTX", "Carla2")

        # ----------------------------------------------------------------------------------------------------
        # main settings

        setHostSettings(self.host)

        # ----------------------------------------------------------------------------------------------------
        # plugin paths

        LADSPA_PATH = toList(settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH))
        DSSI_PATH   = toList(settings.value(CARLA_KEY_PATHS_DSSI,   CARLA_DEFAULT_DSSI_PATH))
        LV2_PATH    = toList(settings.value(CARLA_KEY_PATHS_LV2,    CARLA_DEFAULT_LV2_PATH))
        VST_PATH    = toList(settings.value(CARLA_KEY_PATHS_VST,    CARLA_DEFAULT_VST_PATH))
        VST3_PATH   = toList(settings.value(CARLA_KEY_PATHS_VST3,   CARLA_DEFAULT_VST3_PATH))
        AU_PATH     = toList(settings.value(CARLA_KEY_PATHS_AU,     CARLA_DEFAULT_AU_PATH))
        GIG_PATH    = toList(settings.value(CARLA_KEY_PATHS_GIG,    CARLA_DEFAULT_GIG_PATH))
        SF2_PATH    = toList(settings.value(CARLA_KEY_PATHS_SF2,    CARLA_DEFAULT_SF2_PATH))
        SFZ_PATH    = toList(settings.value(CARLA_KEY_PATHS_SFZ,    CARLA_DEFAULT_SFZ_PATH))

        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, splitter.join(LADSPA_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI,   splitter.join(DSSI_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,    splitter.join(LV2_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST,    splitter.join(VST_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3,   splitter.join(VST3_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_AU,     splitter.join(AU_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_GIG,    splitter.join(GIG_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2,    splitter.join(SF2_PATH))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ,    splitter.join(SFZ_PATH))

        # ----------------------------------------------------------------------------------------------------
        # don't continue if plugin

        if self.host.isPlugin:
            return "Plugin"

        # ----------------------------------------------------------------------------------------------------
        # driver and device settings

        # driver name
        try:
            audioDriver = settings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, type=str)
        except:
            audioDriver = CARLA_DEFAULT_AUDIO_DRIVER

        # driver options
        try:
            audioDevice = settings.value("%s%s/Device" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), "", type=str)
        except:
            audioDevice = ""

        try:
            audioNumPeriods = settings.value("%s%s/NumPeriods" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_NUM_PERIODS, type=int)
        except:
            audioNumPeriods = CARLA_DEFAULT_AUDIO_NUM_PERIODS

        try:
            audioBufferSize = settings.value("%s%s/BufferSize" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_BUFFER_SIZE, type=int)
        except:
            audioBufferSize = CARLA_DEFAULT_AUDIO_BUFFER_SIZE

        try:
            audioSampleRate = settings.value("%s%s/SampleRate" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_SAMPLE_RATE, type=int)
        except:
            audioSampleRate = CARLA_DEFAULT_AUDIO_SAMPLE_RATE

        self.host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE,      0,               audioDevice)
        self.host.set_engine_option(ENGINE_OPTION_AUDIO_NUM_PERIODS, audioNumPeriods, "")
        self.host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, audioBufferSize, "")
        self.host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, audioSampleRate, "")

        # ----------------------------------------------------------------------------------------------------
        # fix things if needed

        if audioDriver != "JACK" and self.host.transportMode == ENGINE_TRANSPORT_MODE_JACK:
            self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_INTERNAL, "")

        # ----------------------------------------------------------------------------------------------------
        # return selected driver name

        return audioDriver

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_engineStart(self):
        audioDriver = self.setEngineSettings()

        if not self.host.engine_init(audioDriver, self.fClientName):
            if self.fFirstEngineInit:
                self.fFirstEngineInit = False
                return

            audioError = self.host.get_last_error()

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fFirstEngineInit = False

    @pyqtSlot()
    def slot_engineStop(self):
        if self.host.get_current_plugin_count() > 0:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask != QMessageBox.Yes:
                return

            self.ui.act_plugin_remove_all.setEnabled(False)
            self.fContainer.removeAllPlugins()

        if self.host.is_engine_running() and not self.host.engine_close():
            print(self.host.get_last_error())

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(str)
    def slot_handleEngineStartedCallback(self, processMode, transportMode, driverName):
        self.fSampleRate = self.host.get_sample_rate()

        check = self.host.is_engine_running()
        self.ui.menu_PluginMacros.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if not self.host.isPlugin:
            self.ui.act_engine_start.setEnabled(not check)
            self.ui.act_engine_stop.setEnabled(check)

            if not self.fSessionManagerName:
                self.ui.act_file_open.setEnabled(check)
                self.ui.act_file_save.setEnabled(check)
                self.ui.act_file_save_as.setEnabled(check)

            self.setTransportMenuEnabled(check)

        if check:
            self.fLastTransportFrame = 0
            self.fLastTransportState = False
            self.refreshTransport(True)
            self.fContainer.engineStarted()

        self.startTimers()

    @pyqtSlot()
    def slot_handleEngineStoppedCallback(self):
        self.killTimers()

        # just in case
        self.ui.act_plugin_remove_all.setEnabled(False)
        self.fContainer.removeAllPlugins()

        check = self.host.is_engine_running()
        self.ui.menu_PluginMacros.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if not self.host.isPlugin:
            self.ui.act_engine_start.setEnabled(not check)
            self.ui.act_engine_stop.setEnabled(check)

            if not self.fSessionManagerName:
                self.ui.act_file_open.setEnabled(check)
                self.ui.act_file_save.setEnabled(check)
                self.ui.act_file_save_as.setEnabled(check)

            self.setTransportMenuEnabled(check)

        if not check:
            self.fTextTransport = ""
            self.fContainer.engineStopped()

        self.fSampleRate = 0.0

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fSampleRate = newSampleRate
        self.refreshTransport(True)

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (plugins)

    @pyqtSlot()
    def slot_pluginAdd(self, pluginToReplace = -1):
        dialog = PluginDatabaseW(self, self.host)

        if not dialog.exec_():
            return

        if not self.host.is_engine_running():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return

        btype    = dialog.fRetPlugin['build']
        ptype    = dialog.fRetPlugin['type']
        filename = dialog.fRetPlugin['filename']
        label    = dialog.fRetPlugin['label']
        uniqueId = dialog.fRetPlugin['uniqueId']
        extraPtr = self.getExtraPtr(dialog.fRetPlugin)

        if pluginToReplace >= 0:
            if not self.host.replace_plugin(pluginToReplace):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to replace plugin"), self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
                return

        ok = self.host.add_plugin(btype, ptype, filename, None, label, uniqueId, extraPtr)

        if pluginToReplace >= 0:
            self.host.replace_plugin(self.host.get_max_plugin_number())

        if not ok:
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot()
    def slot_pluginRemoveAll(self):
        self.ui.act_plugin_remove_all.setEnabled(False)

        count = self.host.get_current_plugin_count()

        if count == 0:
            return

        self.fContainer.projectLoadingStarted()

        app = QApplication.instance()
        for i in range(count):
            app.processEvents()
            self.host.remove_plugin(count-i-1)

        self.fContainer.projectLoadingFinished()

        #self.fContainer.removeAllPlugins()
        #self.host.remove_all_plugins()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName):
        self.ui.act_plugin_remove_all.setEnabled(self.host.get_current_plugin_count() > 0)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        self.ui.act_plugin_remove_all.setEnabled(self.host.get_current_plugin_count() > 0)

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (plugin data)

    def getExtraPtr(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']

            self.maybeLoadRDFs()

            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype in (PLUGIN_GIG, PLUGIN_SF2):
            if plugin['name'].lower().endswith(" (16 outputs)"):
                global _true
                _true = c_char_p("true".encode("utf-8"))
                return _true

        return None

    def maybeLoadRDFs(self):
        if not self.fLadspaRdfNeedsUpdate:
            return

        self.fLadspaRdfNeedsUpdate = False
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

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (timers)

    def startTimers(self):
        if self.fIdleTimerFast == 0:
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])

        if self.fIdleTimerSlow == 0:
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*4)

    def restartTimersIfNeeded(self):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*4)

    def killTimers(self):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff (transport)

    def refreshTransport(self, forced = False):
        if self.fSampleRate == 0.0 or not self.host.is_engine_running():
            return

        timeInfo = self.host.get_transport_info()
        playing  = timeInfo['playing']
        frame    = timeInfo['frame']

        if playing != self.fLastTransportState or forced:
            if playing:
                icon = getIcon("media-playback-pause")
                self.ui.act_transport_play.setChecked(True)
                self.ui.act_transport_play.setIcon(icon)
                self.ui.act_transport_play.setText(self.tr("&Pause"))
            else:
                icon = getIcon("media-playback-start")
                self.ui.act_transport_play.setChecked(False)
                self.ui.act_transport_play.setIcon(icon)
                self.ui.act_transport_play.setText(self.tr("&Play"))

            self.fLastTransportState = playing

        if frame != self.fLastTransportFrame or forced:
            time = frame / self.fSampleRate
            secs = time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60

            self.fTextTransport = "Transport %s, at %02i:%02i:%02i" % ("playing" if playing else "stopped", hrs, mins, secs)
            self.fLastTransportFrame = frame

    def setTransportMenuEnabled(self, enabled):
        self.ui.act_transport_play.setEnabled(enabled)
        self.ui.act_transport_stop.setEnabled(enabled)
        self.ui.act_transport_backwards.setEnabled(enabled)
        self.ui.act_transport_forwards.setEnabled(enabled)
        self.ui.menu_Transport.setEnabled(enabled)

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if not self.host.is_engine_running():
            return

        if toggled:
            self.host.transport_play()
        else:
            self.host.transport_pause()

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if not self.host.is_engine_running():
            return

        self.host.transport_pause()
        self.host.transport_relocate(0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() - 100000

        if newFrame < 0:
            newFrame = 0

        self.host.transport_relocate(newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() + 100000
        self.host.transport_relocate(newFrame)

    # -----------------------------------------------------------------
    # Internal stuff (settings)

    def loadSettings(self, firstTime):
        settings = QSettings()

        if firstTime:
            self.restoreGeometry(settings.value("Geometry", ""))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

            #if settings.contains("SplitterState"):
                #self.ui.splitter.restoreState(settings.value("SplitterState", ""))
            #else:
                #self.ui.splitter.setSizes([210, 99999])

            diskFolders = toList(settings.value("DiskFolders", [HOME]))

            self.ui.cb_disk.setItemData(0, HOME)

            for i in range(len(diskFolders)):
                if i == 0: continue
                folder = diskFolders[i]
                self.ui.cb_disk.addItem(os.path.basename(folder), folder)

            #if MACOS and not settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, True, type=bool):
            #    self.setUnifiedTitleAndToolBarOnMac(True)

        # ---------------------------------------------

        # TODO

        self.fSavedSettings = {
            CARLA_KEY_MAIN_PROJECT_FOLDER:     settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER,     CARLA_DEFAULT_MAIN_PROJECT_FOLDER,     type=str),
            CARLA_KEY_MAIN_REFRESH_INTERVAL:   settings.value(CARLA_KEY_MAIN_REFRESH_INTERVAL,   CARLA_DEFAULT_MAIN_REFRESH_INTERVAL,   type=int),
            CARLA_KEY_MAIN_USE_CUSTOM_SKINS:   settings.value(CARLA_KEY_MAIN_USE_CUSTOM_SKINS,   CARLA_DEFAULT_MAIN_USE_CUSTOM_SKINS,   type=bool),
            CARLA_KEY_CANVAS_THEME:            settings.value(CARLA_KEY_CANVAS_THEME,            CARLA_DEFAULT_CANVAS_THEME,            type=str),
            CARLA_KEY_CANVAS_SIZE:             settings.value(CARLA_KEY_CANVAS_SIZE,             CARLA_DEFAULT_CANVAS_SIZE,             type=str),
            CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS: settings.value(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS, CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS, type=bool),
            CARLA_KEY_CANVAS_USE_BEZIER_LINES: settings.value(CARLA_KEY_CANVAS_USE_BEZIER_LINES, CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES, type=bool),
            CARLA_KEY_CANVAS_EYE_CANDY:        settings.value(CARLA_KEY_CANVAS_EYE_CANDY,        CARLA_DEFAULT_CANVAS_EYE_CANDY,        type=int),
            CARLA_KEY_CANVAS_USE_OPENGL:       settings.value(CARLA_KEY_CANVAS_USE_OPENGL,       CARLA_DEFAULT_CANVAS_USE_OPENGL,       type=bool),
            CARLA_KEY_CANVAS_ANTIALIASING:     settings.value(CARLA_KEY_CANVAS_ANTIALIASING,     CARLA_DEFAULT_CANVAS_ANTIALIASING,     type=int),
            CARLA_KEY_CANVAS_HQ_ANTIALIASING:  settings.value(CARLA_KEY_CANVAS_HQ_ANTIALIASING,  CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING,  type=bool),
            CARLA_KEY_CUSTOM_PAINTING:        (settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, True, type=bool) and
                                               settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR, "Black", type=str).lower() == "black")
        }

        # ---------------------------------------------

        self.setEngineSettings()

        self.restartTimersIfNeeded()

        # ---------------------------------------------

    def saveSettings(self):
        settings = QSettings()

        settings.setValue("Geometry", self.saveGeometry())
        #settings.setValue("SplitterState", self.ui.splitter.saveState())
        settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())

        diskFolders = []

        for i in range(self.ui.cb_disk.count()):
            diskFolders.append(self.ui.cb_disk.itemData(i))

        settings.setValue("DiskFolders", diskFolders)

        self.fContainer.saveSettings(settings)

    # -----------------------------------------------------------------
    # Internal stuff (gui)

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self, self.host).exec_()

    @pyqtSlot()
    def slot_aboutJuce(self):
        JuceAboutW(self, self.host).exec_()

    @pyqtSlot()
    def slot_aboutQt(self):
        QApplication.instance().aboutQt()

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_diskFolderChanged(self, index):
        if index < 0:
            return
        elif index == 0:
            filename = HOME
            self.ui.b_disk_remove.setEnabled(False)
        else:
            filename = self.ui.cb_disk.itemData(index)
            self.ui.b_disk_remove.setEnabled(True)

        self.fDirModel.setRootPath(filename)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(filename))

    @pyqtSlot()
    def slot_diskFolderAdd(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("New Folder"), "", QFileDialog.ShowDirsOnly)

        if newPath:
            if newPath[-1] == os.sep:
                newPath = newPath[:-1]
            self.ui.cb_disk.addItem(os.path.basename(newPath), newPath)
            self.ui.cb_disk.setCurrentIndex(self.ui.cb_disk.count()-1)
            self.ui.b_disk_remove.setEnabled(True)

    @pyqtSlot()
    def slot_diskFolderRemove(self):
        index = self.ui.cb_disk.currentIndex()

        if index <= 0:
            return

        self.ui.cb_disk.removeItem(index)

        if self.ui.cb_disk.currentIndex() == 0:
            self.ui.b_disk_remove.setEnabled(False)

    @pyqtSlot(QModelIndex)
    def slot_fileTreeDoubleClicked(self, modelIndex):
        filename = self.fDirModel.filePath(modelIndex)

        if not self.host.load_file(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG:", pluginId, value1, value2, value3, valueStr)

    @pyqtSlot(str)
    def slot_handleInfoCallback(self, info):
        QMessageBox.information(self, "Information", info)

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, "Error", error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        pass # TODO

    # -----------------------------------------------------------------

    def showEvent(self, event):
        QMainWindow.showEvent(self, event)

        # set our gui as parent for all plugins UIs
        winIdStr = "%x" % self.winId()
        self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, winIdStr)

    def hideEvent(self, event):
        # disable parent
        self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0")

        QMainWindow.hideEvent(self, event)

    # -----------------------------------------------------------------

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            #if not self.host.isPlugin:
            self.fContainer.idleFast()
            self.refreshTransport()
            self.host.engine_idle()

        elif event.timerId() == self.fIdleTimerSlow:
            self.fContainer.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.killTimers()
        self.saveSettings()

        if self.host.isPlugin:
            pass

        elif self.host.is_engine_running():
            self.host.set_engine_about_to_close()

            count = self.host.get_current_plugin_count()

            if count > 0:
                # simulate project loading, to disable container
                self.ui.act_plugin_remove_all.setEnabled(False)
                self.fContainer.projectLoadingStarted()

                app = QApplication.instance()
                for i in range(count):
                    app.processEvents()
                    self.host.remove_plugin(count-i-1)

                app.processEvents()

                #self.fContainer.removeAllPlugins()
                #self.host.remove_all_plugins()

            self.slot_engineStop()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def engineCallback(host, action, pluginId, value1, value2, value3, valueStr):
    # kdevelop likes this :)
    if False: host = CarlaHostMeta()

    if action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.processMode   = value1
        host.transportMode = value2
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.processMode   = value1
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.transportMode = value1

    valueStr = charPtrToString(valueStr)

    if action == ENGINE_CALLBACK_DEBUG:
        host.DebugCallback.emit(pluginId, value1, value2, value3, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_ADDED:
        host.PluginAddedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_REMOVED:
        host.PluginRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PLUGIN_RENAMED:
        host.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
        host.PluginUnavailableCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
        host.ParameterValueChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
        host.ParameterDefaultChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        host.ParameterMidiCcChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        host.ParameterMidiChannelChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PROGRAM_CHANGED:
        host.ProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
        host.MidiProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_OPTION_CHANGED:
        host.OptionChangedCallback.emit(pluginId, value1, bool(value2))
    elif action == ENGINE_CALLBACK_UI_STATE_CHANGED:
        host.UiStateChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_NOTE_ON:
        host.NoteOnCallback.emit(pluginId, value1, value2, int(value3))
    elif action == ENGINE_CALLBACK_NOTE_OFF:
        host.NoteOffCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_UPDATE:
        host.UpdateCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_INFO:
        host.ReloadInfoCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PARAMETERS:
        host.ReloadParametersCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PROGRAMS:
        host.ReloadProgramsCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_ALL:
        host.ReloadAllCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
        host.PatchbayClientAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
        host.PatchbayClientRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
        host.PatchbayClientRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
        host.PatchbayClientDataChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
        host.PatchbayPortAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
        host.PatchbayPortRemovedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED:
        host.PatchbayPortRenamedCallback.emit(pluginId, value1, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
        gOut, pOut, gIn, pIn = [int(i) for i in valueStr.split(":")] # FIXME
        host.PatchbayConnectionAddedCallback.emit(pluginId, gOut, pOut, gIn, pIn)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        host.PatchbayConnectionRemovedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.EngineStartedCallback.emit(value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_ENGINE_STOPPED:
        host.EngineStoppedCallback.emit()
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.ProcessModeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.TransportModeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
        host.BufferSizeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
        host.SampleRateChangedCallback.emit(value3)
    elif action == ENGINE_CALLBACK_IDLE:
        QApplication.instance().processEvents()
    elif action == ENGINE_CALLBACK_INFO:
        host.InfoCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_ERROR:
        host.ErrorCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_QUIT:
        host.QuitCallback.emit()

# ------------------------------------------------------------------------------------------------------------
# File callback

def fileCallback(ptr, action, isDir, title, filter):
    ret = ("", "") if config_UseQt5 else ""

    if action == FILE_CALLBACK_DEBUG:
        pass
    elif action == FILE_CALLBACK_OPEN:
        ret = QFileDialog.getOpenFileName(gCarla.gui, charPtrToString(title), "", charPtrToString(filter) ) #, QFileDialog.ShowDirsOnly if isDir else 0x0)
    elif action == FILE_CALLBACK_SAVE:
        ret = QFileDialog.getSaveFileName(gCarla.gui, charPtrToString(title), "", charPtrToString(filter), QFileDialog.ShowDirsOnly if isDir else 0x0)

    if config_UseQt5:
        ret = ret[0]
    if not ret:
        return None

    global fileRet
    fileRet = c_char_p(ret.encode("utf-8"))
    retval  = cast(byref(fileRet), POINTER(c_uintptr))
    return retval.contents.value

# ------------------------------------------------------------------------------------------------------------
# Init host

def initHost(initName, libPrefixOrPluginClass, isControl, isPlugin, failError):
    if isPlugin:
        PluginClass = libPrefixOrPluginClass
        libPrefix   = None
    else:
        PluginClass = None
        libPrefix   = libPrefixOrPluginClass

    # --------------------------------------------------------------------------------------------------------
    # Set Carla library name

    libname = "libcarla_"

    if isControl:
        libname += "control2"
    else:
        libname += "standalone2"

    if WINDOWS:
        libname += ".dll"
    elif MACOS:
        libname += ".dylib"
    else:
        libname += ".so"

    # --------------------------------------------------------------------------------------------------------
    # Set binary dir

    CWDl = CWD.lower()

    # standalone, installed system-wide linux
    if libPrefix is not None:
        pathBinaries  = os.path.join(libPrefix, "lib", "carla")
        pathResources = os.path.join(libPrefix, "share", "carla", "resources")

    # standalone, local source
    elif CWDl.endswith("source"):
        pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "bin"))
        pathResources = os.path.join(pathBinaries, "resources")

    # plugin
    elif CWDl.endswith("resources"):
        # installed system-wide linux
        if CWDl.endswith("/share/carla/resources"):
            pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "..", "..", "lib", "carla"))
            pathResources = CWD

        # local source
        elif CWDl.endswith("native-plugins%sresources" % os.sep):
            pathBinaries  = os.path.abspath(os.path.join(CWD, "..", "..", "..", "..", "bin"))
            pathResources = CWD

        # other
        else:
            pathBinaries  = os.path.abspath(os.path.join(CWD, ".."))
            pathResources = CWD

    # everything else
    else:
        pathBinaries  = CWD
        pathResources = os.path.join(pathBinaries, "resources")

    # --------------------------------------------------------------------------------------------------------
    # Fail if binary dir is not found

    if not os.path.exists(pathBinaries):
        if failError:
            QMessageBox.critical(None, "Error", "Failed to find the carla binaries, cannot continue")
            sys.exit(1)
        return

    # --------------------------------------------------------------------------------------------------------
    # Print info

    print("Carla %s started, status:" % VERSION)
    print("  Python version: %s" % sys.version.split(" ",1)[0])
    print("  Qt version:     %s" % qVersion())
    print("  PyQt version:   %s" % PYQT_VERSION_STR)
    print("  Binary dir:     %s" % pathBinaries)
    print("  Resources dir:  %s" % pathResources)

    # --------------------------------------------------------------------------------------------------------
    # Init host

    if failError:
        # no try
        host = PluginClass() if isPlugin else CarlaHostDLL(os.path.join(pathBinaries, libname))
    else:
        try:
            host = PluginClass() if isPlugin else CarlaHostDLL(os.path.join(pathBinaries, libname))
        except:
            host = CarlaHostNull()

    host.isControl = isControl
    host.isPlugin  = isPlugin

    host.set_engine_callback(lambda h,a,p,v1,v2,v3,vs: engineCallback(host,a,p,v1,v2,v3,vs))
    host.set_file_callback(fileCallback)

    # If it's a plugin the paths are already set
    if not isPlugin:
        host.pathBinaries  = pathBinaries
        host.pathResources = pathResources
        host.set_engine_option(ENGINE_OPTION_PATH_BINARIES,  0, pathBinaries)
        host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, pathResources)

        if not isControl:
            host.set_engine_option(ENGINE_OPTION_NSM_INIT, os.getpid(), initName)

    return host

# ------------------------------------------------------------------------------------------------------------
# Load host settings

def loadHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostMeta()

    settings = QSettings("falkTX", "Carla2")

    # bool values
    try:
        host.forceStereo = settings.value(CARLA_KEY_ENGINE_FORCE_STEREO, CARLA_DEFAULT_FORCE_STEREO, type=bool)
    except:
        host.forceStereo = CARLA_DEFAULT_FORCE_STEREO

    try:
        host.preferPluginBridges = settings.value(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool)
    except:
        host.preferPluginBridges = CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES

    try:
        host.preferUIBridges = settings.value(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES, CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool)
    except:
        host.preferUIBridges = CARLA_DEFAULT_PREFER_UI_BRIDGES

    try:
        host.uisAlwaysOnTop = settings.value(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP, CARLA_DEFAULT_UIS_ALWAYS_ON_TOP, type=bool)
    except:
        host.uisAlwaysOnTop = CARLA_DEFAULT_UIS_ALWAYS_ON_TOP

    # int values
    try:
        host.maxParameters = settings.value(CARLA_KEY_ENGINE_MAX_PARAMETERS, CARLA_DEFAULT_MAX_PARAMETERS, type=int)
    except:
        host.maxParameters = CARLA_DEFAULT_MAX_PARAMETERS

    try:
        host.uiBridgesTimeout = settings.value(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT, CARLA_DEFAULT_UI_BRIDGES_TIMEOUT, type=int)
    except:
        host.uiBridgesTimeout = CARLA_DEFAULT_UI_BRIDGES_TIMEOUT

    if host.isPlugin:
        return

    # enums
    try:
        host.transportMode = settings.value(CARLA_KEY_ENGINE_TRANSPORT_MODE, CARLA_DEFAULT_TRANSPORT_MODE, type=int)
    except:
        host.transportMode = CARLA_DEFAULT_TRANSPORT_MODE

    if not host.processModeForced:
        try:
            host.processMode = settings.value(CARLA_KEY_ENGINE_PROCESS_MODE, CARLA_DEFAULT_PROCESS_MODE, type=int)
        except:
            host.processMode = CARLA_DEFAULT_PROCESS_MODE

    # --------------------------------------------------------------------------------------------------------
    # fix things if needed

    if host.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS and LADISH_APP_NAME:
        print("LADISH detected but using multiple clients (not allowed), forcing single client now")
        host.processMode = ENGINE_PROCESS_MODE_SINGLE_CLIENT

# ------------------------------------------------------------------------------------------------------------
# Set host settings

def setHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostMeta()

    host.set_engine_option(ENGINE_OPTION_FORCE_STEREO,          host.forceStereo,         "")
    host.set_engine_option(ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, host.preferPluginBridges, "")
    host.set_engine_option(ENGINE_OPTION_PREFER_UI_BRIDGES,     host.preferUIBridges,     "")
    host.set_engine_option(ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     host.uisAlwaysOnTop,      "")
    host.set_engine_option(ENGINE_OPTION_MAX_PARAMETERS,        host.maxParameters,       "")
    host.set_engine_option(ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    host.uiBridgesTimeout,    "")

    if host.isPlugin:
        return

    host.set_engine_option(ENGINE_OPTION_PROCESS_MODE,          host.processMode,         "")
    host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,        host.transportMode,       "")

# ------------------------------------------------------------------------------------------------------------
