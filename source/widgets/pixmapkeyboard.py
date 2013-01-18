#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Keyboard, a custom Qt4 widget
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
from PyQt4.QtCore import pyqtSlot, qCritical, Qt, QPointF, QRectF, QTimer, SIGNAL, SLOT
from PyQt4.QtGui import QFont, QPainter, QPixmap, QWidget

midi_key2rect_map_horizontal = {
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

midi_key2rect_map_vertical = {
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

midi_keyboard2key_map = {
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

        self.m_octaves = 6
        self.m_lastMouseNote = -1

        self.m_needsUpdate = False
        self.m_enabledKeys = []

        self.m_font   = QFont("Monospace", 8, QFont.Normal)
        self.m_pixmap = QPixmap("")

        self.setCursor(Qt.PointingHandCursor)
        self.setMode(self.HORIZONTAL)

    def allNotesOff(self):
        self.m_enabledKeys = []

        self.m_needsUpdate = True
        QTimer.singleShot(0, self, SLOT("slot_updateOnce()"))

        self.emit(SIGNAL("notesOff()"))

    def sendNoteOn(self, note, sendSignal=True):
        if 0 <= note <= 127 and note not in self.m_enabledKeys:
            self.m_enabledKeys.append(note)
            if sendSignal:
                self.emit(SIGNAL("noteOn(int)"), note)

            self.m_needsUpdate = True
            QTimer.singleShot(0, self, SLOT("slot_updateOnce()"))

        if len(self.m_enabledKeys) == 1:
            self.emit(SIGNAL("notesOn()"))

    def sendNoteOff(self, note, sendSignal=True):
        if 0 <= note <= 127 and note in self.m_enabledKeys:
            self.m_enabledKeys.remove(note)
            if sendSignal:
                self.emit(SIGNAL("noteOff(int)"), note)

            self.m_needsUpdate = True
            QTimer.singleShot(0, self, SLOT("slot_updateOnce()"))

        if len(self.m_enabledKeys) == 0:
            self.emit(SIGNAL("notesOff()"))

    def setMode(self, mode, color=COLOR_ORANGE):
        if color == self.COLOR_CLASSIC:
            self.m_colorStr = "classic"
        elif color == self.COLOR_ORANGE:
            self.m_colorStr = "orange"
        else:
            qCritical("PixmapKeyboard::setMode(%i, %i) - invalid color" % (mode, color))
            return self.setMode(mode)

        if mode == self.HORIZONTAL:
            self.m_midi_map = midi_key2rect_map_horizontal
            self.m_pixmap.load(":/bitmaps/kbd_h_%s.png" % self.m_colorStr)
            self.m_pixmap_mode = self.HORIZONTAL
            self.p_width  = self.m_pixmap.width()
            self.p_height = self.m_pixmap.height() / 2
        elif mode == self.VERTICAL:
            self.m_midi_map = midi_key2rect_map_vertical
            self.m_pixmap.load(":/bitmaps/kbd_v_%s.png" % self.m_colorStr)
            self.m_pixmap_mode = self.VERTICAL
            self.p_width  = self.m_pixmap.width() / 2
            self.p_height = self.m_pixmap.height()
        else:
            qCritical("PixmapKeyboard::setMode(%i, %i) - invalid mode" % (mode, color))
            return self.setMode(self.HORIZONTAL)

        self.setOctaves(self.m_octaves)

    def setOctaves(self, octaves):
        if octaves < 1:
            octaves = 1
        elif octaves > 8:
            octaves = 8
        self.m_octaves = octaves

        if self.m_pixmap_mode == self.HORIZONTAL:
            self.setMinimumSize(self.p_width * self.m_octaves, self.p_height)
            self.setMaximumSize(self.p_width * self.m_octaves, self.p_height)
        elif self.m_pixmap_mode == self.VERTICAL:
            self.setMinimumSize(self.p_width, self.p_height * self.m_octaves)
            self.setMaximumSize(self.p_width, self.p_height * self.m_octaves)

        self.update()

    def handleMousePos(self, pos):
        if self.m_pixmap_mode == self.HORIZONTAL:
            if pos.x() < 0 or pos.x() > self.m_octaves * 144:
                return
            posX = pos.x() - 1
            octave = int(posX / self.p_width)
            n_pos  = QPointF(posX % self.p_width, pos.y())
        elif self.m_pixmap_mode == self.VERTICAL:
            if pos.y() < 0 or pos.y() > self.m_octaves * 144:
                return
            posY = pos.y() - 1
            octave = int(self.m_octaves - posY / self.p_height)
            n_pos  = QPointF(pos.x(), posY % self.p_height)
        else:
            return

        octave += 3

        if self.m_midi_map['1'].contains(n_pos):   # C#
            note = 1
        elif self.m_midi_map['3'].contains(n_pos): # D#
            note = 3
        elif self.m_midi_map['6'].contains(n_pos): # F#
            note = 6
        elif self.m_midi_map['8'].contains(n_pos): # G#
            note = 8
        elif self.m_midi_map['10'].contains(n_pos):# A#
            note = 10
        elif self.m_midi_map['0'].contains(n_pos): # C
            note = 0
        elif self.m_midi_map['2'].contains(n_pos): # D
            note = 2
        elif self.m_midi_map['4'].contains(n_pos): # E
            note = 4
        elif self.m_midi_map['5'].contains(n_pos): # F
            note = 5
        elif self.m_midi_map['7'].contains(n_pos): # G
            note = 7
        elif self.m_midi_map['9'].contains(n_pos): # A
            note = 9
        elif self.m_midi_map['11'].contains(n_pos):# B
            note = 11
        else:
            note = -1

        if note != -1:
            note += octave * 12
            if self.m_lastMouseNote != note:
                self.sendNoteOff(self.m_lastMouseNote)
                self.sendNoteOn(note)
        else:
            self.sendNoteOff(self.m_lastMouseNote)

        self.m_lastMouseNote = note

    def keyPressEvent(self, event):
        if not event.isAutoRepeat():
            qKey = str(event.key())
            if qKey in midi_keyboard2key_map.keys():
                self.sendNoteOn(midi_keyboard2key_map.get(qKey))
        QWidget.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if not event.isAutoRepeat():
            qKey = str(event.key())
            if qKey in midi_keyboard2key_map.keys():
                self.sendNoteOff(midi_keyboard2key_map.get(qKey))
        QWidget.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        self.m_lastMouseNote = -1
        self.handleMousePos(event.pos())
        self.setFocus()
        QWidget.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        self.handleMousePos(event.pos())
        QWidget.mousePressEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.m_lastMouseNote != -1:
            self.sendNoteOff(self.m_lastMouseNote)
            self.m_lastMouseNote = -1
        QWidget.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)

        # -------------------------------------------------------------
        # Paint clean keys (as background)

        for octave in range(self.m_octaves):
            if self.m_pixmap_mode == self.HORIZONTAL:
                target = QRectF(self.p_width * octave, 0, self.p_width, self.p_height)
            elif self.m_pixmap_mode == self.VERTICAL:
                target = QRectF(0, self.p_height * octave, self.p_width, self.p_height)
            else:
                return

            source = QRectF(0, 0, self.p_width, self.p_height)
            painter.drawPixmap(target, self.m_pixmap, source)

        # -------------------------------------------------------------
        # Paint (white) pressed keys

        paintedWhite = False

        for i in range(len(self.m_enabledKeys)):
            note = self.m_enabledKeys[i]
            pos = self._getRectFromMidiNote(note)

            if self._isNoteBlack(note):
                continue

            if note < 36:
                # cannot paint this note
                continue
            elif note < 48:
                octave = 0
            elif note < 60:
                octave = 1
            elif note < 72:
                octave = 2
            elif note < 84:
                octave = 3
            elif note < 96:
                octave = 4
            elif note < 108:
                octave = 5
            elif note < 120:
                octave = 6
            elif note < 132:
                octave = 7
            else:
                # cannot paint this note either
                continue

            if self.m_pixmap_mode == self.VERTICAL:
                octave = self.m_octaves - octave - 1

            if self.m_pixmap_mode == self.HORIZONTAL:
                target = QRectF(pos.x() + (self.p_width * octave), 0, pos.width(), pos.height())
                source = QRectF(pos.x(), self.p_height, pos.width(), pos.height())
            elif self.m_pixmap_mode == self.VERTICAL:
                target = QRectF(pos.x(), pos.y() + (self.p_height * octave), pos.width(), pos.height())
                source = QRectF(self.p_width, pos.y(), pos.width(), pos.height())
            else:
                return

            paintedWhite = True
            painter.drawPixmap(target, self.m_pixmap, source)

        # -------------------------------------------------------------
        # Clear white keys border

        if paintedWhite:
            for octave in range(self.m_octaves):
                for note in (1, 3, 6, 8, 10):
                    pos = self._getRectFromMidiNote(note)
                    if self.m_pixmap_mode == self.HORIZONTAL:
                        target = QRectF(pos.x() + (self.p_width * octave), 0, pos.width(), pos.height())
                        source = QRectF(pos.x(), 0, pos.width(), pos.height())
                    elif self.m_pixmap_mode == self.VERTICAL:
                        target = QRectF(pos.x(), pos.y() + (self.p_height * octave), pos.width(), pos.height())
                        source = QRectF(0, pos.y(), pos.width(), pos.height())
                    else:
                        return

                    painter.drawPixmap(target, self.m_pixmap, source)

        # -------------------------------------------------------------
        # Paint (black) pressed keys

        for i in range(len(self.m_enabledKeys)):
            note = self.m_enabledKeys[i]
            pos = self._getRectFromMidiNote(note)

            if not self._isNoteBlack(note):
                continue

            if note < 36:
                # cannot paint this note
                continue
            elif note < 48:
                octave = 0
            elif note < 60:
                octave = 1
            elif note < 72:
                octave = 2
            elif note < 84:
                octave = 3
            elif note < 96:
                octave = 4
            elif note < 108:
                octave = 5
            elif note < 120:
                octave = 6
            elif note < 132:
                octave = 7
            else:
                # cannot paint this note either
                continue

            if self.m_pixmap_mode == self.VERTICAL:
                octave = self.m_octaves - octave - 1

            if self.m_pixmap_mode == self.HORIZONTAL:
                target = QRectF(pos.x() + (self.p_width * octave), 0, pos.width(), pos.height())
                source = QRectF(pos.x(), self.p_height, pos.width(), pos.height())
            elif self.m_pixmap_mode == self.VERTICAL:
                target = QRectF(pos.x(), pos.y() + (self.p_height * octave), pos.width(), pos.height())
                source = QRectF(self.p_width, pos.y(), pos.width(), pos.height())
            else:
                return

            painter.drawPixmap(target, self.m_pixmap, source)

        # Paint C-number note info
        painter.setFont(self.m_font)
        painter.setPen(Qt.black)

        for i in range(self.m_octaves):
            if self.m_pixmap_mode == self.HORIZONTAL:
                painter.drawText(i * 144, 48, 18, 18, Qt.AlignCenter, "C%i" % int(i + 2))
            elif self.m_pixmap_mode == self.VERTICAL:
                painter.drawText(45, (self.m_octaves * 144) - (i * 144) - 16, 18, 18, Qt.AlignCenter, "C%i" % int(i + 2))

    @pyqtSlot()
    def slot_updateOnce(self):
        if self.m_needsUpdate:
            self.update()
            self.m_needsUpdate = False

    def _isNoteBlack(self, note):
        baseNote = note % 12
        return bool(baseNote in (1, 3, 6, 8, 10))

    def _getRectFromMidiNote(self, note):
        return self.m_midi_map.get(str(note % 12))
