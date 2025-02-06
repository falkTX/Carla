#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import cos, floor, pi, sin, copysign
import numpy as np

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt5.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, Qt, QEvent, QPointF, QRectF, QTimer, QSize
    from PyQt6.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen

from .commondial import CommonDial
from carla_shared import fontMetricsHorizontalAdvance

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class ScalableDial(CommonDial):
    def __init__(self, parent, index=0):
        CommonDial.__init__(self, parent, index)
        self.setImage(0)

    def getBaseSize(self):
        return self.fImageBaseSize

    def updateSizes(self):
        self.setMinimumSize(self.fImageBaseSize, self.fImageBaseSize + self.fLabelHeight + 5)
        self.setMaximumSize(self.fImageBaseSize, self.fImageBaseSize + self.fLabelHeight + 5)
        if not self.fLabel:
            self.fLabelHeight = 0
            self.fLabelWidth  = 0
            return

        metrics = QFontMetrics(self.fLabelFont)
        self.fLabelWidth  = fontMetricsHorizontalAdvance(metrics, self.fLabel)
        self.fLabelHeight = metrics.height()

        self.fLabelPos.setX(float(self.fImageBaseSize)/2.0 - float(self.fLabelWidth)/2.0)

        if self.fImageId in (1, 2, 7, 8, 9, 10):
            self.fLabelPos.setY(self.fImageBaseSize + self.fLabelHeight)
        elif self.fImageId in (11,):
            self.fLabelPos.setY(self.fImageBaseSize + self.fLabelHeight*2/3)
        else:
            self.fLabelPos.setY(self.fImageBaseSize + self.fLabelHeight/2)

        self.fLabelGradient.setStart(0, float(self.fImageBaseSize)/2.0)
        self.fLabelGradient.setFinalStop(0, self.fImageBaseSize + self.fLabelHeight + 5)

        self.fLabelGradientRect = QRectF(float(self.fImageBaseSize)/8.0, float(self.fImageBaseSize)/2.0,
                                         float(self.fImageBaseSize*3)/4.0, self.fImageBaseSize+self.fLabelHeight+5)

    def setImage(self, imageId):
        self.fImageId = imageId
        self.fImageBaseSize = 30      # internal
        if (imageId == 0):
            return

        if imageId in (7, 8, 9, 10):  # calf
            self.fImageBaseSize = 40
        elif imageId in (11, 12, 13): # openav
            self.fImageBaseSize = 32

        # special svgs
        if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_NULL:
            # reserved for carla-wet, carla-vol, carla-pan and color
            if self.fImageId == 3:
                self.fCustomPaintMode = self.CUSTOM_PAINT_MODE_COLOR

            # reserved for carla-L and carla-R
            elif self.fImageId == 4:
                self.fCustomPaintMode = self.CUSTOM_PAINT_MODE_CARLA_L
                self.fImageBaseSize = 26

        self.updateSizes()
        self.update()

    @pyqtSlot()
    def slot_updateImage(self):
        self.setImage(int(self.fImageId))

    def minimumSizeHint(self):
        return QSize(self.fImageBaseSize, self.fImageBaseSize)

    def sizeHint(self):
        return QSize(self.fImageBaseSize, self.fImageBaseSize)

    def changeEvent(self, event):
        CommonDial.changeEvent(self, event)

        # Force svg update if enabled state changes
        if event.type() == QEvent.EnabledChange:
            self.slot_updateImage()

    def paintDial(self, painter):

        # Replace Qt draw over substrate bitmap or svg to
        # all-in-one widget generated from stratch using Qt only,
        # make it highly tuneable, and uniformly look like
        # using HSL color model to make same brightness of colored things.
        # We can also easily have color tinted (themed) knobs.
        # Some things were simplified a little, to gain more speed.
        # R: knob nib (cap) radius
        def createDial(R, hue1, hue2, barWidth, angleSpan, ticks, dialType, value):
            # X,Y: center
            X = Y = self.fImageBaseSize / 2

            ang0 = int((angleSpan/2+90)*16)
            ang1 = -angleSpan*16

            if (value < -1.1):
                value = 0
                enabled = 0
                enlight = 0
            else:
                enabled = 1
                enlight = self.fHoverStep / 40

            def rectBorder(w):
                return QRectF(X-R-w, Y-R-w, (R+w)*2, (R+w)*2)

            def gray(luma):
                return QColor.fromHslF(0, 0, luma, 1)

            S = 0.9     # Saturation
            color0 = Qt.black
            color1 = QColor.fromHslF(hue1, S, 0.5+enlight, 1)
            color2 = QColor.fromHslF(hue2, S, 0.5+enlight, 1)

            if dialType == 1: # mimic svg dial
                # light arc substrate: near black, 0.5 px exposed
                painter.setPen(QPen(gray(0.10), barWidth+1, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)

                # light arc: gray bar
                # should be combined with light (value) arc to be a bit faster. TODO
                g1 = QConicalGradient(X, Y, 270)
                g1.setColorAt(0.0, gray(0.20)) # right part
                g1.setColorAt(1.0, gray(0.15)) # left part
                painter.setPen(QPen(g1, barWidth, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)

                # cap
                g2 = QRadialGradient(X-R, Y-R, R*2)
                g2.setColorAt(0.0, gray(0.45+enlight))
                g2.setColorAt(1.0, gray(0.15+enlight))
                painter.setBrush(g2)
                painter.setPen(QPen(g2, 1))
                painter.drawEllipse(rectBorder(0.5))

            elif dialType == 2: # calf
                # outer chamfer
                g1 = QConicalGradient(X, Y, 135)
                g1.setColorAt(0.0, gray(0.15))
                g1.setColorAt(0.5, gray(0.50))
                g1.setColorAt(1.0, gray(0.15))
                painter.setPen(QPen(g1, 2, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(barWidth*2-1), 0, 360*16)

                # knob chamfer
                g2 = QConicalGradient(X, Y, -45)
                g2.setColorAt(0.0, gray(0.15))
                g2.setColorAt(0.5, gray(0.50))
                g2.setColorAt(1.0, gray(0.15))
                painter.setPen(QPen(g2, 2, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(1), 0, 360*16)

                # leds substrate
                # should be combined with light (value) "arc" to be a bit faster. TODO
                # painter.setPen(QPen(gray(0.05+enlight), barWidth + 1))
                painter.setPen(QPen(QColor.fromHslF(hue1, S, 0.05+enlight/2, 1), barWidth+1))
                painter.drawArc(rectBorder(barWidth), 0, 360*16)

                # machined shiny cap
                g3 = QConicalGradient(X, Y, 0)
                for i in (5.9, 10.7, 15.7, 20.8, 25.8, 30.6, 40.6, 45.9,
                         55.9, 60.7, 65.7, 70.8, 75.8, 80.6, 90.6, 95.9):
                    g3.setColorAt(int(i)/100.0, gray(i % 1.0))
                painter.setBrush(g3)
                painter.setPen(QPen(g3, 1))
                painter.drawEllipse(rectBorder(0))

            elif dialType == 3: # openav
                # light arc substrate
                painter.setPen(QPen(gray(0.20+enlight), barWidth))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)


            # do not draw marks on disabled items
            if enabled == 1:
                # Forward or reverse (for 'R' ch knob)
                a = ((0.5-abs(value)) * copysign(angleSpan, value) + 90) * pi/180

                if dialType == 1:    # ball
                    x = X + R * 0.8 * cos(a)
                    y = Y - R * 0.8 * sin(a)
                    w = barWidth - 0.5
                    painter.setBrush(color1)
                    painter.setPen(QPen(color1, 0))
                    painter.drawEllipse(QRectF(x-w/2, y-w/2, w, w))

                elif dialType == 2:  # line for calf
                    x = X + R * 0.9 * cos(a)
                    y = Y - R * 0.9 * sin(a)
                    x1 = X + R * 0.6 * cos(a)
                    y1 = Y - R * 0.6 * sin(a)
                    painter.setPen(QPen(Qt.black, 1.5))
                    painter.drawLine(QPointF(x, y), QPointF(x1, y1))

                elif dialType == 3:  # line for openav
                    x = X + (R + barWidth) * cos(a)
                    y = Y - (R + barWidth) * sin(a)
                    painter.setPen(QPen(color1, barWidth, cap=Qt.RoundCap))
                    painter.drawLine(QPointF(x, y), QPointF(X, Y))

            # draw arc: forward, or reverse (for 'R' ch knob)
            startAngle = int((copysign(angleSpan/2, value) + 90)*16)

            gradient = QConicalGradient(X, Y, 270)
            if (ticks == 0):
                spanAngle = int(-angleSpan*16 * value)
                gradient.setColorAt(0.25, color1)
                gradient.setColorAt(0.75, color2)
            else:
                spanAngle = int(-int(angleSpan*value/(360/ticks)+0.5)*(360/ticks)*16)
                n = ticks*2
                for i in range(0, n, 2):
                    gradient.setColorAt(((i-0.40)/n % 1.0), color0)
                    gradient.setColorAt(((i+0.85)/n % 1.0), color1.lighter(100))

            if dialType == 3:  # openav
                painter.setPen(QPen(gradient, barWidth, cap=Qt.RoundCap))
            else:
                painter.setPen(QPen(gradient, barWidth, cap=Qt.FlatCap))
            painter.drawArc(QRectF(rectBorder(barWidth)), startAngle, spanAngle)

        if self.isEnabled():
            normValue = float(self.fRealValue - self.fMinimum) / float(self.fMaximum - self.fMinimum)
        else:
            normValue = -9.9

        # Custom knobs (Dry/Wet and Volume)
        if self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_WET, self.CUSTOM_PAINT_MODE_CARLA_VOL):
            hue1 = 0.3 # Green
            hue2 = 0.5 # Blue
            if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_VOL:
                hue1 = hue2

            createDial(10, hue1, hue2, 3, 260, 0, 1, normValue)

        # Custom knobs (L and R)
        elif self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_L, self.CUSTOM_PAINT_MODE_CARLA_R):
            hue = 0.21 # Lime
            if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_R:
                # Shift to full negative range (incl. 0) for reversed knob
                normValue = normValue - 1 - np.nextafter(0, 1)

            createDial(8, hue, hue, 2.5, 260, 0, 1, normValue)

        # Custom knobs (Color)
        elif self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_COLOR:
            # NOTE here all incoming color data, except hue, is lost.
            hue = self.fCustomPaintColor.hueF()

            createDial(10, hue, hue, 3, 260, 0, 1, normValue)

        # Custom knobs
        elif self.fImageId in (11, 12, 13):            # openav
            if self.fImageId == 12:
                hue1 = hue2 = 0.5 # Blue
            elif self.fImageId == 13:
                hue1 = 0.3 # Green
                hue2 = 0.5 # Blue
            else:
                hue1 = hue2 = 0.05 # Orange

            createDial(12, hue1, hue2, 2.5, 270, 0, 3, normValue)

        elif self.fImageId in (1, 2, 7, 8, 9, 10):     # calf
            hue = 0.52 # Aqua

            createDial(12, hue, hue, 4, 296, 36, 2, normValue)

        else:
            print("Unknown type knob.")
            return


        # Maybe it's better to be disabled when not self.isEnabled() ? FIXME
        if self.HOVER_MIN < self.fHoverStep < self.HOVER_MAX:
            self.fHoverStep += 1 if self.fIsHovered else -1
            QTimer.singleShot(20, self.update)

# ---------------------------------------------------------------------------------------------------------------------
