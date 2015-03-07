#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Parameter SpinBox, a custom Qt4 widget
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

from math import isnan, modf
from random import random

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QTimer
    from PyQt5.QtGui import QCursor
    from PyQt5.QtWidgets import QAbstractSpinBox, QApplication, QComboBox, QDialog, QMenu, QProgressBar
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt, QTimer
    from PyQt4.QtGui import QAbstractSpinBox, QApplication, QComboBox, QCursor, QDialog, QMenu, QProgressBar

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_inputdialog_value

# ------------------------------------------------------------------------------------------------------------
# Get a fixed value within min/max bounds

def geFixedValue(name, value, minimum, maximum):
    if isnan(value):
        print("Parameter '%s' is NaN! - %f" % (name, value))
        return minimum
    if value < minimum:
        print("Parameter '%s' too low! - %f/%f" % (name, value, minimum))
        return minimum
    if value > maximum:
        print("Parameter '%s' too high! - %f/%f" % (name, value, maximum))
        return maximum
    return value

# ------------------------------------------------------------------------------------------------------------
# Custom InputDialog with Scale Points support

class CustomInputDialog(QDialog):
    def __init__(self, parent, label, current, minimum, maximum, step, stepSmall, scalePoints):
        QDialog.__init__(self, parent)
        self.ui = ui_inputdialog_value.Ui_Dialog()
        self.ui.setupUi(self)

        # calculate num decimals from stepSmall
        if stepSmall >= 1.0:
            decimals = 0
        elif step >= 1.0:
            decimals = 2
        else:
            decfrac, decwhole = modf(stepSmall)

            if "000" in str(decfrac):
                decfrac = round(decfrac, str(decfrac).find("000"))
            else:
                decfrac = round(decfrac, 12)

            decimals = abs(len(str(decfrac))-len(str(decwhole))-1)

        self.ui.label.setText(label)
        self.ui.doubleSpinBox.setDecimals(decimals)
        self.ui.doubleSpinBox.setRange(minimum, maximum)
        self.ui.doubleSpinBox.setSingleStep(step)
        self.ui.doubleSpinBox.setValue(current)

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

        self.accepted.connect(self.slot_setReturnValue)

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
    # signals
    valueChanged = pyqtSignal(float)

    def __init__(self, parent):
        QProgressBar.__init__(self, parent)

        self.fLeftClickDown = False
        self.fIsInteger     = False

        self.fMinimum   = 0.0
        self.fMaximum   = 1.0
        self.fRealValue = 0.0

        self.fLabel = ""
        self.fName  = ""
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
        div = float(self.fMaximum - self.fMinimum)

        if div == 0.0:
            print("Parameter '%s' division by 0 prevented (value:%f, min:%f, max:%f)" % (self.fName, value, self.fMaximum, self.fMinimum))
            vper = 1.0
        else:
            vper = float(value - self.fMinimum) / div

        QProgressBar.setValue(self, int(vper * 10000))

    def setLabel(self, label):
        self.fLabel = label.strip()

        if self.fLabel == "(coef)":
            self.fLabel = ""
            self.fPreLabel = "*"

        self.update()

    def setName(self, name):
        self.fName = name

    def setTextCall(self, textCall):
        self.fTextCall = textCall

    def handleMouseEventPos(self, pos):
        xper  = float(pos.x()) / float(self.width())
        value = xper * (self.fMaximum - self.fMinimum) + self.fMinimum

        if value < self.fMinimum:
            value = self.fMinimum
        elif value > self.fMaximum:
            value = self.fMaximum

        self.valueChanged.emit(value)

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
        elif self.fIsInteger:
            self.setFormat("%s %i %s" % (self.fPreLabel, int(self.fRealValue), self.fLabel))
        else:
            self.setFormat("%s %f %s" % (self.fPreLabel, self.fRealValue, self.fLabel))

        QProgressBar.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Special SpinBox used for parameters

