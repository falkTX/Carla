#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Svg Button, a custom Qt widget
# Copyright (C) 2020 Filipe Coelho <falktx@falktx.com>
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
from PyQt5.QtGui import QColor, QFont, QPainter
from PyQt5.QtSvg import QSvgWidget
from PyQt5.QtWidgets import QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class


class ScalableButton(QPushButton):
    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.fSvgNormal = QSvgWidget()
        self.fSvgDown   = QSvgWidget()
        self.fSvgHover  = QSvgWidget()

        self.fIsHovered = False

        self.fTopText      = ""
        self.fTopTextColor = QColor()
        self.fTopTextFont  = QFont()

        self.setText("")

    def setSvgs(self, normal, down, hover):
        self.fSvgNormal.load(normal)
        self.fSvgDown.load(down)
        self.fSvgHover.load(hover)

        width  = self.fSvgNormal.sizeHint().width()
        height = self.fSvgNormal.sizeHint().height()

        self.fSvgRect = QRectF(0, 0, width, height)

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
            self.fSvgNormal.renderer().render(painter, self.fSvgRect)
            painter.restore()

        elif self.isChecked() or self.isDown():
            self.fSvgDown.renderer().render(painter, self.fSvgRect)

        elif self.fIsHovered:
            self.fSvgHover.renderer().render(painter, self.fSvgRect)
        else:
            self.fSvgNormal.renderer().render(painter, self.fSvgRect)
        if not self.fTopText:
            return

        painter.save()
        painter.setPen(self.fTopTextColor)
        painter.setBrush(self.fTopTextColor)
        painter.setFont(self.fTopTextFont)
        painter.drawText(QPointF(10, 16), self.fTopText)
        painter.restore()
