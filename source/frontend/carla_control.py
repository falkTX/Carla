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

from liblo import (
  Address,
  AddressError,
  ServerError,
  Server,
  make_method,
  send as lo_send,
  TCP as LO_TCP,
  UDP as LO_UDP,
)

from random import random

# ------------------------------------------------------------------------------------------------------------
# Host OSC object

class CarlaHostOSC(CarlaHostQtPlugin):
    def __init__(self):
        CarlaHostQtPlugin.__init__(self)

        self.lo_server_tcp = None
        self.lo_server_udp = None
        self.lo_target_tcp = None
        self.lo_target_udp = None
        self.lo_target_tcp_name = ""
        self.lo_target_udp_name = ""

    # -------------------------------------------------------------------

    def printAndReturnError(self, error):
        print(error)
        self.fLastError = error
        return False

    def sendMsg(self, lines):
        if len(lines) < 1:
            return self.printAndReturnError("not enough arguments")

        method = lines.pop(0)

        if method == "set_engine_option":
            print(method, lines)
            return True

        if self.lo_target_tcp is None:
            return self.printAndReturnError("lo_target_tcp is None")
        if self.lo_target_tcp_name is None:
            return self.printAndReturnError("lo_target_tcp_name is None")

        if method in ("clear_engine_xruns",
                      "cancel_engine_action",
                      #"load_file",
                      #"load_project",
                      #"save_project",
                      #"clear_project_filename",
                      "patchbay_connect",
                      "patchbay_disconnect",
                      "patchbay_refresh",
                      "transport_play",
                      "transport_pause",
                      "transport_bpm",
                      "transport_relocate",
                      #"add_plugin",
                      "remove_plugin",
                      "remove_all_plugins",
                      "rename_plugin",
                      "clone_plugin",
                      #"replace_plugin",
                      "switch_plugins",
                      #"load_plugin_state",
                      #"save_plugin_state",
                      ):
            path = "/ctrl/" + method

        elif method in (#"set_option",
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
            pluginId = lines.pop(0)
            path = "/%s/%i/%s" % (self.lo_target_tcp_name, pluginId, method)

        else:
            return self.printAndReturnError("invalid method '%s'" % method)

        args = [int(line) if isinstance(line, bool) else line for line in lines]
        #print(path, args)

        lo_send(self.lo_target_tcp, path, *args)
        return True

    # -------------------------------------------------------------------

    def engine_init(self, driverName, clientName):
        print("engine_init", self.lo_target_tcp is not None)
        return self.lo_target_tcp is not None

    def engine_close(self):
        print("engine_close")
        return True

    def engine_idle(self):
        return

    def is_engine_running(self):
        return self.lo_target_tcp is not None

    def set_engine_about_to_close(self):
        return

# ---------------------------------------------------------------------------------------------------------------------
# OSC Control server

class CarlaControlServerTCP(Server):
    def __init__(self, host):
        Server.__init__(self, proto=LO_TCP)

        self.host = host

    def idle(self):
        self.fReceivedMsgs = False

        while self.recv(0) and self.fReceivedMsgs:
            pass

    def getFullURL(self):
        return "%sctrl" % self.get_url()

    @make_method('/ctrl/cb', 'iiiiifs')
    def carla_cb(self, path, args):
        self.fReceivedMsgs = True
        action, pluginId, value1, value2, value3, valuef, valueStr = args
        self.host._setViaCallback(action, pluginId, value1, value2, value3, valuef, valueStr)
        engineCallback(self.host, action, pluginId, value1, value2, value3, valuef, valueStr)

    @make_method('/ctrl/init', 'is') # FIXME set name in add method
    def carla_init(self, path, args):
        self.fReceivedMsgs = True
        pluginId, pluginName = args
        self.host._add(pluginId)
        self.host._set_pluginInfoUpdate(pluginId, {'name': pluginName})

    @make_method('/ctrl/ports', 'iiiiiiii')
    def carla_ports(self, path, args):
        self.fReceivedMsgs = True
        pluginId, audioIns, audioOuts, midiIns, midiOuts, paramIns, paramOuts, paramTotal = args
        self.host._set_audioCountInfo(pluginId, {'ins': audioIns, 'outs': audioOuts})
        self.host._set_midiCountInfo(pluginId, {'ins': midiOuts, 'outs': midiOuts})
        self.host._set_parameterCountInfo(pluginId, paramTotal, {'ins': paramIns, 'outs': paramOuts})

    @make_method('/ctrl/info', 'iiiihssss')
    def carla_info(self, path, args):
        self.fReceivedMsgs = True
        pluginId, type_, category, hints, uniqueId, realName, label, maker, copyright = args
        optsAvail = optsEnabled = 0x0 # FIXME
        filename = name = iconName = "" # FIXME

        hints &= ~PLUGIN_HAS_CUSTOM_UI

        pinfo = {
            'type': type_,
            'category': category,
            'hints': hints,
            'optionsAvailable': optsAvail,
            'optionsEnabled': optsEnabled,
            'uniqueId': uniqueId,
            'filename': filename,
            #'name':  name, # FIXME
            'label': label,
            'maker': maker,
            'copyright': copyright,
            'iconName': iconName
        }

        self.host._set_pluginInfoUpdate(pluginId, pinfo)
        self.host._set_pluginRealName(pluginId, realName)

    @make_method('/ctrl/param', 'iiiiiissfffffff')
    def carla_param(self, path, args):
        self.fReceivedMsgs = True
        (
          pluginId, paramId, type_, hints, midiChan, midiCC, name, unit,
          def_, min_, max_, step, stepSmall, stepLarge, value
        ) = args

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
            'midiCC': midiCC,
            'midiChannel': midiChan,
        }
        self.host._set_parameterData(pluginId, paramId, paramData)

        paramRanges = {
            'def': def_,
            'min': min_,
            'max': max_,
            'step': step,
            'stepSmall': stepSmall,
            'stepLarge': stepLarge,
        }
        self.host._set_parameterRangesUpdate(pluginId, paramId, paramRanges)

        self.host._set_parameterValue(pluginId, paramId, value)

    @make_method('/ctrl/iparams', 'ifffffff')
    def carla_iparams(self, path, args):
        self.fReceivedMsgs = True
        pluginId, active, drywet, volume, balLeft, balRight, pan, ctrlChan = args
        self.host._set_internalValue(pluginId, PARAMETER_ACTIVE, active)
        self.host._set_internalValue(pluginId, PARAMETER_DRYWET, drywet)
        self.host._set_internalValue(pluginId, PARAMETER_VOLUME, volume)
        self.host._set_internalValue(pluginId, PARAMETER_BALANCE_LEFT, balLeft)
        self.host._set_internalValue(pluginId, PARAMETER_BALANCE_RIGHT, balRight)
        self.host._set_internalValue(pluginId, PARAMETER_PANNING, pan)
        self.host._set_internalValue(pluginId, PARAMETER_CTRL_CHANNEL, ctrlChan)

    #@make_method('/ctrl/set_program_count', 'ii')
    #def set_program_count_callback(self, path, args):
        #print(path, args)
        #self.fReceivedMsgs = True
        #pluginId, count = args
        #self.host._set_programCount(pluginId, count)

    #@make_method('/ctrl/set_midi_program_count', 'ii')
    #def set_midi_program_count_callback(self, path, args):
        #print(path, args)
        #self.fReceivedMsgs = True
        #pluginId, count = args
        #self.host._set_midiProgramCount(pluginId, count)

    #@make_method('/ctrl/set_program_name', 'iis')
    #def set_program_name_callback(self, path, args):
        #print(path, args)
        #self.fReceivedMsgs = True
        #pluginId, progId, progName = args
        #self.host._set_programName(pluginId, progId, progName)

    #@make_method('/ctrl/set_midi_program_data', 'iiiis')
    #def set_midi_program_data_callback(self, path, args):
        #print(path, args)
        #self.fReceivedMsgs = True
        #pluginId, midiProgId, bank, program, name = args
        #self.host._set_midiProgramData(pluginId, midiProgId, {'bank': bank, 'program': program, 'name': name})

    #@make_method('/note_on', 'iiii')

    @make_method('/ctrl/exit', '')
    def carla_exit(self, path, args):
        self.fReceivedMsgs = True
        #self.host.lo_target_tcp = None
        self.host.QuitCallback.emit()

    @make_method('/ctrl/exit-error', 's')
    def carla_exit_error(self, path, args):
        self.fReceivedMsgs = True
        error, = args
        self.host.lo_target_tcp = None
        self.host.QuitCallback.emit()
        self.host.ErrorCallback.emit(error)

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServerTCP::fallback(\"%s\") - unknown message, args =" % path, args)
        self.fReceivedMsgs = True

