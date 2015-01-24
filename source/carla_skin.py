#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin/slot skin code
# Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
    from PyQt5.QtCore import Qt, QRectF
    from PyQt5.QtGui import QFont, QFontDatabase, QPen, QPixmap
    from PyQt5.QtWidgets import QFrame, QPushButton
else:
    from PyQt4.QtCore import Qt, QRectF
    from PyQt4.QtGui import QFont, QFontDatabase, QFrame, QPen, QPixmap, QPushButton

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_plugin_default
import ui_carla_plugin_basic_fx
import ui_carla_plugin_calf
import ui_carla_plugin_sf2
import ui_carla_plugin_zynfx

from carla_widgets import *
from digitalpeakmeter import DigitalPeakMeter
from pixmapdial import PixmapDial

# ------------------------------------------------------------------------------------------------------------
# Try to "shortify" a parameter name

def getParameterShortName(paramName):
    paramName = paramName.split("/",1)[0].split(" (",1)[0].split(" [",1)[0].strip()
    paramLow  = paramName.lower()

    if paramLow.startswith("compressor "):
        paramName = paramName.replace("ompressor ", ".", 1)
        paramLow  = paramName.lower()

    # Cut generic names
    if "attack" in paramLow:
        paramName = paramName.replace("ttack", "tk")
    elif "bandwidth" in paramLow:
        paramName = paramName.replace("andwidth", "w")
    elif "damping" in paramLow:
        paramName = paramName.replace("amping", "amp")
    elif "distortion" in paramLow:
        paramName = paramName.replace("istortion", "ist")
    elif "feedback" in paramLow:
        paramName = paramName.replace("eedback", "b")
    elif "frequency" in paramLow:
        paramName = paramName.replace("requency", "req")
    elif "makeup" in paramLow:
        paramName = paramName.replace("akeup", "kUp" if "Make" in paramName else "kup")
    elif "output" in paramLow:
        paramName = paramName.replace("utput", "ut")
    elif "random" in paramLow:
        paramName = paramName.replace("andom", "nd")
    elif "threshold" in paramLow:
        paramName = paramName.replace("hreshold", "hres")

    # Cut useless prefix
    elif paramLow.startswith("room "):
        paramName = paramName.split(" ",1)[1]

    # Cut useless suffix
    elif paramLow.endswith(" level"):
        paramName = paramName.rsplit(" ",1)[0]
    elif paramLow.endswith(" time"):
        paramName = paramName.rsplit(" ",1)[0]

    if len(paramName) > 7:
        paramName = paramName[:7]

    return paramName.strip()

# ------------------------------------------------------------------------------------------------------------
# Abstract plugin slot

