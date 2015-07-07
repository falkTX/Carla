#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# A piano roll viewer/editor
# Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
# Copyright (C) 2014-2015 Perry Nguyen
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
    from PyQt5.QtCore import Qt, QRectF, QPointF, pyqtSignal
    from PyQt5.QtGui import QColor, QFont, QPen, QPainter
    from PyQt5.QtWidgets import QGraphicsItem, QGraphicsLineItem, QGraphicsOpacityEffect, QGraphicsRectItem, QGraphicsSimpleTextItem
    from PyQt5.QtWidgets import QGraphicsScene, QGraphicsView
    from PyQt5.QtWidgets import QWidget, QLabel, QComboBox, QHBoxLayout, QVBoxLayout, QStyle
else:
    from PyQt4.QtCore import Qt, QRectF, QPointF, pyqtSignal
    from PyQt4.QtGui import QColor, QFont, QPen, QPainter
    from PyQt4.QtGui import QGraphicsItem, QGraphicsLineItem, QGraphicsOpacityEffect, QGraphicsRectItem, QGraphicsSimpleTextItem
    from PyQt4.QtGui import QGraphicsScene, QGraphicsView
    from PyQt4.QtGui import QWidget, QLabel, QComboBox, QSlider, QHBoxLayout, QVBoxLayout, QStyle

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# MIDI definitions, copied from CarlaMIDI.h

MAX_MIDI_CHANNELS = 16
MAX_MIDI_NOTE     = 128
MAX_MIDI_VALUE    = 128
MAX_MIDI_CONTROL  = 120 # 0x77

MIDI_STATUS_BIT  = 0xF0
MIDI_CHANNEL_BIT = 0x0F

# MIDI Messages List
MIDI_STATUS_NOTE_OFF              = 0x80 # note (0-127), velocity (0-127)
MIDI_STATUS_NOTE_ON               = 0x90 # note (0-127), velocity (0-127)
MIDI_STATUS_POLYPHONIC_AFTERTOUCH = 0xA0 # note (0-127), pressure (0-127)
MIDI_STATUS_CONTROL_CHANGE        = 0xB0 # see 'Control Change Messages List'
MIDI_STATUS_PROGRAM_CHANGE        = 0xC0 # program (0-127), none
MIDI_STATUS_CHANNEL_PRESSURE      = 0xD0 # pressure (0-127), none
MIDI_STATUS_PITCH_WHEEL_CONTROL   = 0xE0 # LSB (0-127), MSB (0-127)

# MIDI Message type
def MIDI_IS_CHANNEL_MESSAGE(status): return status >= MIDI_STATUS_NOTE_OFF and status <  MIDI_STATUS_BIT
def MIDI_IS_SYSTEM_MESSAGE(status):  return status >= MIDI_STATUS_BIT      and status <= 0xFF
def MIDI_IS_OSC_MESSAGE(status):     return status == '/'                   or status == '#'

# MIDI Channel message type
def MIDI_IS_STATUS_NOTE_OFF(status):              return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_NOTE_OFF
def MIDI_IS_STATUS_NOTE_ON(status):               return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_NOTE_ON
def MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status): return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_POLYPHONIC_AFTERTOUCH
def MIDI_IS_STATUS_CONTROL_CHANGE(status):        return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_CONTROL_CHANGE
def MIDI_IS_STATUS_PROGRAM_CHANGE(status):        return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_PROGRAM_CHANGE
def MIDI_IS_STATUS_CHANNEL_PRESSURE(status):      return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_CHANNEL_PRESSURE
def MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status):   return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_PITCH_WHEEL_CONTROL

# MIDI Utils
def MIDI_GET_STATUS_FROM_DATA(data):  return data[0] & MIDI_STATUS_BIT  if MIDI_IS_CHANNEL_MESSAGE(data[0]) else data[0]
def MIDI_GET_CHANNEL_FROM_DATA(data): return data[0] & MIDI_CHANNEL_BIT if MIDI_IS_CHANNEL_MESSAGE(data[0]) else 0

# ------------------------------------------------------------------------------------------------------------
# Graphics Items