# ---------------------------------------------------------------------------------------------------------------------

class CarlaControlServerUDP(Server):
    def __init__(self, host):
        Server.__init__(self, proto=LO_UDP)

        self.host = host

    def idle(self):
        self.fReceivedMsgs = False

        while self.recv(0) and self.fReceivedMsgs:
            pass

    def getFullURL(self):
        return "%sctrl" % self.get_url()

    @make_method('/ctrl/runtime', 'fiihiiif')
    def carla_runtime(self, path, args):
        self.fReceivedMsgs = True
        load, xruns, playing, frame, bar, beat, tick, bpm = args
        self.host._set_runtime_info(load, xruns)
        self.host._set_transport(bool(playing), frame, bar, beat, tick, bpm)

    @make_method('/ctrl/param', 'iif')
    def carla_param_fixme(self, path, args):
        self.fReceivedMsgs = True
        pluginId, paramId, paramValue = args
        self.host._set_parameterValue(pluginId, paramId, paramValue)

    @make_method('/ctrl/peaks', 'iffff')
    def carla_peaks(self, path, args):
        self.fReceivedMsgs = True
        pluginId, in1, in2, out1, out2 = args
        self.host._set_peaks(pluginId, in1, in2, out1, out2)

    @make_method(None, None)
    def fallback(self, path, args):
        print("ControlServerUDP::fallback(\"%s\") - unknown message, args =" % path, args)
        self.fReceivedMsgs = True

