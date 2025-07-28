#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import isnan, log10

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QPoint, QPointF, QRectF, QEvent, QTimer
    from PyQt5.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt5.QtWidgets import QWidget, QToolTip, QInputDialog
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, pyqtSlot, Qt, QPoint, QPointF, QRectF, QEvent, QTimer
    from PyQt6.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt6.QtWidgets import QWidget, QToolTip, QInputDialog

from carla_shared import strLim
from widgets.paramspinbox import CustomInputDialog
from carla_backend import PARAMETER_NULL, PARAMETER_DRYWET, PARAMETER_VOLUME, PARAMETER_BALANCE_LEFT, PARAMETER_BALANCE_RIGHT, PARAMETER_PANNING, PARAMETER_FORTH

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

# to be implemented by subclasses
    #def updateSizes(self):
    #def paintDial(self, painter):

class CommonDial(QWidget):
    # enum CustomPaintMode
    CUSTOM_PAINT_MODE_NULL        = 0 # default (NOTE: only this mode has label gradient)
    CUSTOM_PAINT_MODE_CARLA_WET   = 1 # color blue-green gradient (reserved #3)
    CUSTOM_PAINT_MODE_CARLA_VOL   = 2 # color blue (reserved #3)
    CUSTOM_PAINT_MODE_CARLA_L     = 3 # color yellow (reserved #4)
    CUSTOM_PAINT_MODE_CARLA_R     = 4 # color yellow (reserved #4)
    CUSTOM_PAINT_MODE_CARLA_PAN   = 5 # color yellow (reserved #3)
    CUSTOM_PAINT_MODE_CARLA_FORTH = 6 # Experimental
    CUSTOM_PAINT_MODE_COLOR       = 7 # May be deprecated (unless zynfx internal mode)
    CUSTOM_PAINT_MODE_NO_GRADIENT = 8 # skip label gradient
    CUSTOM_PAINT_MODE_CARLA_WET_MINI =  9 # for compacted slot
    CUSTOM_PAINT_MODE_CARLA_VOL_MINI = 10 # for compacted slot

    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    HOVER_MIN = 0
    HOVER_MAX = 9

    MODE_DEFAULT = 0
    MODE_LINEAR = 1

    # signals
    dragStateChanged = pyqtSignal(bool)
    realValueChanged = pyqtSignal(float)

    def __init__(self, parent, index, precision, default, minimum, maximum, label, paintMode, colorHint, unit, skinStyle, whiteLabels, tweaks, isInteger, isButton, isOutput, isVuOutput, isVisible):
        QWidget.__init__(self, parent)

        self.fIndex           = index
        self.fPrecision       = precision
        self.fDefault         = default
        self.fMinimum         = minimum
        self.fMaximum         = maximum
        self.fCustomPaintMode = paintMode
        self.fColorHint       = colorHint
        self.fUnit            = unit
        self.fSkinStyle       = skinStyle
        self.fWhiteLabels     = whiteLabels
        self.fTweaks          = tweaks
        self.fIsInteger       = isInteger
        self.fIsButton        = isButton
        self.fIsOutput        = isOutput
        self.fIsVuOutput      = isVuOutput
        self.fIsVisible       = isVisible

        self.fDialMode = self.MODE_LINEAR

        self.fLabel     = label
        self.fLastLabel = ""
        self.fRealValue = 0.0
        self.fLastValue = self.fDefault

        self.fScalePoints = []
        self.fNumScalePoints = 0
        self.fScalePointsPrefix = ""
        self.fScalePointsSuffix = ""

        self.fIsHovered = False
        self.fIsPressed = False
        self.fHoverStep = self.HOVER_MIN

        self.fLastDragPos = None
        self.fLastDragValue = 0.0

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

        self.fCustomPaintColor = QColor(0xff, 0xff, 0xff)

        self.addContrast    = int(bool(self.getTweak('HighContrast', 0)))
        self.colorFollow    = bool(self.getTweak('ColorFollow', 0))
        self.knobPusheable  = bool(self.getTweak('WetVolPush', 0))
        self.displayTooltip = bool(self.getTweak('Tooltips', 1))

        # We have two group of knobs, non-repaintable (like in Edit dialog) and normal.
        # For non-repaintable, we init sizes/color once here;
        # for normals, it should be (re)inited separately: we do not init it here
        # to save CPU, some parameters are not known yet, repaint need anyway.
        if self.fColorHint == -1:
            self.updateSizes()

        self.update()

