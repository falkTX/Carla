#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla host code
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

from PyQt4.QtCore import qCritical, QModelIndex, QTimer
from PyQt4.QtGui import QApplication, QFileSystemModel, QMainWindow, QPalette

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_host

from carla_database import *
from carla_settings import *
from carla_style import *
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

class CarlaDummyW(object):
    def __init__(self, parent):
        object.__init__(self)

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return 0

    # -----------------------------------------------------------------

    def addPlugin(self, pluginId, isProjectLoading):
        pass

    def removePlugin(self, pluginId):
        pass

    def renamePlugin(self, pluginId, newName):
        pass

    def disablePlugin(self, pluginId, errorMsg):
        pass

    def removeAllPlugins(self):
        pass

    # -----------------------------------------------------------------

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass

    def engineChanged(self):
        pass

    # -----------------------------------------------------------------

    def idleFast(self):
        pass

    def idleSlow(self):
        pass

    # -----------------------------------------------------------------

    def saveSettings(self, settings):
        pass

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
    # signals
    DebugCallback = pyqtSignal(int, int, int, float, str)
    PluginAddedCallback = pyqtSignal(int, str)
    PluginRemovedCallback = pyqtSignal(int)
    PluginRenamedCallback = pyqtSignal(int, str)
    PluginUnavailableCallback = pyqtSignal(int, str)
    ParameterValueChangedCallback = pyqtSignal(int, int, float)
    ParameterDefaultChangedCallback = pyqtSignal(int, int, float)
    ParameterMidiCcChangedCallback = pyqtSignal(int, int, int)
    ParameterMidiChannelChangedCallback = pyqtSignal(int, int, int)
    ProgramChangedCallback = pyqtSignal(int, int)
    MidiProgramChangedCallback = pyqtSignal(int, int)
    UiStateChangedCallback = pyqtSignal(int, int)
    NoteOnCallback = pyqtSignal(int, int, int, int)
    NoteOffCallback = pyqtSignal(int, int, int)
    UpdateCallback = pyqtSignal(int)
    ReloadInfoCallback = pyqtSignal(int)
    ReloadParametersCallback = pyqtSignal(int)
    ReloadProgramsCallback = pyqtSignal(int)
    ReloadAllCallback = pyqtSignal(int)
    PatchbayClientAddedCallback = pyqtSignal(int, int, str)
    PatchbayClientRemovedCallback = pyqtSignal(int)
    PatchbayClientRenamedCallback = pyqtSignal(int, str)
    PatchbayClientDataChangedCallback = pyqtSignal(int, int, int)
    PatchbayPortAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayPortRemovedCallback = pyqtSignal(int, int)
    PatchbayPortRenamedCallback = pyqtSignal(int, int, str)
    PatchbayConnectionAddedCallback = pyqtSignal(int, int, int)
    PatchbayConnectionRemovedCallback = pyqtSignal(int, int, int)
    EngineStartedCallback = pyqtSignal(int, int, str)
    EngineStoppedCallback = pyqtSignal()
    ProcessModeChangedCallback = pyqtSignal(int)
    TransportModeChangedCallback = pyqtSignal(int)
    BufferSizeChangedCallback = pyqtSignal(int)
    SampleRateChangedCallback = pyqtSignal(float)
    InfoCallback = pyqtSignal(str)
    ErrorCallback = pyqtSignal(str)
    QuitCallback = pyqtSignal()
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    def __init__(self, parent):
        QMainWindow.__init__(self, parent)
        self.ui = ui_carla_host.Ui_CarlaHostW()
        self.ui.setupUi(self)

        if False:
            Carla.gui = self
            self.fContainer = CarlaDummyW(self)

        # -------------------------------------------------------------
        # Set callback, TODO put somewhere else

        if Carla.host is not None:
            Carla.host.set_engine_callback(engineCallback)

        # -------------------------------------------------------------
        # Internal stuff

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fIsProjectLoading = False
        self.fProjectFilename  = ""

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fLastTransportFrame = 0
        self.fLastTransportState = False
        self.fTransportText = ""

        # when true, call engineChanged() asap
        self.fEngineChanged = False

        # first attempt of auto-start engine doesn't show an error
        self.fFirstEngineInit = True

        self.fSavedSettings = {}

        if LADISH_APP_NAME:
            self.fClientName         = LADISH_APP_NAME
            self.fSessionManagerName = "LADISH"
        elif NSM_URL:
            self.fClientName         = "Carla.tmp"
            self.fSessionManagerName = "Non Session Manager"
        else:
            self.fClientName         = "Carla"
            self.fSessionManagerName = ""

        # -------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)

        # -------------------------------------------------------------
        # Set up GUI (engine stopped)

        if Carla.isPlugin:
            self.ui.act_file_new.setEnabled(False)
            self.ui.act_file_open.setEnabled(False)
            self.ui.act_engine_start.setEnabled(False)
            self.ui.menu_Engine.setEnabled(False)
        else:
            self.ui.act_engine_start.setEnabled(True)

        self.ui.act_file_save.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        self.ui.menu_PluginMacros.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)

        self.setTransportMenuEnabled(False)

        # -------------------------------------------------------------
        # Set up GUI (right panel)

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setRootPath(HOME)

        if Carla.host is not None:
            self.fDirModel.setNameFilters(Carla.host.get_supported_file_extensions().split(";"))

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        self.setProperWindowTitle()

        # -------------------------------------------------------------
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
        self.ui.act_help_about_qt.triggered.connect(self.slot_aboutQt)

        self.ui.cb_disk.currentIndexChanged.connect(self.slot_diskFolderChanged)
        self.ui.b_disk_add.clicked.connect(self.slot_diskFolderAdd)
        self.ui.b_disk_remove.clicked.connect(self.slot_diskFolderRemove)
        self.ui.fileTreeView.doubleClicked.connect(self.slot_fileTreeDoubleClicked)

        self.DebugCallback.connect(self.slot_handleDebugCallback)

        self.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        self.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        self.PluginRenamedCallback.connect(self.slot_handlePluginRenamedCallback)
        self.PluginUnavailableCallback.connect(self.slot_handlePluginUnavailableCallback)

        # parameter (rack, patchbay)
        # program, midi-program, ui-state (rack, patchbay)
        # note on, off (rack, patchbay)
        # update, reload (rack, patchbay)
        # patchbay

        self.EngineStartedCallback.connect(self.slot_handleEngineStartedCallback)
        self.EngineStoppedCallback.connect(self.slot_handleEngineStoppedCallback)

        self.ProcessModeChangedCallback.connect(self.slot_handleProcessModeChangedCallback)
        self.TransportModeChangedCallback.connect(self.slot_handleTransportModeChangedCallback)

        self.BufferSizeChangedCallback.connect(self.slot_handleBufferSizeChangedCallback)
        self.SampleRateChangedCallback.connect(self.slot_handleSampleRateChangedCallback)

        self.InfoCallback.connect(self.slot_handleInfoCallback)
        self.ErrorCallback.connect(self.slot_handleErrorCallback)
        self.QuitCallback.connect(self.slot_handleQuitCallback)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

        # -------------------------------------------------------------
        # Final setup

        QTimer.singleShot(0, self.slot_engineStart)

    # -----------------------------------------------------------------
    # Called by containers

    def openSettingsWindow(self, hasCanvas, hasCanvasGL):
        dialog = CarlaSettingsW(self, hasCanvas, hasCanvasGL)
        return dialog.exec_()

    def setupContainer(self, showMiniCanvas, canvasThemeData = []):
        if showMiniCanvas:
            canvasWidth, canvasHeight, canvasBg, canvasBrush, canvasPen = canvasThemeData
            self.ui.miniCanvasPreview.setViewTheme(canvasBg, canvasBrush, canvasPen)
            self.ui.miniCanvasPreview.init(self.fContainer.scene, canvasWidth, canvasHeight, self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING])
        else:
            self.ui.miniCanvasPreview.hide()

        self.ui.splitter.insertWidget(1, self.fContainer)

    def updateContainer(self, canvasThemeData):
        canvasWidth, canvasHeight, canvasBg, canvasBrush, canvasPen = canvasThemeData
        self.ui.miniCanvasPreview.setViewTheme(canvasBg, canvasBrush, canvasPen)
        self.ui.miniCanvasPreview.init(self.fContainer.scene, canvasWidth, canvasHeight, self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING])

    # -----------------------------------------------------------------
    # Internal stuff (files)

    def loadProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: loading project without filename set")

        self.fIsProjectLoading = True
        Carla.host.load_project(self.fProjectFilename)
        self.fIsProjectLoading = False

    @pyqtSlot()
    def slot_loadProjectNow(self):
        self.loadProjectNow()

    def loadProjectLater(self, filename):
        self.fProjectFilename = filename
        self.setProperWindowTitle()
        QTimer.singleShot(0, self.slot_loadProjectNow)

    def saveProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: saving project without filename set")

        Carla.host.save_project(self.fProjectFilename)

    # -----------------------------------------------------------------
    # Internal stuff (engine)

    def setEngineSettings(self, settings = None):
        if Carla.isPlugin:
            return "Plugin"

        if settings is None: settings = QSettings()

        # -------------------------------------------------------------
        # read settings

        # bool values
        try:
            forceStereo = settings.value(CARLA_KEY_ENGINE_FORCE_STEREO, CARLA_DEFAULT_FORCE_STEREO, type=bool)
        except:
            forceStereo = CARLA_DEFAULT_FORCE_STEREO

        try:
            preferPluginBridges = settings.value(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool)
        except:
            preferPluginBridges = CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES

        try:
            preferUiBridges = settings.value(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES, CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool)
        except:
            preferUiBridges = CARLA_DEFAULT_PREFER_UI_BRIDGES

        try:
            uisAlwaysOnTop = settings.value(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP, CARLA_DEFAULT_UIS_ALWAYS_ON_TOP, type=bool)
        except:
            uisAlwaysOnTop = CARLA_DEFAULT_UIS_ALWAYS_ON_TOP

        # int values
        try:
            maxParameters = settings.value(CARLA_KEY_ENGINE_MAX_PARAMETERS, CARLA_DEFAULT_MAX_PARAMETERS, type=int)
        except:
            maxParameters = CARLA_DEFAULT_MAX_PARAMETERS

        try:
            uiBridgesTimeout = settings.value(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT, CARLA_DEFAULT_UI_BRIDGES_TIMEOUT, type=int)
        except:
            uiBridgesTimeout = CARLA_DEFAULT_UI_BRIDGES_TIMEOUT

        # enums
        try:
            processMode = settings.value(CARLA_KEY_ENGINE_PROCESS_MODE, CARLA_DEFAULT_PROCESS_MODE, type=int)
        except:
            processMode = CARLA_DEFAULT_PROCESS_MODE

        try:
            transportMode = settings.value(CARLA_KEY_ENGINE_TRANSPORT_MODE, CARLA_DEFAULT_TRANSPORT_MODE, type=int)
        except:
            transportMode = CARLA_DEFAULT_TRANSPORT_MODE

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

        # -------------------------------------------------------------
        # fix things if needed

        if processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS and LADISH_APP_NAME:
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            processMode = ENGINE_PROCESS_MODE_SINGLE_CLIENT

        if audioDriver != "JACK" and transportMode == ENGINE_TRANSPORT_MODE_JACK:
            transportMode = ENGINE_TRANSPORT_MODE_INTERNAL

        # -------------------------------------------------------------
        # apply to engine

        Carla.host.set_engine_option(ENGINE_OPTION_FORCE_STEREO,          forceStereo,         "")
        Carla.host.set_engine_option(ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")
        Carla.host.set_engine_option(ENGINE_OPTION_PREFER_UI_BRIDGES,     preferUiBridges,     "")
        Carla.host.set_engine_option(ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     uisAlwaysOnTop,      "")

        Carla.host.set_engine_option(ENGINE_OPTION_MAX_PARAMETERS,        maxParameters,       "")
        Carla.host.set_engine_option(ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    uiBridgesTimeout,    "")

        Carla.host.set_engine_option(ENGINE_OPTION_PROCESS_MODE,          processMode,         "")
        Carla.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,        transportMode,       "")

        Carla.host.set_engine_option(ENGINE_OPTION_AUDIO_NUM_PERIODS,     audioNumPeriods,     "")
        Carla.host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE,     audioBufferSize,     "")
        Carla.host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE,     audioSampleRate,     "")
        Carla.host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE,          0,                   audioDevice)

        # save this for later
        Carla.maxParameters = maxParameters

        # return selected driver name
        return audioDriver

    def startEngine(self):
        audioDriver = self.setEngineSettings()

        if not Carla.host.engine_init(audioDriver, self.fClientName):
            if self.fFirstEngineInit:
                self.fFirstEngineInit = False
                return

            audioError = Carla.host.get_last_error()

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fFirstEngineInit = False

    def stopEngine(self):
        if self.fContainer.getPluginCount() > 0:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask != QMessageBox.Yes:
                return

            self.ui.act_plugin_remove_all.setEnabled(False)
            self.fContainer.removeAllPlugins()

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(Carla.host.get_last_error())

    # -----------------------------------------------------------------
    # Internal stuff (plugins)

    def getExtraPtr(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']

            self.maybeLoadRDFs()

            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype in (PLUGIN_FILE_GIG, PLUGIN_FILE_SF2):
            if plugin['name'].lower().endswith(" (16 outputs)"):
                return c_char_p("true".encode("utf-8"))

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

    def setLoadRDFsNeeded(self):
        self.fLadspaRdfNeedsUpdate = True

    # -----------------------------------------------------------------
    # Internal stuff (transport)

    def refreshTransport(self, forced = False):
        if Carla.sampleRate == 0.0 or not Carla.host.is_engine_running():
            return

        timeInfo = Carla.host.get_transport_info()
        playing  = bool(timeInfo['playing'])
        frame    = int(timeInfo['frame'])

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
            time = frame / Carla.sampleRate
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

    # -----------------------------------------------------------------
    # Internal stuff (settings)

    def loadSettings(self, firstTime):
        settings = QSettings()

        if firstTime:
            self.restoreGeometry(settings.value("Geometry", ""))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

            if settings.contains("SplitterState"):
                self.ui.splitter.restoreState(settings.value("SplitterState", ""))
            else:
                self.ui.splitter.setSizes([210, 99999])

            diskFolders = toList(settings.value("DiskFolders", [HOME]))

            self.ui.cb_disk.setItemData(0, HOME)

            for i in range(len(diskFolders)):
                if i == 0: continue
                folder = diskFolders[i]
                self.ui.cb_disk.addItem(os.path.basename(folder), folder)

            if MACOS and not settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, True, type=bool):
                self.setUnifiedTitleAndToolBarOnMac(True)

        # ---------------------------------------------
        # plugin checks

        if settings.value("Engine/DisableChecks", False, type=bool):
            os.environ["CARLA_DISCOVERY_NO_PROCESSING_CHECKS"] = "true"

        elif os.getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"):
            os.environ.pop("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")

        # ---------------------------------------------

        if not Carla.isPlugin:
            # engine
            self.setEngineSettings(settings)

            # plugin paths
            LADSPA_PATH = toList(settings.value("Paths/LADSPA", Carla.DEFAULT_LADSPA_PATH))
            DSSI_PATH   = toList(settings.value("Paths/DSSI",   Carla.DEFAULT_DSSI_PATH))
            LV2_PATH    = toList(settings.value("Paths/LV2",    Carla.DEFAULT_LV2_PATH))
            VST_PATH    = toList(settings.value("Paths/VST",    Carla.DEFAULT_VST_PATH))
            AU_PATH     = toList(settings.value("Paths/AU",     Carla.DEFAULT_AU_PATH))
            CSOUND_PATH = toList(settings.value("Paths/CSOUND", Carla.DEFAULT_CSOUND_PATH))
            GIG_PATH    = toList(settings.value("Paths/GIG",    Carla.DEFAULT_GIG_PATH))
            SF2_PATH    = toList(settings.value("Paths/SF2",    Carla.DEFAULT_SF2_PATH))
            SFZ_PATH    = toList(settings.value("Paths/SFZ",    Carla.DEFAULT_SFZ_PATH))

            os.environ["LADSPA_PATH"] = splitter.join(LADSPA_PATH)
            os.environ["DSSI_PATH"]   = splitter.join(DSSI_PATH)
            os.environ["LV2_PATH"]    = splitter.join(LV2_PATH)
            os.environ["VST_PATH"]    = splitter.join(VST_PATH)
            os.environ["AU_PATH"]     = splitter.join(AU_PATH)
            os.environ["CSOUND_PATH"] = splitter.join(CSOUND_PATH)
            os.environ["GIG_PATH"]    = splitter.join(GIG_PATH)
            os.environ["SF2_PATH"]    = splitter.join(SF2_PATH)
            os.environ["SFZ_PATH"]    = splitter.join(SFZ_PATH)

        # ---------------------------------------------

        # TODO

        self.fSavedSettings = {
            CARLA_KEY_MAIN_PROJECT_FOLDER:     settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER,     CARLA_DEFAULT_MAIN_PROJECT_FOLDER,     type=str),
            CARLA_KEY_MAIN_REFRESH_INTERVAL:   settings.value(CARLA_KEY_MAIN_REFRESH_INTERVAL,   CARLA_DEFAULT_MAIN_REFRESH_INTERVAL,   type=int),
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

        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*2)

    def saveSettings(self):
        settings = QSettings()

        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("SplitterState", self.ui.splitter.saveState())
        settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())

        diskFolders = []

        for i in range(self.ui.cb_disk.count()):
            diskFolders.append(self.ui.cb_disk.itemData(i))

        settings.setValue("DiskFolders", diskFolders)

        self.fContainer.saveSettings(settings)

    # -----------------------------------------------------------------
    # Internal stuff (gui)

    def setProperWindowTitle(self):
        title = self.fClientName

        if self.fProjectFilename:
            title += " - %s" % os.path.basename(self.fProjectFilename)
        if self.fSessionManagerName:
            title += " (%s)" % self.fSessionManagerName

        self.setWindowTitle(title)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_fileNew(self):
        self.fContainer.removeAllPlugins()
        self.fProjectFilename = ""
        self.setProperWindowTitle()

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if not filenameTry:
            return
        filename = filenameTry if isinstance(filenameTry, str) else filenameTry[0]

        newFile = True

        if self.fContainer.getPluginCount() > 0:
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

        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if not filenameTry:
            return
        filename = filenameTry if isinstance(filenameTry, str) else filenameTry[0]

        if not filename.endswith(".carxp"):
            filename += ".carxp"

        if self.fProjectFilename != filename:
            self.fProjectFilename = filename
            self.setProperWindowTitle()

        self.saveProjectNow()

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_engineStart(self, doStart = True):
        if doStart: self.startEngine()

        check = Carla.host.is_engine_running()
        self.ui.menu_PluginMacros.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if not Carla.isPlugin:
            self.ui.act_file_save.setEnabled(check)
            self.ui.act_engine_start.setEnabled(not check)
            self.ui.act_engine_stop.setEnabled(check)

            if self.fSessionManagerName != "Non Session Manager":
                self.ui.act_file_open.setEnabled(check)
                self.ui.act_file_save_as.setEnabled(check)

            self.setTransportMenuEnabled(check)

        if check:
            if not Carla.isPlugin:
                self.refreshTransport(True)

            self.fContainer.engineStarted()

    @pyqtSlot()
    def slot_engineStop(self, doStop = True):
        if doStop: self.stopEngine()

        # FIXME?
        if self.fContainer.getPluginCount() > 0:
            self.ui.act_plugin_remove_all.setEnabled(False)
            self.fContainer.removeAllPlugins()

        check = Carla.host.is_engine_running()
        self.ui.menu_PluginMacros.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if not Carla.isPlugin:
            self.ui.act_file_save.setEnabled(check)
            self.ui.act_engine_start.setEnabled(not check)
            self.ui.act_engine_stop.setEnabled(check)

            if self.fSessionManagerName != "Non Session Manager":
                self.ui.act_file_open.setEnabled(check)
                self.ui.act_file_save_as.setEnabled(check)

            self.setTransportMenuEnabled(check)

        if not check:
            self.fTextTransport = ""
            self.fContainer.engineStopped()

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginAdd(self):
        dialog = PluginDatabaseW(self)

        if not dialog.exec_():
            return

        if not Carla.host.is_engine_running():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return

        btype    = dialog.fRetPlugin['build']
        ptype    = dialog.fRetPlugin['type']
        filename = dialog.fRetPlugin['filename']
        label    = dialog.fRetPlugin['label']
        extraPtr = self.getExtraPtr(dialog.fRetPlugin)

        if not Carla.host.add_plugin(btype, ptype, filename, None, label, extraPtr):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), charPtrToString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
            return

    @pyqtSlot()
    def slot_pluginRemoveAll(self):
        self.ui.act_plugin_remove_all.setEnabled(False)
        self.fContainer.removeAllPlugins()
        Carla.host.remove_all_plugins()

    # -----------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if not Carla.host.is_engine_running():
            return

        if toggled:
            Carla.host.transport_play()
        else:
            Carla.host.transport_pause()

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if not Carla.host.is_engine_running():
            return

        Carla.host.transport_pause()
        Carla.host.transport_relocate(0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if not Carla.host.is_engine_running():
            return

        newFrame = Carla.host.get_current_transport_frame() - 100000

        if newFrame < 0:
            newFrame = 0

        Carla.host.transport_relocate(newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if not Carla.host.is_engine_running():
            return

        newFrame = Carla.host.get_current_transport_frame() + 100000
        Carla.host.transport_relocate(newFrame)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

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

        if not Carla.host.load_file(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             Carla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG:", pluginId, value1, value2, value3, valueStr)
        #self.ui.pte_log.appendPlainText(valueStr.replace("[30;1m", "DEBUG: ").replace("[31m", "ERROR: ").replace("[0m", "").replace("\n", ""))

    # -----------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName):
        self.fContainer.addPlugin(pluginId, self.fIsProjectLoading)

        if self.fContainer.getPluginCount() == 1:
            self.ui.act_plugin_remove_all.setEnabled(True)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        self.fContainer.removePlugin(pluginId)

        if self.fContainer.getPluginCount() == 0:
            self.ui.act_plugin_remove_all.setEnabled(False)

    @pyqtSlot(int, str)
    def slot_handlePluginRenamedCallback(self, pluginId, newName):
        self.fContainer.renamePlugin(pluginId, newName)

    @pyqtSlot(int, str)
    def slot_handlePluginUnavailableCallback(self, pluginId, errorMsg):
        self.fContainer.disablePlugin(pluginId, errorMsg)

    # -----------------------------------------------------------------

    @pyqtSlot(str)
    def slot_handleEngineStartedCallback(self, processMode, transportMode, driverName):
        Carla.processMode   = processMode
        Carla.transportMode = transportMode
        Carla.bufferSize    = Carla.host.get_buffer_size()
        Carla.sampleRate    = Carla.host.get_sample_rate()

        self.slot_engineStart(False)

        if self.fIdleTimerFast == 0:
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])
        if self.fIdleTimerSlow == 0:
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*2)

    @pyqtSlot()
    def slot_handleEngineStoppedCallback(self):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

        self.slot_engineStop(False)

        Carla.bufferSize = 0
        Carla.sampleRate = 0.0

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleProcessModeChangedCallback(self, newProcessMode):
        self.fEngineChanged = True

    @pyqtSlot(int)
    def slot_handleTransportModeChangedCallback(self, newTransportMode):
        self.fEngineChanged = True

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleBufferSizeChangedCallback(self, newBufferSize):
        self.fEngineChanged = True

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fEngineChanged = True

    # -----------------------------------------------------------------

    @pyqtSlot(str)
    def slot_handleInfoCallback(self, info):
        pass

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        pass

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        pass

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self.slot_fileSave)

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    # -----------------------------------------------------------------

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            #if not Carla.isPlugin:
            Carla.host.engine_idle()
            self.refreshTransport()

            self.fContainer.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            if self.fEngineChanged:
                self.fContainer.engineChanged()
                self.fEngineChanged = False

            self.fContainer.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

        self.saveSettings()

        if Carla.host.is_engine_running():
            Carla.host.set_engine_about_to_close()
            self.ui.act_plugin_remove_all.setEnabled(False)
            self.fContainer.removeAllPlugins()
            self.stopEngine()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def engineCallback(ptr, action, pluginId, value1, value2, value3, valueStr):
    if action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        Carla.processMode = value1
        if Carla.gui is not None:
            Carla.gui.ProcessModeChangedCallback.emit(value1)
        return

    if action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        Carla.transportMode = value1
        if Carla.gui is not None:
            Carla.gui.TransportModeChangedCallback.emit(value1)
        return

    if action == ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
        Carla.bufferSize = value1
        if Carla.gui is not None:
            Carla.gui.BufferSizeChangedCallback.emit(value1)
        return

    if action == ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
        Carla.sampleRate = value1
        if Carla.gui is not None:
            Carla.gui.SampleRateChangedCallback.emit(value3)
        return

    if Carla.gui is None:
        print("WARNING: Got engine callback but UI is not ready : ", pluginId, value1, value2, value3, valueStr)
        return

    valueStr = charPtrToString(valueStr)

    if action == ENGINE_CALLBACK_DEBUG:
        Carla.gui.DebugCallback.emit(pluginId, value1, value2, value3, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_ADDED:
        Carla.gui.PluginAddedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_REMOVED:
        Carla.gui.PluginRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PLUGIN_RENAMED:
        Carla.gui.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
        Carla.gui.PluginUnavailableCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
        Carla.gui.ParameterValueChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
        Carla.gui.ParameterDefaultChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        Carla.gui.ParameterMidiCcChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        Carla.gui.ParameterMidiChannelChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PROGRAM_CHANGED:
        Carla.gui.ProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
        Carla.gui.MidiProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_UI_STATE_CHANGED:
        Carla.gui.UiStateChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_NOTE_ON:
        Carla.gui.NoteOnCallback.emit(pluginId, value1, value2, int(value3))
    elif action == ENGINE_CALLBACK_NOTE_OFF:
        Carla.gui.NoteOffCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_UPDATE:
        Carla.gui.UpdateCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_INFO:
        Carla.gui.ReloadInfoCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PARAMETERS:
        Carla.gui.ReloadParametersCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PROGRAMS:
        Carla.gui.ReloadProgramsCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_ALL:
        Carla.gui.ReloadAllCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
        Carla.gui.PatchbayClientAddedCallback.emit(pluginId, value1, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
        Carla.gui.PatchbayClientRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
        Carla.gui.PatchbayClientRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
        Carla.gui.PatchbayClientDataChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
        Carla.gui.PatchbayPortAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
        Carla.gui.PatchbayPortRemovedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED:
        Carla.gui.PatchbayPortRenamedCallback.emit(pluginId, value1, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
        Carla.gui.PatchbayConnectionAddedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        Carla.gui.PatchbayConnectionRemovedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_ENGINE_STARTED:
        Carla.gui.EngineStartedCallback.emit(value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_ENGINE_STOPPED:
        Carla.gui.EngineStoppedCallback.emit()
    elif action == ENGINE_CALLBACK_INFO:
        Carla.gui.InfoCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_ERROR:
        Carla.gui.ErrorCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_QUIT:
        Carla.gui.QuitCallback.emit()
