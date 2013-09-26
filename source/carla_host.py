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

try:
    from PyQt5.QtWidgets import QApplication, QMainWindow
except:
    from PyQt4.QtGui import QApplication, QMainWindow

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_host

from carla_database import *
from carla_settings import *
from carla_widgets import *

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

    def removeAllPlugins(self):
        pass

    # -----------------------------------------------------------------

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass

    # -----------------------------------------------------------------

    def idleFast(self):
        pass

    def idleSlow(self):
        pass

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
    # signals
    DebugCallback = pyqtSignal(int, int, int, float, str)
    PluginAddedCallback = pyqtSignal(int)
    PluginRemovedCallback = pyqtSignal(int)
    PluginRenamedCallback = pyqtSignal(int, str)
    ParameterValueChangedCallback = pyqtSignal(int, int, float)
    ParameterDefaultChangedCallback = pyqtSignal(int, int, float)
    ParameterMidiChannelChangedCallback = pyqtSignal(int, int, int)
    ParameterMidiCcChangedCallback = pyqtSignal(int, int, int)
    ProgramChangedCallback = pyqtSignal(int, int)
    MidiProgramChangedCallback = pyqtSignal(int, int)
    NoteOnCallback = pyqtSignal(int, int, int, int)
    NoteOffCallback = pyqtSignal(int, int, int)
    ShowGuiCallback = pyqtSignal(int, int)
    UpdateCallback = pyqtSignal(int)
    ReloadInfoCallback = pyqtSignal(int)
    ReloadParametersCallback = pyqtSignal(int)
    ReloadProgramsCallback = pyqtSignal(int)
    ReloadAllCallback = pyqtSignal(int)
    PatchbayClientAddedCallback = pyqtSignal(int, int, str)
    PatchbayClientRemovedCallback = pyqtSignal(int, str)
    PatchbayClientRenamedCallback = pyqtSignal(int, str)
    PatchbayPortAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayPortRemovedCallback = pyqtSignal(int, int, str)
    PatchbayPortRenamedCallback = pyqtSignal(int, int, str)
    PatchbayConnectionAddedCallback = pyqtSignal(int, int, int)
    PatchbayConnectionRemovedCallback = pyqtSignal(int)
    PatchbayIconChangedCallback = pyqtSignal(int, int)
    BufferSizeChangedCallback = pyqtSignal(int)
    SampleRateChangedCallback = pyqtSignal(float)
    NSM_AnnounceCallback = pyqtSignal(str)
    NSM_OpenCallback = pyqtSignal(str)
    NSM_SaveCallback = pyqtSignal()
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
        # Set callback

        Carla.host.set_engine_callback(EngineCallback)

        # -------------------------------------------------------------
        # Internal stuff

        self.fBufferSize = 0
        self.fSampleRate = 0.0

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0
        self.fIsProjectLoading = False

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fLastTransportFrame = 0
        self.fLastTransportState = False

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
        # Set up GUI (engine stopped)

        self.ui.act_file_save.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_start.setEnabled(True)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)
        #self.ui.menu_Plugins.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)

        self.setTransportMenuEnabled(False)

        # -------------------------------------------------------------
        # Connect actions to functions

        #self.ui.act_file_new.triggered.connect(self.slot_fileNew)
        #self.ui.act_file_open.triggered.connect(self.slot_fileOpen)
        #self.ui.act_file_save.triggered.connect(self.slot_fileSave)
        #self.ui.act_file_save_as.triggered.connect(self.slot_fileSaveAs)

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

        #self.ui.splitter.splitterMoved.connect(self.slot_splitterMoved)

        #self.ui.cb_disk.currentIndexChanged.connect(self.slot_diskFolderChanged)
        #self.ui.b_disk_add.clicked.connect(self.slot_diskFolderAdd)
        #self.ui.b_disk_remove.clicked.connect(self.slot_diskFolderRemove)
        #self.ui.fileTreeView.doubleClicked.connect(self.slot_fileTreeDoubleClicked)

        self.DebugCallback.connect(self.slot_handleDebugCallback)
        self.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        self.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        self.PluginRenamedCallback.connect(self.slot_handlePluginRenamedCallback)
        self.BufferSizeChangedCallback.connect(self.slot_handleBufferSizeChangedCallback)
        self.SampleRateChangedCallback.connect(self.slot_handleSampleRateChangedCallback)
        #self.NSM_AnnounceCallback.connect(self.slot_handleNSM_AnnounceCallback)
        #self.NSM_OpenCallback.connect(self.slot_handleNSM_OpenCallback)
        #self.NSM_SaveCallback.connect(self.slot_handleNSM_SaveCallback)
        #self.ErrorCallback.connect(self.slot_handleErrorCallback)
        #self.QuitCallback.connect(self.slot_handleQuitCallback)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

    # -----------------------------------------------------------------
    # Called by containers

    def openSettings(self, hasCanvas, hasCanvasGL):
        dialog = CarlaSettingsW(self, hasCanvas, hasCanvasGL)
        return dialog.exec_()

    # -----------------------------------------------------------------
    # Internal stuff

    def startEngine(self):
        # ---------------------------------------------
        # Engine settings

        settings = QSettings()

        forceStereo         = settings.value("Engine/ForceStereo",         CARLA_DEFAULT_FORCE_STEREO,          type=bool)
        preferPluginBridges = settings.value("Engine/PreferPluginBridges", CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool)
        preferUiBridges     = settings.value("Engine/PreferUiBridges",     CARLA_DEFAULT_PREFER_UI_BRIDGES,     type=bool)
        uisAlwaysOnTop      = settings.value("Engine/OscUiTimeout",        CARLA_DEFAULT_UIS_ALWAYS_ON_TOP,     type=bool)
        uiBridgesTimeout    = settings.value("Engine/OscUiTimeout",        CARLA_DEFAULT_UI_BRIDGES_TIMEOUT,    type=int)

        Carla.processMode   = settings.value("Engine/ProcessMode",         CARLA_DEFAULT_PROCESS_MODE,          type=int)
        Carla.maxParameters = settings.value("Engine/MaxParameters",       CARLA_DEFAULT_MAX_PARAMETERS,        type=int)

        audioDriver         = settings.value("Engine/AudioDriver",         CARLA_DEFAULT_AUDIO_DRIVER,          type=str)

        if audioDriver == "JACK":
            #transportMode  = settings.value("Engine/TransportMode",       TRANSPORT_MODE_JACK,                 type=int)
            transportMode   = TRANSPORT_MODE_JACK
        else:
            transportMode   = TRANSPORT_MODE_INTERNAL
            audioNumPeriods = settings.value("Engine/AudioBufferSize", CARLA_DEFAULT_AUDIO_NUM_PERIODS, type=int)
            audioBufferSize = settings.value("Engine/AudioBufferSize", CARLA_DEFAULT_AUDIO_BUFFER_SIZE, type=int)
            audioSampleRate = settings.value("Engine/AudioSampleRate", CARLA_DEFAULT_AUDIO_SAMPLE_RATE, type=int)
            audioDevice     = settings.value("Engine/AudioDevice",     "",                              type=str)

            Carla.host.set_engine_option(OPTION_AUDIO_NUM_PERIODS, audioNumPeriods, "")
            Carla.host.set_engine_option(OPTION_AUDIO_BUFFER_SIZE, audioBufferSize, "")
            Carla.host.set_engine_option(OPTION_AUDIO_SAMPLE_RATE, audioSampleRate, "")
            Carla.host.set_engine_option(OPTION_AUDIO_DEVICE,   0, audioDevice)

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif Carla.processMode == PROCESS_MODE_MULTIPLE_CLIENTS: # and LADISH_APP_NAME:
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            Carla.processMode = PROCESS_MODE_SINGLE_CLIENT

        Carla.host.set_engine_option(OPTION_FORCE_STEREO,          forceStereo,         "")
        Carla.host.set_engine_option(OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")
        Carla.host.set_engine_option(OPTION_PREFER_UI_BRIDGES,     preferUiBridges,     "")
        Carla.host.set_engine_option(OPTION_UIS_ALWAYS_ON_TOP,     uisAlwaysOnTop,      "")
        Carla.host.set_engine_option(OPTION_UI_BRIDGES_TIMEOUT,    uiBridgesTimeout,    "")
        Carla.host.set_engine_option(OPTION_PROCESS_MODE,          Carla.processMode,   "")
        Carla.host.set_engine_option(OPTION_MAX_PARAMETERS,        Carla.maxParameters, "")
        Carla.host.set_engine_option(OPTION_TRANSPORT_MODE,        transportMode,       "")

        # ---------------------------------------------
        # Start

        if not Carla.host.engine_init(audioDriver, self.fClientName):
            #if self.fFirstEngineInit:
                #self.fFirstEngineInit = False
                #return

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fBufferSize = Carla.host.get_buffer_size()
        self.fSampleRate = Carla.host.get_sample_rate()

        #self.fFirstEngineInit = False

        # Peaks and TimeInfo
        #self.fIdleTimerFast = self.startTimer(self.fSavedSettings["Main/RefreshInterval"])
        # LEDs and edit dialog parameters
        #self.fIdleTimerSlow = self.startTimer(self.fSavedSettings["Main/RefreshInterval"]*2)
        self.fIdleTimerFast = self.startTimer(50)
        self.fIdleTimerSlow = self.startTimer(50*2)

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
            print(cString(Carla.host.get_last_error()))

        self.fBufferSize = 0
        self.fSampleRate = 0.0

        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

    def getExtraStuff(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']

            self.maybeLoadRDFs()

            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype in (PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
            if plugin['name'].lower().endswith(" (16 outputs)"):
                # return a dummy non-null pointer
                INTPOINTER = POINTER(c_int)
                int1 = c_int(0x1)
                addr = addressof(int1)
                return cast(addr, INTPOINTER)

        return c_nullptr

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

    def refreshTransport(self, forced = False):
        if not Carla.host.is_engine_running():
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

            #textTransport = "Transport %s, at %02i:%02i:%02i" % ("playing" if playing else "stopped", hrs, mins, secs)
            #self.fInfoLabel.setText("%s | %s" % (self.fInfoText, textTransport))

            self.fLastTransportFrame = frame

    def setTransportMenuEnabled(self, enabled):
        self.ui.act_transport_play.setEnabled(enabled)
        self.ui.act_transport_stop.setEnabled(enabled)
        self.ui.act_transport_backwards.setEnabled(enabled)
        self.ui.act_transport_forwards.setEnabled(enabled)
        self.ui.menu_Transport.setEnabled(enabled)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_engineStart(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_save.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if self.fSessionManagerName != "Non Session Manager":
            self.ui.act_file_open.setEnabled(check)
            self.ui.act_file_save_as.setEnabled(check)

        if check:
            #self.fInfoText = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)
            self.refreshTransport(True)
            self.fContainer.engineStarted()

        self.setTransportMenuEnabled(check)

    @pyqtSlot()
    def slot_engineStop(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_save.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)
        self.ui.menu_Canvas.setEnabled(check)

        if self.fSessionManagerName != "Non Session Manager":
            self.ui.act_file_open.setEnabled(check)
            self.ui.act_file_save_as.setEnabled(check)

        if not check:
            self.fContainer.engineStopped()
            #self.fInfoText = ""
            #self.fInfoLabel.setText("Engine stopped")

        self.setTransportMenuEnabled(check)

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
        filename = dialog.fRetPlugin['binary']
        label    = dialog.fRetPlugin['label']
        extraStuff = self.getExtraStuff(dialog.fRetPlugin)

        if not Carla.host.add_plugin(btype, ptype, filename, None, label, c_nullptr):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
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

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG:", pluginId, value1, value2, value3, valueStr)
        #self.ui.pte_log.appendPlainText(valueStr.replace("[30;1m", "DEBUG: ").replace("[31m", "ERROR: ").replace("[0m", "").replace("\n", ""))

    @pyqtSlot(int)
    def slot_handlePluginAddedCallback(self, pluginId):
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

    @pyqtSlot(int)
    def slot_handleBufferSizeChangedCallback(self, newBufferSize):
        self.fBufferSize = newBufferSize
        #self.fInfoText   = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fSampleRate = newSampleRate
        #self.fInfoText   = "Engine running | SampleRate: %g | BufferSize: %i" % (self.fSampleRate, self.fBufferSize)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        #QTimer.singleShot(0, self, SLOT("slot_fileSave)

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    # -----------------------------------------------------------------

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            Carla.host.engine_idle()
            self.fContainer.idleFast()
            self.refreshTransport()

        elif event.timerId() == self.fIdleTimerSlow:
            self.fContainer.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

        #self.saveSettings()

        if Carla.host.is_engine_running():
            Carla.host.set_engine_about_to_close()
            self.ui.act_plugin_remove_all.setEnabled(False)
            self.fContainer.removeAllPlugins()
            self.stopEngine()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def EngineCallback(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId < 0 or not Carla.gui:
        return

    if action == CALLBACK_DEBUG:
        Carla.gui.DebugCallback.emit(pluginId, value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PLUGIN_ADDED:
        Carla.gui.PluginAddedCallback.emit(pluginId)
    elif action == CALLBACK_PLUGIN_REMOVED:
        Carla.gui.PluginRemovedCallback.emit(pluginId)
    elif action == CALLBACK_PLUGIN_RENAMED:
        Carla.gui.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        Carla.gui.ParameterValueChangedCallback.emit(pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_DEFAULT_CHANGED:
        Carla.gui.ParameterDefaultChangedCallback.emit(pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        Carla.gui.ParameterMidiChannelChangedCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        Carla.gui.ParameterMidiCcChangedCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_PROGRAM_CHANGED:
        Carla.gui.ProgramChangedCallback.emit(pluginId, value1)
    elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        Carla.gui.MidiProgramChangedCallback.emit(pluginId, value1)
    elif action == CALLBACK_NOTE_ON:
        Carla.gui.NoteOnCallback.emit(pluginId, value1, value2, value3)
    elif action == CALLBACK_NOTE_OFF:
        Carla.gui.NoteOffCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_SHOW_GUI:
        Carla.gui.ShowGuiCallback.emit(pluginId, value1)
    elif action == CALLBACK_UPDATE:
        Carla.gui.UpdateCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_INFO:
        Carla.gui.ReloadInfoCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_PARAMETERS:
        Carla.gui.ReloadParametersCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_PROGRAMS:
        Carla.gui.ReloadProgramsCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_ALL:
        Carla.gui.ReloadAllCallback.emit(pluginId)
    elif action == CALLBACK_PATCHBAY_CLIENT_ADDED:
        Carla.gui.PatchbayClientAddedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_REMOVED:
        Carla.gui.PatchbayClientRemovedCallback.emit(value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_RENAMED:
        Carla.gui.PatchbayClientRenamedCallback.emit(value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_ADDED:
        Carla.gui.PatchbayPortAddedCallback.emit(value1, value2, int(value3), cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_REMOVED:
        Carla.gui.PatchbayPortRemovedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_RENAMED:
        Carla.gui.PatchbayPortRenamedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CONNECTION_ADDED:
        Carla.gui.PatchbayConnectionAddedCallback.emit(value1, value2, value3)
    elif action == CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        Carla.gui.PatchbayConnectionRemovedCallback.emit(value1)
    elif action == CALLBACK_PATCHBAY_ICON_CHANGED:
        Carla.gui.PatchbayIconChangedCallback.emit(value1, value2)
    elif action == CALLBACK_BUFFER_SIZE_CHANGED:
        Carla.gui.BufferSizeChangedCallback.emit(value1)
    elif action == CALLBACK_SAMPLE_RATE_CHANGED:
        Carla.gui.SampleRateChangedCallback.emit(value3)
    elif action == CALLBACK_NSM_ANNOUNCE:
        Carla.gui.NSM_AnnounceCallback.emit(cString(valueStr))
    elif action == CALLBACK_NSM_OPEN:
        Carla.gui.NSM_OpenCallback.emit(cString(valueStr))
    elif action == CALLBACK_NSM_SAVE:
        Carla.gui.NSM_SaveCallback.emit()
    elif action == CALLBACK_ERROR:
        Carla.gui.ErrorCallback.emit(cString(valueStr))
    elif action == CALLBACK_QUIT:
        Carla.gui.QuitCallback.emit()
