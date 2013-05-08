#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla OSC controller
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
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt4.QtCore import QLibrary
from PyQt4.QtGui import QApplication, QInputDialog, QMainWindow
from liblo import make_method, Address, ServerError, ServerThread
from liblo import send as lo_send
from liblo import TCP as LO_TCP

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_control
from carla_shared import *

global lo_target, lo_targetName
lo_target     = None
lo_targetName = ""

Carla.isControl = True
Carla.isLocal   = False

# ------------------------------------------------------------------------------------------------------------
# Python Object dicts compatible to carla-backend struct ctypes

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

MidiProgramData = {
    'bank': 0,
    'program': 0,
    'name': None
}

CustomData = {
    'type': CUSTOM_DATA_INVALID,
    'key': None,
    'value': None
}

# ------------------------------------------------------------------------------------------------------------

CarlaPluginInfo = {
    'type': PLUGIN_NONE,
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'optionsAvailable': 0x0,
    'optionsEnabled': 0x0,
    'binary': None,
    'name':  None,
    'label': None,
    'maker': None,
    'copyright': None,
    'uniqueId': 0,
    'latency': 0
}

CarlaNativePluginInfo = {
    'category': PLUGIN_CATEGORY_NONE,
    'hints': 0x0,
    'audioIns': 0,
    'audioOuts': 0,
    'midiIns': 0,
    'midiOuts': 0,
    'parameterIns': 0,
    'parameterOuts': 0,
    'name':  None,
    'label': None,
    'maker': None,
    'copyright': None
}

CarlaPortCountInfo = {
    'ins': 0,
    'outs': 0,
    'total': 0
}

CarlaParameterInfo = {
    'name': None,
    'symbol': None,
    'unit': None,
    'scalePointCount': 0,
}

CarlaScalePointInfo = {
    'value': 0.0,
    'label': None
}

CarlaTransportInfo = {
    'playing': False,
    'frame': 0,
    'bar': 0,
    'beat': 0,
    'tick': 0,
    'bpm': 0.0
}

# ------------------------------------------------------------------------------------------------------------
# Helper class

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

# ------------------------------------------------------------------------------------------------------------
# Python Object class compatible to 'Host' on the Carla Backend code

