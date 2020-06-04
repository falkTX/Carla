#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Scalable Button, a custom Qt widget
# Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
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
from PyQt5.QtSvg import QSvgWidget
from PyQt5.QtWidgets import QPushButton

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class ScalableButton(QPushButton):
    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.fImageNormal  = None
        self.fImageDown    = None
        self.fImageHover   = None
        self.fImageRect    = QRectF()
        self.fIsHovered    = False
        self.fTopText      = ""
        self.fTopTextColor = QColor()
        self.fTopTextFont  = QFont()
        self.setText("")

    def setPixmaps(self, normal, down, hover):
        self.fImageNormal = QPixmap()
        self.fImageDown   = QPixmap()
        self.fImageHover  = QPixmap()
        self._loadImages(normal, down, hover)

    def setSvgs(self, normal, down, hover):
        self.fImageNormal = QSvgWidget()
        self.fImageDown   = QSvgWidget()
        self.fImageHover  = QSvgWidget()
        self._loadImages(normal, down, hover)

    def _loadImages(self, normal, down, hover):
        self.fImageNormal.load(normal)
        self.fImageDown.load(down)
        self.fImageHover.load(hover)

        if isinstance(self.fImageNormal, QPixmap):
            width  = self.fImageNormal.width()
            height = self.fImageNormal.height()
        else:
            width  = self.fImageNormal.sizeHint().width()
            height = self.fImageNormal.sizeHint().height()

        self.fImageRect = QRectF(0, 0, width, height)

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

        if self.fImageNormal is None:
            return

        if isinstance(self.fImageNormal, QPixmap):
            self.paintEventPixmap(painter)
        else:
            self.paintEventSvg(painter)

        if not self.fTopText:
            return

        painter.save()
        painter.setPen(self.fTopTextColor)
        painter.setBrush(self.fTopTextColor)
        painter.setFont(self.fTopTextFont)
        painter.drawText(QPointF(10, 16), self.fTopText)
        painter.restore()

    def paintEventPixmap(self, painter):
        if not self.isEnabled():
            painter.save()
            painter.setOpacity(0.2)
            painter.drawPixmap(self.fImageRect, self.fImageNormal, self.fImageRect)
            painter.restore()

        elif self.isChecked() or self.isDown():
            painter.drawPixmap(self.fImageRect, self.fImageDown, self.fImageRect)

        elif self.fIsHovered:
            painter.drawPixmap(self.fImageRect, self.fImageHover, self.fImageRect)

        else:
            painter.drawPixmap(self.fImageRect, self.fImageNormal, self.fImageRect)

    def paintEventSvg(self, painter):
        if not self.isEnabled():
            painter.save()
            painter.setOpacity(0.2)
            self.fImageNormal.renderer().render(painter, self.fImageRect)
            painter.restore()

        elif self.isChecked() or self.isDown():
            self.fImageDown.renderer().render(painter, self.fImageRect)

        elif self.fIsHovered:
            self.fImageHover.renderer().render(painter, self.fImageRect)

        else:
            self.fImageNormal.renderer().render(painter, self.fImageRect)
