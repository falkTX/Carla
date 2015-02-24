#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Dial, a custom Qt4 widget
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

from math import cos, floor, pi, sin

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt5.QtGui import QColor, QConicalGradient, QFont, QFontMetrics
    from PyQt5.QtGui import QLinearGradient, QPainter, QPainterPath, QPen, QPixmap
    from PyQt5.QtWidgets import QDial
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt4.QtGui import QColor, QConicalGradient, QFont, QFontMetrics
    from PyQt4.QtGui import QDial, QLinearGradient, QPainter, QPainterPath, QPen, QPixmap

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class PixmapDial(QDial):
    # enum CustomPaintMode
    CUSTOM_PAINT_MODE_NULL        = 0 # default (NOTE: only this mode has label gradient)
    CUSTOM_PAINT_MODE_CARLA_WET   = 1 # color blue-green gradient (reserved #3)
    CUSTOM_PAINT_MODE_CARLA_VOL   = 2 # color blue (reserved #3)
    CUSTOM_PAINT_MODE_CARLA_L     = 3 # color yellow (reserved #4)
    CUSTOM_PAINT_MODE_CARLA_R     = 4 # color yellow (reserved #4)
    CUSTOM_PAINT_MODE_CARLA_PAN   = 5 # color yellow (reserved #3)
    CUSTOM_PAINT_MODE_COLOR       = 6 # color, selectable (reserved #3)
    CUSTOM_PAINT_MODE_ZITA        = 7 # custom zita knob (reserved #6)
    CUSTOM_PAINT_MODE_NO_GRADIENT = 8 # skip label gradient

    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    HOVER_MIN = 0
    HOVER_MAX = 9

    # signals
    realValueChanged = pyqtSignal(float)

    def __init__(self, parent, index=0):
        QDial.__init__(self, parent)

        self.fMinimum   = 0.0
        self.fMaximum   = 1.0
        self.fRealValue = 0.0

        self.fIsHovered = False
        self.fHoverStep = self.HOVER_MIN

        self.fIndex     = index
        self.fPixmap    = QPixmap(":/bitmaps/dial_01d.png")
        self.fPixmapNum = "01"

        if self.fPixmap.width() > self.fPixmap.height():
            self.fPixmapOrientation = self.HORIZONTAL
        else:
            self.fPixmapOrientation = self.VERTICAL

        self.fLabel     = ""
        self.fLabelPos  = QPointF(0.0, 0.0)
        self.fLabelFont = QFont(self.font())
        self.fLabelFont.setPixelSize(8)
        self.fLabelWidth  = 0
        self.fLabelHeight = 0

        if self.palette().window().color().lightness() > 100:
            # Light background
            c = self.palette().dark().color()
            self.fLabelGradientColor1 = c
            self.fLabelGradientColor2 = QColor(c.red(), c.green(), c.blue(), 0)
            self.fLabelGradientColorT = [self.palette().buttonText().color(), self.palette().mid().color()]
        else:
            # Dark background
            self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
            self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
            self.fLabelGradientColorT = [Qt.white, Qt.darkGray]

        self.fLabelGradient = QLinearGradient(0, 0, 0, 1)
        self.fLabelGradient.setColorAt(0.0, self.fLabelGradientColor1)
        self.fLabelGradient.setColorAt(0.6, self.fLabelGradientColor1)
        self.fLabelGradient.setColorAt(1.0, self.fLabelGradientColor2)

        self.fLabelGradientRect = QRectF(0.0, 0.0, 0.0, 0.0)

        self.fCustomPaintMode  = self.CUSTOM_PAINT_MODE_NULL
        self.fCustomPaintColor = QColor(0xff, 0xff, 0xff)

        self.updateSizes()

        # Fake internal value, 10'000 precision
        QDial.setMinimum(self, 0)
        QDial.setMaximum(self, 10000)
        QDial.setValue(self, 0)

        self.valueChanged.connect(self.slot_valueChanged)

    def getIndex(self):
        return self.fIndex

    def getBaseSize(self):
        return self.fPixmapBaseSize

    def forceWhiteLabelGradientText(self):
        self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
        self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
        self.fLabelGradientColorT = [Qt.white, Qt.darkGray]

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

        self.fLabelWidth  = QFontMetrics(self.fLabelFont).width(self.fLabel)
        self.fLabelHeight = QFontMetrics(self.fLabelFont).height()

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

    def setCustomPaintMode(self, paintMode):
        if self.fCustomPaintMode == paintMode:
            return

        self.fCustomPaintMode = paintMode
        self.update()

    def setCustomPaintColor(self, color):
        if self.fCustomPaintColor == color:
            return

        self.fCustomPaintColor = color
        self.update()

    def setLabel(self, label):
        if self.fLabel == label:
            return

        self.fLabel = label
        self.updateSizes()
        self.update()

    def setIndex(self, index):
        self.fIndex = index

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

    def setMinimum(self, value):
        self.fMinimum = value

    def setMaximum(self, value):
        self.fMaximum = value

    def setValue(self, value):
        if self.fRealValue == value:
            return

        self.fRealValue = value
        normValue = float(value - self.fMinimum) / float(self.fMaximum - self.fMinimum)
        QDial.setValue(self, int(normValue * 10000))

    @pyqtSlot(int)
    def slot_valueChanged(self, value):
        self.fRealValue = float(value)/10000.0 * (self.fMaximum - self.fMinimum) + self.fMinimum
        self.realValueChanged.emit(self.fRealValue)

    @pyqtSlot()
    def slot_updatePixmap(self):
        self.setPixmap(int(self.fPixmapNum))

    def minimumSizeHint(self):
        return QSize(self.fPixmapBaseSize, self.fPixmapBaseSize)

    def sizeHint(self):
        return QSize(self.fPixmapBaseSize, self.fPixmapBaseSize)

    def changeEvent(self, event):
        QDial.changeEvent(self, event)

        # Force pixmap update if enabled state changes
        if event.type() == QEvent.EnabledChange:
            self.setPixmap(int(self.fPixmapNum))

    def enterEvent(self, event):
        self.fIsHovered = True
        if self.fHoverStep == self.HOVER_MIN:
            self.fHoverStep = self.HOVER_MIN + 1
        QDial.enterEvent(self, event)

    def leaveEvent(self, event):
        self.fIsHovered = False
        if self.fHoverStep == self.HOVER_MAX:
            self.fHoverStep = self.HOVER_MAX - 1
        QDial.leaveEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, True)

        if self.fLabel:
            if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_NULL:
                painter.setPen(self.fLabelGradientColor2)
                painter.setBrush(self.fLabelGradient)
                painter.drawRect(self.fLabelGradientRect)

            painter.setFont(self.fLabelFont)
            painter.setPen(self.fLabelGradientColorT[0 if self.isEnabled() else 1])
            painter.drawText(self.fLabelPos, self.fLabel)

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

        painter.restore()

    def resizeEvent(self, event):
        QDial.resizeEvent(self, event)
        self.updateSizes()

# ------------------------------------------------------------------------------------------------------------
# Main Testing

if __name__ == '__main__':
    import sys
    from PyQt4.QtGui import QApplication
    import resources_rc

    app = QApplication(sys.argv)

    gui = PixmapDial(None)
    #gui.setEnabled(True)
    #gui.setEnabled(False)
    gui.setPixmap(3)
    gui.setLabel("hahaha")
    gui.show()

    sys.exit(app.exec_())
