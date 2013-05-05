#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Custom Mini Canvas Preview, a custom Qt4 widget
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
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt4.QtCore import Qt, QRectF, QTimer, SIGNAL, SLOT
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
    def __init__(self, parent):
        QFrame.__init__(self, parent)

        self.fUseCustomPaint = False

        self.fMouseDown = False

        self.fViewBg    = QColor(0, 0, 0)
        self.fViewBrush = QBrush(QColor(75, 75, 255, 30))
        self.fViewPen   = QPen(Qt.blue, 1)

        self.fScale = 1.0
        self.fScene = None
        self.fRealParent = None
        self.fFakeWidth  = 0.0
        self.fFakeHeight = 0.0

        self.fRenderSource = self.getRenderSource()
        self.fRenderTarget = QRectF(0, 0, 0, 0)

        self.fViewPadX = 0.0
        self.fViewPadY = 0.0
        self.fViewRect = [0.0, 0.0, 10.0, 10.0]

    def init(self, scene, realWidth, realHeight, useCustomPaint = False):
        padding = 6

        self.fScene = scene
        self.fFakeWidth  = float(realWidth) / 15
        self.fFakeHeight = float(realHeight) / 15

        self.setMinimumSize(self.fFakeWidth+padding,   self.fFakeHeight+padding)
        self.setMaximumSize(self.fFakeWidth*4+padding, self.fFakeHeight+padding)

        self.fRenderTarget.setWidth(realWidth)
        self.fRenderTarget.setHeight(realHeight)

        if self.fUseCustomPaint != useCustomPaint:
            self.fUseCustomPaint = useCustomPaint
            self.repaint()

    def setRealParent(self, parent):
        self.fRealParent = parent

    def getRenderSource(self):
        xPadding = (float(self.width())  - self.fFakeWidth) / 2.0
        yPadding = (float(self.height()) - self.fFakeHeight) / 2.0
        return QRectF(xPadding, yPadding, self.fFakeWidth, self.fFakeHeight)

    def setViewPosX(self, xp):
        x = self.fFakeWidth*xp
        xRatio = (x / self.fFakeWidth) * self.fViewRect[iWidth] / self.fScale
        self.fViewRect[iX] = x - xRatio + self.fRenderSource.x()
        self.update()

    def setViewPosY(self, yp):
        y = self.fFakeHeight*yp
        yRatio = (y / self.fFakeHeight) * self.fViewRect[iHeight] / self.fScale
        self.fViewRect[iY] = y - yRatio + self.fRenderSource.y()
        self.update()

    def setViewScale(self, scale):
        self.fScale = scale
        QTimer.singleShot(0, self.fRealParent, SLOT("slot_miniCanvasCheckAll()"))

    def setViewSize(self, width, height):
        self.fViewRect[iWidth]  = width  * self.fFakeWidth
        self.fViewRect[iHeight] = height * self.fFakeHeight
        self.update()

    def setViewTheme(self, bgColor, brushColor, penColor):
        brushColor.setAlpha(40)
        penColor.setAlpha(100)
        self.fViewBg    = bgColor
        self.fViewBrush = QBrush(brushColor)
        self.fViewPen   = QPen(penColor, 1)

    def handleMouseEvent(self, eventX, eventY):
        x = float(eventX) - self.fRenderSource.x() - (self.fViewRect[iWidth]  / self.fScale / 2)
        y = float(eventY) - self.fRenderSource.y() - (self.fViewRect[iHeight] / self.fScale / 2)

        maxWidth  = self.fViewRect[iWidth] / self.fScale
        maxHeight = self.fViewRect[iHeight] / self.fScale

        if maxWidth > self.fFakeWidth:
            maxWidth = self.fFakeWidth
        if maxHeight > self.fFakeHeight:
            maxHeight = self.fFakeHeight

        if x < 0.0:
            x = 0.0
        elif x > self.fFakeWidth - maxWidth:
            x = self.fFakeWidth - maxWidth

        if y < 0.0:
            y = 0.0
        elif y > self.fFakeHeight - maxHeight:
            y = self.fFakeHeight - maxHeight

        self.fViewRect[iX] = x + self.fRenderSource.x()
        self.fViewRect[iY] = y + self.fRenderSource.y()
        self.update()

        self.emit(SIGNAL("miniCanvasMoved(double, double)"), x * self.fScale / self.fFakeWidth, y * self.fScale / self.fFakeHeight)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.fMouseDown = True
            self.setCursor(QCursor(Qt.SizeAllCursor))
            self.handleMouseEvent(event.x(), event.y())
        event.accept()

    def mouseMoveEvent(self, event):
        if self.fMouseDown:
            self.handleMouseEvent(event.x(), event.y())
        event.accept()

    def mouseReleaseEvent(self, event):
        if self.fMouseDown:
            self.setCursor(QCursor(Qt.ArrowCursor))
        self.fMouseDown = False
        QFrame.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)

        if self.fUseCustomPaint:
            painter.setBrush(self.fViewBg)
            painter.setPen(QColor(12, 12, 12))
            painter.drawRect(0, 0, self.width(), self.height()-2)

            painter.setBrush(QColor(36, 36, 36))
            painter.setPen(QColor(62, 62, 62))
            painter.drawRect(1, 1, self.width()-2, self.height()-4)

            painter.setBrush(self.fViewBg)
            painter.setPen(self.fViewBg)
            painter.drawRect(2, 3, self.width()-5, self.height()-7)

        else:
            painter.setBrush(self.fViewBg)
            painter.setPen(self.fViewBg)
            painter.drawRoundRect(2, 2, self.width()-6, self.height()-6, 3, 3)

        self.fScene.render(painter, self.fRenderSource, self.fRenderTarget, Qt.KeepAspectRatio)

        maxWidth  = self.fViewRect[iWidth]  / self.fScale
        maxHeight = self.fViewRect[iHeight] / self.fScale

        if maxWidth > self.fFakeWidth:
            maxWidth = self.fFakeWidth
        if maxHeight > self.fFakeHeight:
            maxHeight = self.fFakeHeight

        painter.setBrush(self.fViewBrush)
        painter.setPen(self.fViewPen)
        painter.drawRect(self.fViewRect[iX], self.fViewRect[iY], maxWidth, maxHeight)

        if self.fUseCustomPaint:
            event.accept()
        else:
            QFrame.paintEvent(self, event)

    def resizeEvent(self, event):
        self.fRenderSource = self.getRenderSource()
        if self.fRealParent:
            QTimer.singleShot(0, self.fRealParent, SLOT("slot_miniCanvasCheckAll()"))
        QFrame.resizeEvent(self, event)
