#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code (OSC stuff)
# Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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
# Imports (Custom)

from carla_host import *

# ------------------------------------------------------------------------------------------------------------
# Imports (liblo)

from liblo import make_method, Address, ServerError, ServerThread
from liblo import send as lo_send
from liblo import TCP as LO_TCP
from liblo import UDP as LO_UDP

from random import random

# ------------------------------------------------------------------------------------------------------------
# Global liblo objects

global lo_target, lo_target_name
lo_target      = None
lo_target_name = ""

# ------------------------------------------------------------------------------------------------------------
# Host OSC object

class CarlaHostOSC(CarlaHostQtPlugin):
    def __init__(self):
        CarlaHostQtPlugin.__init__(self)

    # -------------------------------------------------------------------

    def printAndReturnError(self, error):
        print(error)
        self.fLastError = error
        return False

    def sendMsg(self, lines):
        global lo_target, lo_target_name

        if lo_target is None:
            return self.printAndReturnError("lo_target is None")
        if lo_target_name is None:
            return self.printAndReturnError("lo_target_name is None")
        if len(lines) < 2:
            return self.printAndReturnError("not enough arguments")

        method = lines.pop(0)

        if method not in (
                          #"set_option",
                          "set_active",
                          "set_drywet",
                          "set_volume",
                          "set_balance_left",
                          "set_balance_right",
                          "set_panning",
                          #"set_ctrl_channel",
                          "set_parameter_value",
                          "set_parameter_midi_channel",
                          "set_parameter_midi_cc",
                          "set_program",
                          "set_midi_program",
                          #"set_custom_data",
                          #"set_chunk_data",
                          #"prepare_for_save",
                          #"reset_parameters",
                          #"randomize_parameters",
                          "send_midi_note"
                          ):
            return self.printAndReturnError("invalid method '%s'" % method)

        pluginId = lines.pop(0)

        args = []

        if method == "send_midi_note":
            channel, note, velocity = lines

            if velocity:
                method = "note_on"
                args   = [channel, note, velocity]
            else:
                method = "note_off"
                args   = [channel, note]

        else:
            for line in lines:
                if isinstance(line, bool):
                    args.append(int(line))
                else:
                    args.append(line)

        path = "/%s/%i/%s" % (lo_target_name, pluginId, method)

        print(path, args)

        lo_send(lo_target, path, *args)
        return True

    # -------------------------------------------------------------------

    def engine_init(self, driverName, clientName):
        global lo_target
        return lo_target is not None

    def engine_close(self):
        return True

    def engine_idle(self):
        return

    def is_engine_running(self):
        global lo_target
        return lo_target is not None

    def set_engine_about_to_close(self):
        return

# ------------------------------------------------------------------------------------------------------------
# OSC Control server

