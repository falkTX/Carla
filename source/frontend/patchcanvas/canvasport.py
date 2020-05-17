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

from math import floor

from PyQt5.QtCore import qCritical, Qt, QLineF, QPointF, QRectF, QTimer
from PyQt5.QtGui import QCursor, QFont, QFontMetrics, QPainter, QPainterPath, QPen, QPolygonF
from PyQt5.QtWidgets import QGraphicsItem, QMenu

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    features,
    options,
    port_mode2str,
    port_type2str,
    CanvasPortType,
    ANTIALIASING_FULL,
    ACTION_PORT_INFO,
    ACTION_PORT_RENAME,
    ACTION_PORTS_CONNECT,
    ACTION_PORTS_DISCONNECT,
    PORT_MODE_INPUT,
    PORT_MODE_OUTPUT,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
)

from .canvasbezierlinemov import CanvasBezierLineMov
from .canvaslinemov import CanvasLineMov
from .theme import Theme
from .utils import CanvasGetFullPortName, CanvasGetPortConnectionList

# ------------------------------------------------------------------------------------------------------------

class CanvasPort(QGraphicsItem):
    def __init__(self, group_id, port_id, port_name, port_mode, port_type, is_alternate, parent):
        QGraphicsItem.__init__(self)
        self.setParentItem(parent)

        # Save Variables, useful for later
        self.m_group_id = group_id
        self.m_port_id = port_id
        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_name = port_name
        self.m_is_alternate = is_alternate

        # Base Variables
        self.m_port_width = 15
        self.m_port_height = canvas.theme.port_height
        self.m_port_font = QFont()
        self.m_port_font.setFamily(canvas.theme.port_font_name)
        self.m_port_font.setPixelSize(canvas.theme.port_font_size)
        self.m_port_font.setWeight(canvas.theme.port_font_state)

        self.m_line_mov = None
        self.m_hover_item = None

        self.m_mouse_down = False
        self.m_cursor_moving = False

        self.setFlags(QGraphicsItem.ItemIsSelectable)

        if options.auto_select_items:
            self.setAcceptHoverEvents(True)

    def getGroupId(self):
        return self.m_group_id

    def getPortId(self):
        return self.m_port_id

    def getPortMode(self):
        return self.m_port_mode

    def getPortType(self):
        return self.m_port_type

    def getPortName(self):
        return self.m_port_name

    def getFullPortName(self):
        return self.parentItem().getGroupName() + ":" + self.m_port_name

    def getPortWidth(self):
        return self.m_port_width

    def getPortHeight(self):
        return self.m_port_height

    def setPortMode(self, port_mode):
        self.m_port_mode = port_mode
        self.update()

    def setPortType(self, port_type):
        self.m_port_type = port_type
        self.update()

    def setPortName(self, port_name):
        if QFontMetrics(self.m_port_font).width(port_name) < QFontMetrics(self.m_port_font).width(self.m_port_name):
            QTimer.singleShot(0, canvas.scene.update)

        self.m_port_name = port_name
        self.update()

    def setPortWidth(self, port_width):
        if port_width < self.m_port_width:
            QTimer.singleShot(0, canvas.scene.update)

        self.m_port_width = port_width
        self.update()

    def type(self):
        return CanvasPortType

    def hoverEnterEvent(self, event):
        if options.auto_select_items:
            self.setSelected(True)
        QGraphicsItem.hoverEnterEvent(self, event)

    def hoverLeaveEvent(self, event):
        if options.auto_select_items:
            self.setSelected(False)
        QGraphicsItem.hoverLeaveEvent(self, event)

    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton or event.source() == Qt.MouseEventSynthesizedByApplication:
            event.ignore()
            return

        if self.m_mouse_down:
            self.handleMouseRelease()
        self.m_hover_item = None
        self.m_mouse_down = bool(event.button() == Qt.LeftButton)
        self.m_cursor_moving = False
        QGraphicsItem.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if not self.m_mouse_down:
            QGraphicsItem.mouseMoveEvent(self, event)
            return

        event.accept()

        if not self.m_cursor_moving:
            self.setCursor(QCursor(Qt.CrossCursor))
            self.m_cursor_moving = True

            for connection in canvas.connection_list:
                if (
                    (connection.group_out_id == self.m_group_id and
                     connection.port_out_id == self.m_port_id)
                    or
                    (connection.group_in_id == self.m_group_id and
                     connection.port_in_id == self.m_port_id)
                   ):
                    connection.widget.setLocked(True)

        if not self.m_line_mov:
            if options.use_bezier_lines:
                self.m_line_mov = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, self)
            else:
                self.m_line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type, self)

            canvas.last_z_value += 1
            self.m_line_mov.setZValue(canvas.last_z_value)
            canvas.last_z_value += 1
            self.parentItem().setZValue(canvas.last_z_value)

        item = None
        items = canvas.scene.items(event.scenePos(), Qt.ContainsItemShape, Qt.AscendingOrder)
        #for i in range(len(items)):
        for _, itemx in enumerate(items):
            if itemx.type() != CanvasPortType:
                continue
            if itemx == self:
                continue
            if item is None or itemx.parentItem().zValue() > item.parentItem().zValue():
                item = itemx

        if self.m_hover_item and self.m_hover_item != item:
            self.m_hover_item.setSelected(False)

        if item is not None:
            if item.getPortMode() != self.m_port_mode and item.getPortType() == self.m_port_type:
                item.setSelected(True)
                self.m_hover_item = item
            else:
                self.m_hover_item = None
        else:
            self.m_hover_item = None

        self.m_line_mov.updateLinePos(event.scenePos())

    def handleMouseRelease(self):
        if self.m_mouse_down:
            if self.m_line_mov is not None:
                item = self.m_line_mov
                self.m_line_mov = None
                canvas.scene.removeItem(item)
                del item

            for connection in canvas.connection_list:
                if (
                    (connection.group_out_id == self.m_group_id and
                     connection.port_out_id == self.m_port_id)
                    or
                    (connection.group_in_id == self.m_group_id and
                     connection.port_in_id == self.m_port_id)
                   ):
                    connection.widget.setLocked(False)

            if self.m_hover_item:
                # TODO: a better way to check already existing connection
                for connection in canvas.connection_list:
                    hover_group_id = self.m_hover_item.getGroupId()
                    hover_port_id = self.m_hover_item.getPortId()

                    # FIXME clean this big if stuff
                    if (
                        (connection.group_out_id == self.m_group_id and
                         connection.port_out_id == self.m_port_id and
                         connection.group_in_id == hover_group_id and
                         connection.port_in_id == hover_port_id)
                        or
                        (connection.group_out_id == hover_group_id and
                         connection.port_out_id == hover_port_id and
                         connection.group_in_id == self.m_group_id and
                         connection.port_in_id == self.m_port_id)
                       ):
                        canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                        break
                else:
                    if self.m_port_mode == PORT_MODE_OUTPUT:
                        conn = "%i:%i:%i:%i" % (self.m_group_id, self.m_port_id,
                                                self.m_hover_item.getGroupId(), self.m_hover_item.getPortId())
                        canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)
                    else:
                        conn = "%i:%i:%i:%i" % (self.m_hover_item.getGroupId(),
                                                self.m_hover_item.getPortId(), self.m_group_id, self.m_port_id)
                        canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)

                canvas.scene.clearSelection()

        if self.m_cursor_moving:
            self.unsetCursor()

        self.m_hover_item = None
        self.m_mouse_down = False
        self.m_cursor_moving = False

    def mouseReleaseEvent(self, event):
        self.handleMouseRelease()
        QGraphicsItem.mouseReleaseEvent(self, event)

    def contextMenuEvent(self, event):
        event.accept()

        canvas.scene.clearSelection()
        self.setSelected(True)

        menu = QMenu()
        discMenu = QMenu("Disconnect", menu)

        conn_list = CanvasGetPortConnectionList(self.m_group_id, self.m_port_id)

        if len(conn_list) > 0:
            for conn_id, group_id, port_id in conn_list:
                act_x_disc = discMenu.addAction(CanvasGetFullPortName(group_id, port_id))
                act_x_disc.setData(conn_id)
                act_x_disc.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
        else:
            act_x_disc = discMenu.addAction("No connections")
            act_x_disc.setEnabled(False)

        menu.addMenu(discMenu)
        act_x_disc_all = menu.addAction("Disconnect &All")
        act_x_sep_1 = menu.addSeparator()
        act_x_info = menu.addAction("Get &Info")
        act_x_rename = menu.addAction("&Rename")

        if not features.port_info:
            act_x_info.setVisible(False)

        if not features.port_rename:
            act_x_rename.setVisible(False)

        if not (features.port_info and features.port_rename):
            act_x_sep_1.setVisible(False)

        act_selected = menu.exec_(event.screenPos())

        if act_selected == act_x_disc_all:
            self.triggerDisconnect(conn_list)

        elif act_selected == act_x_info:
            canvas.callback(ACTION_PORT_INFO, self.m_group_id, self.m_port_id, "")

        elif act_selected == act_x_rename:
            canvas.callback(ACTION_PORT_RENAME, self.m_group_id, self.m_port_id, "")

    def setPortSelected(self, yesno):
        for connection in canvas.connection_list:
            if (
                (connection.group_out_id == self.m_group_id and
                 connection.port_out_id == self.m_port_id)
                or
                (connection.group_in_id == self.m_group_id and
                 connection.port_in_id == self.m_port_id)
               ):
                connection.widget.updateLineSelected()

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemSelectedHasChanged:
            self.setPortSelected(value)
        return QGraphicsItem.itemChange(self, change, value)

    def triggerDisconnect(self, conn_list=None):
        if not conn_list:
            conn_list = CanvasGetPortConnectionList(self.m_group_id, self.m_port_id)
        for conn_id, group_id, port_id in conn_list:
            canvas.callback(ACTION_PORTS_DISCONNECT, conn_id, 0, "")

    def boundingRect(self):
        return QRectF(0, 0, self.m_port_width + 12, self.m_port_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))

        selected = self.isSelected()
        theme = canvas.theme
        if self.m_port_type == PORT_TYPE_AUDIO_JACK:
            poly_color = theme.port_audio_jack_bg_sel if selected else theme.port_audio_jack_bg
            poly_pen = theme.port_audio_jack_pen_sel  if selected else theme.port_audio_jack_pen
            text_pen = theme.port_audio_jack_text_sel if selected else theme.port_audio_jack_text
            conn_pen = QPen(theme.port_audio_jack_pen_sel)
        elif self.m_port_type == PORT_TYPE_MIDI_JACK:
            poly_color = theme.port_midi_jack_bg_sel if selected else theme.port_midi_jack_bg
            poly_pen = theme.port_midi_jack_pen_sel  if selected else theme.port_midi_jack_pen
            text_pen = theme.port_midi_jack_text_sel if selected else theme.port_midi_jack_text
            conn_pen = QPen(theme.port_midi_jack_pen_sel)
        elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
            poly_color = theme.port_midi_alsa_bg_sel if selected else theme.port_midi_alsa_bg
            poly_pen = theme.port_midi_alsa_pen_sel  if selected else theme.port_midi_alsa_pen
            text_pen = theme.port_midi_alsa_text_sel if selected else theme.port_midi_alsa_text
            conn_pen = QPen(theme.port_midi_alsa_pen_sel)
        elif self.m_port_type == PORT_TYPE_PARAMETER:
            poly_color = theme.port_parameter_bg_sel if selected else theme.port_parameter_bg
            poly_pen = theme.port_parameter_pen_sel  if selected else theme.port_parameter_pen
            text_pen = theme.port_parameter_text_sel if selected else theme.port_parameter_text
            conn_pen = QPen(theme.port_parameter_pen_sel)
        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port type '%s'" % port_type2str(self.m_port_type))
            return

        # To prevent quality worsening
        poly_pen = QPen(poly_pen)
        poly_pen.setWidthF(poly_pen.widthF() + 0.00001)

        if self.m_is_alternate:
            poly_color = poly_color.darker(180)
            #poly_pen.setColor(poly_pen.color().darker(110))
            #text_pen.setColor(text_pen.color()) #.darker(150))
            #conn_pen.setColor(conn_pen.color()) #.darker(150))

        lineHinting = poly_pen.widthF() / 2

        poly_locx = [0, 0, 0, 0, 0]
        poly_corner_xhinting = (float(canvas.theme.port_height)/2) % floor(float(canvas.theme.port_height)/2)
        if poly_corner_xhinting == 0:
            poly_corner_xhinting = 0.5 * (1 - 7 / (float(canvas.theme.port_height)/2))

        if self.m_port_mode == PORT_MODE_INPUT:
            text_pos = QPointF(3, canvas.theme.port_text_ypos)

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = lineHinting
                poly_locx[1] = self.m_port_width + 5 - lineHinting
                poly_locx[2] = self.m_port_width + 12 - poly_corner_xhinting
                poly_locx[3] = self.m_port_width + 5 - lineHinting
                poly_locx[4] = lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = lineHinting
                poly_locx[1] = self.m_port_width + 5 - lineHinting
                poly_locx[2] = self.m_port_width + 5 - lineHinting
                poly_locx[3] = self.m_port_width + 5 - lineHinting
                poly_locx[4] = lineHinting
            else:
                qCritical("PatchCanvas::CanvasPort.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        elif self.m_port_mode == PORT_MODE_OUTPUT:
            text_pos = QPointF(9, canvas.theme.port_text_ypos)

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = self.m_port_width + 12 - lineHinting
                poly_locx[1] = 7 + lineHinting
                poly_locx[2] = 0 + poly_corner_xhinting
                poly_locx[3] = 7 + lineHinting
                poly_locx[4] = self.m_port_width + 12 - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_port_width + 12 - lineHinting
                poly_locx[1] = 5 + lineHinting
                poly_locx[2] = 5 + lineHinting
                poly_locx[3] = 5 + lineHinting
                poly_locx[4] = self.m_port_width + 12 - lineHinting
            else:
                qCritical("PatchCanvas::CanvasPort.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port mode '%s'" % port_mode2str(self.m_port_mode))
            return

        polygon = QPolygonF()
        polygon += QPointF(poly_locx[0], lineHinting)
        polygon += QPointF(poly_locx[1], lineHinting)
        polygon += QPointF(poly_locx[2], float(canvas.theme.port_height)/2)
        polygon += QPointF(poly_locx[3], canvas.theme.port_height - lineHinting)
        polygon += QPointF(poly_locx[4], canvas.theme.port_height - lineHinting)
        polygon += QPointF(poly_locx[0], lineHinting)

        if canvas.theme.port_bg_pixmap:
            portRect = polygon.boundingRect().adjusted(-lineHinting+1, -lineHinting+1, lineHinting-1, lineHinting-1)
            portPos = portRect.topLeft()
            painter.drawTiledPixmap(portRect, canvas.theme.port_bg_pixmap, portPos)
        else:
            painter.setBrush(poly_color) #.lighter(200))

        painter.setPen(poly_pen)
        painter.drawPolygon(polygon)

        painter.setPen(text_pen)
        painter.setFont(self.m_port_font)
        painter.drawText(text_pos, self.m_port_name)

        if canvas.theme.idx == Theme.THEME_OOSTUDIO and canvas.theme.port_bg_pixmap:
            conn_pen.setCosmetic(True)
            conn_pen.setWidthF(0.4)
            painter.setPen(conn_pen)

            if self.m_port_mode == PORT_MODE_INPUT:
                connLineX = portRect.left()+1
            else:
                connLineX = portRect.right()-1
            conn_path = QPainterPath()
            conn_path.addRect(QRectF(connLineX-1, portRect.top(), 2, portRect.height()))
            painter.fillPath(conn_path, conn_pen.brush())
            painter.drawLine(QLineF(connLineX, portRect.top(), connLineX, portRect.bottom()))

        painter.restore()

# ------------------------------------------------------------------------------------------------------------