#        self.valueChanged.connect(self.slot_valueChanged)  # FIXME

    def forceWhiteLabelGradientText(self):
        self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
        self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
        self.fLabelGradientColorT = [Qt.white, Qt.darkGray]

    # def setLabelColor(self, enabled, disabled):
    #     self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
    #     self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
    #     self.fLabelGradientColorT = [enabled, disabled]

    def getIndex(self):
        return self.fIndex

    def rvalue(self):
        return self.fRealValue

    def pushLabel(self, label):
        if self.fLastLabel == "":
            self.fLastLabel = self.fLabel
            self.fLabel = label
            self.updateSizes()
            self.update()

    def popLabel(self):
        if not (self.fLastLabel == ""):
            self.fLabel = self.fLastLabel
            self.fLastLabel = ""
            self.updateSizes()
            self.update()

    def setScalePPS(self, scalePoints, prefix, suffix):
        self.fScalePoints = scalePoints
        self.fNumScalePoints = len(self.fScalePoints)
        self.fScalePointsPrefix = prefix
        self.fScalePointsSuffix = suffix

    def setValue(self, value, emitSignal=False):
        if self.fRealValue == value or isnan(value):
            return

        if (not self.fIsOutput) and value <= self.fMinimum:
            self.fRealValue = self.fMinimum

        elif (not self.fIsOutput) and value >= self.fMaximum:
            self.fRealValue = self.fMaximum

        elif self.fIsInteger or (abs(value - int(value)) < 1e-8): # tiny "notch"
            self.fRealValue = round(value)

        else:
            self.fRealValue = value

        if emitSignal:
            self.realValueChanged.emit(self.fRealValue)

        self.update()

    def setCustomPaintColor(self, color):
        if self.fCustomPaintColor == color:
            return

        self.fCustomPaintColor = color
        self.updateSizes()
        self.update()

    def getTweak(self, tweakName, default):
        return self.fTweaks.get(self.fSkinStyle + tweakName, self.fTweaks.get(tweakName, default))

    def getIsVisible(self):
        # print (self.fIsVisible)
        return self.fIsVisible

    @pyqtSlot(int)
    def slot_valueChanged(self, value):
        self.fRealValue = float(value)/self.fPrecision * (self.fMaximum - self.fMinimum) + self.fMinimum
        self.realValueChanged.emit(self.fRealValue)

    # jpka: TODO should be replaced by common dialog, but
    # PluginEdit.slot_knobCustomMenu(...) - not found, import not work.
    # So this is copy w/o access to 'step's.
    def knobCustomInputDialog(self):
        if self.fIndex < PARAMETER_NULL:
            percent = 100.0
        else:
            percent = 1

        if self.fIsInteger:
            step = max(1, int((self.fMaximum - self.fMinimum)/100))
            stepSmall = max(1, int(step/10))
        else:
            step = 10 ** (round(log10((self.fMaximum - self.fMinimum) * percent))-2)
            stepSmall = step / 100

        dialog = CustomInputDialog(self, self.fLabel, self.fRealValue * percent, self.fMinimum * percent, self.fMaximum * percent, step, stepSmall, self.fScalePoints, "", "", self.fUnit)
        if not dialog.exec_():
            return

        self.setValue(dialog.returnValue() / percent, True)


    def enterEvent(self, event):
        self.setFocus()
        self.fIsHovered = True
        if self.fHoverStep == self.HOVER_MIN:
            self.fHoverStep = self.HOVER_MIN + 1
        self.update()


    def leaveEvent(self, event):
        self.fIsHovered = False
        if self.fHoverStep == self.HOVER_MAX:
            self.fHoverStep = self.HOVER_MAX - 1
        self.update()


    def nextScalePoint(self):
        for i in range(self.fNumScalePoints):
            value = self.fScalePoints[i]['value']
            if value > self.fRealValue:
                self.setValue(value, True)
                return
        self.setValue(self.fScalePoints[0]['value'], True)


    def mousePressEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT or self.fIsOutput:
            return

        if event.button() == Qt.LeftButton:
            # if self.fNumScalePoints:
            #     self.nextScalePoint()
            #
            if self.fIsButton:
                value = int(self.fRealValue) + 1;
                if (value > self.fMaximum):
                    value = 0
                self.setValue(value, True)
            else:
                self.fIsPressed = True
                self.fLastDragPos = event.pos()
                self.fLastDragValue = self.fRealValue
                self.dragStateChanged.emit(True)

        elif event.button() == Qt.MiddleButton:
            if self.fIsOutput:
                return
            self.setValue(self.fDefault, True)


    def mouseDoubleClickEvent(self, event):
        if self.knobPusheable and self.fIndex in (PARAMETER_DRYWET, PARAMETER_VOLUME, PARAMETER_PANNING, PARAMETER_FORTH): # -3, -4, -7, -9
            return # Mutex with special Single Click

        if event.button() == Qt.LeftButton:
            if self.fIsButton:
                value = int(self.fRealValue) + 1;
                if (value > self.fMaximum):
                    value = 0
                self.setValue(value, True)
            else:
                if self.fIsOutput:
                    return

                self.knobCustomInputDialog()


    def mouseMoveEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT or self.fIsOutput:
            return

        if not self.fIsPressed:
            return

        pos = event.pos()
        delta = (float(pos.x() - self.fLastDragPos.x()) - float(pos.y() - self.fLastDragPos.y())) / 10

        mod = event.modifiers()
        self.applyDelta(mod, delta, True)


    def mouseReleaseEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT or self.fIsOutput:
            return

        if self.fIsPressed:
            self.fIsPressed = False
            self.dragStateChanged.emit(False)

        if event.button() == Qt.LeftButton:
            if event.pos() == self.fLastDragPos:
                if self.fNumScalePoints:
                    self.nextScalePoint()
                else:
                    self.knobPush()

    # NOTE: fLastLabel state managed @ scalabledial
    def knobPush(self):
        if self.knobPusheable and self.fIndex in (PARAMETER_DRYWET, PARAMETER_VOLUME, PARAMETER_PANNING, PARAMETER_FORTH): # -3, -4, -7, -9

            if self.fLastLabel == "":   # push value
                self.fLastValue = self.fRealValue
                self.setValue(0, True)  # Thru or Mute
            else:                       # pop value
                self.setValue(self.fLastValue, True)


    def applyDelta(self, mod, delta, anchor = False):
        if self.fIsOutput:
            return

        if self.fIsButton:
            self.setValue(self.fRealValue + delta, True)
            return

        if self.fIsInteger: # 4 to 50 ticks per revolution
            if (mod & Qt.ShiftModifier):
                delta = delta * 5
            elif (mod & Qt.ControlModifier):
                delta = delta / min(int((self.fMaximum-self.fMinimum)/self.fPrecision), 5)
        else: # Floats are 250 to 500 ticks per revolution
