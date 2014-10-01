#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla patchbay widget code
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
    from PyQt5.QtCore import QPointF, QTimer
    from PyQt5.QtGui import QImage
    from PyQt5.QtPrintSupport import QPrinter, QPrintDialog
    from PyQt5.QtWidgets import QFrame, QGraphicsView, QGridLayout
else:
    from PyQt4.QtCore import QPointF, QTimer
    from PyQt4.QtGui import QFrame, QGraphicsView, QGridLayout, QImage, QPrinter, QPrintDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas

from carla_host import *
from digitalpeakmeter import DigitalPeakMeter
from pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------
# Patchbay widget

class CarlaPatchbayW(QFrame, PluginEditParentMeta):
#class CarlaPatchbayW(QFrame, PluginEditParentMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, doSetup = True, onlyPatchbay = True, is3D = False):
        QFrame.__init__(self, parent)
        self.host = host

        # -------------------------------------------------------------
        # Connect actions to functions

        #self.fKeys.keyboard.noteOn.connect(self.slot_noteOn)
        #self.fKeys.keyboard.noteOff.connect(self.slot_noteOff)

        # -------------------------------------------------------------
        # Connect actions to functions (part 2)

        #host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        #host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velocity):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOff(note, False)

    # -----------------------------------------------------------------
    # PluginEdit callbacks

    def editDialogNotePressed(self, pluginId, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOn(note, False)

    def editDialogNoteReleased(self, pluginId, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOff(note, False)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        for pluginId in self.fSelectedPlugins:
            self.host.send_midi_note(pluginId, 0, note, 100)

            pedit = self.getPluginEditDialog(pluginId)
            pedit.noteOn(0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        for pluginId in self.fSelectedPlugins:
            self.host.send_midi_note(pluginId, 0, note, 0)

            pedit = self.getPluginEditDialog(pluginId)
            pedit.noteOff(0, note)
