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

from PyQt5.QtCore import qCritical, Qt, QLineF, QPointF, QRectF, QTimer, QSizeF
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
    CanvasPortGroupType,
    ANTIALIASING_FULL,
    ACTION_PORT_GROUP_ADD,
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
from .utils import (
    CanvasGetFullPortName,
    CanvasGetPortConnectionList,
    CanvasGetPortGroupPosition,
    CanvasGetPortPrintName,
    CanvasConnectionMatches,
    CanvasConnectionConcerns,
    CanvasCallback)

# ------------------------------------------------------------------------------------------------------------

class CanvasPort(QGraphicsItem):
    def __init__(self, group_id, port_id, port_name, port_mode, 
                 port_type, is_alternate, parent):
        QGraphicsItem.__init__(self)
        self.setParentItem(parent)

        # Save Variables, useful for later
        self.m_group_id = group_id
        self.m_port_id = port_id
        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_name = port_name
        self.m_portgrp_id = 0
        self.m_is_alternate = is_alternate

        # Base Variables
        self.m_port_width = 15
        self.m_port_height = canvas.theme.port_height
        self.m_port_font = QFont()
        self.m_port_font.setFamily(canvas.theme.port_font_name)
        self.m_port_font.setPixelSize(canvas.theme.port_font_size)
        self.m_port_font.setWeight(canvas.theme.port_font_state)

        self.m_line_mov_list = []
        self.m_dotcon_list = []
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

    def getPortGroupId(self):
        return self.m_portgrp_id
    
    def getFullPortName(self):
        return "%s:%s" % (self.parentItem().getGroupName(), self.m_port_name)

    def getPortWidth(self):
        return self.m_port_width

    def getPortHeight(self):
        return self.m_port_height

    def getPortGroupPosition(self):
        return CanvasGetPortGroupPosition(self.m_group_id, self.m_port_id,
                                          self.m_portgrp_id)
    
    def setPortMode(self, port_mode):
        self.m_port_mode = port_mode
        self.update()

    def setPortType(self, port_type):
        self.m_port_type = port_type
        self.update()

    def setPortGroupId(self, portgrp_id):
        self.m_portgrp_id = portgrp_id
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
    
    def resetLineMovPositions(self):
        for i in range(len(self.m_line_mov_list)):
            line_mov = self.m_line_mov_list[i]
            if i < 1:
                line_mov.setDestinationPortGroupPosition(i, 1)
            else:
                item = line_mov
                canvas.scene.removeItem(item)
                del item
        
        self.m_line_mov_list = self.m_line_mov_list[:1]
    
    def SetAsStereo(self, port_id):
        port_id_list = []
        for port in canvas.port_list:
            if (port.group_id == self.m_group_id
                    and port.port_id in (self.m_port_id, port_id)):
                port_id_list.append(port.port_id)
        
        data = "%i:%i:%i:%i:%i" % (
            self.m_group_id, self.m_port_mode, self.m_port_type,
            port_id_list[0], port_id_list[1])
        
        CanvasCallback(ACTION_PORT_GROUP_ADD, 0, 0, data)
    
    def connectToHover(self):
        if self.m_hover_item:
            if self.m_hover_item.type() == CanvasPortType:
                hover_port_id_list = [ self.m_hover_item.getPortId() ]
            elif self.m_hover_item.type() == CanvasPortGroupType:
                hover_port_id_list = self.m_hover_item.getPortsList()
            
            hover_group_id = self.m_hover_item.getGroupId()
            con_list = []
            ports_connected_list = []
            
            # FIXME clean this big if stuff
            for hover_port_id in hover_port_id_list:
                for connection in canvas.connection_list:
                    if CanvasConnectionMatches(connection,
                                    self.m_group_id, [self.m_port_id],
                                    hover_group_id, [hover_port_id]):
                        con_list.append(connection)
                        ports_connected_list.append(hover_port_id)
            
            if len(con_list) == len(hover_port_id_list):
                for connection in con_list:
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
            else:
                for porthover_id in hover_port_id_list:
                    if not porthover_id in ports_connected_list:
                        if self.m_port_mode == PORT_MODE_OUTPUT:
                            conn = "%i:%i:%i:%i" % (self.m_group_id, self.m_port_id, hover_group_id, porthover_id) 
                        else:
                            conn = "%i:%i:%i:%i" % (hover_group_id, porthover_id, self.m_group_id, self.m_port_id)
                        canvas.callback(ACTION_PORTS_CONNECT, '', '', conn)
    
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
                if CanvasConnectionConcerns(connection,
                                self.m_group_id, [self.m_port_id]):
                    connection.widget.setLocked(True)

        if not self.m_line_mov_list:
            if options.use_bezier_lines:
                line_mov = CanvasBezierLineMov(self.m_port_mode, 
                                               self.m_port_type, 0, 1, self)
            else:
                line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type, 
                                         0, 1, self)
            
            self.m_line_mov_list.append(line_mov)
            line_mov.setZValue(canvas.last_z_value)
            canvas.last_z_value += 1
            self.parentItem().setZValue(canvas.last_z_value)

        item = None
        items = canvas.scene.items(event.scenePos(), Qt.ContainsItemShape, Qt.AscendingOrder)
        
        #for i in range(len(items)):
        for _, itemx in enumerate(items):
            if not itemx.type() in (CanvasPortType, CanvasPortGroupType):
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
                if item.type() == CanvasPortGroupType:
                    if self.m_hover_item != item:
                        self.m_hover_item = item
                        
                        if len(self.m_line_mov_list) <= 1:
                            # make original line going to first port of the hover portgrp
                            for line_mov in self.m_line_mov_list:
                                line_mov.setDestinationPortGroupPosition(
                                    0, self.m_hover_item.getPortLength())
                                
                            port_pos, portgrp_len = CanvasGetPortGroupPosition(
                                self.m_group_id, self.m_port_id, self.m_portgrp_id)
                            
                            # create one line for each port of the hover portgrp
                            for i in range(1, self.m_hover_item.getPortLength()):
                                if options.use_bezier_lines:
                                    line_mov = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, port_pos, portgrp_len, self)
                                else:
                                    line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type, port_pos, portgrp_len, self)
                                line_mov.setDestinationPortGroupPosition(
                                    i, self.m_hover_item.getPortLength())
                                self.m_line_mov_list.append(line_mov)
                    
                elif item.type() == CanvasPortType:
                    if self.m_hover_item !=  item:
                        self.m_hover_item = item
                        self.resetLineMovPositions()
            else:
                self.m_hover_item = None
                self.resetLineMovPositions()
        else:
            self.m_hover_item = None
            self.resetLineMovPositions()
        
        for line_mov in self.m_line_mov_list:
            line_mov.updateLinePos(event.scenePos())
        
        return event.accept()

        QGraphicsItem.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.m_mouse_down:
            if self.m_line_mov_list:
                for line_mov in self.m_line_mov_list:
                    item = line_mov
                    canvas.scene.removeItem(item)
                    del item
                self.m_line_mov_list.clear()

            for connection in canvas.connection_list:
                if CanvasConnectionConcerns(connection, 
                            self.m_group_id, [self.m_port_id]):
                    connection.widget.setLocked(False)

            if self.m_hover_item:
                self.connectToHover()

                canvas.scene.clearSelection()

        if self.m_cursor_moving:
            self.unsetCursor()

        self.m_hover_item = None
        self.m_mouse_down = False
        self.m_cursor_moving = False
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
                act_x_disc.setData([conn_id])
                act_x_disc.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
        else:
            act_x_disc = discMenu.addAction("No connections")
            act_x_disc.setEnabled(False)

        menu.addMenu(discMenu)
        act_x_disc_all = menu.addAction("Disconnect &All")
        act_x_sep_1 = menu.addSeparator()
        
        if self.m_port_type == PORT_TYPE_AUDIO_JACK and not self.m_portgrp_id:
            StereoMenu = QMenu('Set as Stereo with', menu)
            menu.addMenu(StereoMenu)
            
            # get list of available mono ports settables as stereo with port
            port_cousin_list = []
            for port in canvas.port_list:
                if (port.port_type == PORT_TYPE_AUDIO_JACK
                        and port.group_id == self.m_group_id
                        and port.port_mode == self.m_port_mode):
                    port_cousin_list.append(port.port_id)
            
            selfport_index = port_cousin_list.index(self.m_port_id)
            stereo_able_ids_list = []
            if selfport_index > 0:
                stereo_able_ids_list.append(port_cousin_list[selfport_index -1])
            if selfport_index < len(port_cousin_list) -1:
                stereo_able_ids_list.append(port_cousin_list[selfport_index +1])
                
            at_least_one = False
            for port in canvas.port_list:
                if port.port_id in stereo_able_ids_list and not port.portgrp_id:
                    act_x_setasstereo = StereoMenu.addAction(port.port_name)
                    act_x_setasstereo.setData([self, port.port_id])
                    act_x_setasstereo.triggered.connect(canvas.qobject.SetasStereoWith)
                    at_least_one = True
                    
            if not at_least_one:
                act_x_setasstereo = StereoMenu.addAction('no available mono port')
                act_x_setasstereo.setEnabled(False)
        
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
            for conn_id, group_id, port_id in conn_list:
                canvas.callback(ACTION_PORTS_DISCONNECT, conn_id, 0, "")

        elif act_selected == act_x_info:
            canvas.callback(ACTION_PORT_INFO, self.m_group_id, self.m_port_id, "")

        elif act_selected == act_x_rename:
            canvas.callback(ACTION_PORT_RENAME, self.m_group_id, self.m_port_id, "")
            
    def setPortSelected(self, yesno):
        print('ofko')
        for connection in canvas.connection_list:
            if CanvasConnectionConcerns(connection,
                            self.m_group_id, [self.m_port_id]):
                connection.widget.updateLineSelected()

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemSelectedHasChanged:
            print('oroor')
            self.setPortSelected(value)
        return QGraphicsItem.itemChange(self, change, value)

    def triggerDisconnect(self, conn_list=None):
        if not conn_list:
            conn_list = CanvasGetPortConnectionList(self.m_group_id, self.m_port_id)
        for conn_id, group_id, port_id in conn_list:
            canvas.callback(ACTION_PORTS_DISCONNECT, conn_id, 0, "")

    def boundingRect(self):
        if self.m_portgrp_id:
            if self.m_port_mode == PORT_MODE_INPUT:
                return QRectF(0, 0, canvas.theme.port_in_portgrp_width, self.m_port_height)
            else:
                return QRectF(self.m_port_width +12 - canvas.theme.port_in_portgrp_width,
                              0, canvas.theme.port_in_portgrp_width, self.m_port_height)
        else:
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

        poly_locx = [0, 0, 0, 0, 0, 0]
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
                poly_locx[5] = canvas.theme.port_in_portgrp_width
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = lineHinting
                poly_locx[1] = self.m_port_width + 5 - lineHinting
                poly_locx[2] = self.m_port_width + 5 - lineHinting
                poly_locx[3] = self.m_port_width + 5 - lineHinting
                poly_locx[4] = lineHinting
                poly_locx[5] = canvas.theme.port_in_portgrp_width
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
                poly_locx[5] = self.m_port_width + 12 - canvas.theme.port_in_portgrp_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_port_width + 12 - lineHinting
                poly_locx[1] = 5 + lineHinting
                poly_locx[2] = 5 + lineHinting
                poly_locx[3] = 5 + lineHinting
                poly_locx[4] = self.m_port_width + 12 - lineHinting
                poly_locx[5] = self.m_port_width + 12 -canvas.theme.port_in_portgrp_width - lineHinting
            else:
                qCritical("PatchCanvas::CanvasPort.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port mode '%s'" % port_mode2str(self.m_port_mode))
            return

        if self.m_is_alternate:
            poly_color = poly_color.darker(180)
            #poly_pen.setColor(poly_pen.color().darker(110))
            #text_pen.setColor(text_pen.color()) #.darker(150))
            #conn_pen.setColor(conn_pen.color()) #.darker(150))
        
        polygon  = QPolygonF()
        
        if self.m_portgrp_id:
            first_of_portgrp = False
            last_of_portgrp = False
            
            for portgrp in canvas.portgrp_list:
                if portgrp.portgrp_id == self.m_portgrp_id:
                    if self.m_port_id == portgrp.port_id_list[0]:
                        first_of_portgrp = True
                    if self.m_port_id == portgrp.port_id_list[-1]:
                        last_of_portgrp = True
                    break
                
            if first_of_portgrp:
                polygon += QPointF(poly_locx[0] , lineHinting)
                polygon += QPointF(poly_locx[5] , lineHinting)
            else:
                polygon += QPointF(poly_locx[0] , 0)
                polygon += QPointF(poly_locx[5] , 0)
                
            if last_of_portgrp:
                polygon += QPointF(poly_locx[5], canvas.theme.port_height - lineHinting)
                polygon += QPointF(poly_locx[0], canvas.theme.port_height - lineHinting)
            else:
                polygon += QPointF(poly_locx[5], canvas.theme.port_height)
                polygon += QPointF(poly_locx[0], canvas.theme.port_height)
        else:
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
        
        if self.m_portgrp_id:
            print_name = CanvasGetPortPrintName(
                            self.m_group_id, self.m_port_id, 
                            self.m_portgrp_id)
            print_name_size = QFontMetrics(self.m_port_font).width(print_name)    
            if self.m_port_mode == PORT_MODE_OUTPUT:
                
                text_pos = QPointF(self.m_port_width + 9 - print_name_size, canvas.theme.port_text_ypos) 

            if print_name_size > (canvas.theme.port_in_portgrp_width - 6):
                painter.setPen(QPen(poly_color, 3))
                painter.drawLine(poly_locx[5], 3, poly_locx[5], canvas.theme.port_height - 3)
                painter.setPen(text_pen)
                painter.setFont(self.m_port_font)
            painter.drawText(text_pos, print_name)
            
        else:
            painter.drawText(text_pos, self.m_port_name)

        if canvas.theme.idx == Theme.THEME_OOSTUDIO and canvas.theme.port_bg_pixmap:
            painter.setPen(Qt.NoPen)
            painter.setBrush(conn_pen.brush())

            if self.m_port_mode == PORT_MODE_INPUT:
                connRect = QRectF(portRect.topLeft(), QSizeF(2, portRect.height()))
            else:
                connRect = QRectF(QPointF(portRect.right()-2, portRect.top()), QSizeF(2, portRect.height()))

            painter.drawRect(connRect)

        painter.restore()

# ------------------------------------------------------------------------------------------------------------
