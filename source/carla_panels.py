#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla host code
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
    from PyQt5.QtCore import pyqtSlot
    from PyQt5.QtWidgets import QDialog
else:
    from PyQt4.QtCore import pyqtSlot
    from PyQt4.QtGui import QDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_shared import *

import ui_carla_panel_time

# ------------------------------------------------------------------------------------------------------------
# Time Panel

class CarlaPanelTime(QDialog):
    def __init__(self, host, parent):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_panel_time.Ui_Dialog()
        self.ui.setupUi(self)

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fLastTransportFrame = 0
        self.fLastTransportState = False
        self.fSampleRate         = 0.0

        #if view == TRANSPORT_VIEW_HMS:
            #self.fCurTransportView = TRANSPORT_VIEW_HMS
            #self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("00:00:00") + 3)
        #elif view == TRANSPORT_VIEW_BBT:
            #self.fCurTransportView = TRANSPORT_VIEW_BBT
            #self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("000|00|0000") + 3)
        #elif view == TRANSPORT_VIEW_FRAMES:
            #self.fCurTransportView = TRANSPORT_VIEW_FRAMES
            #self.ui.label_time.setMinimumWidth(QFontMetrics(self.ui.label_time.font()).width("000'000'000") + 3)

        # ----------------------------------------------------------------------------------------------------
        # Disable buttons if plugin

        if host.isPlugin:
            self.ui.b_play.setEnabled(False)
            self.ui.b_stop.setEnabled(False)
            self.ui.b_backwards.setEnabled(False)
            self.ui.b_forwards.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.b_play.clicked.connect(self.slot_transportPlayPause)
        self.ui.b_stop.clicked.connect(self.slot_transportStop)
        self.ui.b_backwards.clicked.connect(self.slot_transportBackwards)
        self.ui.b_forwards.clicked.connect(self.slot_transportForwards)

        host.EngineStartedCallback.connect(self.slot_handleEngineStartedCallback)
        host.SampleRateChangedCallback.connect(self.slot_handleSampleRateChangedCallback)

    # --------------------------------------------------------------------------------------------------------
    # Button actions

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        if toggled:
            self.host.transport_play()
        else:
            self.host.transport_pause()

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        self.host.transport_pause()
        self.host.transport_relocate(0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() - 100000

        if newFrame < 0:
            newFrame = 0

        self.host.transport_relocate(newFrame)

    @pyqtSlot()
    def slot_transportForwards(self):
        if self.fSampleRate == 0.0 or self.host.isPlugin or not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() + int(self.fSampleRate*2.5)
        self.host.transport_relocate(newFrame)

    def refreshTransport(self, forced = False):
        if self.fSampleRate == 0.0 or not self.host.is_engine_running():
            return

        timeInfo = self.host.get_transport_info()
        playing  = timeInfo['playing']
        frame    = timeInfo['frame']

        if playing != self.fLastTransportState or forced:
            if playing:
                icon = getIcon("media-playback-pause")
                self.ui.b_play.setChecked(True)
                self.ui.b_play.setIcon(icon)
                #self.ui.b_play.setText(self.tr("&Pause"))
            else:
                icon = getIcon("media-playback-start")
                self.ui.b_play.setChecked(False)
                self.ui.b_play.setIcon(icon)
                #self.ui.b_play.setText(self.tr("&Play"))

            self.fLastTransportState = playing

        if frame != self.fLastTransportFrame or forced:
            time = frame / self.fSampleRate
            secs =  time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60

            self.fLastTransportFrame = frame
            self.ui.l_time.setText("Transport %s, at %02i:%02i:%02i" % ("playing" if playing else "stopped", hrs, mins, secs))

        #if self.fCurTransportView == TRANSPORT_VIEW_HMS:
            #time = pos.frame / int(self.fSampleRate)
            #secs = time % 60
            #mins = (time / 60) % 60
            #hrs  = (time / 3600) % 60
            #self.ui.label_time.setText("%02i:%02i:%02i" % (hrs, mins, secs))

        #elif self.fCurTransportView == TRANSPORT_VIEW_BBT:
            #if pos.valid & jacklib.JackPositionBBT:
                #bar  = pos.bar
                #beat = pos.beat if bar != 0 else 0
                #tick = pos.tick if bar != 0 else 0
            #else:
                #bar  = 0
                #beat = 0
                #tick = 0

            #self.ui.label_time.setText("%03i|%02i|%04i" % (bar, beat, tick))

        #elif self.fCurTransportView == TRANSPORT_VIEW_FRAMES:
            #frame1 =  pos.frame % 1000
            #frame2 = (pos.frame / 1000) % 1000
            #frame3 = (pos.frame / 1000000) % 1000
            #self.ui.label_time.setText("%03i'%03i'%03i" % (frame3, frame2, frame1))

        #if pos.valid & jacklib.JackPositionBBT:
            #if self.fLastBPM != pos.beats_per_minute:
                #self.ui.sb_bpm.setValue(pos.beats_per_minute)
                #self.ui.sb_bpm.setStyleSheet("")
        #else:
            #pos.beats_per_minute = -1.0
            #if self.fLastBPM != pos.beats_per_minute:
                #self.ui.sb_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

        #self.fLastBPM = pos.beats_per_minute

        #if state != self.fLastTransportState:
            #self.fLastTransportState = state

            #if state == jacklib.JackTransportStopped:
                #icon = getIcon("media-playback-start")
                #self.ui.act_transport_play.setChecked(False)
                #self.ui.act_transport_play.setText(self.tr("&Play"))
                #self.ui.b_transport_play.setChecked(False)
            #else:
                #icon = getIcon("media-playback-pause")
                #self.ui.act_transport_play.setChecked(True)
                #self.ui.act_transport_play.setText(self.tr("&Pause"))
                #self.ui.b_transport_play.setChecked(True)

            #self.ui.act_transport_play.setIcon(icon)
            #self.ui.b_transport_play.setIcon(icon)

    # --------------------------------------------------------------------------------------------------------
    # Engine callbacks

    @pyqtSlot(str)
    def slot_handleEngineStartedCallback(self, processMode, transportMode, driverName):
        self.fSampleRate = self.host.get_sample_rate()
        self.refreshTransport(True)

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fSampleRate = newSampleRate
        self.refreshTransport(True)

# ------------------------------------------------------------------------------------------------------------
