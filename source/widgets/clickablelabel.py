#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Clickable Label, a custom Qt4 widget
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt4.QtCore import pyqtSlot, Qt, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QLabel

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class ClickableLabel(QLabel):
    def __init__(self, parent):
        QLabel.__init__(self, parent)

        self.setCursor(Qt.PointingHandCursor)

    def mousePressEvent(self, event):
        self.emit(SIGNAL("clicked()"))
        # Use busy cursor for 2 secs
        self.setCursor(Qt.WaitCursor)
        QTimer.singleShot(2000, self, SLOT("slot_setNormalCursor()"))
        QLabel.mousePressEvent(self, event)

    @pyqtSlot()
    def slot_setNormalCursor(self):
        self.setCursor(Qt.PointingHandCursor)
