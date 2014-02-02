#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla rack widget code
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

from PyQt4.QtCore import QSize, QTimer
from PyQt4.QtGui import QApplication, QListWidget, QListWidgetItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_skin import *

# ------------------------------------------------------------------------------------------------------------
# Rack widget item

class CarlaRackItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1

    def __init__(self, parent, pluginId):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)

        self.widget = createPluginSlot(parent, pluginId)
        self.widget.setFixedHeight(self.widget.getFixedHeight())

        self.setSizeHint(QSize(300, self.widget.getFixedHeight()))

        parent.setItemWidget(self, self.widget)

    # -----------------------------------------------------------------

    def close(self):
        self.widget.fEditDialog.close()

    #def setId(self, idx):
        #self.widget.setId(idx)

    #def setName(self, newName):
        #self.widget.ui.label_name.setText(newName)
        #self.widget.ui.edit_dialog.setName(newName)

# ------------------------------------------------------------------------------------------------------------
# Rack widget

class CarlaRackW(QListWidget):
    def __init__(self, parent, doSetup = True):
        QListWidget.__init__(self, parent)

        # -------------------------------------------------------------
        # Internal stuff

        self.fParent      = parent
        self.fPluginCount = 0
        self.fPluginList  = []

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.setMinimumWidth(591)
        self.setSortingEnabled(False)

        app  = QApplication.instance()
        pal1 = app.palette().base().color()
        pal2 = app.palette().button().color()
        col1 = "stop:0 rgb(%i, %i, %i)" % (pal1.red(), pal1.green(), pal1.blue())
        col2 = "stop:1 rgb(%i, %i, %i)" % (pal2.red(), pal2.green(), pal2.blue())

        self.setStyleSheet("""
          QListWidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                %s,
                %s
            );
          }
        """ % (col1, col2))

        # -------------------------------------------------------------
        # Connect actions to functions

        if not doSetup: return

        parent.ui.menu_Canvas.hide()

        parent.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        parent.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        parent.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        parent.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        parent.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        parent.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        parent.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        parent.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)

        parent.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

        parent.ParameterValueChangedCallback.connect(self.slot_handleParameterValueChangedCallback)
        parent.ParameterDefaultChangedCallback.connect(self.slot_handleParameterDefaultChangedCallback)
        parent.ParameterMidiChannelChangedCallback.connect(self.slot_handleParameterMidiChannelChangedCallback)
        parent.ParameterMidiCcChangedCallback.connect(self.slot_handleParameterMidiCcChangedCallback)
        parent.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        parent.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        parent.UiStateChangedCallback.connect(self.slot_handleUiStateChangedCallback)
        parent.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        parent.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        parent.UpdateCallback.connect(self.slot_handleUpdateCallback)
        parent.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        parent.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        parent.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        parent.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return self.fPluginCount

    # -----------------------------------------------------------------

    def addPlugin(self, pluginId, isProjectLoading):
        pitem = CarlaRackItem(self, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        if not isProjectLoading:
            pitem.widget.setActive(True, True, True)

    def removePlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        self.takeItem(pluginId)

        pitem.close()
        del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pitem = self.fPluginList[i]
            pitem.widget.setId(i)

    def renamePlugin(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setName(newName)

    def disablePlugin(self, pluginId, errorMsg):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

    def removeAllPlugins(self):
        while (self.takeItem(0)):
            pass

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.close()
            del pitem

        self.fPluginCount = 0
        self.fPluginList  = []

    # -----------------------------------------------------------------

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass

    def engineChanged(self):
        pass

    # -----------------------------------------------------------------

    def idleFast(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.widget.idleFast()

    def idleSlow(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.widget.idleSlow()

    # -----------------------------------------------------------------

    def saveSettings(self, settings):
        pass

    def showEditDialog(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.slot_showEditDialog(True)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setActive(True, True, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setActive(False, True, True)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_VOLUME, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_VOLUME, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_DRYWET, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_DRYWET, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PARAMETER_BALANCE_LEFT, -1.0)
            pitem.widget.setInternalParameter(PARAMETER_BALANCE_RIGHT, 1.0)
            pitem.widget.setInternalParameter(PARAMETER_PANNING, 0.0)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_configureCarla(self):
        if self.fParent is None or not self.fParent.openSettingsWindow(False, False):
            return

        self.fParent.loadSettings(False)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterValue(index, value)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterDefault(index, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, index, cc):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterMidiControl(index, cc)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, index, channel):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterMidiChannel(index, channel)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setProgram(index)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setMidiProgram(index)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_handleUiStateChangedCallback(self, pluginId, state):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.customUiStateChanged(state)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.sendNoteOn(channel, note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.sendNoteOff(channel, note)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadAll()

    # -----------------------------------------------------------------
