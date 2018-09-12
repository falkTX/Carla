#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Keyboard, a custom Qt4 widget
# Copyright (C) 2011-2018 Filipe Coelho <falktx@falktx.com>
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

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, qCritical, Qt, QPointF, QRectF, QTimer, QSettings, QSize
    from PyQt5.QtGui import QColor, QFont, QPainter, QPixmap
    from PyQt5.QtWidgets import QMenu, QScrollArea, QWidget
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, qCritical, Qt, QPointF, QRectF, QTimer, QSettings, QSize
    from PyQt4.QtGui import QColor, QFont, QPainter, QPixmap
    from PyQt4.QtGui import QMenu, QScrollArea, QWidget

# ------------------------------------------------------------------------------------------------------------

kMidiKey2RectMapHorizontal = [
    QRectF(0,   0, 24, 57), # C
    QRectF(14,  0, 15, 33), # C#
    QRectF(24,  0, 24, 57), # D
    QRectF(42,  0, 15, 33), # D#
    QRectF(48,  0, 24, 57), # E
    QRectF(72,  0, 24, 57), # F
    QRectF(84,  0, 15, 33), # F#
    QRectF(96,  0, 24, 57), # G
    QRectF(112, 0, 15, 33), # G#
    QRectF(120, 0, 24, 57), # A
    QRectF(140, 0, 15, 33), # A#
    QRectF(144, 0, 24, 57), # B
]

kMidiKey2RectMapVertical = [
    QRectF(0, 144, 57, 24), # C
    QRectF(0, 139, 33, 15), # C#
    QRectF(0, 120, 57, 24), # D
    QRectF(0, 111, 33, 15), # D#
    QRectF(0, 96,  57, 24), # E
    QRectF(0, 72,  57, 24), # F
    QRectF(0, 69,  33, 15), # F#
    QRectF(0, 48,  57, 24), # G
    QRectF(0, 41,  33, 15), # G#
    QRectF(0, 24,  57, 24), # A
    QRectF(0, 13,  33, 15), # A#
    QRectF(0,  0,  57, 24), # B
]

kPcKeys_qwerty = [
    # 1st octave
    str(Qt.Key_Z),
    str(Qt.Key_S),
    str(Qt.Key_X),
    str(Qt.Key_D),
    str(Qt.Key_C),
    str(Qt.Key_V),
    str(Qt.Key_G),
    str(Qt.Key_B),
    str(Qt.Key_H),
    str(Qt.Key_N),
    str(Qt.Key_J),
    str(Qt.Key_M),
    # 2nd octave
    str(Qt.Key_Q),
    str(Qt.Key_2),
    str(Qt.Key_W),
    str(Qt.Key_3),
    str(Qt.Key_E),
    str(Qt.Key_R),
    str(Qt.Key_5),
    str(Qt.Key_T),
    str(Qt.Key_6),
    str(Qt.Key_Y),
    str(Qt.Key_7),
    str(Qt.Key_U),
    # 3rd octave
    str(Qt.Key_I),
    str(Qt.Key_9),
    str(Qt.Key_O),
    str(Qt.Key_0),
    str(Qt.Key_P),
]

kPcKeys_qwertz = [
    # 1st octave
    str(Qt.Key_Y),
    str(Qt.Key_S),
    str(Qt.Key_X),
    str(Qt.Key_D),
    str(Qt.Key_C),
    str(Qt.Key_V),
    str(Qt.Key_G),
    str(Qt.Key_B),
    str(Qt.Key_H),
    str(Qt.Key_N),
    str(Qt.Key_J),
    str(Qt.Key_M),
    # 2nd octave
    str(Qt.Key_Q),
    str(Qt.Key_2),
    str(Qt.Key_W),
    str(Qt.Key_3),
    str(Qt.Key_E),
    str(Qt.Key_R),
    str(Qt.Key_5),
    str(Qt.Key_T),
    str(Qt.Key_6),
    str(Qt.Key_Z),
    str(Qt.Key_7),
    str(Qt.Key_U),
    # 3rd octave
    str(Qt.Key_I),
    str(Qt.Key_9),
    str(Qt.Key_O),
    str(Qt.Key_0),
    str(Qt.Key_P),
]

