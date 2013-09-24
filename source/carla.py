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
from PyQt5.QtCore import Qt, QModelIndex, QPointF, QSize
from PyQt5.QtGui import QImage, QPalette, QResizeEvent, QSyntaxHighlighter
from PyQt5.QtPrintSupport import QPrinter, QPrintDialog
from PyQt5.QtWidgets import QApplication, QDialogButtonBox, QFileSystemModel, QLabel, QMainWindow

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_carla

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Static Variables

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

LADISH_APP_NAME = os.getenv("LADISH_APP_NAME")
NSM_URL = os.getenv("NSM_URL")

# ------------------------------------------------------------------------------------------------------------
# Global Variables

appName = os.path.basename(__file__) if "__file__" in dir() and os.path.dirname(__file__) in PATH else sys.argv[0]
libPrefix = None
projectFilename = None

# ------------------------------------------------------------------------------------------------------------
# Log Syntax Highlighter

class LogSyntaxHighlighter(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        palette = parent.palette()

        self.fColorDebug = palette.color(QPalette.Disabled, QPalette.WindowText)
        self.fColorError = Qt.red

        # -------------------------------------------------------------

    def highlightBlock(self, text):
        if text.startswith("DEBUG:"):
            self.setFormat(0, len(text), self.fColorDebug)
        elif text.startswith("ERROR:"):
            self.setFormat(0, len(text), self.fColorError)

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CarlaMainW(QMainWindow):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.ui =  ui_carla.Ui_CarlaMainW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fBufferSize = 0
        self.fSampleRate = 0.0

        self.fEngineStarted   = False
        self.fFirstEngineInit = True

        self.fProjectFilename = None
        self.fProjectLoading  = False

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLastTransportFrame = 0
        self.fLastTransportState = False

        self.fClientName = "Carla"
        self.fSessionManagerName = "LADISH" if LADISH_APP_NAME else ""

        # -------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.fInfoLabel = QLabel(self)
        self.fInfoLabel.setText("")
        self.fInfoText = ""

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setNameFilters(cString(Carla.host.get_supported_file_types()).split(";"))
        self.fDirModel.setRootPath(HOME)

        if not WINDOWS:
            self.fSyntaxLog = LogSyntaxHighlighter(self.ui.pte_log)
            self.fSyntaxLog.setDocument(self.ui.pte_log.document())

            if LADISH_APP_NAME:
                self.ui.miniCanvasPreview.setVisible(False)
                self.ui.tabMain.removeTab(1)
        else:
            self.ui.tabMain.removeTab(2)

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        self.ui.act_engine_start.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        # FIXME: Qt4 needs this so it properly creates & resizes the canvas
        self.ui.tabMain.setCurrentIndex(1)
        self.ui.tabMain.setCurrentIndex(0)

        # -------------------------------------------------------------
        # Set-up Canvas Preview

        self.ui.miniCanvasPreview.setRealParent(self)
        self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.ui.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT, self.fSavedSettings["UseCustomMiniCanvasPaint"])
        QTimer.singleShot(100, self, SLOT("slot_miniCanvasInit()"))

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.ui.act_file_new, SIGNAL("triggered()"), SLOT("slot_fileNew()"))
        self.connect(self.ui.act_file_open, SIGNAL("triggered()"), SLOT("slot_fileOpen()"))
        self.connect(self.ui.act_file_save, SIGNAL("triggered()"), SLOT("slot_fileSave()"))
        self.connect(self.ui.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_fileSaveAs()"))
        #self.connect(self.ui.act_file_export_lv2, SIGNAL("triggered()"), SLOT("slot_fileExportLv2Preset()"))

        self.connect(self.ui.act_engine_start, SIGNAL("triggered()"), SLOT("slot_engineStart()"))
        self.connect(self.ui.act_engine_stop, SIGNAL("triggered()"), SLOT("slot_engineStop()"))

        self.connect(self.ui.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        self.connect(self.ui.act_plugin_add2, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        #self.connect(self.ui.act_plugin_refresh, SIGNAL("triggered()"), SLOT("slot_pluginRefresh()"))
        self.connect(self.ui.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_pluginRemoveAll()"))

        self.connect(self.ui.act_plugins_enable, SIGNAL("triggered()"), SLOT("slot_pluginsEnable()"))
        self.connect(self.ui.act_plugins_disable, SIGNAL("triggered()"), SLOT("slot_pluginsDisable()"))
        self.connect(self.ui.act_plugins_panic, SIGNAL("triggered()"), SLOT("slot_pluginsDisable()"))
        self.connect(self.ui.act_plugins_volume100, SIGNAL("triggered()"), SLOT("slot_pluginsVolume100()"))
        self.connect(self.ui.act_plugins_mute, SIGNAL("triggered()"), SLOT("slot_pluginsMute()"))
        self.connect(self.ui.act_plugins_wet100, SIGNAL("triggered()"), SLOT("slot_pluginsWet100()"))
        self.connect(self.ui.act_plugins_bypass, SIGNAL("triggered()"), SLOT("slot_pluginsBypass()"))
        self.connect(self.ui.act_plugins_center, SIGNAL("triggered()"), SLOT("slot_pluginsCenter()"))

        self.connect(self.ui.act_transport_play, SIGNAL("triggered(bool)"), SLOT("slot_transportPlayPause(bool)"))
        self.connect(self.ui.act_transport_stop, SIGNAL("triggered()"), SLOT("slot_transportStop()"))
        self.connect(self.ui.act_transport_backwards, SIGNAL("triggered()"), SLOT("slot_transportBackwards()"))
        self.connect(self.ui.act_transport_forwards, SIGNAL("triggered()"), SLOT("slot_transportForwards()"))

        self.ui.act_canvas_arrange.setEnabled(False) # TODO, later
        self.connect(self.ui.act_canvas_arrange, SIGNAL("triggered()"), SLOT("slot_canvasArrange()"))
        self.connect(self.ui.act_canvas_refresh, SIGNAL("triggered()"), SLOT("slot_canvasRefresh()"))
        self.connect(self.ui.act_canvas_zoom_fit, SIGNAL("triggered()"), SLOT("slot_canvasZoomFit()"))
        self.connect(self.ui.act_canvas_zoom_in, SIGNAL("triggered()"), SLOT("slot_canvasZoomIn()"))
        self.connect(self.ui.act_canvas_zoom_out, SIGNAL("triggered()"), SLOT("slot_canvasZoomOut()"))
        self.connect(self.ui.act_canvas_zoom_100, SIGNAL("triggered()"), SLOT("slot_canvasZoomReset()"))
        self.connect(self.ui.act_canvas_print, SIGNAL("triggered()"), SLOT("slot_canvasPrint()"))
        self.connect(self.ui.act_canvas_save_image, SIGNAL("triggered()"), SLOT("slot_canvasSaveImage()"))

        self.connect(self.ui.act_settings_show_toolbar, SIGNAL("triggered(bool)"), SLOT("slot_toolbarShown()"))
        self.connect(self.ui.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))

        self.connect(self.ui.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.ui.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self.ui.splitter, SIGNAL("splitterMoved(int, int)"), SLOT("slot_splitterMoved()"))

        self.connect(self.ui.cb_disk, SIGNAL("currentIndexChanged(int)"), SLOT("slot_diskFolderChanged(int)"))
        self.connect(self.ui.b_disk_add, SIGNAL("clicked()"), SLOT("slot_diskFolderAdd()"))
        self.connect(self.ui.b_disk_remove, SIGNAL("clicked()"), SLOT("slot_diskFolderRemove()"))
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
        self.connect(self, SIGNAL("PatchbayClientAddedCallback(int, int, QString)"), SLOT("slot_handlePatchbayClientAddedCallback(int, int, QString)"))
        self.connect(self, SIGNAL("PatchbayClientRemovedCallback(int, QString)"), SLOT("slot_handlePatchbayClientRemovedCallback(int, QString)"))
        self.connect(self, SIGNAL("PatchbayClientRenamedCallback(int, QString)"), SLOT("slot_handlePatchbayClientRenamedCallback(int, QString)"))
        self.connect(self, SIGNAL("PatchbayPortAddedCallback(int, int, int, QString)"), SLOT("slot_handlePatchbayPortAddedCallback(int, int, int, QString)"))
        self.connect(self, SIGNAL("PatchbayPortRemovedCallback(int, int, QString)"), SLOT("slot_handlePatchbayPortRemovedCallback(int, int, QString)"))
        self.connect(self, SIGNAL("PatchbayPortRenamedCallback(int, int, QString)"), SLOT("slot_handlePatchbayPortRenamedCallback(int, int, QString)"))
        self.connect(self, SIGNAL("PatchbayConnectionAddedCallback(int, int, int)"), SLOT("slot_handlePatchbayConnectionAddedCallback(int, int, int)"))
        self.connect(self, SIGNAL("PatchbayConnectionRemovedCallback(int)"), SLOT("slot_handlePatchbayConnectionRemovedCallback(int)"))
        self.connect(self, SIGNAL("PatchbayIconChangedCallback(int, int)"), SLOT("slot_handlePatchbayIconChangedCallback(int, int)"))
        self.connect(self, SIGNAL("BufferSizeChangedCallback(int)"), SLOT("slot_handleBufferSizeChangedCallback(int)"))
        self.connect(self, SIGNAL("SampleRateChangedCallback(double)"), SLOT("slot_handleSampleRateChangedCallback(double)"))
        self.connect(self, SIGNAL("NSM_AnnounceCallback(QString)"), SLOT("slot_handleNSM_AnnounceCallback(QString)"))
        self.connect(self, SIGNAL("NSM_OpenCallback(QString)"), SLOT("slot_handleNSM_OpenCallback(QString)"))
        self.connect(self, SIGNAL("NSM_SaveCallback()"), SLOT("slot_handleNSM_SaveCallback()"))
        self.connect(self, SIGNAL("ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        self.setProperWindowTitle()

        if NSM_URL:
            Carla.host.nsm_ready()
        else:
            QTimer.singleShot(0, self, SLOT("slot_engineStart()"))

    def startEngine(self):
        # ---------------------------------------------
        # Engine settings

        settings = QSettings()

        audioDriver         = settings.value("Engine/AudioDriver", CARLA_DEFAULT_AUDIO_DRIVER, type=str)
        transportMode       = settings.value("Engine/TransportMode", TRANSPORT_MODE_JACK, type=int)
        forceStereo         = settings.value("Engine/ForceStereo", CARLA_DEFAULT_FORCE_STEREO, type=bool)
        preferPluginBridges = settings.value("Engine/PreferPluginBridges", CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool)
        preferUiBridges     = settings.value("Engine/PreferUiBridges", CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool)
        useDssiVstChunks    = settings.value("Engine/UseDssiVstChunks", CARLA_DEFAULT_USE_DSSI_VST_CHUNKS, type=bool)
        #oscUiTimeout        = settings.value("Engine/OscUiTimeout", CARLA_DEFAULT_OSC_UI_TIMEOUT, type=int)

        Carla.processMode   = settings.value("Engine/ProcessMode", CARLA_DEFAULT_PROCESS_MODE, type=int)
        Carla.maxParameters = settings.value("Engine/MaxParameters", CARLA_DEFAULT_MAX_PARAMETERS, type=int)

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif Carla.processMode == PROCESS_MODE_MULTIPLE_CLIENTS and LADISH_APP_NAME:
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            Carla.processMode = PROCESS_MODE_SINGLE_CLIENT

        Carla.host.set_engine_option(OPTION_PROCESS_MODE, Carla.processMode, "")
        Carla.host.set_engine_option(OPTION_MAX_PARAMETERS, Carla.maxParameters, "")
        Carla.host.set_engine_option(OPTION_FORCE_STEREO, forceStereo, "")
        Carla.host.set_engine_option(OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")
        Carla.host.set_engine_option(OPTION_PREFER_UI_BRIDGES, preferUiBridges, "")
        Carla.host.set_engine_option(OPTION_USE_DSSI_VST_CHUNKS, useDssiVstChunks, "")
        #Carla.host.set_engine_option(OPTION_OSC_UI_TIMEOUT, oscUiTimeout, "")

        if audioDriver == "JACK":
            pass
            #jackAutoConnect = settings.value("Engine/JackAutoConnect", CARLA_DEFAULT_JACK_AUTOCONNECT, type=bool)
            #jackTimeMaster  = settings.value("Engine/JackTimeMaster", CARLA_DEFAULT_JACK_TIMEMASTER, type=bool)

            #if jackAutoConnect and LADISH_APP_NAME:
                #print("LADISH detected but using JACK auto-connect (not desired), disabling auto-connect now")
                #jackAutoConnect = False

            #Carla.host.set_engine_option(OPTION_JACK_AUTOCONNECT, jackAutoConnect, "")
            #Carla.host.set_engine_option(OPTION_JACK_TIMEMASTER, jackTimeMaster, "")

        else:
            rtaudioBufferSize = settings.value("Engine/RtAudioBufferSize", CARLA_DEFAULT_RTAUDIO_BUFFER_SIZE, type=int)
            rtaudioSampleRate = settings.value("Engine/RtAudioSampleRate", CARLA_DEFAULT_RTAUDIO_SAMPLE_RATE, type=int)
            rtaudioDevice     = settings.value("Engine/RtAudioDevice", "", type=str)

            Carla.host.set_engine_option(OPTION_RTAUDIO_BUFFER_SIZE, rtaudioBufferSize, "")
            Carla.host.set_engine_option(OPTION_RTAUDIO_SAMPLE_RATE, rtaudioSampleRate, "")
            Carla.host.set_engine_option(OPTION_RTAUDIO_DEVICE, 0, rtaudioDevice)

        # ---------------------------------------------
        # Start

        if not Carla.host.engine_init(audioDriver, self.fClientName):
            if self.fFirstEngineInit:
                self.fFirstEngineInit = False
                return

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fBufferSize = Carla.host.get_buffer_size()
        self.fSampleRate = Carla.host.get_sample_rate()

        self.fEngineStarted   = True
        self.fFirstEngineInit = False

        self.fPluginCount = 0
        self.fPluginList  = []

        if transportMode == TRANSPORT_MODE_JACK and audioDriver != "JACK":
            transportMode = TRANSPORT_MODE_INTERNAL

        Carla.host.set_engine_option(OPTION_TRANSPORT_MODE, transportMode, "")

        # Peaks and TimeInfo
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

        self.fEngineStarted = False

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(cString(Carla.host.get_last_error()))

        self.fBufferSize = 0
        self.fSampleRate = 0.0

        self.fPluginCount = 0
        self.fPluginList  = []

        self.killTimer(self.fIdleTimerFast)
        self.killTimer(self.fIdleTimerSlow)

        patchcanvas.clear()

    def loadProject(self, filename):
        self.fProjectFilename = filename
        self.setProperWindowTitle()

        self.fProjectLoading = True
        Carla.host.load_project(filename)
        self.fProjectLoading = False

    def loadProjectLater(self, filename):
        self.fProjectFilename = filename
        self.setProperWindowTitle()
        QTimer.singleShot(0, self, SLOT("slot_loadProjectLater()"))

    def saveProject(self, filename):
        self.fProjectFilename = filename
        self.setProperWindowTitle()
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

            pwidget.ui.edit_dialog.close()
            pwidget.close()
            pwidget.deleteLater()
            del pwidget

        self.fPluginCount = 0
        self.fPluginList  = []

        Carla.host.remove_all_plugins()

    def menuTransport(self, enabled):
        self.ui.act_transport_play.setEnabled(enabled)
        self.ui.act_transport_stop.setEnabled(enabled)
        self.ui.act_transport_backwards.setEnabled(enabled)
        self.ui.act_transport_forwards.setEnabled(enabled)
        self.ui.menu_Transport.setEnabled(enabled)

    def refreshTransport(self, forced = False):
        if not self.fEngineStarted:
            return
        if self.fSampleRate == 0.0:
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
            time = frame / self.fSampleRate
            secs = time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60

            textTransport = "Transport %s, at %02i:%02i:%02i" % ("playing" if playing else "stopped", hrs, mins, secs)
            self.fInfoLabel.setText("%s | %s" % (self.fInfoText, textTransport))

            self.fLastTransportFrame = frame

    def setProperWindowTitle(self):
        title = LADISH_APP_NAME if LADISH_APP_NAME else "Carla"

        if self.fProjectFilename:
            title += " - %s" % os.path.basename(self.fProjectFilename)
        if self.fSessionManagerName:
            title += " (%s)" % self.fSessionManagerName

        self.setWindowTitle(title)

    def updateInfoLabelPos(self):
        tabBar = self.ui.tabMain.tabBar()
        y = tabBar.mapFromParent(self.ui.centralwidget.pos()).y()+tabBar.height()/4

        if not self.ui.toolBar.isVisible():
            y -= self.ui.toolBar.height()

        self.fInfoLabel.move(self.fInfoLabel.x(), y)

    def updateInfoLabelSize(self):
        tabBar = self.ui.tabMain.tabBar()
        self.fInfoLabel.resize(self.ui.tabMain.width()-tabBar.width()-20, self.fInfoLabel.height())

    @pyqtSlot()
    def slot_fileNew(self):
        self.removeAllPlugins()
        self.fProjectFilename = None
        self.fProjectLoading  = False
        self.setProperWindowTitle()

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            # FIXME - show dialog to user (remove all plugins?)
            self.removeAllPlugins()
            self.loadProject(filenameTry)

    @pyqtSlot()
    def slot_fileSave(self, saveAs=False):
        if self.fProjectFilename and not saveAs:
            return self.saveProject(self.fProjectFilename)

        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if not filenameTry:
            return

        if not filenameTry.endswith(".carxp"):
            filenameTry += ".carxp"

        self.saveProject(filenameTry)

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    @pyqtSlot()
    def slot_fileExportLv2Preset(self):
        fileFilter  = self.tr("LV2 Preset (*.lv2)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if not filenameTry:
            return

        if not filenameTry.endswith(".lv2"):
            filenameTry += ".lv2"

        if os.path.exists(filenameTry) and not os.path.isdir(filenameTry):
            # TODO - error
            return

        # Save current project to a tmp file, and read it
        tmpFile = os.path.join(TMP, "carla-plugin-export.carxp")

        if not Carla.host.save_project(tmpFile):
            # TODO - error
            return

        tmpFileFd = open(tmpFile, "r")
        presetContents = tmpFileFd.read()
        tmpFileFd.close()
        os.remove(tmpFile)

        # Create LV2 Preset
        os.mkdir(filenameTry)

        manifestPath = os.path.join(filenameTry, "manifest.ttl")

        manifestFd = open(manifestPath, "w")
        manifestFd.write("""# LV2 Preset for the Carla LV2 Plugin
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .
@prefix pset:  <http://lv2plug.in/ns/ext/presets#> .
@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
@prefix state: <http://lv2plug.in/ns/ext/state#> .

<file://%s>
    a pset:Preset ;
    lv2:appliesTo <http://kxstudio.sf.net/carla> ;
    rdfs:label "%s" ;
    state:state [
        <http://kxstudio.sf.net/ns/carla/string>
\"\"\"
%s
\"\"\"
    ] .
""" % (manifestPath, os.path.basename(filenameTry), presetContents))
        manifestFd.close()

    @pyqtSlot()
    def slot_loadProjectLater(self):
        self.fProjectLoading = True
        Carla.host.load_project(self.fProjectFilename)
        self.fProjectLoading = False

    @pyqtSlot()
    def slot_engineStart(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

        if self.fSessionManagerName != "Non Session Manager":
            self.ui.act_file_open.setEnabled(check)

        if check:
            self.fInfoText = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)
            self.refreshTransport(True)

        self.menuTransport(check)

    @pyqtSlot()
    def slot_engineStop(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

        if self.fSessionManagerName != "Non Session Manager":
            self.ui.act_file_open.setEnabled(check)

        if not check:
            self.fInfoText = ""
            self.fInfoLabel.setText("Engine stopped")

        self.menuTransport(check)

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

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            pwidget.setActive(True, True, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            pwidget.setActive(False, True, True)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            if pwidget.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME:
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_VOLUME, 1.0)
                Carla.host.set_volume(i, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            if pwidget.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME:
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_VOLUME, 0.0)
                Carla.host.set_volume(i, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            if pwidget.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET:
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_DRYWET, 1.0)
                Carla.host.set_drywet(i, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            if pwidget.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET:
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_DRYWET, 0.0)
                Carla.host.set_drywet(i, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not self.fEngineStarted:
            return

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            if pwidget.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE:
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_BALANCE_LEFT, -1.0)
                pwidget.ui.edit_dialog.setParameterValue(PARAMETER_BALANCE_RIGHT, 1.0)
                Carla.host.set_balance_left(i, -1.0)
                Carla.host.set_balance_right(i, 1.0)

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if not self.fEngineStarted:
            return

        if toggled:
            Carla.host.transport_play()
        else:
            Carla.host.transport_pause()

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if not self.fEngineStarted:
            return

        Carla.host.transport_pause()
        Carla.host.transport_relocate(0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if not self.fEngineStarted:
            return

        newFrame = Carla.host.get_current_transport_frame() - 100000

        if newFrame < 0:
            newFrame = 0

        Carla.host.transport_relocate(newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if not self.fEngineStarted:
            return

        newFrame = Carla.host.get_current_transport_frame() + 100000
        Carla.host.transport_relocate(newFrame)

    @pyqtSlot()
    def slot_toolbarShown(self):
        self.updateInfoLabelPos()

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self)
        if dialog.exec_():
            self.loadSettings(False)
            patchcanvas.clear()

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

            if self.fEngineStarted:
                Carla.host.patchbay_refresh()

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot()
    def slot_splitterMoved(self):
        self.updateInfoLabelSize()

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

        if not Carla.host.load_filename(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.ui.graphicsView.horizontalScrollBar().setValue(xp * DEFAULT_CANVAS_WIDTH)
        self.ui.graphicsView.verticalScrollBar().setValue(yp * DEFAULT_CANVAS_HEIGHT)

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

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        self.ui.miniCanvasPreview.update()

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.ui.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_fileSave()"))

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        settings = QSettings()
        self.ui.graphicsView.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH / 3, type=int))
        self.ui.graphicsView.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT * 3 / 8, type=int))

        tabBar = self.ui.tabMain.tabBar()
        x = tabBar.width()+20
        y = tabBar.mapFromParent(self.ui.centralwidget.pos()).y()+tabBar.height()/4
        self.fInfoLabel.move(x, y)
        self.fInfoLabel.resize(self.ui.tabMain.width()-x, tabBar.height())

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.ui.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.ui.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.ui.miniCanvasPreview.setViewSize(float(self.ui.graphicsView.width()) / DEFAULT_CANVAS_WIDTH, float(self.ui.graphicsView.height()) / DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        self.ui.pte_log.appendPlainText(valueStr.replace("[30;1m", "DEBUG: ").replace("[31m", "ERROR: ").replace("[0m", "").replace("\n", ""))

    @pyqtSlot(int)
    def slot_handlePluginAddedCallback(self, pluginId):
        pwidget = PluginWidget(self, pluginId)

        self.ui.w_plugins.layout().addWidget(pwidget)

        self.fPluginList.append(pwidget)
        self.fPluginCount += 1

        if not self.fProjectLoading:
            pwidget.setActive(True, True, True)

        if self.fPluginCount == 1:
            self.ui.act_plugin_remove_all.setEnabled(True)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        self.ui.w_plugins.layout().removeWidget(pwidget)

        pwidget.ui.edit_dialog.close()
        pwidget.close()
        pwidget.deleteLater()
        del pwidget

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
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

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayClientAddedCallback(self, clientId, clientIcon, clientName):
        pcSplit = patchcanvas.SPLIT_UNDEF
        pcIcon  = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcSplit = patchcanvas.SPLIT_YES
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE
        elif clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN

        patchcanvas.addGroup(clientId, clientName, pcSplit, pcIcon)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRemovedCallback(self, clientId, clientName):
        if not self.fEngineStarted: return
        patchcanvas.removeGroup(clientId)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

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
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRemovedCallback(self, groupId, portId, fullPortName):
        if not self.fEngineStarted: return
        patchcanvas.removePort(portId)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRenamedCallback(self, groupId, portId, newPortName):
        patchcanvas.renamePort(portId, newPortName)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, portOutId, portInId):
        patchcanvas.connectPorts(connectionId, portOutId, portInId)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId):
        if not self.fEngineStarted: return
        patchcanvas.disconnectPorts(connectionId)
        QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot(int, int)
    def slot_handlePatchbayIconChangedCallback(self, clientId, clientIcon):
        pcIcon = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE
        elif clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN

        patchcanvas.setGroupIcon(clientId, pcIcon)

    @pyqtSlot(int)
    def slot_handleBufferSizeChangedCallback(self, newBufferSize):
        self.fBufferSize = newBufferSize
        self.fInfoText   = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fSampleRate = newSampleRate
        self.fInfoText   = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)

    @pyqtSlot(str)
    def slot_handleNSM_AnnounceCallback(self, smName):
        self.fSessionManagerName = smName
        self.ui.act_file_new.setEnabled(False)
        self.ui.act_file_open.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_start.setEnabled(True)
        self.ui.act_engine_stop.setEnabled(False)

    @pyqtSlot(str)
    def slot_handleNSM_OpenCallback(self, data):
        projectPath, clientId = data.rsplit(":", 1)
        self.fClientName = clientId

        # restart engine
        if self.fEngineStarted:
            self.stopEngine()

        self.slot_engineStart()

        if self.fEngineStarted:
            self.loadProject(projectPath)

        Carla.host.nsm_reply_open()

    @pyqtSlot()
    def slot_handleNSM_SaveCallback(self):
        self.saveProject(self.fProjectFilename)
        Carla.host.nsm_reply_save()

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, self.tr("Error"), error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("Engine has been stopped or crashed.\nPlease restart Carla"),
            self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    def loadSettings(self, geometry):
        settings = QSettings()

        if geometry:
            self.restoreGeometry(settings.value("Geometry", ""))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

            if settings.contains("SplitterState"):
                self.ui.splitter.restoreState(settings.value("SplitterState", ""))
            else:
                self.ui.splitter.setSizes([99999, 210])

            diskFolders = toList(settings.value("DiskFolders", [HOME]))

            self.ui.cb_disk.setItemData(0, HOME)

            for i in range(len(diskFolders)):
                if i == 0: continue
                folder = diskFolders[i]
                self.ui.cb_disk.addItem(os.path.basename(folder), folder)

            if MACOS and not settings.value("Main/UseProTheme", True, type=bool):
                self.setUnifiedTitleAndToolBarOnMac(True)

        useCustomMiniCanvasPaint = bool(settings.value("Main/UseProTheme", True, type=bool) and
                                        settings.value("Main/ProThemeColor", "Black", type=str) == "Black")

        self.fSavedSettings = {
            "Main/DefaultProjectFolder": settings.value("Main/DefaultProjectFolder", HOME, type=str),
            "Main/RefreshInterval": settings.value("Main/RefreshInterval", 50, type=int),
            "Canvas/Theme": settings.value("Canvas/Theme", patchcanvas.getDefaultThemeName(), type=str),
            "Canvas/AutoHideGroups": settings.value("Canvas/AutoHideGroups", False, type=bool),
            "Canvas/UseBezierLines": settings.value("Canvas/UseBezierLines", True, type=bool),
            "Canvas/EyeCandy": settings.value("Canvas/EyeCandy", patchcanvas.EYECANDY_SMALL, type=int),
            "Canvas/UseOpenGL": settings.value("Canvas/UseOpenGL", False, type=bool),
            "Canvas/Antialiasing": settings.value("Canvas/Antialiasing", patchcanvas.ANTIALIASING_SMALL, type=int),
            "Canvas/HighQualityAntialiasing": settings.value("Canvas/HighQualityAntialiasing", False, type=bool),
            "UseCustomMiniCanvasPaint": useCustomMiniCanvasPaint
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

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("SplitterState", self.ui.splitter.saveState())
        settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())
        settings.setValue("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().value())

        diskFolders = []

        for i in range(self.ui.cb_disk.count()):
            diskFolders.append(self.ui.cb_disk.itemData(i))

        settings.setValue("DiskFolders", diskFolders)

    def dragEnterEvent(self, event):
        if event.source() == self.ui.fileTreeView:
            event.accept()
        elif self.ui.tabMain.contentsRect().contains(event.pos()):
            event.accept()
        else:
            QMainWindow.dragEnterEvent(self, event)

    def dropEvent(self, event):
        event.accept()

        urls = event.mimeData().urls()

        for url in urls:
            filename = url.toLocalFile()

            if not Carla.host.load_filename(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    def resizeEvent(self, event):
        if self.ui.tabMain.currentIndex() == 0:
            # Force update of 2nd tab
            width  = self.ui.tab_plugins.width()-4
            height = self.ui.tab_plugins.height()-4
            self.ui.miniCanvasPreview.setViewSize(float(width) / DEFAULT_CANVAS_WIDTH, float(height) / DEFAULT_CANVAS_HEIGHT)
        else:
            QTimer.singleShot(0, self, SLOT("slot_miniCanvasCheckSize()"))

        self.updateInfoLabelSize()

        QMainWindow.resizeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            if not self.fEngineStarted:
                return

            Carla.host.engine_idle()

            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleFast()

            self.refreshTransport()

        elif event.timerId() == self.fIdleTimerSlow:
            if not self.fEngineStarted:
                return

            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()

        if self.fEngineStarted:
            Carla.host.set_engine_about_to_close()
            self.removeAllPlugins()
            self.stopEngine()

        QMainWindow.closeEvent(self, event)

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
        Carla.gui.emit(SIGNAL("PatchbayClientAddedCallback(int, int, QString)"), value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayClientRemovedCallback(int, QString)"), value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_RENAMED:
        Carla.gui.emit(SIGNAL("PatchbayClientRenamedCallback(int, QString)"), value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_ADDED:
        Carla.gui.emit(SIGNAL("PatchbayPortAddedCallback(int, int, int, QString)"), value1, value2, int(value3), cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayPortRemovedCallback(int, int, QString)"), value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_RENAMED:
        Carla.gui.emit(SIGNAL("PatchbayPortRenamedCallback(int, int, QString)"), value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CONNECTION_ADDED:
        Carla.gui.emit(SIGNAL("PatchbayConnectionAddedCallback(int, int, int)"), value1, value2, value3)
    elif action == CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        Carla.gui.emit(SIGNAL("PatchbayConnectionRemovedCallback(int)"), value1)
    elif action == CALLBACK_PATCHBAY_ICON_CHANGED:
        Carla.gui.emit(SIGNAL("PatchbayIconChangedCallback(int, int)"), value1, value2)
    elif action == CALLBACK_BUFFER_SIZE_CHANGED:
        Carla.gui.emit(SIGNAL("BufferSizeChangedCallback(int)"), value1)
    elif action == CALLBACK_SAMPLE_RATE_CHANGED:
        Carla.gui.emit(SIGNAL("SampleRateChangedCallback(double)"), value3)
    elif action == CALLBACK_NSM_ANNOUNCE:
        Carla.gui.emit(SIGNAL("NSM_AnnounceCallback(QString)"), cString(valueStr))
    elif action == CALLBACK_NSM_OPEN:
        Carla.gui.emit(SIGNAL("NSM_OpenCallback(QString)"), cString(valueStr))
    elif action == CALLBACK_NSM_SAVE:
        Carla.gui.emit(SIGNAL("NSM_SaveCallback()"))
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

    argv = app.arguments()
    argc = len(argv)

    for i in range(argc):
        if i == 0: continue
        argument = argv[i]

        if argument.startswith("--with-appname="):
            appName = os.path.basename(argument.replace("--with-appname=", ""))

        elif argument.startswith("--with-libprefix="):
            libPrefix = argument.replace("--with-libprefix=", "")

        elif os.path.exists(argument):
            projectFilename = argument

    if libPrefix is not None:
        libPath = os.path.join(libPrefix, "lib", "carla")
        libName = os.path.join(libPath, carla_libname)
    else:
        libPath = carla_library_filename.replace(carla_libname, "")
        libName = carla_library_filename

    # Init backend
    Carla.host = Host(libName)

    if NSM_URL:
        Carla.host.nsm_announce(NSM_URL, appName, os.getpid())
    else:
        Carla.host.set_engine_option(OPTION_PROCESS_NAME, 0, "carla")

    Carla.host.set_engine_option(OPTION_PATH_RESOURCES, 0, libPath)

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

        if carla_bridge_vst_mac:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_MAC, 0, carla_bridge_vst_mac)

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

    # Only now we're ready to handle events
    Carla.host.set_engine_callback(engineCallback)

    # Set-up custom signal handling
    setUpSignals()

    # Show GUI
    Carla.gui.show()

    # Load project file if set
    if projectFilename and not NSM_URL:
        Carla.gui.loadProjectLater(projectFilename)

    # App-Loop
    ret = app.exec_()

    # Destroy GUI
    tmp = Carla.gui
    Carla.gui = None
    del tmp

    # Exit properly
    sys.exit(ret)
