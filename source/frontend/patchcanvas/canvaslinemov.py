#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import qWarning, Qt, QLineF
    from PyQt5.QtGui import QPainter, QPen
    from PyQt5.QtWidgets import QGraphicsLineItem
elif qt_config == 6:
    from PyQt6.QtCore import qWarning, Qt, QLineF
    from PyQt6.QtGui import QPainter, QPen
    from PyQt6.QtWidgets import QGraphicsLineItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    options,
    port_mode2str,
    port_type2str,
    CanvasLineMovType,
    PORT_MODE_INPUT,
    PORT_MODE_OUTPUT,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
)

# ------------------------------------------------------------------------------------------------------------

class CanvasLineMov(QGraphicsLineItem):
    def __init__(self, port_mode, port_type, parent):
        QGraphicsLineItem.__init__(self)
        self.setParentItem(parent)

        self.m_port_mode = port_mode
        self.m_port_type = port_type

        # Port position doesn't change while moving around line
        self.p_lineX = self.scenePos().x()
        self.p_lineY = self.scenePos().y()
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
            qWarning("PatchCanvas::CanvasLineMov({}, {}, {}) - invalid port type".format(
                     port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)

        pen.setCapStyle(Qt.RoundCap)
        pen.setWidthF(pen.widthF() + 0.00001)
        self.setPen(pen)

    def updateLinePos(self, scenePos):
        item_pos = [0, 0]

        if self.m_port_mode == PORT_MODE_INPUT:
            item_pos[0] = 0
            item_pos[1] = float(canvas.theme.port_height)/2
        elif self.m_port_mode == PORT_MODE_OUTPUT:
            item_pos[0] = self.p_width + 12
            item_pos[1] = float(canvas.theme.port_height)/2
        else:
            return

        line = QLineF(item_pos[0], item_pos[1], scenePos.x() - self.p_lineX, scenePos.y() - self.p_lineY)
        self.setLine(line)

    def type(self):
        return CanvasLineMovType

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsLineItem.paint(self, painter, option, widget)
        painter.restore()

# ------------------------------------------------------------------------------------------------------------
