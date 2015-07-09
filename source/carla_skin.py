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

import ui_carla_plugin_calf
import ui_carla_plugin_classic
import ui_carla_plugin_compact
import ui_carla_plugin_default
import ui_carla_plugin_presets

from carla_widgets import *
from digitalpeakmeter import DigitalPeakMeter
from pixmapdial import PixmapDial

# ------------------------------------------------------------------------------------------------------------
# Plugin Skin Rules (WORK IN PROGRESS)

# Base is a QFrame (NoFrame, Plain, 0-size lines), with "PluginWidget" as object name.
# Spacing of the top-most layout must be 1px.
# Top and bottom margins must be 3px (can be splitted between different qt layouts).
# Left and right margins must be 6px (can be splitted between different qt layouts).
# If the left or right side has built-in margins, say a transparent png border,
# those margins must be taken into consideration.
#
# There's a top and bottom layout, separated by a horizontal line.
# Compacted skins do not have the bottom layout and separating line.

#  T O P  A R E A
#
#  -----------------------------------------------------------------
# |         <> | <>                            [ WIDGETS ] [ LEDS ] |
# | BUTTONS <> | <> PLUGIN NAME   < spacer >   [ WIDGETS ] [ LEDS ] |
# |         <> | <>                            [ WIDGETS ] [ LEDS ] |
#  -----------------------------------------------------------------
#
# Buttons area has size fixed. (TBA)
# Spacers at the left of the plugin name must be 8x1 in size (fixed).
# The line before the plugin name must be height-10px (fixed).
# WIDGETS area can be extended to the left, if using meters they should have 80px.
# WIDGETS margins are 4px for left+right and 2px for top+bottom, with 4px spacing.

# ------------------------------------------------------------------------------------------------------------
# Try to "shortify" a parameter name

def getParameterShortName(paramName):
    paramName = paramName.split("/",1)[0].split(" (",1)[0].split(" [",1)[0].strip()
    paramLow  = paramName.lower()

    # Cut useless prefix
    if paramLow.startswith("compressor "):
        paramName = paramName.replace("ompressor ", ".", 1)
        paramLow  = paramName.lower()
    elif paramLow.startswith("room "):
        paramName = paramName.split(" ",1)[1]
        paramLow  = paramName.lower()

    # Cut useless suffix
    if paramLow.endswith(" level"):
        paramName = paramName.rsplit(" ",1)[0]
        paramLow  = paramName.lower()
    elif paramLow.endswith(" time"):
        paramName = paramName.rsplit(" ",1)[0]
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
    elif "input" in paramLow:
        paramName = paramName.replace("nput", "n")
    elif "makeup" in paramLow:
        paramName = paramName.replace("akeup", "kUp" if "Make" in paramName else "kup")
    elif "output" in paramLow:
        paramName = paramName.replace("utput", "ut")
    elif "random" in paramLow:
        paramName = paramName.replace("andom", "nd")
    elif "threshold" in paramLow:
        paramName = paramName.replace("hreshold", "hres")

    # remove space if last char from 1st word is lowercase and the first char from the 2nd is uppercase,
    # or if 2nd is a number
    if " " in paramName:
        name1, name2 = paramName.split(" ", 1)
        if (name1[-1].islower() and name2[0].isupper()) or name2.isdigit():
            paramName = paramName.replace(" ", "", 1)

    # cut stuff if too big
    if len(paramName) > 7:
        paramName = paramName.replace("a","").replace("e","").replace("i","").replace("o","").replace("u","")

        if len(paramName) > 7:
            paramName = paramName[:7]

    return paramName.strip()

# ------------------------------------------------------------------------------------------------------------
# Get RGB colors for a plugin category

def getColorFromCategory(category):
    r = 40
    g = 40
    b = 40

    if category == PLUGIN_CATEGORY_MODULATOR:
        r += 10
    elif category == PLUGIN_CATEGORY_EQ:
        g += 10
    elif category == PLUGIN_CATEGORY_FILTER:
        b += 10
    elif category == PLUGIN_CATEGORY_DELAY:
        r += 15
        b -= 15
    elif category == PLUGIN_CATEGORY_DISTORTION:
        g += 10
        b += 10
    elif category == PLUGIN_CATEGORY_DYNAMICS:
        r += 10
        b += 10
    elif category == PLUGIN_CATEGORY_UTILITY:
        r += 10
        g += 10

    return (r, g, b)

