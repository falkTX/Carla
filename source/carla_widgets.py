#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla widgets code
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
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, pyqtWrapperType, Qt, QByteArray, QSettings, QTimer
    from PyQt5.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt5.QtWidgets import QDialog, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, pyqtWrapperType, Qt, QByteArray, QSettings, QTimer
    from PyQt4.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt4.QtGui import QDialog, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_about
import ui_carla_about_juce
import ui_carla_edit
import ui_carla_parameter

from carla_shared import *
from carla_utils import *
from paramspinbox import CustomInputDialog
from pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------------------
# Carla GUI defines

ICON_STATE_ON   = 3 # turns on, sets as wait
ICON_STATE_WAIT = 2 # nothing, sets as off
ICON_STATE_OFF  = 1 # turns off, sets as null
ICON_STATE_NULL = 0 # nothing

# ------------------------------------------------------------------------------------------------------------
# Carla About dialog

class CarlaAboutW(QDialog):
    def __init__(self, parent, host):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_about.Ui_CarlaAboutW()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()

        if host.isControl:
            extraInfo = " - <b>%s</b>" % self.tr("OSC Bridge Version")
        elif host.isPlugin:
            extraInfo = " - <b>%s</b>" % self.tr("Plugin Version")
        else:
            extraInfo = ""

        self.ui.l_about.setText(self.tr(""
                                     "<br>Version %s"
                                     "<br>Carla is a fully-featured audio plugin host%s.<br>"
                                     "<br>Copyright (C) 2011-2014 falkTX<br>"
                                     "" % (VERSION, extraInfo)))

        if host.isControl:
            self.ui.l_extended.hide()
            self.ui.tabWidget.removeTab(2)
            self.ui.tabWidget.removeTab(1)

        self.ui.l_extended.setText(gCarla.utils.get_complete_license_text())

        if host.is_engine_running() and not host.isControl:
            self.ui.le_osc_url_tcp.setText(host.get_host_osc_url_tcp())
            self.ui.le_osc_url_udp.setText(host.get_host_osc_url_udp())
        else:
            self.ui.le_osc_url_tcp.setText(self.tr("(Engine not running)"))
            self.ui.le_osc_url_udp.setText(self.tr("(Engine not running)"))

        self.ui.l_osc_cmds.setText("<table>"
                                   "<tr><td>" "/set_active"                 "&nbsp;</td><td>&lt;" "i-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_drywet"                 "&nbsp;</td><td>&lt;" "f-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_volume"                 "&nbsp;</td><td>&lt;" "f-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_balance_left"           "&nbsp;</td><td>&lt;" "f-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_balance_right"          "&nbsp;</td><td>&lt;" "f-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_panning"                "&nbsp;</td><td>&lt;" "f-value" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_parameter_value"        "&nbsp;</td><td>&lt;" "i-index" "&gt;</td><td>&lt;" "f-value"   "&gt;</td></tr>"
                                   "<tr><td>" "/set_parameter_midi_cc"      "&nbsp;</td><td>&lt;" "i-index" "&gt;</td><td>&lt;" "i-cc"      "&gt;</td></tr>"
                                   "<tr><td>" "/set_parameter_midi_channel" "&nbsp;</td><td>&lt;" "i-index" "&gt;</td><td>&lt;" "i-channel" "&gt;</td></tr>"
                                   "<tr><td>" "/set_program"                "&nbsp;</td><td>&lt;" "i-index" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/set_midi_program"           "&nbsp;</td><td>&lt;" "i-index" "&gt;</td><td>"                     "</td></tr>"
                                   "<tr><td>" "/note_on"                    "&nbsp;</td><td>&lt;" "i-note"  "&gt;</td><td>&lt;" "i-velo"    "&gt;</td></tr>"
                                   "<tr><td>" "/note_off"                   "&nbsp;</td><td>&lt;" "i-note"  "&gt;</td><td>"                     "</td></tr>"
                                   "</table>"
                                  )

        self.ui.l_example.setText("/Carla/2/set_parameter_value 5 1.0")
        self.ui.l_example_help.setText("<i>(as in this example, \"2\" is the plugin number and \"5\" the parameter)</i>")

        self.ui.l_ladspa.setText(self.tr("Everything! (Including LRDF)"))
        self.ui.l_dssi.setText(self.tr("Everything! (Including CustomData/Chunks)"))
        self.ui.l_lv2.setText(self.tr("About 110&#37; complete (using custom extensions)<br/>"
                                      "Implemented Feature/Extensions:"
                                      "<ul>"
                                      "<li>http://lv2plug.in/ns/ext/atom</li>"
                                      "<li>http://lv2plug.in/ns/ext/buf-size</li>"
                                      "<li>http://lv2plug.in/ns/ext/data-access</li>"
                                      #"<li>http://lv2plug.in/ns/ext/dynmanifest</li>"
                                      "<li>http://lv2plug.in/ns/ext/event</li>"
                                      "<li>http://lv2plug.in/ns/ext/instance-access</li>"
                                      "<li>http://lv2plug.in/ns/ext/log</li>"
                                      "<li>http://lv2plug.in/ns/ext/midi</li>"
                                      #"<li>http://lv2plug.in/ns/ext/morph</li>"
                                      "<li>http://lv2plug.in/ns/ext/options</li>"
                                      "<li>http://lv2plug.in/ns/ext/parameters</li>"
                                      #"<li>http://lv2plug.in/ns/ext/patch</li>"
                                      #"<li>http://lv2plug.in/ns/ext/port-groups</li>"
                                      "<li>http://lv2plug.in/ns/ext/port-props</li>"
                                      "<li>http://lv2plug.in/ns/ext/presets</li>"
                                      "<li>http://lv2plug.in/ns/ext/resize-port</li>"
                                      "<li>http://lv2plug.in/ns/ext/state</li>"
                                      "<li>http://lv2plug.in/ns/ext/time</li>"
                                      "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                      "<li>http://lv2plug.in/ns/ext/urid</li>"
                                      "<li>http://lv2plug.in/ns/ext/worker</li>"
                                      "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                      "<li>http://lv2plug.in/ns/extensions/units</li>"
                                      "<li>http://home.gna.org/lv2dynparam/rtmempool/v1</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/external-ui</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/props</li>"
                                      "<li>http://kxstudio.sf.net/ns/lv2ext/rtmempool</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                      "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                      "</ul>"))
        self.ui.l_vst.setText(self.tr("<p>About 85&#37; complete (missing vst bank/presets and some minor stuff)</p>"))

        # 2nd tab is usually longer than the 1st
        # adjust appropriately
        self.ui.tabWidget.setCurrentIndex(1)
        self.adjustSize()

        self.ui.tabWidget.setCurrentIndex(0)
        self.setFixedSize(self.size())

        if WINDOWS:
            self.setWindowFlags(self.windowFlags()|Qt.MSWindowsFixedSizeDialogHint)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# JUCE About dialog

