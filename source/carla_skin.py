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
    from PyQt5.QtGui import QFont, QPen, QPixmap
    from PyQt5.QtWidgets import QFrame, QPushButton
else:
    from PyQt4.QtCore import Qt, QRectF
    from PyQt4.QtGui import QFont, QFrame, QPen, QPixmap, QPushButton

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_plugin_default
import ui_carla_plugin_basic_fx
import ui_carla_plugin_calf
import ui_carla_plugin_zita
import ui_carla_plugin_zynfx

from carla_widgets import *
from pixmapdial import PixmapDial

# ------------------------------------------------------------------------------------------------------------
# Abstract plugin slot

class AbstractPluginSlot(QFrame):
    def __init__(self, parent, pluginId):
        QFrame.__init__(self, parent)

        # -------------------------------------------------------------
        # Get plugin info

        self.fPluginId   = pluginId
        self.fPluginInfo = gCarla.host.get_plugin_info(self.fPluginId) if gCarla.host is not None else gFakePluginInfo

        if not gCarla.isLocal:
            self.fPluginInfo['hints'] &= ~PLUGIN_HAS_CUSTOM_UI

        # -------------------------------------------------------------
        # Internal stuff

        self.fIsActive   = bool(gCarla.host.get_internal_parameter_value(self.fPluginId, PARAMETER_ACTIVE) >= 0.5) if gCarla.host is not None else True
        self.fIsSelected = False

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        self.fParameterIconTimer = ICON_STATE_NULL
        self.fParameterList      = [] # index, widget

        if gCarla.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK or gCarla.host is None:
            self.fPeaksInputCount  = 2
            self.fPeaksOutputCount = 2
        else:
            audioCountInfo = gCarla.host.get_audio_port_count_info(self.fPluginId)

            self.fPeaksInputCount  = int(audioCountInfo['ins'])
            self.fPeaksOutputCount = int(audioCountInfo['outs'])

            if self.fPeaksInputCount > 2:
                self.fPeaksInputCount = 2

            if self.fPeaksOutputCount > 2:
                self.fPeaksOutputCount = 2

        # -------------------------------------------------------------
        # Set-up GUI

        self.fEditDialog = PluginEdit(self, self.fPluginId)

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

        self.peak_in  = None
        self.peak_out = None

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
            self.peak_in.setColor(self.peak_in.GREEN)
            self.peak_in.setChannels(self.fPeaksInputCount)
            self.peak_in.setOrientation(self.peak_in.HORIZONTAL)

        if self.peak_out is not None:
            self.peak_out.setColor(self.peak_in.BLUE)
            self.peak_out.setChannels(self.fPeaksOutputCount)
            self.peak_out.setOrientation(self.peak_out.HORIZONTAL)

        if gCarla.host is None:
            return

        for paramIndex, paramWidget in self.fParameterList:
            paramWidget.setValue(gCarla.host.get_internal_parameter_value(self.fPluginId, paramIndex))
            paramWidget.realValueChanged.connect(self.slot_parameterValueChanged)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 32

    def getHints(self):
        return self.fPluginInfo['hints']

    def getPluginId(self):
        return self.fPluginId

    #------------------------------------------------------------------

    def recheckPluginHints(self, hints):
        self.fPluginInfo['hints'] = hints

        if self.b_gui is not None:
            self.b_gui.setEnabled(bool(hints & PLUGIN_HAS_CUSTOM_UI))

    def setId(self, idx):
        self.fPluginId = idx
        self.fEditDialog.setId(idx)

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

    def setActive(self, active, sendGui=False, sendCallback=True):
        self.fIsActive = active

        if sendGui:      self.activeChanged(active)
        if sendCallback: gCarla.host.set_active(self.fPluginId, active)

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
            gCarla.host.set_drywet(self.fPluginId, value)

        elif parameterId == PARAMETER_VOLUME:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME) == 0: return
            gCarla.host.set_volume(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_LEFT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            gCarla.host.set_balance_left(self.fPluginId, value)

        elif parameterId == PARAMETER_BALANCE_RIGHT:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_BALANCE) == 0: return
            gCarla.host.set_balance_right(self.fPluginId, value)

        elif parameterId == PARAMETER_PANNING:
            if (self.fPluginInfo['hints'] & PLUGIN_CAN_PANNING) == 0: return
            gCarla.host.set_panning(self.fPluginId, value)

        elif parameterId == PARAMETER_CTRL_CHANNEL:
            gCarla.host.set_ctrl_channel(self.fPluginId, value)

        self.fEditDialog.setParameterValue(parameterId, value)

    #------------------------------------------------------------------

    def setParameterValue(self, parameterId, value, sendCallback):
        self.fParameterIconTimer = ICON_STATE_ON

        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, False)

        self.fEditDialog.setParameterValue(parameterId, value)

        if sendCallback:
            self.parameterValueChanged(parameterId, value)

    def setParameterDefault(self, parameterId, value):
        self.fEditDialog.setParameterDefault(parameterId, value)

    def setParameterMidiControl(self, parameterId, control):
        self.fEditDialog.setParameterMidiControl(parameterId, control)

    def setParameterMidiChannel(self, parameterId, channel):
        self.fEditDialog.setParameterMidiChannel(parameterId, channel)

    #------------------------------------------------------------------

    def setProgram(self, index, sendCallback):
        self.fParameterIconTimer = ICON_STATE_ON
        self.fEditDialog.setProgram(index)

        if sendCallback:
            self.programChanged(index)

    def setMidiProgram(self, index, sendCallback):
        self.fParameterIconTimer = ICON_STATE_ON
        self.fEditDialog.setMidiProgram(index)

        if sendCallback:
            self.midiProgramChanged(index)

    #------------------------------------------------------------------

    def sendNoteOn(self, channel, note):
        if self.fEditDialog.sendNoteOn(channel, note):
            self.midiActivityChanged(True)

    def sendNoteOff(self, channel, note):
        if self.fEditDialog.sendNoteOff(channel, note):
            self.midiActivityChanged(False)

    #------------------------------------------------------------------

    def activeChanged(self, onOff):
        self.fIsActive = onOff

        if self.b_enable is None:
            return

        self.b_enable.blockSignals(True)
        self.b_enable.setChecked(onOff)
        self.b_enable.blockSignals(False)

    def editDialogChanged(self, visible):
        if self.b_edit is None:
            return

        self.b_edit.blockSignals(True)
        self.b_edit.setChecked(visible)
        self.b_edit.blockSignals(False)

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

    def parameterValueChanged(self, parameterId, value):
        for paramIndex, paramWidget in self.fParameterList:
            if paramIndex != parameterId:
                continue

            paramWidget.blockSignals(True)
            paramWidget.setValue(value)
            paramWidget.blockSignals(False)
            break

    def programChanged(self, index):
        if self.cb_presets is None:
            return

        self.cb_presets.blockSignals(True)
        self.cb_presets.setCurrentIndex(index)
        self.cb_presets.blockSignals(False)

    def midiProgramChanged(self, index):
        if self.cb_presets is None:
            return

        self.cb_presets.blockSignals(True)
        self.cb_presets.setCurrentIndex(index)
        self.cb_presets.blockSignals(False)

    def notePressed(self, note):
        pass

    def noteReleased(self, note):
        pass

    #------------------------------------------------------------------

    def idleFast(self):
        # Input peaks
        if self.fPeaksInputCount > 0:
            if self.fPeaksInputCount > 1:
                peak1 = gCarla.host.get_input_peak_value(self.fPluginId, True)
                peak2 = gCarla.host.get_input_peak_value(self.fPluginId, False)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                if self.peak_in is not None:
                    self.peak_in.displayMeter(1, peak1)
                    self.peak_in.displayMeter(2, peak2)

            else:
                peak = gCarla.host.get_input_peak_value(self.fPluginId, True)
                ledState = bool(peak != 0.0)

                if self.peak_in is not None:
                    self.peak_in.displayMeter(1, peak)

            if self.fLastGreenLedState != ledState and self.led_audio_in is not None:
                self.fLastGreenLedState = ledState
                self.led_audio_in.setChecked(ledState)

        # Output peaks
        if self.fPeaksOutputCount > 0:
            if self.fPeaksOutputCount > 1:
                peak1 = gCarla.host.get_output_peak_value(self.fPluginId, True)
                peak2 = gCarla.host.get_output_peak_value(self.fPluginId, False)
                ledState = bool(peak1 != 0.0 or peak2 != 0.0)

                if self.peak_out is not None:
                    self.peak_out.displayMeter(1, peak1)
                    self.peak_out.displayMeter(2, peak2)

            else:
                peak = gCarla.host.get_output_peak_value(self.fPluginId, True)
                ledState = bool(peak != 0.0)

                if self.peak_out is not None:
                    self.peak_out.displayMeter(1, peak)

            if self.fLastBlueLedState != ledState and self.led_audio_out is not None:
                self.fLastBlueLedState = ledState
                self.led_audio_out.setChecked(ledState)

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

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            return

        if actSel == actActive:
            self.setActive(not isEnabled, True, True)

        elif actSel == actReset:
            if gCarla.host is None: return
            gCarla.host.reset_parameters(self.fPluginId)

        elif actSel == actRandom:
            if gCarla.host is None: return
            gCarla.host.randomize_parameters(self.fPluginId)

        elif actSel == actGui:
            bGui.click()

        elif actSel == actEdit:
            bEdit.click()

        elif actSel == actClone:
            if gCarla.host is not None and not gCarla.host.clone_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       gCarla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRename:
            oldName    = self.fPluginInfo['name']
            newNameTry = QInputDialog.getText(self, self.tr("Rename Plugin"), self.tr("New plugin name:"), QLineEdit.Normal, oldName)

            if not (newNameTry[1] and newNameTry[0] and oldName != newNameTry[0]):
                return

            newName = newNameTry[0]

            if gCarla.host is None or gCarla.host.rename_plugin(self.fPluginId, newName):
                self.setName(newName)
            else:
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       gCarla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actReplace:
            gCarla.gui.slot_pluginAdd(self.fPluginId)

        elif actSel == actRemove:
            if gCarla.host is not None and not gCarla.host.remove_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       gCarla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_enableClicked(self, yesNo):
        self.setActive(yesNo, False, True)

    @pyqtSlot()
    def slot_showDefaultCustomMenu(self):
        self.showDefaultCustomMenu(self.fIsActive, self.b_edit, self.b_gui)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_showCustomUi(self, show):
        gCarla.host.show_custom_ui(self.fPluginId, show)

    @pyqtSlot(bool)
    def slot_showEditDialog(self, show):
        self.fEditDialog.setVisible(show)

    @pyqtSlot()
    def slot_removePlugin(self):
        gCarla.host.remove_plugin(self.fPluginId)

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_parameterValueChanged(self, value):
        index = self.sender().getIndex()

        if index < 0:
            self.setInternalParameter(index, value)
        else:
            gCarla.host.set_parameter_value(self.fPluginId, index, value)
            self.setParameterValue(index, value, False)

    @pyqtSlot(int)
    def slot_programChanged(self, index):
        gCarla.host.set_program(self.fPluginId, index)
        self.setProgram(index, False)

    @pyqtSlot(int)
    def slot_midiProgramChanged(self, index):
        gCarla.host.set_midi_program(self.fPluginId, index)
        self.setMidiProgram(index, False)

    #------------------------------------------------------------------

    def paintEvent(self, event):
        self.drawOutline()
        QFrame.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Default(AbstractPluginSlot):
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
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
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
        self.ui = ui_carla_plugin_basic_fx.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set-up GUI

        labelFont = QFont()
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

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = gCarla.host.get_parameter_count(self.fPluginId) if gCarla.host is not None else 0

        index = 0
        for i in range(min(parameterCount, 8)):
            paramInfo   = gCarla.host.get_parameter_info(self.fPluginId, i)
            paramData   = gCarla.host.get_parameter_data(self.fPluginId, i)
            paramRanges = gCarla.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            paramName = paramInfo['name'].split("/", 1)[0].split(" (", 1)[0].strip()
            paramLow  = paramName.lower()

            if "Bandwidth" in paramName:
                paramName = paramName.replace("Bandwidth", "Bw")
            elif "Frequency" in paramName:
                paramName = paramName.replace("Frequency", "Freq")
            elif "Output" in paramName:
                paramName = paramName.replace("Output", "Out")
            elif paramLow == "threshold":
                paramName = "Thres"

            if len(paramName) > 9:
                paramName = paramName[:9]

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
            _r = 255 - float(i)/8*200
            _g =  55 + float(i)/8*200
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

        self.ui.dial_vol.setIndex(PARAMETER_VOLUME)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_VOL)
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.forceWhiteLabelGradientText()

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

        self.peak_in  = self.ui.peak_in
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
        painter.drawRect(0, 1, self.width()-1, 76)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Nekobi(AbstractPluginSlot):
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
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

