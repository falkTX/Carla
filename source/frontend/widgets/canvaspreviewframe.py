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

from math import floor

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, Qt, QRectF, QTimer
    from PyQt5.QtGui import QBrush, QColor, QCursor, QPainter, QPainterPath, QPen
    from PyQt5.QtWidgets import QFrame
else:
    from PyQt4.QtCore import pyqtSignal, Qt, QRectF, QTimer
    from PyQt4.QtGui import QBrush, QColor, QCursor, QFrame, QPainter, QPainterPath, QPen

# ------------------------------------------------------------------------------------------------------------
# Antialiasing settings

from patchcanvas import options, ANTIALIASING_FULL

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
        self.fFrameWidth = 0.0

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
        self.fFrameWidth = 2 if useCustomPaint else self.frameWidth()

        if self.fUseCustomPaint != useCustomPaint:
            self.fUseCustomPaint = useCustomPaint
            self.repaint()

    def setRealParent(self, parent):
        self.fRealParent = parent

    def setViewPosX(self, xp):
        self.fViewRect[iX] = xp * (self.kInternalWidth - self.fViewRect[iWidth]/self.fScale)
        self.update()

    def setViewPosY(self, yp):
        self.fViewRect[iY] = yp * (self.kInternalHeight - self.fViewRect[iHeight]/self.fScale)
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

        self.fViewRect[iX] = xp * (self.kInternalWidth - self.fViewRect[iWidth]/self.fScale)
        self.fViewRect[iY] = yp * (self.kInternalHeight - self.fViewRect[iHeight]/self.fScale)
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
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))

        # Brightness-aware out-of-canvas shading
        bg_color = self.fViewBg
        bg_black = bg_color.black()
        bg_shade = -12 if bg_black < 127 else 12
        r,g,b,a = bg_color.getRgb()
        bg_color = QColor(r+bg_shade, g+bg_shade, b+bg_shade)

        frameWidth = self.fFrameWidth
        if self.fUseCustomPaint:
            # Shadow
            painter.setPen(QColor(0,0,0,100))
            painter.setBrush(Qt.transparent)
            painter.drawRect(QRectF(0.5, 0.5, self.width()-1, self.height()-1))

            # Background
            painter.setBrush(bg_color)
            painter.setPen(bg_color)
            painter.drawRect(QRectF(1.5, 1.5, self.width()-3, self.height()-3))

            # Edge (overlay)
            painter.setBrush(Qt.transparent)
            painter.setPen(QColor(255,255,255,62))
            painter.drawRect(QRectF(1.5, 1.5, self.width()-3, self.height()-3))
        else:
            use_rounding = int(frameWidth > 1)

            rounding = 1 * use_rounding
            painter.setBrush(bg_color)
            painter.setPen(bg_color)
            painter.drawRoundedRect(QRectF(0.5+frameWidth, 0.5+frameWidth, self.width()-1-frameWidth*2, self.height()-1-frameWidth*2), rounding, rounding)

            clipPath = QPainterPath()
            rounding = 2 * use_rounding
            clipPath.addRoundedRect(QRectF(frameWidth, frameWidth, self.width()-frameWidth*2, self.height()-frameWidth*2), rounding, rounding)
            painter.setClipPath(clipPath)

        self.fScene.render(painter, self.fRenderTarget, self.fRenderSource, Qt.KeepAspectRatio)

        # Allow cursor frame to look joined with minicanvas frame
        painter.setClipping(False)

        width  = self.fViewRect[iWidth]/self.fScale
        height = self.fViewRect[iHeight]/self.fScale

        # cursor
        painter.setBrush(self.fViewBrush)
        painter.setPen(self.fViewPen)
        lineHinting = painter.pen().widthF() / 2
        painter.drawRect(QRectF(
            floor(self.fViewRect[iX]+self.fInitialX)+lineHinting,
            floor(self.fViewRect[iY]+3)+lineHinting,
            round(width)-1, round(height)-1))

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
