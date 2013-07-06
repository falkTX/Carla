#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Parameter SpinBox, a custom Qt4 widget
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

from PyQt4.QtCore import pyqtSlot, Qt, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QAbstractSpinBox, QApplication, QComboBox, QCursor, QDialog, QMenu, QProgressBar
from math import isnan

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_inputdialog_value

def fixValue(value, minimum, maximum):
    if isnan(value):
        print("Parameter is NaN! - %f" % value)
        return minimum
    if value < minimum:
        print("Parameter too low! - %f/%f" % (value, minimum))
        return minimum
    if value > maximum:
        print("Parameter too high! - %f/%f" % (value, maximum))
        return maximum
    return value

# ------------------------------------------------------------------------------------------------------------
# Custom InputDialog with Scale Points support

class CustomInputDialog(QDialog):
    def __init__(self, parent, label, current, minimum, maximum, step, scalePoints):
        QDialog.__init__(self, parent)
        self.ui = ui_inputdialog_value.Ui_Dialog()
        self.ui.setupUi(self)

        self.ui.label.setText(label)
        self.ui.doubleSpinBox.setMinimum(minimum)
        self.ui.doubleSpinBox.setMaximum(maximum)
        self.ui.doubleSpinBox.setValue(current)
        self.ui.doubleSpinBox.setSingleStep(step)

        if not scalePoints:
            self.ui.groupBox.setVisible(False)
            self.resize(200, 0)
        else:
            text = "<table>"
            for scalePoint in scalePoints:
                text += "<tr><td align='right'>%f</td><td align='left'> - %s</td></tr>" % (scalePoint['value'], scalePoint['label'])
            text += "</table>"
            self.ui.textBrowser.setText(text)
            self.resize(200, 300)

        self.fRetValue = current

        self.connect(self, SIGNAL("accepted()"), SLOT("slot_setReturnValue()"))

    def returnValue(self):
        return self.fRetValue

    @pyqtSlot()
    def slot_setReturnValue(self):
        self.fRetValue = self.ui.doubleSpinBox.value()

    def done(self, r):
        QDialog.done(self, r)
        self.close()

# ------------------------------------------------------------------------------------------------------------
# ProgressBar used for ParamSpinBox

class ParamProgressBar(QProgressBar):
    def __init__(self, parent):
        QProgressBar.__init__(self, parent)

        self.fLeftClickDown = False

        self.fMinimum   = 0.0
        self.fMaximum   = 1.0
        self.fRealValue = 0.0

        self.fLabel = ""
        self.fPreLabel = " "
        self.fTextCall = None

        self.setFormat("(none)")

        # Fake internal value, 10'000 precision
        QProgressBar.setMinimum(self, 0)
        QProgressBar.setMaximum(self, 10000)
        QProgressBar.setValue(self, 0)

    def setMinimum(self, value):
        self.fMinimum = value

    def setMaximum(self, value):
        self.fMaximum = value

    def setValue(self, value):
        self.fRealValue = value
        vper = float(value - self.fMinimum) / float(self.fMaximum - self.fMinimum)
        QProgressBar.setValue(self, int(vper * 10000))

    def setLabel(self, label):
        self.fLabel = label.strip()

        if self.fLabel == "(coef)":
            self.fLabel = ""
            self.fPreLabel = "*"

        self.update()

    def setTextCall(self, textCall):
        self.fTextCall = textCall

    def handleMouseEventPos(self, pos):
        xper  = float(pos.x()) / float(self.width())
        value = xper * (self.fMaximum - self.fMinimum) + self.fMinimum

        if value < self.fMinimum:
            value = self.fMinimum
        elif value > self.fMaximum:
            value = self.fMaximum

        self.emit(SIGNAL("valueChanged(double)"), value)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.handleMouseEventPos(event.pos())
            self.fLeftClickDown = True
        else:
            self.fLeftClickDown = False
        QProgressBar.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.fLeftClickDown:
            self.handleMouseEventPos(event.pos())
        QProgressBar.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        self.fLeftClickDown = False
        QProgressBar.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        if self.fTextCall is not None:
            self.setFormat("%s %s %s" % (self.fPreLabel, self.fTextCall(), self.fLabel))
        else:
            self.setFormat("%s %f %s" % (self.fPreLabel, self.fRealValue, self.fLabel))

        QProgressBar.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Special SpinBox used for parameters

