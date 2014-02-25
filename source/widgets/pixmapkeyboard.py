#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Keyboard, a custom Qt4 widget
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
# Imports (Global)

from PyQt4.QtCore import pyqtSignal, pyqtSlot, qCritical, Qt, QPointF, QRectF, QTimer, QSize
from PyQt4.QtGui import QColor, QFont, QPainter, QPixmap, QScrollArea, QWidget

# ------------------------------------------------------------------------------------------------------------

kMidiKey2RectMapHorizontal = {
    '0':  QRectF(0,   0, 24, 57), # C
    '1':  QRectF(14,  0, 15, 33), # C#
    '2':  QRectF(24,  0, 24, 57), # D
    '3':  QRectF(42,  0, 15, 33), # D#
    '4':  QRectF(48,  0, 24, 57), # E
    '5':  QRectF(72,  0, 24, 57), # F
    '6':  QRectF(84,  0, 15, 33), # F#
    '7':  QRectF(96,  0, 24, 57), # G
    '8':  QRectF(112, 0, 15, 33), # G#
    '9':  QRectF(120, 0, 24, 57), # A
    '10': QRectF(140, 0, 15, 33), # A#
    '11': QRectF(144, 0, 24, 57)  # B
}

kMidiKey2RectMapVertical = {
    '11': QRectF(0,  0,  57, 24), # B
    '10': QRectF(0, 13,  33, 15), # A#
    '9':  QRectF(0, 24,  57, 24), # A
    '8':  QRectF(0, 41,  33, 15), # G#
    '7':  QRectF(0, 48,  57, 24), # G
    '6':  QRectF(0, 69,  33, 15), # F#
    '5':  QRectF(0, 72,  57, 24), # F
    '4':  QRectF(0, 96,  57, 24), # E
    '3':  QRectF(0, 111, 33, 15), # D#
    '2':  QRectF(0, 120, 57, 24), # D
    '1':  QRectF(0, 139, 33, 15), # C#
    '0':  QRectF(0, 144, 57, 24)  # C
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
    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    # signals
    noteOn   = pyqtSignal(int)
    noteOff  = pyqtSignal(int)
    notesOn  = pyqtSignal()
    notesOff = pyqtSignal()

    def __init__(self, parent):
        QWidget.__init__(self, parent)

        self.fOctaves = 6
        self.fLastMouseNote = -1

        self.fEnabledKeys = []

        self.fFont = self.font()
        self.fFont.setFamily("Monospace")
        self.fFont.setPixelSize(12)
        self.fFont.setBold(True)

        self.fPixmap = QPixmap("")

        self.setCursor(Qt.PointingHandCursor)
        self.setMode(self.HORIZONTAL)

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

    def setMode(self, mode):
        if mode == self.HORIZONTAL:
            self.fMidiMap = kMidiKey2RectMapHorizontal
            self.fPixmap.load(":/bitmaps/kbd_h_dark.png")
            self.fPixmapMode = self.HORIZONTAL
            self.fWidth  = self.fPixmap.width()
            self.fHeight = self.fPixmap.height() / 2
        elif mode == self.VERTICAL:
            self.fMidiMap = kMidiKey2RectMapVertical
            self.fPixmap.load(":/bitmaps/kbd_v_dark.png")
            self.fPixmapMode = self.VERTICAL
            self.fWidth  = self.fPixmap.width() / 2
            self.fHeight = self.fPixmap.height()
        else:
            qCritical("PixmapKeyboard::setMode(%i) - invalid mode" % mode)
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
            if pos.x() < 0 or pos.x() > self.fOctaves * self.fWidth:
                return
            octave = int(pos.x() / self.fWidth)
            keyPos = QPointF(pos.x() % self.fWidth, pos.y())
        elif self.fPixmapMode == self.VERTICAL:
            if pos.y() < 0 or pos.y() > self.fOctaves * self.fHeight:
                return
            octave = int(self.fOctaves - pos.y() / self.fHeight)
            keyPos = QPointF(pos.x(), (pos.y()-1) % self.fHeight)
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

    def minimumSizeHint(self):
        return QSize(self.fWidth, self.fHeight)

    def sizeHint(self):
        if self.fPixmapMode == self.HORIZONTAL:
            return QSize(self.fWidth * self.fOctaves, self.fHeight)
        elif self.fPixmapMode == self.VERTICAL:
            return QSize(self.fWidth, self.fHeight * self.fOctaves)
        else:
            return QSize(self.fWidth, self.fHeight)

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
                painter.drawText(i * 168 + (4 if i == 0 else 3), 35, 20, 20, Qt.AlignCenter, "C%i" % (i-1))
            elif self.fPixmapMode == self.VERTICAL:
                painter.drawText(33, (self.fOctaves * 168) - (i * 168) - 20, 20, 20, Qt.AlignCenter, "C%i" % (i-1))

    def _isNoteBlack(self, note):
        baseNote = note % 12
        return bool(baseNote in kBlackNotes)

    def _getRectFromMidiNote(self, note):
        baseNote = note % 12
        return self.fMidiMap.get(str(baseNote))

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
    from PyQt4.QtGui import QApplication
    import resources_rc

    app = QApplication(sys.argv)

    gui = PixmapKeyboard(None)
    gui.setMode(gui.HORIZONTAL)
    #gui.setMode(gui.VERTICAL)
    gui.setEnabled(True)
    gui.show()

    sys.exit(app.exec_())
