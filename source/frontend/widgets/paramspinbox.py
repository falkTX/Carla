#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import isnan, modf
from random import random

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, QT_VERSION, Qt, QTimer
    from PyQt5.QtGui import QCursor, QPalette
    from PyQt5.QtWidgets import QAbstractSpinBox, QApplication, QComboBox, QDialog, QMenu, QProgressBar
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, pyqtSlot, QT_VERSION, Qt, QTimer
    from PyQt6.QtGui import QCursor, QPalette
    from PyQt6.QtWidgets import QAbstractSpinBox, QApplication, QComboBox, QDialog, QMenu, QProgressBar

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_inputdialog_value

from carla_backend import CARLA_OS_MAC
from carla_shared import countDecimalPoints

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
    def __init__(self, parent, label, current, minimum, maximum, step, stepSmall, scalePoints, prefix, suffix):
        QDialog.__init__(self, parent)
        self.ui = ui_inputdialog_value.Ui_Dialog()
        self.ui.setupUi(self)

        decimals = countDecimalPoints(step, stepSmall)
        self.ui.label.setText(label)
        self.ui.doubleSpinBox.setDecimals(decimals)
        self.ui.doubleSpinBox.setRange(minimum, maximum)
        self.ui.doubleSpinBox.setSingleStep(step)
        self.ui.doubleSpinBox.setValue(current)
        self.ui.doubleSpinBox.setPrefix(prefix)
        self.ui.doubleSpinBox.setSuffix(suffix)

        if CARLA_OS_MAC:
            self.setWindowModality(Qt.WindowModal)

        if not scalePoints:
            self.ui.groupBox.setVisible(False)
            self.resize(200, 0)
        else:
            text = "<table>"
            for scalePoint in scalePoints:
                valuestr = ("%i" if decimals == 0 else "%f") % scalePoint['value']
                text += "<tr>"
                text += f"<td align='right'>{valuestr}</td><td align='left'> - {scalePoint['label']}</td>"
                text += "</tr>"
            text += "</table>"
            self.ui.textBrowser.setText(text)
            self.resize(200, 300)

        self.fRetValue = current
        self.adjustSize()
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
    dragStateChanged = pyqtSignal(bool)
    valueChanged = pyqtSignal(float)

    def __init__(self, parent):
        QProgressBar.__init__(self, parent)

        self.fLeftClickDown = False
        self.fIsInteger     = False
        self.fIsReadOnly    = False

        self.fMinimum   = 0.0
        self.fMaximum   = 1.0
        self.fInitiated = False
        self.fRealValue = 0.0

        self.fLastPaintedValue   = None
        self.fCurrentPaintedText = ""

        self.fName  = ""
        self.fLabelPrefix = ""
        self.fLabelSuffix = ""
        self.fTextCall  = None
        self.fValueCall = None

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
        if (self.fRealValue == value or isnan(value)) and self.fInitiated:
            return False

        self.fInitiated = True
        self.fRealValue = value
        div = float(self.fMaximum - self.fMinimum)

        if div == 0.0:
            print("Parameter '%s' division by 0 prevented (value:%f, min:%f, max:%f)" % (self.fName,
                                                                                         value,
                                                                                         self.fMaximum,
                                                                                         self.fMinimum))
            vper = 1.0
        elif isnan(value):
            print("Parameter '%s' is NaN (value:%f, min:%f, max:%f)" % (self.fName,
                                                                        value,
                                                                        self.fMaximum,
                                                                        self.fMinimum))
            vper = 1.0
        else:
            vper = float(value - self.fMinimum) / div

            if vper < 0.0:
                vper = 0.0
            elif vper > 1.0:
                vper = 1.0

        if self.fValueCall is not None:
            self.fValueCall(value)

        QProgressBar.setValue(self, int(vper * 10000))
        return True

    def setSuffixes(self, prefix, suffix):
        self.fLabelPrefix = prefix
        self.fLabelSuffix = suffix

        # force refresh of text value
        self.fLastPaintedValue = None

        self.update()

    def setName(self, name):
        self.fName = name

    def setReadOnly(self, yesNo):
        self.fIsReadOnly = yesNo

    def setTextCall(self, textCall):
        self.fTextCall = textCall

    def setValueCall(self, valueCall):
        self.fValueCall = valueCall

    def handleMouseEventPos(self, pos):
        if self.fIsReadOnly:
            return

        xper  = float(pos.x()) / float(self.width())
        value = xper * (self.fMaximum - self.fMinimum) + self.fMinimum

        if self.fIsInteger:
            value = round(value)

        if value < self.fMinimum:
            value = self.fMinimum
        elif value > self.fMaximum:
            value = self.fMaximum

        if self.setValue(value):
            self.valueChanged.emit(value)

    def mousePressEvent(self, event):
        if self.fIsReadOnly:
            return

        if event.button() == Qt.LeftButton:
            self.handleMouseEventPos(event.pos())
            self.fLeftClickDown = True
            self.dragStateChanged.emit(True)
        else:
            self.fLeftClickDown = False

        QProgressBar.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.fIsReadOnly:
            return

        if self.fLeftClickDown:
            self.handleMouseEventPos(event.pos())

        QProgressBar.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.fIsReadOnly:
            return

        self.fLeftClickDown = False
        self.dragStateChanged.emit(False)
        QProgressBar.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        if self.fTextCall is not None:
            if self.fLastPaintedValue != self.fRealValue:
                self.fLastPaintedValue   = self.fRealValue
                self.fCurrentPaintedText = self.fTextCall()
            self.setFormat("%s%s%s" % (self.fLabelPrefix, self.fCurrentPaintedText, self.fLabelSuffix))

        elif self.fIsInteger:
            self.setFormat("%s%i%s" % (self.fLabelPrefix, int(self.fRealValue), self.fLabelSuffix))

        else:
            self.setFormat("%s%f%s" % (self.fLabelPrefix, self.fRealValue, self.fLabelSuffix))

        QProgressBar.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Special SpinBox used for parameters