class AbstractPluginSlot(QFrame, PluginEditParentMeta):
#class AbstractPluginSlot(QFrame, PluginEditParentMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, pluginId):
        QFrame.__init__(self, parent)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostMeta()
            self.host = host

        # -------------------------------------------------------------
        # Get plugin info

        self.fPluginId   = pluginId
        self.fPluginInfo = host.get_plugin_info(self.fPluginId)

        #if not gCarla.isLocal:
            #self.fPluginInfo['hints'] &= ~PLUGIN_HAS_CUSTOM_UI

        # -------------------------------------------------------------
        # Internal stuff

        self.fIsActive    = bool(host.get_internal_parameter_value(self.fPluginId, PARAMETER_ACTIVE) >= 0.5)
        self.fIsCollapsed = False
        self.fIsSelected  = False

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        self.fParameterIconTimer = ICON_STATE_NULL
        self.fParameterList      = [] # index, widget

        audioCountInfo = host.get_audio_port_count_info(self.fPluginId)

        self.fPeaksInputCount  = int(audioCountInfo['ins'])
        self.fPeaksOutputCount = int(audioCountInfo['outs'])

        if self.fPeaksInputCount > 2:
            self.fPeaksInputCount = 2

        if self.fPeaksOutputCount > 2:
            self.fPeaksOutputCount = 2

        # used during testing
        self.fIdleTimerId = 0

        # -------------------------------------------------------------
        # Set-up GUI

        self.fEditDialog = PluginEdit(self, host, self.fPluginId)

        # -------------------------------------------------------------
        # Set-up common widgets (as none)

        self.b_enable = None
        self.b_gui    = None
        self.b_edit   = None
        self.b_remove = None

        self.cb_presets = None

        self.label_name = None
        self.label_type = None

        self.led_control   = None
        self.led_midi      = None
        self.led_audio_in  = None
        self.led_audio_out = None

        self.line = None

        self.peak_in  = None
        self.peak_out = None

        # -------------------------------------------------------------
        # Set-up connections

        host.PluginRenamedCallback.connect(self.slot_handlePluginRenamedCallback)
        host.PluginUnavailableCallback.connect(self.slot_handlePluginUnavailableCallback)
        host.ParameterValueChangedCallback.connect(self.slot_handleParameterValueChangedCallback)
        host.ParameterDefaultChangedCallback.connect(self.slot_handleParameterDefaultChangedCallback)
        host.ParameterMidiChannelChangedCallback.connect(self.slot_handleParameterMidiChannelChangedCallback)
        host.ParameterMidiCcChangedCallback.connect(self.slot_handleParameterMidiCcChangedCallback)
        host.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        host.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        host.OptionChangedCallback.connect(self.slot_handleOptionChangedCallback)
        host.UiStateChangedCallback.connect(self.slot_handleUiStateChangedCallback)

    # -----------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginRenamedCallback(self, pluginId, newName):
        if self.fPluginId == pluginId:
            self.setName(newName)

    @pyqtSlot(int, str)
    def slot_handlePluginUnavailableCallback(self, pluginId, errorMsg):
        if self.fPluginId == pluginId:
            pass

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, index, value):
        if self.fPluginId == pluginId:
            self.setParameterValue(index, value, True)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, index, value):
        if self.fPluginId == pluginId:
            self.setParameterDefault(index, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, index, cc):
        if self.fPluginId == pluginId:
            self.setParameterMidiControl(index, cc)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, index, channel):
        if self.fPluginId == pluginId:
            self.setParameterMidiChannel(index, channel)

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, index):
        if self.fPluginId == pluginId:
            self.setProgram(index, True)


    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, index):
        if self.fPluginId == pluginId:
            self.setMidiProgram(index, True)

    @pyqtSlot(int, int, bool)
    def slot_handleOptionChangedCallback(self, pluginId, option, yesNo):
        if self.fPluginId == pluginId:
            self.setOption(option, yesNo)

    @pyqtSlot(int, int)
    def slot_handleUiStateChangedCallback(self, pluginId, state):
        if self.fPluginId == pluginId:
            self.customUiStateChanged(state)

    #------------------------------------------------------------------

    def ready(self):
        if self.b_enable is not None:
            self.b_enable.setChecked(self.fIsActive)
            self.b_enable.clicked.connect(self.slot_enableClicked)

        if self.b_gui is not None:
            self.b_gui.clicked.connect(self.slot_showCustomUi)
            self.b_gui.setEnabled(bool(self.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI))

        if self.b_edit is None:
            # Edit dialog *must* be available
            self.b_edit = QPushButton(self)
            self.b_edit.setCheckable(True)
            self.b_edit.hide()

        self.b_edit.clicked.connect(self.slot_showEditDialog)

        if self.b_remove is not None:
            self.b_remove.clicked.connect(self.slot_removePlugin)

        if self.label_name is not None:
            self.label_name.setText(self.fPluginInfo['name'])

        if self.label_type is not None:
            self.label_type.setText(getPluginTypeAsString(self.fPluginInfo['type']))

        if self.led_control is not None:
            self.led_control.setColor(self.led_control.YELLOW)
            self.led_control.setEnabled(False)

        if self.led_midi is not None:
            self.led_midi.setColor(self.led_midi.RED)
            self.led_midi.setEnabled(False)

        if self.led_audio_in is not None:
            self.led_audio_in.setColor(self.led_audio_in.GREEN)
            self.led_audio_in.setEnabled(False)

        if self.led_audio_out is not None:
            self.led_audio_out.setColor(self.led_audio_out.BLUE)
            self.led_audio_out.setEnabled(False)

        if self.peak_in is not None:
            self.peak_in.setChannelCount(self.fPeaksInputCount)
            self.peak_in.setMeterColor(DigitalPeakMeter.COLOR_GREEN)
            self.peak_in.setMeterOrientation(DigitalPeakMeter.HORIZONTAL)
            if (self.fPeaksInputCount == 0 and not isinstance(self, PluginSlot_Default)) or self.fIsCollapsed:
                self.peak_in.hide()

        if self.peak_out is not None:
            self.peak_out.setChannelCount(self.fPeaksOutputCount)
            self.peak_out.setMeterColor(DigitalPeakMeter.COLOR_BLUE)
            self.peak_out.setMeterOrientation(DigitalPeakMeter.HORIZONTAL)
            if (self.fPeaksOutputCount == 0 and not isinstance(self, PluginSlot_Default)) or self.fIsCollapsed:
                self.peak_out.hide()

        if self.line is not None:
            if self.fIsCollapsed:
                self.line.hide()

        for paramIndex, paramWidget in self.fParameterList:
            paramWidget.setContextMenuPolicy(Qt.CustomContextMenu)
            paramWidget.customContextMenuRequested.connect(self.slot_knobCustomMenu)
            paramWidget.realValueChanged.connect(self.slot_parameterValueChanged)
            paramWidget.setValue(self.host.get_internal_parameter_value(self.fPluginId, paramIndex))

            if self.fIsCollapsed:
                paramWidget.hide()

        self.setWindowTitle(self.fPluginInfo['name'])

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 32

    def getHints(self):
        return self.fPluginInfo['hints']

    def getPluginId(self):
        return self.fPluginId

    #------------------------------------------------------------------

    def setPluginId(self, idx):
        self.fPluginId = idx
        self.fEditDialog.setPluginId(idx)

    def setName(self, name):
        self.fEditDialog.setName(name)

        if self.label_name is not None:
            self.label_name.setText(name)

    def setSelected(self, yesNo):
        if self.fIsSelected == yesNo:
            return

        self.fIsSelected = yesNo
        self.update()

    #------------------------------------------------------------------

    def setActive(self, active, sendCallback=False, sendHost=True):
        self.fIsActive = active

        if sendCallback:
            self.fParameterIconTimer = ICON_STATE_ON
            self.activeChanged(active)

        if sendHost:
            self.host.set_active(self.fPluginId, active)

        if active:
            self.fEditDialog.clearNotes()
            self.midiActivityChanged(False)

    # called from rack, checks if param is possible first
    def setInternalParameter(self, parameterId, value):
        if parameterId <= PARAMETER_MAX or parameterId >= PARAMETER_NULL:
            return

        elif parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, True)

        elif parameterId == PARAMETER_DRYWET:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET) == 0: return
            self.host.set_drywet(self.fPluginId, value)

        elif parameterId == PARAMETER_VOLUME:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME) == 0: return
            self.host.set_volume(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_LEFT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            self.host.set_balance_left(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_RIGHT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            self.host.set_balance_right(self.fPluginId, value)

        elif parameterId == PARAMETER_PANNING:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_PANNING) == 0: return
            self.host.set_panning(self.fPluginId, value)

        elif parameterId == PARAMETER_CTRL_CHANNEL:
            self.host.set_ctrl_channel(self.fPluginId, value)

        self.fEditDialog.setParameterValue(parameterId, value)

    #------------------------------------------------------------------

    def setParameterValue(self, parameterId, value, sendCallback):
        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, False)

        self.fEditDialog.setParameterValue(parameterId, value)

        if sendCallback:
            self.fParameterIconTimer = ICON_STATE_ON
            self.editDialogParameterValueChanged(self.fPluginId, parameterId, value)

    def setParameterDefault(self, parameterId, value):
        self.fEditDialog.setParameterDefault(parameterId, value)

    def setParameterMidiControl(self, parameterId, control):
        self.fEditDialog.setParameterMidiControl(parameterId, control)

    def setParameterMidiChannel(self, parameterId, channel):
        self.fEditDialog.setParameterMidiChannel(parameterId, channel)

    #------------------------------------------------------------------

    def setProgram(self, index, sendCallback):
        self.fEditDialog.setProgram(index)

        if sendCallback:
            self.fParameterIconTimer = ICON_STATE_ON
            self.editDialogProgramChanged(self.fPluginId, index)

        self.updateParameterValues()

    def setMidiProgram(self, index, sendCallback):
        self.fEditDialog.setMidiProgram(index)

        if sendCallback:
            self.fParameterIconTimer = ICON_STATE_ON
            self.editDialogMidiProgramChanged(self.fPluginId, index)

        self.updateParameterValues()

    #------------------------------------------------------------------

    def setOption(self, option, yesNo):
        self.fEditDialog.setOption(option, yesNo)

    #------------------------------------------------------------------

    def activeChanged(self, onOff):
        self.fIsActive = onOff

        if self.b_enable is None:
            return

        self.b_enable.blockSignals(True)
        self.b_enable.setChecked(onOff)
        self.b_enable.blockSignals(False)

    def customUiStateChanged(self, state):
        if self.b_gui is None:
            return

        self.b_gui.blockSignals(True)
        if state == 0:
            self.b_gui.setChecked(False)
            self.b_gui.setEnabled(True)
        elif state == 1:
            self.b_gui.setChecked(True)
            self.b_gui.setEnabled(True)
        elif state == -1:
            self.b_gui.setChecked(False)
            self.b_gui.setEnabled(False)
        self.b_gui.blockSignals(False)

    def parameterActivityChanged(self, onOff):
        if self.led_control is None:
            return

        self.led_control.setChecked(onOff)

    def midiActivityChanged(self, onOff):
        if self.led_midi is None:
            return

        self.led_midi.setChecked(onOff)

    def optionChanged(self, option, yesNo):
        pass

    # -----------------------------------------------------------------
    # PluginEdit callbacks

    def editDialogVisibilityChanged(self, pluginId, visible):
        if self.b_edit is None:
            return

        self.b_edit.blockSignals(True)
        self.b_edit.setChecked(visible)
        self.b_edit.blockSignals(False)

    def editDialogPluginHintsChanged(self, pluginId, hints):
        self.fPluginInfo['hints'] = hints

        for paramIndex, paramWidget in self.fParameterList:
            if paramIndex == PARAMETER_DRYWET:
                paramWidget.setVisible(hints & PLUGIN_CAN_DRYWET)
            elif paramIndex == PARAMETER_VOLUME:
                paramWidget.setVisible(hints & PLUGIN_CAN_VOLUME)

        if self.b_gui is not None:
            self.b_gui.setEnabled(bool(hints & PLUGIN_HAS_CUSTOM_UI))

    def editDialogParameterValueChanged(self, pluginId, parameterId, value):
        for paramIndex, paramWidget in self.fParameterList:
            if paramIndex != parameterId:
                continue

            paramWidget.blockSignals(True)
            paramWidget.setValue(value)
            paramWidget.blockSignals(False)
            break

    def editDialogProgramChanged(self, pluginId, index):
        if self.cb_presets is None:
            return

        self.cb_presets.blockSignals(True)
        self.cb_presets.setCurrentIndex(index)
        self.cb_presets.blockSignals(False)

        # FIXME
        self.updateParameterValues()

    def editDialogMidiProgramChanged(self, pluginId, index):
        if self.cb_presets is None:
            return

        self.cb_presets.blockSignals(True)
        self.cb_presets.setCurrentIndex(index)
        self.cb_presets.blockSignals(False)

        # FIXME
        self.updateParameterValues()

    def editDialogNotePressed(self, pluginId, note):
        pass

    def editDialogNoteReleased(self, pluginId, note):
        pass

    def editDialogMidiActivityChanged(self, pluginId, onOff):
        self.midiActivityChanged(onOff)

    #------------------------------------------------------------------

    def idleFast(self):
        # Input peaks
        if self.fPeaksInputCount > 0:
            if self.fPeaksInputCount > 1:
                peak1 = self.host.get_input_peak_value(self.fPluginId, True)
                peak2 = self.host.get_input_peak_value(self.fPluginId, False)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                if self.peak_in is not None:
                    self.peak_in.displayMeter(1, peak1)
                    self.peak_in.displayMeter(2, peak2)

            else:
                peak = self.host.get_input_peak_value(self.fPluginId, True)
                ledState = bool(peak != 0.0)

                if self.peak_in is not None:
                    self.peak_in.displayMeter(1, peak)

            if self.fLastGreenLedState != ledState and self.led_audio_in is not None:
                self.fLastGreenLedState = ledState
                self.led_audio_in.setChecked(ledState)

        # Output peaks
        if self.fPeaksOutputCount > 0:
            if self.fPeaksOutputCount > 1:
                peak1 = self.host.get_output_peak_value(self.fPluginId, True)
                peak2 = self.host.get_output_peak_value(self.fPluginId, False)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                if self.peak_out is not None:
                    self.peak_out.displayMeter(1, peak1)
                    self.peak_out.displayMeter(2, peak2)

            else:
                peak = self.host.get_output_peak_value(self.fPluginId, True)
                ledState = bool(peak != 0.0)

                if self.peak_out is not None:
                    self.peak_out.displayMeter(1, peak)

            if self.fLastBlueLedState != ledState and self.led_audio_out is not None:
                self.fLastBlueLedState = ledState
                self.led_audio_out.setChecked(ledState)

    def idleSlow(self):
        if self.fParameterIconTimer == ICON_STATE_ON:
            self.parameterActivityChanged(True)
            self.fParameterIconTimer = ICON_STATE_WAIT

        elif self.fParameterIconTimer == ICON_STATE_WAIT:
            self.fParameterIconTimer = ICON_STATE_OFF

        elif self.fParameterIconTimer == ICON_STATE_OFF:
            self.parameterActivityChanged(False)
            self.fParameterIconTimer = ICON_STATE_NULL

        self.fEditDialog.idleSlow()

    #------------------------------------------------------------------

    def drawOutline(self):
        painter = QPainter(self)

        if self.fIsSelected:
            painter.setPen(QPen(Qt.cyan, 4))
            painter.setBrush(Qt.transparent)
            painter.drawRect(0, 0, self.width(), self.height())
        else:
            painter.setPen(QPen(Qt.black, 1))
            painter.setBrush(Qt.black)
            painter.drawLine(0, self.height()-1, self.width(), self.height()-1)

    def showDefaultCustomMenu(self, isEnabled, bEdit = None, bGui = None):
        menu = QMenu(self)

        actActive = menu.addAction(self.tr("Disable") if isEnabled else self.tr("Enable"))
        menu.addSeparator()

        actReset  = menu.addAction(self.tr("Reset parameters"))
        actRandom = menu.addAction(self.tr("Randomize parameters"))
        menu.addSeparator()

        if bEdit is not None:
            actEdit = menu.addAction(self.tr("Edit"))
            actEdit.setCheckable(True)
            actEdit.setChecked(bEdit.isChecked())
        else:
            actEdit = None

        if bGui is not None:
            actGui = menu.addAction(self.tr("Show Custom UI"))
            actGui.setCheckable(True)
            actGui.setChecked(bGui.isChecked())
            actGui.setEnabled(bGui.isEnabled())
        else:
            actGui = None

        menu.addSeparator()
        actClone   = menu.addAction(self.tr("Clone"))
        actReplace = menu.addAction(self.tr("Replace..."))
        actRename  = menu.addAction(self.tr("Rename..."))
        actRemove  = menu.addAction(self.tr("Remove"))

        if self.fIdleTimerId != 0:
            actRemove.setVisible(False)

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            return

        if actSel == actActive:
            self.setActive(not isEnabled, True, True)

        elif actSel == actReset:
            self.host.reset_parameters(self.fPluginId)

        elif actSel == actRandom:
            self.host.randomize_parameters(self.fPluginId)

        elif actSel == actGui:
            bGui.click()

        elif actSel == actEdit:
            bEdit.click()

        elif actSel == actClone:
            if not self.host.clone_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRename:
            oldName    = self.fPluginInfo['name']
            newNameTry = QInputDialog.getText(self, self.tr("Rename Plugin"), self.tr("New plugin name:"), QLineEdit.Normal, oldName)

            if not (newNameTry[1] and newNameTry[0] and oldName != newNameTry[0]):
                return

            newName = newNameTry[0]

            if self.host.rename_plugin(self.fPluginId, newName):
                self.setName(newName)
            else:
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actReplace:
            gCarla.gui.slot_pluginAdd(self.fPluginId)

        elif actSel == actRemove:
            if not self.host.remove_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    def updateParameterValues(self):
        for paramIndex, paramWidget in self.fParameterList:
            if paramIndex < 0:
                continue

            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramIndex))
            paramWidget.blockSignals(False)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_enableClicked(self, yesNo):
        self.setActive(yesNo, False, True)

    @pyqtSlot()
    def slot_showDefaultCustomMenu(self):
        self.showDefaultCustomMenu(self.fIsActive, self.b_edit, self.b_gui)

    @pyqtSlot()
    def slot_knobCustomMenu(self):
        sender  = self.sender()
        index   = sender.fIndex
        minimum = sender.fMinimum
        maximum = sender.fMaximum
        current = sender.fRealValue
        label   = sender.fLabel

        if index in (PARAMETER_DRYWET, PARAMETER_VOLUME):
            default   = 1.0
            resetText = self.tr("Reset (%i%%)" % int(default*100.0))
            minimText = self.tr("Set to Minimum (%i%%)" % int(minimum*100.0))
            maximText = self.tr("Set to Maximum (%i%%)" % int(maximum*100.0))
        else:
            default   = self.host.get_default_parameter_value(self.fPluginId, index)
            resetText = self.tr("Reset (%f)" % default)
            minimText = self.tr("Set to Minimum (%f)" % minimum)
            maximText = self.tr("Set to Maximum (%f)" % maximum)

        menu = QMenu(self)
        actReset = menu.addAction(resetText)
        menu.addSeparator()
        actMinimum = menu.addAction(minimText)
        actMaximum = menu.addAction(maximText)
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            valueTry = QInputDialog.getDouble(self, self.tr("Set value"), label, current, minimum, maximum, 3) # FIXME - 3 decimals
            if valueTry[1]:
                value = valueTry[0] * 10
            else:
                return

        elif actSelected == actMinimum:
            value = minimum
        elif actSelected == actMaximum:
            value = maximum
        elif actSelected == actReset:
            value = default
        else:
            return

        self.sender().setValue(value)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_showCustomUi(self, show):
        self.host.show_custom_ui(self.fPluginId, show)

    @pyqtSlot(bool)
    def slot_showEditDialog(self, show):
        self.fEditDialog.setVisible(show)

    @pyqtSlot()
    def slot_removePlugin(self):
        if not self.host.remove_plugin(self.fPluginId):
            CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                    self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_parameterValueChanged(self, value):
        index = self.sender().getIndex()

        if index < 0:
            self.setInternalParameter(index, value)
        else:
            self.host.set_parameter_value(self.fPluginId, index, value)
            self.setParameterValue(index, value, False)

    @pyqtSlot(int)
    def slot_programChanged(self, index):
        self.host.set_program(self.fPluginId, index)
        self.setProgram(index, False)

    @pyqtSlot(int)
    def slot_midiProgramChanged(self, index):
        self.host.set_midi_program(self.fPluginId, index)
        self.setMidiProgram(index, False)

    #------------------------------------------------------------------

    def testTimer(self):
        self.fIdleTimerId = self.startTimer(25)

    #------------------------------------------------------------------

    def closeEvent(self, event):
        if self.fIdleTimerId != 0:
            self.killTimer(self.fIdleTimerId)
            self.fIdleTimerId = 0

            self.host.engine_close()

        QFrame.closeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerId:
            self.host.engine_idle()
            self.idleFast()
            self.idleSlow()

        QFrame.timerEvent(self, event)

    def paintEvent(self, event):
        self.drawOutline()
        QFrame.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Default(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_default.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fColorTop    = QColor(60, 60, 60)
        self.fColorBottom = QColor(47, 47, 47)
        self.fColorSeprtr = QColor(70, 70, 70)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
            QLabel#label_name {
                color: #BBB;
            }
        """)

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        if self.fPluginInfo['iconName'] == "distrho":
            self.ui.b_gui.setPixmaps(":/bitmaps/button_distrho.png", ":/bitmaps/button_distrho_down.png", ":/bitmaps/button_distrho_hover.png")
        elif self.fPluginInfo['iconName'] == "file":
            self.ui.b_gui.setPixmaps(":/bitmaps/button_file.png", ":/bitmaps/button_file_down.png", ":/bitmaps/button_file_hover.png")
        else:
            self.ui.b_gui.setPixmaps(":/bitmaps/button_gui.png", ":/bitmaps/button_gui_down.png", ":/bitmaps/button_gui_hover.png")

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit

        self.label_name    = self.ui.label_name
        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_in  = self.ui.led_audio_in
        self.led_audio_out = self.ui.led_audio_out

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 36

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.save()

        areaX  = self.ui.area_right.x()+7
        width  = self.width()
        height = self.height()

        painter.setPen(QPen(QColor(17, 17, 17), 1))
        painter.setBrush(QColor(17, 17, 17))
        painter.drawRect(0, 0, width, height)

        painter.setPen(self.fColorSeprtr.lighter(110))
        painter.setBrush(self.fColorBottom)
        painter.setRenderHint(QPainter.Antialiasing, True)

        # name -> leds arc
        path = QPainterPath()
        path.moveTo(areaX-20, height-4)
        path.cubicTo(areaX, height-5, areaX-20, 4.75, areaX, 4.75)
        path.lineTo(areaX, height-5)
        painter.drawPath(path)

        painter.setPen(self.fColorSeprtr)
        painter.setRenderHint(QPainter.Antialiasing, False)

        # separator lines
        painter.drawLine(0, height-5, areaX-20, height-5)
        painter.drawLine(areaX, 4, width, 4)

        painter.setPen(self.fColorBottom)
        painter.setBrush(self.fColorBottom)

        # top, bottom and left lines
        painter.drawLine(0, 0, width, 0)
        painter.drawRect(0, height-4, areaX, 4)
        painter.drawRoundedRect(areaX-20, height-5, areaX, 5, 22, 22)
        painter.drawLine(0, 0, 0, height)

        # fill the rest
        painter.drawRect(areaX-1, 5, width, height)

        # bottom 1px line
        painter.setPen(self.fColorSeprtr)
        painter.drawLine(0, height-1, width, height-1)

        painter.restore()
        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_BasicFX(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_basic_fx.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        labelFont = self.ui.label_name.font()
        labelFont.setBold(True)
        labelFont.setPointSize(9)
        self.ui.label_name.setFont(labelFont)

        r = 40
        g = 40
        b = 40

        if self.fPluginInfo['category'] == PLUGIN_CATEGORY_MODULATOR:
            r += 10
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_EQ:
            g += 10
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_FILTER:
            b += 10
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_DELAY:
            r += 15
            b -= 15
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_DISTORTION:
            g += 10
            b += 10
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_DYNAMICS:
            r += 10
            b += 10
        elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_UTILITY:
            r += 10
            g += 10

        bg = "noise1"

        if self.fPluginInfo['maker'] in ("falkTX, Michael Gruhn", "DISTRHO") and "3bandeq" in self.fPluginInfo['label'].lower():
            bg = "3bandeq"

        self.setStyleSheet("""
            PluginSlot_BasicFX#PluginWidget {
                background-color: rgb(%i, %i, %i);
                background-image: url(:/bitmaps/background_%s.png);
                background-repeat: repeat-xy;
            }
            QLabel#label_name {
                color: #BBB;
            }
        """ % (r, g, b, bg))

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        if self.fPluginInfo['iconName'] == "distrho":
            self.ui.b_gui.setPixmaps(":/bitmaps/button_distrho.png", ":/bitmaps/button_distrho_down.png", ":/bitmaps/button_distrho_hover.png")
        elif self.fPluginInfo['iconName'] == "file":
            self.ui.b_gui.setPixmaps(":/bitmaps/button_file.png", ":/bitmaps/button_file_down.png", ":/bitmaps/button_file_hover.png")
        else:
            self.ui.b_gui.setPixmaps(":/bitmaps/button_gui.png", ":/bitmaps/button_gui_down.png", ":/bitmaps/button_gui_hover.png")

        #if self.fIsCollapsed:
            #self.ui.w_knobs.hide()
            #self.ui.horizontalLayout_2.setContentsMargins(0,0,0,0)
            #self.ui.horizontalLayout_2.setSpacing(0)
            #self.ui.horizontalLayout_2.SetMaximumSize(0,0)

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = self.host.get_parameter_count(self.fPluginId) if not self.fIsCollapsed else 0

        index = 0
        for i in range(parameterCount):
            if index >= 8:
                break

            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = getParameterShortName(paramInfo['name'])

            #if self.fPluginInfo['category'] == PLUGIN_CATEGORY_FILTER:
                #_r =  55 + float(i)/8*200
                #_g = 255 - float(i)/8*200
                #_b = 127 - r*2
            #elif self.fPluginInfo['category'] == PLUGIN_CATEGORY_DELAY:
                #_r = 127
                #_g =  55 + float(i)/8*200
                #_b = 255 - float(i)/8*200
            #elif r < b < g:
                #_r =  55 + float(i)/8*200
                #_g = 127
                #_b = 255 - float(i)/8*200
            #else:
            _r = 255 - float(index)/8*200
            _g =  55 + float(index)/8*200
            _b =  (r-40)*4

            #if _r < 140: _r = 140
            #if _g < 140: _g = 140
            #if _b < 140: _b = 140

            widget = PixmapDial(self, i)
            widget.setPixmap(3)
            widget.setLabel(paramName)
            widget.setCustomPaintColor(QColor(_r, _g, _b))
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_COLOR)
            widget.forceWhiteLabelGradientText()
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.ui.w_knobs.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        self.ui.dial_drywet.setIndex(PARAMETER_DRYWET)
        self.ui.dial_drywet.setPixmap(3)
        self.ui.dial_drywet.setLabel("Dry/Wet")
        self.ui.dial_drywet.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_WET)
        self.ui.dial_drywet.setMinimum(0.0)
        self.ui.dial_drywet.setMaximum(1.0)
        self.ui.dial_drywet.forceWhiteLabelGradientText()
        self.ui.dial_drywet.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET)

        self.ui.dial_vol.setIndex(PARAMETER_VOLUME)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_VOL)
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.forceWhiteLabelGradientText()
        self.ui.dial_vol.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME)

        self.fParameterList.append([PARAMETER_DRYWET, self.ui.dial_drywet])
        self.fParameterList.append([PARAMETER_VOLUME, self.ui.dial_vol])

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit

        self.label_name = self.ui.label_name

        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_in  = self.ui.led_audio_in
        self.led_audio_out = self.ui.led_audio_out

        self.line = self.ui.line

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 28 if self.fIsCollapsed else 79

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(42, 42, 42), 1))
        painter.drawRect(0, 1, self.width()-1, 79-3)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Calf(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_calf.Ui_PluginWidget()
        self.ui.setupUi(self)

        # FIXME
        self.fIsCollapsed = False

        audioCount = self.host.get_audio_port_count_info(self.fPluginId)
        midiCount  = self.host.get_midi_port_count_info(self.fPluginId)

        # -------------------------------------------------------------
        # Internal stuff

        self.fButtonFont = self.ui.b_gui.font()
        self.fButtonFont.setBold(False)
        self.fButtonFont.setPointSize(8)

        # Use black for mono plugins
        self.fBackgroundBlack = audioCount['ins'] == 1

        self.fButtonColorOn  = QColor( 18,  41,  87)
        self.fButtonColorOff = QColor(150, 150, 150)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
            QLabel#label_name, QLabel#label_audio_in, QLabel#label_audio_out, QLabel#label_midi {
                color: #BBB;
            }
            PluginSlot_Calf#PluginWidget {
                background-image: url(:/bitmaps/background_calf_%s.png);
                background-repeat: repeat-xy;
                border: 2px;
            }
        """ % ("black" if self.fBackgroundBlack else "blue"))

        self.ui.b_gui.setPixmaps(":/bitmaps/button_calf2.png", ":/bitmaps/button_calf2_down.png", ":/bitmaps/button_calf2_hover.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_calf2.png", ":/bitmaps/button_calf2_down.png", ":/bitmaps/button_calf2_hover.png")
        self.ui.b_remove.setPixmaps(":/bitmaps/button_calf1.png", ":/bitmaps/button_calf1_down.png", ":/bitmaps/button_calf1_hover.png")

        self.ui.b_edit.setTopText(self.tr("Edit"), self.fButtonColorOn, self.fButtonFont)
        self.ui.b_remove.setTopText(self.tr("Remove"), self.fButtonColorOn, self.fButtonFont)

        if self.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOn, self.fButtonFont)
        else:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOff, self.fButtonFont)

        labelFont = self.ui.label_name.font()
        labelFont.setBold(True)
        labelFont.setPointSize(10)
        self.ui.label_name.setFont(labelFont)

        if audioCount['ins'] == 0:
            self.ui.label_audio_in.hide()
            self.ui.peak_in.hide()

            if audioCount['outs'] > 0:
                self.ui.peak_out.setMinimumWidth(200)

        if audioCount['outs'] == 0:
            self.ui.label_audio_out.hide()
            self.ui.peak_out.hide()

        if midiCount['ins'] == 0:
            self.ui.label_midi.hide()
            self.ui.led_midi.hide()

        if self.fIdleTimerId != 0:
            self.ui.b_remove.setEnabled(False)
            self.ui.b_remove.setVisible(False)

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = self.host.get_parameter_count(self.fPluginId) if not self.fIsCollapsed else 0

        index = 0
        limit = 7 if midiCount['ins'] == 0 else 6
        for i in range(parameterCount):
            if index >= limit:
                break

            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = getParameterShortName(paramInfo['name'])

            widget = PixmapDial(self, i)
            widget.setPixmap(7)
            widget.setLabel(paramName)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.ui.w_knobs.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        # -------------------------------------------------------------

        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit
        self.b_remove = self.ui.b_remove

        self.label_name = self.ui.label_name
        self.led_midi   = self.ui.led_midi

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.ui.led_midi.setColor(self.ui.led_midi.CALF)

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 88

    #------------------------------------------------------------------

    def editDialogPluginHintsChanged(self, pluginId, hints):
        if hints & PLUGIN_HAS_CUSTOM_UI:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOn, self.fButtonFont)
        else:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOff, self.fButtonFont)

        AbstractPluginSlot.editDialogPluginHintsChanged(self, pluginId, hints)

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(20, 20, 20) if self.fBackgroundBlack else QColor(75, 86, 99), 1))
        painter.drawRect(0, 1, self.width()-1, 88-3)

        painter.setPen(QPen(QColor(45, 45, 45) if self.fBackgroundBlack else QColor(86, 99, 114), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_OpenAV(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_basic_fx.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        QFontDatabase.addApplicationFont(":/fonts/uranium.ttf")

        labelFont = QFont()
        labelFont.setFamily("Uranium")
        labelFont.setPointSize(13)
        labelFont.setCapitalization(QFont.AllUppercase)
        self.ui.label_name.setFont(labelFont)

        self.setStyleSheet("""
            PluginSlot_OpenAV#PluginWidget {
                background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                                  stop: 0 #303030, stop: 0.35 #111111, stop: 1.0 #111111);
            }
        """)

        self.ui.label_name.setStyleSheet("* { color: #FF5100; }")
        #self.ui.line.setStyleSheet("* { background-color: #FF5100; color: #FF5100; }")

        self.ui.peak_in.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)
        self.ui.peak_out.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")
        self.ui.b_gui.setPixmaps(":/bitmaps/button_gui.png", ":/bitmaps/button_gui_down.png", ":/bitmaps/button_gui_hover.png")

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = self.host.get_parameter_count(self.fPluginId) if not self.fIsCollapsed else 0

        index = 0
        for i in range(parameterCount):
            if index >= 8:
                break

            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = getParameterShortName(paramInfo['name'])

            widget = PixmapDial(self, i)
            widget.setPixmap(11)
            widget.setLabel(paramName)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.ui.w_knobs.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        self.ui.dial_drywet.setIndex(PARAMETER_DRYWET)
        self.ui.dial_drywet.setPixmap(13)
        self.ui.dial_drywet.setLabel("Dry/Wet")
        self.ui.dial_drywet.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        self.ui.dial_drywet.setMinimum(0.0)
        self.ui.dial_drywet.setMaximum(1.0)
        self.ui.dial_drywet.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET)

        self.ui.dial_vol.setIndex(PARAMETER_VOLUME)
        self.ui.dial_vol.setPixmap(12)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME)

        self.fParameterList.append([PARAMETER_DRYWET, self.ui.dial_drywet])
        self.fParameterList.append([PARAMETER_VOLUME, self.ui.dial_vol])

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit

        self.label_name = self.ui.label_name

        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_in  = self.ui.led_audio_in
        self.led_audio_out = self.ui.led_audio_out

        self.line = self.ui.line

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 28 if self.fIsCollapsed else 79

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(42, 42, 42), 1))
        painter.drawRect(0, 1, self.width()-1, 79-3)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Nekobi(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        #self.ui = ui_carla_plugin_basic_fx.Ui_PluginWidget()
        #self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        self.fPixmapCenter = QPixmap(":/bitmaps/background_nekobi.png")

        self.fPixmapLeft     = QPixmap(":/bitmaps/background_nekobi_left.png")
        self.fPixmapLeftRect = QRectF(0, 0, self.fPixmapLeft.width(), self.fPixmapLeft.height())

        self.fPixmapRight     = QPixmap(":/bitmaps/background_nekobi_right.png")
        self.fPixmapRightRect = QRectF(0, 0, self.fPixmapRight.width(), self.fPixmapRight.height())

        #self.setStyleSheet("""
            #PluginSlot_Nekobi#PluginWidget {
                #background-image: url(:/bitmaps/background_nekobi.png);
                #background-repeat: repeat-xy;
            #}
            #QLabel#label_name {
                #color: #BBB;
            #}
        #""")

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 108

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)

        # main bg (center)
        painter.drawTiledPixmap(0, 0, self.width(), self.height(), self.fPixmapCenter)

        # left side
        painter.drawPixmap(self.fPixmapLeftRect, self.fPixmapLeft, self.fPixmapLeftRect)

        # right side
        rightTarget = QRectF(self.fPixmapRightRect)
        rightTarget.moveLeft(self.width()-rightTarget.width())
        painter.drawPixmap(rightTarget, self.fPixmapRight, self.fPixmapRightRect)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_SF2(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_sf2.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        #labelFont = self.ui.label_name.font()
        #labelFont.setBold(True)
        #labelFont.setPointSize(9)
        #self.ui.label_name.setFont(labelFont)

        self.setStyleSheet("""
            PluginSlot_SF2#PluginWidget {
                background-image: url(:/bitmaps/background_3bandeq.png);
                background-repeat: repeat-xy;
            }
            QLabel#label_name {
                color: #BBB;
            }
        """)

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = self.host.get_parameter_count(self.fPluginId) if not self.fIsCollapsed else 0

        index = 0
        for i in range(parameterCount):
            if index >= 8:
                break

            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = getParameterShortName(paramInfo['name'])

            widget = PixmapDial(self, i)
            widget.setPixmap(3)
            widget.setLabel(paramName)
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.ui.w_knobs.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        self.ui.dial_vol.setIndex(PARAMETER_VOLUME)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_VOL)
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.forceWhiteLabelGradientText()
        self.ui.dial_vol.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME)

        self.fParameterList.append([PARAMETER_VOLUME, self.ui.dial_vol])

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_edit   = self.ui.b_edit

        self.label_name = self.ui.label_name

        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_out = self.ui.led_audio_out

        self.line = self.ui.line

        self.peak_out = self.ui.peak_out

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 79

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(42, 42, 42), 1))
        painter.drawRect(0, 1, self.width()-1, 79-3)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_ZynFX(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId)
        self.ui = ui_carla_plugin_zynfx.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
            PluginSlot_ZynFX#PluginWidget {
                background-image: url(:/bitmaps/background_zynfx.png);
                background-repeat: repeat-xy;
                border: 2px;
            }
            QLabel#label_name, QLabel#label_presets {
                color: #BBB;
            }
        """)

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        labelFont = self.ui.label_name.font()
        labelFont.setBold(True)
        labelFont.setPointSize(9)
        self.ui.label_name.setFont(labelFont)

        presetFont = self.ui.label_presets.font()
        presetFont.setBold(True)
        presetFont.setPointSize(8)
        self.ui.label_presets.setFont(presetFont)

        self.ui.peak_in.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)
        self.ui.peak_out.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = self.host.get_parameter_count(self.fPluginId) if not self.fIsCollapsed else 0

        index = 0
        for i in range(parameterCount):
            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = paramInfo['name']

            # real zyn fx plugins
            if self.fPluginInfo['label'] == "zynalienwah":
                if   i == 0: paramName = "Freq"
                elif i == 1: paramName = "Rnd"
                elif i == 2: paramName = "L type" # combobox
                elif i == 3: paramName = "St.df"
                elif i == 5: paramName = "Fb"
                elif i == 7: paramName = "L/R"

            elif self.fPluginInfo['label'] == "zynchorus":
                if   i == 0: paramName = "Freq"
                elif i == 1: paramName = "Rnd"
                elif i == 2: paramName = "L type" # combobox
                elif i == 3: paramName = "St.df"
                elif i == 6: paramName = "Fb"
                elif i == 7: paramName = "L/R"
                elif i == 8: paramName = "Flngr" # button
                elif i == 9: paramName = "Subst" # button

            elif self.fPluginInfo['label'] == "zyndistortion":
                if   i == 0: paramName = "LRc."
                elif i == 4: paramName = "Neg." # button
                elif i == 5: paramName = "LPF"
                elif i == 6: paramName = "HPF"
                elif i == 7: paramName = "St." # button
                elif i == 8: paramName = "PF"  # button

            elif self.fPluginInfo['label'] == "zyndynamicfilter":
                if   i == 0: paramName = "Freq"
                elif i == 1: paramName = "Rnd"
                elif i == 2: paramName = "L type" # combobox
                elif i == 3: paramName = "St.df"
                elif i == 4: paramName = "LfoD"
                elif i == 5: paramName = "A.S."
                elif i == 6: paramName = "A.Inv." # button
                elif i == 7: paramName = "A.M."

            elif self.fPluginInfo['label'] == "zynecho":
                if   i == 1: paramName = "LRdl."
                elif i == 2: paramName = "LRc."
                elif i == 3: paramName = "Fb."
                elif i == 4: paramName = "Damp"

            elif self.fPluginInfo['label'] == "zynphaser":
                if   i ==  0: paramName = "Freq"
                elif i ==  1: paramName = "Rnd"
                elif i ==  2: paramName = "L type" # combobox
                elif i ==  3: paramName = "St.df"
                elif i ==  5: paramName = "Fb"
                elif i ==  7: paramName = "L/R"
                elif i ==  8: paramName = "Subst" # button
                elif i ==  9: paramName = "Phase"
                elif i == 11: paramName = "Dist"

            elif self.fPluginInfo['label'] == "zynreverb":
                if   i ==  2: paramName = "I.delfb"
                elif i ==  5: paramName = "LPF"
                elif i ==  6: paramName = "HPF"
                elif i ==  9: paramName = "R.S."
                elif i == 10: paramName = "I.del"

            else:
                paramName = getParameterShortName(paramInfo['name'])

            widget = PixmapDial(self, i)

            widget.setPixmap(5)
            widget.setLabel(paramName)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.ui.w_knobs.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        self.ui.dial_drywet.setIndex(PARAMETER_DRYWET)
        self.ui.dial_drywet.setPixmap(5)
        self.ui.dial_drywet.setLabel("Wet")
        self.ui.dial_drywet.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        self.ui.dial_drywet.setMinimum(0.0)
        self.ui.dial_drywet.setMaximum(1.0)
        self.ui.dial_drywet.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET)

        self.ui.dial_vol.setIndex(PARAMETER_VOLUME)
        self.ui.dial_vol.setPixmap(5)
        self.ui.dial_vol.setLabel("Vol")
        self.ui.dial_vol.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.setVisible(self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME)

        self.fParameterList.append([PARAMETER_DRYWET, self.ui.dial_drywet])
        self.fParameterList.append([PARAMETER_VOLUME, self.ui.dial_vol])

        # -------------------------------------------------------------
        # Set-up MIDI programs

        midiProgramCount = self.host.get_midi_program_count(self.fPluginId)

        if midiProgramCount > 0:
            self.ui.cb_presets.setEnabled(True)
            self.ui.label_presets.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = self.host.get_midi_program_data(self.fPluginId, i)
                mpName = mpData['name']

                self.ui.cb_presets.addItem(mpName)

            self.fCurrentMidiProgram = self.host.get_current_midi_program_index(self.fPluginId)
            self.ui.cb_presets.setCurrentIndex(self.fCurrentMidiProgram)

        else:
            self.fCurrentMidiProgram = -1
            self.ui.cb_presets.setEnabled(False)
            self.ui.cb_presets.setVisible(False)
            self.ui.label_presets.setEnabled(False)
            self.ui.label_presets.setVisible(False)

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_edit   = self.ui.b_edit

        self.cb_presets = self.ui.cb_presets

        self.label_name = self.ui.label_name

        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_in  = self.ui.led_audio_in
        self.led_audio_out = self.ui.led_audio_out

        self.line = self.ui.line

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)
        self.ui.cb_presets.currentIndexChanged.connect(self.slot_midiProgramChanged)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 30 if self.fIsCollapsed else 79

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawRect(0, 1, self.width()-1, self.height()-3)

        painter.setPen(QPen(QColor(94, 94, 94), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

def createPluginSlot(parent, host, pluginId, useSkins):
    if not useSkins:
        return PluginSlot_Default(parent, host, pluginId)

    pluginInfo  = host.get_plugin_info(pluginId)
    pluginName  = host.get_real_plugin_name(pluginId)
    pluginLabel = pluginInfo['label']
    pluginMaker = pluginInfo['maker']
    uniqueId    = pluginInfo['uniqueId']

    #pluginIcon  = pluginInfo['iconName']

    if pluginInfo['type'] == PLUGIN_SF2:
        return PluginSlot_SF2(parent, host, pluginId)

    if pluginMaker == "OpenAV Productions":
        return PluginSlot_OpenAV(parent, host, pluginId)

    if pluginInfo['type'] == PLUGIN_INTERNAL:
        if pluginLabel.startswith("zyn") and pluginInfo['category'] != PLUGIN_CATEGORY_SYNTH:
            return PluginSlot_ZynFX(parent, host, pluginId)

    elif pluginInfo['type'] == PLUGIN_LADSPA:
        if pluginLabel.startswith("Zyn") and pluginMaker.startswith("Josep Andreu"):
            return PluginSlot_ZynFX(parent, host, pluginId)

    if pluginName.split(" ", 1)[0].lower() == "calf":
        return PluginSlot_Calf(parent, host, pluginId)

    #if pluginName.lower() == "nekobi":
        #return PluginSlot_Nekobi(parent, pluginId)

    return PluginSlot_BasicFX(parent, host, pluginId)

# ------------------------------------------------------------------------------------------------------------
# Main Testing

if __name__ == '__main__':
    from carla_app import CarlaApplication
    from carla_host import initHost, loadHostSettings
    import resources_rc

    app = CarlaApplication("Carla-Skins")
    host = initHost("Skins", None, False, False, False)
    loadHostSettings(host)

    host.engine_init("JACK", "Carla-Widgets")
    host.add_plugin(BINARY_NATIVE, PLUGIN_INTERNAL, "", "", "zynreverb", 0, None, 0x0)
    #host.add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/karplong.so", "karplong", "karplong", 0, None, 0x0)
    #host.add_plugin(BINARY_NATIVE, PLUGIN_LV2, "", "", "http://www.openavproductions.com/sorcer", 0, None, 0x0)
    #host.add_plugin(BINARY_NATIVE, PLUGIN_LV2, "", "", "http://calf.sourceforge.net/plugins/Compressor", 0, None, 0x0)
    host.set_active(0, True)

    gui = createPluginSlot(None, host, 0, True)
    gui.testTimer()
    gui.show()

    app.exec_()
