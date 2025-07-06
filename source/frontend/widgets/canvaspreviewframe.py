#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import floor, ceil

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, QT_VERSION, Qt, QRectF, QTimer, QEvent, QPoint
    from PyQt5.QtGui import QBrush, QColor, QCursor, QPainter, QPainterPath, QPen, QPixmap
    from PyQt5.QtWidgets import QFrame, QGraphicsScene
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, QT_VERSION, Qt, QRectF, QTimer, QEvent, QPoint
    from PyQt6.QtGui import QBrush, QColor, QCursor, QPainter, QPainterPath, QPen, QPixmap
    from PyQt6.QtWidgets import QFrame, QGraphicsScene

# ---------------------------------------------------------------------------------------------------------------------
# Antialiasing settings

from patchcanvas import options, ANTIALIASING_FULL

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class CanvasPreviewFrame(QFrame):
    miniCanvasMoved = pyqtSignal(float, float)

    # x = 2
    # y = 2
    # w = width - 4
    # h = height - 3
    # bounds -1 px

    _kRectX = 0
    _kRectY = 1
    _kRectWidth = 2
    _kRectHeight = 3

    _kCursorName = 0
    _kCursorZoom = 1
    _kCursorZoomIn = 2
    _kCursorZoomOut = 3
    _kCursorZoomCount = 4

    _MOUSE_MODE_NONE = 0
    _MOUSE_MODE_MOVE = 1
    _MOUSE_MODE_SCALE = 2

    _RUBBERBAND_BLENDING_DEFAULT = 0
    _RUBBERBAND_BLENDING_PLUS = 1
    _RUBBERBAND_BLENDING_DIFF = 2

    # -----------------------------------------------------------------------------------------------------------------

    def __init__(self, parent):
        QFrame.__init__(self, parent)

        #self.setMinimumWidth(210)
        #self.setMinimumHeight(162)
        #self.setMaximumHeight(162)

        self.fRealParent = None
        self.fInternalWidth  = 210-6 # -4 for width + -1px*2 bounds
        self.fInternalHeight = 162-5 # -3 for height + -1px*2 bounds
        self.fInternalRatio  = self.fInternalWidth / self.fInternalHeight

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

        self.fMouseMode = self._MOUSE_MODE_NONE
        self.fMouseLeftDown = False
        self.fMouseRightDown = False
        self.fMouseInitialZoomPos = None

        self.fRubberBandBlending = self._RUBBERBAND_BLENDING_DEFAULT

        self.fZoomCursors = [None for _ in range(self._kCursorZoomCount)]
        self.fZoomCursors[self._kCursorName] = "black"
        self.fZoomCursors[self._kCursorZoom] = QCursor(QPixmap(":/cursors/zoom_black.png"), 8, 7)
        self.fZoomCursors[self._kCursorZoomIn] = QCursor(QPixmap(":/cursors/zoom-in_black.png"), 8, 7)
        self.fZoomCursors[self._kCursorZoomOut] = QCursor(QPixmap(":/cursors/zoom-out_black.png"), 8, 7)

    def init(self, scene: QGraphicsScene, realWidth: float, realHeight: float, useCustomPaint: bool = False):
        self.fScene = scene
        self.fRenderSource = QRectF(0.0, 0.0, realWidth, realHeight)
        self.fInternalRatio = realWidth / realHeight
        self._updateStyle()

        if self.fUseCustomPaint != useCustomPaint:
            self.fUseCustomPaint = useCustomPaint
            self.repaint()

    def setRealParent(self, parent):
        self.fRealParent = parent

    # -----------------------------------------------------------------------------------------------------------------

    def setViewPosX(self, xp: float):
        self.fViewRect[self._kRectX] = xp * (self.fInternalWidth - self.fViewRect[self._kRectWidth]/self.fScale)
        self.update()

    def setViewPosY(self, yp: float):
        self.fViewRect[self._kRectY] = yp * (self.fInternalHeight - self.fViewRect[self._kRectHeight]/self.fScale)
        self.update()

    def setViewScale(self, scale: float):
        self.fScale = scale

        if self.fRealParent is not None:
            QTimer.singleShot(0, self.fRealParent.slot_miniCanvasCheckAll)

    def setViewSize(self, width: float, height: float):
        self.fViewRect[self._kRectWidth] = width * self.fInternalWidth
        self.fViewRect[self._kRectHeight] = height * self.fInternalHeight
        self.update()

    def setViewTheme(self, bgColor, brushColor, penColor):
        bg_black      = bgColor.blackF()
        brush_black   = brushColor.blackF()
        r0, g0, b0, _ = bgColor.getRgb()
        r1, g1, b1, _ = brushColor.getRgb()

        if brush_black < bg_black:
            self.fRubberBandBlending = self._RUBBERBAND_BLENDING_PLUS
            brushColor = QColor(r1-r0, g1-g0, b1-b0, 40)
        elif bg_black < brush_black:
            self.fRubberBandBlending = self._RUBBERBAND_BLENDING_DIFF
            brushColor = QColor(r0-r1, g0-g1, b0-b1, 40)
        else:
            bgColor.setAlpha(40)
            self.fRubberBandBlending = self._RUBBERBAND_BLENDING_DEFAULT

        penColor.setAlpha(100)
        self.fViewBg    = bgColor
        self.fViewBrush = QBrush(brushColor)
        self.fViewPen   = QPen(penColor, 1)

        cursorName = "black" if bg_black < 0.5 else "white"
        if self.fZoomCursors[self._kCursorName] != cursorName:
            prefix = ":/cursors/zoom"
            self.fZoomCursors[self._kCursorName] = cursorName
            self.fZoomCursors[self._kCursorZoom] = QCursor(QPixmap(f"{prefix}_{cursorName}.png"), 8, 7)
            self.fZoomCursors[self._kCursorZoomIn] = QCursor(QPixmap(f"{prefix}-in_{cursorName}.png"), 8, 7)
            self.fZoomCursors[self._kCursorZoomOut] = QCursor(QPixmap(f"{prefix}-out_{cursorName}.png"), 8, 7)

    # -----------------------------------------------------------------------------------------------------------------

    def changeEvent(self, event):
        if event.type() in (QEvent.StyleChange, QEvent.PaletteChange):
            self._updateStyle()
        QFrame.changeEvent(self, event)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            event.accept()
            self.fMouseLeftDown = True
            self._updateMouseMode(event)
            return
        if event.button() == Qt.RightButton:
            event.accept()
            self.fMouseRightDown = True
            self._updateMouseMode(event)
            return
        if event.button() == Qt.MiddleButton:
            event.accept()
            self.fMouseLeftDown = True
            self.fMouseRightDown = True
            self._updateMouseMode(event)
            return
        QFrame.mouseMoveEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.fMouseMode == self._MOUSE_MODE_MOVE:
            event.accept()
            if QT_VERSION >= 0x60000:
                pos = event.position()
                x = pos.x()
                y = pos.y()
            else:
                x = event.x()
                y = event.y()
            self._moveViewRect(x, y)
            return
        if self.fMouseMode == self._MOUSE_MODE_SCALE:
            event.accept()
            if qt_config == 5:
                y = event.globalY()
            else:
                y = event.globalPosition().y()
            self._scaleViewRect(y)
            return
        QFrame.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            event.accept()
            self.fMouseLeftDown = False
            self._updateMouseMode(event)
            return
        if event.button() == Qt.RightButton:
            event.accept()
            self.fMouseRightDown = False
            self._updateMouseMode(event)
            return
        if event.button() == Qt.MiddleButton:
            event.accept()
            self.fMouseLeftDown = event.buttons() & Qt.LeftButton
            self.fMouseRightDown = event.buttons() & Qt.RightButton
            self._updateMouseMode(event)
            return
        QFrame.mouseReleaseEvent(self, event)

    def wheelEvent(self, event):
        if self.fScene is None:
            QFrame.wheelEvent(self, event)
            return

        event.accept()
        self.fScene.zoom_wheel(event.angleDelta().y())

    def paintEvent(self, event):
        if self.fScene is None:
            QFrame.paintEvent(self, event)
            return

        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))

        # Brightness-aware out-of-canvas shading
        bg_color = self.fViewBg
        bg_black = bg_color.black()
        bg_shade = -12 if bg_black < 127 else 12
        r,g,b,_ = bg_color.getRgb()
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
            painter.drawRoundedRect(QRectF(0.5+frameWidth,
                                           0.5+frameWidth,
                                           self.width()-1-frameWidth*2,
                                           self.height()-1-frameWidth*2), rounding, rounding)

            clipPath = QPainterPath()
            rounding = 1.0 * use_rounding
            clipPath.addRoundedRect(QRectF(frameWidth,
                                           frameWidth,
                                           self.width()-frameWidth*2,
                                           self.height()-frameWidth*2), rounding, rounding)
            painter.setClipPath(clipPath)

        self.fScene.render(painter, self.fRenderTarget, self.fRenderSource, Qt.KeepAspectRatio)

        # Allow cursor frame to look joined with minicanvas frame
        painter.setClipping(False)

        width  = self.fViewRect[self._kRectWidth]/self.fScale
        height = self.fViewRect[self._kRectHeight]/self.fScale

        # cursor
        lineHinting = self.fViewPen.widthF() / 2
        x = self.fViewRect[self._kRectX]+self.fInitialX
        y = self.fViewRect[self._kRectY]+self.fInitialY
        scr_x = floor(x)
        scr_y = floor(y)
        rect = QRectF(
            scr_x+lineHinting,
            scr_y+lineHinting,
            ceil(width+x-scr_x)-lineHinting*2,
            ceil(height+y-scr_y)-lineHinting*2 )

        if self.fRubberBandBlending == self._RUBBERBAND_BLENDING_PLUS:
            painter.setCompositionMode(QPainter.CompositionMode_Plus)
        elif self.fRubberBandBlending == self._RUBBERBAND_BLENDING_DIFF:
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
        if extRatio >= self.fInternalRatio:
            self.fInternalHeight = floor(height - self.fFrameWidth * 2)
            self.fInternalWidth  = floor(self.fInternalHeight * self.fInternalRatio)
            self.fInitialX       = floor(float(width - self.fInternalWidth) / 2.0)
            self.fInitialY       = self.fFrameWidth
        else:
            self.fInternalWidth  = floor(width - self.fFrameWidth * 2)
            self.fInternalHeight = floor(self.fInternalWidth / self.fInternalRatio)
            self.fInitialX       = self.fFrameWidth
            self.fInitialY       = floor(float(height - self.fInternalHeight) / 2.0)

        self.fRenderTarget = QRectF(self.fInitialX,
                                    self.fInitialY,
                                    float(self.fInternalWidth),
                                    float(self.fInternalHeight))

        if self.fRealParent is not None:
            QTimer.singleShot(0, self.fRealParent.slot_miniCanvasCheckAll)

        QFrame.resizeEvent(self, event)

    # -----------------------------------------------------------------------------------------------------------------

    def _moveViewRect(self, x: float, y: float):
        if self.fScene is None:
            return

        x -= self.fInitialX
        y -= self.fInitialY

        fixPos = False
        rCentX = self.fViewRect[self._kRectWidth] / self.fScale / 2
        rCentY = self.fViewRect[self._kRectHeight] / self.fScale / 2
        if x < rCentX:
            x = rCentX
            fixPos = True
        elif x > self.fInternalWidth - rCentX:
            x = self.fInternalWidth - rCentX
            fixPos = True

        if y < rCentY:
            y = rCentY
            fixPos = True
        elif y > self.fInternalHeight - rCentY:
            y = self.fInternalHeight - rCentY
            fixPos = True

        if fixPos:
            globalPos = self.mapToGlobal(QPoint(int(self.fInitialX + x), int(self.fInitialY + y)))
            self.cursor().setPos(globalPos)

        x = self.fRenderSource.width() * x / self.fInternalWidth
        y = self.fRenderSource.height() * y / self.fInternalHeight
        self.fScene.m_view.centerOn(x, y)

    def _scaleViewRect(self, globalY: int):
        if self.fScene is None:
            return

        dy = self.fMouseInitialZoomPos.y() - globalY
        if dy != 0:
            self.setCursor(self.fZoomCursors[self._kCursorZoomIn if dy > 0 else self._kCursorZoomOut])
            self.fScene.zoom_wheel(dy)

        # FIXME do not rely on this
        self.cursor().setPos(self.fMouseInitialZoomPos)

    def _updateMouseMode(self, event):
        if self.fMouseLeftDown and self.fMouseRightDown:
            if qt_config == 5:
                self.fMouseInitialZoomPos = event.globalPos()
            else:
                self.fMouseInitialZoomPos = event.globalPosition().toPoint()
            self.setCursor(self.fZoomCursors[self._kCursorZoom])
            self.fMouseMode = self._MOUSE_MODE_SCALE

        elif self.fMouseLeftDown:
            self.setCursor(QCursor(Qt.SizeAllCursor))
            if self.fMouseMode == self._MOUSE_MODE_NONE:
                if qt_config == 5:
                    x = event.x()
                    y = event.y()
                else:
                    pos = event.position()
                    x = pos.x()
                    y = pos.y()
                self._moveViewRect(x, y)
            self.fMouseMode = self._MOUSE_MODE_MOVE

        else:
            self.unsetCursor()
            self.fMouseMode = self._MOUSE_MODE_NONE

    def _updateStyle(self):
        self.fFrameWidth = 1 if self.fUseCustomPaint else self.frameWidth()

# ---------------------------------------------------------------------------------------------------------------------
