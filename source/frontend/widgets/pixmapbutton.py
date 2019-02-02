#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Button, a custom Qt widget
# Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import QPointF, QRectF
from PyQt5.QtGui import QColor, QFont, QPainter, QPixmap
from PyQt5.QtWidgets import QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class PixmapButton(QPushButton):
    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.fPixmapNormal = QPixmap()
        self.fPixmapDown   = QPixmap()
        self.fPixmapHover  = QPixmap()
        self.fPixmapRect   = QRectF(0, 0, 0, 0)

        self.fIsHovered = False

        self.fTopText      = ""
        self.fTopTextColor = QColor()
        self.fTopTextFont  = QFont()

        self.setText("")

    def setPixmaps(self, normal, down, hover):
        self.fPixmapNormal.load(normal)
        self.fPixmapDown.load(down)
        self.fPixmapHover.load(hover)

        width  = self.fPixmapNormal.width()
        height = self.fPixmapNormal.height()

        self.fPixmapRect = QRectF(0, 0, width, height)

        self.setMinimumSize(width, height)
        self.setMaximumSize(width, height)

    def setTopText(self, text, color, font):
        self.fTopText      = text
        self.fTopTextColor = color
        self.fTopTextFont  = font

    def enterEvent(self, event):
        self.fIsHovered = True
        QPushButton.enterEvent(self, event)

    def leaveEvent(self, event):
        self.fIsHovered = False
        QPushButton.leaveEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        if not self.isEnabled():
            painter.save()
            painter.setOpacity(0.2)
            painter.drawPixmap(self.fPixmapRect, self.fPixmapNormal, self.fPixmapRect)
            painter.restore()

        elif self.isChecked() or self.isDown():
            painter.drawPixmap(self.fPixmapRect, self.fPixmapDown, self.fPixmapRect)

        elif self.fIsHovered:
            painter.drawPixmap(self.fPixmapRect, self.fPixmapHover, self.fPixmapRect)

        else:
            painter.drawPixmap(self.fPixmapRect, self.fPixmapNormal, self.fPixmapRect)

        if not self.fTopText:
            return

        painter.save()
        painter.setPen(self.fTopTextColor)
        painter.setBrush(self.fTopTextColor)
        painter.setFont(self.fTopTextFont)
        painter.drawText(QPointF(10, 16), self.fTopText)
        painter.restore()
