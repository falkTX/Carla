#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import isnan

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QPoint, QPointF, QRectF, QEvent
    from PyQt5.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt5.QtWidgets import QDial, QToolTip
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, pyqtSlot, Qt, QPoint, QPointF, QRectF, QEvent
    from PyQt6.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt6.QtWidgets import QDial, QToolTip

# ---------------------------------------------------------------------------------------------------------------------
# Widget Class

# to be implemented by subclasses
    #def updateSizes(self):
    #def paintDial(self, painter):

class CommonDial(QDial):
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

    MODE_DEFAULT = 0
    MODE_LINEAR = 1

    # signals
    dragStateChanged = pyqtSignal(bool)
    realValueChanged = pyqtSignal(float)

    def __init__(self, parent, index):
        QDial.__init__(self, parent)

        self.fDialMode = self.MODE_LINEAR

        self.fMinimum   = 0.0
        self.fMaximum   = 1.0
        self.fRealValue = 0.0
        self.fPrecision = 10000
        self.fIsInteger = False
        self.fIsButton  = False
        self.fScalePoints = []
        self.fScalePointsPrefix = ""
        self.fScalePointsSuffix = ""

        self.fIsHovered = False
        self.fIsPressed = False
        self.fHoverStep = self.HOVER_MIN

        self.fLastDragPos = None
        self.fLastDragValue = 0.0

        self.fIndex = index

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

        # Fake internal value, custom precision
        QDial.setMinimum(self, 0)
        QDial.setMaximum(self, self.fPrecision)
        QDial.setValue(self, 0)

        self.valueChanged.connect(self.slot_valueChanged)

    def forceWhiteLabelGradientText(self):
        self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
        self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
        self.fLabelGradientColorT = [Qt.white, Qt.darkGray]

    def setLabelColor(self, enabled, disabled):
        self.fLabelGradientColor1 = QColor(0, 0, 0, 255)
        self.fLabelGradientColor2 = QColor(0, 0, 0, 0)
        self.fLabelGradientColorT = [enabled, disabled]

    def getIndex(self):
        return self.fIndex

    def setIndex(self, index):
        self.fIndex = index

    def setPrecision(self, value, isInteger, scalePoints, prefix, suffix):
        self.fPrecision = value
        self.fIsInteger = isInteger
        self.fScalePoints = scalePoints
        self.fScalePointsPrefix = prefix
        self.fScalePointsSuffix = suffix
        # NOTE: Booleans are mimic as isInteger with range [0 or 1].
        if self.fIsInteger and (self.fMinimum == 0) and (self.fMaximum in (1, 2)):
            self.fIsButton = True
        QDial.setMaximum(self, int(value))
        # print("setPrecision "+str(value)+" "+str(isInteger)+" "+str(self.fIsButton))

    def setMinimum(self, value):
        self.fMinimum = value

    def setMaximum(self, value):
        self.fMaximum = value

    def rvalue(self):
        return self.fRealValue

    def setValue(self, value, emitSignal=False):
        if self.fRealValue == value or isnan(value):
            return

        if value <= self.fMinimum:
            qtValue = 0
            self.fRealValue = self.fMinimum

        elif value >= self.fMaximum:
            qtValue = int(self.fPrecision)
            self.fRealValue = self.fMaximum

        else:
            qtValue = round(float(value - self.fMinimum) / float(self.fMaximum - self.fMinimum) * self.fPrecision)
            self.fRealValue = value

        # Block change signal, we'll handle it ourselves
        self.blockSignals(True)
        QDial.setValue(self, qtValue)
        self.blockSignals(False)

        if emitSignal:
            self.realValueChanged.emit(self.fRealValue)

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

    @pyqtSlot(int)
    def slot_valueChanged(self, value):
        self.fRealValue = float(value)/self.fPrecision * (self.fMaximum - self.fMinimum) + self.fMinimum
        self.realValueChanged.emit(self.fRealValue)


    def enterEvent(self, event):

        self.setFocus(Qt.MouseFocusReason)

        self.fIsHovered = True
        if self.fHoverStep == self.HOVER_MIN:
            self.fHoverStep = self.HOVER_MIN + 1
        QDial.enterEvent(self, event)

    def leaveEvent(self, event):
        self.fIsHovered = False
        if self.fHoverStep == self.HOVER_MAX:
            self.fHoverStep = self.HOVER_MAX - 1
        QDial.leaveEvent(self, event)

    def mousePressEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT:
            QDial.mousePressEvent(self, event)
            return

        if event.button() == Qt.LeftButton:
            if self.fIsButton:
                value = self.value() + 1;
                if (value > self.fMaximum):
                    value = 0;
                self.setValue(value, True)
            else:
                self.fIsPressed = True
                self.fLastDragPos = event.pos()
                self.fLastDragValue = self.fRealValue
                self.dragStateChanged.emit(True)

    def mouseMoveEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT:
            QDial.mouseMoveEvent(self, event)
            return

        if not self.fIsPressed:
            return

        diff  = (self.fMaximum - self.fMinimum) / 4.0
        pos   = event.pos()
        dx    = diff * float(pos.x() - self.fLastDragPos.x()) / self.width()
        dy    = diff * float(pos.y() - self.fLastDragPos.y()) / self.height()
        value = self.fLastDragValue + dx - dy

        if value < self.fMinimum:
            value = self.fMinimum
        elif value > self.fMaximum:
            value = self.fMaximum
        elif self.fIsInteger:
            value = float(round(value))

        self.setValue(value, True)

    def mouseReleaseEvent(self, event):
        if self.fDialMode == self.MODE_DEFAULT:
            QDial.mouseReleaseEvent(self, event)
            return

        if self.fIsPressed:
            self.fIsPressed = False
            self.dragStateChanged.emit(False)

    def applyDelta(self, mod, delta):
        if self.fIsButton:
            self.setValue(self.rvalue() + delta, True)
            return

        if self.fIsInteger: # 4 to 50 ticks per revolution
            if (mod & Qt.ShiftModifier):
                delta = delta * 5
        else: # Floats are 250 to 500 ticks per revolution
            if (mod == (Qt.ControlModifier + Qt.ShiftModifier)):
                delta = delta * 2/5
            elif (mod & Qt.ControlModifier):
                delta = delta * 2
            elif (mod & Qt.ShiftModifier):
                delta = delta * 50
            else:
                delta = delta * 10

        self.setValue(self.rvalue() + float(self.fMaximum-self.fMinimum)*(float(delta)/float(self.fPrecision)), True)
        return

    def wheelEvent(self, event):
        direction = event.angleDelta().y()
        if direction < 0:
            delta = -1.0
        elif direction > 0:
            delta = 1.0
        else:
            return

        mod = int(event.modifiers())
        self.applyDelta(mod, delta)
        return

    def keyPressEvent(self, event):
        key = event.key()
        mod = int(event.modifiers())
        # print(str(self.value())+" "+str(self.rvalue())+" "+str(self.fMinimum)+" "+str(self.fMaximum))

        match key:
            case key if Qt.Key_0 <= key <= Qt.Key_9:
                if self.fIsButton or (self.fIsInteger and (self.fMinimum == 0) and (self.fMaximum > 2) and (self.fMaximum <= 10)):
                    self.setValue(key-Qt.Key_0, True)
                else:
                    self.setValue(self.fMinimum + float(self.fMaximum-self.fMinimum)/10.0*(key-Qt.Key_0), True)
            case Qt.Key_Home: # NOTE: interferes with Canvas control hotkey
                self.setValue(self.fMinimum, True)
            case Qt.Key_End:
                self.setValue(self.fMaximum, True)
            case Qt.Key_PageDown:
                self.applyDelta(mod, -1)

            case Qt.Key_PageUp:
                self.applyDelta(mod, 1)

        return

    def paintEvent(self, event):

        def strFiveDigits(x):
            if self.fIsInteger:
                x = int(x)
            ret = lambda x: float(x) if '.' in str(x) else int(x)
            return str(ret(str(x)[:max(str(x).find('.'), 6 + ('-' in str(x)))].strip('.')))

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

        if self.fIsButton:
            self.paintButton(painter, self.fMaximum)
        else:
            self.paintDial(painter)

        # Display tooltip, above the knob (OS-independent, unlike of mouse tooltip).
        # BUG Do not work if value not changed.
        if self.fHoverStep == self.HOVER_MAX:
            # It looks like {'value': 0.0, 'label': 'Channels 1 + 2'}

            # First, we need to find exact or nearest match (index from value).
            # It is also tests if we have scale points at all.
            num = -1
            realValue = self.rvalue()
            for i in range(len(self.fScalePoints)):
                scaleValue = self.fScalePoints[i]['value']
                if i == 0:
                    finalValue = scaleValue
                    num = 0
                else:
                    srange2 = abs(realValue - finalValue)
                    srange1 = abs(realValue - scaleValue)
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
                          strFiveDigits(self.fScalePoints[num]['value']) + \
                          self.fScalePointsSuffix + ": " + tip
            # We most probably not need tooltip for button, if it is not scalepoint.
            elif not self.fIsButton:
                tip = strFiveDigits(realValue)

            # Wrong vert. position for Calf:
            # QToolTip.showText(self.mapToGlobal(QPoint(0, 0-self.geometry().height())), tip)
            # FIXME Still wrong vert. position for QT_SCALE_FACTOR=2.
            QToolTip.showText(self.mapToGlobal(QPoint(0, 0-45)), tip)
        else:
            QToolTip.hideText()

        painter.restore()

    def resizeEvent(self, event):
        QDial.resizeEvent(self, event)
        self.updateSizes()

# ---------------------------------------------------------------------------------------------------------------------
