#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Dial, a custom Qt4 widget
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

from math import floor
from PyQt4.QtCore import Qt, QPointF, QRectF, QTimer, QSize, SLOT
from PyQt4.QtGui import QColor, QConicalGradient, QDial, QFont, QFontMetrics
from PyQt4.QtGui import QLinearGradient, QPainter, QPainterPath, QPen, QPixmap

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class PixmapDial(QDial):
    # enum CustomPaint
    CUSTOM_PAINT_NULL      = 0
    CUSTOM_PAINT_CARLA_WET = 1
    CUSTOM_PAINT_CARLA_VOL = 2
    CUSTOM_PAINT_CARLA_L   = 3
    CUSTOM_PAINT_CARLA_R   = 4

    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    HOVER_MIN = 0
    HOVER_MAX = 9

    def __init__(self, parent):
        QDial.__init__(self, parent)

        self.fPixmap      = QPixmap(":/bitmaps/dial_01d.png")
        self.fPixmapNum   = "01"
        self.fCustomPaint = self.CUSTOM_PAINT_NULL

        self.fHovered   = False
        self.fHoverStep = self.HOVER_MIN

        if self.fPixmap.width() > self.fPixmap.height():
            self.fOrientation = self.HORIZONTAL
        else:
            self.fOrientation = self.VERTICAL

        self.fLabel     = ""
        self.fLabelPos  = QPointF(0.0, 0.0)
        self.fLabelFont = QFont()
        self.fLabelFont.setPointSize(6)
        self.fLabelWidth  = 0
        self.fLabelHeight = 0
        self.fLabelGradient = QLinearGradient(0, 0, 0, 1)

        if self.palette().window().color().lightness() > 100:
            # Light background
            c = self.palette().dark().color()
            self.fColor1 = c
            self.fColor2 = QColor(c.red(), c.green(), c.blue(), 0)
            self.fColorT = [self.palette().buttonText().color(), self.palette().mid().color()]
        else:
            # Dark background
            self.fColor1 = QColor(0, 0, 0, 255)
            self.fColor2 = QColor(0, 0, 0, 0)
            self.fColorT = [Qt.white, Qt.darkGray]

        self.updateSizes()

    def getSize(self):
        return self.fSize

    def setCustomPaint(self, paint):
        self.fCustomPaint = paint
        self.fLabelPos.setY(self.fSize + self.fLabelHeight/2)
        self.update()

    def setEnabled(self, enabled):
        if self.isEnabled() != enabled:
            self.fPixmap.load(":/bitmaps/dial_%s%s.png" % (self.fPixmapNum, "" if enabled else "d"))
            self.updateSizes()
            self.update()
        QDial.setEnabled(self, enabled)

    def setLabel(self, label):
        self.fLabel = label

        self.fLabelWidth  = QFontMetrics(self.fLabelFont).width(label)
        self.fLabelHeight = QFontMetrics(self.fLabelFont).height()

        self.fLabelPos.setX(float(self.fSize)/2.0 - float(self.fLabelWidth)/2.0)
        self.fLabelPos.setY(self.fSize + self.fLabelHeight)

        self.fLabelGradient.setColorAt(0.0, self.fColor1)
        self.fLabelGradient.setColorAt(0.6, self.fColor1)
        self.fLabelGradient.setColorAt(1.0, self.fColor2)

        self.fLabelGradient.setStart(0, float(self.fSize)/2.0)
        self.fLabelGradient.setFinalStop(0, self.fSize + self.fLabelHeight + 5)

        self.fLabelGradientRect = QRectF(float(self.fSize)/8.0, float(self.fSize)/2.0, float(self.fSize*6)/8.0, self.fSize+self.fLabelHeight+5)
        self.update()

    def setPixmap(self, pixmapId):
        self.fPixmapNum = "%02i" % pixmapId
        self.fPixmap.load(":/bitmaps/dial_%s%s.png" % (self.fPixmapNum, "" if self.isEnabled() else "d"))

        if self.fPixmap.width() > self.fPixmap.height():
            self.fOrientation = self.HORIZONTAL
        else:
            self.fOrientation = self.VERTICAL

        self.updateSizes()
        self.update()

    def minimumSizeHint(self):
        return QSize(self.fSize, self.fSize)

    def sizeHint(self):
        return QSize(self.fSize, self.fSize)

    def updateSizes(self):
        self.fWidth  = self.fPixmap.width()
        self.fHeight = self.fPixmap.height()

        if self.fWidth < 1:
            self.fWidth = 1

        if self.fHeight < 1:
            self.fHeight = 1

        if self.fOrientation == self.HORIZONTAL:
            self.fSize  = self.fHeight
            self.fCount = self.fWidth / self.fHeight
        else:
            self.fSize  = self.fWidth
            self.fCount = self.fHeight / self.fWidth

        self.setMinimumSize(self.fSize, self.fSize + self.fLabelHeight + 5)
        self.setMaximumSize(self.fSize, self.fSize + self.fLabelHeight + 5)

    def enterEvent(self, event):
        self.fHovered = True
        if self.fHoverStep == self.HOVER_MIN:
            self.fHoverStep = self.HOVER_MIN + 1
        QDial.enterEvent(self, event)

    def leaveEvent(self, event):
        self.fHovered = False
        if self.fHoverStep == self.HOVER_MAX:
            self.fHoverStep = self.HOVER_MAX - 1
        QDial.leaveEvent(self, event)

    def paintEvent(self, event):
        event.accept()

        painter = QPainter(self)
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, True)

        if self.fLabel:
            if self.fCustomPaint == self.CUSTOM_PAINT_NULL:
                painter.setPen(self.fColor2)
                painter.setBrush(self.fLabelGradient)
                painter.drawRect(self.fLabelGradientRect)

            painter.setFont(self.fLabelFont)
            painter.setPen(self.fColorT[0 if self.isEnabled() else 1])
            painter.drawText(self.fLabelPos, self.fLabel)

        if self.isEnabled():
            current = float(self.value() - self.minimum())
            divider = float(self.maximum() - self.minimum())

            if divider == 0.0:
                return

            value  = current / divider
            target = QRectF(0.0, 0.0, self.fSize, self.fSize)

            per = int((self.fCount - 1) * value)

            if self.fOrientation == self.HORIZONTAL:
                xpos = self.fSize * per
                ypos = 0.0
            else:
                xpos = 0.0
                ypos = self.fSize * per

            source = QRectF(xpos, ypos, self.fSize, self.fSize)
            painter.drawPixmap(target, self.fPixmap, source)

            # Custom knobs (Dry/Wet and Volume)
            if self.fCustomPaint in (self.CUSTOM_PAINT_CARLA_WET, self.CUSTOM_PAINT_CARLA_VOL):
                # knob color
                colorGreen = QColor(0x5D, 0xE7, 0x3D, 191 + self.fHoverStep*7)
                colorBlue  = QColor(0x3E, 0xB8, 0xBE, 191 + self.fHoverStep*7)

                # draw small circle
                ballRect = QRectF(8.0, 8.0, 15.0, 15.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*value)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                # draw arc
                startAngle = 216*16
                spanAngle  = -252*16*value

                if self.fCustomPaint == self.CUSTOM_PAINT_CARLA_WET:
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
            elif self.fCustomPaint in (self.CUSTOM_PAINT_CARLA_L, self.CUSTOM_PAINT_CARLA_R):
                # knob color
                color = QColor(0xAD + self.fHoverStep*5, 0xD5 + self.fHoverStep*4, 0x4B + self.fHoverStep*5)

                # draw small circle
                ballRect = QRectF(7.0, 8.0, 11.0, 12.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*value)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                painter.setBrush(color)
                painter.setPen(QPen(color, 0))
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.0, 2.0))

                # draw arc
                if self.fCustomPaint == self.CUSTOM_PAINT_CARLA_L:
                    startAngle = 216*16
                    spanAngle  = -252.0*16*value
                elif self.fCustomPaint == self.CUSTOM_PAINT_CARLA_R:
                    startAngle = 324.0*16
                    spanAngle  = 252.0*16*(1.0-value)
                else:
                    return

                painter.setPen(QPen(color, 2))
                painter.drawArc(3.5, 4.5, 22.0, 22.0, startAngle, spanAngle)

            if self.HOVER_MIN < self.fHoverStep < self.HOVER_MAX:
                self.fHoverStep += 1 if self.fHovered else -1
                QTimer.singleShot(20, self, SLOT("update()"))

        else: # isEnabled()
            target = QRectF(0.0, 0.0, self.fSize, self.fSize)
            painter.drawPixmap(target, self.fPixmap, target)

        painter.restore()

    def resizeEvent(self, event):
        self.updateSizes()
        QDial.resizeEvent(self, event)