class CarlaControlServerThread(ServerThread):
    def __init__(self, host, mode):
        ServerThread.__init__(self, 8998 + int(random()*9000), mode)

        self.host = host

    def getFullURL(self):
        return "%scarla-control" % self.get_url()

    @make_method('/carla-control/add_plugin_start', 'is') # FIXME skip name
    def add_plugin_start_callback(self, path, args):
        print(path, args)
        pluginId, pluginName = args
        self.host._add(pluginId)
        self.host._set_pluginInfoUpdate(pluginId, {'name': pluginName})

    @make_method('/carla-control/add_plugin_end', 'i') # FIXME skip name
    def add_plugin_end_callback(self, path, args):
        print(path, args)
        pluginId, = args
        self.host.PluginAddedCallback.emit(pluginId, "") #self.fPluginsInfo[pluginId].pluginInfo['name'])

    @make_method('/carla-control/remove_plugin', 'i')
    def remove_plugin_callback(self, path, args):
        print(path, args)
        pluginId, = args
        self.host.PluginRemovedCallback.emit(pluginId)

    @make_method('/carla-control/set_plugin_info1', 'iiiih')
    def set_plugin_info1_callback(self, path, args):
        print(path, args)
        pluginId, type_, category, hints, uniqueId = args # , optsAvail, optsEnabled
        optsAvail = optsEnabled = 0x0 # FIXME

        hints &= ~PLUGIN_HAS_CUSTOM_UI

        pinfo = {
            'type': type_,
            'category': category,
            'hints': hints,
            'optionsAvailable': optsAvail,
            'optionsEnabled': optsEnabled,
            'uniqueId': uniqueId
        }

        self.host._set_pluginInfoUpdate(pluginId, pinfo)

    @make_method('/carla-control/set_plugin_info2', 'issss')
    def set_plugin_info2_callback(self, path, args):
        print(path, args)
        pluginId, realName, label, maker, copyright = args # , filename, name, iconName
        filename = name = iconName = "" # FIXME

        pinfo = {
            'filename': filename,
            #'name':  name, # FIXME
            'label': label,
            'maker': maker,
            'copyright': copyright,
            'iconName': iconName
        }

        self.host._set_pluginInfoUpdate(pluginId, pinfo)
        self.host._set_pluginRealName(pluginId, realName)

    @make_method('/carla-control/set_audio_count', 'iii')
    def set_audio_count_callback(self, path, args):
        print(path, args)
        pluginId, ins, outs = args
        self.host._set_audioCountInfo(pluginId, {'ins': ins, 'outs': outs})

    @make_method('/carla-control/set_midi_count', 'iii')
    def set_midi_count_callback(self, path, args):
        print(path, args)
        pluginId, ins, outs = args
        self.host._set_midiCountInfo(pluginId, {'ins': ins, 'outs': outs})

    @make_method('/carla-control/set_parameter_count', 'iii') # FIXME
    def set_parameter_count_callback(self, path, args):
        print(path, args)
        pluginId, ins, outs = args # , count
        count = ins + outs
        self.host._set_parameterCountInfo(pluginId, count, {'ins': ins, 'outs': outs})

    @make_method('/carla-control/set_program_count', 'ii')
    def set_program_count_callback(self, path, args):
        print(path, args)
        pluginId, count = args
        self.host._set_programCount(pluginId, count)

    @make_method('/carla-control/set_midi_program_count', 'ii')
    def set_midi_program_count_callback(self, path, args):
        print(path, args)
        pluginId, count = args
        self.host._set_midiProgramCount(pluginId, count)

    @make_method('/carla-control/set_parameter_data', 'iiiiss')
    def set_parameter_data_callback(self, path, args):
        print(path, args)
        pluginId, paramId, type_, hints, name, unit = args

        hints &= ~(PARAMETER_USES_SCALEPOINTS | PARAMETER_USES_CUSTOM_TEXT)

        paramInfo = {
            'name': name,
            'symbol': "",
            'unit': unit,
            'scalePointCount': 0,
        }
        self.host._set_parameterInfo(pluginId, paramId, paramInfo)

        paramData = {
            'type': type_,
            'hints': hints,
            'index': paramId,
            'rindex': -1,
            'midiCC': -1,
            'midiChannel': 0
        }
        self.host._set_parameterData(pluginId, paramId, paramData)

    @make_method('/carla-control/set_parameter_ranges1', 'iifff')
    def set_parameter_ranges1_callback(self, path, args):
        print(path, args)
        pluginId, paramId, def_, min_, max_ = args

        paramRanges = {
            'def': def_,
            'min': min_,
            'max': max_
        }

        self.host._set_parameterRangesUpdate(pluginId, paramId, paramRanges)

    @make_method('/carla-control/set_parameter_ranges2', 'iifff')
    def set_parameter_ranges2_callback(self, path, args):
        print(path, args)
        pluginId, paramId, step, stepSmall, stepLarge = args

        paramRanges = {
            'step': step,
            'stepSmall': stepSmall,
            'stepLarge': stepLarge
        }

        self.host._set_parameterRangesUpdate(pluginId, paramId, paramRanges)

    @make_method('/carla-control/set_parameter_midi_cc', 'iii')
    def set_parameter_midi_cc_callback(self, path, args):
        print(path, args)
        pluginId, paramId, cc = args
        self.host._set_parameterMidiCC(pluginId, paramId, cc)
        self.host.ParameterMidiCcChangedCallback.emit(pluginId, paramId, cc)

    @make_method('/carla-control/set_parameter_midi_channel', 'iii')
    def set_parameter_midi_channel_callback(self, path, args):
        print(path, args)
        pluginId, paramId, channel = args
        self.host._set_parameterMidiChannel(pluginId, paramId, channel)
        self.host.ParameterMidiChannelChangedCallback.emit(pluginId, paramId, channel)

    @make_method('/carla-control/set_parameter_value', 'iif')
    def set_parameter_value_callback(self, path, args):
        pluginId, paramId, paramValue = args

        if paramId < 0:
            self.host._set_internalValue(pluginId, paramId, paramValue)
        else:
            self.host._set_parameterValue(pluginId, paramId, paramValue)

        self.host.ParameterValueChangedCallback.emit(pluginId, paramId, paramValue)

    @make_method('/carla-control/set_default_value', 'iif')
    def set_default_value_callback(self, path, args):
        print(path, args)
        pluginId, paramId, paramValue = args
        self.host._set_parameterDefault(pluginId, paramId, paramValue)
        self.host.ParameterDefaultChangedCallback.emit(pluginId, paramId, paramValue)

    @make_method('/carla-control/set_current_program', 'ii')
    def set_current_program_callback(self, path, args):
        print(path, args)
        pluginId, current = args
        self.host._set_currentProgram(pluginId, current)
        self.host.ProgramChangedCallback.emit(current)

    @make_method('/carla-control/set_current_midi_program', 'ii')
    def set_current_midi_program_callback(self, path, args):
        print(path, args)
        pluginId, current = args
        self.host._set_currentMidiProgram(pluginId, current)
        #self.host.MidiProgramChangedCallback.emit() # FIXME

    @make_method('/carla-control/set_program_name', 'iis')
    def set_program_name_callback(self, path, args):
        print(path, args)
        pluginId, progId, progName = args
        self.host._set_programName(pluginId, progId, progName)

    @make_method('/carla-control/set_midi_program_data', 'iiiis')
    def set_midi_program_data_callback(self, path, args):
        print(path, args)
        pluginId, midiProgId, bank, program, name = args
        self.host._set_midiProgramData(pluginId, midiProgId, {'bank': bank, 'program': program, 'name': name})

    @make_method('/carla-control/note_on', 'iiii')
    def set_note_on_callback(self, path, args):
        print(path, args)
        pluginId, channel, note, velocity = args
        self.host.NoteOnCallback.emit(pluginId, channel, note, velocity)

    @make_method('/carla-control/note_off', 'iii')
    def set_note_off_callback(self, path, args):
        print(path, args)
        pluginId, channel, note = args
        self.host.NoteOffCallback.emit(pluginId, channel, note)

    @make_method('/carla-control/set_peaks', 'iffff')
    def set_peaks_callback(self, path, args):
        pluginId, in1, in2, out1, out2 = args
        self.host._set_peaks(pluginId, in1, in2, out1, out2)

    @make_method('/carla-control/exit', '')
    def set_exit_callback(self, path, args):
        print(path, args)
        self.host.QuitCallback.emit()

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServer::fallback(\"%s\") - unknown message, args =" % path, args)

