#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Keyboard, a custom Qt4 widget
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

from PyQt4.QtCore import pyqtSlot, qCritical, Qt, QPointF, QRectF, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QFont, QPainter, QPixmap, QWidget

# ------------------------------------------------------------------------------------------------------------

kMidiKey2RectMapHorizontal = {
    '0':  QRectF(0,   0, 18, 64), # C
    '1':  QRectF(13,  0, 11, 42), # C#
    '2':  QRectF(18,  0, 25, 64), # D
    '3':  QRectF(37,  0, 11, 42), # D#
    '4':  QRectF(42,  0, 18, 64), # E
    '5':  QRectF(60,  0, 18, 64), # F
    '6':  QRectF(73,  0, 11, 42), # F#
    '7':  QRectF(78,  0, 25, 64), # G
    '8':  QRectF(97,  0, 11, 42), # G#
    '9':  QRectF(102, 0, 25, 64), # A
    '10': QRectF(121, 0, 11, 42), # A#
    '11': QRectF(126, 0, 18, 64)  # B
}

kMidiKey2RectMapVertical = {
    '11': QRectF(0,  0,  64, 18), # B
    '10': QRectF(0, 14,  42,  7), # A#
    '9':  QRectF(0, 18,  64, 24), # A
    '8':  QRectF(0, 38,  42,  7), # G#
    '7':  QRectF(0, 42,  64, 24), # G
    '6':  QRectF(0, 62,  42,  7), # F#
    '5':  QRectF(0, 66,  64, 18), # F
    '4':  QRectF(0, 84,  64, 18), # E
    '3':  QRectF(0, 98,  42,  7), # D#
    '2':  QRectF(0, 102, 64, 24), # D
    '1':  QRectF(0, 122, 42,  7), # C#
    '0':  QRectF(0, 126, 64, 18)  # C
}

kMidiKeyboard2KeyMap = {
    # 3th octave
    '%i' % Qt.Key_Z: 48,
    '%i' % Qt.Key_S: 49,
    '%i' % Qt.Key_X: 50,
    '%i' % Qt.Key_D: 51,
    '%i' % Qt.Key_C: 52,
    '%i' % Qt.Key_V: 53,
    '%i' % Qt.Key_G: 54,
    '%i' % Qt.Key_B: 55,
    '%i' % Qt.Key_H: 56,
    '%i' % Qt.Key_N: 57,
    '%i' % Qt.Key_J: 58,
    '%i' % Qt.Key_M: 59,
    # 4th octave
    '%i' % Qt.Key_Q: 60,
    '%i' % Qt.Key_2: 61,
    '%i' % Qt.Key_W: 62,
    '%i' % Qt.Key_3: 63,
    '%i' % Qt.Key_E: 64,
    '%i' % Qt.Key_R: 65,
    '%i' % Qt.Key_5: 66,
    '%i' % Qt.Key_T: 67,
    '%i' % Qt.Key_6: 68,
    '%i' % Qt.Key_Y: 69,
    '%i' % Qt.Key_7: 70,
    '%i' % Qt.Key_U: 71,
}

kBlackNotes = (1, 3, 6, 8, 10)

# ------------------------------------------------------------------------------------------------------------
# MIDI Keyboard, using a pixmap for painting