# jpka: 1. Should i use these steps?
# 2. And what do i do when i TODO add MODE_LOG along with MODE_LINEAR?
# 3. And they're too small for large ints like in TAP Reverb, and strange for scalepoints.
#   paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)
#   paramRanges['step'], paramRanges['stepSmall'], paramRanges['stepLarge']
            if (mod & Qt.ControlModifier) and (mod & Qt.ShiftModifier):
                delta = delta * 2/5
            elif (mod & Qt.ControlModifier):
                delta = delta * 2
            elif (mod & Qt.ShiftModifier):
                delta = delta * 50
            else:
                delta = delta * 10

        difference = float(self.fMaximum-self.fMinimum) * float(delta) / float(self.fPrecision)

        if anchor:
            self.setValue((self.fLastDragValue + difference), True)
        else:
            self.setValue((self.fRealValue + difference), True)

        return


    def wheelEvent(self, event):
        if self.fIsOutput:
            return

        direction = event.angleDelta().y()
        if direction < 0:
            delta = -1.0
        elif direction > 0:
            delta = 1.0
        else:
            return

        mod = event.modifiers()
        self.applyDelta(mod, delta)
        return


    def keyPressEvent(self, event):
        if self.fIsOutput:
            return

        key = event.key()
        mod = event.modifiers()
        modsNone = not ((mod & Qt.ShiftModifier) | (mod & Qt.ControlModifier) | (mod & Qt.AltModifier))

        if modsNone:
            match key:
                case Qt.Key_Space | Qt.Key_Enter | Qt.Key_Return :
                    if self.fIsButton:
                        value = int(self.fRealValue) + 1
                        if (value > self.fMaximum):
                            value = 0
                        self.setValue(value, True)

                    elif not key == Qt.Key_Space:
                        self.knobCustomInputDialog()
                    else:
                        if self.fNumScalePoints:
                            self.nextScalePoint()
                        else:
                            self.knobPush()

                case Qt.Key_E:
                        self.knobCustomInputDialog()

                case key if Qt.Key_0 <= key <= Qt.Key_9:
                    if self.fIsInteger and (self.fMinimum == 0) and (self.fMaximum <= 10):
                        self.setValue(key-Qt.Key_0, True)

                    else:
                        self.setValue(self.fMinimum + float(self.fMaximum-self.fMinimum)/10.0*(key-Qt.Key_0), True)

                case Qt.Key_Home: # NOTE: interferes with Canvas control hotkey
                    self.setValue(self.fMinimum, True)

                case Qt.Key_End:
                    self.setValue(self.fMaximum, True)

                case Qt.Key_D:
                    self.setValue(self.fDefault, True)

                case Qt.Key_R:
                    self.setValue(self.fDefault, True)

        match key:
            case Qt.Key_PageDown:
                self.applyDelta(mod, -1)

            case Qt.Key_PageUp:
                self.applyDelta(mod, 1)

        return


    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, True)

        enabled = int(bool(self.isEnabled()))
        if enabled:
            self.setContextMenuPolicy(Qt.CustomContextMenu)
        else:
            self.setContextMenuPolicy(Qt.NoContextMenu)

        if self.fLabel:
            # if self.fCustomPaintMode == self.CUSTOM_PAINT_MODE_NULL:
            #     painter.setPen(self.fLabelGradientColor2)
            #     painter.setBrush(self.fLabelGradient)
            #     # painter.drawRect(self.fLabelGradientRect)  FIXME restore gradients.

            luma = int(bool(self.fWhiteLabels)) - 0.5
            if enabled:
                L = (luma * (1.6 + self.addContrast * 0.4)) / 2 + 0.5
            else:
                L = (luma * (0.2 + self.addContrast * 0.2)) / 2 + 0.5

            painter.setFont(self.fLabelFont)
            # painter.setPen(self.fLabelGradientColorT[0 if self.fIsEnabled() else 1])
            painter.setPen(QColor.fromHslF(0, 0, L, 1))
            painter.drawText(self.fLabelPos, self.fLabel)

        X = self.fWidth / 2
        Y = self.fHeight / 2

        S = enabled * 0.9                       # saturation

        E = enabled * self.fHoverStep / 40      # enlight
        L = 0.6 + E
        if self.addContrast:
            L = min(L + 0.3, 1)                 # luma

        normValue = float(self.fRealValue - self.fMinimum) / float(self.fMaximum - self.fMinimum)
        #                           Work In Progress FIXME
        H=0
        if self.fIsOutput:
            if self.fIsButton:
                # self.paintLed    (painter, X, Y, H, S, L, E, normValue)
                self.paintDisplay(painter, X, Y, H, S, L, E, normValue, enabled)
            else:
                self.paintDisplay(painter, X, Y, H, S, L, E, normValue, enabled)
        else:
            if self.fIsButton:
                self.paintButton (painter, X, Y, H, S, L, E, normValue, enabled)
            else:
                self.paintDial   (painter, X, Y, H, S, L, E, normValue, enabled)

        # Display tooltip, above the knob (OS-independent, unlike of mouse tooltip).
        # Note, update/redraw Qt's tooltip eats much more CPU than expected,
        # so we have tweak for turn it off. See also #1934.
        if self.fHoverStep == self.HOVER_MAX and self.displayTooltip:
            # First, we need to find exact or nearest match (index from value).
            # It is also tests if we have scale points at all.
            num = -1
            for i in range(self.fNumScalePoints):
                scaleValue = self.fScalePoints[i]['value']
                if i == 0:
                    finalValue = scaleValue
                    num = 0
                else:
                    srange2 = abs(self.fRealValue - finalValue)
                    srange1 = abs(self.fRealValue - scaleValue)
                    if srange2 > srange1:
                        finalValue = scaleValue
                        num = i
                    if (srange1 == 0): # Exact match, save some CPU.
                        break

            tip = ""
            if (num >= 0): # Scalepoints are used
                tip = str(self.fScalePoints[num]['label'])
                if not self.fIsButton:
                    tip = self.fScalePointsPrefix + \
                          strLim(self.fScalePoints[num]['value']) + \
                          self.fScalePointsSuffix + ": " + tip
            # ? We most probably not need tooltip for button, if it is not scalepoint.
            # elif not self.fIsButton:
            else:
                if self.fRealValue == 0 and self.fIndex == PARAMETER_DRYWET: #-3,-4,-7,-9
                    tip = "THRU"
                elif self.fRealValue == 0 and self.fIndex == PARAMETER_VOLUME:
                    tip = "MUTE"
                elif self.fRealValue == 0 and self.fIndex == PARAMETER_PANNING:
                    tip = "Center"
                elif self.fRealValue == 0 and self.fIndex == PARAMETER_FORTH:
                    tip = "Midway"
                else:
                    if self.fIndex < PARAMETER_NULL:
                        percent = 100.0
                    else:
                        percent = 1

                    tip = (strLim(self.fRealValue * percent) + " " + self.fUnit).strip()
                    if self.fIsOutput:
                        tip = tip + "  [" + strLim(self.fMinimum * percent) + "..." + \
                                            strLim(self.fMaximum * percent) + "]"

            # Wrong vert. position for Calf:
            # QToolTip.showText(self.mapToGlobal(QPoint(0, 0-self.geometry().height())), tip)
            # FIXME Still wrong vert. position for QT_SCALE_FACTOR=2.
            QToolTip.showText(self.mapToGlobal(QPoint(0, 0-45)), tip)
        else:
            QToolTip.hideText()

        if enabled:
            if self.HOVER_MIN < self.fHoverStep < self.HOVER_MAX:
                self.fHoverStep += 1 if self.fIsHovered else -1
                QTimer.singleShot(20, self.update)

        painter.restore()

    # def resizeEvent(self, event):
    #     QWidget.resizeEvent(self, event)
    #     self.updateSizes()

# ---------------------------------------------------------------------------------------------------------------------