def getModColorFromCategory(category):
    if category == PLUGIN_CATEGORY_NONE:
        return (40, 40, 40)
    if category == PLUGIN_CATEGORY_SYNTH:
        return (30, 150, 49)
    if category == PLUGIN_CATEGORY_DELAY:
        return (47, 47, 47)
    if category in (PLUGIN_CATEGORY_EQ, PLUGIN_CATEGORY_FILTER):
        return (255, 220, 25)
    if category == PLUGIN_CATEGORY_DISTORTION:
        return (255, 127, 25)
    if category == PLUGIN_CATEGORY_DYNAMICS:
        return (255, 25, 25)
    if category == PLUGIN_CATEGORY_MODULATOR:
        return (109, 31, 142)
    if category == PLUGIN_CATEGORY_UTILITY:
        return (95, 95, 95)
    if category == PLUGIN_CATEGORY_OTHER:
        return (92, 210, 254)
    return (40, 40, 40)

# ------------------------------------------------------------------------------------------------------------
#

def setPixmapDialStyle(widget, parameterId, parameterCount, skinStyle):
    if "calf" in skinStyle:
        widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        widget.setPixmap(7)

    elif skinStyle == "mod":
        widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        widget.setPixmap(14)

    elif skinStyle == "openav":
        widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_NO_GRADIENT)
        if parameterId == PARAMETER_DRYWET:
            widget.setPixmap(13)
        elif parameterId == PARAMETER_VOLUME:
            widget.setPixmap(12)
        else:
            widget.setPixmap(11)

    else:
        if parameterId == PARAMETER_DRYWET:
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_WET)
        elif parameterId == PARAMETER_VOLUME:
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_VOL)
        else:
            _r = 255 - int((float(parameterId)/float(parameterCount))*200.0)
            _g =  55 + int((float(parameterId)/float(parameterCount))*200.0)
            _b = 0 #(r-40)*4
            widget.setCustomPaintColor(QColor(_r, _g, _b))
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_COLOR)

        widget.setPixmap(3)
        widget.forceWhiteLabelGradientText()

# ------------------------------------------------------------------------------------------------------------
# Abstract plugin slot