class JuceAboutW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_about_juce.Ui_JuceAboutW()
        self.ui.setupUi(self)

        self.ui.l_text2.setText(self.tr("This program uses JUCE version %s." % gCarla.utils.get_juce_version()))

        self.adjustSize()
        self.setFixedSize(self.size())

        if WINDOWS:
            self.setWindowFlags(self.windowFlags()|Qt.MSWindowsFixedSizeDialogHint)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Parameter

class PluginParameter(QWidget):
    midiControlChanged = pyqtSignal(int, int)
    midiChannelChanged = pyqtSignal(int, int)
    valueChanged       = pyqtSignal(int, float)

    def __init__(self, parent, host, pInfo, pluginId, tabIndex):
        QWidget.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_parameter.Ui_PluginParameter()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        # -------------------------------------------------------------
        # Internal stuff

        self.fMidiControl = -1
        self.fMidiChannel = 1
        self.fParameterId = pInfo['index']
        self.fPluginId    = pluginId
        self.fTabIndex    = tabIndex

        # -------------------------------------------------------------
        # Set-up GUI

        pType  = pInfo['type']
        pHints = pInfo['hints']

        self.ui.label.setText(pInfo['name'])
        self.ui.widget.setName(pInfo['name'])

        self.ui.widget.setMinimum(pInfo['minimum'])
        self.ui.widget.setMaximum(pInfo['maximum'])
        self.ui.widget.setDefault(pInfo['default'])
        self.ui.widget.setValue(pInfo['current'])
        self.ui.widget.setLabel(pInfo['unit'])
        self.ui.widget.setStep(pInfo['step'])
        self.ui.widget.setStepSmall(pInfo['stepSmall'])
        self.ui.widget.setStepLarge(pInfo['stepLarge'])
        self.ui.widget.setScalePoints(pInfo['scalePoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

        if pType == PARAMETER_INPUT:
            if not pHints & PARAMETER_IS_ENABLED:
                self.ui.label.setEnabled(False)
                self.ui.widget.setEnabled(False)
                self.ui.widget.setReadOnly(True)
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

            elif not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

            if pHints & PARAMETER_IS_READ_ONLY:
                self.ui.widget.setReadOnly(True)

        elif pType == PARAMETER_OUTPUT:
            self.ui.widget.setReadOnly(True)

        else:
            self.ui.widget.setVisible(False)
            self.ui.sb_control.setVisible(False)
            self.ui.sb_channel.setVisible(False)

        if pHints & PARAMETER_USES_CUSTOM_TEXT and not host.isPlugin:
            self.ui.widget.setTextCallback(self._textCallBack)

        self.ui.widget.updateAll()

        self.setMidiControl(pInfo['midiCC'])
        self.setMidiChannel(pInfo['midiChannel'])

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.sb_control.customContextMenuRequested.connect(self.slot_controlSpinboxCustomMenu)
        self.ui.sb_channel.customContextMenuRequested.connect(self.slot_channelSpinboxCustomMenu)
        self.ui.sb_control.valueChanged.connect(self.slot_controlSpinboxChanged)
        self.ui.sb_channel.valueChanged.connect(self.slot_channelSpinboxChanged)
        self.ui.widget.valueChanged.connect(self.slot_widgetValueChanged)

        # -------------------------------------------------------------

    def getPluginId(self):
        return self.fPluginId

    def getTabIndex(self):
        return self.fTabIndex

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId

    def setDefault(self, value):
        self.ui.widget.setDefault(value)

    def setValue(self, value):
        self.ui.widget.blockSignals(True)
        self.ui.widget.setValue(value)
        self.ui.widget.blockSignals(False)

    def setMidiControl(self, control):
        self.fMidiControl = control
        self.ui.sb_control.blockSignals(True)
        self.ui.sb_control.setValue(control)
        self.ui.sb_control.blockSignals(False)

    def setMidiChannel(self, channel):
        self.fMidiChannel = channel
        self.ui.sb_channel.blockSignals(True)
        self.ui.sb_channel.setValue(channel)
        self.ui.sb_channel.blockSignals(False)

    def setLabelWidth(self, width):
        self.ui.label.setFixedWidth(width)

    @pyqtSlot()
    def slot_controlSpinboxCustomMenu(self):
        menu = QMenu(self)

        actNone = menu.addAction(self.tr("None"))

        if self.fMidiControl == -1:
            actNone.setCheckable(True)
            actNone.setChecked(True)

        for cc in MIDI_CC_LIST:
            action = menu.addAction(cc)

            if self.fMidiControl != -1 and int(cc.split(" ", 1)[0], 16) == self.fMidiControl:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            pass
        elif actSel == actNone:
            self.ui.sb_control.setValue(-1)
        else:
            selControlStr = actSel.text()
            selControl    = int(selControlStr.split(" ", 1)[0], 16)
            self.ui.sb_control.setValue(selControl)

    @pyqtSlot()
    def slot_channelSpinboxCustomMenu(self):
        menu = QMenu(self)

        for i in range(1, 16+1):
            action = menu.addAction("%i" % i)

            if self.fMidiChannel == i:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if actSel:
            selChannel = int(actSel.text())
            self.ui.sb_channel.setValue(selChannel)

    @pyqtSlot(int)
    def slot_controlSpinboxChanged(self, control):
        self.fMidiControl = control
        self.midiControlChanged.emit(self.fParameterId, control)

    @pyqtSlot(int)
    def slot_channelSpinboxChanged(self, channel):
        self.fMidiChannel = channel
        self.midiChannelChanged.emit(self.fParameterId, channel)

    @pyqtSlot(float)
    def slot_widgetValueChanged(self, value):
        self.valueChanged.emit(self.fParameterId, value)

    def _textCallBack(self):
        return self.host.get_parameter_text(self.fPluginId, self.fParameterId)

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor Parent (Meta class)

class PluginEditParentMeta():
#class PluginEditParentMeta(metaclass=ABCMeta):
    @abstractmethod
    def editDialogVisibilityChanged(self, pluginId, visible):
        raise NotImplementedError

    @abstractmethod
    def editDialogPluginHintsChanged(self, pluginId, hints):
        raise NotImplementedError

    @abstractmethod
    def editDialogParameterValueChanged(self, pluginId, parameterId, value):
        raise NotImplementedError

    @abstractmethod
    def editDialogProgramChanged(self, pluginId, index):
        raise NotImplementedError

    @abstractmethod
    def editDialogMidiProgramChanged(self, pluginId, index):
        raise NotImplementedError

    @abstractmethod
    def editDialogNotePressed(self, pluginId, note):
        raise NotImplementedError

    @abstractmethod
    def editDialogNoteReleased(self, pluginId, note):
        raise NotImplementedError

    @abstractmethod
    def editDialogMidiActivityChanged(self, pluginId, onOff):
        raise NotImplementedError

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor (Built-in)

class PluginEdit(QDialog):
    # settings
    kParamsPerPage = 8

    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    def __init__(self, parent, host, pluginId):
        QDialog.__init__(self, parent.window() if parent is not None else None)
        self.host = host
        self.ui = ui_carla_edit.Ui_PluginEdit()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            parent = PluginEditParentMeta()
            host = CarlaHostNull()
            self.host = host

        # -------------------------------------------------------------
        # Internal stuff

        self.fGeometry   = QByteArray()
        self.fParent     = parent
        self.fPluginId   = pluginId
        self.fPluginInfo = None

        self.fCurrentStateFilename = None
        self.fControlChannel = round(host.get_internal_parameter_value(pluginId, PARAMETER_CTRL_CHANNEL))
        self.fFirstInit      = True

        self.fParameterList      = [] # (type, id, widget)
        self.fParametersToUpdate = [] # (id, value)

        self.fPlayingNotes = [] # (channel, note)

        self.fTabIconOff    = QIcon(":/bitmaps/led_off.png")
        self.fTabIconOn     = QIcon(":/bitmaps/led_yellow.png")
        self.fTabIconTimers = []

        # used during testing
        self.fIdleTimerId = 0

        # -------------------------------------------------------------
        # Set-up GUI

        labelPluginFont = self.ui.label_plugin.font()
        labelPluginFont.setPixelSize(15)
        labelPluginFont.setWeight(75)
        self.ui.label_plugin.setFont(labelPluginFont)

        self.ui.dial_drywet.setCustomPaintMode(self.ui.dial_drywet.CUSTOM_PAINT_MODE_CARLA_WET)
        self.ui.dial_drywet.setPixmap(3)
        self.ui.dial_drywet.setLabel("Dry/Wet")
        self.ui.dial_drywet.setMinimum(0.0)
        self.ui.dial_drywet.setMaximum(1.0)
        self.ui.dial_drywet.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_DRYWET))

        self.ui.dial_vol.setCustomPaintMode(self.ui.dial_vol.CUSTOM_PAINT_MODE_CARLA_VOL)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_VOLUME))

        self.ui.dial_b_left.setCustomPaintMode(self.ui.dial_b_left.CUSTOM_PAINT_MODE_CARLA_L)
        self.ui.dial_b_left.setPixmap(4)
        self.ui.dial_b_left.setLabel("L")
        self.ui.dial_b_left.setMinimum(-1.0)
        self.ui.dial_b_left.setMaximum(1.0)
        self.ui.dial_b_left.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_BALANCE_LEFT))

        self.ui.dial_b_right.setCustomPaintMode(self.ui.dial_b_right.CUSTOM_PAINT_MODE_CARLA_R)
        self.ui.dial_b_right.setPixmap(4)
        self.ui.dial_b_right.setLabel("R")
        self.ui.dial_b_right.setMinimum(-1.0)
        self.ui.dial_b_right.setMaximum(1.0)
        self.ui.dial_b_right.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_BALANCE_RIGHT))

        self.ui.dial_pan.setCustomPaintMode(self.ui.dial_b_right.CUSTOM_PAINT_MODE_CARLA_PAN)
        self.ui.dial_pan.setPixmap(4)
        self.ui.dial_pan.setLabel("Pan")
        self.ui.dial_pan.setMinimum(-1.0)
        self.ui.dial_pan.setMaximum(1.0)
        self.ui.dial_pan.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_PANNING))

        self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)

        self.ui.scrollArea = PixmapKeyboardHArea(self)
        self.ui.keyboard   = self.ui.scrollArea.keyboard
        self.ui.keyboard.setEnabled(self.fControlChannel >= 0)
        self.layout().addWidget(self.ui.scrollArea)

        self.ui.scrollArea.setEnabled(False)
        self.ui.scrollArea.setVisible(False)

        # todo
        self.ui.rb_balance.setEnabled(False)
        self.ui.rb_balance.setVisible(False)
        self.ui.rb_pan.setEnabled(False)
        self.ui.rb_pan.setVisible(False)

        self.reloadAll()

        self.fFirstInit = False

        # -------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_finished)

        self.ui.ch_fixed_buffer.clicked.connect(self.slot_optionChanged)
        self.ui.ch_force_stereo.clicked.connect(self.slot_optionChanged)
        self.ui.ch_map_program_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_use_chunks.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_program_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_control_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_channel_pressure.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_note_aftertouch.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_pitchbend.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_all_sound_off.clicked.connect(self.slot_optionChanged)

        self.ui.dial_drywet.realValueChanged.connect(self.slot_dryWetChanged)
        self.ui.dial_vol.realValueChanged.connect(self.slot_volumeChanged)
        self.ui.dial_b_left.realValueChanged.connect(self.slot_balanceLeftChanged)
        self.ui.dial_b_right.realValueChanged.connect(self.slot_balanceRightChanged)
        self.ui.dial_pan.realValueChanged.connect(self.slot_panChanged)
        self.ui.sb_ctrl_channel.valueChanged.connect(self.slot_ctrlChannelChanged)

        self.ui.dial_drywet.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_vol.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_left.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_right.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_pan.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.sb_ctrl_channel.customContextMenuRequested.connect(self.slot_channelCustomMenu)

        self.ui.keyboard.noteOn.connect(self.slot_noteOn)
        self.ui.keyboard.noteOff.connect(self.slot_noteOff)

        self.ui.cb_programs.currentIndexChanged.connect(self.slot_programIndexChanged)
        self.ui.cb_midi_programs.currentIndexChanged.connect(self.slot_midiProgramIndexChanged)

        self.ui.b_save_state.clicked.connect(self.slot_stateSave)
        self.ui.b_load_state.clicked.connect(self.slot_stateLoad)

        #host.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        #host.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        host.UpdateCallback.connect(self.slot_handleUpdateCallback)
        host.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        host.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        host.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        host.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

    #------------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velocity):
        if self.fPluginId != pluginId: return

        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOn(note, False)

        playItem = (channel, note)

        if playItem not in self.fPlayingNotes:
            self.fPlayingNotes.append(playItem)

        if len(self.fPlayingNotes) == 1 and self.fParent is not None:
            self.fParent.editDialogMidiActivityChanged(self.fPluginId, True)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if self.fPluginId != pluginId: return

        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOff(note, False)

        playItem = (channel, note)

        if playItem in self.fPlayingNotes:
            self.fPlayingNotes.remove(playItem)

        if len(self.fPlayingNotes) == 0 and self.fParent is not None:
            self.fParent.editDialogMidiActivityChanged(self.fPluginId, False)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadAll()

    #------------------------------------------------------------------

    def updateInfo(self):
        # Update current program text
        if self.ui.cb_programs.count() > 0:
            pIndex = self.ui.cb_programs.currentIndex()
            pName  = self.host.get_program_name(self.fPluginId, pIndex)
            #pName  = pName[:40] + (pName[40:] and "...")
            self.ui.cb_programs.setItemText(pIndex, pName)

        # Update current midi program text
        if self.ui.cb_midi_programs.count() > 0:
            mpIndex = self.ui.cb_midi_programs.currentIndex()
            mpData  = self.host.get_midi_program_data(self.fPluginId, mpIndex)
            mpBank  = mpData['bank']
            mpProg  = mpData['program']
            mpName  = mpData['name']
            #mpName  = mpName[:40] + (mpName[40:] and "...")
            self.ui.cb_midi_programs.setItemText(mpIndex, "%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

        # Update all parameter values
        for paramType, paramId, paramWidget in self.fParameterList:
            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

        # and the internal ones too
        self.ui.dial_drywet.blockSignals(True)
        self.ui.dial_drywet.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_DRYWET))
        self.ui.dial_drywet.blockSignals(False)

        self.ui.dial_vol.blockSignals(True)
        self.ui.dial_vol.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_VOLUME))
        self.ui.dial_vol.blockSignals(False)

        self.ui.dial_b_left.blockSignals(True)
        self.ui.dial_b_left.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_BALANCE_LEFT))
        self.ui.dial_b_left.blockSignals(False)

        self.ui.dial_b_right.blockSignals(True)
        self.ui.dial_b_right.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_BALANCE_RIGHT))
        self.ui.dial_b_right.blockSignals(False)

        self.ui.dial_pan.blockSignals(True)
        self.ui.dial_pan.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_PANNING))
        self.ui.dial_pan.blockSignals(False)

        self.fControlChannel = round(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_CTRL_CHANNEL))
        self.ui.sb_ctrl_channel.blockSignals(True)
        self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)
        self.ui.sb_ctrl_channel.blockSignals(False)
        self.ui.keyboard.allNotesOff()
        self._updateCtrlPrograms()

        self.fParametersToUpdate = []

    #------------------------------------------------------------------

    def reloadAll(self):
        self.fPluginInfo = self.host.get_plugin_info(self.fPluginId)

        self.reloadInfo()
        self.reloadParameters()
        self.reloadPrograms()

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_save_state.setEnabled(False)

        if not self.ui.scrollArea.isEnabled():
            self.resize(self.width(), self.height()-self.ui.scrollArea.height())

        # Workaround for a Qt4 bug, see https://bugreports.qt-project.org/browse/QTBUG-7792
        if LINUX: QTimer.singleShot(0, self.slot_fixNameWordWrap)

    @pyqtSlot()
    def slot_fixNameWordWrap(self):
        self.adjustSize()
        self.setMinimumSize(self.width(), self.height())

    #------------------------------------------------------------------

    def reloadInfo(self):
        realPluginName = self.host.get_real_plugin_name(self.fPluginId)
        #audioCountInfo = self.host.get_audio_port_count_info(self.fPluginId)
        midiCountInfo  = self.host.get_midi_port_count_info(self.fPluginId)
        #paramCountInfo = self.host.get_parameter_count_info(self.fPluginId)

        pluginHints = self.fPluginInfo['hints']

        self.ui.le_type.setText(getPluginTypeAsString(self.fPluginInfo['type']))

        self.ui.label_name.setEnabled(bool(realPluginName))
        self.ui.le_name.setEnabled(bool(realPluginName))
        self.ui.le_name.setText(realPluginName)
        self.ui.le_name.setToolTip(realPluginName)

        self.ui.label_label.setEnabled(bool(self.fPluginInfo['label']))
        self.ui.le_label.setEnabled(bool(self.fPluginInfo['label']))
        self.ui.le_label.setText(self.fPluginInfo['label'])
        self.ui.le_label.setToolTip(self.fPluginInfo['label'])

        self.ui.label_maker.setEnabled(bool(self.fPluginInfo['maker']))
        self.ui.le_maker.setEnabled(bool(self.fPluginInfo['maker']))
        self.ui.le_maker.setText(self.fPluginInfo['maker'])
        self.ui.le_maker.setToolTip(self.fPluginInfo['maker'])

        self.ui.label_copyright.setEnabled(bool(self.fPluginInfo['copyright']))
        self.ui.le_copyright.setEnabled(bool(self.fPluginInfo['copyright']))
        self.ui.le_copyright.setText(self.fPluginInfo['copyright'])
        self.ui.le_copyright.setToolTip(self.fPluginInfo['copyright'])

        self.ui.label_unique_id.setEnabled(bool(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setEnabled(bool(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setText(str(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setToolTip(str(self.fPluginInfo['uniqueId']))

        self.ui.label_plugin.setText("\n%s\n" % (self.fPluginInfo['name'] or "(none)"))
        self.setWindowTitle(self.fPluginInfo['name'] or "(none)")

        self.ui.dial_drywet.setEnabled(pluginHints & PLUGIN_CAN_DRYWET)
        self.ui.dial_vol.setEnabled(pluginHints & PLUGIN_CAN_VOLUME)
        self.ui.dial_b_left.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_b_right.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_pan.setEnabled(pluginHints & PLUGIN_CAN_PANNING)

        self.ui.ch_use_chunks.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_USE_CHUNKS)
        self.ui.ch_use_chunks.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_USE_CHUNKS)
        self.ui.ch_fixed_buffer.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_fixed_buffer.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_force_stereo.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_force_stereo.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_map_program_changes.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_map_program_changes.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_send_control_changes.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
        self.ui.ch_send_control_changes.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
        self.ui.ch_send_channel_pressure.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
        self.ui.ch_send_channel_pressure.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
        self.ui.ch_send_note_aftertouch.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
        self.ui.ch_send_note_aftertouch.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
        self.ui.ch_send_pitchbend.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_PITCHBEND)
        self.ui.ch_send_pitchbend.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_PITCHBEND)
        self.ui.ch_send_all_sound_off.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
        self.ui.ch_send_all_sound_off.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)

        canSendPrograms = bool((self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) != 0 and
                               (self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) == 0)
        self.ui.ch_send_program_changes.setEnabled(canSendPrograms)
        self.ui.ch_send_program_changes.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)

        self.ui.sw_programs.setCurrentIndex(0 if self.fPluginInfo['type'] in (PLUGIN_VST2, PLUGIN_GIG, PLUGIN_SFZ) else 1)

        # Show/hide keyboard
        showKeyboard = (self.fPluginInfo['category'] == PLUGIN_CATEGORY_SYNTH or midiCountInfo['ins'] > 0 < midiCountInfo['outs'])
        self.ui.scrollArea.setEnabled(showKeyboard)
        self.ui.scrollArea.setVisible(showKeyboard)

        # Force-update parent for new hints
        if self.fParent is not None and not self.fFirstInit:
            self.fParent.editDialogPluginHintsChanged(self.fPluginId, pluginHints)

    def reloadParameters(self):
        # Reset
        self.fParameterList      = []
        self.fParametersToUpdate = []
        self.fTabIconTimers      = []

        # Remove all previous parameters
        for x in range(self.ui.tabWidget.count()-1):
            self.ui.tabWidget.widget(1).deleteLater()
            self.ui.tabWidget.removeTab(1)

        parameterCount = self.host.get_parameter_count(self.fPluginId)

        # -----------------------------------------------------------------

        if parameterCount <= 0:
            return

        # -----------------------------------------------------------------

        paramInputList   = []
        paramOutputList  = []
        paramInputWidth  = 0
        paramOutputWidth = 0

        paramInputListFull  = [] # ([params], width)
        paramOutputListFull = [] # ([params], width)

        for i in range(min(parameterCount, self.host.maxParameters)):
            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramData   = self.host.get_parameter_data(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)
            paramValue  = self.host.get_current_parameter_value(self.fPluginId, i)

            if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                continue

            parameter = {
                'type':  paramData['type'],
                'hints': paramData['hints'],
                'name':  paramInfo['name'],
                'unit':  paramInfo['unit'],
                'scalePoints': [],

                'index':   paramData['index'],
                'default': paramRanges['def'],
                'minimum': paramRanges['min'],
                'maximum': paramRanges['max'],
                'step':    paramRanges['step'],
                'stepSmall': paramRanges['stepSmall'],
                'stepLarge': paramRanges['stepLarge'],
                'midiCC':    paramData['midiCC'],
                'midiChannel': paramData['midiChannel']+1,

                'current': paramValue
            }

            for j in range(paramInfo['scalePointCount']):
                scalePointInfo = self.host.get_parameter_scalepoint_info(self.fPluginId, i, j)

                parameter['scalePoints'].append({
                    'value': scalePointInfo['value'],
                    'label': scalePointInfo['label']
                })

            #parameter['name'] = parameter['name'][:30] + (parameter['name'][30:] and "...")

            # -----------------------------------------------------------------
            # Get width values, in packs of 10

            if parameter['type'] == PARAMETER_INPUT:
                paramInputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                if paramInputWidthTMP > paramInputWidth:
                    paramInputWidth = paramInputWidthTMP

                paramInputList.append(parameter)

                if len(paramInputList) == self.kParamsPerPage:
                    paramInputListFull.append((paramInputList, paramInputWidth))
                    paramInputList  = []
                    paramInputWidth = 0

            else:
                paramOutputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                if paramOutputWidthTMP > paramOutputWidth:
                    paramOutputWidth = paramOutputWidthTMP

                paramOutputList.append(parameter)

                if len(paramOutputList) == self.kParamsPerPage:
                    paramOutputListFull.append((paramOutputList, paramOutputWidth))
                    paramOutputList  = []
                    paramOutputWidth = 0

        # for i in range(parameterCount)
        else:
            # Final page width values
            if 0 < len(paramInputList) < 10:
                paramInputListFull.append((paramInputList, paramInputWidth))

            if 0 < len(paramOutputList) < 10:
                paramOutputListFull.append((paramOutputList, paramOutputWidth))

        # Create parameter tabs + widgets
        self._createParameterWidgets(PARAMETER_INPUT,  paramInputListFull,  self.tr("Parameters"))
        self._createParameterWidgets(PARAMETER_OUTPUT, paramOutputListFull, self.tr("Outputs"))

    def reloadPrograms(self):
        # Programs
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.clear()

        programCount = self.host.get_program_count(self.fPluginId)

        if programCount > 0:
            self.ui.cb_programs.setEnabled(True)
            self.ui.label_programs.setEnabled(True)

            for i in range(programCount):
                pName = self.host.get_program_name(self.fPluginId, i)
                #pName = pName[:40] + (pName[40:] and "...")
                self.ui.cb_programs.addItem(pName)

            self.ui.cb_programs.setCurrentIndex(self.host.get_current_program_index(self.fPluginId))

        else:
            self.ui.cb_programs.setEnabled(False)
            self.ui.label_programs.setEnabled(False)

        self.ui.cb_programs.blockSignals(False)

        # MIDI Programs
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.clear()

        midiProgramCount = self.host.get_midi_program_count(self.fPluginId)

        if midiProgramCount > 0:
            self.ui.cb_midi_programs.setEnabled(True)
            self.ui.label_midi_programs.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = self.host.get_midi_program_data(self.fPluginId, i)
                mpBank = mpData['bank']
                mpProg = mpData['program']
                mpName = mpData['name']
                #mpName = mpName[:40] + (mpName[40:] and "...")

                self.ui.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

            self.ui.cb_midi_programs.setCurrentIndex(self.host.get_current_midi_program_index(self.fPluginId))

        else:
            self.ui.cb_midi_programs.setEnabled(False)
            self.ui.label_midi_programs.setEnabled(False)

        self.ui.cb_midi_programs.blockSignals(False)

        self.ui.sw_programs.setEnabled(programCount > 0 or midiProgramCount > 0)

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_load_state.setEnabled(programCount > 0)

    #------------------------------------------------------------------

    def clearNotes(self):
         self.fPlayingNotes = []
         self.ui.keyboard.allNotesOff()

    def noteOn(self, channel, note, velocity):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOn(note, False)

    def noteOff(self, channel, note):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOff(note, False)

    #------------------------------------------------------------------

    def getHints(self):
        return self.fPluginInfo['hints']

    def setPluginId(self, idx):
        self.fPluginId = idx

    def setName(self, name):
        self.fPluginInfo['name'] = name
        self.ui.label_plugin.setText("\n%s\n" % name)
        self.setWindowTitle(name)

    #------------------------------------------------------------------

    def setParameterValue(self, parameterId, value):
        for paramItem in self.fParametersToUpdate:
            if paramItem[0] == parameterId:
                paramItem[1] = value
                break
        else:
            self.fParametersToUpdate.append([parameterId, value])

    def setParameterDefault(self, parameterId, value):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setDefault(value)
                break

    def setParameterMidiControl(self, parameterId, control):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMidiControl(control)
                break

    def setParameterMidiChannel(self, parameterId, channel):
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMidiChannel(channel+1)
                break

    def setProgram(self, index):
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.setCurrentIndex(index)
        self.ui.cb_programs.blockSignals(False)
        self._updateParameterValues()

    def setMidiProgram(self, index):
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.setCurrentIndex(index)
        self.ui.cb_midi_programs.blockSignals(False)
        self._updateParameterValues()

    def setOption(self, option, yesNo):
        if option == PLUGIN_OPTION_USE_CHUNKS:
            widget = self.ui.ch_use_chunks
        elif option == PLUGIN_OPTION_FIXED_BUFFERS:
            widget = self.ui.ch_fixed_buffer
        elif option == PLUGIN_OPTION_FORCE_STEREO:
            widget = self.ui.ch_force_stereo
        elif option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
            widget = self.ui.ch_map_program_changes
        elif option == PLUGIN_OPTION_SEND_PROGRAM_CHANGES:
            widget = self.ui.ch_send_program_changes
        elif option == PLUGIN_OPTION_SEND_CONTROL_CHANGES:
            widget = self.ui.ch_send_control_changes
        elif option == PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
            widget = self.ui.ch_send_channel_pressure
        elif option == PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
            widget = self.ui.ch_send_note_aftertouch
        elif option == PLUGIN_OPTION_SEND_PITCHBEND:
            widget = self.ui.ch_send_pitchbend
        elif option == PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
            widget = self.ui.ch_send_all_sound_off
        else:
            return

        widget.blockSignals(True)
        widget.setChecked(yesNo)
        widget.blockSignals(False)

    #------------------------------------------------------------------

    def setVisible(self, yesNo):
        if yesNo:
            if not self.fGeometry.isNull():
                self.restoreGeometry(self.fGeometry)
        else:
            self.fGeometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

    #------------------------------------------------------------------

    def idleSlow(self):
        # Check Tab icons
        for i in range(len(self.fTabIconTimers)):
            if self.fTabIconTimers[i] == ICON_STATE_ON:
                self.fTabIconTimers[i] = ICON_STATE_WAIT
            elif self.fTabIconTimers[i] == ICON_STATE_WAIT:
                self.fTabIconTimers[i] = ICON_STATE_OFF
            elif self.fTabIconTimers[i] == ICON_STATE_OFF:
                self.fTabIconTimers[i] = ICON_STATE_NULL
                self.ui.tabWidget.setTabIcon(i+1, self.fTabIconOff)

        # Check parameters needing update
        for index, value in self.fParametersToUpdate:
            if index == PARAMETER_DRYWET:
                self.ui.dial_drywet.blockSignals(True)
                self.ui.dial_drywet.setValue(value)
                self.ui.dial_drywet.blockSignals(False)

            elif index == PARAMETER_VOLUME:
                self.ui.dial_vol.blockSignals(True)
                self.ui.dial_vol.setValue(value)
                self.ui.dial_vol.blockSignals(False)

            elif index == PARAMETER_BALANCE_LEFT:
                self.ui.dial_b_left.blockSignals(True)
                self.ui.dial_b_left.setValue(value)
                self.ui.dial_b_left.blockSignals(False)

            elif index == PARAMETER_BALANCE_RIGHT:
                self.ui.dial_b_right.blockSignals(True)
                self.ui.dial_b_right.setValue(value)
                self.ui.dial_b_right.blockSignals(False)

            elif index == PARAMETER_PANNING:
                self.ui.dial_pan.blockSignals(True)
                self.ui.dial_pan.setValue(value)
                self.ui.dial_pan.blockSignals(False)

            elif index == PARAMETER_CTRL_CHANNEL:
                self.fControlChannel = round(value)
                self.ui.sb_ctrl_channel.blockSignals(True)
                self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)
                self.ui.sb_ctrl_channel.blockSignals(False)
                self.ui.keyboard.allNotesOff()
                self._updateCtrlPrograms()

            elif index >= 0:
                for paramType, paramId, paramWidget in self.fParameterList:
                    if paramId != index:
                        continue
                    # FIXME see below
                    if paramType != PARAMETER_INPUT:
                        continue

                    paramWidget.blockSignals(True)
                    paramWidget.setValue(value)
                    paramWidget.blockSignals(False)

                    #if paramType == PARAMETER_INPUT:
                    tabIndex = paramWidget.getTabIndex()

                    if self.fTabIconTimers[tabIndex-1] == ICON_STATE_NULL:
                        self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOn)

                    self.fTabIconTimers[tabIndex-1] = ICON_STATE_ON
                    break

        # Clear all parameters
        self.fParametersToUpdate = []

        # Update parameter outputs | FIXME needed?
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramType != PARAMETER_OUTPUT:
                continue

            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_stateSave(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            # TODO
            return

        if self.fCurrentStateFilename:
            askTry = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if askTry == QMessageBox.Ok:
                self.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)
                return

            self.fCurrentStateFilename = None

        fileFilter = self.tr("Carla State File (*.carxs)")
        filename   = QFileDialog.getSaveFileName(self, self.tr("Save Plugin State File"), filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        if not filename.lower().endswith(".carxs"):
            filename += ".carxs"

        self.fCurrentStateFilename = filename
        self.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    @pyqtSlot()
    def slot_stateLoad(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            presetList = []

            for i in range(self.host.get_program_count(self.fPluginId)):
                presetList.append("%03i - %s" % (i+1, self.host.get_program_name(self.fPluginId, i)))

            ret = QInputDialog.getItem(self, self.tr("Open LV2 Preset"), self.tr("Select an LV2 Preset:"), presetList, 0, False)

            if ret[1]:
                index = int(ret[0].split(" - ", 1)[0])-1
                self.host.set_midi_program(self.fPluginId, -1)
                self.host.set_program(self.fPluginId, index)
                self.setMidiProgram(-1)

            return

        fileFilter = self.tr("Carla State File (*.carxs)")
        filename   = QFileDialog.getOpenFileName(self, self.tr("Open Plugin State File"), filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        self.fCurrentStateFilename = filename
        self.host.load_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_optionChanged(self, clicked):
        sender = self.sender()

        if sender == self.ui.ch_use_chunks:
            option = PLUGIN_OPTION_USE_CHUNKS
        elif sender == self.ui.ch_fixed_buffer:
            option = PLUGIN_OPTION_FIXED_BUFFERS
        elif sender == self.ui.ch_force_stereo:
            option = PLUGIN_OPTION_FORCE_STEREO
        elif sender == self.ui.ch_map_program_changes:
            option = PLUGIN_OPTION_MAP_PROGRAM_CHANGES
        elif sender == self.ui.ch_send_program_changes:
            option = PLUGIN_OPTION_SEND_PROGRAM_CHANGES
        elif sender == self.ui.ch_send_control_changes:
            option = PLUGIN_OPTION_SEND_CONTROL_CHANGES
        elif sender == self.ui.ch_send_channel_pressure:
            option = PLUGIN_OPTION_SEND_CHANNEL_PRESSURE
        elif sender == self.ui.ch_send_note_aftertouch:
            option = PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH
        elif sender == self.ui.ch_send_pitchbend:
            option = PLUGIN_OPTION_SEND_PITCHBEND
        elif sender == self.ui.ch_send_all_sound_off:
            option = PLUGIN_OPTION_SEND_ALL_SOUND_OFF
        else:
            return

        #--------------------------------------------------------------
        # handle map-program-changes and send-program-changes conflict

        if option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES and clicked:
            self.ui.ch_send_program_changes.setEnabled(False)

            # disable send-program-changes if needed
            if self.ui.ch_send_program_changes.isChecked():
                self.host.set_option(self.fPluginId, PLUGIN_OPTION_SEND_PROGRAM_CHANGES, False)

        #--------------------------------------------------------------
        # set option

        self.host.set_option(self.fPluginId, option, clicked)

        #--------------------------------------------------------------
        # handle map-program-changes and send-program-changes conflict

        if option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES and not clicked:
            self.ui.ch_send_program_changes.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)

            # restore send-program-changes if needed
            if self.ui.ch_send_program_changes.isChecked():
                self.host.set_option(self.fPluginId, PLUGIN_OPTION_SEND_PROGRAM_CHANGES, True)

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_dryWetChanged(self, value):
        self.host.set_drywet(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_DRYWET, value)

    @pyqtSlot(int)
    def slot_volumeChanged(self, value):
        self.host.set_volume(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_VOLUME, value)

    @pyqtSlot(int)
    def slot_balanceLeftChanged(self, value):
        self.host.set_balance_left(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_BALANCE_LEFT, value)

    @pyqtSlot(int)
    def slot_balanceRightChanged(self, value):
        self.host.set_balance_right(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_BALANCE_RIGHT, value)

    @pyqtSlot(int)
    def slot_panChanged(self, value):
        self.host.set_panning(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_PANNING, value)

    @pyqtSlot(int)
    def slot_ctrlChannelChanged(self, value):
        self.fControlChannel = value-1
        self.host.set_ctrl_channel(self.fPluginId, self.fControlChannel)

        self.ui.keyboard.allNotesOff()
        self._updateCtrlPrograms()

    #------------------------------------------------------------------

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameterId, value):
        self.host.set_parameter_value(self.fPluginId, parameterId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, parameterId, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiControlChanged(self, parameterId, control):
        self.host.set_parameter_midi_cc(self.fPluginId, parameterId, control)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameterId, channel):
        self.host.set_parameter_midi_channel(self.fPluginId, parameterId, channel-1)

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        self.host.set_program(self.fPluginId, index)

        if self.fParent is not None:
            self.fParent.editDialogProgramChanged(self.fPluginId, index)

        self._updateParameterValues()

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        self.host.set_midi_program(self.fPluginId, index)

        if self.fParent is not None:
            self.fParent.editDialogMidiProgramChanged(self.fPluginId, index)

        self._updateParameterValues()

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        if self.fControlChannel >= 0:
            self.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 100)

        if self.fParent is not None:
            self.fParent.editDialogNotePressed(self.fPluginId, note)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        if self.fControlChannel >= 0:
            self.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 0)

        if self.fParent is not None:
            self.fParent.editDialogNoteReleased(self.fPluginId, note)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_finished(self):
        if self.fParent is not None:
            self.fParent.editDialogVisibilityChanged(self.fPluginId, False)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_knobCustomMenu(self):
        sender   = self.sender()
        knobName = sender.objectName()

        if knobName == "dial_drywet":
            minimum = 0.0
            maximum = 1.0
            default = 1.0
            label   = "Dry/Wet"
        elif knobName == "dial_vol":
            minimum = 0.0
            maximum = 1.27
            default = 1.0
            label   = "Volume"
        elif knobName == "dial_b_left":
            minimum = -1.0
            maximum = 1.0
            default = -1.0
            label   = "Balance-Left"
        elif knobName == "dial_b_right":
            minimum = -1.0
            maximum = 1.0
            default = 1.0
            label   = "Balance-Right"
        elif knobName == "dial_pan":
            minimum = -1.0
            maximum = 1.0
            default = 0.0
            label   = "Panning"
        else:
            minimum = 0.0
            maximum = 1.0
            default = 0.5
            label   = "Unknown"

        menu = QMenu(self)
        actReset = menu.addAction(self.tr("Reset (%i%%)" % (default*100)))
        menu.addSeparator()
        actMinimum = menu.addAction(self.tr("Set to Minimum (%i%%)" % (minimum*100)))
        actCenter  = menu.addAction(self.tr("Set to Center"))
        actMaximum = menu.addAction(self.tr("Set to Maximum (%i%%)" % (maximum*100)))
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        if label not in ("Balance-Left", "Balance-Right", "Panning"):
            menu.removeAction(actCenter)

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            current   = minimum + (maximum-minimum)*(float(sender.value())/10000)
            value, ok = QInputDialog.getInt(self, self.tr("Set value"), label, round(current*100.0), round(minimum*100.0), round(maximum*100.0), 1)
            if ok: value = float(value)/100.0

            if not ok:
                return

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

    @pyqtSlot()
    def slot_channelCustomMenu(self):
        menu = QMenu(self)

        actNone = menu.addAction(self.tr("None"))

        if self.fControlChannel+1 == 0:
            actNone.setCheckable(True)
            actNone.setChecked(True)

        for i in range(1, 16+1):
            action = menu.addAction("%i" % i)

            if self.fControlChannel+1 == i:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            pass
        elif actSel == actNone:
            self.ui.sb_ctrl_channel.setValue(0)
        elif actSel:
            selChannel = int(actSel.text())
            self.ui.sb_ctrl_channel.setValue(selChannel)

    #------------------------------------------------------------------

    def _createParameterWidgets(self, paramType, paramListFull, tabPageName):
        i = 1
        for paramList, width in paramListFull:
            if len(paramList) == 0:
                break

            tabIndex         = self.ui.tabWidget.count()
            tabPageContainer = QWidget(self.ui.tabWidget)
            tabPageLayout    = QVBoxLayout(tabPageContainer)
            tabPageContainer.setLayout(tabPageLayout)

            for paramInfo in paramList:
                paramWidget = PluginParameter(tabPageContainer, self.host, paramInfo, self.fPluginId, tabIndex)
                paramWidget.setLabelWidth(width)
                tabPageLayout.addWidget(paramWidget)

                self.fParameterList.append((paramType, paramInfo['index'], paramWidget))

                if paramType == PARAMETER_INPUT:
                    paramWidget.valueChanged.connect(self.slot_parameterValueChanged)

                paramWidget.midiControlChanged.connect(self.slot_parameterMidiControlChanged)
                paramWidget.midiChannelChanged.connect(self.slot_parameterMidiChannelChanged)

            tabPageLayout.addStretch()

            self.ui.tabWidget.addTab(tabPageContainer, "%s (%i)" % (tabPageName, i))
            i += 1

            if paramType == PARAMETER_INPUT:
                self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOff)

            self.fTabIconTimers.append(ICON_STATE_NULL)

    def _updateCtrlPrograms(self):
        self.ui.keyboard.setEnabled(self.fControlChannel >= 0)

        if self.fPluginInfo['category'] != PLUGIN_CATEGORY_SYNTH or self.fPluginInfo['type'] not in (PLUGIN_INTERNAL, PLUGIN_SF2, PLUGIN_GIG):
            return

        if self.fControlChannel < 0:
            self.ui.cb_programs.setEnabled(False)
            self.ui.cb_midi_programs.setEnabled(False)
            return

        self.ui.cb_programs.setEnabled(True)
        self.ui.cb_midi_programs.setEnabled(True)

        pIndex = self.host.get_current_program_index(self.fPluginId)

        if self.ui.cb_programs.currentIndex() != pIndex:
            self.setProgram(pIndex)

        mpIndex = self.host.get_current_midi_program_index(self.fPluginId)

        if self.ui.cb_midi_programs.currentIndex() != mpIndex:
            self.setMidiProgram(mpIndex)

    def _updateParameterValues(self):
        for paramType, paramId, paramWidget in self.fParameterList:
            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

    #------------------------------------------------------------------

    def testTimer(self):
        self.fIdleTimerId = self.startTimer(50)

        self.SIGTERM.connect(self.testTimerClose)
        gCarla.gui = self
        setUpSignals()

    def testTimerClose(self):
        self.close()
        app.quit()

    #------------------------------------------------------------------

    def closeEvent(self, event):
        if self.fIdleTimerId != 0:
            self.killTimer(self.fIdleTimerId)
            self.fIdleTimerId = 0

            self.host.engine_close()

        QDialog.closeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerId:
            self.host.engine_idle()
            self.idleSlow()

        QDialog.timerEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    from carla_app import CarlaApplication
    from carla_host import initHost, loadHostSettings

    app  = CarlaApplication()
    host = initHost("Widgets", None, False, False, False)
    loadHostSettings(host)

    host.engine_init("JACK", "Carla-Widgets")
    host.add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/karplong.so", "karplong", "karplong", 0, None, 0x0)
    host.set_active(0, True)

    gui1 = CarlaAboutW(None, host)
    gui1.show()

    gui2 = PluginEdit(None, host, 0)
    gui2.testTimer()
    gui2.show()

    app.exit_exec()
