#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import QRectF
    from PyQt5.QtGui import QPainter, QPixmap
    from PyQt5.QtSvg import QSvgWidget
    from PyQt5.QtWidgets import QPushButton
elif qt_config == 6:
    from PyQt6.QtCore import QRectF
    from PyQt6.QtGui import QPainter, QPixmap
    from PyQt6.QtSvgWidgets import QSvgWidget
    from PyQt6.QtWidgets import QPushButton

# ---------------------------------------------------------------------------------------------------------------------
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
            #if isinstance(self.fImage, QPixmap):
                #size = self.fImage.width()
            #else:
                #size = self.fImage.sizeHint().width()

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
                return False
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

# ---------------------------------------------------------------------------------------------------------------------
