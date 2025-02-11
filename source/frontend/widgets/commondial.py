#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import isnan

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QPointF, QRectF
    from PyQt5.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt5.QtWidgets import QDial
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, pyqtSlot, Qt, QPointF, QRectF
    from PyQt6.QtGui import QColor, QFont, QLinearGradient, QPainter
    from PyQt6.QtWidgets import QDial

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

    def setPrecision(self, value, isInteger):
        self.fPrecision = value
        self.fIsInteger = isInteger
        QDial.setMaximum(self, int(value))

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

        self.paintDial(painter)

        painter.restore()

    def resizeEvent(self, event):
        QDial.resizeEvent(self, event)
        self.updateSizes()

# ---------------------------------------------------------------------------------------------------------------------