class Host(object):
    def __init__(self):
        object.__init__(self)

        self.fPluginsInfo = []

    def _add(self, pluginId):
        if len(self.fPluginsInfo) != pluginId:
            return

        info = deepcopy(ControlPluginInfo)
        info.pluginInfo = CarlaPluginInfo
        info.pluginRealName = None
        info.audioCountInfo = CarlaPortCountInfo
        info.midiCountInfo  = CarlaPortCountInfo
        info.parameterCountInfo = CarlaPortCountInfo
        info.parameterInfoS  = []
        info.parameterDataS  = []
        info.parameterRangeS = []
        info.parameterValueS = []
        info.programCount = 0
        info.programCurrent = -1
        info.programNameS = []
        info.midiProgramCount = 0
        info.midiProgramCurrent = -1
        info.midiProgramDataS = []
        info.peaks = [0.0, 0.0, 0.0, 0.0]
        self.fPluginsInfo.append(info)

    def _set_pluginInfo(self, index, info):
        self.fPluginsInfo[index].pluginInfo = info

    def _set_pluginRealName(self, index, realName):
        self.fPluginsInfo[index].pluginRealName = realName

    def _set_audioCountInfo(self, index, info):
        self.fPluginsInfo[index].audioCountInfo = info

    def _set_midiCountInfo(self, index, info):
        self.fPluginsInfo[index].midiCountInfo = info

    def _set_parameterCountInfo(self, index, info):
        self.fPluginsInfo[index].parameterCountInfo = info

        # clear
        self.fPluginsInfo[index].parameterInfoS  = []
        self.fPluginsInfo[index].parameterDataS  = []
        self.fPluginsInfo[index].parameterRangeS = []
        self.fPluginsInfo[index].parameterValueS = []

        # add placeholders
        for x in range(info['total']):
            self.fPluginsInfo[index].parameterInfoS.append(CarlaParameterInfo)
            self.fPluginsInfo[index].parameterDataS.append(ParameterData)
            self.fPluginsInfo[index].parameterRangeS.append(ParameterRanges)
            self.fPluginsInfo[index].parameterValueS.append(0.0)

    def _set_programCount(self, index, count):
        self.fPluginsInfo[index].programCount = count

        # clear
        self.fPluginsInfo[index].programNameS = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[index].programNameS.append(None)

    def _set_midiProgramCount(self, index, count):
        self.fPluginsInfo[index].midiProgramCount = count

        # clear
        self.fPluginsInfo[index].midiProgramDataS = []

        # add placeholders
        for x in range(count):
            self.fPluginsInfo[index].midiProgramDataS.append(MidiProgramData)

    def _set_parameterInfoS(self, index, paramIndex, data):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterInfoS[paramIndex] = data

    def _set_parameterDataS(self, index, paramIndex, data):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterDataS[paramIndex] = data

    def _set_parameterRangeS(self, index, paramIndex, data):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterRangeS[paramIndex] = data

    def _set_parameterValueS(self, index, paramIndex, value):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterValueS[paramIndex] = value

    def _set_parameterDefaultValue(self, index, paramIndex, value):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterRangeS[paramIndex]['def'] = value

    def _set_parameterMidiCC(self, index, paramIndex, cc):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterDataS[paramIndex]['midiCC'] = cc

    def _set_parameterMidiChannel(self, index, paramIndex, channel):
        if paramIndex < self.fPluginsInfo[index].parameterCountInfo['total']:
            self.fPluginsInfo[index].parameterDataS[paramIndex]['midiChannel'] = channel

    def _set_currentProgram(self, index, pIndex):
        self.fPluginsInfo[index].programCurrent = pIndex

    def _set_currentMidiProgram(self, index, mpIndex):
        self.fPluginsInfo[index].midiProgramCurrent = mpIndex

    def _set_programNameS(self, index, pIndex, data):
        if pIndex < self.fPluginsInfo[index].programCount:
            self.fPluginsInfo[index].programNameS[pIndex] = data

    def _set_midiProgramDataS(self, index, mpIndex, data):
        if mpIndex < self.fPluginsInfo[index].midiProgramCount:
            self.fPluginsInfo[index].midiProgramDataS[mpIndex] = data

    def _set_peaks(self, index, in1, in2, out1, out2):
        self.fPluginsInfo[index].peaks = [in1, in2, out1, out2]

    # get_extended_license_text
    # get_supported_file_types
    # get_engine_driver_count
    # get_engine_driver_name
    # get_engine_driver_device_names
    # get_internal_plugin_count
    # get_internal_plugin_info
    # engine_init
    # engine_close
    # engine_idle
    # is_engine_running
    # set_engine_about_to_close
    # set_engine_callback
    # set_engine_option
    # load_filename
    # load_project
    # save_project
    # patchbay_connect
    # patchbay_disconnect
    # patchbay_refresh
    # transport_play
    # transport_pause
    # transport_relocate
    # get_current_transport_frame
    # get_transport_info
    # add_plugin
    # remove_plugin
    # remove_all_plugins
    # rename_plugin
    # clone_plugin
    # replace_plugin
    # switch_plugins
    # load_plugin_state
    # save_plugin_state

    def get_plugin_info(self, pluginId):
        return self.fPluginsInfo[pluginId].pluginInfo

    def get_audio_port_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].audioCountInfo

    def get_midi_port_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].midiCountInfo

    def get_parameter_count_info(self, pluginId):
        return self.fPluginsInfo[pluginId].parameterCountInfo

    def get_parameter_info(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterInfoS[parameterId]

    def get_parameter_scalepoint_info(self, pluginId, parameterId, scalepoint_id):
        return CarlaScalePointInfo

    def get_parameter_data(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterDataS[parameterId]

    def get_parameter_ranges(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterRangeS[parameterId]

    def get_midi_program_data(self, pluginId, midiProgramId):
        return self.fPluginsInfo[pluginId].midiProgramDataS[midiProgramId]

    def get_custom_data(self, pluginId, custom_data_id):
        return CustomData

    def get_chunk_data(self, pluginId):
        return None

    def get_parameter_count(self, pluginId):
        return self.fPluginsInfo[pluginId].parameterCountInfo['total']

    def get_program_count(self, pluginId):
        return self.fPluginsInfo[pluginId].programCount

    def get_midi_program_count(self, pluginId):
        return self.fPluginsInfo[pluginId].midiProgramCount

    def get_custom_data_count(self, pluginId):
        return 0

    def get_parameter_text(self, pluginId, parameterId):
        return None

    def get_program_name(self, pluginId, programId):
        return self.fPluginsInfo[pluginId].programNameS[programId]

    def get_midi_program_name(self, pluginId, midiProgramId):
        return self.fPluginsInfo[pluginId].midiProgramDataS[midiProgramId]['label']

    def get_real_plugin_name(self, pluginId):
        return self.fPluginsInfo[pluginId].pluginRealName

    def get_current_program_index(self, pluginId):
        return self.fPluginsInfo[pluginId].programCurrent

    def get_current_midi_program_index(self, pluginId):
        return self.fPluginsInfo[pluginId].midiProgramCurrent

    def get_default_parameter_value(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterRangeS[parameterId]['def']

    def get_current_parameter_value(self, pluginId, parameterId):
        return self.fPluginsInfo[pluginId].parameterValueS[parameterId]

    def get_input_peak_value(self, pluginId, portId):
        return self.fPluginsInfo[pluginId].peaks[portId-1]

    def get_output_peak_value(self, pluginId, portId):
        return self.fPluginsInfo[pluginId].peaks[2+portId-1]

    def set_option(self, pluginId, option, yesNo):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_option" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, option, yesNo)

    def set_active(self, pluginId, onoff):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_active" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, 1 if onoff else 0)

    def set_drywet(self, pluginId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_drywet" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, value)

    def set_volume(self, pluginId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_volume" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, value)

    def set_balance_left(self, pluginId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_balance_left" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, value)

    def set_balance_right(self, pluginId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_balance_right" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, value)

    def set_panning(self, pluginId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_panning" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, value)

    def set_ctrl_channel(self, pluginId, channel):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_ctrl_channel" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, channel)

    def set_parameter_value(self, pluginId, parameterId, value):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_value" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, parameterId, value)

    def set_parameter_midi_cc(self, pluginId, parameterId, midi_cc):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_midi_cc" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, parameterId, midi_cc)

    def set_parameter_midi_channel(self, pluginId, parameterId, channel):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_parameter_midi_channel" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, parameterId, channel)

    def set_program(self, pluginId, programId):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_program" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, programId)

    def set_midi_program(self, pluginId, midiProgramId):
        global to_target, lo_targetName
        lo_path = "/%s/%i/set_midi_program" % (lo_targetName, pluginId)
        lo_send(lo_target, lo_path, midiProgramId)

    # set_custom_data
    # set_chunk_data
    # prepare_for_save

    def send_midi_note(self, pluginId, channel, note, velocity):
        global to_target, lo_targetName
        if velocity:
            lo_path = "/%s/%i/note_on" % (lo_targetName, pluginId)
            lo_send(lo_target, lo_path, channel, note, velocity)
        else:
            lo_path = "/%s/%i/note_off" % (lo_targetName, pluginId)
            lo_send(lo_target, lo_path, channel, note)

    # show_gui
    # get_buffer_size
    # get_sample_rate
    # get_last_error
    # get_host_osc_url
    # nsm_announce
    # nsm_reply_open
    # nsm_reply_save

# ------------------------------------------------------------------------------------------------------------
# OSC Control server

class ControlServer(ServerThread):
    def __init__(self, parent):
        ServerThread.__init__(self, 8087, LO_TCP)

        self.fParent = parent

    def getFullURL(self):
        return "%scarla-control" % self.get_url()

    @make_method('/carla-control/add_plugin_start', 'is')
    def add_plugin_start_callback(self, path, args):
        pluginId, pluginName = args
        self.fParent.emit(SIGNAL("AddPluginStart(int, QString)"), pluginId, pluginName)

    @make_method('/carla-control/add_plugin_end', 'i')
    def add_plugin_end_callback(self, path, args):
        pluginId, = args
        self.fParent.emit(SIGNAL("AddPluginEnd(int)"), pluginId)

    @make_method('/carla-control/remove_plugin', 'i')
    def remove_plugin_callback(self, path, args):
        pluginId, = args
        self.fParent.emit(SIGNAL("RemovePlugin(int)"), pluginId)

    @make_method('/carla-control/set_plugin_data', 'iiiissssh')
    def set_plugin_data_callback(self, path, args):
        pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId = args
        self.fParent.emit(SIGNAL("SetPluginData(int, int, int, int, QString, QString, QString, QString, int)"), pluginId, ptype, category, hints, realName, label, maker, copyright_, uniqueId)

    @make_method('/carla-control/set_plugin_ports', 'iiiiiiii')
    def set_plugin_ports_callback(self, path, args):
        pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals = args
        self.fParent.emit(SIGNAL("SetPluginPorts(int, int, int, int, int, int, int, int)"), pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals)

    @make_method('/carla-control/set_parameter_data', 'iiiissf')
    def set_parameter_data_callback(self, path, args):
        pluginId, index, type_, hints, name, label, current = args
        self.fParent.emit(SIGNAL("SetParameterData(int, int, int, int, QString, QString, double)"), pluginId, index, type_, hints, name, label, current)

    @make_method('/carla-control/set_parameter_ranges', 'iiffffff')
    def set_parameter_ranges_callback(self, path, args):
        pluginId, index, min_, max_, def_, step, stepSmall, stepLarge = args
        self.fParent.emit(SIGNAL("SetParameterRanges(int, int, double, double, double, double, double, double)"), pluginId, index, min_, max_, def_, step, stepSmall, stepLarge)

    @make_method('/carla-control/set_parameter_midi_cc', 'iii')
    def set_parameter_midi_cc_callback(self, path, args):
        pluginId, index, cc = args
        self.fParent.emit(SIGNAL("SetParameterMidiCC(int, int, int)"), pluginId, index, cc)

    @make_method('/carla-control/set_parameter_midi_channel', 'iii')
    def set_parameter_midi_channel_callback(self, path, args):
        pluginId, index, channel = args
        self.fParent.emit(SIGNAL("SetParameterMidiChannel(int, int, int)"), pluginId, index, channel)

    @make_method('/carla-control/set_parameter_value', 'iif')
    def set_parameter_value_callback(self, path, args):
        pluginId, index, value = args
        self.fParent.emit(SIGNAL("SetParameterValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_default_value', 'iif')
    def set_default_value_callback(self, path, args):
        pluginId, index, value = args
        self.fParent.emit(SIGNAL("SetDefaultValue(int, int, double)"), pluginId, index, value)

    @make_method('/carla-control/set_program', 'ii')
    def set_program_callback(self, path, args):
        pluginId, index = args
        self.fParent.emit(SIGNAL("SetProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_program_count', 'ii')
    def set_program_count_callback(self, path, args):
        pluginId, count = args
        self.fParent.emit(SIGNAL("SetProgramCount(int, int)"), pluginId, count)

    @make_method('/carla-control/set_program_name', 'iis')
    def set_program_name_callback(self, path, args):
        pluginId, index, name = args
        self.fParent.emit(SIGNAL("SetProgramName(int, int, QString)"), pluginId, index, name)

    @make_method('/carla-control/set_midi_program', 'ii')
    def set_midi_program_callback(self, path, args):
        pluginId, index = args
        self.fParent.emit(SIGNAL("SetMidiProgram(int, int)"), pluginId, index)

    @make_method('/carla-control/set_midi_program_count', 'ii')
    def set_midi_program_count_callback(self, path, args):
        pluginId, count = args
        self.fParent.emit(SIGNAL("SetMidiProgramCount(int, int)"), pluginId, count)

    @make_method('/carla-control/set_midi_program_data', 'iiiis')
    def set_midi_program_data_callback(self, path, args):
        pluginId, index, bank, program, name = args
        self.fParent.emit(SIGNAL("SetMidiProgramData(int, int, int, int, QString)"), pluginId, index, bank, program, name)

    @make_method('/carla-control/note_on', 'iiii')
    def note_on_callback(self, path, args):
        pluginId, channel, note, velo = args
        self.fParent.emit(SIGNAL("NoteOn(int, int, int, int)"), pluginId, channel, note, velo)

    @make_method('/carla-control/note_off', 'iii')
    def note_off_callback(self, path, args):
        pluginId, channel, note = args
        self.fParent.emit(SIGNAL("NoteOff(int, int, int)"), pluginId, channel, note)

    @make_method('/carla-control/set_peaks', 'iffff')
    def set_output_peak_value_callback(self, path, args):
        pluginId, in1, in2, out1, out2 = args
        self.fParent.emit(SIGNAL("SetPeaks(int, double, double, double, double)"), pluginId, in1, in2, out1, out2)

    @make_method('/carla-control/exit', '')
    def exit_callback(self, path, args):
        self.fParent.emit(SIGNAL("Exit()"))

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServer::fallback(\"%s\") - unknown message, args =" % path, args)

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CarlaControlW(QMainWindow):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.ui = ui_carla_control.Ui_CarlaControlW()
        self.ui.setupUi(self)

        if MACOS:
            self.setUnifiedTitleAndToolBarOnMac(True)

        # -------------------------------------------------------------
        # Load Settings

        self.loadSettings()

        # -------------------------------------------------------------
        # Internal stuff

        self.fProjectFilename = None
        self.fProjectLoading  = False

        self.fPluginCount = 0
        self.fPluginList  = []

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLastPluginName = ""

        self.lo_address = ""
        self.lo_server  = None

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.ui.act_file_refresh.setEnabled(False)
        #self.ui.act_plugin_remove_all.setEnabled(False)

        self.resize(self.width(), 0)

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.ui.act_file_connect, SIGNAL("triggered()"), SLOT("slot_fileConnect()"))
        self.connect(self.ui.act_file_refresh, SIGNAL("triggered()"), SLOT("slot_fileRefresh()"))

        #self.connect(self.ui.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        #self.connect(self.ui.act_plugin_add2, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        #self.connect(self.ui.act_plugin_refresh, SIGNAL("triggered()"), SLOT("slot_pluginRefresh()"))
        #self.connect(self.ui.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_pluginRemoveAll()"))

        #self.connect(self.ui.act_settings_show_toolbar, SIGNAL("triggered(bool)"), SLOT("slot_toolbarShown()"))
        #self.connect(self.ui.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))

        self.connect(self.ui.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarlaControl()"))
        self.connect(self.ui.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        self.connect(self, SIGNAL("SIGTERM()"), SLOT("slot_handleSIGTERM()"))

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
        self.connect(self, SIGNAL("NoteOn(int, int, int, int)"), SLOT("slot_handleNoteOn(int, int, int, int)"))
        self.connect(self, SIGNAL("NoteOff(int, int, int)"), SLOT("slot_handleNoteOff(int, int, int)"))
        self.connect(self, SIGNAL("SetPeaks(int, double, double, double, double)"), SLOT("slot_handleSetPeaks(int, double, double, double, double)"))
        self.connect(self, SIGNAL("Exit()"), SLOT("slot_handleExit()"))

    def removeAll(self):
        self.killTimer(self.fIdleTimerFast)
        self.killTimer(self.fIdleTimerSlow)
        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            pwidget.ui.edit_dialog.close()
            pwidget.close()
            pwidget.deleteLater()
            del pwidget

        self.fPluginCount = 0
        self.fPluginList  = []
        Carla.host.fPluginsInfo = []

        self.fIdleTimerFast = self.startTimer(60)
        self.fIdleTimerSlow = self.startTimer(60*2)

    @pyqtSlot()
    def slot_fileConnect(self):
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
            self.ui.act_file_refresh.setEnabled(True)
            lo_send(lo_target, "/register", self.lo_server.getFullURL())

    @pyqtSlot()
    def slot_fileRefresh(self):
        global lo_target
        if lo_target and self.lo_server:
            self.removeAll()
            lo_send(lo_target, "/unregister")
            lo_send(lo_target, "/register", self.lo_server.getFullURL())

    @pyqtSlot()
    def slot_aboutCarlaControl(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot(int, str)
    def slot_handleAddPluginStart(self, pluginId, pluginName):
        self.fLastPluginName = pluginName
        Carla.host._add(pluginId)

    @pyqtSlot(int)
    def slot_handleAddPluginEnd(self, pluginId):
        pwidget = PluginWidget(self, pluginId)
        pwidget.setRefreshRate(60)

        self.ui.w_plugins.layout().addWidget(pwidget)

        self.fPluginCount += 1
        self.fPluginList.append(pwidget)

    @pyqtSlot(int)
    def slot_handleRemovePlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            print("handleRemovePlugin(%i) - invalid plugin id" % pluginId)
            return

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            print("handleRemovePlugin(%i) - invalid plugin" % pluginId)
            return

        self.fPluginList.pop(pluginId)
        self.fPluginCount -= 1

        self.ui.w_plugins.layout().removeWidget(pwidget)

        pwidget.ui.edit_dialog.close()
        pwidget.close()
        pwidget.deleteLater()
        del pwidget

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            self.fPluginList[i].setId(i)

        Carla.host.fPluginsInfo.pop(pluginId)

    @pyqtSlot(int, int, int, int, str, str, str, str, int)
    def slot_handleSetPluginData(self, pluginId, type_, category, hints, realName, label, maker, copyright, uniqueId):
        info = deepcopy(CarlaPluginInfo)
        info['type']      = type_
        info['category']  = category
        info['hints']     = hints
        info['name']      = self.fLastPluginName
        info['label']     = label
        info['maker']     = maker
        info['copyright'] = copyright
        info['uniqueId']  = uniqueId
        Carla.host._set_pluginInfo(pluginId, info)
        Carla.host._set_pluginRealName(pluginId, realName)

    @pyqtSlot(int, int, int, int, int, int, int, int)
    def slot_handleSetPluginPorts(self, pluginId, audioIns, audioOuts, midiIns, midiOuts, cIns, cOuts, cTotals):
        audioInfo = deepcopy(CarlaPortCountInfo)
        midiInfo  = deepcopy(CarlaPortCountInfo)
        paramInfo = deepcopy(CarlaPortCountInfo)

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

        info = deepcopy(CarlaParameterInfo)
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

    @pyqtSlot(int, int, float)
    def slot_handleSetParameterValue(self, pluginId, parameterId, value):
        if parameterId >= 0:
            Carla.host._set_parameterValueS(pluginId, parameterId, value)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterValue(parameterId, value)

    @pyqtSlot(int, int, float)
    def slot_handleSetDefaultValue(self, pluginId, parameterId, value):
        Carla.host._set_parameterDefaultValue(pluginId, parameterId, value)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterDefault(parameterId, value)

    @pyqtSlot(int, int, int)
    def slot_handleSetParameterMidiCC(self, pluginId, index, cc):
        Carla.host._set_parameterMidiCC(pluginId, index, cc)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiControl(index, cc)

    @pyqtSlot(int, int, int)
    def slot_handleSetParameterMidiChannel(self, pluginId, index, channel):
        Carla.host._set_parameterMidiChannel(pluginId, index, channel)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiChannel(index, channel)

    @pyqtSlot(int, int)
    def slot_handleSetProgram(self, pluginId, index):
        Carla.host._set_currentProgram(pluginId, index)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setProgram(index)

    @pyqtSlot(int, int)
    def slot_handleSetProgramCount(self, pluginId, count):
        Carla.host._set_programCount(pluginId, count)

    @pyqtSlot(int, int, str)
    def slot_handleSetProgramName(self, pluginId, index, name):
        Carla.host._set_programNameS(pluginId, index, name)

    @pyqtSlot(int, int)
    def slot_handleSetMidiProgram(self, pluginId, index):
        Carla.host._set_currentMidiProgram(pluginId, index)

        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setMidiProgram(index)

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
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.sendNoteOn(note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOff(self, pluginId, channel, note):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.sendNoteOff(note)

    @pyqtSlot(int, float, float, float, float)
    def slot_handleSetPeaks(self, pluginId, in1, in2, out1, out2):
        Carla.host._set_peaks(pluginId, in1, in2, out1, out2)

    @pyqtSlot()
    def slot_handleExit(self):
        self.removeAll()

        if self.lo_server:
            self.lo_server.stop()
            self.lo_server = None
            self.ui.act_file_refresh.setEnabled(False)

        global lo_target, lo_targetName
        lo_target     = None
        lo_targetName = ""
        self.lo_address = ""

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("Geometry", self.saveGeometry())
        #settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())

    def loadSettings(self):
        settings = QSettings()
        self.restoreGeometry(settings.value("Geometry", ""))

        #showToolbar = settings.value("ShowToolbar", True, type=bool)
        #self.ui.act_settings_show_toolbar.setChecked(showToolbar)
        #self.ui.toolBar.setVisible(showToolbar)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleSlow()

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

    libPrefix = None

    for i in range(len(app.arguments())):
        if i == 0: continue
        argument = app.arguments()[i]

        if argument.startswith("--with-libprefix="):
            libPrefix = argument.replace("--with-libprefix=", "")

    if libPrefix is not None:
        libName = os.path.join(libPrefix, "lib", "carla", carla_libname)
    else:
        libName = carla_library_path

    # Load backend DLL, so it registers the theme
    libDLL = QLibrary()
    libDLL.setFileName(libName)

    try:
        libDLL.load()
    except:
        pass

    # Init backend (OSC bridge version)
    Carla.host = Host()

    # Create GUI
    Carla.gui = CarlaControlW()

    # Set-up custom signal handling
    setUpSignals()

    # Show GUI
    Carla.gui.show()

    # App-Loop
    ret = app.exec_()

    # Close backend DLL
    if libDLL.isLoaded():
        # Need to destroy app before theme
        del app
        libDLL.unload()

    sys.exit(ret)
