#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas engine using QGraphicsView/Scene
# Copyright (C) 2010-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import Qt, QPointF
from PyQt5.QtGui import QColor, QLinearGradient, QPainter, QPainterPath, QPen
from PyQt5.QtWidgets import QGraphicsPathItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    options,
    CanvasBezierLineType,
    ACTION_PORTS_DISCONNECT,
    EYECANDY_FULL,
    PORT_MODE_OUTPUT,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
)

from .canvasportglow import CanvasPortGlow

# ------------------------------------------------------------------------------------------------------------

class CanvasBezierLine(QGraphicsPathItem):
    def __init__(self, item1, item2, parent):
        QGraphicsPathItem.__init__(self)
        self.setParentItem(parent)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False

        self.setBrush(QColor(0, 0, 0, 0))
        self.setGraphicsEffect(None)
        self.updateLinePos()

    def isLocked(self):
        return self.m_locked

    def setLocked(self, yesno):
        self.m_locked = yesno

    def isLineSelected(self):
        return self.m_lineSelected

    def updateLineSelected(self):
        if self.m_locked:
            return

        yesno = self.item1.isSelected() or self.item2.isSelected()
        if yesno != self.m_lineSelected and options.eyecandy == EYECANDY_FULL:
            if yesno:
                self.setGraphicsEffect(CanvasPortGlow(self.item1.getPortType(), self.toGraphicsObject()))
            else:
                self.setGraphicsEffect(None)

        self.m_lineSelected = yesno
        self.updateLineGradient()

    def triggerDisconnect(self):
        for connection in canvas.connection_list:
            if (connection.port_out_id == self.item1.getPortId() and connection.port_in_id == self.item2.getPortId()):
                canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                break

    def updateLinePos(self):
        if self.item1.getPortMode() == PORT_MODE_OUTPUT:
            rect1 = self.item1.sceneBoundingRect()
            rect2 = self.item2.sceneBoundingRect()

            item1_x = rect1.right()
            item2_x = rect2.left()
            item1_y = rect1.top() + float(canvas.theme.port_height)/2
            item2_y = rect2.top() + float(canvas.theme.port_height)/2
            item1_new_x = item1_x + abs(item1_x - item2_x) / 2
            item2_new_x = item2_x - abs(item1_x - item2_x) / 2

            path = QPainterPath(QPointF(item1_x, item1_y))
            path.cubicTo(item1_new_x, item1_y, item2_new_x, item2_y, item2_x, item2_y)
            self.setPath(path)

            self.m_lineSelected = False
            self.updateLineGradient()

    def type(self):
        return CanvasBezierLineType

    def updateLineGradient(self):
        pos_top = self.boundingRect().top()
        pos_bot = self.boundingRect().bottom()
        if self.item2.scenePos().y() >= self.item1.scenePos().y():
            pos1 = 0
            pos2 = 1
        else:
            pos1 = 1
            pos2 = 0

        port_type1 = self.item1.getPortType()
        port_type2 = self.item2.getPortType()
        port_gradient = QLinearGradient(0, pos_top, 0, pos_bot)

        if port_type1 == PORT_TYPE_AUDIO_JACK:
            port_gradient.setColorAt(pos1, canvas.theme.line_audio_jack_sel if self.m_lineSelected else canvas.theme.line_audio_jack)
        elif port_type1 == PORT_TYPE_MIDI_JACK:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_jack_sel if self.m_lineSelected else canvas.theme.line_midi_jack)
        elif port_type1 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)
        elif port_type1 == PORT_TYPE_PARAMETER:
            port_gradient.setColorAt(pos1, canvas.theme.line_parameter_sel if self.m_lineSelected else canvas.theme.line_parameter)

        if port_type2 == PORT_TYPE_AUDIO_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_audio_jack_sel if self.m_lineSelected else canvas.theme.line_audio_jack)
        elif port_type2 == PORT_TYPE_MIDI_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_jack_sel if self.m_lineSelected else canvas.theme.line_midi_jack)
        elif port_type2 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)
        elif port_type2 == PORT_TYPE_PARAMETER:
            port_gradient.setColorAt(pos2, canvas.theme.line_parameter_sel if self.m_lineSelected else canvas.theme.line_parameter)

        self.setPen(QPen(port_gradient, 2.00001, Qt.SolidLine, Qt.FlatCap))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))

        pen = self.pen()
        cosm_pen = QPen(pen)
        cosm_pen.setCosmetic(True)
        cosm_pen.setWidthF(1.00001)

        QGraphicsPathItem.paint(self, painter, option, widget)

        painter.setPen(cosm_pen)
        painter.setBrush(Qt.NoBrush)
        painter.setOpacity(0.2)
        painter.drawPath(self.path())

        painter.restore()

# ------------------------------------------------------------------------------------------------------------
