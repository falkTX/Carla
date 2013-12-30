#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LED Button, a custom Qt4 widget
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

from PyQt4.QtCore import qCritical, QRectF
from PyQt4.QtGui import QPainter, QPixmap, QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class LEDButton(QPushButton):
    OFF    = 0
    BLUE   = 1
    GREEN  = 2
    RED    = 3
    YELLOW = 4

    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.fPixmap     = QPixmap()
        self.fPixmapRect = QRectF(0, 0, 0, 0)

        self.setCheckable(True)
        self.setText("")

        self.setColor(self.BLUE)

        self.fLastColor = self.OFF
        self.fPixmap.load(":/bitmaps/led_off.png")

    def setColor(self, color):
        self.fColor = color
        size = 14

        self.fPixmapRect = QRectF(0, 0, size, size)

        self.setMinimumSize(size, size)
        self.setMaximumSize(size, size)

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

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
                else:
                    return

                self.fLastColor = self.fColor

        elif self.fLastColor != self.OFF:
            self.fPixmap.load(":/bitmaps/led_off.png")
            self.fLastColor = self.OFF

        painter.drawPixmap(self.fPixmapRect, self.fPixmap, self.fPixmapRect)
