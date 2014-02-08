#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# External UI
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

# -----------------------------------------------------------------------
# Imports

from os import fdopen, O_NONBLOCK
from fcntl import fcntl, F_GETFL, F_SETFL
from sys import argv

# -----------------------------------------------------------------------
# External UI

class ExternalUI(object):
    def __init__(self):
        object.__init__(self)

        self.fPipeRecv     = None
        self.fPipeSend     = None
        self.fQuitReceived = False

        if len(argv) > 1:
            self.fSampleRate = float(argv[1])
            self.fUiName     = argv[2]
            self.fPipeRecvFd = int(argv[3])
            self.fPipeSendFd = int(argv[4])

            fcntl(self.fPipeRecvFd, F_SETFL, fcntl(self.fPipeRecvFd, F_GETFL) | O_NONBLOCK)

            self.fPipeRecv = fdopen(self.fPipeRecvFd, 'r')
            self.fPipeSend = fdopen(self.fPipeSendFd, 'w')

        else:
            self.fSampleRate = 44100.0
            self.fUiName     = "TestUI"

            self.fPipeRecv = None
            self.fPipeSend = None

        # send empty line (just newline char)
        self.send([""])

    def showUiIfTesting(self):
        if self.fPipeRecv is None:
            self.d_uiShow()

    # -------------------------------------------------------------------
    # Host DSP State

    def d_getSampleRate(self):
        return self.fSampleRate

    def d_editParameter(self, index, started):
        self.send(["editParam", index, started])

    def d_setParameterValue(self, index, value):
        self.send(["control", index, value])

    def d_setState(self, key, value):
        self.send(["configure", key, value])

    def d_sendNote(self, onOff, channel, note, velocity):
        self.send(["note", onOff, channel, note, velocity])

    # -------------------------------------------------------------------
    # DSP Callbacks

    def d_parameterChanged(self, index, value):
        return

    def d_programChanged(self, index):
        return

    def d_stateChanged(self, key, value):
        return

    def d_noteReceived(self, onOff, channel, note, velocity):
        return

    # -------------------------------------------------------------------
    # ExternalUI Callbacks

    def d_uiShow(self):
        return

    def d_uiHide(self):
        return

    def d_uiQuit(self):
        return

    def d_uiTitleChanged(self, uiTitle):
        return

    # -------------------------------------------------------------------
    # Public methods

    def closeExternalUI(self):
        if not self.fQuitReceived:
            self.send(["exiting"])

        if self.fPipeRecv is not None:
          self.fPipeRecv.close()
          self.fPipeRecv = None

        if self.fPipeSend is not None:
          self.fPipeSend.close()
          self.fPipeSend = None

    def idleExternalUI(self):
        while True:
            if self.fPipeRecv is None:
                return True

            try:
                msg = self.fPipeRecv.readline().strip()
            except IOError:
                return False

            if not msg:
                return True

            elif msg == "control":
                index = int(self.fPipeRecv.readline())
                value = float(self.fPipeRecv.readline())
                self.d_parameterChanged(index, value)

            elif msg == "program":
                index = int(self.fPipeRecv.readline())
                self.d_programChanged(index)

            elif msg == "configure":
                key   = self.fPipeRecv.readline().strip().replace("\r", "\n")
                value = self.fPipeRecv.readline().strip().replace("\r", "\n")
                self.d_stateChanged(key, value)

            elif msg == "note":
                onOff    = bool(self.fPipeRecv.readline().strip() == "true")
                channel  = int(self.fPipeRecv.readline())
                note     = int(self.fPipeRecv.readline())
                velocity = int(self.fPipeRecv.readline())
                self.d_noteReceived(onOff, channel, note, velocity)

            elif msg == "show":
                self.d_uiShow()

            elif msg == "hide":
                self.d_uiHide()

            elif msg == "quit":
                self.fQuitReceived = True
                self.d_uiQuit()

            elif msg == "uiTitle":
                uiTitle = self.fPipeRecv.readline().strip().replace("\r", "\n")
                self.d_uiTitleChanged(uiTitle)

            else:
                print("unknown message: \"" + msg + "\"")

        return True

    # -------------------------------------------------------------------
    # Internal stuff

    def send(self, lines):
        if self.fPipeSend is None:
            return

        for line in lines:
            if line is None:
                line2 = "(null)"
            elif isinstance(line, str):
                line2 = line.replace("\n", "\r")
            else:
                if isinstance(line, bool):
                    line2 = "true" if line else "false"
                elif isinstance(line, int):
                    line2 = "%i" % line
                elif isinstance(line, float):
                    line2 = "%.10f" % line
                else:
                    print("unknown data type to send:", type(line))
                    return

            self.fPipeSend.write(line2 + "\n")
            self.fPipeSend.flush()