# ---------------------------------------------------------------------------------------------------------------------
# Main Window

class HostWindowOSC(HostWindow):
    def __init__(self, host, oscAddr):
        HostWindow.__init__(self, host, True)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostPlugin()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_connect.triggered.connect(self.slot_fileConnect)
        self.ui.act_file_refresh.triggered.connect(self.slot_fileRefresh)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        if oscAddr:
            QTimer.singleShot(0, self.connectOsc)

    def connectOsc(self, addrTCP = None, addrUDP = None):
        if addrTCP is not None:
            self.fOscAddressTCP = addrTCP
        if addrUDP is not None:
            self.fOscAddressUDP = addrUDP

        lo_target_tcp_name = self.fOscAddressTCP.rsplit("/", 1)[-1]
        lo_target_udp_name = self.fOscAddressUDP.rsplit("/", 1)[-1]

        err = None

        try:
            lo_target_tcp = Address(self.fOscAddressTCP)
            lo_server_tcp = CarlaControlServerTCP(self.host)
            lo_send(lo_target_tcp, "/register", lo_server_tcp.getFullURL())
            print(lo_server_tcp.getFullURL())

            lo_target_udp = Address(self.fOscAddressUDP)
            lo_server_udp = CarlaControlServerUDP(self.host)
            lo_send(lo_target_udp, "/register", lo_server_udp.getFullURL())
            print(lo_server_udp.getFullURL())

        except AddressError as e:
            err = e
        except OSError as e:
            err = e
        except:
            err = Exception()

        if err is not None:
            fullError = self.tr("Failed to connect to the Carla instance.")

            if len(err.args) > 0:
                fullError += " %s\n%s\n" % (self.tr("Error was:"), err.args[0])

            fullError += "\n"
            fullError += self.tr("Make sure the remote Carla is running and the URL and Port are correct.") + "\n"
            fullError += self.tr("If it still does not work, check your current device and the remote's firewall.")

            CustomMessageBox(self,
                             QMessageBox.Warning,
                             self.tr("Error"),
                             self.tr("Connection failed"),
                             fullError,
                             QMessageBox.Ok,
                             QMessageBox.Ok)
            return

        self.host.lo_server_tcp = lo_server_tcp
        self.host.lo_target_tcp = lo_target_tcp
        self.host.lo_target_tcp_name = lo_target_tcp_name

        self.host.lo_server_udp = lo_server_udp
        self.host.lo_target_udp = lo_target_udp
        self.host.lo_target_udp_name = lo_target_udp_name

        self.ui.act_file_refresh.setEnabled(True)

        self.startTimers()

    def disconnectOsc(self):
        self.killTimers()
        self.unregister()
        self.removeAllPlugins()
        patchcanvas.clear()

        self.ui.act_file_refresh.setEnabled(False)

    # --------------------------------------------------------------------------------------------------------

    def unregister(self):
        if self.host.lo_server_tcp is not None:
            if self.host.lo_target_tcp is not None:
                try:
                    lo_send(self.host.lo_target_tcp, "/unregister", self.host.lo_server_tcp.getFullURL())
                except:
                    pass
                self.host.lo_target_tcp = None

            while self.host.lo_server_tcp.recv(0):
                pass
            #self.host.lo_server_tcp.free()
            self.host.lo_server_tcp = None

        if self.host.lo_server_udp is not None:
            if self.host.lo_target_udp is not None:
                try:
                    lo_send(self.host.lo_target_udp, "/unregister", self.host.lo_server_udp.getFullURL())
                except:
                    pass
                self.host.lo_target_udp = None

            while self.host.lo_server_udp.recv(0):
                pass
            #self.host.lo_server_udp.free()
            self.host.lo_server_udp = None

        self.host.lo_target_tcp_name = ""
        self.host.lo_target_udp_name = ""

    # --------------------------------------------------------------------------------------------------------
    # Timers

    def idleFast(self):
        HostWindow.idleFast(self)

        if self.host.lo_server_tcp is not None:
            self.host.lo_server_tcp.idle()
        else:
            self.disconnectOsc()

        if self.host.lo_server_udp is not None:
            self.host.lo_server_udp.idle()
        else:
            self.disconnectOsc()

    # --------------------------------------------------------------------------------------------------------

    def removeAllPlugins(self):
        self.host.fPluginsInfo = []
        HostWindow.removeAllPlugins(self)

    # --------------------------------------------------------------------------------------------------------

    def loadSettings(self, firstTime):
        settings = HostWindow.loadSettings(self, firstTime)
        self.fOscAddressTCP = settings.value("RemoteAddressTCP", "osc.tcp://127.0.0.1:22752/Carla", type=str)
        self.fOscAddressUDP = settings.value("RemoteAddressUDP", "osc.udp://127.0.0.1:22752/Carla", type=str)

    def saveSettings(self):
        settings = HostWindow.saveSettings(self)
        if self.fOscAddressTCP:
            settings.setValue("RemoteAddressTCP", self.fOscAddressTCP)
        if self.fOscAddressUDP:
            settings.setValue("RemoteAddressUDP", self.fOscAddressUDP)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_fileConnect(self):
        dialog = QInputDialog(self)
        dialog.setInputMode(QInputDialog.TextInput)
        dialog.setLabelText(self.tr("Address:"))
        dialog.setTextValue(self.fOscAddressTCP or "osc.tcp://127.0.0.1:22752/Carla")
        dialog.setWindowTitle(self.tr("Carla Control - Connect"))
        dialog.resize(400,1)

        ok = dialog.exec_()
        addr = dialog.textValue().strip()
        del dialog

        if not ok:
            return
        if not addr:
            return

        self.disconnectOsc()

        if addr:
            self.connectOsc(addr.replace("osc.udp:", "osc.tcp:"), addr.replace("osc.tcp:", "osc.udp:"))

    @pyqtSlot()
    def slot_fileRefresh(self):
        if None in (self.host.lo_server_tcp, self.host.lo_server_udp, self.host.lo_target_tcp, self.host.lo_target_udp):
            return

        lo_send(self.host.lo_target_udp, "/unregister", self.host.lo_server_udp.getFullURL())
        while self.host.lo_server_udp.recv(0):
            pass
        #self.host.lo_server_udp.free()

        lo_send(self.host.lo_target_tcp, "/unregister", self.host.lo_server_tcp.getFullURL())
        while self.host.lo_server_tcp.recv(0):
            pass
        #self.host.lo_server_tcp.free()

        self.removeAllPlugins()
        patchcanvas.clear()

        self.host.lo_server_tcp = CarlaControlServerTCP(self.host)
        self.host.lo_server_udp = CarlaControlServerUDP(self.host)

        try:
            lo_send(self.host.lo_target_tcp, "/register", self.host.lo_server_tcp.getFullURL())
        except:
            self.disconnectOsc()
            return

        try:
            lo_send(self.host.lo_target_udp, "/register", self.host.lo_server_udp.getFullURL())
        except:
            self.disconnectOsc()
            return

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        self.disconnectOsc()
        HostWindow.slot_handleQuitCallback(self)

    # --------------------------------------------------------------------------------------------------------

    def closeEvent(self, event):
        self.killTimers()
        self.unregister()

        HostWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