class PluginSlot_Calf(AbstractPluginSlot):
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
        self.ui = ui_carla_plugin_calf.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fButtonFont = QFont()
        #self.fButtonFont.setBold(False)
        self.fButtonFont.setPointSize(8)

        self.fButtonColorOn  = QColor( 18,  41,  87)
        self.fButtonColorOff = QColor(150, 150, 150)

        # -------------------------------------------------------------
        # Set-up GUI

        self.setStyleSheet("""
            QLabel#label_audio_in, QLabel#label_audio_out, QLabel#label_midi {
                color: black;
            }
            PluginSlot_Calf#PluginWidget {
                background-image: url(:/bitmaps/background_calf.png);
                background-repeat: repeat-xy;
                border: 2px;
            }
        """)

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
        labelFont.setPointSize(labelFont.pointSize()+3)
        self.ui.label_name.setFont(labelFont)

        audioCount = gCarla.host.get_audio_port_count_info(self.fPluginId) if gCarla.host is not None else {'ins': 2, 'outs': 2 }
        midiCount  = gCarla.host.get_midi_port_count_info(self.fPluginId) if gCarla.host is not None else {'ins': 1, 'outs': 0 }

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

        # -------------------------------------------------------------

        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit
        self.b_remove = self.ui.b_remove

        self.label_name    = self.ui.label_name
        self.led_midi      = self.ui.led_midi

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.ui.led_midi.setColor(self.ui.led_midi.CALF)

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 70

    #------------------------------------------------------------------

    def recheckPluginHints(self, hints):
        if hints & PLUGIN_HAS_CUSTOM_UI:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOn, self.fButtonFont)
        else:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOff, self.fButtonFont)

        AbstractPluginSlot.recheckPluginHints(self, hints)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_ZitaRev(AbstractPluginSlot):
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
        self.ui = ui_carla_plugin_zita.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        audioCount = gCarla.host.get_audio_port_count_info(self.fPluginId) if gCarla.host is not None else {'ins': 2, 'outs': 2 }

        # -------------------------------------------------------------
        # Set-up GUI

        self.setMinimumWidth(640)

        self.setStyleSheet("""
            PluginSlot_ZitaRev#PluginWidget {
                background-color: #404040;
                border: 2px solid transparent;
            }
            QWidget#w_revsect {
                background-image: url(:/bitmaps/zita-rev/revsect.png);
            }
            QWidget#w_eq1sect {
                background-image: url(:/bitmaps/zita-rev/eq1sect.png);
            }
            QWidget#w_eq2sect {
                background-image: url(:/bitmaps/zita-rev/eq2sect.png);
            }
            QWidget#w_ambmixsect {
                background-image: url(:/bitmaps/zita-rev/%s.png);
            }
            QWidget#w_redzita {
                background-image: url(:/bitmaps/zita-rev/redzita.png);
            }
        """ % ("mixsect" if audioCount['outs'] == 2 else "ambsect"))

        # -------------------------------------------------------------
        # Set-up Knobs

        self.fKnobDelay = PixmapDial(self, 0)
        self.fKnobDelay.setPixmap(6)
        self.fKnobDelay.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobDelay.setMinimum(0.02)
        self.fKnobDelay.setMaximum(0.10)

        self.fKnobXover = PixmapDial(self, 1)
        self.fKnobXover.setPixmap(6)
        self.fKnobXover.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobXover.setMinimum(50.0)
        self.fKnobXover.setMaximum(1000.0)

        self.fKnobRtLow = PixmapDial(self, 2)
        self.fKnobRtLow.setPixmap(6)
        self.fKnobRtLow.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobRtLow.setMinimum(1.0)
        self.fKnobRtLow.setMaximum(8.0)

        self.fKnobRtMid = PixmapDial(self, 3)
        self.fKnobRtMid.setPixmap(6)
        self.fKnobRtMid.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobRtMid.setMinimum(1.0)
        self.fKnobRtMid.setMaximum(8.0)

        self.fKnobDamping = PixmapDial(self, 4)
        self.fKnobDamping.setPixmap(6)
        self.fKnobDamping.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobDamping.setMinimum(1500.0)
        self.fKnobDamping.setMaximum(24000.0)

        self.fKnobEq1Freq = PixmapDial(self, 5)
        self.fKnobEq1Freq.setPixmap(6)
        self.fKnobEq1Freq.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobEq1Freq.setMinimum(40.0)
        self.fKnobEq1Freq.setMaximum(10000.0)

        self.fKnobEq1Gain = PixmapDial(self, 6)
        self.fKnobEq1Gain.setPixmap(6)
        self.fKnobEq1Gain.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobEq1Gain.setMinimum(-20.0)
        self.fKnobEq1Gain.setMaximum(20.0)

        self.fKnobEq2Freq = PixmapDial(self, 7)
        self.fKnobEq2Freq.setPixmap(6)
        self.fKnobEq2Freq.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobEq2Freq.setMinimum(40.0)
        self.fKnobEq2Freq.setMaximum(10000.0)

        self.fKnobEq2Gain = PixmapDial(self, 8)
        self.fKnobEq2Gain.setPixmap(6)
        self.fKnobEq2Gain.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobEq2Gain.setMinimum(-20.0)
        self.fKnobEq2Gain.setMaximum(20.0)

        self.fKnobMix = PixmapDial(self, 9)
        self.fKnobMix.setPixmap(6)
        self.fKnobMix.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_ZITA)
        self.fKnobMix.setMinimum(0.0)
        self.fKnobMix.setMaximum(1.0)

        self.fParameterList.append([0, self.fKnobDelay])
        self.fParameterList.append([1, self.fKnobXover])
        self.fParameterList.append([2, self.fKnobRtLow])
        self.fParameterList.append([3, self.fKnobRtMid])
        self.fParameterList.append([4, self.fKnobDamping])
        self.fParameterList.append([5, self.fKnobEq1Freq])
        self.fParameterList.append([6, self.fKnobEq1Gain])
        self.fParameterList.append([7, self.fKnobEq2Freq])
        self.fParameterList.append([8, self.fKnobEq2Gain])
        self.fParameterList.append([9, self.fKnobMix])

        # -------------------------------------------------------------

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 79

    #------------------------------------------------------------------

    def paintEvent(self, event):
        AbstractPluginSlot.paintEvent(self, event)
        self.drawOutline()

    def resizeEvent(self, event):
        self.fKnobDelay.move(self.ui.w_revsect.x()+31, self.ui.w_revsect.y()+33)
        self.fKnobXover.move(self.ui.w_revsect.x()+93, self.ui.w_revsect.y()+18)
        self.fKnobRtLow.move(self.ui.w_revsect.x()+148, self.ui.w_revsect.y()+18)
        self.fKnobRtMid.move(self.ui.w_revsect.x()+208, self.ui.w_revsect.y()+18)
        self.fKnobDamping.move(self.ui.w_revsect.x()+268, self.ui.w_revsect.y()+18)
        self.fKnobEq1Freq.move(self.ui.w_eq1sect.x()+20, self.ui.w_eq1sect.y()+33)
        self.fKnobEq1Gain.move(self.ui.w_eq1sect.x()+69, self.ui.w_eq1sect.y()+18)
        self.fKnobEq2Freq.move(self.ui.w_eq2sect.x()+20, self.ui.w_eq2sect.y()+33)
        self.fKnobEq2Gain.move(self.ui.w_eq2sect.x()+69, self.ui.w_eq2sect.y()+18)
        self.fKnobMix.move(self.ui.w_ambmixsect.x()+24, self.ui.w_ambmixsect.y()+33)
        AbstractPluginSlot.resizeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_ZynFX(AbstractPluginSlot):
    def __init__(self, parent, pluginId):
        AbstractPluginSlot.__init__(self, parent, pluginId)
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

        # -------------------------------------------------------------
        # Set-up parameters

        parameterCount = gCarla.host.get_parameter_count(self.fPluginId) if gCarla.host is not None else 0

        index = 0
        for i in range(parameterCount):
            paramInfo   = gCarla.host.get_parameter_info(self.fPluginId, i)
            paramData   = gCarla.host.get_parameter_data(self.fPluginId, i)
            paramRanges = gCarla.host.get_parameter_ranges(self.fPluginId, i)

            if paramData['type'] != PARAMETER_INPUT:
                continue

            paramName = paramInfo['name']
            #paramLow  = paramName.lower()

            # real zyn fx plugins
            if self.fPluginInfo['label'] == "zynalienwah":
                if   i == 0: paramName = "Freq"
                elif i == 1: paramName = "Rnd"
                elif i == 2: paramName = "L type" # combobox
                elif i == 3: paramName = "St.df"
                elif i == 5: paramName = "Fb"
                elif i == 7: paramName = "L/R"

            if self.fPluginInfo['label'] == "zynchorus":
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

            #elif paramLow.find("damp"):
                #paramName = "Damp"
            #elif paramLow.find("frequency"):
                #paramName = "Freq"

            # Cut generic names
            #elif paramName == "Depth":      paramName = "Dpth"
            #elif paramName == "Feedback":   paramName = "Fb"
            #elif paramName == "L/R Cross": #paramName = "L/R"
            #elif paramName == "Random":     paramName = "Rnd"

            widget = PixmapDial(self, i)
            widget.setEnabled(True)

            widget.setPixmap(5)
            widget.setLabel(paramName)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
            #widget.setMidWhiteText()

            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])

            #if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                #widget.setEnabled(False)

            self.ui.container.layout().insertWidget(index, widget)
            index += 1

            self.fParameterList.append([i, widget])

        # -------------------------------------------------------------
        # Set-up MIDI programs

        midiProgramCount = gCarla.host.get_midi_program_count(self.fPluginId) if gCarla.host is not None else 0

        if midiProgramCount > 0:
            self.ui.cb_presets.setEnabled(True)
            self.ui.label_presets.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = gCarla.host.get_midi_program_data(self.fPluginId, i)
                mpName = mpData['name']

                self.ui.cb_presets.addItem(mpName)

            self.fCurrentMidiProgram = gCarla.host.get_current_midi_program_index(self.fPluginId)
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

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.ready()

        self.peak_in.setOrientation(self.peak_in.VERTICAL)
        self.peak_out.setOrientation(self.peak_out.VERTICAL)

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)
        self.ui.cb_presets.currentIndexChanged.connect(self.slot_midiProgramChanged)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 70