class NoteExpander(QGraphicsRectItem):
    def __init__(self, length, height, parent):
        QGraphicsRectItem.__init__(self, 0, 0, length, height, parent)
        self.parent = parent
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setAcceptHoverEvents(True)

        clearpen = QPen(QColor(0,0,0,0))
        self.setPen(clearpen)

        self.orig_brush = QColor(0, 0, 0, 0)
        self.hover_brush = QColor(200, 200, 200)
        self.stretch = False

    def mousePressEvent(self, event):
        QGraphicsRectItem.mousePressEvent(self, event)
        self.stretch = True

    def hoverEnterEvent(self, event):
        QGraphicsRectItem.hoverEnterEvent(self, event)
        if self.parent.isSelected():
            self.parent.setBrush(self.parent.select_brush)
        else:
            self.parent.setBrush(self.parent.orig_brush)
        self.setBrush(self.hover_brush)

    def hoverLeaveEvent(self, event):
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        if self.parent.isSelected():
            self.parent.setBrush(self.parent.select_brush)
        elif self.parent.hovering:
            self.parent.setBrush(self.parent.hover_brush)
        else:
            self.parent.setBrush(self.parent.orig_brush)
        self.setBrush(self.orig_brush)

class NoteItem(QGraphicsRectItem):
    '''a note on the pianoroll sequencer'''
    def __init__(self, height, length, note_info):
        QGraphicsRectItem.__init__(self, 0, 0, length, height)

        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setAcceptHoverEvents(True)

        clearpen = QPen(QColor(0,0,0,0))
        self.setPen(clearpen)
        self.orig_brush = QColor(note_info[3], 0, 0)
        self.hover_brush = QColor(note_info[3] + 100, 200, 100)
        self.select_brush = QColor(note_info[3] + 100, 100, 100)
        self.setBrush(self.orig_brush)

        self.note = note_info
        self.length = length
        self.piano = self.scene

        self.pressed = False
        self.hovering = False
        self.moving_diff = (0,0)
        self.expand_diff = 0

        l = 5
        self.front = NoteExpander(l, height, self)
        self.back = NoteExpander(l, height, self)
        self.back.setPos(length - l, 0)

    def paint(self, painter, option, widget=None):
        paint_option = option
        paint_option.state &= ~QStyle.State_Selected
        QGraphicsRectItem.paint(self, painter, paint_option, widget)

    def setSelected(self, boolean):
        QGraphicsRectItem.setSelected(self, boolean)
        if boolean: self.setBrush(self.select_brush)
        else: self.setBrush(self.orig_brush)

    def hoverEnterEvent(self, event):
        self.hovering = True
        QGraphicsRectItem.hoverEnterEvent(self, event)
        if not self.isSelected():
            self.setBrush(self.hover_brush)

    def hoverLeaveEvent(self, event):
        self.hovering = False
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        if not self.isSelected():
            self.setBrush(self.orig_brush)
        elif self.isSelected():
            self.setBrush(self.select_brush)

    def mousePressEvent(self, event):
        QGraphicsRectItem.mousePressEvent(self, event)
        self.setSelected(True)
        self.pressed = True

    def mouseMoveEvent(self, event):
        pass

    def moveEvent(self, event):
        offset = event.scenePos() - event.lastScenePos()

        if self.back.stretch:
            self.expand(self.back, offset)
        else:
            self.move_pos = self.scenePos() + offset \
                    + QPointF(self.moving_diff[0],self.moving_diff[1])
            pos = self.piano().enforce_bounds(self.move_pos)
            pos_x, pos_y = pos.x(), pos.y()
            pos_sx, pos_sy = self.piano().snap(pos_x, pos_y)
            self.moving_diff = (pos_x-pos_sx, pos_y-pos_sy)
            if self.front.stretch:
                right = self.rect().right() - offset.x() + self.expand_diff
                if (self.scenePos().x() == self.piano().piano_width and offset.x() < 0) \
                        or right < 10:
                    self.expand_diff = 0
                    return
                self.expand(self.front, offset)
                self.setPos(pos_sx, self.scenePos().y())
            else:
                self.setPos(pos_sx, pos_sy)

    def expand(self, rectItem, offset):
        rect = self.rect()
        right = rect.right() + self.expand_diff
        if rectItem == self.back:
            right += offset.x()
            if right > self.piano().grid_width:
                right = self.piano().grid_width
            elif right < 10:
                right = 10
            new_x = self.piano().snap(right)
        else:
            right -= offset.x()
            new_x = self.piano().snap(right+2.75)
        if self.piano().snap_value: new_x -= 2.75 # where does this number come from?!
        self.expand_diff = right - new_x
        self.back.setPos(new_x - 5, 0)
        rect.setRight(new_x)
        self.setRect(rect)

    def updateNoteInfo(self, pos_x, pos_y):
        self.note[0] = self.piano().get_note_num_from_y(pos_y)
        self.note[1] = self.piano().get_note_start_from_x(pos_x)
        self.note[2] = self.piano().get_note_length_from_x(
                self.rect().right() - self.rect().left())
        #print("note: {}".format(self.note))

    def mouseReleaseEvent(self, event):
        QGraphicsRectItem.mouseReleaseEvent(self, event)
        self.pressed = False
        if event.button() == Qt.LeftButton:
            self.moving_diff = (0,0)
            self.expand_diff = 0
            self.back.stretch = False
            self.front.stretch = False
            (pos_x, pos_y,) = self.piano().snap(self.pos().x(), self.pos().y())
            self.setPos(pos_x, pos_y)
            self.updateNoteInfo(pos_x, pos_y)

    def updateVelocity(self, event):
        offset = event.scenePos().x() - event.lastScenePos().x()
        self.note[3] += int(offset/5)
        if self.note[3] > 127:
            self.note[3] = 127
        elif self.note[3] < 0:
            self.note[3] = 0
        print("new vel: {}".format(self.note[3]))
        self.orig_brush = QColor(self.note[3], 0, 0)
        self.select_brush = QColor(self.note[3] + 100, 100, 100)
        self.setBrush(self.orig_brush)