# ------------------------------------------------------------------------------------------------------------
# Main Window

class HostWindowOSC(HostWindow):
    def __init__(self, host, oscAddr):
        HostWindow.__init__(self, host, False)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostPlugin()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fOscAddress = oscAddr
        self.fOscServer  = None

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (not connected)

        self.ui.act_file_refresh.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_connect.triggered.connect(self.slot_fileConnect)
        self.ui.act_file_refresh.triggered.connect(self.slot_fileRefresh)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        if oscAddr:
            QTimer.singleShot(0, self.connectOsc)

    def connectOsc(self, addr = None):
        global lo_target, lo_target_name

        if addr is not None:
            self.fOscAddress = addr

        lo_target      = Address(self.fOscAddress)
        lo_target_name = self.fOscAddress.rsplit("/", 1)[-1]
        print("Connecting to \"%s\" as '%s'..." % (self.fOscAddress, lo_target_name))

        try:
            self.fOscServer = CarlaControlServerThread(self.host, LO_UDP if self.fOscAddress.startswith("osc.udp") else LO_TCP)
        except: # ServerError as err:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to connect, operation failed."))
            return

        self.fOscServer.start()
        lo_send(lo_target, "/register", self.fOscServer.getFullURL())

        self.startTimers()
        self.ui.act_file_refresh.setEnabled(True)

    def disconnectOsc(self):
        global lo_target, lo_target_name

        self.killTimers()
        self.ui.act_file_refresh.setEnabled(False)

        if lo_target is not None:
            lo_send(lo_target, "/unregister")

        if self.fOscServer is not None:
            self.fOscServer.stop()
            self.fOscServer = None

        self.removeAllPlugins()

        lo_target        = None
        lo_target_name   = ""
        self.fOscAddress = ""

    def removeAllPlugins(self):
        self.host.fPluginsInfo = []
        HostWindow.removeAllPlugins(self)

    @pyqtSlot()
    def slot_fileConnect(self):
        global lo_target, lo_target_name

        if lo_target and self.fOscServer:
            urlText = self.fOscAddress
        else:
            urlText = "osc.tcp://127.0.0.1:19000/Carla"

        addr, ok = QInputDialog.getText(self, self.tr("Carla Control - Connect"), self.tr("Address"), text=urlText)

        if not ok:
            return

        self.disconnectOsc()
        self.connectOsc(addr)

    @pyqtSlot()
    def slot_fileRefresh(self):
        global lo_target

        if lo_target is None or self.fOscServer is None:
            return

        self.killTimers()
        lo_send(lo_target, "/unregister")
        self.removeAllPlugins()
        lo_send(lo_target, "/register", self.fOscServer.getFullURL())
        self.startTimers()

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        self.disconnectOsc()

    def closeEvent(self, event):
        global lo_target

        self.killTimers()

        if lo_target is not None and self.fOscServer is not None:
            lo_send(lo_target, "/unregister")

        HostWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