class AbstractPluginSlot(QFrame, PluginEditParentMeta):
#class AbstractPluginSlot(QFrame, PluginEditParentMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, pluginId, skinStyle):
        QFrame.__init__(self, parent)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        # -------------------------------------------------------------
        # Get plugin info

        self.fPluginId   = pluginId
        self.fPluginInfo = host.get_plugin_info(self.fPluginId)
        self.fSkinStyle  = skinStyle

        # -------------------------------------------------------------
        # Internal stuff

        self.fIsActive   = False
        self.fIsSelected = False

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        self.fParameterIconTimer = ICON_STATE_NULL
        self.fParameterList      = [] # index, widget

        audioCountInfo = host.get_audio_port_count_info(self.fPluginId)

        self.fPeaksInputCount  = audioCountInfo['ins']
        self.fPeaksOutputCount = audioCountInfo['outs']

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

        self.label_name    = None
        self.label_presets = None
        self.label_type    = None

        self.led_control   = None
        self.led_midi      = None
        self.led_audio_in  = None
        self.led_audio_out = None

        self.peak_in  = None
        self.peak_out = None

        self.w_knobs_left  = None
        self.w_knobs_right = None

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
        self.fIsActive = bool(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_ACTIVE) >= 0.5)

        if self.b_enable is not None:
            self.b_enable.setChecked(self.fIsActive)
            self.b_enable.clicked.connect(self.slot_enableClicked)

            if "calf" in self.fSkinStyle and not isinstance(self, PluginSlot_Compact):
                self.b_enable.setPixmaps(":/bitmaps/button_calf3.png", ":/bitmaps/button_calf3_down.png", ":/bitmaps/button_calf3.png")
            else:
                self.b_enable.setPixmaps(":/bitmaps/button_off.png", ":/bitmaps/button_on.png", ":/bitmaps/button_off.png")

        if self.b_gui is not None:
            self.b_gui.clicked.connect(self.slot_showCustomUi)
            self.b_gui.setEnabled(bool(self.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI))

            if "calf" in self.fSkinStyle and not isinstance(self, PluginSlot_Compact):
                self.b_gui.setPixmaps(":/bitmaps/button_calf2.png", ":/bitmaps/button_calf2_down.png", ":/bitmaps/button_calf2_hover.png")
            elif self.fPluginInfo['iconName'] == "distrho" or self.fSkinStyle in ("3bandeq","3bandsplitter","pingpongpan"):
                self.b_gui.setPixmaps(":/bitmaps/button_distrho.png", ":/bitmaps/button_distrho_down.png", ":/bitmaps/button_distrho_hover.png")
            elif self.fPluginInfo['iconName'] == "file":
                self.b_gui.setPixmaps(":/bitmaps/button_file.png", ":/bitmaps/button_file_down.png", ":/bitmaps/button_file_hover.png")
            else:
                self.b_gui.setPixmaps(":/bitmaps/button_gui.png", ":/bitmaps/button_gui_down.png", ":/bitmaps/button_gui_hover.png")

        if self.b_edit is not None:
            self.b_edit.clicked.connect(self.slot_showEditDialog)

            if "calf" in self.fSkinStyle and not isinstance(self, PluginSlot_Compact):
                self.b_edit.setPixmaps(":/bitmaps/button_calf2.png", ":/bitmaps/button_calf2_down.png", ":/bitmaps/button_calf2_hover.png")
            else:
                self.b_edit.setPixmaps(":/bitmaps/button_edit.png", ":/bitmaps/button_edit_down.png", ":/bitmaps/button_edit_hover.png")

        else:
            # Edit button *must* be available
            self.b_edit = QPushButton(self)
            self.b_edit.setCheckable(True)
            self.b_edit.hide()

        if self.b_remove is not None:
            self.b_remove.clicked.connect(self.slot_removePlugin)

        if self.label_name is not None:
            self.label_name.setEnabled(self.fIsActive)
            self.label_name.setText(self.fPluginInfo['name'])

            nameFont = self.label_name.font()

            if self.fSkinStyle == "openav":
                QFontDatabase.addApplicationFont(":/fonts/uranium.ttf")
                nameFont.setFamily("Uranium")
                nameFont.setPixelSize(15)
                nameFont.setCapitalization(QFont.AllUppercase)

            elif "calf" in self.fSkinStyle:
                nameFont.setBold(True)
                nameFont.setPixelSize(12)

            else:
                nameFont.setBold(True)
                nameFont.setPixelSize(11)

            self.label_name.setFont(nameFont)

        if self.label_presets is not None:
            presetFont = self.label_presets.font()
            presetFont.setBold(True)
            presetFont.setPixelSize(10)
            self.label_presets.setFont(presetFont)

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

            if "calf" in self.fSkinStyle:
                self.peak_in.setMeterStyle(DigitalPeakMeter.STYLE_CALF)
            elif self.fSkinStyle == "rncbc":
                self.peak_in.setMeterStyle(DigitalPeakMeter.STYLE_RNCBC)
            elif self.fSkinStyle in ("mod", "openav", "zynfx"):
                self.peak_in.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)

            if self.fPeaksInputCount == 0 and not isinstance(self, PluginSlot_Classic):
                self.peak_in.hide()

        if self.peak_out is not None:
            self.peak_out.setChannelCount(self.fPeaksOutputCount)
            self.peak_out.setMeterColor(DigitalPeakMeter.COLOR_BLUE)
            self.peak_out.setMeterOrientation(DigitalPeakMeter.HORIZONTAL)

            if "calf" in self.fSkinStyle:
                self.peak_out.setMeterStyle(DigitalPeakMeter.STYLE_CALF)
            elif self.fSkinStyle == "rncbc":
                self.peak_out.setMeterStyle(DigitalPeakMeter.STYLE_RNCBC)
            elif self.fSkinStyle in ("mod", "openav", "zynfx"):
                self.peak_out.setMeterStyle(DigitalPeakMeter.STYLE_OPENAV)

            if self.fPeaksOutputCount == 0 and not isinstance(self, PluginSlot_Classic):
                self.peak_out.hide()

        # -------------------------------------------------------------

        if self.fSkinStyle == "mod":
            styleSheet = """
                QFrame#PluginWidget {
                    background-color: rgb(%i, %i, %i);
                }
                QLabel#label_name          { color: #FFFFFF; }
                QLabel#label_name:disabled { color: #505050; }
            """ % getModColorFromCategory(self.fPluginInfo['category'])

        elif self.fSkinStyle == "openav":
            styleSheet = """
                QFrame#PluginWidget {
                    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                                      stop: 0 #303030, stop: %f #111111, stop: 1.0 #111111);
                }
                QLabel#label_name          { color: #FF5100; }
                QLabel#label_name:disabled { color: #505050; }
            """ % (0.95 if isinstance(self, PluginSlot_Compact) else 0.35)

        else:
            if self.fSkinStyle in ("3bandeq", "calf_black", "calf_blue", "nekobi", "zynfx"):
                styleSheet2  = "background-image: url(:/bitmaps/background_%s.png);" % self.fSkinStyle
            else:
                styleSheet2  = "background-color: rgb(%i, %i, %i);" % getColorFromCategory(self.fPluginInfo['category'])
                styleSheet2 += "background-image: url(:/bitmaps/background_noise1.png);"

            styleSheet = """
                QFrame#PluginWidget {
                    %s
                    background-repeat: repeat-xy;
                }
                QLabel#label_name,
                QLabel#label_audio_in,
                QLabel#label_audio_out,
                QLabel#label_midi,
                QLabel#label_presets       { color: #BBB; }
                QLabel#label_name:disabled { color: #555; }
            """ % styleSheet2

        self.setStyleSheet(styleSheet)

        # -------------------------------------------------------------
        # Set-up parameters

        if self.w_knobs_left is not None:
            parameterCount = self.host.get_parameter_count(self.fPluginId)

            if "calf" in self.fSkinStyle:
                maxWidgets = 7
            else:
                maxWidgets = 8

            index = 0
            for i in range(parameterCount):
                if index >= maxWidgets:
                    break

                paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
                paramData   = self.host.get_parameter_data(self.fPluginId, i)
                paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)

                if paramData['type'] != PARAMETER_INPUT:
                    continue
                if paramData['hints'] & PARAMETER_IS_BOOLEAN:
                    continue
                if (paramData['hints'] & PARAMETER_IS_INTEGER) != 0 and paramRanges['max']-paramRanges['min'] <= 10.0:
                    continue
                if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                    continue
                if paramInfo['name'].startswith("unused"):
                    continue

                paramName = getParameterShortName(paramInfo['name'])

                widget = PixmapDial(self, i)
                widget.setLabel(paramName)
                widget.setMinimum(paramRanges['min'])
                widget.setMaximum(paramRanges['max'])
                setPixmapDialStyle(widget, i, parameterCount, self.fSkinStyle)

                index += 1
                self.fParameterList.append([i, widget])
                self.w_knobs_left.layout().addWidget(widget)

        if self.w_knobs_right is not None and (self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET) != 0:
            widget = PixmapDial(self, PARAMETER_DRYWET)
            widget.setLabel("Dry/Wet")
            widget.setMinimum(0.0)
            widget.setMaximum(1.0)
            setPixmapDialStyle(widget, PARAMETER_DRYWET, 0, self.fSkinStyle)

            self.fParameterList.append([PARAMETER_DRYWET, widget])
            self.w_knobs_right.layout().addWidget(widget)

        if self.w_knobs_right is not None and (self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME) != 0:
            widget = PixmapDial(self, PARAMETER_VOLUME)
            widget.setLabel("Volume")
            widget.setMinimum(0.0)
            widget.setMaximum(1.27)
            setPixmapDialStyle(widget, PARAMETER_VOLUME, 0, self.fSkinStyle)

            self.fParameterList.append([PARAMETER_VOLUME, widget])
            self.w_knobs_right.layout().addWidget(widget)

        for paramIndex, paramWidget in self.fParameterList:
            paramWidget.setContextMenuPolicy(Qt.CustomContextMenu)
            paramWidget.customContextMenuRequested.connect(self.slot_knobCustomMenu)
            paramWidget.realValueChanged.connect(self.slot_parameterValueChanged)
            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_internal_parameter_value(self.fPluginId, paramIndex))
            paramWidget.blockSignals(False)

        # -------------------------------------------------------------

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

        if self.label_name is not None:
            self.label_name.setEnabled(self.fIsActive)

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
        menu = QMenu(self)

        # -------------------------------------------------------------
        # Expand/Minimize

        actCompact = menu.addAction(self.tr("Expand") if isinstance(self, PluginSlot_Compact) else self.tr("Minimize"))
        menu.addSeparator()

        # -------------------------------------------------------------
        # Bypass and Enable/Disable

        actBypass = menu.addAction(self.tr("Bypass"))
        actEnable = menu.addAction(self.tr("Disable") if self.fIsActive else self.tr("Enable"))
        menu.addSeparator()

        if self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET:
            actBypass.setCheckable(True)
            actBypass.setChecked(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_DRYWET) == 0.0)
        else:
            actBypass.setVisible(False)

        # -------------------------------------------------------------
        # Reset and Randomize parameters

        actReset  = menu.addAction(self.tr("Reset parameters"))
        actRandom = menu.addAction(self.tr("Randomize parameters"))
        menu.addSeparator()

        # -------------------------------------------------------------
        # Edit and Show Custom UI

        actEdit = menu.addAction(self.tr("Edit"))
        actGui  = menu.addAction(self.tr("Show Custom UI"))
        menu.addSeparator()

        if self.b_edit is not None:
            actEdit.setCheckable(True)
            actEdit.setChecked(self.b_edit.isChecked())
        else:
            actEdit.setVisible(False)

        if self.b_gui is not None:
            actGui.setCheckable(True)
            actGui.setChecked(self.b_gui.isChecked())
            actGui.setEnabled(self.b_gui.isEnabled())
        else:
            actGui.setVisible(False)

        # -------------------------------------------------------------
        # Other stuff

        actClone   = menu.addAction(self.tr("Clone"))
        actReplace = menu.addAction(self.tr("Replace..."))
        actRename  = menu.addAction(self.tr("Rename..."))
        actRemove  = menu.addAction(self.tr("Remove"))

        if self.fIdleTimerId != 0:
            actRemove.setVisible(False)

        # -------------------------------------------------------------
        # exec

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            return

        # -------------------------------------------------------------
        # Expand/Minimize

        elif actSel == actCompact:
            # FIXME
            gCarla.gui.compactPlugin(self.fPluginId)

        # -------------------------------------------------------------
        # Bypass and Enable/Disable

        elif actSel == actBypass:
            value = 0.0 if actBypass.isChecked() else 1.0
            self.host.set_drywet(self.fPluginId, value)
            self.setParameterValue(PARAMETER_DRYWET, value, True)

        elif actSel == actEnable:
            self.setActive(not self.fIsActive, True, True)

        # -------------------------------------------------------------
        # Reset and Randomize parameters

        elif actSel == actReset:
            self.host.reset_parameters(self.fPluginId)

        elif actSel == actRandom:
            self.host.randomize_parameters(self.fPluginId)

        # -------------------------------------------------------------
        # Edit and Show Custom UI

        elif actSel == actEdit:
            self.b_edit.click()

        elif actSel == actGui:
            self.b_gui.click()

        # -------------------------------------------------------------
        # Clone

        elif actSel == actClone:
            if not self.host.clone_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        # -------------------------------------------------------------
        # Rename

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

        # -------------------------------------------------------------
        # Replace

        elif actSel == actReplace:
            # FIXME
            gCarla.gui.slot_pluginAdd(self.fPluginId)

        # -------------------------------------------------------------
        # Remove

        elif actSel == actRemove:
            if not self.host.remove_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        # -------------------------------------------------------------

    @pyqtSlot()
    def slot_knobCustomMenu(self):
        sender  = self.sender()
        index   = sender.fIndex
        minimum = sender.fMinimum
        maximum = sender.fMaximum
        current = sender.fRealValue
        label   = sender.fLabel

        if index in (PARAMETER_NULL, PARAMETER_CTRL_CHANNEL) or index <= PARAMETER_MAX:
            return
        elif index in (PARAMETER_DRYWET, PARAMETER_VOLUME):
            default = 1.0
        elif index == PARAMETER_BALANCE_LEFT:
            default = -1.0
        elif index == PARAMETER_BALANCE_RIGHT:
            default = 1.0
        elif index == PARAMETER_PANNING:
            default = 0.0
        else:
            default = self.host.get_default_parameter_value(self.fPluginId, index)

        if index < PARAMETER_NULL:
            # show in integer percentage
            textReset = self.tr("Reset (%i%%)"          % round(default*100.0))
            textMinim = self.tr("Set to Minimum (%i%%)" % round(minimum*100.0))
            textMaxim = self.tr("Set to Maximum (%i%%)" % round(maximum*100.0))
        else:
            # show in full float value
            textReset = self.tr("Reset (%f)"          % default)
            textMinim = self.tr("Set to Minimum (%f)" % minimum)
            textMaxim = self.tr("Set to Maximum (%f)" % maximum)

        menu = QMenu(self)
        actReset = menu.addAction(textReset)
        menu.addSeparator()
        actMinimum = menu.addAction(textMinim)
        actCenter  = menu.addAction(self.tr("Set to Center"))
        actMaximum = menu.addAction(textMaxim)
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        if index > PARAMETER_NULL or index not in (PARAMETER_BALANCE_LEFT, PARAMETER_BALANCE_RIGHT, PARAMETER_PANNING):
            menu.removeAction(actCenter)

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            if index < PARAMETER_NULL:
                value, ok = QInputDialog.getInt(self, self.tr("Set value"), label, round(current*100), round(minimum*100), round(maximum*100), 1)

                if not ok:
                    return

                value = float(value)/100.0

            else:
                paramInfo   = self.host.get_parameter_info(self.fPluginId, index)
                paramRanges = self.host.get_parameter_ranges(self.fPluginId, index)
                scalePoints = []

                for i in range(paramInfo['scalePointCount']):
                    scalePoints.append(self.host.get_parameter_scalepoint_info(self.fPluginId, index, i))

                dialog = CustomInputDialog(self, label, current, minimum, maximum,
                                                 paramRanges['step'], paramRanges['stepSmall'], scalePoints)

                if not dialog.exec_():
                    return

                value = dialog.returnValue()

        elif actSelected == actMinimum:
            value = minimum
        elif actSelected == actMaximum:
            value = maximum
        elif actSelected == actReset:
            value = default
        elif actSelected == actCenter:
            value = 0.0
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

    def mouseDoubleClickEvent(self, event):
        QFrame.mouseDoubleClickEvent(self, event)

        # FIXME
        gCarla.gui.compactPlugin(self.fPluginId)

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