class PianoKeyItem(QGraphicsRectItem):
    def __init__(self, width, height, parent):
        QGraphicsRectItem.__init__(self, 0, 0, width, height, parent)
        self.setPen(QPen(QColor(0,0,0,80)))
        self.width = width
        self.height = height
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setAcceptHoverEvents(True)
        self.hover_brush = QColor(200, 0, 0)
        self.click_brush = QColor(255, 100, 100)
        self.pressed = False

    def hoverEnterEvent(self, event):
        QGraphicsRectItem.hoverEnterEvent(self, event)
        self.orig_brush = self.brush()
        self.setBrush(self.hover_brush)

    def hoverLeaveEvent(self, event):
        if self.pressed:
            self.pressed = False
            self.setBrush(self.hover_brush)
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        self.setBrush(self.orig_brush)

    #def mousePressEvent(self, event):
    #    self.pressed = True
    #    self.setBrush(self.click_brush)

    def mouseMoveEvent(self, event):
        """this may eventually do something"""
        pass

    def mouseReleaseEvent(self, event):
        self.pressed = False
        QGraphicsRectItem.mouseReleaseEvent(self, event)
        self.setBrush(self.hover_brush)

class PianoRoll(QGraphicsScene):
    '''the piano roll'''

    midievent = pyqtSignal(list)
    measureupdate = pyqtSignal(int)
    modeupdate = pyqtSignal(str)

    def __init__(self, time_sig = '4/4', num_measures = 4, quantize_val = '1/8'):
        QGraphicsScene.__init__(self)
        self.setBackgroundBrush(QColor(50, 50, 50))
        self.mousePos = QPointF()

        self.notes = []
        self.selected_notes = []
        self.piano_keys = []

        self.marquee_select = False
        self.insert_mode = False
        self.velocity_mode = False
        self.place_ghost = False
        self.ghost_note = None
        self.default_ghost_vel = 100
        self.ghost_vel = self.default_ghost_vel

        ## dimensions
        self.padding = 2

        ## piano dimensions
        self.note_height = 10
        self.start_octave = -2
        self.end_octave = 8
        self.notes_in_octave = 12
        self.total_notes = (self.end_octave - self.start_octave) \
                * self.notes_in_octave + 1
        self.piano_height = self.note_height * self.total_notes
        self.octave_height = self.notes_in_octave * self.note_height

        self.piano_width = 34

        ## height
        self.header_height = 20
        self.total_height = self.piano_height - self.note_height + self.header_height
        #not sure why note_height is subtracted

        ## width
        self.full_note_width = 250 # i.e. a 4/4 note
        self.snap_value = None
        self.quantize_val = quantize_val

        ### dummy vars that will be changed
        self.time_sig = 0
        self.measure_width = 0
        self.num_measures = 0
        self.max_note_length = 0
        self.grid_width = 0
        self.value_width = 0
        self.grid_div = 0
        self.piano = None
        self.header = None
        self.play_head = None

        self.setTimeSig(time_sig)
        self.setMeasures(num_measures)
        self.setGridDiv()
        self.default_length = 1. / self.grid_div


    # -------------------------------------------------------------------------
    # Callbacks

    def movePlayHead(self, transport_info):
        # TODO: need conversion between frames and PPQ
        x = 105. # works for 120bpm
        total_duration = self.time_sig[0] * self.num_measures * x
        pos = transport_info['frame'] / x
        frac = (pos % total_duration) / total_duration
        self.play_head.setPos(QPointF(frac * self.grid_width, 0))

    def setTimeSig(self, time_sig):
        try:
           new_time_sig = list(map(float, time_sig.split('/')))
           if len(new_time_sig)==2:
               self.time_sig = new_time_sig

               self.measure_width = self.full_note_width * self.time_sig[0]/self.time_sig[1]
               self.max_note_length = self.num_measures * self.time_sig[0]/self.time_sig[1]
               self.grid_width = self.measure_width * self.num_measures
               self.setGridDiv()
        except ValueError:
            pass

    def setMeasures(self, measures):
        try:
            self.num_measures = float(measures)
            self.max_note_length = self.num_measures * self.time_sig[0]/self.time_sig[1]
            self.grid_width = self.measure_width * self.num_measures
            self.refreshScene()
        except:
            pass

    def setDefaultLength(self, length):
        try:
            v = list(map(float, length.split('/')))
            if len(v) < 3:
                self.default_length = \
                        v[0] if len(v)==1 else \
                        v[0] / v[1]
                pos = self.enforce_bounds(self.mousePos)
                if self.insert_mode: self.makeGhostNote(pos.x(), pos.y())
        except ValueError:
            pass

    def setGridDiv(self, div=None):
        if not div: div = self.quantize_val
        try:
            val = list(map(int, div.split('/')))
            if len(val) < 3:
                self.quantize_val = div
                self.grid_div = val[0] if len(val)==1 else val[1]
                self.value_width = self.full_note_width / float(self.grid_div) if self.grid_div else None
                self.setQuantize(div)

                self.refreshScene()
        except ValueError:
            pass

    def setQuantize(self, value):
        try:
            val = list(map(float, value.split('/')))
            if len(val) == 1:
                self.quantize(val[0])
                self.quantize_val = value
            elif len(val) == 2:
                self.quantize(val[0] / val[1])
                self.quantize_val = value
        except ValueError:
            pass

    # -------------------------------------------------------------------------
    # Event Callbacks

    def keyPressEvent(self, event):
        QGraphicsScene.keyPressEvent(self, event)
        if event.key() == Qt.Key_F:
            if not self.insert_mode:
                self.velocity_mode = False
                self.insert_mode = True
                self.makeGhostNote(self.mousePos.x(), self.mousePos.y())
                self.modeupdate.emit('insert_mode')
            elif self.insert_mode:
                self.insert_mode = False
                if self.place_ghost: self.place_ghost = False
                self.removeItem(self.ghost_note)
                self.ghost_note = None
                self.modeupdate.emit('')
        elif event.key() == Qt.Key_D:
            if self.velocity_mode:
                self.velocity_mode = False
                self.modeupdate.emit('')
            else:
                if self.insert_mode:
                    self.removeItem(self.ghost_note)
                self.ghost_note = None
                self.insert_mode = False
                self.place_ghost = False
                self.velocity_mode = True
                self.modeupdate.emit('velocity_mode')
        elif event.key() == Qt.Key_A:
            if all((note.isSelected() for note in self.notes)):
                for note in self.notes:
                    note.setSelected(False)
                self.selected_notes = []
            else:
                for note in self.notes:
                    note.setSelected(True)
                self.selected_notes = self.notes[:]
        elif event.key() in (Qt.Key_Delete, Qt.Key_Backspace):
            self.notes = [note for note in self.notes if note not in self.selected_notes]
            for note in self.selected_notes:
                self.removeItem(note)
                self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
                del note
            self.selected_notes = []

    def mousePressEvent(self, event):
        QGraphicsScene.mousePressEvent(self, event)
        if not (any(key.pressed for key in self.piano_keys)
                or any(note.pressed for note in self.notes)):
            for note in self.selected_notes:
                note.setSelected(False)
            self.selected_notes = []

            if event.button() == Qt.LeftButton:
                if self.insert_mode:
                    self.place_ghost = True
                else:
                    self.marquee_select = True
                    self.marquee_rect = QRectF(event.scenePos().x(), event.scenePos().y(), 1, 1)
                    self.marquee = QGraphicsRectItem(self.marquee_rect)
                    self.marquee.setBrush(QColor(255, 255, 255, 100))
                    self.addItem(self.marquee)
        else:
            for s_note in self.notes:
                if s_note.pressed and s_note in self.selected_notes:
                    break
                elif s_note.pressed and s_note not in self.selected_notes:
                    for note in self.selected_notes:
                        note.setSelected(False)
                    self.selected_notes = [s_note]
                    break
            for note in self.selected_notes:
                if not self.velocity_mode:
                    note.mousePressEvent(event)

    def mouseMoveEvent(self, event):
        QGraphicsScene.mouseMoveEvent(self, event)
        self.mousePos = event.scenePos()
        if not (any((key.pressed for key in self.piano_keys))):
            m_pos = event.scenePos()
            if self.insert_mode and self.place_ghost: #placing a note
                m_width = self.ghost_rect.x() + self.ghost_rect_orig_width
                if m_pos.x() > m_width:
                    m_new_x = self.snap(m_pos.x())
                    self.ghost_rect.setRight(m_new_x)
                    self.ghost_note.setRect(self.ghost_rect)
                #self.adjust_note_vel(event)
            else:
                m_pos = self.enforce_bounds(m_pos)

                if self.insert_mode: #ghostnote follows mouse around
                    (m_new_x, m_new_y) = self.snap(m_pos.x(), m_pos.y())
                    self.ghost_rect.moveTo(m_new_x, m_new_y)
                    try:
                        self.ghost_note.setRect(self.ghost_rect)
                    except RuntimeError:
                        self.ghost_note = None
                        self.makeGhostNote(m_new_x, m_new_y)

                elif self.marquee_select:
                    marquee_orig_pos = event.buttonDownScenePos(Qt.LeftButton)
                    if marquee_orig_pos.x() < m_pos.x() and marquee_orig_pos.y() < m_pos.y():
                        self.marquee_rect.setBottomRight(m_pos)
                    elif marquee_orig_pos.x() < m_pos.x() and marquee_orig_pos.y() > m_pos.y():
                        self.marquee_rect.setTopRight(m_pos)
                    elif marquee_orig_pos.x() > m_pos.x() and marquee_orig_pos.y() < m_pos.y():
                        self.marquee_rect.setBottomLeft(m_pos)
                    elif marquee_orig_pos.x() > m_pos.x() and marquee_orig_pos.y() > m_pos.y():
                        self.marquee_rect.setTopLeft(m_pos)
                    self.marquee.setRect(self.marquee_rect)
                    self.selected_notes = []
                    for item in self.collidingItems(self.marquee):
                        if item in self.notes:
                            self.selected_notes.append(item)

                    for note in self.notes:
                        if note in self.selected_notes: note.setSelected(True)
                        else: note.setSelected(False)

                elif self.velocity_mode:
                    if Qt.LeftButton == event.buttons():
                        for note in self.selected_notes:
                            note.updateVelocity(event)

                elif not self.marquee_select: #move selected
                    if Qt.LeftButton == event.buttons():
                        x = y = False
                        if any(note.back.stretch for note in self.selected_notes):
                            x = True
                        elif any(note.front.stretch for note in self.selected_notes):
                            y = True
                        for note in self.selected_notes:
                            note.back.stretch = x
                            note.front.stretch = y
                            note.moveEvent(event)

    def mouseReleaseEvent(self, event):
        if not (any((key.pressed for key in self.piano_keys)) or any((note.pressed for note in self.notes))):
            if event.button() == Qt.LeftButton:
                if self.place_ghost and self.insert_mode:
                    self.place_ghost = False
                    note_start = self.get_note_start_from_x(self.ghost_rect.x())
                    note_num = self.get_note_num_from_y(self.ghost_rect.y())
                    note_length = self.get_note_length_from_x(self.ghost_rect.width())
                    self.drawNote(note_num, note_start, note_length, self.ghost_vel)
                    self.midievent.emit(["midievent-add", note_num, note_start, note_length, self.ghost_vel])
                    self.makeGhostNote(self.mousePos.x(), self.mousePos.y())
                elif self.marquee_select:
                    self.marquee_select = False
                    self.removeItem(self.marquee)
        elif not self.marquee_select:
            for note in self.selected_notes:
                old_info = note.note[:]
                note.mouseReleaseEvent(event)
                if self.velocity_mode:
                    note.setSelected(True)
                if not old_info == note.note:
                    self.midievent.emit(["midievent-remove", old_info[0], old_info[1], old_info[2], old_info[3]])
                    self.midievent.emit(["midievent-add", note.note[0], note.note[1], note.note[2], note.note[3]])

    # -------------------------------------------------------------------------
    # Internal Functions

    def drawHeader(self):
        self.header = QGraphicsRectItem(0, 0, self.grid_width, self.header_height)
        #self.header.setZValue(1.0)
        self.header.setPos(self.piano_width, 0)
        self.addItem(self.header)

    def drawPiano(self):
        piano_keys_width = self.piano_width - self.padding
        labels = ('B','Bb','A','Ab','G','Gb','F','E','Eb','D','Db','C')
        black_notes = (2,4,6,9,11)
        piano_label = QFont()
        piano_label.setPointSize(6)
        self.piano = QGraphicsRectItem(0, 0, piano_keys_width, self.piano_height)
        self.piano.setPos(0, self.header_height)
        self.addItem(self.piano)

        key = PianoKeyItem(piano_keys_width, self.note_height, self.piano)
        label = QGraphicsSimpleTextItem('C8', key)
        label.setPos(18, 1)
        label.setFont(piano_label)
        key.setBrush(QColor(255, 255, 255))
        for i in range(self.end_octave - self.start_octave, self.start_octave - self.start_octave, -1):
            for j in range(self.notes_in_octave, 0, -1):
                if j in black_notes:
                    key = PianoKeyItem(piano_keys_width/1.4, self.note_height, self.piano)
                    key.setBrush(QColor(0, 0, 0))
                    key.setZValue(1.0)
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1))
                elif (j - 1) and (j + 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 2, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1) - self.note_height/2.)
                elif (j - 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 3./2, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1) - self.note_height/2.)
                elif (j + 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 3./2, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1))
                if j == 12:
                    label = QGraphicsSimpleTextItem('{}{}'.format(labels[j - 1], self.end_octave - i), key )
                    label.setPos(18, 6)
                    label.setFont(piano_label)
                self.piano_keys.append(key)

    def drawGrid(self):
        black_notes = [2,4,6,9,11]
        scale_bar = QGraphicsRectItem(0, 0, self.grid_width, self.note_height, self.piano)
        scale_bar.setPos(self.piano_width, 0)
        scale_bar.setBrush(QColor(100,100,100))
        clearpen = QPen(QColor(0,0,0,0))
        for i in range(self.end_octave - self.start_octave, self.start_octave - self.start_octave, -1):
            for j in range(self.notes_in_octave, 0, -1):
                scale_bar = QGraphicsRectItem(0, 0, self.grid_width, self.note_height, self.piano)
                scale_bar.setPos(self.piano_width, self.note_height * j + self.octave_height * (i - 1))
                scale_bar.setPen(clearpen)
                if j not in black_notes:
                    scale_bar.setBrush(QColor(120,120,120))
                else:
                    scale_bar.setBrush(QColor(100,100,100))

        measure_pen = QPen(QColor(0, 0, 0, 120), 3)
        half_measure_pen = QPen(QColor(0, 0, 0, 40), 2)
        line_pen = QPen(QColor(0, 0, 0, 40))
        for i in range(0, int(self.num_measures) + 1):
            measure = QGraphicsLineItem(0, 0, 0, self.piano_height + self.header_height - measure_pen.width(), self.header)
            measure.setPos(self.measure_width * i, 0.5 * measure_pen.width())
            measure.setPen(measure_pen)
            if i < self.num_measures:
                number = QGraphicsSimpleTextItem('%d' % (i + 1), self.header)
                number.setPos(self.measure_width * i + 5, 2)
                number.setBrush(Qt.white)
                for j in self.frange(0, self.time_sig[0]*self.grid_div/self.time_sig[1], 1.):
                    line = QGraphicsLineItem(0, 0, 0, self.piano_height, self.header)
                    line.setZValue(1.0)
                    line.setPos(self.measure_width * i + self.value_width * j, self.header_height)
                    if j == self.time_sig[0]*self.grid_div/self.time_sig[1] / 2.0:
                        line.setPen(half_measure_pen)
                    else:
                        line.setPen(line_pen)

    def drawPlayHead(self):
        self.play_head = QGraphicsLineItem(self.piano_width, self.header_height, self.piano_width, self.total_height)
        self.play_head.setPen(QPen(QColor(255,255,255,50), 2))
        self.play_head.setZValue(1.)
        self.addItem(self.play_head)

    def refreshScene(self):
        list(map(self.removeItem, self.notes))
        self.selected_notes = []
        self.piano_keys = []
        self.clear()
        self.drawPiano()
        self.drawHeader()
        self.drawGrid()
        self.drawPlayHead()
        for note in self.notes[:]:
            if note.note[1] >= (self.num_measures * self.time_sig[0]):
                self.notes.remove(note)
                self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
            elif note.note[2] > self.max_note_length:
                new_note = note.note[:]
                new_note[2] = self.max_note_length
                self.notes.remove(note)
                self.drawNote(new_note[0], new_note[1], self.max_note_length, new_note[3], False)
                self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
                self.midievent.emit(["midievent-add", new_note[0], new_note[1], new_note[2], new_note[3]])
        list(map(self.addItem, self.notes))
        if self.views():
            self.views()[0].setSceneRect(self.itemsBoundingRect())

    def clearNotes(self):
        self.clear()
        self.notes = []
        self.selected_notes = []
        self.drawPiano()
        self.drawHeader()
        self.drawGrid()

    def makeGhostNote(self, pos_x, pos_y):
        """creates the ghostnote that is placed on the scene before the real one is."""
        if self.ghost_note:
            self.removeItem(self.ghost_note)
        length = self.full_note_width * self.default_length
        (start, note) = self.snap(pos_x, pos_y)
        self.ghost_vel = self.default_ghost_vel
        self.ghost_rect = QRectF(start, note, length, self.note_height)
        self.ghost_rect_orig_width = self.ghost_rect.width()
        self.ghost_note = QGraphicsRectItem(self.ghost_rect)
        self.ghost_note.setBrush(QColor(230, 221, 45, 100))
        self.addItem(self.ghost_note)

    def drawNote(self, note_num, note_start=None, note_length=None, note_velocity=None, add=True):
        """
        note_num: midi number, 0 - 127
        note_start: 0 - (num_measures * time_sig[0]) so this is in beats
        note_length: 0 - (num_measures  * time_sig[0]/time_sig[1]) this is in measures
        note_velocity: 0 - 127
        """

        info = [note_num, note_start, note_length, note_velocity]

        if not note_start % (self.num_measures * self.time_sig[0]) == note_start:
            #self.midievent.emit(["midievent-remove", note_num, note_start, note_length, note_velocity])
            while not note_start % (self.num_measures * self.time_sig[0]) == note_start:
                self.setMeasures(self.num_measures+1)
            self.measureupdate.emit(self.num_measures)
            self.refreshScene()

        x_start = self.get_note_x_start(note_start)
        if note_length > self.max_note_length:
            note_length = self.max_note_length + 0.25
        x_length = self.get_note_x_length(note_length)
        y_pos = self.get_note_y_pos(note_num)

        note = NoteItem(self.note_height, x_length, info)
        note.setPos(x_start, y_pos)

        self.notes.append(note)
        if add:
            self.addItem(note)

    # -------------------------------------------------------------------------
    # Helper Functions

    def frange(self, x, y, t):
        while x < y:
            yield x
            x += t

    def quantize(self, value):
        self.snap_value = float(self.full_note_width) * value if value else None

    def snap(self, pos_x, pos_y = None):
        if self.snap_value:
            pos_x = int(round((pos_x - self.piano_width) / self.snap_value)) \
                    * self.snap_value + self.piano_width
        if pos_y:
            pos_y = int((pos_y - self.header_height) / self.note_height) \
                    * self.note_height + self.header_height
        return (pos_x, pos_y) if pos_y else pos_x

    def adjust_note_vel(self, event):
        m_pos = event.scenePos()
        #bind velocity to vertical mouse movement
        self.ghost_vel += (event.lastScenePos().y() - m_pos.y())/10
        if self.ghost_vel < 0:
            self.ghost_vel = 0
        elif self.ghost_vel > 127:
            self.ghost_vel = 127

        m_width = self.ghost_rect.x() + self.ghost_rect_orig_width
        if m_pos.x() < m_width:
            m_pos.setX(m_width)
        m_new_x = self.snap(m_pos.x())
        self.ghost_rect.setRight(m_new_x)
        self.ghost_note.setRect(self.ghost_rect)


    def enforce_bounds(self, pos):
        if pos.x() < self.piano_width:
            pos.setX(self.piano_width)
        elif pos.x() > self.grid_width + self.piano_width:
            pos.setX(self.grid_width + self.piano_width)
        if pos.y() < self.header_height + self.padding:
            pos.setY(self.header_height + self.padding)
        return pos

    def get_note_start_from_x(self, note_x):
        return (note_x - self.piano_width) / (self.grid_width / self.num_measures / self.time_sig[0])


    def get_note_x_start(self, note_start):
        return self.piano_width + \
                (self.grid_width / self.num_measures / self.time_sig[0]) * note_start

    def get_note_x_length(self, note_length):
        return float(self.time_sig[1]) / self.time_sig[0] * note_length * self.grid_width / self.num_measures

    def get_note_length_from_x(self, note_x):
        return float(self.time_sig[0]) / self.time_sig[1] * self.num_measures / self.grid_width \
                * note_x


    def get_note_y_pos(self, note_num):
        return self.header_height + self.note_height * (self.total_notes - note_num - 1)

    def get_note_num_from_y(self, note_y_pos):
        return -(((note_y_pos - self.header_height) / self.note_height) - self.total_notes + 1)