class PixmapKeyboard(QWidget):
    # enum Color
    COLOR_CLASSIC = 0
    COLOR_ORANGE  = 1

    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.fOctaves = 6
        self.fLastMouseNote = -1

        self.fEnabledKeys = []

        self.fFont   = QFont("Monospace", 7, QFont.Normal)
        self.fPixmap = QPixmap("")

        self.setCursor(Qt.PointingHandCursor)
        self.setMode(self.HORIZONTAL)

    def allNotesOff(self):
        self.fEnabledKeys = []

        self.emit(SIGNAL("notesOff()"))
        self.update()

    def sendNoteOn(self, note, sendSignal=True):
        if 0 <= note <= 127 and note not in self.fEnabledKeys:
            self.fEnabledKeys.append(note)

            if sendSignal:
                self.emit(SIGNAL("noteOn(int)"), note)

            self.update()

        if len(self.fEnabledKeys) == 1:
            self.emit(SIGNAL("notesOn()"))

    def sendNoteOff(self, note, sendSignal=True):
        if 0 <= note <= 127 and note in self.fEnabledKeys:
            self.fEnabledKeys.remove(note)

            if sendSignal:
                self.emit(SIGNAL("noteOff(int)"), note)

            self.update()

        if len(self.fEnabledKeys) == 0:
            self.emit(SIGNAL("notesOff()"))

    def setMode(self, mode, color=COLOR_ORANGE):
        if color == self.COLOR_CLASSIC:
            self.fColorStr = "classic"
        elif color == self.COLOR_ORANGE:
            self.fColorStr = "orange"
        else:
            qCritical("PixmapKeyboard::setMode(%i, %i) - invalid color" % (mode, color))
            return self.setMode(mode)

        if mode == self.HORIZONTAL:
            self.fMidiMap = kMidiKey2RectMapHorizontal
            self.fPixmap.load(":/bitmaps/kbd_h_%s.png" % self.fColorStr)
            self.fPixmapMode = self.HORIZONTAL
            self.fWidth  = self.fPixmap.width()
            self.fHeight = self.fPixmap.height() / 2
        elif mode == self.VERTICAL:
            self.fMidiMap = kMidiKey2RectMapVertical
            self.fPixmap.load(":/bitmaps/kbd_v_%s.png" % self.fColorStr)
            self.fPixmapMode = self.VERTICAL
            self.fWidth  = self.fPixmap.width() / 2
            self.fHeight = self.fPixmap.height()
        else:
            qCritical("PixmapKeyboard::setMode(%i, %i) - invalid mode" % (mode, color))
            return self.setMode(self.HORIZONTAL)

        self.setOctaves(self.fOctaves)

    def setOctaves(self, octaves):
        if octaves < 1:
            octaves = 1
        elif octaves > 10:
            octaves = 10

        self.fOctaves = octaves

        if self.fPixmapMode == self.HORIZONTAL:
            self.setMinimumSize(self.fWidth * self.fOctaves, self.fHeight)
            self.setMaximumSize(self.fWidth * self.fOctaves, self.fHeight)
        elif self.fPixmapMode == self.VERTICAL:
            self.setMinimumSize(self.fWidth, self.fHeight * self.fOctaves)
            self.setMaximumSize(self.fWidth, self.fHeight * self.fOctaves)

        self.update()

    def handleMousePos(self, pos):
        if self.fPixmapMode == self.HORIZONTAL:
            if pos.x() < 0 or pos.x() > self.fOctaves * 144:
                return
            posX   = pos.x() - 1
            octave = int(posX / self.fWidth)
            keyPos = QPointF(posX % self.fWidth, pos.y())
        elif self.fPixmapMode == self.VERTICAL:
            if pos.y() < 0 or pos.y() > self.fOctaves * 144:
                return
            posY   = pos.y() - 1
            octave = int(self.fOctaves - posY / self.fHeight)
            keyPos = QPointF(pos.x(), posY % self.fHeight)
        else:
            return

        if self.fMidiMap['1'].contains(keyPos):   # C#
            note = 1
        elif self.fMidiMap['3'].contains(keyPos): # D#
            note = 3
        elif self.fMidiMap['6'].contains(keyPos): # F#
            note = 6
        elif self.fMidiMap['8'].contains(keyPos): # G#
            note = 8
        elif self.fMidiMap['10'].contains(keyPos):# A#
            note = 10
        elif self.fMidiMap['0'].contains(keyPos): # C
            note = 0
        elif self.fMidiMap['2'].contains(keyPos): # D
            note = 2
        elif self.fMidiMap['4'].contains(keyPos): # E
            note = 4
        elif self.fMidiMap['5'].contains(keyPos): # F
            note = 5
        elif self.fMidiMap['7'].contains(keyPos): # G
            note = 7
        elif self.fMidiMap['9'].contains(keyPos): # A
            note = 9
        elif self.fMidiMap['11'].contains(keyPos):# B
            note = 11
        else:
            note = -1

        if note != -1:
            note += octave * 12

            if self.fLastMouseNote != note:
                self.sendNoteOff(self.fLastMouseNote)
                self.sendNoteOn(note)

        elif self.fLastMouseNote != -1:
            self.sendNoteOff(self.fLastMouseNote)

        self.fLastMouseNote = note

    def keyPressEvent(self, event):
        if not event.isAutoRepeat():
            qKey = str(event.key())
            if qKey in kMidiKeyboard2KeyMap.keys():
                self.sendNoteOn(kMidiKeyboard2KeyMap.get(qKey))
        QWidget.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if not event.isAutoRepeat():
            qKey = str(event.key())
            if qKey in kMidiKeyboard2KeyMap.keys():
                self.sendNoteOff(kMidiKeyboard2KeyMap.get(qKey))
        QWidget.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        self.fLastMouseNote = -1
        self.handleMousePos(event.pos())
        self.setFocus()
        QWidget.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
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
            if self.fPixmapMode == self.HORIZONTAL:
                target = QRectF(self.fWidth * octave, 0, self.fWidth, self.fHeight)
            elif self.fPixmapMode == self.VERTICAL:
                target = QRectF(0, self.fHeight * octave, self.fWidth, self.fHeight)
            else:
                return

            source = QRectF(0, 0, self.fWidth, self.fHeight)
            painter.drawPixmap(target, self.fPixmap, source)

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

            if self.fPixmapMode == self.VERTICAL:
                octave = self.fOctaves - octave - 1

            if self.fPixmapMode == self.HORIZONTAL:
                target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
                source = QRectF(pos.x(), self.fHeight, pos.width(), pos.height())
            elif self.fPixmapMode == self.VERTICAL:
                target = QRectF(pos.x(), pos.y() + (self.fHeight * octave), pos.width(), pos.height())
                source = QRectF(self.fWidth, pos.y(), pos.width(), pos.height())
            else:
                return

            paintedWhite = True
            painter.drawPixmap(target, self.fPixmap, source)

        # -------------------------------------------------------------
        # Clear white keys border

        if paintedWhite:
            for octave in range(self.fOctaves):
                for note in kBlackNotes:
                    pos = self._getRectFromMidiNote(note)

                    if self.fPixmapMode == self.HORIZONTAL:
                        target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
                        source = QRectF(pos.x(), 0, pos.width(), pos.height())
                    elif self.fPixmapMode == self.VERTICAL:
                        target = QRectF(pos.x(), pos.y() + (self.fHeight * octave), pos.width(), pos.height())
                        source = QRectF(0, pos.y(), pos.width(), pos.height())
                    else:
                        return

                    painter.drawPixmap(target, self.fPixmap, source)

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

            if self.fPixmapMode == self.VERTICAL:
                octave = self.fOctaves - octave - 1

            if self.fPixmapMode == self.HORIZONTAL:
                target = QRectF(pos.x() + (self.fWidth * octave), 0, pos.width(), pos.height())
                source = QRectF(pos.x(), self.fHeight, pos.width(), pos.height())
            elif self.fPixmapMode == self.VERTICAL:
                target = QRectF(pos.x(), pos.y() + (self.fHeight * octave), pos.width(), pos.height())
                source = QRectF(self.fWidth, pos.y(), pos.width(), pos.height())
            else:
                return

            painter.drawPixmap(target, self.fPixmap, source)

        # Paint C-number note info
        painter.setFont(self.fFont)
        painter.setPen(Qt.black)

        for i in range(self.fOctaves):
            if self.fPixmapMode == self.HORIZONTAL:
                painter.drawText(i * 144, 48, 18, 18, Qt.AlignCenter, "C%i" % (i-1))
            elif self.fPixmapMode == self.VERTICAL:
                painter.drawText(45, (self.fOctaves * 144) - (i * 144) - 16, 18, 18, Qt.AlignCenter, "C%i" % (i-1))

    def _isNoteBlack(self, note):
        baseNote = note % 12
        return bool(baseNote in kBlackNotes)

    def _getRectFromMidiNote(self, note):
        baseNote = note % 12
        return self.fMidiMap.get(str(baseNote))
