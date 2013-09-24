#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla widgets code
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
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, QByteArray, QSettings
    from PyQt5.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt5.QtWidgets import QDialog, QFrame, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget
except:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, QByteArray, QSettings
    from PyQt4.QtGui import QColor, QCursor, QFontMetrics, QPainter, QPainterPath
    from PyQt4.QtGui import QDialog, QFrame, QInputDialog, QLineEdit, QMenu, QVBoxLayout, QWidget

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_about
import ui_carla_edit
import ui_carla_parameter
import ui_carla_plugin

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Carla GUI defines

ICON_STATE_NULL  = 0
ICON_STATE_OFF   = 1
ICON_STATE_WAIT  = 2
ICON_STATE_ON    = 3

# ------------------------------------------------------------------------------------------------------------
# Fake plugin info for easy testing

gFakePluginInfo = {
    "type": PLUGIN_NONE,
    "category": PLUGIN_CATEGORY_SYNTH,
    "hints": PLUGIN_CAN_DRYWET|PLUGIN_CAN_VOLUME|PLUGIN_CAN_PANNING, # PLUGIN_IS_SYNTH
    "optionsAvailable": 0x1FF, # all
    "optionsEnabled": 0x1FF, # all
    "binary": "AwesoomeBinary.yeah",
    "name": "Awesoome Name",
    "label": "awesoomeLabel",
    "maker": "Awesoome Maker",
    "copyright": "Awesoome Copyright",
    "iconName": "plugin",
    "uniqueId": 0,
    "latency": 0
}

gFakeParamInfo = {
    'type':  PARAMETER_INPUT,
    'hints': PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE,
    'name':  "Parameter Name",
    'unit':  "",
    'scalePoints': [],

    'index':   0,
    'default': 0.0,
    'minimum': 0.0,
    'maximum': 1.0,
    'step':    0.01,
    'stepSmall': 0.01,
    'stepLarge': 0.01,
    'midiCC':   -1,
    'midiChannel': 1,

    'current': 0.0
}

gFakeCountInfo = {
    "ins": 0,
    "outs": 0,
    "total": 0
}

# ------------------------------------------------------------------------------------------------------------
# Carla About dialog