class PianoRollView(QGraphicsView):
    def __init__(self, parent, time_sig = '4/4', num_measures = 4, quantize_val = '1/8'):
        QGraphicsView.__init__(self, parent)
        self.piano = PianoRoll(time_sig, num_measures, quantize_val)
        self.setScene(self.piano)
        #self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        x = 0   * self.sceneRect().width() + self.sceneRect().left()
        y = 0.4 * self.sceneRect().height() + self.sceneRect().top()
        self.centerOn(x, y)

        self.setAlignment(Qt.AlignLeft)
        self.o_transform = self.transform()
        self.zoom_x = 1
        self.zoom_y = 1

    def setZoomX(self, scale_x):
        self.setTransform(self.o_transform)
        self.zoom_x = 1 + scale_x / float(99) * 2
        self.scale(self.zoom_x, self.zoom_y)

    def setZoomY(self, scale_y):
        self.setTransform(self.o_transform)
        self.zoom_y = 1 + scale_y / float(99)
        self.scale(self.zoom_x, self.zoom_y)

# ------------------------------------------------------------------------------------------------------------

class ModeIndicator(QWidget):
    def __init__(self, parent):
        QWidget.__init__(self, parent)
        #self.setGeometry(0, 0, 30, 20)
        self.setFixedSize(30,20)
        self.mode = None

    def paintEvent(self, event):
        painter = QPainter(self)
        event.accept()

        painter.begin(self)

        painter.setPen(QPen(QColor(0, 0, 0, 0)))
        if self.mode == 'velocity_mode':
            painter.setBrush(QColor(127, 0, 0))
        elif self.mode == 'insert_mode':
            painter.setBrush(QColor(0, 100, 127))
        else:
            painter.setBrush(QColor(0, 0, 0, 0))
        painter.drawRect(0, 0, 30, 20)

        # FIXME
        painter.end()

    def changeMode(self, new_mode):
        self.mode = new_mode
        self.update()

# ------------------------------------------------------------------------------------------------------------
