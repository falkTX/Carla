#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla bridge for LV2 modguis
# Copyright (C) 2015-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QPoint, QSize, QUrl
from PyQt5.QtGui import QImage, QPainter, QPalette
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtWebKit import QWebSettings
from PyQt5.QtWebKitWidgets import QWebView

import sys

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_host import charPtrToString, gCarla
from .webserver import WebServerThread, PORT

# ------------------------------------------------------------------------------------------------------------
# Imports (MOD)

from modtools.utils import get_plugin_info, init as lv2_init

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    # --------------------------------------------------------------------------------------------------------

    def __init__(self):
        QMainWindow.__init__(self)
        gCarla.gui = self

        URI = sys.argv[1]

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fCurrentFrame = None
        self.fDocElemement = None
        self.fCanSetValues = False
        self.fNeedsShow    = False
        self.fSizeSetup    = False
        self.fQuitReceived = False
        self.fWasRepainted = False

        lv2_init()

        self.fPlugin      = get_plugin_info(URI)
        self.fPorts       = self.fPlugin['ports']
        self.fPortSymbols = {}
        self.fPortValues  = {}
        self.fParamTypes  = {}
        self.fParamValues = {}

        for port in self.fPorts['control']['input']:
            self.fPortSymbols[port['index']] = (port['symbol'], False)
            self.fPortValues [port['index']] = port['ranges']['default']

        for port in self.fPorts['control']['output']:
            self.fPortSymbols[port['index']] = (port['symbol'], True)
            self.fPortValues [port['index']] = port['ranges']['default']

        for parameter in self.fPlugin['parameters']:
            if parameter['ranges'] is None:
                continue
            if parameter['type'] == "http://lv2plug.in/ns/ext/atom#Bool":
                paramtype = 'b'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#Int":
                paramtype = 'i'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#Long":
                paramtype = 'l'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#Float":
                paramtype = 'f'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#Double":
                paramtype = 'g'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#String":
                paramtype = 's'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#Path":
                paramtype = 'p'
            elif parameter['type'] == "http://lv2plug.in/ns/ext/atom#URI":
                paramtype = 'u'
            else:
                continue
            if paramtype not in ('s','p','u') and parameter['ranges']['minimum'] == parameter['ranges']['maximum']:
                continue
            self.fParamTypes [parameter['uri']] = paramtype
            self.fParamValues[parameter['uri']] = parameter['ranges']['default']

        # ----------------------------------------------------------------------------------------------------
        # Init pipe

        if len(sys.argv) == 7:
            self.fPipeClient = gCarla.utils.pipe_client_new(lambda s,msg: self.msgCallback(msg))
        else:
            self.fPipeClient = None

        # ----------------------------------------------------------------------------------------------------
        # Init Web server

        self.fWebServerThread = WebServerThread(self)
        self.fWebServerThread.start()

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI

        self.setContentsMargins(0, 0, 0, 0)

        self.fWebview = QWebView(self)
        #self.fWebview.setAttribute(Qt.WA_OpaquePaintEvent, False)
        #self.fWebview.setAttribute(Qt.WA_TranslucentBackground, True)
        self.setCentralWidget(self.fWebview)

        page = self.fWebview.page()
        page.setViewportSize(QSize(980, 600))

        mainFrame = page.mainFrame()
        mainFrame.setScrollBarPolicy(Qt.Horizontal, Qt.ScrollBarAlwaysOff)
        mainFrame.setScrollBarPolicy(Qt.Vertical, Qt.ScrollBarAlwaysOff)

        palette = self.fWebview.palette()
        palette.setBrush(QPalette.Base, palette.brush(QPalette.Window))
        page.setPalette(palette)
        self.fWebview.setPalette(palette)

        settings = self.fWebview.settings()
        settings.setAttribute(QWebSettings.DeveloperExtrasEnabled, True)

        self.fWebview.loadFinished.connect(self.slot_webviewLoadFinished)

        url = "http://127.0.0.1:%s/icon.html#%s" % (PORT, URI)
        print("url:", url)
        self.fWebview.load(QUrl(url))

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.SIGTERM.connect(self.slot_handleSIGTERM)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        self.fIdleTimer = self.startTimer(30)

        if self.fPipeClient is None:
            # testing, show UI only
            self.setWindowTitle("TestUI")
            self.fNeedsShow = True

    # --------------------------------------------------------------------------------------------------------

    def closeExternalUI(self):
        self.fWebServerThread.stopWait()

        if self.fPipeClient is None:
            return

        if not self.fQuitReceived:
            self.send(["exiting"])

        gCarla.utils.pipe_client_destroy(self.fPipeClient)
        self.fPipeClient = None

    def idleStuff(self):
        if self.fPipeClient is not None:
            gCarla.utils.pipe_client_idle(self.fPipeClient)
            self.checkForRepaintChanges()

        if self.fSizeSetup:
            return
        if self.fDocElemement is None or self.fDocElemement.isNull():
            return

        pedal = self.fDocElemement.findFirst(".mod-pedal")

        if pedal.isNull():
            return

        size = pedal.geometry().size()

        if size.width() <= 10 or size.height() <= 10:
            return

        # render web frame to image
        image = QImage(self.fWebview.page().viewportSize(), QImage.Format_ARGB32_Premultiplied)
        image.fill(Qt.transparent)

        painter = QPainter(image)
        self.fCurrentFrame.render(painter)
        painter.end()

        #image.save("/tmp/test.png")

        # get coordinates and size from image
        #x = -1
        #y = -1
        #lastx = -1
        #lasty = -1
        #bgcol = self.fHostColor.rgba()

        #for h in range(0, image.height()):
            #hasNonTransPixels = False

            #for w in range(0, image.width()):
                #if image.pixel(w, h) not in (0, bgcol): # 0xff070707):
                    #hasNonTransPixels = True
                    #if x == -1 or x > w:
                        #x = w
                    #lastx = max(lastx, w)

            #if hasNonTransPixels:
                ##if y == -1:
                    ##y = h
                #lasty = h

        # set size and position accordingly
        #if -1 not in (x, lastx, lasty):
            #self.setFixedSize(lastx-x, lasty)
            #self.fCurrentFrame.setScrollPosition(QPoint(x, 0))
        #else:

        # TODO that^ needs work
        if True:
            self.setFixedSize(size)

        # set initial values
        self.fCurrentFrame.evaluateJavaScript("icongui.setPortWidgetsValue(':bypass', 0, null)")

        for index in self.fPortValues.keys():
            symbol, isOutput = self.fPortSymbols[index]
            value            = self.fPortValues[index]
            if isOutput:
                self.fCurrentFrame.evaluateJavaScript("icongui.setOutputPortValue('%s', %f)" % (symbol, value))
            else:
                self.fCurrentFrame.evaluateJavaScript("icongui.setPortWidgetsValue('%s', %f, null)" % (symbol, value))

        for uri in self.fParamValues.keys():
            ptype = self.fParamTypes[uri]
            value = str(self.fParamValues[uri])
            print("icongui.setWritableParameterValue('%s', '%c', %s, 'from-carla')" % (uri, ptype, value))
            self.fCurrentFrame.evaluateJavaScript("icongui.setWritableParameterValue('%s', '%c', %s, 'from-carla')" % (
                  uri, ptype, value))

        # final setup
        self.fCanSetValues = True
        self.fSizeSetup    = True
        self.fDocElemement = None

        if self.fNeedsShow:
            self.show()

    def checkForRepaintChanges(self):
        if not self.fWasRepainted:
            return

        self.fWasRepainted = False

        if not self.fCanSetValues:
            return

        for index in self.fPortValues.keys():
            symbol, isOutput = self.fPortSymbols[index]

            if isOutput:
                continue

            oldValue = self.fPortValues[index]
            newValue = self.fCurrentFrame.evaluateJavaScript("icongui.controls['%s'].value" % (symbol,))

            if oldValue != newValue:
                self.fPortValues[index] = newValue
                self.send(["control", index, newValue])

        for uri in self.fParamValues.keys():
            oldValue = self.fParamValues[uri]
            newValue = self.fCurrentFrame.evaluateJavaScript("icongui.parameters['%s'].value" % (uri,))

            if oldValue != newValue:
                self.fParamValues[uri] = newValue
                self.send(["pcontrol", uri, newValue])

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_webviewLoadFinished(self, ok):
        page = self.fWebview.page()
        page.repaintRequested.connect(self.slot_repaintRequested)

        self.fCurrentFrame = page.currentFrame()
        self.fDocElemement = self.fCurrentFrame.documentElement()

    def slot_repaintRequested(self):
        if self.fCanSetValues:
            self.fWasRepainted = True

    # --------------------------------------------------------------------------------------------------------
    # Callback

    def msgCallback(self, msg):
        msg = charPtrToString(msg)

        if msg == "control":
            index = self.readlineblock_int()
            value = self.readlineblock_float()
            self.dspControlChanged(index, value)

        elif msg == "parameter":
            uri   = self.readlineblock()
            value = self.readlineblock_float()
            self.dspParameterChanged(uri, value)

        elif msg == "program":
            index = self.readlineblock_int()
            self.dspProgramChanged(index)

        elif msg == "midiprogram":
            bank    = self.readlineblock_int()
            program = self.readlineblock_int()
            self.dspMidiProgramChanged(bank, program)

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

        elif msg == "atom":
            index      = self.readlineblock_int()
            atomsize   = self.readlineblock_int()
            base64size = self.readlineblock_int()
            base64atom = self.readlineblock()
            # nothing to do yet

        elif msg == "urid":
            urid = self.readlineblock_int()
            size = self.readlineblock_int()
            uri  = self.readlineblock()
            # nothing to do yet

        elif msg == "uiOptions":
            sampleRate     = self.readlineblock_float()
            bgColor        = self.readlineblock_int()
            fgColor        = self.readlineblock_int()
            uiScale        = self.readlineblock_float()
            useTheme       = self.readlineblock_bool()
            useThemeColors = self.readlineblock_bool()
            windowTitle    = self.readlineblock()
            transWindowId  = self.readlineblock_int()
            self.uiTitleChanged(windowTitle)

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

    # --------------------------------------------------------------------------------------------------------

    def dspControlChanged(self, index, value):
        self.fPortValues[index] = value

        if self.fCurrentFrame is None or not self.fCanSetValues:
            return

        symbol, isOutput = self.fPortSymbols[index]

        if isOutput:
            self.fCurrentFrame.evaluateJavaScript("icongui.setOutputPortValue('%s', %f)" % (symbol, value))
        else:
            self.fCurrentFrame.evaluateJavaScript("icongui.setPortWidgetsValue('%s', %f, null)" % (symbol, value))

    def dspParameterChanged(self, uri, value):
        print("dspParameterChanged", uri, value)
        if uri not in self.fParamValues:
            return

        self.fParamValues[uri] = value

        if self.fCurrentFrame is None or not self.fCanSetValues:
            return

        ptype = self.fParamTypes[uri]

        self.fCurrentFrame.evaluateJavaScript("icongui.setWritableParameterValue('%s', '%c', %f, 'from-carla')" % (
            uri, ptype, value))

    def dspProgramChanged(self, index):
        return

    def dspMidiProgramChanged(self, bank, program):
        return

    def dspStateChanged(self, key, value):
        return

    def dspNoteReceived(self, onOff, channel, note, velocity):
        return

    # --------------------------------------------------------------------------------------------------------

    def uiShow(self):
        if self.fSizeSetup:
            self.show()
        else:
            self.fNeedsShow = True

    def uiFocus(self):
        if not self.fSizeSetup:
            return

        self.setWindowState((self.windowState() & ~Qt.WindowMinimized) | Qt.WindowActive)
        self.show()

        self.raise_()
        self.activateWindow()

    def uiHide(self):
        self.hide()

    def uiQuit(self):
        self.closeExternalUI()
        self.close()
        QApplication.instance().quit()

    def uiTitleChanged(self, uiTitle):
        self.setWindowTitle(uiTitle)

    # --------------------------------------------------------------------------------------------------------
    # Qt events

    def closeEvent(self, event):
        self.closeExternalUI()
        QMainWindow.closeEvent(self, event)

        # there might be other qt windows open which will block carla-modgui from quitting
        QApplication.instance().quit()

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimer:
            self.idleStuff()

        QMainWindow.timerEvent(self, event)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    # --------------------------------------------------------------------------------------------------------
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
            return

        gCarla.utils.pipe_client_lock(self.fPipeClient)

        # this must never fail, we need to unlock at the end
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
                    return

                gCarla.utils.pipe_client_write_msg(self.fPipeClient, line2 + "\n")
        except:
            pass

        gCarla.utils.pipe_client_flush_and_unlock(self.fPipeClient)