class PluginSlot_Calf(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId, skinStyle):
        AbstractPluginSlot.__init__(self, parent, host, pluginId, skinStyle)
        self.ui = ui_carla_plugin_calf.Ui_PluginWidget()
        self.ui.setupUi(self)

        audioCount = self.host.get_audio_port_count_info(self.fPluginId)
        midiCount  = self.host.get_midi_port_count_info(self.fPluginId)

        # -------------------------------------------------------------
        # Internal stuff

        self.fButtonFont = self.ui.b_gui.font()
        self.fButtonFont.setBold(False)
        self.fButtonFont.setPixelSize(10)

        self.fButtonColorOn  = QColor( 18,  41,  87)
        self.fButtonColorOff = QColor(150, 150, 150)

        # -------------------------------------------------------------
        # Set-up GUI

        self.ui.label_active.setFont(self.fButtonFont)

        self.ui.b_remove.setPixmaps(":/bitmaps/button_calf1.png", ":/bitmaps/button_calf1_down.png", ":/bitmaps/button_calf1_hover.png")

        self.ui.b_edit.setTopText(self.tr("Edit"), self.fButtonColorOn, self.fButtonFont)
        self.ui.b_remove.setTopText(self.tr("Remove"), self.fButtonColorOn, self.fButtonFont)

        if self.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOn, self.fButtonFont)
        else:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOff, self.fButtonFont)

        if audioCount['ins'] == 0:
            self.ui.label_audio_in.hide()

        if audioCount['outs'] == 0:
            self.ui.label_audio_out.hide()

        if midiCount['ins'] == 0:
            self.ui.label_midi.hide()
            self.ui.led_midi.hide()

        if self.fIdleTimerId != 0:
            self.ui.b_remove.setEnabled(False)
            self.ui.b_remove.setVisible(False)

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit
        self.b_remove = self.ui.b_remove

        self.label_name = self.ui.label_name
        self.led_midi   = self.ui.led_midi

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        self.w_knobs_left  = self.ui.w_knobs

        self.ready()

        self.ui.led_midi.setColor(self.ui.led_midi.CALF)

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 94 if max(self.peak_in.channelCount(), self.peak_out.channelCount()) < 2 else 106

    #------------------------------------------------------------------

    def editDialogPluginHintsChanged(self, pluginId, hints):
        if hints & PLUGIN_HAS_CUSTOM_UI:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOn, self.fButtonFont)
        else:
            self.ui.b_gui.setTopText(self.tr("GUI"), self.fButtonColorOff, self.fButtonFont)

        AbstractPluginSlot.editDialogPluginHintsChanged(self, pluginId, hints)

    #------------------------------------------------------------------

    def paintEvent(self, event):
        isBlack = bool(self.fSkinStyle == "calf_black")

        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(20, 20, 20) if isBlack else QColor(75, 86, 99), 1))
        painter.drawRect(0, 1, self.width()-1, self.height()-3)

        painter.setPen(QPen(QColor(45, 45, 45) if isBlack else QColor(86, 99, 114), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Classic(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId):
        AbstractPluginSlot.__init__(self, parent, host, pluginId, "classic")
        self.ui = ui_carla_plugin_classic.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fColorTop    = QColor(60, 60, 60)
        self.fColorBottom = QColor(47, 47, 47)
        self.fColorSeprtr = QColor(70, 70, 70)

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

class PluginSlot_Compact(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId, skinStyle):
        AbstractPluginSlot.__init__(self, parent, host, pluginId, skinStyle)
        self.ui = ui_carla_plugin_compact.Ui_PluginWidget()
        self.ui.setupUi(self)

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
        if self.fSkinStyle == "calf_blue":
            return 36
        return 30

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Default(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId, skinStyle):
        AbstractPluginSlot.__init__(self, parent, host, pluginId, skinStyle)
        self.ui = ui_carla_plugin_default.Ui_PluginWidget()
        self.ui.setupUi(self)

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

        self.w_knobs_left  = self.ui.w_knobs_left
        self.w_knobs_right = self.ui.w_knobs_right

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        if self.fSkinStyle == "mod":
            return 86

        return 80

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(42, 42, 42), 1))
        painter.drawRect(0, 1, self.width()-1, self.getFixedHeight()-3)

        painter.setPen(QPen(QColor(60, 60, 60), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

class PluginSlot_Presets(AbstractPluginSlot):
    def __init__(self, parent, host, pluginId, skinStyle):
        AbstractPluginSlot.__init__(self, parent, host, pluginId, skinStyle)
        self.ui = ui_carla_plugin_presets.Ui_PluginWidget()
        self.ui.setupUi(self)

        usingMidiPrograms = bool(skinStyle != "presets")

        # -------------------------------------------------------------
        # Set-up programs

        if usingMidiPrograms:
            programCount = self.host.get_midi_program_count(self.fPluginId)
        else:
            programCount = self.host.get_program_count(self.fPluginId)

        if programCount > 0:
            self.ui.cb_presets.setEnabled(True)
            self.ui.label_presets.setEnabled(True)

            for i in range(programCount):
                if usingMidiPrograms:
                    progName = self.host.get_midi_program_data(self.fPluginId, i)['name']
                else:
                    progName = self.host.get_program_name(self.fPluginId, i)

                self.ui.cb_presets.addItem(progName)

            if usingMidiPrograms:
                curProg = self.host.get_current_midi_program_index(self.fPluginId)
            else:
                curProg = self.host.get_current_program_index(self.fPluginId)

            self.ui.cb_presets.setCurrentIndex(curProg)

        else:
            self.ui.cb_presets.setEnabled(False)
            self.ui.cb_presets.setVisible(False)
            self.ui.label_presets.setEnabled(False)
            self.ui.label_presets.setVisible(False)

        # -------------------------------------------------------------

        self.b_enable = self.ui.b_enable
        self.b_gui    = self.ui.b_gui
        self.b_edit   = self.ui.b_edit

        self.cb_presets = self.ui.cb_presets

        self.label_name    = self.ui.label_name
        self.label_presets = self.ui.label_presets

        self.led_control   = self.ui.led_control
        self.led_midi      = self.ui.led_midi
        self.led_audio_in  = self.ui.led_audio_in
        self.led_audio_out = self.ui.led_audio_out

        self.peak_in  = self.ui.peak_in
        self.peak_out = self.ui.peak_out

        if skinStyle == "zynfx":
            self.setupZynFxParams()
        else:
            self.w_knobs_left  = self.ui.w_knobs_left
            self.w_knobs_right = self.ui.w_knobs_right

        self.ready()

        self.customContextMenuRequested.connect(self.slot_showDefaultCustomMenu)

        if usingMidiPrograms:
            self.ui.cb_presets.currentIndexChanged.connect(self.slot_midiProgramChanged)
        else:
            self.ui.cb_presets.currentIndexChanged.connect(self.slot_programChanged)

        # -------------------------------------------------------------

    def setupZynFxParams(self):
        parameterCount = self.host.get_parameter_count(self.fPluginId)

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

            paramName = paramInfo['name']

            if paramName.startswith("unused"):
                continue

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
                paramName = getParameterShortName(paramName)

            widget = PixmapDial(self, i)

            widget.setLabel(paramName)
            widget.setMinimum(paramRanges['min'])
            widget.setMaximum(paramRanges['max'])
            widget.setPixmap(3)
            widget.setCustomPaintColor(QColor(83, 173, 10))
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_COLOR)
            widget.forceWhiteLabelGradientText()

            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                widget.setEnabled(False)

            self.fParameterList.append([i, widget])
            self.ui.w_knobs_left.layout().addWidget(widget)

        if self.fPluginInfo['hints'] & PLUGIN_CAN_DRYWET:
            widget = PixmapDial(self, PARAMETER_DRYWET)
            widget.setLabel("Wet")
            widget.setMinimum(0.0)
            widget.setMaximum(1.0)
            widget.setPixmap(3)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_WET)
            widget.forceWhiteLabelGradientText()

            self.fParameterList.append([PARAMETER_DRYWET, widget])
            self.ui.w_knobs_right.layout().addWidget(widget)

        if self.fPluginInfo['hints'] & PLUGIN_CAN_VOLUME:
            widget = PixmapDial(self, PARAMETER_VOLUME)
            widget.setLabel("Volume")
            widget.setMinimum(0.0)
            widget.setMaximum(1.27)
            widget.setPixmap(3)
            widget.setCustomPaintMode(PixmapDial.CUSTOM_PAINT_MODE_CARLA_VOL)
            widget.forceWhiteLabelGradientText()

            self.fParameterList.append([PARAMETER_VOLUME, widget])
            self.ui.w_knobs_right.layout().addWidget(widget)

    #------------------------------------------------------------------

    def getFixedHeight(self):
        return 80

    #------------------------------------------------------------------

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setBrush(Qt.transparent)

        painter.setPen(QPen(QColor(50, 50, 50), 1))
        painter.drawRect(0, 1, self.width()-1, self.height()-3)

        painter.setPen(QPen(QColor(64, 64, 64), 1))
        painter.drawLine(0, 0, self.width(), 0)

        AbstractPluginSlot.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------

