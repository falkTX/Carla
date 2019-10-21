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

from PyQt5.QtCore import Qt, QLineF
from PyQt5.QtGui import QLinearGradient, QPainter, QPen
from PyQt5.QtWidgets import QGraphicsLineItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    options,
    CanvasLineType,
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

class CanvasLine(QGraphicsLineItem):
    def __init__(self, item1, item2, parent):
        QGraphicsLineItem.__init__(self)
        self.setParentItem(parent)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False

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
            port_pos_1, portgrp_len_1 = self.item1.getPortGroupPosition()
            
            if portgrp_len_1 > 2:
                phi = 0.75
            else:
                phi = 0.62
            
            if portgrp_len_1 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (portgrp_len_1 - phi)
                delta = (last_old_y - first_old_y) / (portgrp_len_1 -1)
                Y1 = first_old_y + (port_pos_1 * delta) - (canvas.theme.port_height * port_pos_1)
            else:
                Y1 = canvas.theme.port_height / 2
                
            port_pos_2, portgrp_len_2 = self.item2.getPortGroupPosition()
            
            if portgrp_len_2 > 2:
                phi = 0.75
            else:
                phi = 0.62
            
            if portgrp_len_2 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (portgrp_len_2 - phi)
                delta = (last_old_y - first_old_y) / (portgrp_len_2 -1)
                Y2 = first_old_y + (port_pos_2 * delta) - (canvas.theme.port_height * port_pos_2)
            else:
                Y2 = canvas.theme.port_height / 2
            
            line = QLineF(self.item1.scenePos().x() + self.item1.getPortWidth() + 12,
                          self.item1.scenePos().y() + Y1,
                          self.item2.scenePos().x(),
                          self.item2.scenePos().y() + Y2)
            self.setLine(line)

            self.m_lineSelected = False
            self.updateLineGradient()

    def type(self):
        return CanvasLineType

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

        self.setPen(QPen(port_gradient, 2.00001, Qt.SolidLine, Qt.RoundCap))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))

        pen = self.pen()
        cosm_pen = QPen(pen)
        cosm_pen.setCosmetic(True)
        cosm_pen.setWidthF(1.00001)

        QGraphicsLineItem.paint(self, painter, option, widget)

        painter.setPen(cosm_pen)
        painter.setBrush(Qt.NoBrush)
        painter.setOpacity(0.2)
        painter.drawLine(self.line())

        painter.restore()

# ------------------------------------------------------------------------------------------------------------
