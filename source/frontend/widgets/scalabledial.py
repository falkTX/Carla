#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import cos, floor, pi, sin
import ast

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, Qt, QEvent, QPoint, QPointF, QRectF, QTimer, QSize
    from PyQt5.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen, QPolygonF
    from PyQt5.QtWidgets import QToolTip
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, Qt, QEvent, QPoint, QPointF, QRectF, QTimer, QSize
    from PyQt6.QtGui import QColor, QLinearGradient, QRadialGradient, QConicalGradient, QFontMetrics, QPen, QPolygonF
    from PyQt6.QtWidgets import QToolTip

from .commondial import CommonDial
from carla_shared import fontMetricsHorizontalAdvance, RACK_KNOB_GAP
from carla_backend import (
    PARAMETER_NULL,
    PARAMETER_DRYWET,
    PARAMETER_VOLUME,
    PARAMETER_BALANCE_LEFT,
    PARAMETER_BALANCE_RIGHT,
    PARAMETER_PANNING,
    PARAMETER_FORTH,
    PARAMETER_MAX )

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

class ScalableDial(CommonDial):
    def __init__(self, parent, index,
                               precision,
                               default,
                               minimum,
                               maximum,
                               label,
                               paintMode,
                               colorHint    = -1,        # Hue & Sat, -1 = NotColorable
                               unit         = "%",       # Measurement Unit
                               skinStyle    = "default", # Full name (from full list)
                               whiteLabels  = 1,         # Is light/white theme?
                               tweaks       = {},
                               isInteger    = 0,         # Input is Integer
                               isButton     = 0,         # Integer i/o is Button or LED
                               isOutput     = 0,
                               isVuOutput   = 0,         # Output is analog VU meter
                               isVisible    = 1 ):

        # self.fWidth = self.fHeight = 32 # aka fImageBaseSize, not includes label.
        CommonDial.__init__(self, parent, index, precision, default, minimum, maximum, label, paintMode, colorHint, unit, skinStyle, whiteLabels, tweaks, isInteger, isButton, isOutput, isVuOutput, isVisible)

    # FIXME not every repaint need to re-calculate geometry?
    def updateSizes(self):
        knownModes = [
            # default
            self.CUSTOM_PAINT_MODE_NULL          , #  0
            self.CUSTOM_PAINT_MODE_CARLA_WET     , #  1
            self.CUSTOM_PAINT_MODE_CARLA_VOL     , #  2
            self.CUSTOM_PAINT_MODE_CARLA_L       , #  3
            self.CUSTOM_PAINT_MODE_CARLA_R       , #  4
            self.CUSTOM_PAINT_MODE_CARLA_PAN     , #  5
            self.CUSTOM_PAINT_MODE_CARLA_FORTH   , #  6
            self.CUSTOM_PAINT_MODE_CARLA_WET_MINI, #  9
            self.CUSTOM_PAINT_MODE_CARLA_VOL_MINI, # 10
            # calf
            16,
            # openav
            32, 33, 34, 37, 38,
            # zynfx
            48, 49, 50, 53, 54,
            # tube
            64, 65, 66, 69, 70,
        ]

        index = -1
        for i in range(len(knownModes)):
            if knownModes[i] == self.fCustomPaintMode:
                index = i
                break

        if (index == -1):
            print("Unknown paint mode "+ str(self.fCustomPaintMode))
            return

        self.skin    = int(self.fCustomPaintMode / 16)
        self.subSkin = int(self.fCustomPaintMode % 16)

        width, hueA, hueB, travel, radius, size, point, labelLift = [
            # default   Aqua
            [ 32, 0.50, 0.50, 260, 10, 10, 3  , 1/2, ],
            [ 32, 0.3 , 0.50, 260, 10, 10, 3  , 1/2, ], # WET
            [ 32, 0.50, 0.50, 260, 10, 10, 3  , 1/2, ], # VOL
            [ 26, 0.21, 0.21, 260,  8, 10, 2.5, 1/2, ], # L
            [ 26, 0.21, 0.21, 260,  8, 10, 2.5, 1/2, ], # R
            [ 32, 0.50, 0.50, 260, 10, 10, 3  , 1/2, ], # PAN
            [ 32, 0.50, 0.50, 260, 10, 10, 3  , 1/2, ], # FORTH
            [ 28, 0.3 , 0.50, 260,  9, 10, 2.5, 1/2, ], # WET_MINI
            [ 28, 0.50, 0.50, 260,  9, 10, 2.5, 1/2, ], # VOL_MINI
            # calf      Blue
            [ 40, 0.53, 0.53, 290, 12, 12, 4  , 1  , ], # calf absent any wet/vol knobs
            # openav   Orange
            [ 32, 0.05, 0.05, 270, 12, 12, 2.5, 2/3, ],
            [ 32, 0.30, 0.5,  270, 12, 12, 2.5, 2/3, ], # WET
            [ 32, 0.5,  0.5,  270, 12, 12, 2.5, 2/3, ], # VOL
            [ 32, 0.5,  0.5,  270, 12, 12, 2.5, 2/3, ],
            [ 32, 0.5,  0.5,  270, 12, 12, 2.5, 2/3, ],
            # zynfx     Teal
            [ 38, 0.55, 0.55, 264, 12, 12, 4  , 1/4, ],
            [ 38, 0.30, 0.5,  264, 12, 12, 4  , 1/4, ], # WET
            [ 38, 0.5,  0.5,  264, 12, 12, 4  , 1/4, ], # VOL
            [ 38, 0.5,  0.5,  264, 12, 12, 4  , 1/4, ],
            [ 38, 0.5,  0.5,  264, 12, 12, 4  , 1/4, ],
            # tube      VFD
            [ 50, 0.45, 0.45, 258, 12, 12, 4  , 1/2, ],
            [ 50, 0.45, 0.45, 258, 12, 12, 4  , 1/2, ], # WET
            [ 50, 0.45, 0.45, 258, 12, 12, 4  , 1/2, ], # VOL
            [ 50, 0.45, 0.45, 258, 12, 12, 4  , 1/2, ],
            [ 50, 0.45, 0.45, 258, 12, 12, 4  , 1/2, ],
        ] [index]

        #  Geometry & Color of controls & displays, some are tweakable:
        # 1. Try to get value from per-skin tweak;
        # 2. Then try to get value from common tweak;
        # 3. Then use default value from array.
        self.fWidth = self.fHeight = width

        # Angle span (travel)
        # calf must be 360/36*29=290
        # tube must be 360/14*10=257.14 or 360/12*10=300
        self.fTravel = int(self.getTweak('KnobTravel', travel))

        # Radius of some notable element of Knob (not exactly the largest)
        self.fRadius = int(self.getTweak('KnobRadius', radius))

        # Size of Button (half of it, similar to "raduis")
        self.fSize = int(self.getTweak('ButtonSize', size))

        # Point, line or other accent on knob
        self.fPointSize = point

        # Colouring, either only one or both values can be used for skin.
        if (self.subSkin > 0) or (self.skin in (1, 3, 4,)) :
            self.fHueA = hueA
            self.fHueB = hueB
        # default and openav can be re-colored
        elif self.colorFollow:
            self.fHueA = self.fHueB = int(self.fColorHint) / 100.0  # we use hue only yet
        else:
            # NOTE: here all incoming color data, except hue, is lost.
            self.fHueA = self.fHueB = self.fCustomPaintColor.hueF()


        metrics = QFontMetrics(self.fLabelFont)

        if not self.fLabel:
            self.fLabelWidth = 0
        else:
            self.fLabelWidth = fontMetricsHorizontalAdvance(metrics, self.fLabel)

        extraWidthAuto = max((self.fLabelWidth - self.fWidth), 0)

        self.fLabelHeight = metrics.height()

        if (self.fCustomPaintMode % 16) == 0: # exclude: DryWet, Volume, etc.
            extraWidth = int(self.getTweak('GapMin', 0))
            extraWidthLimit = int(self.getTweak('GapMax', 0))

            if self.getTweak('GapAuto', 0):
                extraWidth = max(extraWidth, extraWidthAuto)

            extraWidth = min(extraWidth, extraWidthLimit)

            self.fWidth = self.fWidth + extraWidth

        self.setMinimumSize(self.fWidth, self.fHeight + self.fLabelHeight + RACK_KNOB_GAP)
        self.setMaximumSize(self.fWidth, self.fHeight + self.fLabelHeight + RACK_KNOB_GAP)

        if not self.fLabel:
            self.fLabelHeight = 0
            # self.fLabelWidth  = 0
            return

        self.fLabelPos.setX(float(self.fWidth)/2.0 - float(self.fLabelWidth)/2.0)

        # labelLift = (1/2, 1, 2/3, 1/4, 1/2, 1, 1, 1)[skin % 8]
        self.fLabelPos.setY(self.fHeight + self.fLabelHeight * labelLift)

        # jpka: TODO Can't see how gradients work, looks like it's never triggered.
        self.fLabelGradient.setStart(0, float(self.fHeight)/2.0)
        self.fLabelGradient.setFinalStop(0, self.fHeight + self.fLabelHeight + 5)

        self.fLabelGradientRect = QRectF(float(self.fHeight)/8.0, float(self.fHeight)/2.0,
                                         float(self.fHeight*3)/4.0, self.fHeight+self.fLabelHeight+5)

    def setImage(self, imageId):
        print("Loopback for self.setupZynFxParams(), FIXME!")
        return

    def minimumSizeHint(self):
        return QSize(self.fWidth, self.fHeight)

    def sizeHint(self):
        return QSize(self.fWidth, self.fHeight)

    # def changeEvent(self, event):
    #     CommonDial.changeEvent(self, event)
    #
    #     # Force svg update if enabled state changes
    #     if event.type() == QEvent.EnabledChange:
    #         self.slot_updateImage()

    def drawMark(self, painter, X, Y, r1, r2, angle, width, color):
        A = angle * pi/180
        x = X + r1 * cos(A)
        y = Y - r1 * sin(A)
        painter.setPen(QPen(color, width, cap=Qt.RoundCap))
        if not (r1 == r2):  # line
            x1 = X + r2 * cos(A)
            y1 = Y - r2 * sin(A)
            painter.drawLine(QPointF(x, y), QPointF(x1, y1))
        else:               # ball
            painter.drawEllipse(QRectF(x-width/2, y-width/2, width, width))

    gradMachined = {5.9, 10.7, 15.7, 20.8, 25.8, 30.6, 40.6, 45.9,
                   55.9, 60.7, 65.7, 70.8, 75.8, 80.6, 90.6, 95.9}

    def grayGrad(self, painter, X, Y, a, b, gradPairs, alpha = 1.0):
        if b == -1:
            grad = QConicalGradient(X, Y, a)
        elif b == -2:
            grad = QRadialGradient (X, Y, a)
        else:
            grad = QLinearGradient (X, Y, a, b)

        for i in gradPairs:
            grad.setColorAt(int(i)/100.0, QColor.fromHslF(0, 0, (i % 1.0), alpha))

        return grad

    # Pen is always full opacity (alpha = 1)
    def grayGradPen(self, painter, X, Y, a, b, gradPairs = {0.10, 50.30, 100.10}, width = 1.0):
        painter.setPen(QPen(self.grayGrad(painter, X, Y, a, b, gradPairs, 1), width, Qt.SolidLine, Qt.FlatCap))

    def grayGradBrush(self, painter, X, Y, a, b, gradPairs, alpha = 1.0):
        painter.setBrush(self.grayGrad(painter, X, Y, a, b, gradPairs, alpha))


    # Replace Qt draw over substrate bitmap or svg to
    # all-in-one widget generated from stratch using Qt only,
    # make it highly tuneable, and uniformly look like
    # using HSL color model to make same brightness of colored things.
    # We can also easily have color tinted (themed) knobs.
    # Some things were simplified a little, to gain more speed.
    # R: knob nib (cap) radius
    def paintDial(self, painter, X, Y, H, S, L, E, normValue, enabled):
        R = self.fRadius
        barWidth = self.fPointSize
        angleSpan = self.fTravel

        hueA = self.fHueA
        hueB = self.fHueB

        color0  = QColor.fromHslF(hueA, S, L,        1)
        color0a = QColor.fromHslF(hueA, S, L/2-0.25, 1)
        color1  = QColor.fromHslF(hueB, S, L,        1)

        skin = self.skin

        def ang(value):
            return angleSpan * (0.5 - value) + 90

        def drawArcV(rect, valFrom, valTo, ticks = 0):
            # discretize scale: for 10 points, first will lit at 5%,
            # then 15%, and last at 95% of normalized value,
            # i.e. treshold is: center of point exactly matches knob mark angle
            if ticks:
                valTo = int(valTo * (ticks * angleSpan / 360) + 0.5) / (ticks * angleSpan / 360)

            painter.drawArc(rect, int(ang(valFrom) * 16), int((ang(valTo) - ang(valFrom)) * 16))

        def squareBorder(w):
            return QRectF(X-R-w, Y-R-w, (R+w)*2, (R+w)*2)

        def gray(luma):
            return QColor.fromHslF(0, 0, luma, 1)


        # Knob light arc "base" (starting) value/angle.
        if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_L:
            refValue = 0
        elif self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_CARLA_R:
            refValue = 1
        elif (self.fMinimum == -self.fMaximum) and (skin == 0):
            refValue = 0.5
        else:
            refValue = 0

        knobMuted = (self.knobPusheable and (normValue == refValue))

        haveLed = self.getTweak('WetVolPushLed', 1) and self.fCustomPaintMode in (1, 2, 5, 6, 9, 10,)

        if self.fIndex in (PARAMETER_DRYWET, PARAMETER_VOLUME, PARAMETER_PANNING, PARAMETER_FORTH): # -3, -4, -7, -9
            if knobMuted:
                if self.fIndex == PARAMETER_DRYWET:
                    self.pushLabel("Thru")
                elif self.fIndex == PARAMETER_VOLUME:
                    self.pushLabel("Mute")
                elif self.fIndex == PARAMETER_PANNING:
                    self.pushLabel("Center")
                else:
                    self.pushLabel("Midway")
            else:
                self.popLabel()

        if skin == 0: # mimic svg dial
            # if not knobMuted:
            if not (knobMuted and haveLed):
                # light arc substrate: near black, 0.5 px exposed
                painter.setPen(QPen(gray(0.10), barWidth+1, cap=Qt.FlatCap))
                drawArcV(squareBorder(barWidth), 0, 1)

                # light arc: gray bar
                # should be combined with light (value) arc to be a bit faster ?
                self.grayGradPen(painter, X, Y, 270, -1, {0.20, 100.15}, barWidth)
                drawArcV(squareBorder(barWidth), 0, 1)

            # cap
            self.grayGradBrush(painter, X-R, Y-R, R*2, -2, {0.45+E, 100.15+E})
            painter.setPen(QPen(gray(0.10), 0.5))
            painter.drawEllipse(squareBorder(1))

        elif skin == 1: # calf
            # outer chamfer & leds substrate
            self.grayGradPen(painter, X, Y, 135, -1, {0.15, 50.50, 100.15}, 1.5)
            painter.setBrush(color0a)
            painter.drawEllipse(squareBorder(barWidth*2-1))

            # machined shiny cap with chamfer
            self.grayGradPen(painter, X, Y, -45, -1, {0.15, 50.50, 100.15})
            self.grayGradBrush(painter, X, Y, 0, -1, self.gradMachined)
            painter.drawEllipse(squareBorder(1))

        elif skin == 2: # openav
            # light arc substrate
            painter.setPen(QPen(gray(0.20+E), barWidth))
            drawArcV(squareBorder(barWidth), 0, 1)

        elif skin == 3: # zynfx
            # light arc substrate
            painter.setPen(QPen(QColor.fromHslF(0.57, 0.8, 0.25, 1), barWidth+2, cap=Qt.FlatCap))
            drawArcV(squareBorder(barWidth), 0, 1)

            # cap
            painter.setPen(QPen(gray(0.0), 1))
            painter.setBrush(gray(0.3 + E))
            painter.drawEllipse(squareBorder(-2))

        # These knobs are different for integers and for floats.
        elif skin == 4: # tube / bakelite
            chamfer = 1.5 # It is best when 1.5 at normal zoom, and 1.0 for >2x HiDpi
            # base
            self.grayGradPen(painter, X, Y, -45, -1, width=chamfer)
            self.grayGradBrush(painter, X-5-ang(normValue)/36, -20, 83, -2, {0.2, 50.2, 51.00, 100.00})
            if self.fIsInteger: # chickenhead knob: small base
                painter.drawEllipse(squareBorder(1))
            else:               # round knob: larger base
                painter.drawEllipse(squareBorder(R*0.7))

            polygon = QPolygonF()
            # "chickenhead" pointer
            if self.fIsInteger:
                for i in range(17):
                    A = ((0.01, 0.02, 0.03, 0.06, 0.2, 0.3, 0.44, 0.455, -0.455, -0.44, -0.3, -0.2, -0.06, -0.03, -0.02, -0.01, 0.01)[i] * 360 - ang(normValue)) * pi/180
                    r =  (1,    0.97, 0.91, 0.7,  0.38, 0.39, 0.87, 0.9,   0.9, 0.87, 0.39, 0.38, 0.7, 0.91, 0.97, 1, 1)[i] * R
                    polygon.append(QPointF(X + r * 1.75 * cos(A), Y + r * 1.75 * sin(A)))
            # 8-teeth round knob outline
            else:
                for i in range(64):
                    A = (i / 64 * 360 - ang(normValue)) * pi/180
                    r = R * (1, 0.95, 0.91, 0.89, 0.88, 0.89, 0.91, 0.95)[i % 8]
                    polygon.append(QPointF(X + r * 1.5 * cos(A), Y + r * 1.5 * sin(A)))

            self.grayGradPen(painter, X, Y, -45, -1, {0.10, 50.50, 100.10}, chamfer)
            self.grayGradBrush(painter, X-5-ang(normValue)/36, -20, 75, -2, {0.2, 50.2, 51.00, 100.00})
            painter.drawPolygon(polygon)

            # machined shiny penny with chamfer
            self.grayGradPen(painter, X, Y, 135, -1, {0.15, 50.50, 100.15})
            self.grayGradBrush(painter, X, Y, -ang(normValue)/36, -1, self.gradMachined, 0.75)
            if self.fIsInteger: # chickenhead knob: small circle
                painter.drawEllipse(squareBorder(-R*0.65))
            else:               # round knob: large one
                painter.drawEllipse(squareBorder(-1))

            # Outer scale marks
            for i in range(0, 11):
                angle = ((0.5-i/10) * angleSpan + 90)
                self.drawMark(painter, X, Y, R*2, R*2, angle, barWidth/12 * (4 + 1 * int((i % 10) == 0)), gray(0.5 + E))

        # if knobMuted:
        if (knobMuted and haveLed):
            # if self.getTweak('WetVolPushLed', 1):
            self.drawMark(painter, X, Y, 0, 0, 0, barWidth, color0)
            return

        # draw arc: forward, or reverse (for 'R' ch knob)
        if (not (normValue == refValue)) and (not (skin == 4)):

            gradient = QConicalGradient(X, Y, 270)
            cap=Qt.FlatCap

            if not (skin == 1): # any, except calf
                ticks = 0
                gradient.setColorAt(0.75, color0)
                gradient.setColorAt(0.25, color1)
                if skin == 3: # zynfx
                    # light arc partial (angled) black substrate
                    painter.setPen(QPen(gray(0.0), barWidth+2, cap=Qt.FlatCap))
                    drawArcV(squareBorder(barWidth), refValue-0.013, normValue+0.013)
                elif skin == 2: # openav
                    cap=Qt.RoundCap
            else: # calf
                ticks = 36
                for i in range(2, ticks-2, 1):
                    gradient.setColorAt((i+0.5-0.35)/ticks, color0)
                    gradient.setColorAt((i+0.5)    /ticks, Qt.black)
                    gradient.setColorAt((i+0.5+0.35)/ticks, color0)

            painter.setPen(QPen(gradient, barWidth, Qt.SolidLine, cap))
            drawArcV(QRectF(squareBorder(barWidth)), refValue, normValue, ticks)

        # do not draw marks on disabled items
        if not enabled:
            return

        A = ang(normValue)

        match skin:
            case 0: # ball
                self.drawMark(painter, X, Y, R*0.8, R*0.8, A, barWidth/2+0.5, color0)
            case 1: # line for calf
                self.drawMark(painter, X, Y, R*0.6, R*0.9, A, barWidth/2, Qt.black)
            case 2: # line for openav
                self.drawMark(painter, X, Y, 0, R+barWidth, A, barWidth, color0)
            case 3: # line for zynfx
                self.drawMark(painter, X, Y, 2, R-3, A, barWidth/2+0.5, Qt.white)
            case 4: # ball
                r = R * (int(self.fIsInteger) * 0.25 + 1.2)
                self.drawMark(painter, X, Y, r, r, A, barWidth/2+0.5, Qt.white)


    def paintButton(self, painter, X, Y, H, S, L, E, normValue, enabled):
        # W: button cap half-size ; w: bar width
        W = self.fRadius
        w = self.fPointSize

        hue = self.fHueA

        skin = int(self.fCustomPaintMode / 16)

        def squareBorder(w, dw=0):
            return QRectF(X-W-w-dw, Y-W-w, (W+w+dw)*2, (W+w)*2)

        def gray(luma):
            return QColor.fromHslF(0, 0, luma, 1)

        color = QColor.fromHslF(hue, S, L, 1)

        centerLed = self.getTweak('ButtonHaveLed', 0) # LED itself & size increase
        coloredNeon = self.getTweak('ColoredNeon', 1) # But worse when HighContrast.

        if skin == 0: # internal
            if not centerLed:
                # light bar substrate: near black, 0.5 px exposed
                painter.setPen(QPen(gray(0.10), w+1))
                painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

                # light bar: gray bar
                painter.setPen(QPen(gray(0.20), w))
                painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

            # cap
            self.grayGradBrush(painter, X-W/2, Y-W/2, W*2, -2, {0.13+E, 50.18+E, 100.35+E})
            painter.setPen(QPen(gray(0.05), 1))
            # A bit larger buttons when no top LED, but centered one.
            painter.drawRoundedRect(squareBorder(-1+centerLed), 3, 3)

        elif skin == 1: # calf
            # outer chamfer & leds substrate
            self.grayGradPen(painter, X, Y, 135, -1, {24.25, 26.50, 76.50, 78.25}, 1.5)
            painter.setBrush(QColor.fromHslF(hue, S, 0.05+E/2, 1))
            painter.drawRoundedRect(QRectF(X-W-1, Y-W-w-0-1, W*2+2, W*2+w+0+2), 4, 4)

            # machined shiny cap with chamfer
            self.grayGradPen(painter, X, Y, -45, -1, {24.25, 26.50, 74.50, 76.25})
            self.grayGradBrush(painter, X, Y, -30, -1, self.gradMachined)
            painter.drawRoundedRect(squareBorder(-1), 3, 3)

        elif skin == 2: # openav
            # light substrate
            pen = QPen(gray(0.20+E), w)
            painter.setPen(pen)
            painter.drawRoundedRect(squareBorder(0), 3, 3)

        elif skin == 3: # zynfx
            if not centerLed:
                # light bar substrate: teal, 1 px exposed
                painter.setPen(QPen(QColor.fromHslF(hue, 0.8, 0.25, 1), w+2))
                painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))

            # button
            painter.setPen(QPen(gray(0.0), 1))
            painter.setBrush(gray(0.3 + E))
            painter.drawRoundedRect(squareBorder(-2, 4), 3, 3)

        elif skin == 4: # tube
            # bakelite cap
            self.grayGradPen(painter, X, Y, -45, -1)
            self.grayGradBrush(painter, X-10, -40, 120, -2, {0.2, 50.2, 51.00, 100.00})
            painter.drawRoundedRect(squareBorder(W*0.2), 3, 3)

            # neon lamp
            if (normValue > 0):
                grad = QRadialGradient(X, Y, 10)
                for i in ({0.6, 20.6, 70.4, 100.0}):
                    if coloredNeon:
                        grad.setColorAt(int(i)/100.0, QColor.fromHslF((0.05 - normValue) % 1.0, S, (i % 1.0), 1))
                    else:
                        grad.setColorAt(int(i)/100.0, QColor.fromHslF(0.05, S, (i % 1.0) * normValue, 1))

                painter.setPen(QPen(Qt.NoPen))
                painter.setBrush(grad)
                painter.drawRoundedRect(squareBorder(-W*0.4), 1.5, 1.5)

            # glass over neon lamp
            self.grayGradPen(painter, X, Y, 135, -1)
            self.grayGradBrush(painter, X-10, -40, 124, -2, {0.9, 50.9, 51.4, 100.4}, 0.25)
            painter.drawRoundedRect(squareBorder(-W*0.4), 1.5, 1.5)

        # draw active lights
        if skin == 0:  # internal
            if not centerLed:
                if (normValue > 0):
                    painter.setPen(QPen(color, w))
                    if (normValue < 1):
                        painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X-w/2, Y-W-w))
                    else:
                        painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2, Y-W-w))
            else:
                painter.setPen(QPen(gray(0.05), 0.5))
                painter.setBrush(color.darker(90 + int(300*(1-normValue))))
                painter.drawRoundedRect(squareBorder(w-W), 1, 1)

        elif skin == 1: # calf
            if (normValue > 0):
                grad = QLinearGradient(X-W, Y, X+W, Y)
                for i in ({20.0, 45.6, 55.6, 80.0} if (normValue < 1)
                            else {0.0, 30.6, 40.5, 45.7, 55.7, 60.5, 70.6, 100.0}):
                    grad.setColorAt(int(i)/100.0, QColor.fromHslF(hue, S, (i % 1)+E, 1))
                painter.setPen(QPen(grad, w-0.5, cap=Qt.FlatCap))
                painter.drawLine(QPointF(X-W, Y-W-w/2), QPointF(X+W, Y-W-w/2))

        elif skin == 2:  # openav
            painter.setPen(QPen(color, w, cap=Qt.RoundCap))
            if (normValue > 0):
                painter.drawRoundedRect(squareBorder(-W * (1 - normValue)), 3, 3)
            else:
                painter.drawLine(QPointF(X-0.1, Y), QPointF(X+0.1, Y))

        elif skin == 3:  # zynfx
            if not centerLed:
                if (normValue > 0):
                    dx = (W - w) if (normValue < 1) else 0
                    painter.setPen(QPen(gray(0), w+2))
                    painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2-dx, Y-W-w))
                    painter.setPen(QPen(color, w))
                    painter.drawLine(QPointF(X-W/2, Y-W-w), QPointF(X+W/2-dx, Y-W-w))
            else:
                painter.setPen(QPen(gray(0), 1))
                painter.setBrush(color.darker(90 + int(300*(1-normValue))))
                painter.drawEllipse(squareBorder(w-W+1))

        # do not draw marks on disabled items
        if not enabled:
            return

        match skin:
            case 0: # internal: ball at center
                if not centerLed:
                    self.drawMark(painter, X, Y, 0, 0, 0, w/2+0.5, color)
            # case 3: # openav
            #     painter.setPen(QPen(color, w, cap=Qt.RoundCap))
            #     painter.drawLine(QPointF(X-0.1, Y), QPointF(X+0.1, Y))
            case 3: # zynfx: ball at center
                if not centerLed:
                    self.drawMark(painter, X, Y, 0, 0, 0, w/2, gray(1))


    # Just a text label not so good for fast updated display, see issue #1934.
    # NOTE Work in progress.
    def paintDisplay(self, painter, X, Y, H, S, L, E, normValue, enabled):

        # X, Y: Center of label.
        def plotStr(self, painter, X, Y, st, fontSize, aspectRatio, skew):

            # Due to CPU/speed gain, we use simplest possible 7-segmented digits.
            # Shape to Speed balance: Speed
            h = ["KYNKNY ROZUZ RVKVY ROJUJ",    # 0        NJ RJ VJ
                 "KYVJVQ RVSVZ",                # 1        NR RR VR
                 "KYNJVJVRNRNZVZ",              # 2        NZ RZ VZ
                 "KYNJVJVZNZ RVRNR",            # 3          P]
                 "KYNJNRVR RVJVZ",              # 4
                 "KYVJNJNRVRVZNZ",              # 5
                 "KYVJNJNZVZVRNR",              # 6
                 "KYNJVJVZ",                    # 7
                 "KYNJNZVZVJNJ RNRVR",          # 8
                 "KYNZVZVJNJNRVR",              # 9
                 "KYNRVR",                      # -
                 "OTRZP]",                      # .
                 "KYNNNX RVPNTVX",              # k
                 "KYNXNLRRVLVX"     ]           # M

            def plotHersheyChar(painter, X, Y, c, fontSize, aspectRatio, skew, justGetWidth):
                lm = (ord(h[c][0]) - ord('R')) * fontSize * aspectRatio
                rm = (ord(h[c][1]) - ord('R')) * fontSize * aspectRatio
                if justGetWidth:
                    return X + rm - lm

                points = []
                X = X - lm
                # The speed and CPU load is critical here.
                # I try to make it as efficient as possible, but can it be even faster?
                for i in range(1, int(len(h[c])/2)):
                    a = (h[c][i*2])
                    b = (h[c][i*2+1])
                    if (a == ' ') and (b == 'R'):
                        painter.drawPolyline(points)
                        points = []
                    else:
                        y = (ord(b) - ord('R')) * fontSize
                        x = (ord(a) - ord('R')) * fontSize * aspectRatio + skew * y
                        points.append(QPointF(X+x, Y+y))

                painter.drawPolyline(points)
                X = X + rm
                return X

            def plotDecodedChar(painter, X, Y, st, fontSize, aspectRatio, skew, justGetWidth):
                for i in range(len(st)):
                    digit = "0123456789-.kM".find(st[i])
                    if digit < 0:
                        print("ERROR: Illegal char at " + str(i) + " in " + st)
                    else:
                        X = plotHersheyChar(painter, X, Y, digit, fontSize, aspectRatio, skew, justGetWidth)
                return X

            widthPx = plotDecodedChar(painter, 0, Y, st, fontSize, aspectRatio, skew, 1)
            plotDecodedChar(painter, X-widthPx/2, Y, st, fontSize, aspectRatio, skew, 0)
            return

        def strLimDigits(x):
            s = str(x)
            ret = lambda x: float(x) if '.' in s else int(x)
            return str(ret(s[:max(s.find('.'), 4+1)].strip('.')))
