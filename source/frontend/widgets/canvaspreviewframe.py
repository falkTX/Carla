#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Custom Mini Canvas Preview, a custom Qt widget
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

from math import floor, ceil

from PyQt5.QtCore import pyqtSignal, Qt, QRectF, QTimer, QEvent, QPoint
from PyQt5.QtGui import QBrush, QColor, QCursor, QPainter, QPainterPath, QPen, QCursor, QPixmap
from PyQt5.QtWidgets import QFrame, QWidget

# ------------------------------------------------------------------------------------------------------------
# Antialiasing settings

from patchcanvas import options, ANTIALIASING_FULL

# ------------------------------------------------------------------------------------------------------------
# Static Variables

iX = 0
iY = 1
iWidth  = 2
iHeight = 3

MOUSE_MODE_NONE = 0
MOUSE_MODE_MOVE = 1
MOUSE_MODE_SCALE = 2

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
    kInternalRatio = kInternalWidth / kInternalHeight

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
        self.fInitialY = 0.0
        self.fScale    = 1.0

        self.fViewBg    = QColor(0, 0, 0)
        self.fViewBrush = QBrush(QColor(75, 75, 255, 30))
        self.fViewPen   = QPen(Qt.blue, 1)
        self.fViewRect  = [3.0, 3.0, 10.0, 10.0]

        self.fMouseMode = MOUSE_MODE_NONE
        self.fMouseLeftDown = False
        self.fMouseRightDown = False
        self.fMousePos = None

    def init(self, scene, realWidth, realHeight, useCustomPaint = False):
        realWidth,realHeight = float(realWidth),float(realHeight)
        self.fScene = scene
        self.fRenderSource = QRectF(0.0, 0.0, realWidth, realHeight)
        self.kInternalRatio = realWidth / realHeight
        self.updateStyle()

        if self.fUseCustomPaint != useCustomPaint:
            self.fUseCustomPaint = useCustomPaint
            self.repaint()

    def updateStyle(self):
        self.fFrameWidth = 1 if self.fUseCustomPaint else self.frameWidth()

    def changeEvent(self, event):
        if event.type() in (QEvent.StyleChange, QEvent.PaletteChange):
            self.updateStyle()
        QWidget.changeEvent(self, event)

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
        bg_black = bgColor.blackF()
        brush_black = brushColor.blackF()
        r0,g0,b0,a = bgColor.getRgb()
        r1,g1,b1,a = brushColor.getRgb()
        if brush_black < bg_black:
            self.fRubberBandBlending = 1
            brushColor = QColor(r1-r0, g1-g0, b1-b0, 40)
        elif bg_black < brush_black:
            self.fRubberBandBlending = -1
            brushColor = QColor(r0-r1, g0-g1, b0-b1, 40)
        else:
            bgColor.setAlpha(40)
            self.fRubberBandBlending = 0
        penColor.setAlpha(100)
        self.fViewBg    = bgColor
        self.fViewBrush = QBrush(brushColor)
        self.fViewPen   = QPen(penColor, 1)

        cur_color = "black" if bg_black < 0.5 else "white"
        self.fScaleCursors = ( QCursor(QPixmap(":/cursors/zoom-generic-"+cur_color+".png"), 8, 7),
                               QCursor(QPixmap(":/cursors/zoom-in-"+cur_color+".png"), 8, 7),
                               QCursor(QPixmap(":/cursors/zoom-out-"+cur_color+".png"), 8, 7) )

    def moveViewRect(self, x, y):
        x = float(x) - self.fInitialX
        y = float(y) - self.fInitialY

        fixPos = False
        rCentX = self.fViewRect[iWidth] / self.fScale / 2
        rCentY = self.fViewRect[iHeight] / self.fScale / 2
        if x < rCentX:
            x = rCentX
            fixPos = True
        elif x > self.kInternalWidth - rCentX:
            x = self.kInternalWidth - rCentX
            fixPos = True

        if y < rCentY:
            y = rCentY
            fixPos = True
        elif y > self.kInternalHeight - rCentY:
            y = self.kInternalHeight - rCentY
            fixPos = True

        if fixPos:
            globalPos = self.mapToGlobal(QPoint(self.fInitialX + x, self.fInitialY + y))
            self.cursor().setPos(globalPos)

        x = self.fRenderSource.width() * x / self.kInternalWidth
        y = self.fRenderSource.height() * y / self.kInternalHeight
        self.fScene.m_view.centerOn(x, y)

    def updateMouseMode(self, event=None):
        if self.fMouseLeftDown and self.fMouseRightDown:
            self.fMousePos = event.globalPos()
            self.setCursor(self.fScaleCursors[0])
            self.fMouseMode = MOUSE_MODE_SCALE
        elif self.fMouseLeftDown:
            self.setCursor(QCursor(Qt.SizeAllCursor))
            if self.fMouseMode == MOUSE_MODE_NONE:
                self.moveViewRect(event.x(), event.y())
            self.fMouseMode = MOUSE_MODE_MOVE
        else:
            self.unsetCursor()
            self.fMouseMode = MOUSE_MODE_NONE

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.fMouseLeftDown = True
            self.updateMouseMode(event)
            return event.accept()
        elif event.button() == Qt.RightButton:
            self.fMouseRightDown = True
            self.updateMouseMode(event)
            return event.accept()
        elif event.button() == Qt.MidButton:
            self.fMouseLeftDown = True
            self.fMouseRightDown = True
            self.updateMouseMode(event)
            return event.accept()
        QFrame.mouseMoveEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.fMouseMode == MOUSE_MODE_MOVE:
            self.moveViewRect(event.x(), event.y())
            return event.accept()
        if self.fMouseMode == MOUSE_MODE_SCALE:
            dy = self.fMousePos.y() - event.globalY()
            if dy != 0:
                self.setCursor(self.fScaleCursors[1 if dy > 0 else 2])
                self.fScene.zoom_wheel(dy)
            self.cursor().setPos(self.fMousePos)
        QFrame.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.fMouseLeftDown = False
            self.updateMouseMode()
            return event.accept()
        elif event.button() == Qt.RightButton:
            self.fMouseRightDown = False
            self.updateMouseMode(event)
            return event.accept()
        elif event.button() == Qt.MidButton:
            self.fMouseLeftDown = event.buttons() & Qt.LeftButton
            self.fMouseRightDown = event.buttons() & Qt.RightButton
            self.updateMouseMode(event)
            return event.accept()
        QFrame.mouseReleaseEvent(self, event)

    def wheelEvent(self, event):
        self.fScene.zoom_wheel(event.angleDelta().y())
        return event.accept()

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
            # Inner shadow
            color = QColor.fromHsv(40, 0, 255-max(210, bg_color.black(), bg_black))
            painter.setBrush(Qt.transparent)
            painter.setPen(color)
            painter.drawRect(QRectF(0.5, 0.5, self.width()-1, self.height()-1))

            # Background
            painter.setBrush(bg_color)
            painter.setPen(bg_color)
            painter.drawRect(QRectF(1.5, 1.5, self.width()-3, self.height()-3))
        else:
            use_rounding = int(frameWidth > 1)

            rounding = 0.5 * use_rounding
            painter.setBrush(bg_color)
            painter.setPen(bg_color)
            painter.drawRoundedRect(QRectF(0.5+frameWidth, 0.5+frameWidth, self.width()-1-frameWidth*2, self.height()-1-frameWidth*2), rounding, rounding)

            clipPath = QPainterPath()
            rounding = 1.0 * use_rounding
            clipPath.addRoundedRect(QRectF(frameWidth, frameWidth, self.width()-frameWidth*2, self.height()-frameWidth*2), rounding, rounding)
            painter.setClipPath(clipPath)

        self.fScene.render(painter, self.fRenderTarget, self.fRenderSource, Qt.KeepAspectRatio)

        # Allow cursor frame to look joined with minicanvas frame
        painter.setClipping(False)

        width  = self.fViewRect[iWidth]/self.fScale
        height = self.fViewRect[iHeight]/self.fScale

        # cursor
        lineHinting = self.fViewPen.widthF() / 2
        x = self.fViewRect[iX]+self.fInitialX
        y = self.fViewRect[iY]+self.fInitialY
        scr_x = floor(x)
        scr_y = floor(y)
        rect = QRectF(
            scr_x+lineHinting,
            scr_y+lineHinting,
            ceil(width+x-scr_x)-lineHinting*2,
            ceil(height+y-scr_y)-lineHinting*2 )

        if self.fRubberBandBlending == 1:
            painter.setCompositionMode(QPainter.CompositionMode_Plus)
        elif self.fRubberBandBlending == -1:
            painter.setCompositionMode(QPainter.CompositionMode_Difference)
        painter.setBrush(self.fViewBrush)
        painter.setPen(Qt.NoPen)
        painter.drawRect(rect)

        painter.setCompositionMode(QPainter.CompositionMode_SourceOver)
        painter.setBrush(Qt.NoBrush)
        painter.setPen(self.fViewPen)
        painter.drawRect(rect)

        if self.fUseCustomPaint:
            event.accept()
        else:
            QFrame.paintEvent(self, event)

    def resizeEvent(self, event):
        size = event.size()
        width = size.width()
        height = size.height()
        extRatio = (width - self.fFrameWidth * 2) / (height - self.fFrameWidth * 2)
        if extRatio >= self.kInternalRatio:
            self.kInternalHeight = floor(height - self.fFrameWidth * 2)
            self.kInternalWidth  = floor(self.kInternalHeight * self.kInternalRatio)
            self.fInitialX       = floor(float(width - self.kInternalWidth) / 2.0)
            self.fInitialY       = self.fFrameWidth
        else:
            self.kInternalWidth  = floor(width - self.fFrameWidth * 2)
            self.kInternalHeight = floor(self.kInternalWidth / self.kInternalRatio)
            self.fInitialX       = self.fFrameWidth
            self.fInitialY       = floor(float(height - self.kInternalHeight) / 2.0)

        self.fRenderTarget = QRectF(self.fInitialX, self.fInitialY, float(self.kInternalWidth), float(self.kInternalHeight))

        if self.fRealParent is not None:
            QTimer.singleShot(0, self.fRealParent.slot_miniCanvasCheckAll)

        QFrame.resizeEvent(self, event)
