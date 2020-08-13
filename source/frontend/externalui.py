#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# External UI
# Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
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
# Imports (Custom Stuff)

from carla_backend import charPtrToString
from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# External UI

class ExternalUI(object):
    def __init__(self):
        object.__init__(self)

        self.fQuitReceived = False

        if len(sys.argv) > 1:
            self.fSampleRate = float(sys.argv[1])
            self.fUiName     = sys.argv[2]
            self.fPipeClient = gCarla.utils.pipe_client_new(lambda s,msg: self.msgCallback(msg))
        else:
            self.fSampleRate = 44100.0
            self.fUiName     = "TestUI"
            self.fPipeClient = None

    # -------------------------------------------------------------------
    # Public methods

    def ready(self):
        if self.fPipeClient is None:
            # testing, show UI only
            self.uiShow()

    def isRunning(self):
        if self.fPipeClient is not None:
            return gCarla.utils.pipe_client_is_running(self.fPipeClient)
        return False

    def idleExternalUI(self):
        if self.fPipeClient is not None:
            gCarla.utils.pipe_client_idle(self.fPipeClient)

    def closeExternalUI(self):
        if self.fPipeClient is None:
            return

        # FIXME
        if not self.fQuitReceived:
            self.send(["exiting"])

        gCarla.utils.pipe_client_destroy(self.fPipeClient)
        self.fPipeClient = None

    # -------------------------------------------------------------------
    # Host DSP State

    def getSampleRate(self):
        return self.fSampleRate

    def sendControl(self, index, value):
        self.send(["control", index, value])

    def sendProgram(self, channel, bank, program):
        self.send(["program", channel, bank, program])

    def sendConfigure(self, key, value):
        self.send(["configure", key, value])

    # -------------------------------------------------------------------
    # DSP Callbacks

    def dspParameterChanged(self, index, value):
        return

    def dspProgramChanged(self, channel, bank, program):
        return

    def dspStateChanged(self, key, value):
        return

    def dspNoteReceived(self, onOff, channel, note, velocity):
        return

    # -------------------------------------------------------------------
    # ExternalUI Callbacks

    def uiShow(self):
        return

    def uiFocus(self):
        return

    def uiHide(self):
        return

    def uiQuit(self):
        self.closeExternalUI()

    def uiTitleChanged(self, uiTitle):
        return

    # -------------------------------------------------------------------
    # Callback

    def msgCallback(self, msg):
        msg = charPtrToString(msg)

        #if not msg:
            #return

        if msg == "control":
            index = self.readlineblock_int()
            value = self.readlineblock_float()
            self.dspParameterChanged(index, value)

        elif msg == "program":
            channel = self.readlineblock_int()
            bank    = self.readlineblock_int()
            program = self.readlineblock_int()
            self.dspProgramChanged(channel, bank, program)

        elif msg == "configure":
            key   = self.readlineblock()
            value = self.readlineblock()
            self.dspStateChanged(key, value)

        elif msg == "note":
            onOff    = self.readlineblock_bool()
            channel  = self.readlineblock_int()
            note     = self.readlineblock_int()
            velocity = self.readlineblock_int()
            self.dspNoteReceived(onOff, channel, note, velocity)

        elif msg == "show":
            self.uiShow()

        elif msg == "focus":
            self.uiFocus()

        elif msg == "hide":
            self.uiHide()

        elif msg == "quit":
            self.fQuitReceived = True
            self.uiQuit()

        elif msg == "uiTitle":
            uiTitle = self.readlineblock()
            self.uiTitleChanged(uiTitle)

        else:
            print("unknown message: \"" + msg + "\"")

    # -------------------------------------------------------------------
    # Internal stuff

    def readlineblock(self):
        if self.fPipeClient is None:
            return ""

        return gCarla.utils.pipe_client_readlineblock(self.fPipeClient, 5000)

    def readlineblock_bool(self):
        if self.fPipeClient is None:
            return False

        return gCarla.utils.pipe_client_readlineblock_bool(self.fPipeClient, 5000)

    def readlineblock_int(self):
        if self.fPipeClient is None:
            return 0

        return gCarla.utils.pipe_client_readlineblock_int(self.fPipeClient, 5000)

    def readlineblock_float(self):
        if self.fPipeClient is None:
            return 0.0

        return gCarla.utils.pipe_client_readlineblock_float(self.fPipeClient, 5000)

    def send(self, lines):
        if self.fPipeClient is None or len(lines) == 0:
            return False

        hasError = False
        gCarla.utils.pipe_client_lock(self.fPipeClient)

        try:
            for line in lines:
                if line is None:
                    line2 = "(null)"
                elif isinstance(line, str):
                    line2 = line.replace("\n", "\r")
                elif isinstance(line, bool):
                    line2 = "true" if line else "false"
                elif isinstance(line, int):
                    line2 = "%i" % line
                elif isinstance(line, float):
                    line2 = "%.10f" % line
                else:
                    print("unknown data type to send:", type(line))
                    hasError = True
                    break

                if not gCarla.utils.pipe_client_write_msg(self.fPipeClient, line2 + "\n"):
                    hasError = True
                    break

        finally:
            return gCarla.utils.pipe_client_flush_and_unlock(self.fPipeClient) and not hasError
