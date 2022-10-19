#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
# Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os

from PyQt5.QtCore import pyqtSlot, Qt
from PyQt5.QtWidgets import QDialog, QDialogButtonBox, QWidget

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Carla)

from utils import QSafeSettings

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Local)

from .jackappdialog_ui import Ui_JackAppDialog

# ---------------------------------------------------------------------------------------------------------------------
# Imports (API)

SESSION_MGR_NONE   = 0
SESSION_MGR_AUTO   = 1
SESSION_MGR_JACK   = 2
SESSION_MGR_LADISH = 3
SESSION_MGR_NSM    = 4

FLAG_CONTROL_WINDOW              = 0x01
FLAG_CAPTURE_FIRST_WINDOW        = 0x02
FLAG_BUFFERS_ADDITION_MODE       = 0x10
FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN = 0x20
FLAG_EXTERNAL_START              = 0x40

# ---------------------------------------------------------------------------------------------------------------------
# Jack Application Dialog

UI_SESSION_NONE   = 0
UI_SESSION_LADISH = 1
UI_SESSION_NSM    = 2

class JackApplicationW(QDialog):
    def __init__(self, parent: QWidget, projectFilename: str):
        QDialog.__init__(self, parent)
        self.ui = Ui_JackAppDialog()
        self.ui.setupUi(self)

        self.fProjectFilename = projectFilename

        # --------------------------------------------------------------------------------------------------------------
        # UI setup

        self.ui.group_error.setVisible(False)
        self.adjustSize()
        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        # --------------------------------------------------------------------------------------------------------------
        # Load settings

        self._loadSettings()

        # --------------------------------------------------------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self._slot_saveSettings)
        self.ui.cb_session_mgr.currentIndexChanged.connect(self._slot_sessionManagerChanged)
        self.ui.le_command.textChanged.connect(self._slot_commandChanged)

    # -----------------------------------------------------------------------------------------------------------------
    # public methods

    def getCommandAndFlags(self):
        name    = self.ui.le_name.text()
        command = self.ui.le_command.text()
        smgr    = SESSION_MGR_NONE
        flags   = 0x0

        if not name:
            name = os.path.basename(command.split(" ",1)[0]).title()

        uiSessionMgrIndex = self.ui.cb_session_mgr.currentIndex()
        if uiSessionMgrIndex == UI_SESSION_LADISH:
            smgr = SESSION_MGR_LADISH
        elif uiSessionMgrIndex == UI_SESSION_NSM:
            smgr = SESSION_MGR_NSM

        if self.ui.cb_manage_window.isChecked():
            flags |= FLAG_CONTROL_WINDOW
        if self.ui.cb_capture_first_window.isChecked():
            flags |= FLAG_CAPTURE_FIRST_WINDOW
        if self.ui.cb_buffers_addition_mode.isChecked():
            flags |= FLAG_BUFFERS_ADDITION_MODE
        if self.ui.cb_out_midi_mixdown.isChecked():
            flags |= FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN
        if self.ui.cb_external_start.isChecked():
            flags |= FLAG_EXTERNAL_START

        bv = ord('0')
        v1 = chr(bv + self.ui.sb_audio_ins.value())
        v2 = chr(bv + self.ui.sb_audio_outs.value())
        v3 = chr(bv + self.ui.sb_midi_ins.value())
        v4 = chr(bv + self.ui.sb_midi_outs.value())
        v5 = chr(bv + smgr)
        v6 = chr(bv + flags)
        labelSetup = f"{v1}{v2}{v3}{v4}{v5}{v6}"

        return (command, name, labelSetup)

    # -----------------------------------------------------------------------------------------------------------------
    # private methods

    def _checkIfButtonBoxShouldBeEnabled(self, index: int, command: str):
        enabled = len(command) > 0
        showErr = ""

        # NSM applications must not be abstract or absolute paths, and must not contain arguments
        if enabled and index == UI_SESSION_NSM:
            if command[0] in (".", "/"):
                showErr = self.tr("NSM applications cannot use abstract or absolute paths")
            elif " " in command or ";" in command or "&" in command:
                showErr = self.tr("NSM applications cannot use CLI arguments")
            elif not self.fProjectFilename:
                showErr = self.tr("You need to save the current Carla project before NSM can be used")

        if showErr:
            enabled = False
            self.ui.l_error.setText(showErr)
            self.ui.group_error.setVisible(True)
        else:
            self.ui.group_error.setVisible(False)

        self.ui.buttonBox.button(QDialogButtonBox.Ok).setEnabled(enabled)

    def _loadSettings(self):
        settings = QSafeSettings("falkTX", "CarlaAddJackApp")

        smName = settings.value("SessionManager", "", str)

        if smName == "LADISH (SIGUSR1)":
            self.ui.cb_session_mgr.setCurrentIndex(UI_SESSION_LADISH)
        elif smName == "NSM":
            self.ui.cb_session_mgr.setCurrentIndex(UI_SESSION_NSM)
        else:
            self.ui.cb_session_mgr.setCurrentIndex(UI_SESSION_NONE)

        self.ui.le_command.setText(settings.value("Command", "", str))
        self.ui.le_name.setText(settings.value("Name", "", str))
        self.ui.sb_audio_ins.setValue(settings.value("NumAudioIns", 2, int))
        self.ui.sb_audio_ins.setValue(settings.value("NumAudioIns", 2, int))
        self.ui.sb_audio_outs.setValue(settings.value("NumAudioOuts", 2, int))
        self.ui.sb_midi_ins.setValue(settings.value("NumMidiIns", 0, int))
        self.ui.sb_midi_outs.setValue(settings.value("NumMidiOuts", 0, int))
        self.ui.cb_manage_window.setChecked(settings.value("ManageWindow", True, bool))
        self.ui.cb_capture_first_window.setChecked(settings.value("CaptureFirstWindow", False, bool))
        self.ui.cb_out_midi_mixdown.setChecked(settings.value("MidiOutMixdown", False, bool))

        self._checkIfButtonBoxShouldBeEnabled(self.ui.cb_session_mgr.currentIndex(),
                                              self.ui.le_command.text())

    # -----------------------------------------------------------------------------------------------------------------
    # private slots

    @pyqtSlot(str)
    def _slot_commandChanged(self, command: str):
        self._checkIfButtonBoxShouldBeEnabled(self.ui.cb_session_mgr.currentIndex(), command)

    @pyqtSlot(int)
    def _slot_sessionManagerChanged(self, index: int):
        self._checkIfButtonBoxShouldBeEnabled(index, self.ui.le_command.text())

    @pyqtSlot()
    def _slot_saveSettings(self):
        settings = QSafeSettings("falkTX", "CarlaAddJackApp")
        settings.setValue("Command", self.ui.le_command.text())
        settings.setValue("Name", self.ui.le_name.text())
        settings.setValue("SessionManager", self.ui.cb_session_mgr.currentText())
        settings.setValue("NumAudioIns", self.ui.sb_audio_ins.value())
        settings.setValue("NumAudioOuts", self.ui.sb_audio_outs.value())
        settings.setValue("NumMidiIns", self.ui.sb_midi_ins.value())
        settings.setValue("NumMidiOuts", self.ui.sb_midi_outs.value())
        settings.setValue("ManageWindow", self.ui.cb_manage_window.isChecked())
        settings.setValue("CaptureFirstWindow", self.ui.cb_capture_first_window.isChecked())
        settings.setValue("MidiOutMixdown", self.ui.cb_out_midi_mixdown.isChecked())

# ---------------------------------------------------------------------------------------------------------------------
# Testing

if __name__ == '__main__':
    import sys
    # pylint: disable=ungrouped-imports
    from PyQt5.QtWidgets import QApplication
    # pylint: enable=ungrouped-imports

    _app = QApplication(sys.argv)
    _gui = JackApplicationW(None, "")

    if _gui.exec_():
        _command, _name, _labelSetup = _gui.getCommandAndFlags()
        print("Results:")
        print(f"\tCommand:    {_command}")
        print(f"\tName:       {_name}")
        print(f"\tLabelSetup: {_labelSetup}")

# ---------------------------------------------------------------------------------------------------------------------