#            return str(ret(s[:max(s.find('.'), num+1 + ('-' in s))].strip('.')))

        def plotNixie(n):
            painter.setPen(QPen(QColor.fromHslF(0.05, S, L, 1), 2.5, cap=Qt.RoundCap))

            # We use true arcs instead of polyline/Bezier.
            # Arcs are perfectly matched with original tube.
            # x = 0..20, y = 0..32
            digits = [[[ 2,00,18,16, 480,2400],[   00,-8,48,40,2400,3360],
                       [ 2,16,18,32,3360,5280],[  -28,-8,20,40,5280,6240]],  # 0
                      [[10,00,10,32,   0,   0]],                             # 1
                      [[ 1,-0.5,19,17.5,4608,8640],[1,17,30,46,1680,2880],
                        [1,31.5,19,31.5,   0,   0]],                         # 2
                      [[-2,10,20,32,3500,7300],[    2,00,19,00,   0,   0],
                       [19,00, 8,10,   0,   0]],                             # 3
                      [[ 1,22,17,00,   0,   0],[   17,00,17,32,   0,   0],
                        [1,22,17,22,   0,   0]],                             # 4
                      [[-1,12,19,32,3500,7920],[    4,00,18,00,   0,   0],
                        [4,00, 2,14,   0,   0]],                             # 5
                      [[00,12,20,32,   0,5760],[   0,-10,64,54,2150,2880]],  # 6
                      [[ 1,00,19,00,   0,   0],[   19,00, 8,32,   0,   0]],  # 7
                      [[ 1,14,19,32,   0,5760],[    3,00,17,14,   0,5760]],  # 8
                      [[00,00,20,20,   0,5760],[ 20,42,-44,-22,5030,5760]]]  # 9

            for x0, y0, x1, y1, a0, a1 in digits[n]:
                if a0 == a1 == 0:
                    painter.drawLine(QPointF(x0+X-10, y0+Y-16), QPointF(x1+X-10, y1+Y-16))
                else:
                    rect = QRectF(x0+X-10, y0+Y-16, x1-x0, y1-y0)
                    painter.drawArc(rect, a0, a1-a0)

        def squareBorder(w, dw=0):
            return QRectF(X-W-w-dw, Y-W-w, (W+w+dw)*2, (W+w)*2)

        W = Y-1                             # "radius"
        value = self.fRealValue
        hue = self.fHueA

        # if self.fIsButton: # TODO make it separate paintLED
        if (self.fIsInteger and (self.fMinimum == 0) and (self.fMaximum == 1)): # TODO
            # Neon lamp
            if (self.fCustomPaintMode == 64): # tube
                # bakelite lamp holder
                self.grayGradPen(painter, X, Y, -45, -1)
                self.grayGradBrush(painter, X-10, -40, 120, -2, {0.2, 50.2, 51.00, 100.00})
                painter.drawRoundedRect(squareBorder(-W*0.45), 3, 3)

                # neon lamp
                if (normValue > 0):
                    grad = QRadialGradient(X, Y, 13)
                    for i in ({0.6, 20.6, 70.4, 100.0}):
                        grad.setColorAt(int(i)/100.0, QColor.fromHslF(0.05, 1.0, (i % 1.0) * normValue, 1))
                    painter.setPen(QPen(Qt.NoPen))
                    painter.setBrush(grad)
                    painter.drawRoundedRect(squareBorder(-W*0.6), 1.5, 1.5)

                # glass over neon lamp
                self.grayGradPen(painter, X, Y, 135, -1)
                self.grayGradBrush(painter, X-10, -40, 124, -2, {0.9, 50.9, 51.4, 100.4}, 0.25)
                painter.drawRoundedRect(squareBorder(-W*0.6), 1.5, 1.5)
                return

            painter.setPen(QPen(QColor.fromHslF(0, 0, 0.3-0.15*normValue+E, 1), 1.5))
            painter.setBrush(QColor(QColor.fromHslF(hue, S, L*(normValue*0.8+0.1), 1)))
            painter.drawRoundedRect(squareBorder(-W/2-2), 1.5, 1.5)
            return

        if (self.fCustomPaintMode == 64) and \
        (self.fIsInteger and (self.fMinimum >= 0) and (self.fMaximum < 20)):
            #Nixie tube for 0..9, or 1 1/2 tubes for 11..19
            Y = Y - 2
            self.grayGradPen(painter, X, Y, 135, -1)
            self.grayGradBrush(painter, X-10, -40, 120, -2, {0.2, 50.2, 51.00, 100.00})
            painter.drawRoundedRect(squareBorder(-2, -W*0.2), 3, 3)

            if (value < 10):
                plotNixie(int(value % 10))
            else:
                if (value == 11):
                    X = X + 8
                    plotNixie(1)
                else:
                    X = X + 4
                    plotNixie(int(value % 10))
                X = X - 16
                plotNixie(1)
            return

        # Will it be analog display, or digital 7-segment scale?
        if not self.fIsVuOutput:
            unit = ""
            if abs(value) >= 10000.0:
                value = value / 1000.0
                unit = "k"
            if abs(value) >= 10000.0:
                value = value / 1000.0
                unit = "M"
            # Remove trailing decimal zero and decimal point also.
            if (value % 1.0) == 0:
                value = int(value)

            valueStr = strLimDigits(value) + unit
            valueLen = len(valueStr)
            if valueLen == 0:
                print("Zero length string from " + str(self.fRealValue) + " value.")
                return

        autoFontsize  = int(self.getTweak('Auto7segSize', 0))       # Full auto
        autoFontwidth = int(self.getTweak('Auto7segWidth', 1))      # Width only

        skew = -0.2
        substrate = 1
        dY = 3                      # Work in progress here. NOTE
        fntSize = 0.9
        fntAspect = 0.5
        bgLuma = 0.05
        R = 10  # Replace to W
        width = 4
        lineWidth = 2.0

        if self.fCustomPaintMode == 0:     # default / internal
            fntSize = 0.75
            Y = Y - 2

        elif self.fCustomPaintMode == 16:  # calf
            autoFontsize = 1
            skew = 0
            dY = 4
            bgLuma = 0.1
            R = 12

        elif self.fCustomPaintMode == 32:  # openav
            autoFontsize = 1
            substrate = 0
            R = 13
            lineWidth = 3.0

        elif self.fCustomPaintMode == 48:  # zynfx
            fntSize = 0.99
            Y = Y - 2
            dY = 4
            bgLuma = 0.12
            R = 12

        elif self.fCustomPaintMode == 64:  # tube
            autoFontsize = 1
            R = 13
            lineWidth = 3.0

        else:
            print("Unknown paint mode "+ str(self.fCustomPaintMode) + " display.")
            return

        if not self.fIsVuOutput:
            if autoFontwidth and (valueLen < 4):
                if autoFontsize:
                    fntSize = fntSize * 4/3
                fntAspect = 1.0 - (valueLen-1) * 0.2

            lineWidth = fntSize + 0.5                      # Work in progress here. NOTE

        # substrate
        if substrate:
            substratePen = QPen(QColor.fromHslF(0, 0, 0.4+E, 1), 0.5)
            if (self.fCustomPaintMode == 64): # tube
                if not self.fIsVuOutput:
                    self.grayGradPen(painter, X, Y, 135, -1)
                    self.grayGradBrush(painter, X-10, -80, 200, -2, {0.2, 50.2, 51.00, 100.00})
                    painter.drawRoundedRect(squareBorder(-W*0.4, W*0.3), 3, 3)

            else:
                if self.fCustomPaintMode == 16:  # calf
                    if self.fIsVuOutput:
                        dY = 9
                painter.setPen(substratePen)
                painter.setBrush(QColor(QColor.fromHslF(0, 0, bgLuma, 1)))
                painter.drawRoundedRect(squareBorder(-dY, dY), 3, 3)


        color = QColor.fromHslF(hue, S, L, 1)
        painter.setPen(QPen(color, lineWidth, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
        if not self.fIsVuOutput:
            plotStr(self, painter, X, Y, valueStr, fntSize, fntAspect, skew);
        else:
            if self.fCustomPaintMode == 16:  # calf        # Work in progress here. NOTE
                for i in range(0, 10+1):
                    if normValue > ((i+0.5)/11):
                        painter.drawLine(QPointF(X-15+i*3, Y-R/2), QPointF(X-15+i*3, Y+R/2))
            elif self.fCustomPaintMode == 64:  # tube        # Work in progress here. NOTE
                # Draw Cat eye.
                chamfer = 1.5 # It is best when 1.5 at normal zoom, and 1.0 for >2x HiDpi

                # base
                self.grayGradPen(painter, X, Y, -45, -1, width=chamfer)
                self.grayGradBrush(painter, X-10, -20, 83, -2, {0.2, 50.2, 51.00, 100.00})
                painter.drawEllipse(squareBorder(-W*0.2))

                # green sectors
                rays = 4 # There are 4- or 8-rays (2 or 4 notches) tubes
                gradient = QConicalGradient(X, Y, 0)
                for i in range(rays):
                    sign = 1 - (i % 2) * 2
                    v = min(normValue, 1) * 1.2 + 0.05  # For output, it can be > max.
                    a = (i % 2) + v * sign
                    b = a + 0.02 * sign
                    if v > 0.99 :
                        # did you notice overlapping sectors?
                        gradient.setColorAt((i+a)/rays, color)
                        gradient.setColorAt((i+b)/rays, color.darker(130))
                    else:
                        b = max(-1, min(1, b))
                        gradient.setColorAt((i+a)/rays, color.darker(130))
                        gradient.setColorAt((i+b)/rays, color.darker(300))

                self.grayGradPen(painter, X, Y, 0, -1, width=chamfer)
                painter.setBrush(gradient)
                painter.drawEllipse(squareBorder(-W*0.4))

                # cap is black itself, but looks dark green on working tube.
                self.drawMark(painter, X, Y, 0, 0, 0, W/4, color.darker(800))

            else:
                # VU scale points
                for i in range(0, 5+1):
                    angle = ((0.5-i/5) * 110 + 90)
                    self.drawMark(painter, X, Y + R*0.6, R*1.3, R*1.5, angle, lineWidth-0.5, color.darker(150))

                # VU pointer
                angle = ((0.5-normValue) * 110 + 90)
                self.drawMark(painter, X, Y + R*0.6, 0, R*1.3, angle, lineWidth, color)

                # Draw "settling screw" of VU meter        # Work in progress here. NOTE
                if substrate:
                    painter.setPen(substratePen)
                    painter.drawEllipse(QRectF(X-width, Y+R*0.6-width, width*2, width*2))

# ---------------------------------------------------------------------------------------------------------------------
