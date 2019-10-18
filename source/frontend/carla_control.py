#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code (OSC stuff)
# Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

# ----------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt5.QtCore import QEventLoop

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_osc_connect

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

DEBUG = False

# ----------------------------------------------------------------------------------------------------------------------
# OSC connect Dialog

class ConnectDialog(QDialog):
    def __init__(self, parent):
        QDialog.__init__(self, parent)
        self.ui = ui_carla_osc_connect.Ui_Dialog()
        self.ui.setupUi(self)

        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        # Make PlainTextEdit background the same color as the window background
        palette = self.ui.te_reported_hint.palette()
        palette.setColor(QPalette.Base, palette.color(QPalette.Background))
        self.ui.te_reported_hint.setPalette(palette)

        # -------------------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_saveSettings)
        self.ui.rb_reported_auto.clicked.connect(self.slot_reportedAutoClicked)
        self.ui.rb_reported_custom.clicked.connect(self.slot_reportedCustomClicked)
        self.ui.le_host.textChanged.connect(self.slot_hostChanged)
        self.ui.le_reported_host.textChanged.connect(self.slot_reportedHostChanged)

    # -----------------------------------------------------------------------------------------------------------------

    def getResult(self):
        return (self.ui.le_host.text(),
                self.ui.le_reported_host.text() if self.ui.rb_reported_custom.isChecked() else "",
                self.ui.sb_tcp_port.value(),
                self.ui.sb_udp_port.value())

    def checkIfButtonBoxShouldBeEnabled(self, host, reportedHostAutomatic, reportedHost):
        enabled = len(host) > 0

        if enabled and not reportedHostAutomatic:
            enabled = len(reportedHost) > 0

        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    def loadSettings(self):
        settings = QSettings("falkTX", "CarlaOSCConnect")

        if settings.value("ReportedHostAutomatic", True, type=bool):
            self.ui.rb_reported_custom.setChecked(False)
            self.ui.rb_reported_auto.setChecked(True)
        else:
            self.ui.rb_reported_auto.setChecked(False)
            self.ui.rb_reported_custom.setChecked(True)

        self.ui.le_host.setText(settings.value("Host", "127.0.0.1", type=str))
        self.ui.le_reported_host.setText(settings.value("ReportedHost", "", type=str))
        self.ui.sb_tcp_port.setValue(settings.value("TCPPort", CARLA_DEFAULT_OSC_TCP_PORT_NUMBER, type=int))
        self.ui.sb_udp_port.setValue(settings.value("UDPPort", CARLA_DEFAULT_OSC_UDP_PORT_NUMBER, type=int))

        self.checkIfButtonBoxShouldBeEnabled(self.ui.le_host.text(),
                                             self.ui.rb_reported_auto.isChecked(),
                                             self.ui.le_reported_host.text())

    # ------------------------------------------------------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_reportedAutoClicked(self, clicked):
        self.checkIfButtonBoxShouldBeEnabled(self.ui.le_host.text(),
                                             clicked,
                                             self.ui.le_reported_host.text())

    @pyqtSlot(bool)
    def slot_reportedCustomClicked(self, clicked):
        self.checkIfButtonBoxShouldBeEnabled(self.ui.le_host.text(),
                                             not clicked,
                                             self.ui.le_reported_host.text())

    @pyqtSlot(str)
    def slot_hostChanged(self, text):
        self.checkIfButtonBoxShouldBeEnabled(text,
                                             self.ui.rb_reported_auto.isChecked(),
                                             self.ui.le_reported_host.text())

    @pyqtSlot(str)
    def slot_reportedHostChanged(self, text):
        self.checkIfButtonBoxShouldBeEnabled(self.ui.le_host.text(),
                                             self.ui.rb_reported_auto.isChecked(),
                                             text)

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSettings("falkTX", "CarlaOSCConnect")
        settings.setValue("Host", self.ui.le_host.text())
        settings.setValue("ReportedHost", self.ui.le_reported_host.text())
        settings.setValue("TCPPort", self.ui.sb_tcp_port.value())
        settings.setValue("UDPPort", self.ui.sb_udp_port.value())
        settings.setValue("ReportedHostAutomatic", self.ui.rb_reported_auto.isChecked())

    # ------------------------------------------------------------------------------------------------------------------

    def done(self, r):
        QDialog.done(self, r)
        self.close()

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

        self.resetPendingMessages()

    # -------------------------------------------------------------------

    def resetPendingMessages(self):
        self.lastMessageId = 1
        self.pendingMessages = []
        self.responses = {}

    def printAndReturnError(self, error):
        print(error)
        self.fLastError = error
        return False

    def sendMsg(self, lines):
        if len(lines) < 1:
            return self.printAndReturnError("not enough arguments")

        method = lines.pop(0)

        if method == "set_engine_option":
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
                      "add_plugin",
                      "remove_plugin",
                      "remove_all_plugins",
                      "rename_plugin",
                      "clone_plugin",
                      "replace_plugin",
                      "switch_plugins",
                      #"load_plugin_state",
                      #"save_plugin_state",
                      ):
            path = "/ctrl/" + method
            needResp = True

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
                        ):
            pluginId = lines.pop(0)
            needResp = False
            path = "/%s/%i/%s" % (self.lo_target_tcp_name, pluginId, method)

        elif method == "send_midi_note":
            pluginId = lines.pop(0)
            needResp = False
            channel, note, velocity = lines

            if velocity:
                path = "/%s/%i/note_on" % (self.lo_target_tcp_name, pluginId)
            else:
                path = "/%s/%i/note_off" % (self.lo_target_tcp_name, pluginId)
                lines.pop(2)

        else:
            return self.printAndReturnError("invalid method '%s'" % method)

        if len(self.pendingMessages) != 0:
            return self.printAndReturnError("A previous operation is still pending, please wait")

        args = [int(line) if isinstance(line, bool) else line for line in lines]
        #print(path, args)

        if not needResp:
            lo_send(self.lo_target_tcp, path, *args)
            return True

        messageId = self.lastMessageId
        self.lastMessageId += 1
        self.pendingMessages.append(messageId)

        lo_send(self.lo_target_tcp, path, messageId, *args)

        while messageId in self.pendingMessages:
            QApplication.processEvents(QEventLoop.AllEvents, 100)

        error = self.responses.pop(messageId)

        if not error:
            return True

        self.fLastError = error
        return False

    def sendMsgAndSetError(self, lines):
        return self.sendMsg(lines)

    # -------------------------------------------------------------------

    def engine_init(self, driverName, clientName):
        return self.lo_target_tcp is not None

    def engine_close(self):
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
    def __init__(self, host, rhost):
        Server.__init__(self, proto=LO_TCP)

        if False:
            host = CarlaHostOSC()

        self.host = host
        self.rhost = rhost

    def idle(self):
        self.fReceivedMsgs = False

        while self.recv(0) and self.fReceivedMsgs:
            pass

    def getFullURL(self):
        if self.rhost:
            return "osc.tcp://%s:%i/ctrl" % (self.rhost, self.get_port())
        return "%sctrl" % self.get_url()

    @make_method('/ctrl/cb', 'iiiiifs')
    def carla_cb(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        action, pluginId, value1, value2, value3, valuef, valueStr = args
        self.host._setViaCallback(action, pluginId, value1, value2, value3, valuef, valueStr)
        engineCallback(self.host, action, pluginId, value1, value2, value3, valuef, valueStr)

    @make_method('/ctrl/info', 'iiiihiisssssss')
    def carla_info(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        (
          pluginId, type_, category, hints, uniqueId, optsAvail, optsEnabled,
          name, filename, iconName, realName, label, maker, copyright,
        ) = args

        hints &= ~PLUGIN_HAS_CUSTOM_UI

        pinfo = {
            'type': type_,
            'category': category,
            'hints': hints,
            'optionsAvailable': optsAvail,
            'optionsEnabled': optsEnabled,
            'uniqueId': uniqueId,
            'filename': filename,
            'name':  name,
            'label': label,
            'maker': maker,
            'copyright': copyright,
            'iconName': iconName
        }

        self.host._set_pluginInfoUpdate(pluginId, pinfo)
        self.host._set_pluginRealName(pluginId, realName)

    @make_method('/ctrl/ports', 'iiiiiiii')
    def carla_ports(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, audioIns, audioOuts, midiIns, midiOuts, paramIns, paramOuts, paramTotal = args
        self.host._set_audioCountInfo(pluginId, {'ins': audioIns, 'outs': audioOuts})
        self.host._set_midiCountInfo(pluginId, {'ins': midiOuts, 'outs': midiOuts})
        self.host._set_parameterCountInfo(pluginId, paramTotal, {'ins': paramIns, 'outs': paramOuts})

    @make_method('/ctrl/param', 'iiiiiissfffffff')
    def carla_param(self, path, args):
        if DEBUG: print(path, args)
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

    @make_method('/ctrl/count', 'iiiiii')
    def carla_count(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, pcount, mpcount, cdcount, cp, cmp = args
        self.host._set_programCount(pluginId, pcount)
        self.host._set_midiProgramCount(pluginId, mpcount)
        self.host._set_customDataCount(pluginId, cdcount)
        self.host._set_pluginInfoUpdate(pluginId, { 'programCurrent': cp, 'midiProgramCurrent': cmp })

    @make_method('/ctrl/pcount', 'iii')
    def carla_pcount(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, pcount, mpcount = args
        self.host._set_programCount(pluginId, pcount)
        self.host._set_midiProgramCount(pluginId, mpcount)

    @make_method('/ctrl/prog', 'iis')
    def carla_prog(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, progId, progName = args
        self.host._set_programName(pluginId, progId, progName)

    @make_method('/ctrl/mprog', 'iiiis')
    def carla_mprog(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, midiProgId, bank, program, name = args
        self.host._set_midiProgramData(pluginId, midiProgId, {'bank': bank, 'program': program, 'name': name})

    @make_method('/ctrl/cdata', 'iisss')
    def carla_cdata(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, index, type_, key, value = args
        self.host._set_customData(pluginId, index, { 'type': type_, 'key': key, 'value': value })

    @make_method('/ctrl/iparams', 'ifffffff')
    def carla_iparams(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        pluginId, active, drywet, volume, balLeft, balRight, pan, ctrlChan = args
        self.host._set_internalValue(pluginId, PARAMETER_ACTIVE, active)
        self.host._set_internalValue(pluginId, PARAMETER_DRYWET, drywet)
        self.host._set_internalValue(pluginId, PARAMETER_VOLUME, volume)
        self.host._set_internalValue(pluginId, PARAMETER_BALANCE_LEFT, balLeft)
        self.host._set_internalValue(pluginId, PARAMETER_BALANCE_RIGHT, balRight)
        self.host._set_internalValue(pluginId, PARAMETER_PANNING, pan)
        self.host._set_internalValue(pluginId, PARAMETER_CTRL_CHANNEL, ctrlChan)

    @make_method('/ctrl/resp', 'is')
    def carla_resp(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        messageId, error = args
        self.host.responses[messageId] = error
        self.host.pendingMessages.remove(messageId)

    @make_method('/ctrl/exit', '')
    def carla_exit(self, path, args):
        if DEBUG: print(path, args)
        self.fReceivedMsgs = True
        #self.host.lo_target_tcp = None
        self.host.QuitCallback.emit()

    @make_method('/ctrl/exit-error', 's')
    def carla_exit_error(self, path, args):
        if DEBUG: print(path, args)
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
    def __init__(self, host, rhost):
        Server.__init__(self, proto=LO_UDP)

        if False:
            host = CarlaHostOSC()

        self.host = host
        self.rhost = rhost

    def idle(self):
        self.fReceivedMsgs = False

        while self.recv(0) and self.fReceivedMsgs:
            pass

    def getFullURL(self):
        if self.rhost:
            return "osc.udp://%s:%i/ctrl" % (self.rhost, self.get_port())
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
            host = CarlaHostOSC()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_connect.triggered.connect(self.slot_fileConnect)
        self.ui.act_file_refresh.triggered.connect(self.slot_fileRefresh)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        if oscAddr:
            QTimer.singleShot(0, self.connectOsc)

    def connectOsc(self, addrTCP = None, addrUDP = None, rhost = None):
        if addrTCP is not None:
            self.fOscAddressTCP = addrTCP
        if addrUDP is not None:
            self.fOscAddressUDP = addrUDP
        if rhost is not None:
            self.fOscReportedHost = rhost

        lo_target_tcp_name = self.fOscAddressTCP.rsplit("/", 1)[-1]
        lo_target_udp_name = self.fOscAddressUDP.rsplit("/", 1)[-1]

        err = None

        try:
            lo_target_tcp = Address(self.fOscAddressTCP)
            lo_server_tcp = CarlaControlServerTCP(self.host, self.fOscReportedHost)
            lo_send(lo_target_tcp, "/register", lo_server_tcp.getFullURL())

            lo_target_udp = Address(self.fOscAddressUDP)
            lo_server_udp = CarlaControlServerUDP(self.host, self.fOscReportedHost)
            lo_send(lo_target_udp, "/register", lo_server_udp.getFullURL())

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
        self.host.fPluginsInfo = {}
        HostWindow.removeAllPlugins(self)

    # --------------------------------------------------------------------------------------------------------

    def loadSettings(self, firstTime):
        settings = HostWindow.loadSettings(self, firstTime)
        self.fOscAddressTCP = settings.value("RemoteAddressTCP", "osc.tcp://127.0.0.1:22752/Carla", type=str)
        self.fOscAddressUDP = settings.value("RemoteAddressUDP", "osc.udp://127.0.0.1:22752/Carla", type=str)
        self.fOscReportedHost = settings.value("RemoteReportedHost", "", type=str)

    def saveSettings(self):
        settings = HostWindow.saveSettings(self)
        if self.fOscAddressTCP:
            settings.setValue("RemoteAddressTCP", self.fOscAddressTCP)
        if self.fOscAddressUDP:
            settings.setValue("RemoteAddressUDP", self.fOscAddressUDP)
        if self.fOscReportedHost:
            settings.setValue("RemoteReportedHost", self.fOscReportedHost)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_fileConnect(self):
        dialog = ConnectDialog(self)

        if not dialog.exec_():
            return

        host, rhost, tcpPort, udpPort = dialog.getResult()

        self.disconnectOsc()
        self.connectOsc("osc.tcp://%s:%i/Carla" % (host, tcpPort),
                        "osc.udp://%s:%i/Carla" % (host, udpPort),
                        rhost)

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

        self.host.lo_server_tcp = CarlaControlServerTCP(self.host, self.fOscReportedHost)
        self.host.lo_server_udp = CarlaControlServerUDP(self.host, self.fOscReportedHost)

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

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.host.pendingMessages = []
        self.close()

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
