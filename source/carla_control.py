#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
from PyQt4.QtGui import QApplication, QMainWindow
from liblo import make_method, Address, ServerError, ServerThread
from liblo import send as lo_send
from liblo import TCP as LO_TCP

# Imports (Custom)
import ui_carla_control
from shared_carla import *

global lo_target, lo_targetName
lo_target     = None
lo_targetName = ""

Carla.isControl = True

# Python Object dicts compatible to carla-backend struct ctypes
MidiProgramData = {
    'bank': 0,
    'program': 0,
    'label': None
}

ParameterData = {
    'type': PARAMETER_NULL,
    'index': 0,
    'rindex': -1,
    'hints': 0x0,
    'midiChannel': 0,
    'midiCC': -1
}

ParameterRanges = {
    'def': 0.0,
    'min': 0.0,
    'max': 1.0,
    'step': 0.0,
    'stepSmall': 0.0,
    'stepLarge': 0.0
}

CustomData = {
    'type': CUSTOM_DATA_INVALID,
    'key': None,
    'value': None
}

PluginInfo = {
    'type': PLUGIN_NONE,
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'binary': None,
    'name':  None,
    'label': None,
    'maker': None,
    'copyright': None,
    'uniqueId': 0
}

PortCountInfo = {
    'ins': 0,
    'outs': 0,
    'total': 0
}

ParameterInfo = {
    'name': None,
    'symbol': None,
    'unit': None,
    'scalePointCount': 0,
}

ScalePointInfo = {
    'value': 0.0,
    'label': None
}

GuiInfo = {
    'type': GUI_NONE,
    'resizable': False
}

class ControlPluginInfo(object):
    __slots__ = [
        'pluginInfo',
        'pluginRealName',
        'audioCountInfo',
        'midiCountInfo',
        'parameterCountInfo',
        'parameterInfoS',
        'parameterDataS',
        'parameterRangeS',
        'parameterValueS',
        'programCount',
        'programCurrent',
        'programNameS',
        'midiProgramCount',
        'midiProgramCurrent',
        'midiProgramDataS',
        'inPeak',
        'outPeak'
    ]

