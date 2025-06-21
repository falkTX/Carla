#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import cos, floor, pi, sin, copysign
import numpy as np

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, Qt, QEvent, QPoint, QPointF, QRectF, QTimer, QSize
    from PyQt5.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen
    from PyQt5.QtWidgets import QToolTip
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, Qt, QEvent, QPoint, QPointF, QRectF, QTimer, QSize
    from PyQt6.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen
    from PyQt5.QtWidgets import QToolTip

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

    def checkEnabled(self):
        if self.isEnabled():
            if self.HOVER_MIN < self.fHoverStep < self.HOVER_MAX:
                self.fHoverStep += 1 if self.fIsHovered else -1
                QTimer.singleShot(20, self.update)
            return 1
        else:
            return 0

    def normValue(self):
        return float(self.fRealValue - self.fMinimum) / float(self.fMaximum - self.fMinimum)

    def drawMark(self, painter, X, Y, r1, r2, angle, width, color):
        x = X + r1 * cos(angle)
        y = Y - r1 * sin(angle)
        painter.setPen(QPen(color, width, cap=Qt.RoundCap))
        if not (r1 == r2):  # line
            x1 = X + r2 * cos(angle)
            y1 = Y - r2 * sin(angle)
            painter.drawLine(QPointF(x, y), QPointF(x1, y1))
        else:               # ball
            painter.drawEllipse(QRectF(x-width/2, y-width/2, width, width))

    def gradMachined(self):
        return {5.9, 10.7, 15.7, 20.8, 25.8, 30.6, 40.6, 45.9,
               55.9, 60.7, 65.7, 70.8, 75.8, 80.6, 90.6, 95.9}

    def grayGrad(self, painter, X, Y, a, b, gradPairs):
        if b == -1:
            grad = QConicalGradient(X, Y, a)
        elif b == -2:
            grad = QRadialGradient (X, Y, a)
        else:
            grad = QLinearGradient (X, Y, a, b)

        for i in gradPairs:
            grad.setColorAt(int(i)/100.0, QColor.fromHslF(0, 0, (i % 1.0), 1))

        return grad


    def paintDial(self, painter):

        # Replace Qt draw over substrate bitmap or svg to
        # all-in-one widget generated from stratch using Qt only,
        # make it highly tuneable, and uniformly look like
        # using HSL color model to make same brightness of colored things.
        # We can also easily have color tinted (themed) knobs.
        # Some things were simplified a little, to gain more speed.
        # R: knob nib (cap) radius
        def createDial(R, hue1, hue2, barWidth, angleSpan, ticks, dialType, value):

            def rectBorder(w):
                return QRectF(X-R-w, Y-R-w, (R+w)*2, (R+w)*2)

            def gray(luma):
                return QColor.fromHslF(0, 0, luma, 1)

            X = Y = self.fImageBaseSize / 2     # center

            ang0 = int((angleSpan/2+90)*16)
            ang1 = -angleSpan*16

            enabled = self.checkEnabled()
            E = enabled * self.fHoverStep / 40  # enlight
            S = enabled * 0.9                   # saturation
            color1 = QColor.fromHslF(hue1, S, 0.5+E, 1)
            color2 = QColor.fromHslF(hue2, S, 0.5+E, 1)

            if dialType == 1: # mimic svg dial
                # light arc substrate: near black, 0.5 px exposed
                painter.setPen(QPen(gray(0.10), barWidth+1, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)

                # light arc: gray bar
                # should be combined with light (value) arc to be a bit faster ?
                painter.setPen(QPen(self.grayGrad(painter, X, Y, 270, -1, {0.20, 100.15}), barWidth, cap=Qt.FlatCap))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)

                # cap
                painter.setBrush(self.grayGrad(painter, X-R, Y-R, R*2, -2, {0.45+E, 100.15+E}))
                painter.setPen(QPen(gray(0.10), 0.5))
                painter.drawEllipse(rectBorder(1))

            elif dialType == 2: # calf
                # outer chamfer & leds substrate
                painter.setPen(QPen(self.grayGrad(painter, X, Y, 135, -1, {0.15, 50.50, 100.15}), 1.5, cap=Qt.FlatCap))
                painter.setBrush(QColor.fromHslF(hue1, S, 0.05+E/2, 1))
                painter.drawEllipse(rectBorder(barWidth*2-1))

                # machined shiny cap with chamfer
                painter.setPen(QPen(self.grayGrad(painter, X, Y, -45, -1, {0.15, 50.50, 100.15}), 1, cap=Qt.FlatCap))
                painter.setBrush(self.grayGrad(painter, X, Y, 0, -1, self.gradMachined()))
                painter.drawEllipse(rectBorder(1))

            elif dialType == 3: # openav
                # light arc substrate
                painter.setPen(QPen(gray(0.20+E), barWidth))
                painter.drawArc(rectBorder(barWidth), ang0, ang1)


            # draw arc: forward, or reverse (for 'R' ch knob)
            startAngle = int((copysign(angleSpan/2, value) + 90)*16)
            gradient = QConicalGradient(X, Y, 270)

            if (ticks == 0):
                spanAngle = int(-angleSpan*16 * value)
                gradient.setColorAt(0.25, color1)
                gradient.setColorAt(0.75, color2)
            else:
                # discretize scale: for 10 points, first will lit at 5%,
                # then 15%, and last at 95% of normalized value,
                # i.e. treshold is: center of point exactly matches knob mark angle
                spanAngle = int(-int(angleSpan*value/(360/ticks)+0.5)*(360/ticks)*16)
                for i in range(2, ticks-2, 1):
                    gradient.setColorAt((i+0.5-0.35)/ticks, color1.lighter(100))
                    gradient.setColorAt((i+0.5)    /ticks, Qt.black)
                    gradient.setColorAt((i+0.5+0.35)/ticks, color1.lighter(100))

            painter.setPen(QPen(gradient, barWidth, cap=Qt.RoundCap if dialType == 3 else Qt.FlatCap))
            painter.drawArc(QRectF(rectBorder(barWidth)), startAngle, spanAngle)

            # do not draw marks on disabled items
            if enabled == 0:
                return

            # Forward or reverse (for 'R' ch knob)
            angle = ((0.5-abs(value)) * copysign(angleSpan, value) + 90) * pi/180

            match dialType:
                case 1: # ball
                    self.drawMark(painter,X,Y, R*0.8, R*0.8, angle, barWidth/2, color1)
                case 2: # line for calf
                    self.drawMark(painter,X,Y, R*0.6, R*0.9, angle, barWidth/2, Qt.black)
                case 3: # line for openav
                    self.drawMark(painter,X,Y, 0, R+barWidth, angle, barWidth, color1)


        # Custom knobs (Dry/Wet and Volume)
        if self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_WET, self.CUSTOM_PAINT_MODE_CARLA_VOL):
            hue2 = 0.5 # Blue
            hue1 = hue2 if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_VOL else 0.3 # Green

            createDial(10, hue1, hue2, 3, 260, 0, 1, self.normValue())

        # Custom knobs (L and R)
        elif self.fCustomPaintMode in (self.CUSTOM_PAINT_MODE_CARLA_L, self.CUSTOM_PAINT_MODE_CARLA_R):
            hue = 0.21 # Lime

            # Shift to full negative range (incl. 0) for reversed knob
            createDial(8, hue, hue, 2.5, 260, 0, 1, self.normValue()-1.0-np.nextafter(0, 1) if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_R else self.normValue())

        # Custom knobs (Color)                         # internal
        elif self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_COLOR:
            # NOTE: here all incoming color data, except hue, is lost.
            hue = (self.fCustomPaintColor.hueF() - 0.05) % 1.0

            createDial(10, hue, hue, 3, 260, 0, 1, self.normValue())

        # Custom knobs
        elif self.fImageId in (11, 12, 13):            # openav
            if self.fImageId == 11:
                hue1 = hue2 = 0.05 # Orange
            else:
                hue2 = 0.5 # Blue
                hue1 = hue2 if self.fImageId == 12 else 0.3 # Green

            createDial(12, hue1, hue2, 2.5, 270, 0, 3, self.normValue())

        elif self.fImageId in (1, 2, 7, 8, 9, 10):     # calf
            hue = 0.52 # Aqua

            createDial(12, hue, hue, 4, (360/36)*29, 36, 2, self.normValue())

        else:
            print("Unknown type "+ str(self.fImageId) + " knob.")
            return


    def paintButton(self, painter, range_):

        # W: button cap half-size ; w: bar width
        # positions: can be 2 or 3
        def createButton(W, hue, w, positions, btnType, value):

            def rectBorder(w):
                return QRectF(X-W-w, Y-W-w, (W+w)*2, (W+w)*2)

            def gray(luma):
                return QColor.fromHslF(0, 0, luma, 1)

            X = Y = self.fImageBaseSize / 2     # center

            enabled = self.checkEnabled()
            E = enabled * self.fHoverStep / 40  # enlight
            S = enabled * 0.9                   # saturation
            color = QColor.fromHslF(hue, S, 0.5+E, 1)

            if btnType == 1: # internal
                # light bar substrate: near black, 0.5 px exposed
                painter.setPen(QPen(gray(0.10), w+1))
                painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

                # light bar: gray bar
                painter.setPen(QPen(gray(0.20), w))
                painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

                # cap
                painter.setBrush(self.grayGrad(painter, X-W/2, Y-W/2, W*2, -2, {0.15+E, 100.40+E}))
                painter.setPen(QPen(gray(0.05), 0.5))
                painter.drawRoundedRect(rectBorder(-1), 3, 3)

            elif btnType == 2: # calf
                # outer chamfer & leds substrate
                painter.setPen(QPen(self.grayGrad(painter, X, Y, 135, -1, {24.25, 26.50, 76.50, 78.25}), 1.5, cap=Qt.FlatCap))
                painter.setBrush(QColor.fromHslF(hue, S, 0.05+E/2, 1))
                painter.drawRoundedRect(QRectF(X-W-1, Y-W-w-0-1, W*2+2, W*2+w+0+2), 4, 4)

                # machined shiny cap with chamfer
                painter.setPen(QPen(self.grayGrad(painter, X, Y, -45, -1, {24.25, 26.50, 74.50, 76.25}), 1, cap=Qt.FlatCap))
                painter.setBrush(self.grayGrad(painter, X, Y, -30, -1, self.gradMachined(                )))
                painter.drawRoundedRect(rectBorder(-1), 3, 3)

            elif btnType == 3: # openav
                # light substrate
                pen = QPen(gray(0.20+E), w)
                painter.setPen(pen)
                painter.drawRoundedRect(rectBorder(0), 3, 3)


            # draw active lights
            if (value > (1/2-(positions-2)*(2/3-1/2))):
                if (1/3 < value < 2/3) and (positions == 3):
                    pos = 1 # Middle
                else:
                    pos = 2 # Max
            else:
                pos = 0

            if btnType == 1:  # internal
                if (pos > 0):
                    painter.setPen(QPen(color, w))
                    if (pos == 1):
                        painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X-w/2, Y-W-w))
                    else:
                        painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

            elif btnType == 2: # calf
                if (pos > 0):
                    grad = QLinearGradient(X-W, Y, X+W, Y)
                    for i in ({0.0, 30.6, 40.5, 45.7, 55.7, 60.5, 70.6, 100.0} if (pos>1)
                              else {20.0, 45.6, 55.6, 80.0}):
                        grad.setColorAt(int(i)/100.0, QColor.fromHslF(hue, S, (i % 1)+E, 1))
                    painter.setPen(QPen(grad, w-0.5, cap=Qt.FlatCap))
                    painter.drawLine(QPointF(X-W, Y-W-w/2), QPointF(X+W, Y-W-w/2))

            elif btnType == 3:  # openav
                painter.setPen(QPen(color, w, cap=Qt.RoundCap))
                if (pos > 0):
                    if (pos == 1):
                        painter.drawRoundedRect(rectBorder(-W/2), 3, 3)
                    else:
                        painter.drawRoundedRect(rectBorder(0), 3, 3)
                else:
                    painter.drawLine(QPointF(X-0.1, Y), QPointF(X+0.1, Y))


            # do not draw marks on disabled items
            if enabled == 0:
                return

            match btnType:
                case 1: # ball at center
                    self.drawMark(painter,X,Y, 0, 0, 0, w/2, color)
                # case 3: # openav
                #     painter.setPen(QPen(color, w, cap=Qt.RoundCap))
                #     painter.drawLine(QPointF(X-0.1, Y), QPointF(X+0.1, Y))


        positions = range_ + 1;

        # Custom knobs (Color)                         # internal
        if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_COLOR:
            # NOTE: here all incoming color data, except hue, is lost.
            hue = (self.fCustomPaintColor.hueF() - 0.05) % 1.0

            createButton(10, hue, 3, positions, 1, self.normValue())

        # Custom knobs
        elif self.fImageId in (1, 2, 7, 8, 9, 10):     # calf
            hue = 0.55 # Sky

            createButton(12, hue, 4, positions, 2, self.normValue())

        elif self.fImageId in (11, 12, 13):            # openav
            hue = 0.05 # Orange

            createButton(12, hue, 2.5, positions, 3, self.normValue())

        else:
            print("Unknown type "+ str(self.fImageId) + " button.")
            return

# ---------------------------------------------------------------------------------------------------------------------
