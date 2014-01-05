#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin/slot skin code
# Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

from PyQt4.QtGui import QFrame

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_plugin_default
import ui_carla_plugin_zynfx

from carla_widgets import *
from pixmapdial import PixmapDial

# ------------------------------------------------------------------------------------------------------------

class PluginSlot(QFrame):
    def __init__(self, parent, pluginId):
        QFrame.__init__(self, parent)

        # -------------------------------------------------------------
        # Internal stuff

        self.fPluginId   = pluginId
        self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId) if Carla.host is not None else gFakePluginInfo

        self.fPluginInfo['filename']  = charPtrToString(self.fPluginInfo['filename'])
        self.fPluginInfo['name']      = charPtrToString(self.fPluginInfo['name'])
        self.fPluginInfo['label']     = charPtrToString(self.fPluginInfo['label'])
        self.fPluginInfo['maker']     = charPtrToString(self.fPluginInfo['maker'])
        self.fPluginInfo['copyright'] = charPtrToString(self.fPluginInfo['copyright'])
        self.fPluginInfo['iconName']  = charPtrToString(self.fPluginInfo['iconName'])

        self.fParameterIconTimer = ICON_STATE_NULL

        if not Carla.isLocal:
            self.fPluginInfo['hints'] &= ~PLUGIN_HAS_CUSTOM_UI

        if Carla.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK or Carla.host is None:
            self.fPeaksInputCount  = 2
            self.fPeaksOutputCount = 2

        else:
            audioCountInfo = Carla.host.get_audio_port_count_info(self.fPluginId)

            self.fPeaksInputCount  = int(audioCountInfo['ins'])
            self.fPeaksOutputCount = int(audioCountInfo['outs'])

            if self.fPeaksInputCount > 2:
                self.fPeaksInputCount = 2

            if self.fPeaksOutputCount > 2:
                self.fPeaksOutputCount = 2

        # -------------------------------------------------------------
        # Set-up GUI

        self.fEditDialog = PluginEdit(self, self.fPluginId)
        self.fEditDialog.hide()

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 32

    def getHints(self):
        return self.fPluginInfo['hints']

    #------------------------------------------------------------------

    def recheckPluginHints(self, hints):
        self.fPluginInfo['hints'] = hints

    def setId(self, idx):
        self.fPluginId = idx
        self.fEditDialog.setId(idx)

    def setName(self, name):
        self.fEditDialog.setName(name)

    #------------------------------------------------------------------

    def setActive(self, active, sendGui=False, sendCallback=True):
        if sendGui:      self.activeChanged(active)
        if sendCallback: Carla.host.set_active(self.fPluginId, active)

        if active:
            self.fEditDialog.clearNotes()
            self.midiActivityChanged(False)

    # called from rack, checks if param is possible first
    def setInternalParameter(self, parameterId, value):
        if parameterId <= PARAMETER_MAX or parameterId >= PARAMETER_NULL:
            return

        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, True)

        elif parameterId == PARAMETER_DRYWET:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET) == 0: return
            Carla.host.set_drywet(self.fPluginId, value)

        elif parameterId == PARAMETER_VOLUME:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME) == 0: return
            Carla.host.set_volume(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_LEFT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            Carla.host.set_balance_left(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_RIGHT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            Carla.host.set_balance_right(self.fPluginId, value)

        elif parameterId == PARAMETER_PANNING:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_PANNING) == 0: return
            Carla.host.set_panning(self.fPluginId, value)

        elif parameterId == PARAMETER_CTRL_CHANNEL:
            Carla.host.set_ctrl_channel(self.fPluginId, value)

        self.fEditDialog.setParameterValue(parameterId, value)

    #------------------------------------------------------------------

    def setParameterValue(self, parameterId, value):
        self.fParameterIconTimer = ICON_STATE_ON

        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, False)

        self.fEditDialog.setParameterValue(parameterId, value)

    def setParameterDefault(self, parameterId, value):
        self.fEditDialog.setParameterDefault(parameterId, value)

    def setParameterMidiControl(self, parameterId, control):
        self.fEditDialog.setParameterMidiControl(parameterId, control)

    def setParameterMidiChannel(self, parameterId, channel):
        self.fEditDialog.setParameterMidiChannel(parameterId, channel)

    #------------------------------------------------------------------

    def setProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.fEditDialog.setProgram(index)

    def setMidiProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.fEditDialog.setMidiProgram(index)

    #------------------------------------------------------------------

    def sendNoteOn(self, channel, note):
        if self.fEditDialog.sendNoteOn(channel, note):
            self.midiActivityChanged(True)

    def sendNoteOff(self, channel, note):
        if self.fEditDialog.sendNoteOff(channel, note):
            self.midiActivityChanged(False)

    #------------------------------------------------------------------

    def activeChanged(self, onOff):
        pass

    def editDialogChanged(self, visible):
        pass

    def customUiStateChanged(self, state):
        pass

    def parameterActivityChanged(self, onOff):
        pass

    def midiActivityChanged(self, onOff):
        pass

    #------------------------------------------------------------------

    def idleFast(self):
        pass

    def idleSlow(self):
        if self.fParameterIconTimer == ICON_STATE_ON:
            self.fParameterIconTimer = ICON_STATE_WAIT
            self.parameterActivityChanged(True)
        elif self.fParameterIconTimer == ICON_STATE_WAIT:
            self.fParameterIconTimer = ICON_STATE_OFF
        elif self.fParameterIconTimer == ICON_STATE_OFF:
            self.fParameterIconTimer = ICON_STATE_NULL
            self.parameterActivityChanged(False)

        self.fEditDialog.idleSlow()

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_showCustomUi(self, show):
        Carla.host.show_custom_ui(self.fPluginId, show)

    @pyqtSlot(bool)
    def slot_showEditDialog(self, show):
        self.fEditDialog.setVisible(show)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Default(PluginSlot):
    def __init__(self, parent, pluginId):
        PluginSlot.__init__(self, parent, pluginId)
        self.ui = ui_carla_plugin_default.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        if self.palette().window().color().lightness() > 100:
            # Light background
            labelColor = "333"
            isLight = True

            self.fColorTop    = QColor(60, 60, 60)
            self.fColorBottom = QColor(47, 47, 47)
            self.fColorSeprtr = QColor(70, 70, 70)

        else:
            # Dark background
            labelColor = "BBB"
            isLight = False

            self.fColorTop    = QColor(60, 60, 60)
            self.fColorBottom = QColor(47, 47, 47)
            self.fColorSeprtr = QColor(70, 70, 70)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
        QLabel#label_name {
            color: #%s;
        }""" % labelColor)

        if isLight:
            self.ui.b_enable.setPixmaps(":/bitmaps/button_off2.png", ":/bitmaps/button_on2.png", ":/bitmaps/button_off2.png")
            self.ui.b_edit.setPixmaps(":/bitmaps/button_edit2.png", ":/bitmaps/button_edit_down2.png", ":/bitmaps/button_edit_hover2.png")

            if self.fPluginInfo['iconName'] == "distrho":
                self.ui.b_gui.setPixmaps(":/bitmaps/button_distrho2.png", ":/bitmaps/button_distrho_down2.png", ":/bitmaps/button_distrho_hover2.png")
            elif self.fPluginInfo['iconName'] == "file":
                self.ui.b_gui.setPixmaps(":/bitmaps/button_file2.png", ":/bitmaps/button_file_down2.png", ":/bitmaps/button_file_hover2.png")
            else:
                self.ui.b_gui.setPixmaps(":/bitmaps/button_gui2.png", ":/bitmaps/button_gui_down2.png", ":/bitmaps/button_gui_hover2.png")
        else:
            self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
            self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

            if self.fPluginInfo['iconName'] == "distrho":
                self.ui.b_gui.setPixmaps(":/bitmaps/button_distrho.png", ":/bitmaps/button_distrho_down.png", ":/bitmaps/button_distrho_hover.png")
            elif self.fPluginInfo['iconName'] == "file":
                self.ui.b_gui.setPixmaps(":/bitmaps/button_file.png", ":/bitmaps/button_file_down.png", ":/bitmaps/button_file_hover.png")
            else:
                self.ui.b_gui.setPixmaps(":/bitmaps/button_gui.png", ":/bitmaps/button_gui_down.png", ":/bitmaps/button_gui_hover.png")

        self.ui.b_gui.setEnabled((self.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI) != 0)

        self.ui.led_control.setColor(self.ui.led_control.YELLOW)
        self.ui.led_control.setEnabled(False)

        self.ui.led_midi.setColor(self.ui.led_midi.RED)
        self.ui.led_midi.setEnabled(False)

        self.ui.led_audio_in.setColor(self.ui.led_audio_in.GREEN)
        self.ui.led_audio_in.setEnabled(False)

        self.ui.led_audio_out.setColor(self.ui.led_audio_out.BLUE)
        self.ui.led_audio_out.setEnabled(False)

        self.ui.peak_in.setColor(self.ui.peak_in.GREEN)
        self.ui.peak_in.setChannels(self.fPeaksInputCount)
        self.ui.peak_in.setOrientation(self.ui.peak_in.HORIZONTAL)

        self.ui.peak_out.setColor(self.ui.peak_in.BLUE)
        self.ui.peak_out.setChannels(self.fPeaksOutputCount)
        self.ui.peak_out.setOrientation(self.ui.peak_out.HORIZONTAL)

        self.ui.label_name.setText(self.fPluginInfo['name'])

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.b_enable.clicked.connect(self.slot_enableClicked)
        self.ui.b_gui.clicked.connect(self.slot_showCustomUi)
        self.ui.b_edit.clicked.connect(self.slot_showEditDialog)

        self.customContextMenuRequested.connect(self.slot_showCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 32

    #------------------------------------------------------------------

    def recheckPluginHints(self, hints):
        self.ui.b_gui.setEnabled(hints & PLUGIN_HAS_CUSTOM_UI)
        PluginSlot.recheckPluginHints(self, hints)

    def setName(self, name):
        self.ui.label_name.setText(name)
        PluginSlot.setName(self, name)

    #------------------------------------------------------------------

    def activeChanged(self, onOff):
        self.ui.b_enable.setChecked(onOff)

    def editDialogChanged(self, visible):
        self.ui.b_edit.blockSignals(True)
        self.ui.b_edit.setChecked(visible)
        self.ui.b_edit.blockSignals(False)

    def customUiStateChanged(self, state):
        self.ui.b_gui.blockSignals(True)
        if state == 0:
            self.ui.b_gui.setChecked(False)
            self.ui.b_gui.setEnabled(True)
        elif state == 1:
            self.ui.b_gui.setChecked(True)
            self.ui.b_gui.setEnabled(True)
        elif state == -1:
            self.ui.b_gui.setChecked(False)
            self.ui.b_gui.setEnabled(False)
        self.ui.b_gui.blockSignals(False)

    def parameterActivityChanged(self, onOff):
        self.ui.led_control.setChecked(onOff)

    def midiActivityChanged(self, onOff):
        self.ui.led_midi.setChecked(onOff)

    #------------------------------------------------------------------

    def idleFast(self):
        # Input peaks
        if self.fPeaksInputCount > 0:
            if self.fPeaksInputCount > 1:
                peak1 = Carla.host.get_input_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_input_peak_value(self.fPluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.ui.peak_in.displayMeter(1, peak1)
                self.ui.peak_in.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_input_peak_value(self.fPluginId, 1)
                ledState = bool(peak != 0.0)

                self.ui.peak_in.displayMeter(1, peak)

            if self.fLastGreenLedState != ledState:
                self.fLastGreenLedState = ledState
                self.ui.led_audio_in.setChecked(ledState)

        # Output peaks
        if self.fPeaksOutputCount > 0:
            if self.fPeaksOutputCount > 1:
                peak1 = Carla.host.get_output_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_output_peak_value(self.fPluginId, 2)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                self.ui.peak_out.displayMeter(1, peak1)
                self.ui.peak_out.displayMeter(2, peak2)

            else:
                peak = Carla.host.get_output_peak_value(self.fPluginId, 1)
                ledState = bool(peak != 0.0)

                self.ui.peak_out.displayMeter(1, peak)

            if self.fLastBlueLedState != ledState:
                self.fLastBlueLedState = ledState
                self.ui.led_audio_out.setChecked(ledState)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_enableClicked(self, yesNo):
        self.setActive(yesNo, False, True)

    @pyqtSlot()
    def slot_showCustomMenu(self):
        menu = QMenu(self)

        actActive = menu.addAction(self.tr("Disable") if self.ui.b_enable.isChecked() else self.tr("Enable"))
        menu.addSeparator()

        actGui = menu.addAction(self.tr("Show GUI"))
        actGui.setCheckable(True)
        actGui.setChecked(self.ui.b_gui.isChecked())
        actGui.setEnabled(self.ui.b_gui.isEnabled())

        actEdit = menu.addAction(self.tr("Edit"))
        actEdit.setCheckable(True)
        actEdit.setChecked(self.ui.b_edit.isChecked())

        menu.addSeparator()
        actClone  = menu.addAction(self.tr("Clone"))
        actRename = menu.addAction(self.tr("Rename..."))
        actRemove = menu.addAction(self.tr("Remove"))

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            return

        if actSel == actActive:
            self.setActive(not self.ui.b_enable.isChecked(), True, True)
        elif actSel == actGui:
            self.ui.b_gui.click()
        elif actSel == actEdit:
            self.ui.b_edit.click()
        elif actSel == actClone:
            if not Carla.host.clone_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       Carla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRename:
            oldName    = self.fPluginInfo['name']
            newNameTry = QInputDialog.getText(self, self.tr("Rename Plugin"), self.tr("New plugin name:"), QLineEdit.Normal, oldName)

            if not (newNameTry[1] and newNameTry[0] and oldName != newNameTry[0]):
                return

            newName = newNameTry[0]

            if Carla.host is None or Carla.host.rename_plugin(self.fPluginId, newName):
                self.setName(newName)
            else:
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       Carla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRemove:
            if not Carla.host.remove_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       Carla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.save()

        areaX  = self.ui.area_right.x()+7
        width  = self.width()
        height = self.height()

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
        PluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_ZynFX(PluginSlot):
    def __init__(self, parent, pluginId):
        PluginSlot.__init__(self, parent, pluginId)
        self.ui = ui_carla_plugin_zynfx.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
        QFrame#PluginWidget {
            background-image: url(:/bitmaps/background_zynfx.png);
            background-repeat: repeat-xy;
        }""")

        #background-position: top left;

        self.ui.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")
        self.ui.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        self.ui.peak_in.setColor(self.ui.peak_in.GREEN)
        self.ui.peak_in.setChannels(self.fPeaksInputCount)
        self.ui.peak_in.setOrientation(self.ui.peak_in.VERTICAL)

        self.ui.peak_out.setColor(self.ui.peak_in.BLUE)
        self.ui.peak_out.setChannels(self.fPeaksOutputCount)
        self.ui.peak_out.setOrientation(self.ui.peak_out.VERTICAL)

        self.ui.label_name.setText(self.fPluginInfo['name'])

        # -------------------------------------------------------------
        # Set-up parameters

        self.fParameterList = [] # index, widget

        parameterCount = Carla.host.get_parameter_count(self.fPluginId)

        index = 0
        for i in range(parameterCount):
            paramInfo   = Carla.host.get_parameter_info(self.fPluginId, i)
            paramData   = Carla.host.get_parameter_data(self.fPluginId, i)
            paramRanges = Carla.host.get_parameter_ranges(self.fPluginId, i)
            paramValue  = Carla.host.get_current_parameter_value(self.fPluginId, i)

            if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                continue

            widget = PixmapDial(self)
            widget.setPixmap(5)
            widget.setLabel(charPtrToString(paramInfo['name']))

            widget.setMinimum(paramRanges['min']*100)
            widget.setMaximum(paramRanges['max']*100)
            widget.setValue(paramValue*100)

            self.ui.container.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        # -------------------------------------------------------------
        # Set-up MIDI programs

        midiProgramCount = Carla.host.get_midi_program_count(self.fPluginId) if Carla.host is not None else 0

        if midiProgramCount > 0:
            self.ui.cb_presets.setEnabled(True)
            self.ui.label_presets.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = Carla.host.get_midi_program_data(self.fPluginId, i)
                mpName = charPtrToString(mpData['name'])

                self.ui.cb_presets.addItem(mpName)

            self.fCurrentMidiProgram = Carla.host.get_current_midi_program_index(self.fPluginId)
            self.ui.cb_presets.setCurrentIndex(self.fCurrentMidiProgram)

        else:
            self.fCurrentMidiProgram = -1
            self.ui.cb_presets.setEnabled(False)
            self.ui.label_presets.setEnabled(False)

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.b_enable.clicked.connect(self.slot_enableClicked)
        self.ui.b_edit.clicked.connect(self.slot_showEditDialog)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 75

    #------------------------------------------------------------------

    def setName(self, name):
        self.ui.label_name.setText(name)
        PluginSlot.setName(self, name)

    #------------------------------------------------------------------

    def activeChanged(self, onOff):
        self.ui.b_enable.setChecked(onOff)

    def editDialogChanged(self, visible):
        self.ui.b_edit.blockSignals(True)
        self.ui.b_edit.setChecked(visible)
        self.ui.b_edit.blockSignals(False)

    #------------------------------------------------------------------

    def idleFast(self):
        # Input peaks
        if self.fPeaksInputCount > 0:
            if self.fPeaksInputCount > 1:
                peak1 = Carla.host.get_input_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_input_peak_value(self.fPluginId, 2)
                self.ui.peak_in.displayMeter(1, peak1)
                self.ui.peak_in.displayMeter(2, peak2)
            else:
                peak = Carla.host.get_input_peak_value(self.fPluginId, 1)
                self.ui.peak_in.displayMeter(1, peak)

        # Output peaks
        if self.fPeaksOutputCount > 0:
            if self.fPeaksOutputCount > 1:
                peak1 = Carla.host.get_output_peak_value(self.fPluginId, 1)
                peak2 = Carla.host.get_output_peak_value(self.fPluginId, 2)
                self.ui.peak_out.displayMeter(1, peak1)
                self.ui.peak_out.displayMeter(2, peak2)
            else:
                peak = Carla.host.get_output_peak_value(self.fPluginId, 1)
                self.ui.peak_out.displayMeter(1, peak)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_enableClicked(self, yesNo):
        self.setActive(yesNo, False, True)

    #------------------------------------------------------------------

    #def paintEvent(self, event):
        #painter = QPainter(self)
        #painter.save()

        #painter.setPen(QColor(0, 0, 200))
        #painter.setBrush(QColor(0, 0, 200))
        #painter.drawRect(0, 0, self.width(), self.height())

        #painter.restore()
        #PluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

def createPluginSlot(parent, pluginId):
    pluginInfo = Carla.host.get_plugin_info(pluginId)
    pluginInfo['label']    = charPtrToString(pluginInfo['label'])
    pluginInfo['maker']    = charPtrToString(pluginInfo['maker'])
    pluginInfo['iconName'] = charPtrToString(pluginInfo['iconName'])

    #return PluginSlot(parent, pluginId)
    return PluginSlot_Default(parent, pluginId)
    #return PluginSlot_ZynFX(parent, pluginId)

# ------------------------------------------------------------------------------------------------------------
