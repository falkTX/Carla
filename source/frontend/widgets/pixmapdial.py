#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import cos, floor, pi, sin

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt5.QtGui import QColor, QConicalGradient, QFontMetrics, QPainterPath, QPen, QPixmap
    from PyQt5.QtWidgets import QDial
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt6.QtGui import QColor, QConicalGradient, QFontMetrics, QPainterPath, QPen, QPixmap
    from PyQt6.QtWidgets import QDial

from .commondial import CommonDial
from carla_shared import fontMetricsHorizontalAdvance

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class PixmapDial(CommonDial):
    def __init__(self, parent, index=0):
        CommonDial.__init__(self, parent, index)

        self.fPixmap    = QPixmap(":/bitmaps/dial_01d.png")
        self.fPixmapNum = "01"

        if self.fPixmap.width() > self.fPixmap.height():
            self.fPixmapOrientation = self.HORIZONTAL
        else:
            self.fPixmapOrientation = self.VERTICAL

        self.updateSizes()

    def getBaseSize(self):
        return self.fPixmapBaseSize

    def updateSizes(self):
        self.fPixmapWidth  = self.fPixmap.width()
        self.fPixmapHeight = self.fPixmap.height()

        if self.fPixmapWidth < 1:
            self.fPixmapWidth = 1

        if self.fPixmapHeight < 1:
            self.fPixmapHeight = 1

        if self.fPixmapOrientation == self.HORIZONTAL:
            self.fPixmapBaseSize    = self.fPixmapHeight
            self.fPixmapLayersCount = self.fPixmapWidth / self.fPixmapHeight
        else:
            self.fPixmapBaseSize    = self.fPixmapWidth
            self.fPixmapLayersCount = self.fPixmapHeight / self.fPixmapWidth

        self.setMinimumSize(self.fPixmapBaseSize, self.fPixmapBaseSize + self.fLabelHeight + 5)
        self.setMaximumSize(self.fPixmapBaseSize, self.fPixmapBaseSize + self.fLabelHeight + 5)

        if not self.fLabel:
            self.fLabelHeight = 0
            self.fLabelWidth  = 0
            return

        metrics = QFontMetrics(self.fLabelFont)
        self.fLabelWidth  = fontMetricsHorizontalAdvance(metrics, self.fLabel)
        self.fLabelHeight = metrics.height()

        self.fLabelPos.setX(float(self.fPixmapBaseSize)/2.0 - float(self.fLabelWidth)/2.0)

        if self.fPixmapNum in ("01", "02", "07", "08", "09", "10"):
            self.fLabelPos.setY(self.fPixmapBaseSize + self.fLabelHeight)
        elif self.fPixmapNum in ("11",):
            self.fLabelPos.setY(self.fPixmapBaseSize + self.fLabelHeight*2/3)
        else:
            self.fLabelPos.setY(self.fPixmapBaseSize + self.fLabelHeight/2)

        self.fLabelGradient.setStart(0, float(self.fPixmapBaseSize)/2.0)
        self.fLabelGradient.setFinalStop(0, self.fPixmapBaseSize + self.fLabelHeight + 5)

        self.fLabelGradientRect = QRectF(float(self.fPixmapBaseSize)/8.0, float(self.fPixmapBaseSize)/2.0, float(self.fPixmapBaseSize*3)/4.0, self.fPixmapBaseSize+self.fLabelHeight+5)

    def setPixmap(self, pixmapId):
        self.fPixmapNum = "%02i" % pixmapId
        self.fPixmap.load(":/bitmaps/dial_%s%s.png" % (self.fPixmapNum, "" if self.isEnabled() else "d"))

        if self.fPixmap.width() > self.fPixmap.height():
            self.fPixmapOrientation = self.HORIZONTAL
        else:
            self.fPixmapOrientation = self.VERTICAL

        # special pixmaps
        if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_NULL:
            # reserved for carla-wet, carla-vol, carla-pan and color
            if self.fPixmapNum == "03":
                self.fCustomPaintMode = self.CUSTOM_PAINT_MODE_COLOR

            # reserved for carla-L and carla-R
            elif self.fPixmapNum == "04":
                self.fCustomPaintMode = self.CUSTOM_PAINT_MODE_CARLA_L

            # reserved for zita
            elif self.fPixmapNum == "06":
                self.fCustomPaintMode = self.CUSTOM_PAINT_MODE_ZITA

        self.updateSizes()
        self.update()

    @pyqtSlot()
    def slot_updatePixmap(self):
        self.setPixmap(int(self.fPixmapNum))

    def minimumSizeHint(self):
        return QSize(self.fPixmapBaseSize, self.fPixmapBaseSize)

    def sizeHint(self):
        return QSize(self.fPixmapBaseSize, self.fPixmapBaseSize)

    def changeEvent(self, event):
        CommonDial.changeEvent(self, event)

        # Force pixmap update if enabled state changes
        if event.type() == QEvent.EnabledChange:
            self.setPixmap(int(self.fPixmapNum))

    def paintDial(self, painter):
        if self.isEnabled():
            normValue = float(self.fRealValue - self.fMinimum) / float(self.fMaximum - self.fMinimum)
            target    = QRectF(0.0, 0.0, self.fPixmapBaseSize, self.fPixmapBaseSize)

            curLayer = int((self.fPixmapLayersCount - 1) * normValue)

            if self.fPixmapOrientation == self.HORIZONTAL:
                xpos = self.fPixmapBaseSize * curLayer
                ypos = 0.0
            else:
                xpos = 0.0
                ypos = self.fPixmapBaseSize * curLayer

            source = QRectF(xpos, ypos, self.fPixmapBaseSize, self.fPixmapBaseSize)
            painter.drawPixmap(target, self.fPixmap, source)

            # Custom knobs (Dry/Wet and Volume)
            if self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_WET, self.CUSTOM_PAINT_MODE_CARLA_VOL):
                # knob color
                colorGreen = QColor(0x5D, 0xE7, 0x3D).lighter(100 + self.fHoverStep*6)
                colorBlue  = QColor(0x3E, 0xB8, 0xBE).lighter(100 + self.fHoverStep*6)

                # draw small circle
                ballRect = QRectF(8.0, 8.0, 15.0, 15.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*normValue)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                # draw arc
                startAngle = 216*16
                spanAngle  = -252*16*normValue

                if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_WET:
                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 0))
                    painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2))

                    gradient = QConicalGradient(15.5, 15.5, -45)
                    gradient.setColorAt(0.0,   colorBlue)
                    gradient.setColorAt(0.125, colorBlue)
                    gradient.setColorAt(0.625, colorGreen)
                    gradient.setColorAt(0.75,  colorGreen)
                    gradient.setColorAt(0.76,  colorGreen)
                    gradient.setColorAt(1.0,   colorGreen)
                    painter.setBrush(gradient)
                    painter.setPen(QPen(gradient, 3))

                else:
                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 0))
                    painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2))

                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 3))

                painter.drawArc(4.0, 4.0, 26.0, 26.0, startAngle, spanAngle)

            # Custom knobs (L and R)
            elif self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_L, self.CUSTOM_PAINT_MODE_CARLA_R):
                # knob color
                color = QColor(0xAD, 0xD5, 0x48).lighter(100 + self.fHoverStep*6)

                # draw small circle
                ballRect = QRectF(7.0, 8.0, 11.0, 12.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*normValue)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                painter.setBrush(color)
                painter.setPen(QPen(color, 0))
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.0, 2.0))

                # draw arc
                if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_L:
                    startAngle = 216*16
                    spanAngle  = -252.0*16*normValue
                else:
                    startAngle = 324.0*16
                    spanAngle  = 252.0*16*(1.0-normValue)

                painter.setPen(QPen(color, 2))
                painter.drawArc(3.5, 4.5, 22.0, 22.0, startAngle, spanAngle)

            # Custom knobs (Color)
            elif self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_COLOR:
                # knob color
                color = self.fCustomPaintColor.lighter(100 + self.fHoverStep*6)

                # draw small circle
                ballRect = QRectF(8.0, 8.0, 15.0, 15.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                tmpValue  = (0.375 + 0.75*normValue)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                # draw arc
                startAngle = 216*16
                spanAngle  = -252*16*normValue

                painter.setBrush(color)
                painter.setPen(QPen(color, 0))
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2))

                painter.setBrush(color)
                painter.setPen(QPen(color, 3))
                painter.drawArc(4.0, 4.0, 26.0, 26.0, startAngle, spanAngle)

            # Custom knobs (Zita)
            elif self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_ZITA:
                a = normValue * pi * 1.5 - 2.35
                r = 10.0
                x = 10.5
                y = 10.5
                x += r * sin(a)
                y -= r * cos(a)
                painter.setBrush(Qt.black)
                painter.setPen(QPen(Qt.black, 2))
                painter.drawLine(QPointF(11.0, 11.0), QPointF(x, y))

            # Custom knobs
            else:
                painter.restore()
                return

            if self.HOVER_MIN < self.fHoverStep < self.HOVER_MAX:
                self.fHoverStep += 1 if self.fIsHovered else -1
                QTimer.singleShot(20, self.update)

        else: # isEnabled()
            target = QRectF(0.0, 0.0, self.fPixmapBaseSize, self.fPixmapBaseSize)
            painter.drawPixmap(target, self.fPixmap, target)

# ---------------------------------------------------------------------------------------------------------------------
