#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import sqrt

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import qCritical, Qt, QSize, QLineF, QRectF
    from PyQt5.QtGui import QColor, QLinearGradient, QPainter, QPen, QPixmap
    from PyQt5.QtWidgets import QWidget
elif qt_config == 6:
    from PyQt6.QtCore import qCritical, Qt, QSize, QLineF, QRectF
    from PyQt6.QtGui import QColor, QLinearGradient, QPainter, QPen, QPixmap
    from PyQt6.QtWidgets import QWidget

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class DigitalPeakMeter(QWidget):
    # enum Color
    COLOR_GREEN = 1
    COLOR_BLUE  = 2

    # enum Orientation
    HORIZONTAL = 1
    VERTICAL   = 2

    # enum Style
    STYLE_DEFAULT = 1
    STYLE_OPENAV  = 2
    STYLE_RNCBC   = 3
    STYLE_CALF    = 4

    # -----------------------------------------------------------------------------------------------------------------

    def __init__(self, parent):
        QWidget.__init__(self, parent)
        self.setAttribute(Qt.WA_OpaquePaintEvent)

        # defaults are VERTICAL, COLOR_GREEN, STYLE_DEFAULT

        self.fChannelCount    = 0
        self.fChannelData     = []
        self.fLastChannelData = []

        self.fMeterColor        = self.COLOR_GREEN
        self.fMeterColorBase    = QColor(93, 231, 61)
        self.fMeterColorBaseAlt = QColor(15, 110, 15, 100)

        self.fMeterLinesEnabled = True
        self.fMeterOrientation  = self.VERTICAL
        self.fMeterStyle        = self.STYLE_DEFAULT

        self.fMeterBackground = QColor("#070707")
        self.fMeterGradient   = QLinearGradient(0, 0, 0, 0)
        self.fMeterPixmaps    = ()

        self.fSmoothMultiplier = 2

        self.updateGrandient()

    # -----------------------------------------------------------------------------------------------------------------

    def channelCount(self):
        return self.fChannelCount

    def setChannelCount(self, count):
        if self.fChannelCount == count:
            return

        if count < 0:
            qCritical(f"DigitalPeakMeter::setChannelCount({count}) - channel count must be a positive integer or zero")
            return

        self.fChannelCount    = count
        self.fChannelData     = []
        self.fLastChannelData = []

        for _ in range(count):
            self.fChannelData.append(0.0)
            self.fLastChannelData.append(0.0)

        if self.fMeterStyle == self.STYLE_CALF:
            if self.fChannelCount > 0:
                self.setFixedSize(100, 12*self.fChannelCount)
            else:
                self.setMinimumSize(0, 0)
                self.setMaximumSize(9999, 9999)

    # -----------------------------------------------------------------------------------------------------------------

    def meterColor(self):
        return self.fMeterColor

    def setMeterColor(self, color):
        if self.fMeterColor == color:
            return

        if color not in (self.COLOR_GREEN, self.COLOR_BLUE):
            qCritical(f"DigitalPeakMeter::setMeterColor({color}) - invalid color")
            return

        if color == self.COLOR_GREEN:
            self.fMeterColorBase    = QColor(93, 231, 61)
            self.fMeterColorBaseAlt = QColor(15, 110, 15, 100)
        elif color == self.COLOR_BLUE:
            self.fMeterColorBase    = QColor(82, 238, 248)
            self.fMeterColorBaseAlt = QColor(15, 15, 110, 100)

        self.fMeterColor = color

        self.updateGrandient()

    # -----------------------------------------------------------------------------------------------------------------

    def meterLinesEnabled(self):
        return self.fMeterLinesEnabled

    def setMeterLinesEnabled(self, yesNo):
        if self.fMeterLinesEnabled == yesNo:
            return

        self.fMeterLinesEnabled = yesNo

    # -----------------------------------------------------------------------------------------------------------------

    def meterOrientation(self):
        return self.fMeterOrientation

    def setMeterOrientation(self, orientation):
        if self.fMeterOrientation == orientation:
            return

        if orientation not in (self.HORIZONTAL, self.VERTICAL):
            qCritical(f"DigitalPeakMeter::setMeterOrientation({orientation}) - invalid orientation")
            return

        self.fMeterOrientation = orientation

        self.updateGrandient()

    # -----------------------------------------------------------------------------------------------------------------

    def meterStyle(self):
        return self.fMeterStyle

    def setMeterStyle(self, style):
        if self.fMeterStyle == style:
            return

        if style not in (self.STYLE_DEFAULT, self.STYLE_OPENAV, self.STYLE_RNCBC, self.STYLE_CALF):
            qCritical(f"DigitalPeakMeter::setMeterStyle({style}) - invalid style")
            return

        if style == self.STYLE_DEFAULT:
            self.fMeterBackground = QColor("#070707")
        elif style == self.STYLE_OPENAV:
            self.fMeterBackground = QColor("#1A1A1A")
        elif style == self.STYLE_RNCBC:
            self.fMeterBackground = QColor("#070707")
        elif style == self.STYLE_CALF:
            self.fMeterBackground = QColor("#000")

        if style == self.STYLE_CALF:
            self.fMeterPixmaps = (QPixmap(":/bitmaps/meter_calf_off.png"), QPixmap(":/bitmaps/meter_calf_on.png"))
            if self.fChannelCount > 0:
                self.setFixedSize(100, 12*self.fChannelCount)
        else:
            self.fMeterPixmaps = ()
            self.setMinimumSize(0, 0)
            self.setMaximumSize(9999, 9999)

        self.fMeterStyle = style

        self.updateGrandient()

    # -----------------------------------------------------------------------------------------------------------------

    def smoothMultiplier(self):
        return self.fSmoothMultiplier

    def setSmoothMultiplier(self, value):
        if self.fSmoothMultiplier == value:
            return

        if not isinstance(value, int):
            qCritical("DigitalPeakMeter::setSmoothMultiplier() - value must be an integer")
            return
        if value < 0:
            qCritical(f"DigitalPeakMeter::setSmoothMultiplier({value}) - value must be >= 0")
            return
        if value > 5:
            qCritical(f"DigitalPeakMeter::setSmoothMultiplier({value}) - value must be < 5")
            return

        self.fSmoothMultiplier = value

    # -----------------------------------------------------------------------------------------------------------------

    def displayMeter(self, meter, level, forced = False):
        if not isinstance(meter, int):
            qCritical("DigitalPeakMeter::displayMeter(,) - meter value must be an integer")
            return
        if not isinstance(level, float):
            qCritical(f"DigitalPeakMeter::displayMeter({meter},) - level value must be a float")
            return
        if meter <= 0 or meter > self.fChannelCount:
            qCritical(f"DigitalPeakMeter::displayMeter({meter}, {level}) - invalid meter number")
            return

        i = meter - 1

        if self.fSmoothMultiplier > 0 and not forced:
            level = (
                (self.fLastChannelData[i] * float(self.fSmoothMultiplier) + level)
                / float(self.fSmoothMultiplier + 1)
            )

        if level < 0.001:
            level = 0.0
        elif level > 0.999:
            level = 1.0

        if self.fChannelData[i] != level:
            self.fChannelData[i] = level
            self.update()

        self.fLastChannelData[i] = level

    # -----------------------------------------------------------------------------------------------------------------

    def updateGrandient(self):
        self.fMeterGradient = QLinearGradient(0, 0, 1, 1)

        if self.fMeterStyle == self.STYLE_DEFAULT:
            if self.fMeterOrientation == self.HORIZONTAL:
                self.fMeterGradient.setColorAt(0.0, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.2, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.4, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.6, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.8, Qt.yellow)
                self.fMeterGradient.setColorAt(1.0, Qt.red)

            elif self.fMeterOrientation == self.VERTICAL:
                self.fMeterGradient.setColorAt(0.0, Qt.red)
                self.fMeterGradient.setColorAt(0.2, Qt.yellow)
                self.fMeterGradient.setColorAt(0.4, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.6, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(0.8, self.fMeterColorBase)
                self.fMeterGradient.setColorAt(1.0, self.fMeterColorBase)

        elif self.fMeterStyle == self.STYLE_RNCBC:
            if self.fMeterColor == self.COLOR_BLUE:
                c1 = QColor(40,160,160)
                c2 = QColor(60,220,160)
            elif self.fMeterColor == self.COLOR_GREEN:
                c1 = QColor( 40,160,40)
                c2 = QColor(160,220,20)

            if self.fMeterOrientation == self.HORIZONTAL:
                self.fMeterGradient.setColorAt(0.0, c1)
                self.fMeterGradient.setColorAt(0.2, c1)
                self.fMeterGradient.setColorAt(0.6, c2)
                self.fMeterGradient.setColorAt(0.7, QColor(220,220, 20))
                self.fMeterGradient.setColorAt(0.8, QColor(240,160, 20))
                self.fMeterGradient.setColorAt(0.9, QColor(240,  0, 20))
                self.fMeterGradient.setColorAt(1.0, QColor(240,  0, 20))

            elif self.fMeterOrientation == self.VERTICAL:
                self.fMeterGradient.setColorAt(0.0, QColor(240,  0, 20))
                self.fMeterGradient.setColorAt(0.1, QColor(240,  0, 20))
                self.fMeterGradient.setColorAt(0.2, QColor(240,160, 20))
                self.fMeterGradient.setColorAt(0.3, QColor(220,220, 20))
                self.fMeterGradient.setColorAt(0.4, c2)
                self.fMeterGradient.setColorAt(0.8, c1)
                self.fMeterGradient.setColorAt(1.0, c1)

        elif self.fMeterStyle in (self.STYLE_OPENAV, self.STYLE_CALF):
            self.fMeterGradient.setColorAt(0.0, self.fMeterColorBase)
            self.fMeterGradient.setColorAt(1.0, self.fMeterColorBase)

        self.updateGrandientFinalStop()

    def updateGrandientFinalStop(self):
        if self.fMeterOrientation == self.HORIZONTAL:
            self.fMeterGradient.setFinalStop(self.width(), 0)

        elif self.fMeterOrientation == self.VERTICAL:
            self.fMeterGradient.setFinalStop(0, self.height())

    # -----------------------------------------------------------------------------------------------------------------

    def minimumSizeHint(self):
        return QSize(20, 10) if self.fMeterOrientation == self.HORIZONTAL else QSize(10, 20)

    def sizeHint(self):
        return QSize(self.width(), self.height())

    # -----------------------------------------------------------------------------------------------------------------

    def drawCalf(self, event):
        painter = QPainter(self)
        event.accept()

        # no channels, draw black
        if self.fChannelCount == 0:
            painter.setPen(QPen(Qt.black, 2))
            painter.setBrush(Qt.black)
            painter.drawRect(0, 0, self.width(), self.height())
            return

        for i in range(self.fChannelCount):
            painter.drawPixmap(0, 12*i, self.fMeterPixmaps[0])

        meterPos  = 4
        meterSize = 12

        # draw levels
        for level in self.fChannelData:
            if level != 0.0:
                blevel = int(sqrt(level)*26.0)*3
                painter.drawPixmap(5, meterPos, blevel, 4, self.fMeterPixmaps[1], 0, 0, blevel, 4)
            meterPos += meterSize

    def paintEvent(self, event):
        if self.fMeterStyle == self.STYLE_CALF:
            self.drawCalf(event)
            return

        painter = QPainter(self)
        event.accept()

        width  = self.width()
        height = self.height()

        # draw background
        painter.setPen(QPen(self.fMeterBackground, 2))
        painter.setBrush(self.fMeterBackground)
        painter.drawRect(0, 0, width, height)

        if self.fChannelCount == 0:
            return

        meterPad  = 0
        meterPos  = 0
        meterSize = (height if self.fMeterOrientation == self.HORIZONTAL else width)/self.fChannelCount

        # set pen/brush for levels
        if self.fMeterStyle == self.STYLE_OPENAV:
            colorTrans = QColor(self.fMeterColorBase)
            colorTrans.setAlphaF(0.5)
            painter.setBrush(colorTrans)
            painter.setPen(QPen(self.fMeterColorBase, 1))
            del colorTrans
            meterPad  += 2
            meterSize -= 2

        else:
            painter.setPen(QPen(self.fMeterBackground, 0))
            painter.setBrush(self.fMeterGradient)

        # draw levels
        for level in self.fChannelData:
            if level == 0.0:
                pass
            elif self.fMeterOrientation == self.HORIZONTAL:
                painter.drawRect(QRectF(0, meterPos, sqrt(level) * float(width), meterSize))
            elif self.fMeterOrientation == self.VERTICAL:
                painter.drawRect(QRectF(meterPos, height - sqrt(level) * float(height), meterSize, height))

            meterPos += meterSize+meterPad

        if not self.fMeterLinesEnabled:
            return

        # draw lines
        if self.fMeterOrientation == self.HORIZONTAL:
            # Variables
            lsmall = float(width)
            lfull  = float(height - 1)

            if self.fMeterStyle == self.STYLE_OPENAV:
                painter.setPen(QColor(37, 37, 37, 100))
                painter.drawLine(QLineF(lsmall * 0.25, 2, lsmall * 0.25, lfull-2.0))
                painter.drawLine(QLineF(lsmall * 0.50, 2, lsmall * 0.50, lfull-2.0))
                painter.drawLine(QLineF(lsmall * 0.75, 2, lsmall * 0.75, lfull-2.0))

                if self.fChannelCount > 1:
                    painter.drawLine(QLineF(1, lfull/2-1, lsmall-1, lfull/2-1))

            else:
                # Base
                painter.setBrush(Qt.black)
                painter.setPen(QPen(self.fMeterColorBaseAlt, 1))
                painter.drawLine(QLineF(lsmall * 0.25, 2, lsmall * 0.25, lfull-2.0))
                painter.drawLine(QLineF(lsmall * 0.50, 2, lsmall * 0.50, lfull-2.0))

                # Yellow
                painter.setPen(QColor(110, 110, 15, 100))
                painter.drawLine(QLineF(lsmall * 0.70, 2, lsmall * 0.70, lfull-2.0))
                painter.drawLine(QLineF(lsmall * 0.83, 2, lsmall * 0.83, lfull-2.0))

                # Orange
                painter.setPen(QColor(180, 110, 15, 100))
                painter.drawLine(QLineF(lsmall * 0.90, 2, lsmall * 0.90, lfull-2.0))

                # Red
                painter.setPen(QColor(110, 15, 15, 100))
                painter.drawLine(QLineF(lsmall * 0.96, 2, lsmall * 0.96, lfull-2.0))

        elif self.fMeterOrientation == self.VERTICAL:
            # Variables
            lsmall = float(height)
            lfull  = float(width - 1)

            if self.fMeterStyle == self.STYLE_OPENAV:
                painter.setPen(QColor(37, 37, 37, 100))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.25), lfull-2.0, lsmall - (lsmall * 0.25)))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.50), lfull-2.0, lsmall - (lsmall * 0.50)))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.75), lfull-2.0, lsmall - (lsmall * 0.75)))

                if self.fChannelCount > 1:
                    painter.drawLine(QLineF(lfull/2-1, 1, lfull/2-1, lsmall-1))

            else:
                # Base
                painter.setBrush(Qt.black)
                painter.setPen(QPen(self.fMeterColorBaseAlt, 1))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.25), lfull-2.0, lsmall - (lsmall * 0.25)))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.50), lfull-2.0, lsmall - (lsmall * 0.50)))

                # Yellow
                painter.setPen(QColor(110, 110, 15, 100))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.70), lfull-2.0, lsmall - (lsmall * 0.70)))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.82), lfull-2.0, lsmall - (lsmall * 0.82)))

                # Orange
                painter.setPen(QColor(180, 110, 15, 100))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.90), lfull-2.0, lsmall - (lsmall * 0.90)))

                # Red
                painter.setPen(QColor(110, 15, 15, 100))
                painter.drawLine(QLineF(2, lsmall - (lsmall * 0.96), lfull-2.0, lsmall - (lsmall * 0.96)))

    # -----------------------------------------------------------------------------------------------------------------

    def resizeEvent(self, event):
        QWidget.resizeEvent(self, event)
        self.updateGrandientFinalStop()

# ---------------------------------------------------------------------------------------------------------------------