class ParamSpinBox(QAbstractSpinBox):
    # signals
    valueChanged = pyqtSignal(float)

    def __init__(self, parent):
        QAbstractSpinBox.__init__(self, parent)

        self.fName = ""
        self.fLabelPrefix = ""
        self.fLabelSuffix = ""

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

        barPalette = self.fBar.palette()
        barPalette.setColor(QPalette.Window, Qt.transparent)
        self.fBar.setPalette(barPalette)

        self.fBox = None

        self.lineEdit().hide()

        self.customContextMenuRequested.connect(self.slot_showCustomMenu)
        self.fBar.valueChanged.connect(self.slot_progressBarValueChanged)

        self.dragStateChanged = self.fBar.dragStateChanged

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
        if not self.fIsReadOnly:
            value = geFixedValue(self.fName, value, self.fMinimum, self.fMaximum)

        if self.fBar.fIsInteger:
            value = round(value)

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

        self.fStepSmall = min(self.fStepSmall, value)
        self.fStepLarge = max(self.fStepLarge, value)

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
        prefix = ""
        suffix = label.strip()

        if suffix == "(coef)":
            prefix = "* "
            suffix = ""
        else:
            suffix = " " + suffix

        self.fLabelPrefix = prefix
        self.fLabelSuffix = suffix
        self.fBar.setSuffixes(prefix, suffix)

    def setName(self, name):
        self.fName = name
        self.fBar.setName(name)

    def setTextCallback(self, textCall):
        self.fBar.setTextCall(textCall)

    def setValueCallback(self, valueCall):
        self.fBar.setValueCall(valueCall)

    def setReadOnly(self, yesNo):
        self.fIsReadOnly = yesNo
        self.fBar.setReadOnly(yesNo)
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

        if QT_VERSION >= 0x60000:
            self.fBox.currentTextChanged.connect(self.slot_comboBoxIndexChanged)
        else:
            self.fBox.currentIndexChanged['QString'].connect(self.slot_comboBoxIndexChanged)

    def setToolTip(self, text):
        self.fBar.setToolTip(text)
        QAbstractSpinBox.setToolTip(self, text)

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
                                             self.fStep, self.fStepSmall, self.fScalePoints,
                                             self.fLabelPrefix, self.fLabelSuffix)
            if dialog.exec_():
                value = dialog.returnValue()
                self.setValue(value)

    @pyqtSlot()
    def slot_updateProgressBarGeometry(self):
        geometry = self.lineEdit().geometry()
        dx = geometry.x()-1
        dy = geometry.y()-1
        geometry.adjust(-dx, -dy, dx, dy)
        self.fBar.setGeometry(geometry)
        if self.fUseScalePoints:
            self.fBox.setGeometry(geometry)

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
