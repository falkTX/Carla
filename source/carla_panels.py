# -*- coding: utf-8 -*-

# Carla host code
# Copyright (C) 2011-2017 Filipe Coelho <falktx@falktx.com>
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
    from PyQt5.QtGui import QFontMetrics
    from PyQt5.QtWidgets import QDialog
else:
    from PyQt4.QtCore import pyqtSlot
    from PyQt4.QtGui import QDialog, QFontMetrics

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

        self.fLastTransportBPM   = 0.0
        self.fLastTransportFrame = 0
        self.fLastTransportState = False
        self.fSampleRate         = 0.0

        self.ui.l_time_frame.setMinimumWidth(QFontMetrics(self.ui.l_time_frame.font()).width("000'000'000") + 3)
        self.ui.l_time_clock.setMinimumWidth(QFontMetrics(self.ui.l_time_clock.font()).width("00:00:00") + 3)
        self.ui.l_time_bbt.setMinimumWidth(QFontMetrics(self.ui.l_time_bbt.font()).width("000|00|0000") + 3)

        # ----------------------------------------------------------------------------------------------------
        # Disable buttons if plugin

        if host.isPlugin:
            self.ui.b_play.setEnabled(False)
            self.ui.b_stop.setEnabled(False)
            self.ui.b_backwards.setEnabled(False)
            self.ui.b_forwards.setEnabled(False)
            self.ui.cb_transport_link.setEnabled(False)
            self.ui.dsb_bpm.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.b_play.clicked.connect(self.slot_transportPlayPause)
        self.ui.b_stop.clicked.connect(self.slot_transportStop)
        self.ui.b_backwards.clicked.connect(self.slot_transportBackwards)
        self.ui.b_forwards.clicked.connect(self.slot_transportForwards)
        self.ui.cb_transport_jack.clicked.connect(self.slot_transportJackEnabled)
        self.ui.cb_transport_link.clicked.connect(self.slot_transportLinkEnabled)

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

    @pyqtSlot(bool)
    def slot_transportJackEnabled(self, clicked):
        if not self.host.is_engine_running():
            return
        mode  = ENGINE_TRANSPORT_MODE_JACK if clicked else ENGINE_TRANSPORT_MODE_INTERNAL
        extra = ":link:" if self.ui.cb_transport_link.isChecked() else ""
        self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, mode, extra)

    @pyqtSlot(bool)
    def slot_transportLinkEnabled(self, clicked):
        if not self.host.is_engine_running():
            return
        mode  = ENGINE_TRANSPORT_MODE_JACK if self.ui.cb_transport_jack.isChecked() else ENGINE_TRANSPORT_MODE_INTERNAL
        extra = ":link:" if clicked else ""
        self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, mode, extra)

    def showEvent(self, event):
        self.refreshTransport(True)
        QDialog.showEvent(self, event)

    def refreshTransport(self, forced = False):
        if not self.isVisible():
            return
        if self.fSampleRate == 0.0 or not self.host.is_engine_running():
            return

        timeInfo = self.host.get_transport_info()
        playing  = timeInfo['playing']
        frame    = timeInfo['frame']
        bpm      = timeInfo['bpm']

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
            self.fLastTransportFrame = frame

            time = frame / self.fSampleRate
            secs =  time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60
            self.ui.l_time_clock.setText("%02i:%02i:%02i" % (hrs, mins, secs))

            frame1 =  frame % 1000
            frame2 = (frame / 1000) % 1000
            frame3 = (frame / 1000000) % 1000
            self.ui.l_time_frame.setText("%03i'%03i'%03i" % (frame3, frame2, frame1))

            bar  = timeInfo['bar']
            beat = timeInfo['beat']
            tick = timeInfo['tick']
            self.ui.l_time_bbt.setText("%03i|%02i|%04i" % (bar, beat, tick))

        if bpm != self.fLastTransportBPM or forced:
            self.fLastTransportBPM = bpm

            if bpm > 0.0:
                self.ui.dsb_bpm.setValue(bpm)
                self.ui.dsb_bpm.setStyleSheet("")
            else:
                self.ui.dsb_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

    # --------------------------------------------------------------------------------------------------------
    # Engine callbacks

    @pyqtSlot(int, int, str)
    def slot_handleEngineStartedCallback(self, processMode, transportMode, driverName):
        self.fSampleRate = self.host.get_sample_rate()
        self.refreshTransport(True)

        self.ui.cb_transport_jack.setEnabled(driverName == "JACK")
        self.ui.cb_transport_jack.setChecked(transportMode == ENGINE_TRANSPORT_MODE_JACK)

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.fSampleRate = newSampleRate
        self.refreshTransport(True)

# ------------------------------------------------------------------------------------------------------------
