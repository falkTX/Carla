#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# LED Button, a custom Qt widget
# Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QPainter, QPixmap
from PyQt5.QtSvg import QSvgWidget
from PyQt5.QtWidgets import QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class LEDButton(QPushButton):
    # unset
    UNSET  = -1
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

        self.fLastColor = self.UNSET
        self.fColor     = self.UNSET

        self.fImage = QSvgWidget()
        self.fImage.load(":/scalable/led_off.svg")
        self.fRect = QRectF(self.fImage.rect())

        self.setCheckable(True)
        self.setText("")

        self.setColor(self.BLUE)

    def setColor(self, color):
        self.fColor = color

        if color == self.CALF:
            self.fLastColor = self.UNSET

        if self._loadImageNowIfNeeded():
            if isinstance(self.fImage, QPixmap):
                size = self.fImage.width()
            else:
                size = self.fImage.sizeHint().width()

            self.fRect = QRectF(self.fImage.rect())
            self.setFixedSize(self.fImage.size())

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        self._loadImageNowIfNeeded()

        if isinstance(self.fImage, QPixmap):
            size = self.fImage.width()
            rect = QRectF(0, 0, size, size)
            painter.drawPixmap(rect, self.fImage, rect)
        else:
            size = self.fImage.sizeHint().width()
            rect = QRectF(0, 0, size, size)
            self.fImage.renderer().render(painter, rect)

    def _loadImageNowIfNeeded(self):
        if self.isChecked():
            if self.fLastColor == self.fColor:
                return
            if self.fColor == self.OFF:
                img = ":/scalable/led_off.svg"
            elif self.fColor == self.BLUE:
                img = ":/scalable/led_blue.svg"
            elif self.fColor == self.GREEN:
                img = ":/scalable/led_green.svg"
            elif self.fColor == self.RED:
                img = ":/scalable/led_red.svg"
            elif self.fColor == self.YELLOW:
                img = ":/scalable/led_yellow.svg"
            elif self.fColor == self.CALF:
                img = ":/bitmaps/led_calf_on.png"
            else:
                return False
            self.fLastColor = self.fColor

        elif self.fLastColor != self.OFF:
            img = ":/bitmaps/led_calf_off.png" if self.fColor == self.CALF else ":/scalable/led_off.svg"
            self.fLastColor = self.OFF

        else:
            return False

        if img.endswith(".png"):
            if not isinstance(self.fImage, QPixmap):
                self.fImage = QPixmap()
        else:
            if not isinstance(self.fImage, QSvgWidget):
                self.fImage = QSvgWidget()

        self.fImage.load(img)
        self.update()

        return True
