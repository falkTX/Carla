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

from PyQt5.QtCore import qWarning, Qt, QPointF
from PyQt5.QtGui import QPainter, QPainterPath, QPen
from PyQt5.QtWidgets import QGraphicsPathItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    options,
    port_mode2str,
    port_type2str,
    CanvasBezierLineMovType,
    PORT_MODE_INPUT,
    PORT_MODE_OUTPUT,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
)

# ------------------------------------------------------------------------------------------------------------

class CanvasBezierLineMov(QGraphicsPathItem):
    def __init__(self, port_mode, port_type, parent):
        QGraphicsPathItem.__init__(self)
        self.setParentItem(parent)

        self.m_port_mode = port_mode
        self.m_port_type = port_type

        # Port position doesn't change while moving around line
        self.p_itemX = self.scenePos().x()
        self.p_itemY = self.scenePos().y()
        self.p_width = parent.getPortWidth()

        if port_type == PORT_TYPE_AUDIO_JACK:
            pen = QPen(canvas.theme.line_audio_jack, 2)
        elif port_type == PORT_TYPE_MIDI_JACK:
            pen = QPen(canvas.theme.line_midi_jack, 2)
        elif port_type == PORT_TYPE_MIDI_ALSA:
            pen = QPen(canvas.theme.line_midi_alsa, 2)
        elif port_type == PORT_TYPE_PARAMETER:
            pen = QPen(canvas.theme.line_parameter, 2)
        else:
            qWarning("PatchCanvas::CanvasBezierLineMov({}, {}, {}) - invalid port type".format(
                     port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)

        pen.setCapStyle(Qt.FlatCap)
        pen.setWidthF(pen.widthF() + 0.00001)
        self.setPen(pen)

    def updateLinePos(self, scenePos):
        if self.m_port_mode == PORT_MODE_INPUT:
            old_x = 0
            old_y = float(canvas.theme.port_height)/2
            mid_x = abs(scenePos.x() - self.p_itemX) / 2
            new_x = old_x - mid_x
        elif self.m_port_mode == PORT_MODE_OUTPUT:
            old_x = self.p_width + 12
            old_y = float(canvas.theme.port_height)/2
            mid_x = abs(scenePos.x() - (self.p_itemX + old_x)) / 2
            new_x = old_x + mid_x
        else:
            return

        final_x = scenePos.x() - self.p_itemX
        final_y = scenePos.y() - self.p_itemY

        path = QPainterPath(QPointF(old_x, old_y))
        path.cubicTo(new_x, old_y, new_x, final_y, final_x, final_y)
        self.setPath(path)

    def type(self):
        return CanvasBezierLineMovType

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsPathItem.paint(self, painter, option, widget)
        painter.restore()

# ------------------------------------------------------------------------------------------------------------
