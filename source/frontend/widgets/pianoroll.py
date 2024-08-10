#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-FileCopyrightText: 2014-2015 Perry Nguyen
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt, QRectF, QPointF, pyqtSignal
    from PyQt5.QtGui import QColor, QCursor, QFont, QPen, QPainter
    from PyQt5.QtWidgets import (
        QApplication,
        QGraphicsItem,
        QGraphicsLineItem,
        QGraphicsRectItem,
        QGraphicsSimpleTextItem,
        QGraphicsScene,
        QGraphicsView,
        QStyle,
        QWidget,
    )
elif qt_config == 6:
    from PyQt6.QtCore import Qt, QRectF, QPointF, pyqtSignal
    from PyQt6.QtGui import QColor, QCursor, QFont, QPen, QPainter
    from PyQt6.QtWidgets import (
        QApplication,
        QGraphicsItem,
        QGraphicsLineItem,
        QGraphicsRectItem,
        QGraphicsSimpleTextItem,
        QGraphicsScene,
        QGraphicsView,
        QStyle,
        QWidget,
    )

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

#from carla_shared import *

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
def MIDI_IS_STATUS_NOTE_OFF(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_NOTE_OFF
def MIDI_IS_STATUS_NOTE_ON(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_NOTE_ON
def MIDI_IS_STATUS_POLYPHONIC_AFTERTOUCH(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_POLYPHONIC_AFTERTOUCH
def MIDI_IS_STATUS_CONTROL_CHANGE(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_CONTROL_CHANGE
def MIDI_IS_STATUS_PROGRAM_CHANGE(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_PROGRAM_CHANGE
def MIDI_IS_STATUS_CHANNEL_PRESSURE(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_CHANNEL_PRESSURE
def MIDI_IS_STATUS_PITCH_WHEEL_CONTROL(status):
    return MIDI_IS_CHANNEL_MESSAGE(status) and (status & MIDI_STATUS_BIT) == MIDI_STATUS_PITCH_WHEEL_CONTROL

# MIDI Utils
def MIDI_GET_STATUS_FROM_DATA(data):
    return data[0] & MIDI_STATUS_BIT  if MIDI_IS_CHANNEL_MESSAGE(data[0]) else data[0]
def MIDI_GET_CHANNEL_FROM_DATA(data):
    return data[0] & MIDI_CHANNEL_BIT if MIDI_IS_CHANNEL_MESSAGE(data[0]) else 0

# ---------------------------------------------------------------------------------------------------------------------
# Graphics Items

class NoteExpander(QGraphicsRectItem):
    def __init__(self, length, height, parent):
        QGraphicsRectItem.__init__(self, 0, 0, length, height, parent)
        self.parent = parent
        self.orig_brush = QColor(0, 0, 0, 0)
        self.hover_brush = QColor(200, 200, 200)
        self.stretch = False

        self.setAcceptHoverEvents(True)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setPen(QPen(QColor(0,0,0,0)))

    def paint(self, painter, option, widget=None):
        paint_option = option
        paint_option.state &= ~QStyle.State_Selected
        QGraphicsRectItem.paint(self, painter, paint_option, widget)

    def mousePressEvent(self, event):
        QGraphicsRectItem.mousePressEvent(self, event)
        self.stretch = True

    def mouseReleaseEvent(self, event):
        QGraphicsRectItem.mouseReleaseEvent(self, event)
        self.stretch = False

    def hoverEnterEvent(self, event):
        QGraphicsRectItem.hoverEnterEvent(self, event)
        self.setCursor(QCursor(Qt.SizeHorCursor))
        self.setBrush(self.hover_brush)

    def hoverLeaveEvent(self, event):
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        self.unsetCursor()
        self.setBrush(self.orig_brush)

# ---------------------------------------------------------------------------------------------------------------------

class NoteItem(QGraphicsRectItem):
    '''a note on the pianoroll sequencer'''
    def __init__(self, height, length, note_info):
        QGraphicsRectItem.__init__(self, 0, 0, length, height)

        self.orig_brush = QColor(note_info[3], 0, 0)
        self.hover_brush = QColor(note_info[3] + 98, 200, 100)
        self.select_brush = QColor(note_info[3] + 98, 100, 100)

        self.note = note_info
        self.length = length
        self.piano = self.scene

        self.pressed = False
        self.hovering = False
        self.moving_diff = (0,0)
        self.expand_diff = 0

        self.setAcceptHoverEvents(True)
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setPen(QPen(QColor(0,0,0,0)))
        self.setBrush(self.orig_brush)

        l = 5
        self.front = NoteExpander(l, height, self)
        self.back = NoteExpander(l, height, self)
        self.back.setPos(length - l, 0)

    def paint(self, painter, option, widget=None):
        paint_option = option
        paint_option.state &= ~QStyle.State_Selected
        if self.isSelected():
            self.setBrush(self.select_brush)
        elif self.hovering:
            self.setBrush(self.hover_brush)
        else:
            self.setBrush(self.orig_brush)
        QGraphicsRectItem.paint(self, painter, paint_option, widget)

    def hoverEnterEvent(self, event):
        QGraphicsRectItem.hoverEnterEvent(self, event)
        self.hovering = True
        self.update()
        self.setCursor(QCursor(Qt.OpenHandCursor))

    def hoverLeaveEvent(self, event):
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        self.hovering = False
        self.unsetCursor()
        self.update()

    def mousePressEvent(self, event):
        QGraphicsRectItem.mousePressEvent(self, event)
        self.pressed = True
        self.moving_diff = (0,0)
        self.expand_diff = 0
        self.setCursor(QCursor(Qt.ClosedHandCursor))
        self.setSelected(True)

    def mouseMoveEvent(self, event):
        event.ignore()

    def mouseReleaseEvent(self, event):
        QGraphicsRectItem.mouseReleaseEvent(self, event)
        self.pressed = False
        self.moving_diff = (0,0)
        self.expand_diff = 0
        self.setCursor(QCursor(Qt.OpenHandCursor))

    def moveEvent(self, event):
        offset = event.scenePos() - event.lastScenePos()

        if self.back.stretch:
            self.expand(self.back, offset)
            self.updateNoteInfo(self.scenePos().x(), self.scenePos().y())
            return

        if self.front.stretch:
            self.expand(self.front, offset)
            self.updateNoteInfo(self.scenePos().x(), self.scenePos().y())
            return

        piano = self.piano()

        pos = self.scenePos() + offset + QPointF(self.moving_diff[0],self.moving_diff[1])
        pos = piano.enforce_bounds(pos)
        pos_x = pos.x()
        pos_y = pos.y()
        width = self.rect().width()
        if pos_x + width > piano.grid_width + piano.piano_width:
            pos_x = piano.grid_width + piano.piano_width - width
        pos_sx, pos_sy = piano.snap(pos_x, pos_y)

        if pos_sx + width > piano.grid_width + piano.piano_width:
            self.moving_diff = (0,0)
            self.expand_diff = 0
            return

        self.moving_diff = (pos_x-pos_sx, pos_y-pos_sy)
        self.setPos(pos_sx, pos_sy)

        self.updateNoteInfo(pos_sx, pos_sy)

    def expand(self, rectItem, offset):
        rect = self.rect()
        piano = self.piano()
        width = rect.right() + self.expand_diff

        if rectItem == self.back:
            width += offset.x()
            max_x = piano.grid_width + piano.piano_width
            if width + self.scenePos().x() >= max_x:
                width = max_x - self.scenePos().x() - 1
            elif piano.snap_value and width < piano.snap_value:
                width = piano.snap_value
            elif width < 10:
                width = 10
            new_w = piano.snap(width) - 2.75
            if new_w + self.scenePos().x() >= max_x:
                self.moving_diff = (0,0)
                self.expand_diff = 0
                return

        else:
            width -= offset.x()
            new_w = piano.snap(width+2.75) - 2.75
            if new_w <= 0:
                new_w = piano.snap_value
                self.moving_diff = (0,0)
                self.expand_diff = 0
                return
            diff = rect.right() - new_w
            if diff: # >= piano.snap_value:
                new_x = self.scenePos().x() + diff
                if new_x < piano.piano_width:
                    new_x = piano.piano_width
                    self.moving_diff = (0,0)
                    self.expand_diff = 0
                    return
                print(new_x, new_w, diff)
                self.setX(new_x)

        self.expand_diff = width - new_w
        self.back.setPos(new_w - 5, 0)
        rect.setRight(new_w)
        self.setRect(rect)

    def updateNoteInfo(self, pos_x, pos_y):
        note_info = (self.piano().get_note_num_from_y(pos_y),
                     self.piano().get_note_start_from_x(pos_x),
                     self.piano().get_note_length_from_x(self.rect().width()),
                     self.note[3])
        if self.note != note_info:
            self.piano().move_note(self.note, note_info)
            self.note = note_info

    def updateVelocity(self, event):
        offset = event.scenePos().x() - event.lastScenePos().x()
        offset = int(offset/5)

        note_info = self.note[:]
        note_info[3] += offset
        if note_info[3] > 127:
            note_info[3] = 127
        elif note_info[3] < 0:
            note_info[3] = 0
        if self.note != note_info:
            self.orig_brush = QColor(note_info[3], 0, 0)
            self.hover_brush = QColor(note_info[3] + 98, 200, 100)
            self.select_brush = QColor(note_info[3] + 98, 100, 100)
            self.update()
            self.piano().move_note(self.note, note_info)
            self.note = note_info

# ---------------------------------------------------------------------------------------------------------------------

class PianoKeyItem(QGraphicsRectItem):
    def __init__(self, width, height, note, parent):
        QGraphicsRectItem.__init__(self, 0, 0, width, height, parent)

        self.width = width
        self.height = height
        self.note = note
        self.piano = self.scene
        self.hovered = False
        self.pressed = False

        self.click_brush = QColor(255, 100, 100)
        self.hover_brush = QColor(200, 0, 0)
        self.orig_brush = None

        self.setAcceptHoverEvents(True)
        self.setFlag(QGraphicsItem.ItemIsSelectable)
        self.setPen(QPen(QColor(0,0,0,80)))

    def paint(self, painter, option, widget=None):
        paint_option = option
        paint_option.state &= ~QStyle.State_Selected
        QGraphicsRectItem.paint(self, painter, paint_option, widget)

    def hoverEnterEvent(self, event):
        QGraphicsRectItem.hoverEnterEvent(self, event)
        self.hovered = True
        self.orig_brush = self.brush()
        self.setBrush(self.hover_brush)

    def hoverLeaveEvent(self, event):
        QGraphicsRectItem.hoverLeaveEvent(self, event)
        self.hovered = False
        self.setBrush(self.click_brush if self.pressed else self.orig_brush)

    def mousePressEvent(self, event):
        QGraphicsRectItem.mousePressEvent(self, event)
        self.pressed = True
        self.setBrush(self.click_brush)
        self.piano().noteclicked.emit(self.note, True)

    def mouseReleaseEvent(self, event):
        QGraphicsRectItem.mouseReleaseEvent(self, event)
        self.pressed = False
        self.setBrush(self.hover_brush if self.hovered else self.orig_brush)
        self.piano().noteclicked.emit(self.note, False)

# ---------------------------------------------------------------------------------------------------------------------

class PianoRoll(QGraphicsScene):
    '''the piano roll'''

    noteclicked = pyqtSignal(int,bool)
    midievent = pyqtSignal(list)
    measureupdate = pyqtSignal(int)
    modeupdate = pyqtSignal(str)

    default_ghost_vel = 100

    def __init__(self, time_sig = '4/4', num_measures = 4, quantize_val = '1/8'):
        QGraphicsScene.__init__(self)
        self.setBackgroundBrush(QColor(50, 50, 50))

        self.notes = []
        self.removed_notes = []
        self.selected_notes = []
        self.piano_keys = []

        self.marquee_select = False
        self.marquee_rect = None
        self.marquee = None

        self.ghost_note = None
        self.ghost_rect = None
        self.ghost_rect_orig_width = None
        self.ghost_vel = self.default_ghost_vel

        self.ignore_mouse_events = False
        self.insert_mode = False
        self.velocity_mode = False
        self.place_ghost = False
        self.last_mouse_pos = QPointF()

        ## dimensions
        self.padding = 2

        ## piano dimensions
        self.note_height = 10
        self.start_octave = -2
        self.end_octave = 8
        self.notes_in_octave = 12
        self.total_notes = (self.end_octave - self.start_octave) * self.notes_in_octave + 1
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
        self.time_sig = (0,0)
        self.measure_width = 0
        self.num_measures = 0
        self.max_note_length = 0
        self.grid_width = 0
        self.value_width = 0
        self.grid_div = 0
        self.piano = None
        self.header = None
        self.play_head = None

        self.setGridDiv()
        self.default_length = 1. / self.grid_div


    # -------------------------------------------------------------------------
    # Callbacks

    def movePlayHead(self, transportInfo):
        ticksPerBeat = transportInfo['ticksPerBeat']
        max_ticks = ticksPerBeat * self.time_sig[0] * self.num_measures
        cur_tick = ticksPerBeat * self.time_sig[0] * transportInfo['bar'] + ticksPerBeat * transportInfo['beat'] + transportInfo['tick']
        frac = (cur_tick % max_ticks) / max_ticks
        self.play_head.setPos(QPointF(frac * self.grid_width, 0))

    def setTimeSig(self, time_sig):
        self.time_sig = time_sig
        self.measure_width = self.full_note_width * self.time_sig[0]/self.time_sig[1]
        self.max_note_length = self.num_measures * self.time_sig[0]/self.time_sig[1]
        self.grid_width = self.measure_width * self.num_measures
        self.setGridDiv()

    def setMeasures(self, measures):
        #try:
        self.num_measures = float(measures)
        self.max_note_length = self.num_measures * self.time_sig[0]/self.time_sig[1]
        self.grid_width = self.measure_width * self.num_measures
        self.refreshScene()
        #except:
            #pass

    def setDefaultLength(self, length):
        v = list(map(float, length.split('/')))
        if len(v) < 3:
            self.default_length = v[0] if len(v) == 1 else v[0] / v[1]
            pos = self.enforce_bounds(self.last_mouse_pos)
            if self.insert_mode:
                self.makeGhostNote(pos.x(), pos.y())

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
        val = list(map(float, value.split('/')))
        if len(val) == 1:
            self.quantize(val[0])
            self.quantize_val = value
        elif len(val) == 2:
            self.quantize(val[0] / val[1])
            self.quantize_val = value

    # -------------------------------------------------------------------------
    # Event Callbacks

    def keyPressEvent(self, event):
        QGraphicsScene.keyPressEvent(self, event)

        if event.key() == Qt.Key_Escape:
            QApplication.instance().closeAllWindows()
            return

        if event.key() == Qt.Key_F:
            if not self.insert_mode:
                # turn off velocity mode
                self.velocity_mode = False
                # enable insert mode
                self.insert_mode = True
                self.place_ghost = False
                self.makeGhostNote(self.last_mouse_pos.x(), self.last_mouse_pos.y())
                self.modeupdate.emit('insert_mode')
            else:
                # turn off insert mode
                self.insert_mode = False
                self.place_ghost = False
                if self.ghost_note is not None:
                    self.removeItem(self.ghost_note)
                    self.ghost_note = None
                self.modeupdate.emit('')

        elif event.key() == Qt.Key_D:
            if not self.velocity_mode:
                # turn off insert mode
                self.insert_mode = False
                self.place_ghost = False
                if self.ghost_note is not None:
                    self.removeItem(self.ghost_note)
                    self.ghost_note = None
                # enable velocity mode
                self.velocity_mode = True
                self.modeupdate.emit('velocity_mode')
            else:
                # turn off velocity mode
                self.velocity_mode = False
                self.modeupdate.emit('')

        elif event.key() == Qt.Key_A:
            for note in self.notes:
                if not note.isSelected():
                    has_unselected = True
                    break
            else:
                has_unselected = False

            # select all notes
            if has_unselected:
                for note in self.notes:
                    note.setSelected(True)
                self.selected_notes = self.notes[:]
            # unselect all
            else:
                for note in self.notes:
                    note.setSelected(False)
                self.selected_notes = []

        elif event.key() in (Qt.Key_Delete, Qt.Key_Backspace):
            # remove selected notes from our notes list
            self.notes = [note for note in self.notes if note not in self.selected_notes]
            # delete the selected notes
            for note in self.selected_notes:
                self.removeItem(note)
                self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
                del note
            self.selected_notes = []

    def mousePressEvent(self, event):
        QGraphicsScene.mousePressEvent(self, event)

        # mouse click on left-side piano area
        if self.piano.contains(event.scenePos()):
            self.ignore_mouse_events = True
            return

        clicked_notes = []

        for note in self.notes:
            if note.pressed or note.back.stretch or note.front.stretch:
                clicked_notes.append(note)

        # mouse click on existing notes
        if clicked_notes:
            keep_selection = all(note in self.selected_notes for note in clicked_notes)
            if keep_selection:
                for note in self.selected_notes:
                    note.setSelected(True)
                return

            for note in self.selected_notes:
                if note not in clicked_notes:
                    note.setSelected(False)
            for note in clicked_notes:
                if note not in self.selected_notes:
                    note.setSelected(True)

            self.selected_notes = clicked_notes
            return

        # mouse click on empty area (no note selected)
        for note in self.selected_notes:
            note.setSelected(False)
        self.selected_notes = []

        if event.button() != Qt.LeftButton:
            return

        if self.insert_mode:
            self.place_ghost = True
        else:
            self.marquee_select = True
            self.marquee_rect = QRectF(event.scenePos().x(), event.scenePos().y(), 1, 1)
            self.marquee = QGraphicsRectItem(self.marquee_rect)
            self.marquee.setBrush(QColor(255, 255, 255, 100))
            self.addItem(self.marquee)

    def mouseMoveEvent(self, event):
        QGraphicsScene.mouseMoveEvent(self, event)

        self.last_mouse_pos = event.scenePos()

        if self.ignore_mouse_events:
            return

        pos = self.enforce_bounds(self.last_mouse_pos)

        if self.insert_mode:
            if self.ghost_note is None:
                self.makeGhostNote(pos.x(), pos.y())
            max_x = self.grid_width + self.piano_width

            # placing note, only width needs updating
            if self.place_ghost:
                pos_x = pos.x()
                min_x = self.ghost_rect.x() + self.ghost_rect_orig_width
                if pos_x < min_x:
                    pos_x = min_x
                new_x = self.snap(pos_x)
                self.ghost_rect.setRight(new_x)
                self.ghost_note.setRect(self.ghost_rect)
                #self.adjust_note_vel(event)

            # ghostnote following mouse around
            else:
                pos_x = pos.x()
                if pos_x + self.ghost_rect.width() >= max_x:
                    pos_x = max_x - self.ghost_rect.width()
                elif pos_x > self.piano_width + self.ghost_rect.width()*3/4:
                    pos_x -= self.ghost_rect.width()/2
                new_x, new_y = self.snap(pos_x, pos.y())
                self.ghost_rect.moveTo(new_x, new_y)
                self.ghost_note.setRect(self.ghost_rect)
            return

        if self.marquee_select:
            marquee_orig_pos = event.buttonDownScenePos(Qt.LeftButton)
            if marquee_orig_pos.x() < pos.x() and marquee_orig_pos.y() < pos.y():
                self.marquee_rect.setBottomRight(pos)
            elif marquee_orig_pos.x() < pos.x() and marquee_orig_pos.y() > pos.y():
                self.marquee_rect.setTopRight(pos)
            elif marquee_orig_pos.x() > pos.x() and marquee_orig_pos.y() < pos.y():
                self.marquee_rect.setBottomLeft(pos)
            elif marquee_orig_pos.x() > pos.x() and marquee_orig_pos.y() > pos.y():
                self.marquee_rect.setTopLeft(pos)
            self.marquee.setRect(self.marquee_rect)

            for note in self.selected_notes:
                note.setSelected(False)
            self.selected_notes = []

            for item in self.collidingItems(self.marquee):
                if item in self.notes:
                    item.setSelected(True)
                    self.selected_notes.append(item)
            return

        if event.buttons() != Qt.LeftButton:
            return

        if self.velocity_mode:
            for note in self.selected_notes:
                note.updateVelocity(event)
            return

        x = y = False
        for note in self.selected_notes:
            if note.back.stretch:
                x = True
                break
        for note in self.selected_notes:
            if note.front.stretch:
                y = True
                break
        for note in self.selected_notes:
            note.back.stretch = x
            note.front.stretch = y
            note.moveEvent(event)

    def mouseReleaseEvent(self, event):
        QGraphicsScene.mouseReleaseEvent(self, event)

        if self.ignore_mouse_events:
            self.ignore_mouse_events = False
            return

        if self.marquee_select:
            self.marquee_select = False
            self.removeItem(self.marquee)
            self.marquee = None

        if self.insert_mode and self.place_ghost:
            self.place_ghost = False
            note_start = self.get_note_start_from_x(self.ghost_rect.x())
            note_num = self.get_note_num_from_y(self.ghost_rect.y())
            note_length = self.get_note_length_from_x(self.ghost_rect.width())
            note = self.drawNote(note_num, note_start, note_length, self.ghost_vel)
            note.setSelected(True)
            self.selected_notes.append(note)
            self.midievent.emit(["midievent-add", note_num, note_start, note_length, self.ghost_vel])
            pos = self.enforce_bounds(self.last_mouse_pos)
            pos_x = pos.x()
            if pos_x > self.piano_width + self.ghost_rect.width()*3/4:
                pos_x -= self.ghost_rect.width()/2
            self.makeGhostNote(pos_x, pos.y())

        for note in self.selected_notes:
            note.back.stretch = False
            note.front.stretch = False

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

        key = PianoKeyItem(piano_keys_width, self.note_height, 78, self.piano)
        label = QGraphicsSimpleTextItem('C9', key)
        label.setPos(18, 1)
        label.setFont(piano_label)
        key.setBrush(QColor(255, 255, 255))
        for i in range(self.end_octave - self.start_octave, 0, -1):
            for j in range(self.notes_in_octave, 0, -1):
                note = (self.end_octave - i + 3) * 12 - j
                if j in black_notes:
                    key = PianoKeyItem(piano_keys_width/1.4, self.note_height, note, self.piano)
                    key.setBrush(QColor(0, 0, 0))
                    key.setZValue(1.0)
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1))
                elif (j - 1) and (j + 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 2, note, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1) - self.note_height/2.)
                elif (j - 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 3./2, note, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1) - self.note_height/2.)
                elif (j + 1) in black_notes:
                    key = PianoKeyItem(piano_keys_width, self.note_height * 3./2, note, self.piano)
                    key.setBrush(QColor(255, 255, 255))
                    key.setPos(0, self.note_height * j + self.octave_height * (i - 1))
                if j == 12:
                    label = QGraphicsSimpleTextItem('{}{}'.format(labels[j - 1], self.end_octave - i + 1), key)
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
        self.place_ghost = False
        if self.ghost_note is not None:
            self.removeItem(self.ghost_note)
            self.ghost_note = None
        self.clear()
        self.drawPiano()
        self.drawHeader()
        self.drawGrid()
        self.drawPlayHead()

        for note in self.notes[:]:
            if note.note[1] >= (self.num_measures * self.time_sig[0]):
                self.notes.remove(note)
                self.removed_notes.append(note)
                #self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
            elif note.note[2] > self.max_note_length:
                new_note = note.note[:]
                new_note[2] = self.max_note_length
                self.notes.remove(note)
                self.drawNote(new_note[0], new_note[1], self.max_note_length, new_note[3], False)
                self.midievent.emit(["midievent-remove", note.note[0], note.note[1], note.note[2], note.note[3]])
                self.midievent.emit(["midievent-add", new_note[0], new_note[1], new_note[2], new_note[3]])

        for note in self.removed_notes[:]:
            if note.note[1] < (self.num_measures * self.time_sig[0]):
                self.removed_notes.remove(note)
                self.notes.append(note)

        list(map(self.addItem, self.notes))
        if self.views():
            self.views()[0].setSceneRect(self.itemsBoundingRect())

    def clearNotes(self):
        self.clear()
        self.notes = []
        self.removed_notes = []
        self.selected_notes = []
        self.drawPiano()
        self.drawHeader()
        self.drawGrid()

    def makeGhostNote(self, pos_x, pos_y):
        """creates the ghostnote that is placed on the scene before the real one is."""
        if self.ghost_note is not None:
            self.removeItem(self.ghost_note)
        length = self.full_note_width * self.default_length
        pos_x, pos_y = self.snap(pos_x, pos_y)
        self.ghost_vel = self.default_ghost_vel
        self.ghost_rect = QRectF(pos_x, pos_y, length, self.note_height)
        self.ghost_rect_orig_width = self.ghost_rect.width()
        self.ghost_note = QGraphicsRectItem(self.ghost_rect)
        self.ghost_note.setBrush(QColor(230, 221, 45, 100))
        self.addItem(self.ghost_note)

    def drawNote(self, note_num, note_start, note_length, note_velocity, add=True):
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

        return note

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
            pos_x = int(round((pos_x - self.piano_width) / self.snap_value)) * self.snap_value + self.piano_width
        if pos_y is not None:
            pos_y = int((pos_y - self.header_height) / self.note_height) * self.note_height + self.header_height
        return (pos_x, pos_y) if pos_y is not None else pos_x

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
        pos = QPointF(pos)
        if pos.x() < self.piano_width:
            pos.setX(self.piano_width)
        elif pos.x() >= self.grid_width + self.piano_width:
            pos.setX(self.grid_width + self.piano_width - 1)
        if pos.y() < self.header_height + self.padding:
            pos.setY(self.header_height + self.padding)
        return pos

    def get_note_start_from_x(self, note_x):
        return (note_x - self.piano_width) / (self.grid_width / self.num_measures / self.time_sig[0])

    def get_note_x_start(self, note_start):
        return self.piano_width + (self.grid_width / self.num_measures / self.time_sig[0]) * note_start

    def get_note_x_length(self, note_length):
        return float(self.time_sig[1]) / self.time_sig[0] * note_length * self.grid_width / self.num_measures

    def get_note_length_from_x(self, note_x):
        return float(self.time_sig[0]) / self.time_sig[1] * self.num_measures / self.grid_width * note_x

    def get_note_y_pos(self, note_num):
        return self.header_height + self.note_height * (self.total_notes - note_num - 1)

    def get_note_num_from_y(self, note_y_pos):
        return -(int((note_y_pos - self.header_height) / self.note_height) - self.total_notes + 1)

    def move_note(self, old_note, new_note):
        self.midievent.emit(["midievent-remove", old_note[0], old_note[1], old_note[2], old_note[3]])
        self.midievent.emit(["midievent-add", new_note[0], new_note[1], new_note[2], new_note[3]])

# ------------------------------------------------------------------------------------------------------------

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
        self.mode = None
        self.setFixedSize(30,20)

    def paintEvent(self, event):
        event.accept()

        painter = QPainter(self)
        painter.setPen(QPen(QColor(0, 0, 0, 0)))

        if self.mode == 'velocity_mode':
            painter.setBrush(QColor(127, 0, 0))
        elif self.mode == 'insert_mode':
            painter.setBrush(QColor(0, 100, 127))
        else:
            painter.setBrush(QColor(0, 0, 0, 0))

        painter.drawRect(0, 0, 30, 20)

    def changeMode(self, new_mode):
        self.mode = new_mode
        self.update()

# ------------------------------------------------------------------------------------------------------------