# Python Object class compatible to 'Host' on the Carla Backend code
class Host(object):
    def __init__(self):
        object.__init__(self)

        self.pluginInfo = []

        for i in range(MAX_PLUGINS):
            self.pluginInfo.append(ControlPluginInfo())
            self._clear(i)

    def _clear(self, index):
        self.pluginInfo[index].pluginInfo = PluginInfo
        self.pluginInfo[index].pluginRealName = None
        self.pluginInfo[index].audioCountInfo = PortCountInfo
        self.pluginInfo[index].midiCountInfo  = PortCountInfo
        self.pluginInfo[index].parameterCountInfo = PortCountInfo
        self.pluginInfo[index].parameterInfoS  = []
        self.pluginInfo[index].parameterDataS  = []
        self.pluginInfo[index].parameterRangeS = []
        self.pluginInfo[index].parameterValueS = []
        self.pluginInfo[index].programCount = 0
        self.pluginInfo[index].programCurrent = -1
        self.pluginInfo[index].programNameS = []
        self.pluginInfo[index].midiProgramCount = 0
        self.pluginInfo[index].midiProgramCurrent = -1
        self.pluginInfo[index].midiProgramDataS = []
        self.pluginInfo[index].inPeak  = [0.0, 0.0]
        self.pluginInfo[index].outPeak = [0.0, 0.0]

    def _set_pluginInfo(self, index, info):
        self.pluginInfo[index].pluginInfo = info

    def _set_pluginRealName(self, index, realName):
        self.pluginInfo[index].pluginRealName = realName

    def _set_audioCountInfo(self, index, info):
        self.pluginInfo[index].audioCountInfo = info

    def _set_midiCountInfo(self, index, info):
        self.pluginInfo[index].midiCountInfo = info

    def _set_parameterCountInfo(self, index, info):
        self.pluginInfo[index].parameterCountInfo = info

        # clear
        self.pluginInfo[index].parameterInfoS  = []
        self.pluginInfo[index].parameterDataS  = []
        self.pluginInfo[index].parameterRangeS = []
        self.pluginInfo[index].parameterValueS = []

        # add placeholders
        for x in range(info['total']):
            self.pluginInfo[index].parameterInfoS.append(ParameterInfo)
            self.pluginInfo[index].parameterDataS.append(ParameterData)
            self.pluginInfo[index].parameterRangeS.append(ParameterRanges)
            self.pluginInfo[index].parameterValueS.append(0.0)

    def _set_programCount(self, index, count):
        self.pluginInfo[index].programCount = count

        # clear
        self.pluginInfo[index].programNameS = []

        # add placeholders
        for x in range(count):
            self.pluginInfo[index].programNameS.append(None)

    def _set_midiProgramCount(self, index, count):
        self.pluginInfo[index].midiProgramCount = count

        # clear
        self.pluginInfo[index].midiProgramDataS = []

        # add placeholders
        for x in range(count):
            self.pluginInfo[index].midiProgramDataS.append(MidiProgramData)

    def _set_parameterInfoS(self, index, paramIndex, data):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterInfoS[paramIndex] = data

    def _set_parameterDataS(self, index, paramIndex, data):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterDataS[paramIndex] = data

    def _set_parameterRangeS(self, index, paramIndex, data):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterRangeS[paramIndex] = data

    def _set_parameterValueS(self, index, paramIndex, value):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterValueS[paramIndex] = value

    def _set_parameterDefaultValue(self, index, paramIndex, value):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterRangeS[paramIndex]['def'] = value

    def _set_parameterMidiCC(self, index, paramIndex, cc):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterDataS[paramIndex]['midiCC'] = cc

    def _set_parameterMidiChannel(self, index, paramIndex, channel):
        if paramIndex < self.pluginInfo[index].parameterCountInfo['total']:
            self.pluginInfo[index].parameterDataS[paramIndex]['midiChannel'] = channel

    def _set_currentProgram(self, index, pIndex):
        self.pluginInfo[index].programCurrent = pIndex

    def _set_currentMidiProgram(self, index, mpIndex):
        self.pluginInfo[index].midiProgramCurrent = mpIndex

    def _set_programNameS(self, index, pIndex, data):
        if pIndex < self.pluginInfo[index].programCount:
            self.pluginInfo[index].programNameS[pIndex] = data

    def _set_midiProgramDataS(self, index, mpIndex, data):
        if mpIndex < self.pluginInfo[index].midiProgramCount:
            self.pluginInfo[index].midiProgramDataS[mpIndex] = data

    def _set_inPeak(self, index, port, value):
        self.pluginInfo[index].inPeak[port] = value

    def _set_outPeak(self, index, port, value):
        self.pluginInfo[index].outPeak[port] = value

    def get_plugin_info(self, plugin_id):
        return self.pluginInfo[plugin_id].pluginInfo

    def get_audio_port_count_info(self, plugin_id):
        return self.pluginInfo[plugin_id].audioCountInfo

    def get_midi_port_count_info(self, plugin_id):
        return self.pluginInfo[plugin_id].midiCountInfo

    def get_parameter_count_info(self, plugin_id):
        return self.pluginInfo[plugin_id].parameterCountInfo

    def get_parameter_info(self, plugin_id, parameter_id):
        return self.pluginInfo[plugin_id].parameterInfoS[parameter_id]

    def get_parameter_scalepoint_info(self, plugin_id, parameter_id, scalepoint_id):
        return ScalePointInfo

    def get_gui_info(self, plugin_id):
        return GuiInfo

    def get_parameter_data(self, plugin_id, parameter_id):
        return self.pluginInfo[plugin_id].parameterDataS[parameter_id]

    def get_parameter_ranges(self, plugin_id, parameter_id):
        return self.pluginInfo[plugin_id].parameterRangeS[parameter_id]

    def get_midi_program_data(self, plugin_id, midi_program_id):
        return self.pluginInfo[plugin_id].midiProgramDataS[midi_program_id]

    def get_custom_data(self, plugin_id, custom_data_id):
        return CustomData

    def get_chunk_data(self, plugin_id):
        return None

    def get_parameter_count(self, plugin_id):
        return self.pluginInfo[plugin_id].parameterCountInfo['total']

    def get_program_count(self, plugin_id):
        return self.pluginInfo[plugin_id].programCount

    def get_midi_program_count(self, plugin_id):
        return self.pluginInfo[plugin_id].midiProgramCount

    def get_custom_data_count(self, plugin_id):
        return 0

    def get_parameter_text(self, plugin_id, program_id):
        return None

    def get_program_name(self, plugin_id, program_id):
        return self.pluginInfo[plugin_id].programNameS[program_id]

    def get_midi_program_name(self, plugin_id, midi_program_id):
        return self.pluginInfo[plugin_id].midiProgramDataS[midi_program_id]['label']

    def get_real_plugin_name(self, plugin_id):
        return self.pluginInfo[plugin_id].pluginRealName

    def get_current_program_index(self, plugin_id):
        return self.pluginInfo[plugin_id].programCurrent

    def get_current_midi_program_index(self, plugin_id):
        return self.pluginInfo[plugin_id].midiProgramCurrent

    def get_default_parameter_value(self, plugin_id, parameter_id):
        return self.pluginInfo[plugin_id].parameterRangeS[parameter_id]['def']

    def get_current_parameter_value(self, plugin_id, parameter_id):
        return self.pluginInfo[plugin_id].parameterValueS[parameter_id]

    def get_input_peak_value(self, plugin_id, port_id):
        return self.pluginInfo[plugin_id].inPeak[port_id-1]

    def get_output_peak_value(self, plugin_id, port_id):
        return self.pluginInfo[plugin_id].outPeak[port_id-1]

    def set_active(self, plugin_id, onoff):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_active" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, 1 if onoff else 0)

    def set_drywet(self, plugin_id, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_drywet" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_volume(self, plugin_id, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_volume" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_balance_left(self, plugin_id, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_balance_left" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_balance_right(self, plugin_id, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_balance_right" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, value)

    def set_parameter_value(self, plugin_id, parameter_id, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_value" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, value)

    def set_parameter_midi_channel(self, plugin_id, parameter_id, channel):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_midi_channel" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, channel)

    def set_parameter_midi_cc(self, plugin_id, parameter_id, midi_cc):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_midi_cc" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, parameter_id, midi_cc)

    def set_program(self, plugin_id, program_id):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_program" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, program_id)

    def set_midi_program(self, plugin_id, midi_program_id):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_midi_program" % (lo_targetName, plugin_id)
        lo_send(lo_target, lo_path, midi_program_id)

    def send_midi_note(self, plugin_id, channel, note, velocity):
        global to_target, lo_targetName
        if velocity:
            lo_path = "/%s/%i/note_on" % (lo_targetName, plugin_id)
            lo_send(lo_target, lo_path, channel, note, velocity)
        else:
            lo_path = "/%s/%i/note_off" % (lo_targetName, plugin_id)
            lo_send(lo_target, lo_path, channel, note)

# OSC Control server
class ControlServer(ServerThread):
    def __init__(self, parent):
        ServerThread.__init__(self, 8087, LO_TCP)

        self.parent = parent

    def getFullURL(self):
        return "%scarla-control" % self.get_url()

    @make_method('/carla-control/add_plugin_start', 'is')
    def add_plugin_start_callback(self, path, args):
        pluginId, pluginName = args
        self.parent.emit(SIGNAL("AddPluginStart(int, QString)"), pluginId, pluginName)

    @make_method('/carla-control/add_plugin_end', 'i')
    def add_plugin_end_callback(self, path, args):
        pluginId, = args
        self.parent.emit(SIGNAL("AddPluginEnd(int)"), pluginId)

    @make_method('/carla-control/remove_plugin', 'i')
    def remove_plugin_callback(self, path, args):
        pluginId, = args
        self.parent.emit(SIGNAL("RemovePlugin(int)"), pluginId)

    @make_method('/carla-control/set_plugin_data', 'iiiissssh')
    def set_plugin_data_callback(self, path, args):
        pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId = args
        self.parent.emit(SIGNAL("SetPluginData(int, int, int, int, QString, QString, QString, QString, int)"), pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId)

    @make_method('/carla-control/set_plugin_ports', 'iiiiiiii')
    def set_plugin_ports_callback(self, path, args):
        pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals = args
        self.parent.emit(SIGNAL("SetPluginPorts(int, int, int, int, int, int, int, int)"), pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals)

    @make_method('/carla-control/set_parameter_data', 'iiiissd')
    def set_parameter_data_callback(self, path, args):
        pluginId, index, type_, hints, name, label, current = args
        self.parent.emit(SIGNAL("SetParameterData(int, int, int, int, QString, QString, double)"), pluginId, index, type_, hints, name, label, current)

    @make_method('/carla-control/set_parameter_ranges', 'iidddddd')
    def set_parameter_ranges_callback(self, path, args):
        pluginId, index, min_, max_, def_, step, stepSmall, stepLarge = args
        self.parent.emit(SIGNAL("SetParameterRanges(int, int, double, double, double, double, double, double)"), pluginId, index, min_, max_, def_, step, stepSmall, stepLarge)

    @make_method('/carla-control/set_parameter_midi_cc', 'iii')
    def set_parameter_midi_cc_callback(self, path, args):
        pluginId, index, cc = args
        self.parent.emit(SIGNAL("SetParameterMidiCC(int, int, int)"), pluginId, index, cc)

    @make_method('/carla-control/set_parameter_midi_channel', 'iii')
    def set_parameter_midi_channel_callback(self, path, args):
        pluginId, index, channel = args
        self.parent.emit(SIGNAL("SetParameterMidiChannel(int, int, int)"), pluginId, index, channel)

    @make_method('/carla-control/set_parameter_value', 'iid')
    def set_parameter_value_callback(self, path, args):
        pluginId, index, value = args
        self.parent.emit(SIGNAL("SetParameterValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_default_value', 'iid')
    def set_default_value_callback(self, path, args):
        pluginId, index, value = args
        self.parent.emit(SIGNAL("SetDefaultValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_program', 'ii')
    def set_program_callback(self, path, args):
        pluginId, index = args
        self.parent.emit(SIGNAL("SetProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_program_count', 'ii')
    def set_program_count_callback(self, path, args):
        pluginId, count = args
        self.parent.emit(SIGNAL("SetProgramCount(int, int)"), pluginId, count)

    @make_method('/carla-control/set_program_name', 'iis')
    def set_program_name_callback(self, path, args):
        pluginId, index, name = args
        self.parent.emit(SIGNAL("SetProgramName(int, int, QString)"), pluginId, index, name)

    @make_method('/carla-control/set_midi_program', 'ii')
    def set_midi_program_callback(self, path, args):
        pluginId, index = args
        self.parent.emit(SIGNAL("SetMidiProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_midi_program_count', 'ii')
    def set_midi_program_count_callback(self, path, args):
        pluginId, count = args
        self.parent.emit(SIGNAL("SetMidiProgramCount(int, int)"), pluginId, count)

    @make_method('/carla-control/set_midi_program_data', 'iiiis')
    def set_midi_program_data_callback(self, path, args):
        pluginId, index, bank, program, name = args
        self.parent.emit(SIGNAL("SetMidiProgramData(int, int, int, int, QString)"), pluginId, index, bank, program, name)

    @make_method('/carla-control/set_input_peak_value', 'iid')
    def set_input_peak_value_callback(self, path, args):
        pluginId, portId, value = args
        self.parent.emit(SIGNAL("SetInputPeakValue(int, int, double)"), pluginId, portId, value)

    @make_method('/carla-control/set_output_peak_value', 'iid')
    def set_output_peak_value_callback(self, path, args):
        pluginId, portId, value = args
        self.parent.emit(SIGNAL("SetOutputPeakValue(int, int, double)"), pluginId, portId, value)

    @make_method('/carla-control/note_on', 'iiii')
    def note_on_callback(self, path, args):
        pluginId, channel, note, velo = args
        self.parent.emit(SIGNAL("NoteOn(int, int, int, int)"), pluginId, channel, note, velo)

    @make_method('/carla-control/note_off', 'iii')
    def note_off_callback(self, path, args):
        pluginId, channel, note = args
        self.parent.emit(SIGNAL("NoteOff(int, int, int)"), pluginId, channel, note)

    @make_method('/carla-control/exit', '')
    def exit_callback(self, path, args):
        self.parent.emit(SIGNAL("Exit()"))

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServer::fallback(\"%s\") - unknown message, args =" % path, args)

# Main Window
class CarlaControlW(QMainWindow, ui_carla_control.Ui_CarlaControlW):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)

        # -------------------------------------------------------------
        # Load Settings

        self.settings = QSettings("Cadence", "Carla-Control")
        self.loadSettings()

        self.setStyleSheet("""
          QWidget#centralwidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                stop:0 rgb( 7,  7,  7),
                stop:1 rgb(28, 28, 28)
            );
          }
        """)

        # -------------------------------------------------------------
        # Internal stuff

        self.lo_address = ""
        self.lo_server  = None

        self.m_lastPluginName = None

        self.m_plugin_list = []
        for x in range(MAX_PLUGINS):
            self.m_plugin_list.append(None)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.act_file_refresh.setEnabled(False)
        self.resize(self.width(), 0)

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.act_file_connect, SIGNAL("triggered()"), SLOT("slot_doConnect()"))
        self.connect(self.act_file_refresh, SIGNAL("triggered()"), SLOT("slot_doRefresh()"))

        self.connect(self.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarlaControl()"))
        self.connect(self.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("AddPluginStart(int, QString)"), SLOT("slot_handleAddPluginStart(int, QString)"))
        self.connect(self, SIGNAL("AddPluginEnd(int)"), SLOT("slot_handleAddPluginEnd(int)"))
        self.connect(self, SIGNAL("RemovePlugin(int)"), SLOT("slot_handleRemovePlugin(int)"))
        self.connect(self, SIGNAL("SetPluginData(int, int, int, int, QString, QString, QString, QString, int)"), SLOT("slot_handleSetPluginData(int, int, int, int, QString, QString, QString, QString, int)"))
        self.connect(self, SIGNAL("SetPluginPorts(int, int, int, int, int, int, int, int)"), SLOT("slot_handleSetPluginPorts(int, int, int, int, int, int, int, int)"))
        self.connect(self, SIGNAL("SetParameterData(int, int, int, int, QString, QString, double)"), SLOT("slot_handleSetParameterData(int, int, int, int, QString, QString, double)"))
        self.connect(self, SIGNAL("SetParameterRanges(int, int, double, double, double, double, double, double)"), SLOT("slot_handleSetParameterRanges(int, int, double, double, double, double, double, double)"))
        self.connect(self, SIGNAL("SetParameterMidiCC(int, int, int)"), SLOT("slot_handleSetParameterMidiCC(int, int, int)"))
        self.connect(self, SIGNAL("SetParameterMidiChannel(int, int, int)"), SLOT("slot_handleSetParameterMidiChannel(int, int, int)"))
        self.connect(self, SIGNAL("SetParameterValue(int, int, double)"), SLOT("slot_handleSetParameterValue(int, int, double)"))
        self.connect(self, SIGNAL("SetDefaultValue(int, int, double)"), SLOT("slot_handleSetDefaultValue(int, int, double)"))
        self.connect(self, SIGNAL("SetProgram(int, int)"), SLOT("slot_handleSetProgram(int, int)"))
        self.connect(self, SIGNAL("SetProgramCount(int, int)"), SLOT("slot_handleSetProgramCount(int, int)"))
        self.connect(self, SIGNAL("SetProgramName(int, int, QString)"), SLOT("slot_handleSetProgramName(int, int, QString)"))
        self.connect(self, SIGNAL("SetMidiProgram(int, int)"), SLOT("slot_handleSetMidiProgram(int, int)"))
        self.connect(self, SIGNAL("SetMidiProgramCount(int, int)"), SLOT("slot_handleSetMidiProgramCount(int, int)"))
        self.connect(self, SIGNAL("SetMidiProgramData(int, int, int, int, QString)"), SLOT("slot_handleSetMidiProgramData(int, int, int, int, QString)"))
        self.connect(self, SIGNAL("SetInputPeakValue(int, int, double)"), SLOT("slot_handleSetInputPeakValue(int, int, double)"))
        self.connect(self, SIGNAL("SetOutputPeakValue(int, int, double)"), SLOT("slot_handleSetOutputPeakValue(int, int, double)"))
        self.connect(self, SIGNAL("NoteOn(int, int, int, int)"), SLOT("slot_handleNoteOn(int, int, int, int)"))
        self.connect(self, SIGNAL("NoteOff(int, int, int)"), SLOT("slot_handleNoteOff(int, int, int)"))
        self.connect(self, SIGNAL("Exit()"), SLOT("slot_handleExit()"))

        self.TIMER_GUI_STUFF  = self.startTimer(50)     # Peaks
        self.TIMER_GUI_STUFF2 = self.startTimer(50 * 2) # LEDs and edit dialog

    def remove_all(self):
        for i in range(MAX_PLUGINS):
            if self.m_plugin_list[i]:
                self.slot_handleRemovePlugin(i)

    @pyqtSlot()
    def slot_doConnect(self):
        global lo_target, lo_targetName

        if lo_target and self.lo_server:
            urlText = self.lo_address
        else:
            urlText = "osc.tcp://127.0.0.1:19000/Carla"

        askValue = QInputDialog.getText(self, self.tr("Carla Control - Connect"), self.tr("Address"), text=urlText)

        if not askValue[1]:
            return

        self.slot_handleExit()

        self.lo_address = askValue[0]
        lo_target       = Address(self.lo_address)
        lo_targetName   = self.lo_address.rsplit("/", 1)[-1]
        print("Connecting to \"%s\" as '%s'..." % (self.lo_address, lo_targetName))

        try:
            self.lo_server = ControlServer(self)
        except: # ServerError, err:
            print("Connecting error!")
            #print str(err)
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to connect, operation failed."))

        if self.lo_server:
            self.lo_server.start()
            self.act_file_refresh.setEnabled(True)
            lo_send(lo_target, "/register", self.lo_server.getFullURL())

    @pyqtSlot()
    def slot_doRefresh(self):
        global lo_target
        if lo_target and self.lo_server:
            self.remove_all()
            lo_send(lo_target, "/unregister")
            lo_send(lo_target, "/register", self.lo_server.getFullURL())

    @pyqtSlot()
    def slot_aboutCarlaControl(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot(int, str)
    def slot_handleAddPluginStart(self, pluginId, pluginName):
        self.m_lastPluginName = pluginName

    @pyqtSlot(int)
    def slot_handleAddPluginEnd(self, pluginId):
        pwidget = PluginWidget(self, pluginId)
        self.w_plugins.layout().addWidget(pwidget)
        self.m_plugin_list[pluginId] = pwidget

    @pyqtSlot(int)
    def slot_handleRemovePlugin(self, pluginId):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.close()
            pwidget.close()
            pwidget.deleteLater()
            self.w_plugins.layout().removeWidget(pwidget)
            self.m_plugin_list[pluginId] = None

    @pyqtSlot(int, int, int, int, str, str, str, str, int)
    def slot_handleSetPluginData(self, pluginId, type_, category, hints, realName, label, maker, copyright, uniqueId):
        info = deepcopy(PluginInfo)
        info['type']      = type_
        info['category']  = category
        info['hints']     = hints
        info['name']      = self.m_lastPluginName
        info['label']     = label
        info['maker']     = maker
        info['copyright'] = copyright
        info['uniqueId']  = uniqueId
        Carla.host._set_pluginInfo(pluginId, info)
        Carla.host._set_pluginRealName(pluginId, realName)

    @pyqtSlot(int, int, int, int, int, int, int, int)
    def slot_handleSetPluginPorts(self, pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals):
        audioInfo = deepcopy(PortCountInfo)
        midiInfo  = deepcopy(PortCountInfo)
        paramInfo = deepcopy(PortCountInfo)

        audioInfo['ins']   = audioIns
        audioInfo['outs']  = audioOuts
        audioInfo['total'] = audioIns + audioOuts

        midiInfo['ins']   = midiIns
        midiInfo['outs']  = midiOuts
        midiInfo['total'] = midiIns + midiOuts

        paramInfo['ins']   = cIns
        paramInfo['outs']  = cOuts
        paramInfo['total'] = cTotals

        Carla.host._set_audioCountInfo(pluginId, audioInfo)
        Carla.host._set_midiCountInfo(pluginId, midiInfo)
        Carla.host._set_parameterCountInfo(pluginId, paramInfo)

    @pyqtSlot(int, int, int, int, str, str, float)
    def slot_handleSetParameterData(self, pluginId, index, type_, hints, name, label, current):
        # remove hints not possible in bridge mode
        hints &= ~(PARAMETER_USES_SCALEPOINTS | PARAMETER_USES_CUSTOM_TEXT)

        data = deepcopy(ParameterData)
        data['type']   = type_
        data['index']  = index
        data['rindex'] = index
        data['hints']  = hints

        info = deepcopy(ParameterInfo)
        info['name']  = name
        info['label'] = label

        Carla.host._set_parameterDataS(pluginId, index, data)
        Carla.host._set_parameterInfoS(pluginId, index, info)
        Carla.host._set_parameterValueS(pluginId, index, current)

    @pyqtSlot(int, int, float, float, float, float, float, float)
    def slot_handleSetParameterRanges(self, pluginId, index, min_, max_, def_, step, stepSmall, stepLarge):
        ranges = deepcopy(ParameterRanges)
        ranges['min'] = min_
        ranges['max'] = max_
        ranges['def'] = def_
        ranges['step'] = step
        ranges['stepSmall'] = stepSmall
        ranges['stepLarge'] = stepLarge

        Carla.host._set_parameterRangeS(pluginId, index, ranges)

    @pyqtSlot(int, int, int)
    def slot_handleSetParameterMidiCC(self, pluginId, index, cc):
        Carla.host._set_parameterMidiCC(pluginId, index, cc)

        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_parameter_midi_cc(index, cc, True)

    @pyqtSlot(int, int, int)
    def slot_handleSetParameterMidiChannel(self, pluginId, index, channel):
        Carla.host._set_parameterMidiChannel(pluginId, index, channel)

        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_parameter_midi_channel(index, channel, True)

    @pyqtSlot(int, int, float)
    def slot_handleSetParameterValue(self, pluginId, parameterId, value):
        if parameterId >= 0:
            Carla.host._set_parameterValueS(pluginId, parameterId, value)

        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            if parameterId < PARAMETER_NULL:
                pwidget.m_parameterIconTimer = ICON_STATE_ON
            else:
                for param in pwidget.edit_dialog.m_parameterList:
                    if param[1] == parameterId:
                        if param[0] == PARAMETER_INPUT:
                            pwidget.m_parameterIconTimer = ICON_STATE_ON
                        break

            if parameterId == PARAMETER_ACTIVE:
                pwidget.set_active((value > 0.0), True, False)
            elif parameterId == PARAMETER_DRYWET:
                pwidget.set_drywet(value * 1000, True, False)
            elif parameterId == PARAMETER_VOLUME:
                pwidget.set_volume(value * 1000, True, False)
            elif parameterId == PARAMETER_BALANCE_LEFT:
                pwidget.set_balance_left(value * 1000, True, False)
            elif parameterId == PARAMETER_BALANCE_RIGHT:
                pwidget.set_balance_right(value * 1000, True, False)
            elif parameterId >= 0:
                pwidget.edit_dialog.set_parameter_to_update(parameterId)

    @pyqtSlot(int, int, float)
    def slot_handleSetDefaultValue(self, pluginId, parameterId, value):
        Carla.host._set_parameterDefaultValue(pluginId, parameterId, value)

        #pwidget = self.m_plugin_list[pluginId]
        #if pwidget:
            #pwidget.edit_dialog.set_parameter_default_value(parameterId, value)

    @pyqtSlot(int, int)
    def slot_handleSetProgram(self, pluginId, index):
        Carla.host._set_currentProgram(pluginId, index)

        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_program(index)
            pwidget.m_parameterIconTimer = ICON_STATE_ON

    @pyqtSlot(int, int)
    def slot_handleSetProgramCount(self, pluginId, count):
        Carla.host._set_programCount(pluginId, count)

    @pyqtSlot(int, int, str)
    def slot_handleSetProgramName(self, pluginId, index, name):
        Carla.host._set_programNameS(pluginId, index, name)

    @pyqtSlot(int, int)
    def slot_handleSetMidiProgram(self, pluginId, index):
        Carla.host._set_currentMidiProgram(pluginId, index)

        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.set_midi_program(index)
            pwidget.m_parameterIconTimer = ICON_STATE_ON

    @pyqtSlot(int, int)
    def slot_handleSetMidiProgramCount(self, pluginId, count):
        Carla.host._set_midiProgramCount(pluginId, count)

    @pyqtSlot(int, int, int, int, str)
    def slot_handleSetMidiProgramData(self, pluginId, index, bank, program, name):
        data = deepcopy(MidiProgramData)
        data['bank']    = bank
        data['program'] = program
        data['label']   = name
        Carla.host._set_midiProgramDataS(pluginId, index, data)

    @pyqtSlot(int, int, float)
    def slot_handleSetInputPeakValue(self, pluginId, portId, value):
        Carla.host._set_inPeak(pluginId, portId-1, value)

    @pyqtSlot(int, int, float)
    def slot_handleSetOutputPeakValue(self, pluginId, portId, value):
        Carla.host._set_outPeak(pluginId, portId-1, value)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOn(self, pluginId, channel, note, velo):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOff(self, pluginId, channel, note):
        pwidget = self.m_plugin_list[pluginId]
        if pwidget:
            pwidget.edit_dialog.keyboard.sendNoteOff(note, False)

    @pyqtSlot()
    def slot_handleExit(self):
        self.remove_all()

        if self.lo_server:
            self.lo_server.stop()
            self.lo_server = None
            self.act_file_refresh.setEnabled(False)

        global lo_target, lo_targetName
        lo_target     = None
        lo_targetName = ""
        self.lo_address = ""

    def saveSettings(self):
        self.settings.setValue("Geometry", self.saveGeometry())
        #self.settings.setValue("ShowToolbar", self.toolBar.isVisible())

    def loadSettings(self):
        self.restoreGeometry(self.settings.value("Geometry", ""))

        #show_toolbar = self.settings.value("ShowToolbar", True, type=bool)
        #self.act_settings_show_toolbar.setChecked(show_toolbar)
        #self.toolBar.setVisible(show_toolbar)

    def timerEvent(self, event):
        if event.timerId() == self.TIMER_GUI_STUFF:
            for pwidget in self.m_plugin_list:
                if pwidget: pwidget.check_gui_stuff()
        elif event.timerId() == self.TIMER_GUI_STUFF2:
            for pwidget in self.m_plugin_list:
                if pwidget: pwidget.check_gui_stuff2()
        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        self.saveSettings()

        global lo_target
        if lo_target and self.lo_server:
            lo_send(lo_target, "/unregister")

        QMainWindow.closeEvent(self, event)

#--------------- main ------------------
if __name__ == '__main__':

    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla-Control")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/carla-control.svg"))

    Carla.host = Host()

    # Create GUI
    Carla.gui = CarlaControlW()

    # Set-up custom signal handling
    setUpSignals(Carla.gui)

    # Show GUI
    Carla.gui.show()

    # App-Loop
    sys.exit(app.exec_())