class ParamSpinBox(QAbstractSpinBox):
    def __init__(self, parent):
        QAbstractSpinBox.__init__(self, parent)

        self.fMinimum = 0.0
        self.fMaximum = 1.0
        self.fDefault = 0.0
        self.fValue   = None
        self.fStep    = 0.0
        self.fStepSmall = 0.0
        self.fStepLarge = 0.0

        self.fReadOnly = False
        self.fScalePoints = None
        self.fHaveScalePoints = False

        self.fBar = ParamProgressBar(self)
        self.fBar.setContextMenuPolicy(Qt.NoContextMenu)
        self.fBar.show()

        self.fName = ""

        self.lineEdit().setVisible(False)

        self.connect(self, SIGNAL("customContextMenuRequested(QPoint)"), SLOT("slot_showCustomMenu()"))
        self.connect(self.fBar, SIGNAL("valueChanged(double)"), SLOT("slot_progressBarValueChanged(double)"))

        QTimer.singleShot(0, self, SLOT("slot_updateProgressBarGeometry()"))

    def setDefault(self, value):
        value = fixValue(value, self.fMinimum, self.fMaximum)
        self.fDefault = value

    def setMinimum(self, value):
        self.fMinimum = value
        self.fBar.setMinimum(value)

    def setMaximum(self, value):
        self.fMaximum = value
        self.fBar.setMaximum(value)

    def setValue(self, value, send=True):
        value = fixValue(value, self.fMinimum, self.fMaximum)

        if self.fValue == value:
            return False

        self.fValue = value
        self.fBar.setValue(value)

        if self.fHaveScalePoints:
            self._setScalePointValue(value)

        if send:
            self.emit(SIGNAL("valueChanged(double)"), value)

        self.update()

        return True

    def setStep(self, value):
        if value == 0.0:
            self.fStep = 0.001
        else:
            self.fStep = value

        if self.fStepSmall > value:
            self.fStepSmall = value
        if self.fStepLarge < value:
            self.fStepLarge = value

    def setStepSmall(self, value):
        if value == 0.0:
            self.fStepSmall = 0.0001
        elif value > self.fStep:
            self.fStepSmall = self.fStep
        else:
            self.fStepSmall = value

    def setStepLarge(self, value):
        if value == 0.0:
            self.fStepLarge = 0.1
        elif value < self.fStep:
            self.fStepLarge = self.fStep
        else:
            self.fStepLarge = value

    def setLabel(self, label):
        self.fBar.setLabel(label)

    def setName(self, name):
        self.fName = name

    def setTextCallback(self, textCall):
        self.fBar.setTextCall(textCall)

    def setReadOnly(self, yesNo):
        self.setButtonSymbols(QAbstractSpinBox.UpDownArrows if yesNo else QAbstractSpinBox.NoButtons)
        self.fReadOnly = yesNo
        QAbstractSpinBox.setReadOnly(self, yesNo)

    def setScalePoints(self, scalePoints, useScalePoints):
        if len(scalePoints) == 0:
            self.fScalePoints     = None
            self.fHaveScalePoints = False
            return

        self.fScalePoints     = scalePoints
        self.fHaveScalePoints = useScalePoints

        if useScalePoints:
            # Hide ProgressBar and create a ComboBox
            self.fBar.close()
            self.fBox = QComboBox(self)
            self.fBox.setContextMenuPolicy(Qt.NoContextMenu)
            self.fBox.show()
            self.slot_updateProgressBarGeometry()

            for scalePoint in scalePoints:
                self.fBox.addItem("%f - %s" % (scalePoint['value'], scalePoint['label']))

            if self.fValue != None:
                self._setScalePointValue(self.fValue)

            self.connect(self.fBox, SIGNAL("currentIndexChanged(QString)"), SLOT("slot_comboBoxIndexChanged(QString)"))

    def stepBy(self, steps):
        if steps == 0 or self.fValue is None:
            return

        value = self.fValue + (self.fStep * steps)

        if value < self.fMinimum:
            value = self.fMinimum
        elif value > self.fMaximum:
            value = self.fMaximum

        self.setValue(value)

    def stepEnabled(self):
        if self.fReadOnly or self.fValue is None:
            return QAbstractSpinBox.StepNone
        if self.fValue <= self.fMinimum:
            return QAbstractSpinBox.StepUpEnabled
        if self.fValue >= self.fMaximum:
            return QAbstractSpinBox.StepDownEnabled
        return (QAbstractSpinBox.StepUpEnabled | QAbstractSpinBox.StepDownEnabled)

    def updateAll(self):
        self.update()
        self.fBar.update()
        if self.fHaveScalePoints:
            self.fBox.update()

    def resizeEvent(self, event):
        QTimer.singleShot(0, self, SLOT("slot_updateProgressBarGeometry()"))
        QAbstractSpinBox.resizeEvent(self, event)

    @pyqtSlot(str)
    def slot_comboBoxIndexChanged(self, boxText):
        if self.fReadOnly:
            return

        value          = float(boxText.split(" - ", 1)[0])
        lastScaleValue = self.fScalePoints[-1]["value"]

        if value == lastScaleValue:
            value = self.fMaximum

        self.setValue(value)

    @pyqtSlot(float)
    def slot_progressBarValueChanged(self, value):
        if self.fReadOnly:
            return

        step      = int((value - self.fMinimum) / self.fStep + 0.5)
        realValue = self.fMinimum + (step * self.fStep)

        self.setValue(realValue)

    @pyqtSlot()
    def slot_showCustomMenu(self):
        menu     = QMenu(self)
        actReset = menu.addAction(self.tr("Reset (%f)" % self.fDefault))
        menu.addSeparator()
        actCopy  = menu.addAction(self.tr("Copy (%f)" % self.fValue))

        clipboard  = QApplication.instance().clipboard()
        pasteText  = clipboard.text()
        pasteValue = None

        if pasteText:
            try:
                pasteValue = float(pasteText)
            except:
                pass

        if pasteValue is None:
            actPaste = menu.addAction(self.tr("Paste"))
        else:
            actPaste = menu.addAction(self.tr("Paste (%s)" % pasteValue))

        menu.addSeparator()

        actSet = menu.addAction(self.tr("Set value..."))

        if self.fReadOnly:
            actReset.setEnabled(False)
            actPaste.setEnabled(False)
            actSet.setEnabled(False)

        actSel = menu.exec_(QCursor.pos())

        if actSel == actSet:
            dialog = CustomInputDialog(self, self.fName, self.fValue, self.fMinimum, self.fMaximum, self.fStep, self.fScalePoints)
            if dialog.exec_():
                value = dialog.returnValue()
                self.setValue(value)

        elif actSel == actCopy:
            clipboard.setText("%f" % self.fValue)

        elif actSel == actPaste:
            self.setValue(pasteValue)

        elif actSel == actReset:
            self.setValue(self.fDefault)

    @pyqtSlot()
    def slot_updateProgressBarGeometry(self):
        self.fBar.setGeometry(self.lineEdit().geometry())
        if self.fHaveScalePoints:
            self.fBox.setGeometry(self.lineEdit().geometry())

    def _getNearestScalePoint(self, realValue):
        finalValue = 0.0

        for i in range(len(self.fScalePoints)):
            scaleValue = self.fScalePoints[i]["value"]
            if i == 0:
                finalValue = scaleValue
            else:
                srange1 = abs(realValue - scaleValue)
                srange2 = abs(realValue - finalValue)

                if srange2 > srange1:
                    finalValue = scaleValue

        return finalValue

    def _setScalePointValue(self, value):
        value = self._getNearestScalePoint(value)

        for i in range(self.fBox.count()):
            if float(self.fBox.itemText(i).split(" - ", 1)[0]) == value:
                self.fBox.setCurrentIndex(i)
                break
