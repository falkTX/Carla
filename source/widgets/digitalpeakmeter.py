#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Digital Peak Meter, a custom Qt4 widget
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

from PyQt4.QtCore import qCritical, Qt, QTimer, QSize
from PyQt4.QtGui import QColor, QLinearGradient, QPainter, QWidget

# ------------------------------------------------------------------------------------------------------------
# Widget Class

class DigitalPeakMeter(QWidget):
    # enum Orientation
    HORIZONTAL = 1
    VERTICAL   = 2

    # enum Color
    GREEN = 1
    BLUE  = 2

    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.fChannels    = 0
        self.fOrientation = self.VERTICAL
        self.fSmoothMultiplier = 1

        self.fColorBackground = QColor("#111111")
        self.fGradientMeter   = QLinearGradient(0, 0, 1, 1)

        self.setChannels(0)
        self.setColor(self.GREEN)

    def displayMeter(self, meter, level):
        if meter <= 0 or meter > self.fChannels:
            return qCritical("DigitalPeakMeter::displayMeter(%i, %f) - invalid meter number" % (meter, level))
        if not isinstance(level, float):
            return qCritical("DigitalPeakMeter::displayMeter(%i, %f) - meter value must be float" % (meter, level))

        i = meter - 1

        if self.fSmoothMultiplier > 0:
            level = (self.fLastValueData[i] * self.fSmoothMultiplier + level) / float(self.fSmoothMultiplier + 1)

        if level < 0.001:
            level = 0.0
        elif level > 0.999:
            level = 1.0

        if self.fChannelsData[i] != level:
            self.fChannelsData[i] = level
            self.update()

        self.fLastValueData[i] = level

    def setChannels(self, channels):
        if channels < 0:
            return qCritical("DigitalPeakMeter::setChannels(%i) - 'channels' must be a positive integer" % channels)

        self.fChannels = channels
        self.fChannelsData  = []
        self.fLastValueData = []

        for x in range(channels):
            self.fChannelsData.append(0.0)
            self.fLastValueData.append(0.0)

    def setColor(self, color):
        if color == self.GREEN:
            self.fColorBase    = QColor(93, 231, 61)
            self.fColorBaseAlt = QColor(15, 110, 15, 100)
        elif color == self.BLUE:
            self.fColorBase    = QColor(82, 238, 248)
            self.fColorBaseAlt = QColor(15, 15, 110, 100)
        else:
            return qCritical("DigitalPeakMeter::setColor(%i) - invalid color" % color)

        self.setOrientation(self.fOrientation)

    def setOrientation(self, orientation):
        self.fOrientation = orientation

        if self.fOrientation == self.HORIZONTAL:
            self.fGradientMeter.setColorAt(0.0, self.fColorBase)
            self.fGradientMeter.setColorAt(0.2, self.fColorBase)
            self.fGradientMeter.setColorAt(0.4, self.fColorBase)
            self.fGradientMeter.setColorAt(0.6, self.fColorBase)
            self.fGradientMeter.setColorAt(0.8, Qt.yellow)
            self.fGradientMeter.setColorAt(1.0, Qt.red)

        elif self.fOrientation == self.VERTICAL:
            self.fGradientMeter.setColorAt(0.0, Qt.red)
            self.fGradientMeter.setColorAt(0.2, Qt.yellow)
            self.fGradientMeter.setColorAt(0.4, self.fColorBase)
            self.fGradientMeter.setColorAt(0.6, self.fColorBase)
            self.fGradientMeter.setColorAt(0.8, self.fColorBase)
            self.fGradientMeter.setColorAt(1.0, self.fColorBase)

        else:
            return qCritical("DigitalPeakMeter::setOrientation(%i) - invalid orientation" % orientation)

        self.updateSizes()

    def setSmoothRelease(self, value):
        if value < 0:
            value = 0
        elif value > 5:
            value = 5

        self.fSmoothMultiplier = value

    def minimumSizeHint(self):
        return QSize(10, 10)

    def sizeHint(self):
        return QSize(self.fWidth, self.fHeight)

    def updateSizes(self):
        self.fWidth  = self.width()
        self.fHeight = self.height()
        self.fSizeMeter = 0

        if self.fOrientation == self.HORIZONTAL:
            self.fGradientMeter.setFinalStop(self.fWidth, 0)

            if self.fChannels > 0:
                self.fSizeMeter = self.fHeight / self.fChannels

        elif self.fOrientation == self.VERTICAL:
            self.fGradientMeter.setFinalStop(0, self.fHeight)

            if self.fChannels > 0:
                self.fSizeMeter = self.fWidth / self.fChannels

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        painter.setPen(Qt.black)
        painter.setBrush(Qt.black)
        painter.drawRect(0, 0, self.fWidth, self.fHeight)

        meterX = 0
        painter.setPen(self.fColorBackground)
        painter.setBrush(self.fGradientMeter)

        for i in range(self.fChannels):
            level = self.fChannelsData[i]

            if self.fOrientation == self.HORIZONTAL:
                value = level * float(self.fWidth)
            elif self.fOrientation == self.VERTICAL:
                value = float(self.fHeight) - (level * float(self.fHeight))
            else:
                value = 0.0

            if self.fOrientation == self.HORIZONTAL:
                painter.drawRect(0, meterX, int(value), self.fSizeMeter)
            elif self.fOrientation == self.VERTICAL:
                painter.drawRect(meterX, int(value), self.fSizeMeter, self.fHeight)

            meterX += self.fSizeMeter

        painter.setBrush(Qt.black)

        if self.fOrientation == self.HORIZONTAL:
            # Variables
            lsmall = float(self.fWidth)
            lfull  = float(self.fHeight - 1)

            # Base
            painter.setPen(self.fColorBaseAlt)
            painter.drawLine(lsmall * 0.25, 2, lsmall * 0.25, lfull-2.0)
            painter.drawLine(lsmall * 0.50, 2, lsmall * 0.50, lfull-2.0)

            # Yellow
            painter.setPen(QColor(110, 110, 15, 100))
            painter.drawLine(lsmall * 0.70, 2, lsmall * 0.70, lfull-2.0)
            painter.drawLine(lsmall * 0.83, 2, lsmall * 0.83, lfull-2.0)

            # Orange
            painter.setPen(QColor(180, 110, 15, 100))
            painter.drawLine(lsmall * 0.90, 2, lsmall * 0.90, lfull-2.0)

            # Red
            painter.setPen(QColor(110, 15, 15, 100))
            painter.drawLine(lsmall * 0.96, 2, lsmall * 0.96, lfull-2.0)

        elif self.fOrientation == self.VERTICAL:
            # Variables
            lsmall = float(self.fHeight)
            lfull  = float(self.fWidth - 1)

            # Base
            painter.setPen(self.fColorBaseAlt)
            painter.drawLine(2, lsmall - (lsmall * 0.25), lfull-2.0, lsmall - (lsmall * 0.25))
            painter.drawLine(2, lsmall - (lsmall * 0.50), lfull-2.0, lsmall - (lsmall * 0.50))

            # Yellow
            painter.setPen(QColor(110, 110, 15, 100))
            painter.drawLine(2, lsmall - (lsmall * 0.70), lfull-2.0, lsmall - (lsmall * 0.70))
            painter.drawLine(2, lsmall - (lsmall * 0.82), lfull-2.0, lsmall - (lsmall * 0.82))

            # Orange
            painter.setPen(QColor(180, 110, 15, 100))
            painter.drawLine(2, lsmall - (lsmall * 0.90), lfull-2.0, lsmall - (lsmall * 0.90))

            # Red
            painter.setPen(QColor(110, 15, 15, 100))
            painter.drawLine(2, lsmall - (lsmall * 0.96), lfull-2.0, lsmall - (lsmall * 0.96))

    def resizeEvent(self, event):
        self.updateSizes()
        QWidget.resizeEvent(self, event)