def getSkinStyle(host, pluginId):
    if False:
        # kdevelop likes this :)
        host       = CarlaHostNull()
        progCount  = 0
        pluginInfo = PyCarlaPluginInfo
        pluginName = ""

    pluginInfo  = host.get_plugin_info(pluginId)
    pluginName  = host.get_real_plugin_name(pluginId)
    pluginLabel = pluginInfo['label'].lower()
    pluginMaker = pluginInfo['maker']
    uniqueId    = pluginInfo['uniqueId']

    if pluginInfo['type'] in (PLUGIN_VST2, PLUGIN_VST3, PLUGIN_AU):
        progCount = host.get_program_count(pluginId)
    else:
        progCount = host.get_midi_program_count(pluginId)

    # Samplers
    if pluginInfo['type'] == PLUGIN_GIG:
        return "gig"
    if pluginInfo['type'] == PLUGIN_SF2:
        return "sf2"
    if pluginInfo['type'] == PLUGIN_SFZ:
        return "sfz"

    # Calf
    if pluginName.split(" ", 1)[0].lower() == "calf":
        return "calf_black" if "mono" in pluginLabel else "calf_blue"

    # MOD
    #if pluginLabel.startswith("http://moddevices.com/") or pluginLabel.startswith("http://plugin.org.uk/swh-plugins/"):
        #return "mod"

    # OpenAV
    if pluginMaker == "OpenAV Productions":
        return "openav"

    # ZynFX
    if pluginInfo['type'] == PLUGIN_INTERNAL:
        if pluginLabel.startswith("zyn") and pluginInfo['category'] != PLUGIN_CATEGORY_SYNTH:
            return "zynfx"

    if pluginInfo['type'] == PLUGIN_LADSPA:
        if pluginLabel.startswith("zyn") and pluginMaker.startswith("Josep Andreu"):
            return "zynfx"

    if pluginInfo['type'] == PLUGIN_LV2:
        if pluginLabel.startswith("http://kxstudio.sf.net/carla/plugins/zyn") and pluginName != "ZynAddSubFX":
            return "zynfx"

    # Presets
    if progCount > 1 and (pluginInfo['hints'] & PLUGIN_USES_MULTI_PROGS) == 0:
        if pluginInfo['type'] in (PLUGIN_VST2, PLUGIN_VST3, PLUGIN_AU):
            return "presets"
        return "mpresets"

    # DISTRHO Plugins (needs to be last)
    if pluginMaker.startswith("falkTX, ") or pluginMaker == "DISTRHO" or pluginLabel.startswith("http://distrho.sf.net/plugins/"):
        return pluginLabel.replace("http://distrho.sf.net/plugins/","")

    return "default"

def createPluginSlot(parent, host, pluginId, options):
    if not options['useSkins']:
        return PluginSlot_Classic(parent, host, pluginId)

    skinStyle = getSkinStyle(host, pluginId)

    if "compact" in skinStyle or options['compact']:
        return PluginSlot_Compact(parent, host, pluginId, skinStyle)

    if "calf" in skinStyle:
        return PluginSlot_Calf(parent, host, pluginId, skinStyle)

    if skinStyle in ("mpresets", "presets", "zynfx"):
        return PluginSlot_Presets(parent, host, pluginId, skinStyle)

    return PluginSlot_Default(parent, host, pluginId, skinStyle)

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

    #gui = createPluginSlot(None, host, 0, True)
    gui = PluginSlot_Compact(None, host, 0, "default")
    gui.testTimer()
    gui.show()

    app.exec_()