class CarlaAboutW(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_about.Ui_CarlaAboutW()
        self.ui.setupUi(self)

        if Carla.isControl:
            extraInfo = " - <b>%s</b>" % self.tr("OSC Bridge Version")
        else:
            extraInfo = ""

        self.ui.l_about.setText(self.tr(""
                                     "<br>Version %s"
                                     "<br>Carla is a Multi-Plugin Host for JACK%s.<br>"
                                     "<br>Copyright (C) 2011-2013 falkTX<br>"
                                     "" % (VERSION, extraInfo)))

        if Carla.host is None:
            self.ui.l_extended.hide()
            self.ui.tabWidget.removeTab(1)
            self.ui.tabWidget.removeTab(1)
            self.adjustSize()

        else:
            self.ui.l_extended.setText(cString(Carla.host.get_extended_license_text()))

            if Carla.host.is_engine_running():
                self.ui.le_osc_url_tcp.setText(cString(Carla.host.get_host_osc_url_tcp()))
                self.ui.le_osc_url_udp.setText(cString(Carla.host.get_host_osc_url_udp()))
            else:
                self.ui.le_osc_url_tcp.setText(self.tr("(Engine not running)"))
                self.ui.le_osc_url_udp.setText(self.tr("(Engine not running)"))

            self.ui.l_osc_cmds.setText(""
                                       " /set_active                 <i-value>\n"
                                       " /set_drywet                 <f-value>\n"
                                       " /set_volume                 <f-value>\n"
                                       " /set_balance_left           <f-value>\n"
                                       " /set_balance_right          <f-value>\n"
                                       " /set_panning                <f-value>\n"
                                       " /set_parameter_value        <i-index> <f-value>\n"
                                       " /set_parameter_midi_cc      <i-index> <i-cc>\n"
                                       " /set_parameter_midi_channel <i-index> <i-channel>\n"
                                       " /set_program                <i-index>\n"
                                       " /set_midi_program           <i-index>\n"
                                       " /note_on                    <i-note> <i-velo>\n"
                                       " /note_off                   <i-note>\n"
                                      )

            self.ui.l_example.setText("/Carla/2/set_parameter_value 5 1.0")
            self.ui.l_example_help.setText("<i>(as in this example, \"2\" is the plugin number and \"5\" the parameter)</i>")

            self.ui.l_ladspa.setText(self.tr("Everything! (Including LRDF)"))
            self.ui.l_dssi.setText(self.tr("Everything! (Including CustomData/Chunks)"))
            self.ui.l_lv2.setText(self.tr("About 80&#37; complete (using custom extensions)<br/>"
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
                                         #"<li>http://lv2plug.in/ns/ext/port-props</li>"
                                          "<li>http://lv2plug.in/ns/ext/presets</li>"
                                         #"<li>http://lv2plug.in/ns/ext/resize-port</li>"
                                          "<li>http://lv2plug.in/ns/ext/state</li>"
                                          "<li>http://lv2plug.in/ns/ext/time</li>"
                                          "<li>http://lv2plug.in/ns/ext/uri-map</li>"
                                          "<li>http://lv2plug.in/ns/ext/urid</li>"
                                         #"<li>http://lv2plug.in/ns/ext/worker</li>"
                                          "<li>http://lv2plug.in/ns/extensions/ui</li>"
                                          "<li>http://lv2plug.in/ns/extensions/units</li>"
                                          "<li>http://kxstudio.sf.net/ns/lv2ext/external-ui</li>"
                                          "<li>http://kxstudio.sf.net/ns/lv2ext/programs</li>"
                                          "<li>http://kxstudio.sf.net/ns/lv2ext/rtmempool</li>"
                                          "<li>http://ll-plugins.nongnu.org/lv2/ext/midimap</li>"
                                          "<li>http://ll-plugins.nongnu.org/lv2/ext/miditype</li>"
                                          "</ul>"))
            self.ui.l_vst.setText(self.tr("<p>About 85&#37; complete (missing vst bank/presets and some minor stuff)</p>"))

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Parameter

class PluginParameter(QWidget):
    midiControlChanged = pyqtSignal(int, int)
    midiChannelChanged = pyqtSignal(int, int)
    valueChanged       = pyqtSignal(int, float)

    def __init__(self, parent, pInfo, pluginId, tabIndex):
        QWidget.__init__(self, parent)
        self.ui = ui_carla_parameter.Ui_PluginParameter()
        self.ui.setupUi(self)

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

        if pType == PARAMETER_INPUT:
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setDefault(pInfo['default'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setStep(pInfo['step'])
            self.ui.widget.setStepSmall(pInfo['stepSmall'])
            self.ui.widget.setStepLarge(pInfo['stepLarge'])
            self.ui.widget.setScalePoints(pInfo['scalePoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

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
            self.ui.widget.setMinimum(pInfo['minimum'])
            self.ui.widget.setMaximum(pInfo['maximum'])
            self.ui.widget.setValue(pInfo['current'], False)
            self.ui.widget.setLabel(pInfo['unit'])
            self.ui.widget.setReadOnly(True)

            if not pHints & PARAMETER_IS_AUTOMABLE:
                self.ui.sb_control.setEnabled(False)
                self.ui.sb_channel.setEnabled(False)

        else:
            self.ui.widget.setVisible(False)
            self.ui.sb_control.setVisible(False)
            self.ui.sb_channel.setVisible(False)

        if pHints & PARAMETER_USES_CUSTOM_TEXT:
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

    def pluginId(self):
        return self.fPluginId

    def tabIndex(self):
        return self.fTabIndex

    def setDefault(self, value):
        self.ui.widget.setDefault(value)

    def setValue(self, value, send=True):
        self.ui.widget.setValue(value, send)

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
        self.ui.label.setMinimumWidth(width)
        self.ui.label.setMaximumWidth(width)

    @pyqtSlot()
    def slot_controlSpinboxCustomMenu(self):
        menu = QMenu(self)

        actNone = menu.addAction(self.tr("None"))

        if self.fMidiControl == -1:
            actNone.setCheckable(True)
            actNone.setChecked(True)

        for cc in MIDI_CC_LIST:
            action = menu.addAction(cc)

            if self.fMidiControl != -1 and int(cc.split(" ")[0], 16) == self.fMidiControl:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            pass
        elif actSel == actNone:
            self.ui.sb_control.setValue(-1)
        else:
            selControlStr = actSel.text()
            selControl    = int(selControlStr.split(" ")[0], 16)
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
        if self.fMidiControl != control:
            self.midiControlChanged.emit(self.fParameterId, control)
            self.fMidiControl = control

    @pyqtSlot(int)
    def slot_channelSpinboxChanged(self, channel):
        if self.fMidiChannel != channel:
            self.midiChannelChanged.emit(self.fParameterId, channel)
            self.fMidiChannel = channel

    @pyqtSlot(float)
    def slot_widgetValueChanged(self, value):
        self.valueChanged.emit(self.fParameterId, value)

    def _textCallBack(self):
        return cString(Carla.host.get_parameter_text(self.fPluginId, self.fParameterId))

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor (Built-in)

class PluginEdit(QDialog):
    ParamsPerPage = 8

    def __init__(self, parent, pluginId):
        QDialog.__init__(self, Carla.gui)
        self.ui = ui_carla_edit.Ui_PluginEdit()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fGeometry   = QByteArray()
        self.fPluginId   = pluginId
        self.fPuginInfo  = None
        self.fRealParent = parent

        self.fCurrentProgram = -1
        self.fCurrentMidiProgram = -1
        self.fCurrentStateFilename = None
        self.fControlChannel = 0
        self.fScrollAreaSetup = False

        self.fParameterCount = 0
        self.fParameterList  = []     # (type, id, widget)
        self.fParametersToUpdate = [] # (id, value)

        self.fPlayingNotes = [] # (channel, note)

        self.fTabIconOff = QIcon(":/bitmaps/led_off.png")
        self.fTabIconOn  = QIcon(":/bitmaps/led_yellow.png")
        self.fTabIconCount  = 0
        self.fTabIconTimers = []

        # -------------------------------------------------------------
        # Set-up GUI

        self.ui.dial_drywet.setCustomPaint(self.ui.dial_drywet.CUSTOM_PAINT_CARLA_WET)
        self.ui.dial_drywet.setPixmap(3)
        self.ui.dial_drywet.setLabel("Dry/Wet")

        self.ui.dial_vol.setCustomPaint(self.ui.dial_vol.CUSTOM_PAINT_CARLA_VOL)
        self.ui.dial_vol.setPixmap(3)
        self.ui.dial_vol.setLabel("Volume")

        self.ui.dial_b_left.setCustomPaint(self.ui.dial_b_left.CUSTOM_PAINT_CARLA_L)
        self.ui.dial_b_left.setPixmap(4)
        self.ui.dial_b_left.setLabel("L")

        self.ui.dial_b_right.setCustomPaint(self.ui.dial_b_right.CUSTOM_PAINT_CARLA_R)
        self.ui.dial_b_right.setPixmap(4)
        self.ui.dial_b_right.setLabel("R")

        self.ui.dial_pan.setCustomPaint(self.ui.dial_b_right.CUSTOM_PAINT_CARLA_R) # FIXME
        self.ui.dial_pan.setPixmap(4)
        self.ui.dial_pan.setLabel("Pan")

        self.ui.keyboard.setMode(self.ui.keyboard.HORIZONTAL)
        self.ui.keyboard.setOctaves(10)

        self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)

        self.ui.scrollArea.ensureVisible(self.ui.keyboard.width() / 3, 0)
        self.ui.scrollArea.setEnabled(False)
        self.ui.scrollArea.setVisible(False)

        self.reloadAll()

        # -------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_finished)

        self.ui.ch_fixed_buffer.clicked.connect(self.slot_optionChanged)
        self.ui.ch_force_stereo.clicked.connect(self.slot_optionChanged)
        self.ui.ch_map_program_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_use_chunks.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_control_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_channel_pressure.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_note_aftertouch.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_pitchbend.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_all_sound_off.clicked.connect(self.slot_optionChanged)

        self.ui.dial_drywet.valueChanged.connect(self.slot_dryWetChanged)
        self.ui.dial_vol.valueChanged.connect(self.slot_volumeChanged)
        self.ui.dial_b_left.valueChanged.connect(self.slot_balanceLeftChanged)
        self.ui.dial_b_right.valueChanged.connect(self.slot_balanceRightChanged)
        self.ui.sb_ctrl_channel.valueChanged.connect(self.slot_ctrlChannelChanged)

        self.ui.dial_drywet.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_vol.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_left.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_right.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.sb_ctrl_channel.customContextMenuRequested.connect(self.slot_channelCustomMenu)

        self.ui.keyboard.noteOn.connect(self.slot_noteOn)
        self.ui.keyboard.noteOff.connect(self.slot_noteOff)

        self.ui.cb_programs.currentIndexChanged.connect(self.slot_programIndexChanged)
        self.ui.cb_midi_programs.currentIndexChanged.connect(self.slot_midiProgramIndexChanged)

        if Carla.isLocal:
            self.ui.b_save_state.clicked.connect(self.slot_stateSave)
            self.ui.b_load_state.clicked.connect(self.slot_stateLoad)
        else:
            self.ui.b_load_state.setEnabled(False)
            self.ui.b_save_state.setEnabled(False)

        # -------------------------------------------------------------

    def reloadAll(self):
        if Carla.host is not None:
            self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId)
            self.fPluginInfo['binary']    = cString(self.fPluginInfo['binary'])
            self.fPluginInfo['name']      = cString(self.fPluginInfo['name'])
            self.fPluginInfo['label']     = cString(self.fPluginInfo['label'])
            self.fPluginInfo['maker']     = cString(self.fPluginInfo['maker'])
            self.fPluginInfo['copyright'] = cString(self.fPluginInfo['copyright'])
            self.fPluginInfo['iconName']  = cString(self.fPluginInfo['iconName'])

            if not Carla.isLocal:
                self.fPluginInfo['hints'] &= ~PLUGIN_HAS_GUI

        else:
            self.fPluginInfo = gFakePluginInfo

        self.reloadInfo()
        self.reloadParameters()
        self.reloadPrograms()

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_save_state.setEnabled(False)

        if not self.ui.scrollArea.isEnabled():
            self.resize(self.width(), self.height()-self.ui.scrollArea.height())

    def reloadInfo(self):
        if Carla.host is not None:
            pluginName     = cString(Carla.host.get_real_plugin_name(self.fPluginId))
            audioCountInfo = Carla.host.get_audio_port_count_info(self.fPluginId)
            midiCountInfo  = Carla.host.get_midi_port_count_info(self.fPluginId)
            paramCountInfo = Carla.host.get_parameter_count_info(self.fPluginId)
        else:
            pluginName     = ""
            audioCountInfo = gFakeCountInfo
            midiCountInfo  = gFakeCountInfo
            paramCountInfo = gFakeCountInfo

        pluginType  = self.fPluginInfo['type']
        pluginHints = self.fPluginInfo['hints']

        if pluginType == PLUGIN_INTERNAL:
            self.ui.le_type.setText(self.tr("Internal"))
        elif pluginType == PLUGIN_LADSPA:
            self.ui.le_type.setText("LADSPA")
        elif pluginType == PLUGIN_DSSI:
            self.ui.le_type.setText("DSSI")
        elif pluginType == PLUGIN_LV2:
            self.ui.le_type.setText("LV2")
        elif pluginType == PLUGIN_VST:
            self.ui.le_type.setText("VST")
        elif pluginType == PLUGIN_AU:
            self.ui.le_type.setText("AU")
        elif pluginType == PLUGIN_CSOUND:
            self.ui.le_type.setText("CSOUND")
        elif pluginType == PLUGIN_GIG:
            self.ui.le_type.setText("GIG")
        elif pluginType == PLUGIN_SF2:
            self.ui.le_type.setText("SF2")
        elif pluginType == PLUGIN_SFZ:
            self.ui.le_type.setText("SFZ")
        else:
            self.ui.le_type.setText(self.tr("Unknown"))

        self.ui.le_name.setText(pluginName)
        self.ui.le_name.setToolTip(pluginName)
        self.ui.le_label.setText(self.fPluginInfo['label'])
        self.ui.le_label.setToolTip(self.fPluginInfo['label'])
        self.ui.le_maker.setText(self.fPluginInfo['maker'])
        self.ui.le_maker.setToolTip(self.fPluginInfo['maker'])
        self.ui.le_copyright.setText(self.fPluginInfo['copyright'])
        self.ui.le_copyright.setToolTip(self.fPluginInfo['copyright'])
        self.ui.le_unique_id.setText(str(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setToolTip(str(self.fPluginInfo['uniqueId']))
        self.ui.label_plugin.setText("\n%s\n" % self.fPluginInfo['name'])
        self.setWindowTitle(self.fPluginInfo['name'])

        self.ui.dial_drywet.setEnabled(pluginHints & PLUGIN_CAN_DRYWET)
        self.ui.dial_vol.setEnabled(pluginHints & PLUGIN_CAN_VOLUME)
        self.ui.dial_b_left.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_b_right.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_pan.setEnabled(pluginHints & PLUGIN_CAN_PANNING)

        self.ui.ch_fixed_buffer.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_fixed_buffer.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_force_stereo.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_force_stereo.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_map_program_changes.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_map_program_changes.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_use_chunks.setEnabled(self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_USE_CHUNKS)
        self.ui.ch_use_chunks.setChecked(self.fPluginInfo['optionsEnabled'] & PLUGIN_OPTION_USE_CHUNKS)
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

        if self.fPluginInfo['type'] != PLUGIN_VST:
            self.ui.sw_programs.setCurrentIndex(1)

        # Show/hide keyboard
        showKeyboard = (self.fPluginInfo['category'] == PLUGIN_CATEGORY_SYNTH or midiCountInfo['ins'] > 0 < midiCountInfo['outs'])
        self.ui.scrollArea.setEnabled(showKeyboard)
        self.ui.scrollArea.setVisible(showKeyboard)

        # Force-Update parent for new hints
        if self.fRealParent:
            self.fRealParent.recheckPluginHints(pluginHints)

    def reloadParameters(self):
        # Reset
        self.fParameterCount = 0
        self.fParameterList  = []
        self.fParametersToUpdate = []

        self.fTabIconCount  = 0
        self.fTabIconTimers = []

        # Remove all previous parameters
        for x in range(self.ui.tabWidget.count()-1):
            self.ui.tabWidget.widget(1).deleteLater()
            self.ui.tabWidget.removeTab(1)

        if Carla.host is None:
            paramFakeListFull = []
            paramFakeList  = []
            paramFakeWidth = QFontMetrics(self.font()).width(gFakeParamInfo['name'])

            paramFakeList.append(gFakeParamInfo)
            paramFakeListFull.append((paramFakeList, paramFakeWidth))

            self._createParameterWidgets(PARAMETER_INPUT, paramFakeListFull,  self.tr("Parameters"))
            return

        parameterCount = Carla.host.get_parameter_count(self.fPluginId)

        if parameterCount <= 0:
            pass

        elif parameterCount <= Carla.maxParameters:
            paramInputListFull  = []
            paramOutputListFull = []

            paramInputList   = [] # ([params], width)
            paramInputWidth  = 0
            paramOutputList  = [] # ([params], width)
            paramOutputWidth = 0

            for i in range(parameterCount):
                paramInfo   = Carla.host.get_parameter_info(self.fPluginId, i)
                paramData   = Carla.host.get_parameter_data(self.fPluginId, i)
                paramRanges = Carla.host.get_parameter_ranges(self.fPluginId, i)
                paramValue  = Carla.host.get_current_parameter_value(self.fPluginId, i)

                if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                    continue

                parameter = {
                    'type':  paramData['type'],
                    'hints': paramData['hints'],
                    'name':  cString(paramInfo['name']),
                    'unit':  cString(paramInfo['unit']),
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
                    scalePointInfo  = Carla.host.get_parameter_scalepoint_info(self.fPluginId, i, j)

                    parameter['scalePoints'].append({
                        'value': scalePointInfo['value'],
                        'label': cString(scalePointInfo['label'])
                    })

                #parameter['name'] = parameter['name'][:30] + (parameter['name'][30:] and "...")

                # -----------------------------------------------------------------
                # Get width values, in packs of 10

                if parameter['type'] == PARAMETER_INPUT:
                    paramInputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramInputWidthTMP > paramInputWidth:
                        paramInputWidth = paramInputWidthTMP

                    paramInputList.append(parameter)

                    if len(paramInputList) == self.ParamsPerPage:
                        paramInputListFull.append((paramInputList, paramInputWidth))
                        paramInputList  = []
                        paramInputWidth = 0

                else:
                    paramOutputWidthTMP = QFontMetrics(self.font()).width(parameter['name'])

                    if paramOutputWidthTMP > paramOutputWidth:
                        paramOutputWidth = paramOutputWidthTMP

                    paramOutputList.append(parameter)

                    if len(paramOutputList) == self.ParamsPerPage:
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

            # -----------------------------------------------------------------
            # Create parameter tabs + widgets

            self._createParameterWidgets(PARAMETER_INPUT,  paramInputListFull,  self.tr("Parameters"))
            self._createParameterWidgets(PARAMETER_OUTPUT, paramOutputListFull, self.tr("Outputs"))

        else: # > Carla.maxParameters
            fakeName = self.tr("This plugin has too many parameters to display here!")

            paramFakeListFull = []
            paramFakeList  = []
            paramFakeWidth = QFontMetrics(self.font()).width(fakeName)

            parameter = {
                'type':  PARAMETER_UNKNOWN,
                'hints': 0,
                'name':  fakeName,
                'unit':  "",
                'scalePoints': [],

                'index':   0,
                'default': 0.0,
                'minimum': 0.0,
                'maximum': 0.0,
                'step':    0.0,
                'stepSmall': 0.0,
                'stepLarge': 0.0,
                'midiCC':   -1,
                'midiChannel': 1,

                'current': 0.0
            }

            paramFakeList.append(parameter)
            paramFakeListFull.append((paramFakeList, paramFakeWidth))

            self._createParameterWidgets(PARAMETER_UNKNOWN, paramFakeListFull, self.tr("Information"))

    def reloadPrograms(self):
        # Programs
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.clear()

        programCount = Carla.host.get_program_count(self.fPluginId) if Carla.host is not None else 0

        if programCount > 0:
            self.ui.cb_programs.setEnabled(True)
            self.ui.label_programs.setEnabled(True)

            for i in range(programCount):
                pName = cString(Carla.host.get_program_name(self.fPluginId, i))
                #pName = pName[:40] + (pName[40:] and "...")
                self.ui.cb_programs.addItem(pName)

            self.fCurrentProgram = Carla.host.get_current_program_index(self.fPluginId)
            self.ui.cb_programs.setCurrentIndex(self.fCurrentProgram)

        else:
            self.fCurrentProgram = -1
            self.ui.cb_programs.setEnabled(False)
            self.ui.label_programs.setEnabled(False)

        self.ui.cb_programs.blockSignals(False)

        # MIDI Programs
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.clear()

        midiProgramCount = Carla.host.get_midi_program_count(self.fPluginId) if Carla.host is not None else 0

        if midiProgramCount > 0:
            self.ui.cb_midi_programs.setEnabled(True)
            self.ui.label_midi_programs.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = Carla.host.get_midi_program_data(self.fPluginId, i)
                mpBank = int(mpData['bank'])
                mpProg = int(mpData['program'])
                mpName = cString(mpData['name'])
                #mpName = mpName[:40] + (mpName[40:] and "...")

                self.ui.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

            self.fCurrentMidiProgram = Carla.host.get_current_midi_program_index(self.fPluginId)
            self.ui.cb_midi_programs.setCurrentIndex(self.fCurrentMidiProgram)

        else:
            self.fCurrentMidiProgram = -1
            self.ui.cb_midi_programs.setEnabled(False)
            self.ui.label_midi_programs.setEnabled(False)

        self.ui.cb_midi_programs.blockSignals(False)

        self.ui.sw_programs.setEnabled(programCount > 0 or midiProgramCount > 0)

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_load_state.setEnabled(programCount > 0)

    def updateInfo(self):
        # Update current program text
        if self.ui.cb_programs.count() > 0:
            pIndex = self.ui.cb_programs.currentIndex()
            pName  = cString(Carla.host.get_program_name(self.fPluginId, pIndex))
            #pName  = pName[:40] + (pName[40:] and "...")
            self.ui.cb_programs.setItemText(pIndex, pName)

        # Update current midi program text
        if self.ui.cb_midi_programs.count() > 0:
            mpIndex = self.ui.cb_midi_programs.currentIndex()
            mpData  = Carla.host.get_midi_program_data(self.fPluginId, mpIndex)
            mpBank  = int(mpData['bank'])
            mpProg  = int(mpData['program'])
            mpName  = cString(mpData['name'])
            #mpName  = mpName[:40] + (mpName[40:] and "...")
            self.ui.cb_midi_programs.setItemText(mpIndex, "%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

        # Update all parameter values
        for paramType, paramId, paramWidget in self.fParameterList:
            paramWidget.setValue(Carla.host.get_current_parameter_value(self.fPluginId, paramId), False)
            paramWidget.update()

        self.fParametersToUpdate = []

    def clearNotes(self):
         self.fPlayingNotes = []
         self.ui.keyboard.allNotesOff()

    def setName(self, name):
        self.ui.label_plugin.setText("\n%s\n" % name)
        self.setWindowTitle(name)

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

    def setMidiProgram(self, index):
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.setCurrentIndex(index)
        self.ui.cb_midi_programs.blockSignals(False)

    def sendNoteOn(self, channel, note):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOn(note, False)

        if len(self.fPlayingNotes) == 0 and self.fRealParent:
            self.fRealParent.ui.led_midi.setChecked(True)

        playItem = (channel, note)

        if playItem not in self.fPlayingNotes:
            self.fPlayingNotes.append(playItem)

    def sendNoteOff(self, channel, note):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOff(note, False)

        if len(self.fPlayingNotes) == 1 and self.fRealParent:
            self.fRealParent.ui.led_midi.setChecked(False)

        playItem = (channel, note)

        if playItem in self.fPlayingNotes:
            self.fPlayingNotes.remove(playItem)

    def setVisible(self, yesNo):
        if yesNo:
            if not self.fGeometry.isNull():
                self.restoreGeometry(self.fGeometry)
        else:
            self.fGeometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

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
                self.ui.dial_drywet.setValue(value * 1000)
                self.ui.dial_drywet.blockSignals(False)

            elif index == PARAMETER_VOLUME:
                self.ui.dial_vol.blockSignals(True)
                self.ui.dial_vol.setValue(value * 1000)
                self.ui.dial_vol.blockSignals(False)

            elif index == PARAMETER_BALANCE_LEFT:
                self.ui.dial_b_left.blockSignals(True)
                self.ui.dial_b_left.setValue(value * 1000)
                self.ui.dial_b_left.blockSignals(False)

            elif index == PARAMETER_BALANCE_RIGHT:
                self.ui.dial_b_right.blockSignals(True)
                self.ui.dial_b_right.setValue(value * 1000)
                self.ui.dial_b_right.blockSignals(False)

            #elif index == PARAMETER_PANNING:
                #self.ui.dial_pan.blockSignals(True)
                #self.ui.dial_pan.setValue(value * 1000, True, False)
                #self.ui.dial_pan.blockSignals(False)

            elif index == PARAMETER_CTRL_CHANNEL:
                self.fControlChannel = int(value)
                self.ui.sb_ctrl_channel.blockSignals(True)
                self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)
                self.ui.sb_ctrl_channel.blockSignals(False)
                self.ui.keyboard.allNotesOff()
                self._updateCtrlMidiProgram()

            elif index >= 0:
                for paramType, paramId, paramWidget in self.fParameterList:
                    if paramId != index:
                        continue

                    paramWidget.setValue(value, False)

                    if paramType == PARAMETER_INPUT:
                        tabIndex = paramWidget.tabIndex()

                        if self.fTabIconTimers[tabIndex-1] == ICON_STATE_NULL:
                            self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOn)

                        self.fTabIconTimers[tabIndex-1] = ICON_STATE_ON

                    break

        # Clear all parameters
        self.fParametersToUpdate = []

        # Update parameter outputs
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramType == PARAMETER_OUTPUT:
                value = Carla.host.get_current_parameter_value(self.fPluginId, paramId)
                paramWidget.setValue(value, False)

    @pyqtSlot()
    def slot_stateSave(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            # TODO
            return

        if self.fCurrentStateFilename:
            askTry = QMessageBox.question(self, self.tr("Overwrite?"), self.tr("Overwrite previously created file?"), QMessageBox.Ok|QMessageBox.Cancel)

            if askTry == QMessageBox.Ok:
                Carla.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)
                return

            self.fCurrentStateFilename = None

        fileFilter  = self.tr("Carla State File (*.carxs)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Plugin State File"), filter=fileFilter)

        if filenameTry:
            if not filenameTry.lower().endswith(".carxs"):
                filenameTry += ".carxs"

            self.fCurrentStateFilename = filenameTry
            Carla.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    @pyqtSlot()
    def slot_stateLoad(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            presetList = []

            for i in range(Carla.host.get_program_count(self.fPluginId)):
                presetList.append("%03i - %s" % (i+1, cString(Carla.host.get_program_name(self.fPluginId, i))))

            ret = QInputDialog.getItem(self, self.tr("Open LV2 Preset"), self.tr("Select an LV2 Preset:"), presetList, 0, False)

            if ret[1]:
                index = int(ret[0].split(" - ", 1)[0])-1
                Carla.host.set_midi_program(self.fPluginId, -1)
                Carla.host.set_program(self.fPluginId, index)
                self.setMidiProgram(-1)

            return

        fileFilter  = self.tr("Carla State File (*.carxs)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Plugin State File"), filter=fileFilter)

        if filenameTry:
            self.fCurrentStateFilename = filenameTry
            Carla.host.load_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    @pyqtSlot(bool)
    def slot_optionChanged(self, clicked):
        sender = self.sender()

        if sender == self.ui.ch_fixed_buffer:
            option = PLUGIN_OPTION_FIXED_BUFFER
        elif sender == self.ui.ch_force_stereo:
            option = PLUGIN_OPTION_FORCE_STEREO
        elif sender == self.ui.ch_map_program_changes:
            option = PLUGIN_OPTION_MAP_PROGRAM_CHANGES
        elif sender == self.ui.ch_use_chunks:
            option = PLUGIN_OPTION_USE_CHUNKS
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

        Carla.host.set_option(self.fPluginId, option, clicked)

    @pyqtSlot(int)
    def slot_dryWetChanged(self, value):
        Carla.host.set_drywet(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_volumeChanged(self, value):
        Carla.host.set_volume(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_balanceLeftChanged(self, value):
        Carla.host.set_balance_left(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_balanceRightChanged(self, value):
        Carla.host.set_balance_right(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_panningChanged(self, value):
        Carla.host.set_panning(self.fPluginId, float(value)/1000)

    @pyqtSlot(int)
    def slot_ctrlChannelChanged(self, value):
        self.fControlChannel = value-1
        Carla.host.set_ctrl_channel(self.fPluginId, self.fControlChannel)
        self.ui.keyboard.allNotesOff()
        self._updateCtrlMidiProgram()

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameterId, value):
        Carla.host.set_parameter_value(self.fPluginId, parameterId, value)

    @pyqtSlot(int, int)
    def slot_parameterMidiControlChanged(self, parameterId, control):
        Carla.host.set_parameter_midi_cc(self.fPluginId, parameterId, control)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameterId, channel):
        Carla.host.set_parameter_midi_channel(self.fPluginId, parameterId, channel-1)

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        self.fCurrentProgram = index
        Carla.host.set_program(self.fPluginId, index)

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        self.fCurrentMidiProgram = index
        Carla.host.set_midi_program(self.fPluginId, index)

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        if self.fControlChannel >= 0:
            Carla.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        if self.fControlChannel >= 0:
            Carla.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 0)

    @pyqtSlot()
    def slot_finished(self):
        if self.fRealParent:
            self.fRealParent.editClosed()

    @pyqtSlot()
    def slot_knobCustomMenu(self):
        dialName = self.sender().objectName()
        if dialName == "dial_drywet":
            minimum = 0
            maximum = 100
            default = 100
            label   = "Dry/Wet"
        elif dialName == "dial_vol":
            minimum = 0
            maximum = 127
            default = 100
            label   = "Volume"
        elif dialName == "dial_b_left":
            minimum = -100
            maximum = 100
            default = -100
            label   = "Balance-Left"
        elif dialName == "dial_b_right":
            minimum = -100
            maximum = 100
            default = 100
            label   = "Balance-Right"
        elif dialName == "dial_panning":
            minimum = -100
            maximum = 100
            default = 0
            label   = "Panning"
        else:
            minimum = 0
            maximum = 100
            default = 100
            label   = "Unknown"

        current = self.sender().value() / 10

        menu = QMenu(self)
        actReset = menu.addAction(self.tr("Reset (%i%%)" % default))
        menu.addSeparator()
        actMinimum = menu.addAction(self.tr("Set to Minimum (%i%%)" % minimum))
        actCenter  = menu.addAction(self.tr("Set to Center"))
        actMaximum = menu.addAction(self.tr("Set to Maximum (%i%%)" % maximum))
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        if label not in ("Balance-Left", "Balance-Right"):
            menu.removeAction(actCenter)

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            valueTry = QInputDialog.getInteger(self, self.tr("Set value"), label, current, minimum, maximum, 1)
            if valueTry[1]:
                value = valueTry[0] * 10
            else:
                return

        elif actSelected == actMinimum:
            value = minimum * 10
        elif actSelected == actMaximum:
            value = maximum * 10
        elif actSelected == actReset:
            value = default * 10
        elif actSelected == actCenter:
            value = 0
        else:
            return

        if label == "Dry/Wet":
            self.ui.dial_drywet.setValue(value)
        elif label == "Volume":
            self.ui.dial_vol.setValue(value)
        elif label == "Balance-Left":
            self.ui.dial_b_left.setValue(value)
        elif label == "Balance-Right":
            self.ui.dial_b_right.setValue(value)
        #elif label == "Panning":
            #self.ui.dial_panning.setValue(value)

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
                paramWidget = PluginParameter(tabPageContainer, paramInfo, self.fPluginId, tabIndex)
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

    def _updateCtrlMidiProgram(self):
        if self.fPluginInfo['type'] not in (PLUGIN_INTERNAL, PLUGIN_SF2):
            return
        elif self.fPluginInfo['category'] != PLUGIN_CATEGORY_SYNTH:
            return

        if self.fControlChannel < 0:
            self.ui.cb_midi_programs.setEnabled(False)
            return

        self.ui.cb_midi_programs.setEnabled(True)

        mpIndex = Carla.host.get_current_midi_program_index(self.fPluginId)

        if self.ui.cb_midi_programs.currentIndex() != mpIndex:
            self.setMidiProgram(mpIndex)

    def showEvent(self, event):
        if not self.fScrollAreaSetup:
            self.fScrollAreaSetup = True
            minHeight = self.ui.scrollArea.height()+2
            self.ui.scrollArea.setMinimumHeight(minHeight)
            self.ui.scrollArea.setMaximumHeight(minHeight)

        QDialog.showEvent(self, event)

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# Plugin Widget

class PluginWidget(QFrame):
    def __init__(self, parent, pluginId):
        QFrame.__init__(self, parent)
        self.ui = ui_carla_plugin.Ui_PluginWidget()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fPluginId   = pluginId
        self.fPluginInfo = Carla.host.get_plugin_info(self.fPluginId) if Carla.host is not None else gFakePluginInfo

        self.fPluginInfo['binary']    = cString(self.fPluginInfo['binary'])
        self.fPluginInfo['name']      = cString(self.fPluginInfo['name'])
        self.fPluginInfo['label']     = cString(self.fPluginInfo['label'])
        self.fPluginInfo['maker']     = cString(self.fPluginInfo['maker'])
        self.fPluginInfo['copyright'] = cString(self.fPluginInfo['copyright'])
        self.fPluginInfo['iconName']  = cString(self.fPluginInfo['iconName'])

        if not Carla.isLocal:
            self.fPluginInfo['hints'] &= ~PLUGIN_HAS_GUI

        self.fLastGreenLedState = False
        self.fLastBlueLedState  = False

        self.fParameterIconTimer = ICON_STATE_NULL

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK or Carla.host is None:
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

        self.ui.edit_dialog = PluginEdit(self, self.fPluginId)
        self.ui.edit_dialog.hide()

        self.setFixedHeight(32)

        # -------------------------------------------------------------
        # Set-up connections

        self.customContextMenuRequested.connect(self.slot_showCustomMenu)
        self.ui.b_enable.clicked.connect(self.slot_enableClicked)
        self.ui.b_gui.clicked.connect(self.slot_guiClicked)
        self.ui.b_edit.clicked.connect(self.slot_editClicked)

        # -------------------------------------------------------------

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

    def idleSlow(self):
        # Parameter Activity LED
        if self.fParameterIconTimer == ICON_STATE_ON:
            self.fParameterIconTimer = ICON_STATE_WAIT
            self.ui.led_control.setChecked(True)
        elif self.fParameterIconTimer == ICON_STATE_WAIT:
            self.fParameterIconTimer = ICON_STATE_OFF
        elif self.fParameterIconTimer == ICON_STATE_OFF:
            self.fParameterIconTimer = ICON_STATE_NULL
            self.ui.led_control.setChecked(False)

        # Update edit dialog
        self.ui.edit_dialog.idleSlow()

    def editClosed(self):
        self.ui.b_edit.setChecked(False)

    def recheckPluginHints(self, hints):
        self.fPluginInfo['hints'] = hints
        self.ui.b_gui.setEnabled(hints & PLUGIN_HAS_GUI)

    def setActive(self, active, sendGui=False, sendCallback=True):
        if sendGui:      self.ui.b_enable.setChecked(active)
        if sendCallback: Carla.host.set_active(self.fPluginId, active)

        if active:
            self.ui.edit_dialog.clearNotes()
            self.ui.led_midi.setChecked(False)

    def setParameterValue(self, parameterId, value):
        self.fParameterIconTimer = ICON_STATE_ON

        if parameterId == PARAMETER_ACTIVE:
            return self.setActive(bool(value), True, False)

        self.ui.edit_dialog.setParameterValue(parameterId, value)

    def setParameterDefault(self, parameterId, value):
        self.ui.edit_dialog.setParameterDefault(parameterId, value)

    def setParameterMidiControl(self, parameterId, control):
        self.ui.edit_dialog.setParameterMidiControl(parameterId, control)

    def setParameterMidiChannel(self, parameterId, channel):
        self.ui.edit_dialog.setParameterMidiChannel(parameterId, channel)

    def setProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.ui.edit_dialog.setProgram(index)

    def setMidiProgram(self, index):
        self.fParameterIconTimer = ICON_STATE_ON
        self.ui.edit_dialog.setMidiProgram(index)

    def sendNoteOn(self, channel, note):
        self.ui.edit_dialog.sendNoteOn(channel, note)

    def sendNoteOff(self, channel, note):
        self.ui.edit_dialog.sendNoteOff(channel, note)

    def setId(self, idx):
        self.fPluginId = idx
        self.ui.edit_dialog.fPluginId = idx

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
                                       cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRename:
            oldName    = self.fPluginInfo['name']
            newNameTry = QInputDialog.getText(self, self.tr("Rename Plugin"), self.tr("New plugin name:"), QLineEdit.Normal, oldName)

            if not (newNameTry[1] and newNameTry[0] and oldName != newNameTry[0]):
                return

            newName = newNameTry[0]

            if Carla.host is None or Carla.host.rename_plugin(self.fPluginId, newName):
                self.fPluginInfo['name'] = newName
                self.ui.edit_dialog.fPluginInfo['name'] = newName
                self.ui.edit_dialog.reloadInfo()
                self.ui.label_name.setText(newName)
            else:
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        elif actSel == actRemove:
            if not Carla.host.remove_plugin(self.fPluginId):
                CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                       cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot(bool)
    def slot_enableClicked(self, yesNo):
        self.setActive(yesNo, False, True)

    @pyqtSlot(bool)
    def slot_guiClicked(self, show):
        Carla.host.show_gui(self.fPluginId, show)

    @pyqtSlot(bool)
    def slot_editClicked(self, show):
        self.ui.edit_dialog.setVisible(show)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.save()

        areaX = self.ui.area_right.x()+7

        painter.setPen(self.fColorSeprtr.lighter(110))
        painter.setBrush(self.fColorBottom)
        painter.setRenderHint(QPainter.Antialiasing, True)

        # name -> leds arc
        path = QPainterPath()
        path.moveTo(areaX-20, self.height()-4)
        path.cubicTo(areaX, self.height()-5, areaX-20, 4.75, areaX, 4.75)
        path.lineTo(areaX, self.height()-5)
        painter.drawPath(path)

        painter.setPen(self.fColorSeprtr)
        painter.setRenderHint(QPainter.Antialiasing, False)

        # separator lines
        painter.drawLine(0, self.height()-5, areaX-20, self.height()-5)
        painter.drawLine(areaX, 4, self.width(), 4)

        painter.setPen(self.fColorBottom)
        painter.setBrush(self.fColorBottom)

        # top, bottom and left lines
        painter.drawLine(0, 0, self.width(), 0)
        painter.drawRect(0, self.height()-4, areaX, 4)
        painter.drawRoundedRect(areaX-20, self.height()-5, areaX, 5, 22, 22)
        painter.drawLine(0, 0, 0, self.height())

        # fill the rest
        painter.drawRect(areaX-1, 5, self.width(), self.height())

        # bottom 1px line
        painter.setPen(self.fColorSeprtr)
        painter.drawLine(0, self.height()-1, self.width(), self.height()-1)

        painter.restore()
        QFrame.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# TESTING

#hasGL = True

#from PyQt5.QtWidgets import QApplication
#app = QApplication(sys.argv)
#gui = PluginParameter(None, pInfo, 0, 0)
#gui = PluginEdit(None, 0)
#gui = PluginWidget(None, 0)
#gui.show()
#app.exec_()