# ------------------------------------------------------------------------------------------------------------

def createPluginSlot(parent, pluginId):
    pluginInfo  = gCarla.host.get_plugin_info(pluginId)
    pluginName  = gCarla.host.get_real_plugin_name(pluginId)
    pluginLabel = pluginInfo['label']
    uniqueId    = pluginInfo['uniqueId']

    #pluginMaker = pluginInfo['maker']
    #pluginIcon  = pluginInfo['iconName']

    if pluginInfo['type'] == PLUGIN_INTERNAL:
        if pluginLabel.startswith("zyn") and pluginInfo['category'] != PLUGIN_CATEGORY_SYNTH:
            return PluginSlot_ZynFX(parent, pluginId)

    elif pluginInfo['type'] == PLUGIN_LADSPA:
        if (pluginLabel == "zita-reverb" and uniqueId == 3701) or (pluginLabel == "zita-reverb-amb" and uniqueId == 3702):
            return PluginSlot_ZitaRev(parent, pluginId)

    if pluginName.split(" ", 1)[0].lower() == "calf":
        return PluginSlot_Calf(parent, pluginId)

    #if pluginName.lower() == "nekobi":
        #return PluginSlot_Nekobi(parent, pluginId)

    return PluginSlot_BasicFX(parent, pluginId)
    return PluginSlot_Default(parent, pluginId)

# ------------------------------------------------------------------------------------------------------------
# Main Testing

if __name__ == '__main__':
    from carla_style import CarlaApplication
    import resources_rc

    app = CarlaApplication("Carla-Skins")
    #gui = PluginSlot_BasicFX(None, 0)
    #gui = PluginSlot_Calf(None, 0)
    #gui = PluginSlot_Default(None, 0)
    #gui = PluginSlot_ZitaRev(None, 0)
    gui = PluginSlot_ZynFX(None, 0)
    gui.setSelected(True)
    gui.show()

    app.exec_()