kPcKeys_azerty = [
    # 1st octave
    str(Qt.Key_W),
    str(Qt.Key_S),
    str(Qt.Key_X),
    str(Qt.Key_D),
    str(Qt.Key_C),
    str(Qt.Key_V),
    str(Qt.Key_G),
    str(Qt.Key_B),
    str(Qt.Key_H),
    str(Qt.Key_N),
    str(Qt.Key_J),
    str(Qt.Key_Comma),
    # 2nd octave
    str(Qt.Key_A),
    str(Qt.Key_Eacute),
    str(Qt.Key_Z),
    str(Qt.Key_QuoteDbl),
    str(Qt.Key_E),
    str(Qt.Key_R),
    str(Qt.Key_ParenLeft),
    str(Qt.Key_T),
    str(Qt.Key_Minus),
    str(Qt.Key_Y),
    str(Qt.Key_Egrave ),
    str(Qt.Key_U),
    # 3rd octave
    str(Qt.Key_I),
    str(Qt.Key_Ccedilla),
    str(Qt.Key_O),
    str(Qt.Key_Agrave),
    str(Qt.Key_P),
]

kPcKeysLayouts = {
    'qwerty': kPcKeys_qwerty,
    'qwertz': kPcKeys_qwertz,
    'azerty': kPcKeys_azerty,
}

kBlackNotes = (1, 3, 6, 8, 10)

# ------------------------------------------------------------------------------------------------------------
# MIDI Keyboard, using a pixmap for painting

