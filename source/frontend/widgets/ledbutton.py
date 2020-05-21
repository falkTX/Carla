#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LED Button, a custom Qt widget
# Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtSvg import QSvgWidget
from PyQt5.QtWidgets import QPushButton

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

        self.fSvgWidget = QSvgWidget(self)

        self.setCheckable(True)
        self.setText("")

        self.fLastColor = self.OFF
        self.fSvgWidget.load(":/scalable/led_off.svg")

        self.setColor(self.BLUE)

    def setColor(self, color):
        self.fColor = color
        self._loadSVGNow()

        # Fix calf off
        if color == self.CALF:
            self.fSvgWidget.load(":/bitmaps/led_calf_off.png")

    def paintEvent(self, event):
        event.accept()

        self._loadSVGNow()

    def _loadSVGNow(self):

        if self.isChecked():
            if self.fLastColor != self.fColor:
                if self.fColor == self.OFF:
                    self.fSvgWidget.load(":/scalable/led_off.svg")
                elif self.fColor == self.BLUE:
                    self.fSvgWidget.load(":/scalable/led_blue.svg")
                elif self.fColor == self.GREEN:
                    self.fSvgWidget.load(":/scalable/led_green.svg")
                elif self.fColor == self.RED:
                    self.fSvgWidget.load(":/scalable/led_red.svg")
                elif self.fColor == self.YELLOW:
                    self.fSvgWidget.load(":/scalable/led_yellow.svg")
                elif self.fColor == self.CALF:
                    self.fSvgWidget.load(":/bitmaps/led_calf_on.png")
                else:
                    return

                self.fLastColor = self.fColor

        elif self.fLastColor != self.OFF:
            self.fSvgWidget.load(":/scalable/led_calf_off.png" if self.fColor == self.CALF else ":/scalable/led_off.svg")
            self.fLastColor = self.OFF
