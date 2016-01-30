# -*- coding: utf-8 -*-

# LED Button, a custom Qt4 widget
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
    from PyQt5.QtCore import QRectF
    from PyQt5.QtGui import QPainter, QPixmap
    from PyQt5.QtWidgets import QPushButton
else:
    from PyQt4.QtCore import QRectF
    from PyQt4.QtGui import QPainter, QPixmap, QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class LEDButton(QPushButton):
    # normal
    OFF    = 0
    BLUE   = 1
    GREEN  = 2
    RED    = 3
    YELLOW = 4
    # extra
    CALF   = 5

    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.fPixmap     = QPixmap()
        self.fPixmapRect = QRectF(0, 0, 0, 0)

        self.setCheckable(True)
        self.setText("")

        self.fLastColor = self.OFF
        self.fPixmap.load(":/bitmaps/led_off.png")

        self.setColor(self.BLUE)

    def setColor(self, color):
        self.fColor = color
        self._loadPixmapNow()

        # Fix calf off
        if color == self.CALF:
            self.fPixmap.load(":/bitmaps/led_calf_off.png")

        size = self.fPixmap.width()
        self.fPixmapRect = QRectF(0, 0, size, size)

        self.setMinimumSize(size, size)
        self.setMaximumSize(size, size)

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        self._loadPixmapNow()

        painter.drawPixmap(self.fPixmapRect, self.fPixmap, self.fPixmapRect)

    def _loadPixmapNow(self):

        if self.isChecked():
            if self.fLastColor != self.fColor:
                if self.fColor == self.OFF:
                    self.fPixmap.load(":/bitmaps/led_off.png")
                elif self.fColor == self.BLUE:
                    self.fPixmap.load(":/bitmaps/led_blue.png")
                elif self.fColor == self.GREEN:
                    self.fPixmap.load(":/bitmaps/led_green.png")
                elif self.fColor == self.RED:
                    self.fPixmap.load(":/bitmaps/led_red.png")
                elif self.fColor == self.YELLOW:
                    self.fPixmap.load(":/bitmaps/led_yellow.png")
                elif self.fColor == self.CALF:
                    self.fPixmap.load(":/bitmaps/led_calf_on.png")
                else:
                    return

                self.fLastColor = self.fColor

        elif self.fLastColor != self.OFF:
            self.fPixmap.load(":/bitmaps/led_calf_off.png" if self.fColor == self.CALF else ":/bitmaps/led_off.png")
            self.fLastColor = self.OFF