class ParamSpinBox(QAbstractSpinBox):
    # signals
    valueChanged = pyqtSignal(float)

    def __init__(self, parent):
        QAbstractSpinBox.__init__(self, parent)

        self.fName = ""

        self.fMinimum = 0.0
        self.fMaximum = 1.0
        self.fDefault = 0.0
        self.fValue   = None

        self.fStep      = 0.01
        self.fStepSmall = 0.0001
        self.fStepLarge = 0.1

        self.fIsReadOnly = False
        self.fScalePoints = None
        self.fUseScalePoints = False

        self.fBar = ParamProgressBar(self)
        self.fBar.setContextMenuPolicy(Qt.NoContextMenu)
        #self.fBar.show()

        self.fBox = None

        self.lineEdit().hide()

        self.customContextMenuRequested.connect(self.slot_showCustomMenu)
        self.fBar.valueChanged.connect(self.slot_progressBarValueChanged)

        QTimer.singleShot(0, self.slot_updateProgressBarGeometry)

    def setDefault(self, value):
        value = geFixedValue(self.fName, value, self.fMinimum, self.fMaximum)
        self.fDefault = value

    def setMinimum(self, value):
        self.fMinimum = value
        self.fBar.setMinimum(value)

    def setMaximum(self, value):
        self.fMaximum = value
        self.fBar.setMaximum(value)

    def setValue(self, value):
        value = geFixedValue(self.fName, value, self.fMinimum, self.fMaximum)

        if self.fValue == value:
            return False

        self.fValue = value
        self.fBar.setValue(value)

        if self.fUseScalePoints:
            self._setScalePointValue(value)

        self.valueChanged.emit(value)
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

        self.fBar.fIsInteger = bool(self.fStepSmall == 1.0)

    def setStepSmall(self, value):
        if value == 0.0:
            self.fStepSmall = 0.0001
        elif value > self.fStep:
            self.fStepSmall = self.fStep
        else:
            self.fStepSmall = value

        self.fBar.fIsInteger = bool(self.fStepSmall == 1.0)

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
        self.fBar.setName(name)

    def setTextCallback(self, textCall):
        self.fBar.setTextCall(textCall)

    def setReadOnly(self, yesNo):
        self.fIsReadOnly = yesNo
        self.setButtonSymbols(QAbstractSpinBox.UpDownArrows if yesNo else QAbstractSpinBox.NoButtons)
        QAbstractSpinBox.setReadOnly(self, yesNo)

    # FIXME use change event
    def setEnabled(self, yesNo):
        self.fBar.setEnabled(yesNo)
        QAbstractSpinBox.setEnabled(self, yesNo)

    def setScalePoints(self, scalePoints, useScalePoints):
        if len(scalePoints) == 0:
            self.fScalePoints    = None
            self.fUseScalePoints = False
            return

        self.fScalePoints    = scalePoints
        self.fUseScalePoints = useScalePoints

        if not useScalePoints:
            return

        # Hide ProgressBar and create a ComboBox
        self.fBar.close()
        self.fBox = QComboBox(self)
        self.fBox.setContextMenuPolicy(Qt.NoContextMenu)
        #self.fBox.show()
        self.slot_updateProgressBarGeometry()

        # Add items, sorted
        boxItemValues = []

        for scalePoint in scalePoints:
            value = scalePoint['value']

            if self.fStep == 1.0:
                label = "%i - %s" % (int(value), scalePoint['label'])
            else:
                label = "%f - %s" % (value, scalePoint['label'])

            if len(boxItemValues) == 0:
                self.fBox.addItem(label)
                boxItemValues.append(value)

            else:
                if value < boxItemValues[0]:
                    self.fBox.insertItem(0, label)
                    boxItemValues.insert(0, value)
                elif value > boxItemValues[-1]:
                    self.fBox.addItem(label)
                    boxItemValues.append(value)
                else:
                    for index in range(len(boxItemValues)):
                        if value >= boxItemValues[index]:
                            self.fBox.insertItem(index+1, label)
                            boxItemValues.insert(index+1, value)
                            break

        if self.fValue is not None:
            self._setScalePointValue(self.fValue)

        self.fBox.currentIndexChanged['QString'].connect(self.slot_comboBoxIndexChanged)

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
        if self.fIsReadOnly or self.fValue is None:
            return QAbstractSpinBox.StepNone
        if self.fValue <= self.fMinimum:
            return QAbstractSpinBox.StepUpEnabled
        if self.fValue >= self.fMaximum:
            return QAbstractSpinBox.StepDownEnabled
        return (QAbstractSpinBox.StepUpEnabled | QAbstractSpinBox.StepDownEnabled)

    def updateAll(self):
        self.update()
        self.fBar.update()
        if self.fBox is not None:
            self.fBox.update()

    def resizeEvent(self, event):
        QAbstractSpinBox.resizeEvent(self, event)
        self.slot_updateProgressBarGeometry()

    @pyqtSlot(str)
    def slot_comboBoxIndexChanged(self, boxText):
        if self.fIsReadOnly:
            return

        value          = float(boxText.split(" - ", 1)[0])
        lastScaleValue = self.fScalePoints[-1]['value']

        if value == lastScaleValue:
            value = self.fMaximum

        self.setValue(value)

    @pyqtSlot(float)
    def slot_progressBarValueChanged(self, value):
        if self.fIsReadOnly:
            return

        if value <= self.fMinimum:
            realValue = self.fMinimum
        elif value >= self.fMaximum:
            realValue = self.fMaximum
        else:
            curStep   = int((value - self.fMinimum) / self.fStep + 0.5)
            realValue = self.fMinimum + (self.fStep * curStep)

            if realValue < self.fMinimum:
                realValue = self.fMinimum
            elif realValue > self.fMaximum:
                realValue = self.fMaximum

        self.setValue(realValue)

    @pyqtSlot()
    def slot_showCustomMenu(self):
        clipboard  = QApplication.instance().clipboard()
        pasteText  = clipboard.text()
        pasteValue = None

        if pasteText:
            try:
                pasteValue = float(pasteText)
            except:
                pass

        menu      = QMenu(self)
        actReset  = menu.addAction(self.tr("Reset (%f)" % self.fDefault))
        actRandom = menu.addAction(self.tr("Random"))
        menu.addSeparator()
        actCopy   = menu.addAction(self.tr("Copy (%f)" % self.fValue))

        if pasteValue is None:
            actPaste = menu.addAction(self.tr("Paste"))
            actPaste.setEnabled(False)
        else:
            actPaste = menu.addAction(self.tr("Paste (%f)" % pasteValue))

        menu.addSeparator()

        actSet = menu.addAction(self.tr("Set value..."))

        if self.fIsReadOnly:
            actReset.setEnabled(False)
            actRandom.setEnabled(False)
            actPaste.setEnabled(False)
            actSet.setEnabled(False)

        actSel = menu.exec_(QCursor.pos())

        if actSel == actReset:
            self.setValue(self.fDefault)

        elif actSel == actRandom:
            value = random() * (self.fMaximum - self.fMinimum) + self.fMinimum
            self.setValue(value)

        elif actSel == actCopy:
            clipboard.setText("%f" % self.fValue)

        elif actSel == actPaste:
            self.setValue(pasteValue)

        elif actSel == actSet:
            dialog = CustomInputDialog(self, self.fName, self.fValue, self.fMinimum, self.fMaximum,
                                             self.fStep, self.fStepSmall, self.fScalePoints)
            if dialog.exec_():
                value = dialog.returnValue()
                self.setValue(value)

    @pyqtSlot()
    def slot_updateProgressBarGeometry(self):
        self.fBar.setGeometry(self.lineEdit().geometry())
        if self.fUseScalePoints:
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
