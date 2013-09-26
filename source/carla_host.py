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
# Dummy widget

class CarlaDummyW(object):
    def __init__(self, parent):
        object.__init__(self)

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return 0

    def getPlugin(self, pluginId):
        return None

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

    def setParameterValue(self, pluginId, index, value):
        pass

    def setParameterDefault(self, pluginId, index, value):
        pass

    def setParameterMidiChannel(self, pluginId, index, channel):
        pass

    def setParameterMidiCC(self, pluginId, index, cc):
        pass

    # -----------------------------------------------------------------

    def setProgram(self, pluginId, index):
        pass

    def setMidiProgram(self, pluginId, index):
        pass

    # -----------------------------------------------------------------

    def noteOn(self, pluginId, channel, note, velocity):
        pass

    def noteOff(self, pluginId, channel, note):
        pass

    # -----------------------------------------------------------------

    def setGuiState(self, pluginId, state):
        pass

    # -----------------------------------------------------------------

    def updateInfo(self, pluginId):
        pass

    def reloadInfo(self, pluginId):
        pass

    def reloadParameters(self, pluginId):
        pass

    def reloadPrograms(self, pluginId):
        pass

    def reloadAll(self, pluginId):
        pass

    # -----------------------------------------------------------------

    def patchbayClientAdded(self, clientId, clientIcon, clientName):
        pass

    def patchbayClientRemoved(self, clientId, clientName):
        pass

    def patchbayClientRenamed(self, clientId, newClientName):
        pass

    def patchbayPortAdded(self, clientId, portId, portFlags, portName):
        pass

    def patchbayPortRemoved(self, groupId, portId, fullPortName):
        pass

    def patchbayPortRenamed(self, groupId, portId, newPortName):
        pass

    def patchbayConnectionAdded(self, connectionId, portOutId, portInId):
        pass

    def patchbayConnectionRemoved(self, connectionId):
        pass

    def patchbayIconChanged(self, clientId, clientIcon):
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

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0
        self.fIsProjectLoading = False

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        # -------------------------------------------------------------
        # Connect actions to functions

        #self.ui.act_file_new.triggered.connect(self.slot_fileNew)
        #self.ui.act_file_open.triggered.connect(self.slot_fileOpen)
        #self.ui.act_file_save.triggered.connect(self.slot_fileSave)
        #self.ui.act_file_save_as.triggered.connect(self.slot_fileSaveAs)

        #self.ui.act_engine_start.triggered.connect(self.slot_engineStart)
        #self.ui.act_engine_stop.triggered.connect(self.slot_engineStop)

        self.ui.act_plugin_add.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_add2.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_remove_all.triggered.connect(self.slot_pluginRemoveAll)

        #self.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        #self.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        #self.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)
        #self.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        #self.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        #self.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        #self.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        #self.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)

        #self.ui.act_transport_play-triggered(bool)"), SLOT("slot_transportPlayPause(bool)"))
        #self.ui.act_transport_stop.triggered.connect(self.slot_transportStop)
        #self.ui.act_transport_backwards.triggered.connect(self.slot_transportBackwards)
        #self.ui.act_transport_forwards.triggered.connect(self.slot_transportForwards)

        #self.ui.act_canvas_arrange.setEnabled(False) # TODO, later
        #self.ui.act_canvas_arrange.triggered.connect(self.slot_canvasArrange)
        #self.ui.act_canvas_refresh.triggered.connect(self.slot_canvasRefresh)
        #self.ui.act_canvas_zoom_fit.triggered.connect(self.slot_canvasZoomFit)
        #self.ui.act_canvas_zoom_in.triggered.connect(self.slot_canvasZoomIn)
        #self.ui.act_canvas_zoom_out.triggered.connect(self.slot_canvasZoomOut)
        #self.ui.act_canvas_zoom_100.triggered.connect(self.slot_canvasZoomReset)
        #self.ui.act_canvas_print.triggered.connect(self.slot_canvasPrint)
        #self.ui.act_canvas_save_image.triggered.connect(self.slot_canvasSaveImage)

        self.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

        self.ui.act_help_about.triggered.connect(self.slot_aboutCarla)
        self.ui.act_help_about_qt.triggered.connect(self.slot_aboutQt)

        #self.ui.splitter-splitterMoved.connect(self.slot_splitterMoved)

        #self.ui.cb_disk-currentIndexChanged.connect(self.slot_diskFolderChanged)
        #self.ui.b_disk_add-clicked.connect(self.slot_diskFolderAdd)
        #self.ui.b_disk_remove-clicked.connect(self.slot_diskFolderRemove)
        #self.ui.fileTreeView-doubleClicked(QModelIndex)"), SLOT("slot_fileTreeDoubleClicked(QModelIndex)"))
        #self.ui.miniCanvasPreview-miniCanvasMoved(double, double)"), SLOT("slot_miniCanvasMoved(double, double)"))

        #self.ui.graphicsView.horizontalScrollBar()-valueChanged.connect(self.slot_horizontalScrollBarChanged)
        #self.ui.graphicsView.verticalScrollBar()-valueChanged.connect(self.slot_verticalScrollBarChanged)

        #self.scene-sceneGroupMoved(int, int, QPointF)"), SLOT("slot_canvasItemMoved(int, int, QPointF)"))
        #self.scene-scaleChanged(double)"), SLOT("slot_canvasScaleChanged(double)"))

        self.DebugCallback.connect(self.slot_handleDebugCallback)
        self.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        self.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        self.PluginRenamedCallback.connect(self.slot_handlePluginRenamedCallback)
        self.ParameterValueChangedCallback.connect(self.slot_handleParameterValueChangedCallback)
        self.ParameterDefaultChangedCallback.connect(self.slot_handleParameterDefaultChangedCallback)
        self.ParameterMidiChannelChangedCallback.connect(self.slot_handleParameterMidiChannelChangedCallback)
        self.ParameterMidiCcChangedCallback.connect(self.slot_handleParameterMidiCcChangedCallback)
        self.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        self.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        self.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        self.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        self.ShowGuiCallback.connect(self.slot_handleShowGuiCallback)
        self.UpdateCallback.connect(self.slot_handleUpdateCallback)
        self.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        self.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        self.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        self.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)
        self.PatchbayClientAddedCallback.connect(self.slot_handlePatchbayClientAddedCallback)
        self.PatchbayClientRemovedCallback.connect(self.slot_handlePatchbayClientRemovedCallback)
        self.PatchbayClientRenamedCallback.connect(self.slot_handlePatchbayClientRenamedCallback)
        self.PatchbayPortAddedCallback.connect(self.slot_handlePatchbayPortAddedCallback)
        self.PatchbayPortRemovedCallback.connect(self.slot_handlePatchbayPortRemovedCallback)
        self.PatchbayPortRenamedCallback.connect(self.slot_handlePatchbayPortRenamedCallback)
        self.PatchbayConnectionAddedCallback.connect(self.slot_handlePatchbayConnectionAddedCallback)
        self.PatchbayConnectionRemovedCallback.connect(self.slot_handlePatchbayConnectionRemovedCallback)
        self.PatchbayIconChangedCallback.connect(self.slot_handlePatchbayIconChangedCallback)
        #self.BufferSizeChangedCallback.connect(self.slot_handleBufferSizeChangedCallback)
        #self.SampleRateChangedCallback(double)"), SLOT("slot_handleSampleRateChangedCallback(double)"))
        #self.NSM_AnnounceCallback(QString)"), SLOT("slot_handleNSM_AnnounceCallback(QString)"))
        #self.NSM_OpenCallback(QString)"), SLOT("slot_handleNSM_OpenCallback(QString)"))
        #self.NSM_SaveCallback.connect(self.slot_handleNSM_SaveCallback)
        #self.ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        #self.QuitCallback.connect(self.slot_handleQuitCallback)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

    # -----------------------------------------------------------------

    def init(self):
        self.fIdleTimerFast = self.startTimer(50)
        self.fIdleTimerSlow = self.startTimer(50*2)

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
        self.fContainer.removeAllPlugins()

        Carla.host.remove_all_plugins()

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self, False) # TODO - hasGL

        if not dialog.exec_():
            return

        #self.loadSettings(False)
        #patchcanvas.clear()

        #pOptions = patchcanvas.options_t()
        #pOptions.theme_name       = self.fSavedSettings["Canvas/Theme"]
        #pOptions.auto_hide_groups = self.fSavedSettings["Canvas/AutoHideGroups"]
        #pOptions.use_bezier_lines = self.fSavedSettings["Canvas/UseBezierLines"]
        #pOptions.antialiasing     = self.fSavedSettings["Canvas/Antialiasing"]
        #pOptions.eyecandy         = self.fSavedSettings["Canvas/EyeCandy"]

        #pFeatures = patchcanvas.features_t()
        #pFeatures.group_info   = False
        #pFeatures.group_rename = False
        #pFeatures.port_info    = False
        #pFeatures.port_rename  = False
        #pFeatures.handle_group_pos = True

        #patchcanvas.setOptions(pOptions)
        #patchcanvas.setFeatures(pFeatures)
        #patchcanvas.init("Carla", self.scene, canvasCallback, False)

        #if self.fEngineStarted:
            #Carla.host.patchbay_refresh()

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

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, parameterId, value):
        self.fContainer.setParameterValue(parameterId, value)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, parameterId, value):
        self.fContainer.setParameterDefault(parameterId, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, parameterId, channel):
        self.fContainer.setParameterMidiChannel(parameterId, channel)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, parameterId, cc):
        self.fContainer.setParameterMidiCC(parameterId, cc)

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, programId):
        self.fContainer.setProgram(programId)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, midiProgramId):
        self.fContainer.setMidiProgram(midiProgramId)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        self.fContainer.noteOn(channel, note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        self.fContainer.noteOff(channel, note)

    @pyqtSlot(int, int)
    def slot_handleShowGuiCallback(self, pluginId, state):
        self.fContainer.setGuiState(pluginId, state)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        self.fContainer.updateInfo(pluginId)

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        self.fContainer.reloadInfo(pluginId)

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        self.fContainer.reloadParameters(pluginId)

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        self.fContainer.reloadPrograms(pluginId)

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        self.fContainer.reloadAll(pluginId)

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayClientAddedCallback(self, clientId, clientIcon, clientName):
        self.fContainer.patchbayClientAdded(clientId, clientIcon, clientName)

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRemovedCallback(self, clientId, clientName):
        self.fContainer.patchbayClientRemoved(clientId, clientName)

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        self.fContainer.patchbayClientRenamed(clientId, newClientName)

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortAddedCallback(self, clientId, portId, portFlags, portName):
        self.fContainer.patchbayPortAdded(clientId, portId, portFlags, portName)

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRemovedCallback(self, groupId, portId, fullPortName):
        self.fContainer.patchbayPortRemoved(groupId, portId, fullPortName)

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRenamedCallback(self, groupId, portId, newPortName):
        self.fContainer.patchbayPortRenamed(groupId, portId, newPortName)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, portOutId, portInId):
        self.fContainer.patchbayConnectionAdded(connectionId, portOutId, portInId)

    @pyqtSlot(int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId):
        self.fContainer.patchbayConnectionRemoved(connectionId)

    @pyqtSlot(int, int)
    def slot_handlePatchbayIconChangedCallback(self, clientId, clientIcon):
        self.fContainer.patchbayIconChanged(clientId, clientIcon)

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
