#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Custom Mini Canvas Preview, a custom Qt4 widget
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
    from PyQt5.QtCore import pyqtSignal, Qt, QRectF, QTimer
    from PyQt5.QtGui import QBrush, QColor, QCursor, QPainter, QPen
    from PyQt5.QtWidgets import QFrame
else:
    from PyQt4.QtCore import pyqtSignal, Qt, QRectF, QTimer
    from PyQt4.QtGui import QBrush, QColor, QCursor, QFrame, QPainter, QPen

# ------------------------------------------------------------------------------------------------------------
# Static Variables

iX = 0
iY = 1
iWidth  = 2
iHeight = 3

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class CanvasPreviewFrame(QFrame):
    miniCanvasMoved = pyqtSignal(float, float)

    # x = 2
    # y = 2
    # w = width-4
    # h = height-3
    # bounds -1 px

    kInternalWidth  = 210-6 # -4 for width + -1px*2 bounds
    kInternalHeight = 162-5 # -3 for height + -1px*2 bounds

    def __init__(self, parent):
        QFrame.__init__(self, parent)

        #self.setMinimumWidth(210)
        #self.setMinimumHeight(162)
        #self.setMaximumHeight(162)

        self.fRealParent = None

        self.fScene = None
        self.fRenderSource   = QRectF(0.0, 0.0, 0.0, 0.0)
        self.fRenderTarget   = QRectF(0.0, 0.0, 0.0, 0.0)
        self.fUseCustomPaint = False

        self.fInitialX = 0.0
        self.fScale    = 1.0

        self.fViewBg    = QColor(0, 0, 0)
        self.fViewBrush = QBrush(QColor(75, 75, 255, 30))
        self.fViewPen   = QPen(Qt.blue, 1)
        self.fViewRect  = [3.0, 3.0, 10.0, 10.0]

        self.fMouseDown = False

    def init(self, scene, realWidth, realHeight, useCustomPaint = False):
        self.fScene = scene
        self.fRenderSource = QRectF(0.0, 0.0, float(realWidth), float(realHeight))

        if self.fUseCustomPaint != useCustomPaint:
            self.fUseCustomPaint = useCustomPaint
            self.repaint()

    def setRealParent(self, parent):
        self.fRealParent = parent

    def setViewPosX(self, xp):
        self.fViewRect[iX] = (xp * self.kInternalWidth) - (xp * (self.fViewRect[iWidth]/self.fScale)) + xp
        self.update()

    def setViewPosY(self, yp):
        self.fViewRect[iY] = (yp * self.kInternalHeight) - (yp * (self.fViewRect[iHeight]/self.fScale)) + yp
        self.update()

    def setViewScale(self, scale):
        self.fScale = scale

        if self.fRealParent is not None:
            QTimer.singleShot(0, self.fRealParent.slot_miniCanvasCheckAll)

    def setViewSize(self, width, height):
        self.fViewRect[iWidth]  = width  * self.kInternalWidth
        self.fViewRect[iHeight] = height * self.kInternalHeight
        self.update()

    def setViewTheme(self, bgColor, brushColor, penColor):
        brushColor.setAlpha(40)
        penColor.setAlpha(100)
        self.fViewBg    = bgColor
        self.fViewBrush = QBrush(brushColor)
        self.fViewPen   = QPen(penColor, 1)

    def handleMouseEvent(self, eventX, eventY):
        x = float(eventX) - self.fInitialX
        y = float(eventY) - 3.0

        if x < 0.0:
            x = 0.0
        elif x > self.kInternalWidth:
            x = float(self.kInternalWidth)

        if y < 0.0:
            y = 0.0
        elif y > self.kInternalHeight:
            y = float(self.kInternalHeight)

        xp = x/self.kInternalWidth
        yp = y/self.kInternalHeight

        self.fViewRect[iX] = x - (xp * self.fViewRect[iWidth]/self.fScale) + xp
        self.fViewRect[iY] = y - (yp * self.fViewRect[iHeight]/self.fScale) + yp
        self.update()

        self.miniCanvasMoved.emit(xp, yp)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.fMouseDown = True
            self.setCursor(QCursor(Qt.SizeAllCursor))
            self.handleMouseEvent(event.x(), event.y())
            return event.accept()
        QFrame.mouseMoveEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.fMouseDown:
            self.handleMouseEvent(event.x(), event.y())
            return event.accept()
        QFrame.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.fMouseDown:
            self.setCursor(QCursor(Qt.ArrowCursor))
            self.fMouseDown = False
            return event.accept()
        QFrame.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)

        if self.fUseCustomPaint:
            painter.setBrush(QColor(36, 36, 36))
            painter.setPen(QColor(62, 62, 62))
            painter.drawRect(2, 2, self.width()-4, self.height()-4)

            painter.setBrush(self.fViewBg)
            painter.setPen(self.fViewBg)
            painter.drawRect(3, 3, self.width()-6, self.height()-6)
        else:
            painter.setBrush(self.fViewBg)
            painter.setPen(self.fViewBg)
            painter.drawRoundedRect(3, 3, self.width()-6, self.height()-6, 3, 3)

        self.fScene.render(painter, self.fRenderTarget, self.fRenderSource, Qt.KeepAspectRatio)

        width  = self.fViewRect[iWidth]/self.fScale
        height = self.fViewRect[iHeight]/self.fScale

        if width > self.kInternalWidth:
            width = self.kInternalWidth
        if height > self.kInternalHeight:
            height = self.kInternalHeight

        # cursor
        painter.setBrush(self.fViewBrush)
        painter.setPen(self.fViewPen)
        painter.drawRect(self.fViewRect[iX]+self.fInitialX, self.fViewRect[iY]+3, width, height)

        if self.fUseCustomPaint:
            event.accept()
        else:
            QFrame.paintEvent(self, event)

    def resizeEvent(self, event):
        self.fInitialX     = float(self.width())/2.0 - float(self.kInternalWidth)/2.0
        self.fRenderTarget = QRectF(self.fInitialX, 3.0, float(self.kInternalWidth), float(self.kInternalHeight))

        if self.fRealParent is not None:
            QTimer.singleShot(0, self.fRealParent.slot_miniCanvasCheckAll)

        QFrame.resizeEvent(self, event)