class PixmapKeyboard(QWidget):
    # signals
    noteOn   = pyqtSignal(int)
    noteOff  = pyqtSignal(int)
    notesOn  = pyqtSignal()
    notesOff = pyqtSignal()

    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.fEnabledKeys   = []
        self.fLastMouseNote = -1
        self.fStartOctave   = 0
        self.fPcKeybOffset  = 2
        self.fInitalizing   = True

        self.fFont = self.font()
        self.fFont.setFamily("Monospace")
        self.fFont.setPixelSize(12)
        self.fFont.setBold(True)

        self.fPixmapNormal   = QPixmap(":/bitmaps/kbd_normal.png")
        self.fPixmapDown     = QPixmap(":/bitmaps/kbd_down-blue.png")
        self.fHighlightColor = "Blue"

        self.fkPcKeyLayout = "qwerty"
        self.fkPcKeys      = kPcKeysLayouts["qwerty"]
        self.fKey2RectMap  = kMidiKey2RectMapHorizontal

        self.fWidth  = self.fPixmapNormal.width()
        self.fHeight = self.fPixmapNormal.height()

        self.setCursor(Qt.PointingHandCursor)
        self.setStartOctave(0)
        self.setOctaves(6)

        self.loadSettings()

        self.fInitalizing = False

    def saveSettings(self):
        if self.fInitalizing:
            return

        settings = QSettings("falkTX", "CarlaKeyboard")
        settings.setValue("PcKeyboardLayout", self.fkPcKeyLayout)
        settings.setValue("PcKeyboardOffset", self.fPcKeybOffset)
        settings.setValue("HighlightColor", self.fHighlightColor)
        del settings

    def loadSettings(self):
        settings = QSettings("falkTX", "CarlaKeyboard")
        self.setPcKeyboardLayout(settings.value("PcKeyboardLayout", "qwerty", type=str))
        self.setPcKeyboardOffset(settings.value("PcKeyboardOffset", 2, type=int))
        self.setColor(settings.value("HighlightColor", "Blue", type=str))
        del settings

    def allNotesOff(self, sendSignal=True):
        self.fEnabledKeys = []

        if sendSignal:
            self.notesOff.emit()

        self.update()

    def sendNoteOn(self, note, sendSignal=True):
        if 0 <= note <= 127 and note not in self.fEnabledKeys:
            self.fEnabledKeys.append(note)

            if sendSignal:
                self.noteOn.emit(note)

            self.update()

        if len(self.fEnabledKeys) == 1:
            self.notesOn.emit()

    def sendNoteOff(self, note, sendSignal=True):
        if 0 <= note <= 127 and note in self.fEnabledKeys:
            self.fEnabledKeys.remove(note)

            if sendSignal:
                self.noteOff.emit(note)

            self.update()

        if len(self.fEnabledKeys) == 0:
            self.notesOff.emit()

    def setColor(self, color):
        if color not in ("Blue", "Green", "Orange", "Red"):
            return

        if self.fHighlightColor == color:
            return

        self.fHighlightColor = color
        self.fPixmapDown.load(":/bitmaps/kbd_down-{}.png".format(color.lower()))
        self.saveSettings()

    def setPcKeyboardLayout(self, layout):
        if layout not in kPcKeysLayouts.keys():
            return

        if self.fkPcKeyLayout == layout:
            return

        self.fkPcKeyLayout = layout
        self.fkPcKeys = kPcKeysLayouts[layout]
        self.saveSettings()

    def setPcKeyboardOffset(self, offset):
        if offset < 0:
            offset = 0
        elif offset > 9:
            offset = 9

        if self.fPcKeybOffset == offset:
            return

        self.fPcKeybOffset = offset
        self.saveSettings()

    def setOctaves(self, octaves):
        if octaves < 1:
            octaves = 1
        elif octaves > 10:
            octaves = 10

        self.fOctaves = octaves

        self.setMinimumSize(self.fWidth * self.fOctaves, self.fHeight)
        self.setMaximumSize(self.fWidth * self.fOctaves, self.fHeight)

    def setStartOctave(self, octave):
        if octave < 0:
            octave = 0
        elif octave > 9:
            octave = 9

        if self.fStartOctave == octave:
            return

        self.fStartOctave = octave
        self.update()

    def handleMousePos(self, pos):
        if pos.x() < 0 or pos.x() > self.fOctaves * self.fWidth:
            return
        octave = int(pos.x() / self.fWidth)
        keyPos = QPointF(pos.x() % self.fWidth, pos.y())

        if self.fKey2RectMap[1].contains(keyPos):    # C#
            note = 1
        elif self.fKey2RectMap[ 3].contains(keyPos): # D#
            note = 3
        elif self.fKey2RectMap[ 6].contains(keyPos): # F#
            note = 6
        elif self.fKey2RectMap[ 8].contains(keyPos): # G#
            note = 8
        elif self.fKey2RectMap[10].contains(keyPos): # A#
            note = 10
        elif self.fKey2RectMap[ 0].contains(keyPos): # C
            note = 0
        elif self.fKey2RectMap[ 2].contains(keyPos): # D
            note = 2
        elif self.fKey2RectMap[ 4].contains(keyPos): # E
            note = 4
        elif self.fKey2RectMap[ 5].contains(keyPos): # F
            note = 5
        elif self.fKey2RectMap[ 7].contains(keyPos): # G
            note = 7
        elif self.fKey2RectMap[ 9].contains(keyPos): # A
            note = 9
        elif self.fKey2RectMap[11].contains(keyPos): # B
            note = 11
        else:
            note = -1

        if note != -1:
            note += (self.fStartOctave + octave) * 12

            if self.fLastMouseNote != note:
                self.sendNoteOff(self.fLastMouseNote)
                self.sendNoteOn(note)

        elif self.fLastMouseNote != -1:
            self.sendNoteOff(self.fLastMouseNote)

        self.fLastMouseNote = note

    def showOptions(self, event):
        event.accept()
        menu = QMenu()

        menu.addAction("Note: restart carla to apply globally").setEnabled(False)
        menu.addSeparator()

        menuColor      = QMenu("Highlight color", menu)
        actColorBlue   = menuColor.addAction("Blue")
        actColorGreen  = menuColor.addAction("Green")
        actColorOrange = menuColor.addAction("Orange")
        actColorRed    = menuColor.addAction("Red")
        actColors      = (actColorBlue, actColorGreen, actColorOrange, actColorRed)

        for act in actColors:
            act.setCheckable(True)
            if self.fHighlightColor == act.text():
                act.setChecked(True)

        menuLayout       = QMenu("PC Keyboard layout", menu)
        actLayout_qwerty = menuLayout.addAction("qwerty")
        actLayout_qwertz = menuLayout.addAction("qwertz")
        actLayout_azerty = menuLayout.addAction("azerty")
        actLayouts       = (actLayout_qwerty, actLayout_qwertz, actLayout_azerty)

        for act in actLayouts:
            act.setCheckable(True)
            if self.fkPcKeyLayout == act.text():
                act.setChecked(True)

        menu.addMenu(menuColor)
        menu.addMenu(menuLayout)
        menu.addSeparator()

        actOctaveUp   = menu.addAction("PC Keyboard octave up")
        actOctaveDown = menu.addAction("PC Keyboard octave down")

        if self.fPcKeybOffset == 0:
            actOctaveDown.setEnabled(False)

        actSelected = menu.exec_(event.screenPos().toPoint())

        if not actSelected:
            return

        if actSelected in actColors:
            return self.setColor(actSelected.text())

        if actSelected in actLayouts:
            return self.setPcKeyboardLayout(actSelected.text())

        if actSelected == actOctaveUp:
            return self.setPcKeyboardOffset(self.fPcKeybOffset + 1)

        if actSelected == actOctaveDown:
            return self.setPcKeyboardOffset(self.fPcKeybOffset - 1)

    def minimumSizeHint(self):
        return QSize(self.fWidth, self.fHeight)

    def sizeHint(self):
        return QSize(self.fWidth * self.fOctaves, self.fHeight)

    def keyPressEvent(self, event):
        if not event.isAutoRepeat():
            try:
                qKey  = str(event.key())
                index = self.fkPcKeys.index(qKey)
            except:
                pass
            else:
                self.sendNoteOn(index+(self.fPcKeybOffset*12))

        QWidget.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if not event.isAutoRepeat():
            try:
                qKey  = str(event.key())
                index = self.fkPcKeys.index(qKey)
            except:
                pass
            else:
                self.sendNoteOff(index+(self.fPcKeybOffset*12))

        QWidget.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        if event.button() == Qt.RightButton:
            self.showOptions(event)
        else:
            self.fLastMouseNote = -1
            self.handleMousePos(event.pos())
            self.setFocus()
        QWidget.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if event.button() != Qt.RightButton:
            self.handleMousePos(event.pos())
        QWidget.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.fLastMouseNote != -1:
            self.sendNoteOff(self.fLastMouseNote)
            self.fLastMouseNote = -1
        QWidget.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        # -------------------------------------------------------------
        # Paint clean keys (as background)

        for octave in range(self.fOctaves):
            target = QRectF(self.fWidth * octave, 0, self.fWidth, self.fHeight)
            source = QRectF(0, 0, self.fWidth, self.fHeight)
            painter.drawPixmap(target, self.fPixmapNormal, source)

        if not self.isEnabled():
            painter.setBrush(QColor(0, 0, 0, 150))
            painter.setPen(QColor(0, 0, 0, 150))
            painter.drawRect(0, 0, self.width(), self.height())
            return

        # -------------------------------------------------------------
        # Paint (white) pressed keys

        paintedWhite = False

        for note in self.fEnabledKeys:
            pos = self._getRectFromMidiNote(note)

            if self._isNoteBlack(note):
                continue

            if note < 12:
                octave = 0
            elif note < 24:
                octave = 1
            elif note < 36:
                octave = 2
            elif note < 48:
                octave = 3
            elif note < 60:
                octave = 4
            elif note < 72:
                octave = 5
            elif note < 84:
                octave = 6
            elif note < 96:
                octave = 7
            elif note < 108:
                octave = 8
            elif note < 120:
                octave = 9
            elif note < 132:
                octave = 10
            else:
                # cannot paint this note
                continue

            octave -= self.fStartOctave

            target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
            source = QRectF(pos.x(), 0, pos.width(), pos.height())

            paintedWhite = True
            painter.drawPixmap(target, self.fPixmapDown, source)

        # -------------------------------------------------------------
        # Clear white keys border

        if paintedWhite:
            for octave in range(self.fOctaves):
                for note in kBlackNotes:
                    pos = self._getRectFromMidiNote(note)

                    target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
                    source = QRectF(pos.x(), 0, pos.width(), pos.height())

                    painter.drawPixmap(target, self.fPixmapNormal, source)

        # -------------------------------------------------------------
        # Paint (black) pressed keys

        for note in self.fEnabledKeys:
            pos = self._getRectFromMidiNote(note)

            if not self._isNoteBlack(note):
                continue

            if note < 12:
                octave = 0
            elif note < 24:
                octave = 1
            elif note < 36:
                octave = 2
            elif note < 48:
                octave = 3
            elif note < 60:
                octave = 4
            elif note < 72:
                octave = 5
            elif note < 84:
                octave = 6
            elif note < 96:
                octave = 7
            elif note < 108:
                octave = 8
            elif note < 120:
                octave = 9
            elif note < 132:
                octave = 10
            else:
                # cannot paint this note
                continue

            octave -= self.fStartOctave

            target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
            source = QRectF(pos.x(), 0, pos.width(), pos.height())

            painter.drawPixmap(target, self.fPixmapDown, source)

        # Paint C-number note info
        painter.setFont(self.fFont)
        painter.setPen(Qt.black)

        for i in range(self.fOctaves):
            octave = self.fStartOctave + i - 1
            painter.drawText(i * 168 + (4 if octave == -1 else 3),
                             35, 20, 20,
                             Qt.AlignCenter,
                             "C{}".format(octave))

    def _isNoteBlack(self, note):
        baseNote = note % 12
        return bool(baseNote in kBlackNotes)

    def _getRectFromMidiNote(self, note):
        baseNote = note % 12
        return self.fKey2RectMap[baseNote]

# ------------------------------------------------------------------------------------------------------------
# Horizontal scroll area for keyboard

class PixmapKeyboardHArea(QScrollArea):
    def __init__(self, parent):
        QScrollArea.__init__(self, parent)

        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.keyboard = PixmapKeyboard(self)
        self.keyboard.setOctaves(10)
        self.setWidget(self.keyboard)

        self.setEnabled(False)
        self.setFixedHeight(self.keyboard.height() + self.horizontalScrollBar().height()/2 + 2)

        QTimer.singleShot(0, self.slot_initScrollbarValue)

    # FIXME use change event
    def setEnabled(self, yesNo):
        self.keyboard.setEnabled(yesNo)
        QScrollArea.setEnabled(self, yesNo)

    @pyqtSlot()
    def slot_initScrollbarValue(self):
        self.horizontalScrollBar().setValue(self.horizontalScrollBar().maximum()/2)

# ------------------------------------------------------------------------------------------------------------
# Main Testing

if __name__ == '__main__':
    import sys
    from PyQt5.QtWidgets import QApplication
    import resources_rc

    app = QApplication(sys.argv)

    gui = PixmapKeyboard(None)
    gui.setEnabled(True)
    gui.show()

    sys.exit(app.exec_())
