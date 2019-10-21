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
from PyQt5.QtGui import (QCursor, QFont, QFontMetrics, QPainter, QPainterPath,
                         QPen, QPolygonF, QLinearGradient)
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
    ACTION_PORT_GROUP_REMOVE,
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
from .utils import (CanvasGetFullPortName, CanvasGetPortConnectionList,
                    CanvasGetPortGroupPosition, CanvasGetPortPrintName,
                    CanvasGetPortGroupName, CanvasGetPortGroupFullName, 
                    CanvasUpdateSelectedLines, CanvasCallback)

# ------------------------------------------------------------------------------------------------------------



class CanvasPortGroup(QGraphicsItem):
    def __init__(self, group_id, portgrp_id, port_mode, port_type, port_id_list, parent):
        QGraphicsItem.__init__(self)
        self.setParentItem(parent)

        # Save Variables, useful for later
        self.m_portgrp_id    = portgrp_id
        self.m_port_mode  = port_mode
        self.m_port_type  = port_type
        self.m_port_id_list = port_id_list
        self.m_group_id = group_id
        
        # Base Variables
        self.m_portgrp_width  = 15
        self.m_portgrp_height = canvas.theme.port_height
        self.m_portgrp_font = QFont()
        self.m_portgrp_font.setFamily(canvas.theme.port_font_name)
        self.m_portgrp_font.setPixelSize(canvas.theme.port_font_size)
        self.m_portgrp_font.setWeight(canvas.theme.port_font_state)

        self.m_line_mov_list = []
        self.m_r_click_conn = None
        self.m_r_click_time = 0
        self.m_dotcon_list = []
        self.m_hover_item = None
        self.m_last_selected_state = False

        self.m_mouse_down = False
        self.m_cursor_moving = False
        self.setFlags(QGraphicsItem.ItemIsSelectable)
      
    def getPortGroupId(self):
        return self.m_portgrp_id

    def getPortMode(self):
        return self.m_port_mode

    def getPortType(self):
        return self.m_port_type
    
    def getGroupId(self):
        return self.m_group_id
    
    def getPortWidth(self):
        return self.m_portgrp_width
    
    def getPortGroupWidth(self):
        return self.m_portgrp_width

    def getPortGroupHeight(self):
        return self.m_port_height
    
    def getPortsList(self):
        return self.m_port_id_list
    
    def getPortLength(self):
        return len(self.m_port_id_list)
    
    def setPortMode(self, port_mode):
        self.m_port_mode = port_mode
        self.update()

    def type(self):
        return CanvasPortGroupType

    def setPortGroupWidth(self, portgrp_width):
        if portgrp_width < self.m_portgrp_width:
            QTimer.singleShot(0, canvas.scene.update)

        self.m_portgrp_width = portgrp_width
        self.update()

    def SplitToMonos(self):
        CanvasCallback(ACTION_PORT_GROUP_REMOVE, self.m_group_id, self.m_portgrp_id, "")
        
    def ConnectToHover(self):
        if self.m_hover_item:
            if self.m_hover_item.type() == CanvasPortType:
                hover_port_id_list = [ self.m_hover_item.getPortId() ]
            elif self.m_hover_item.type() == CanvasPortGroupType:
                hover_port_id_list = self.m_hover_item.getPortsList()
                
            con_list = []
            ports_connected_list = []
            
            for port in canvas.port_list:
                if port.port_id == hover_port_id_list[0]:
                    hover_portgrp_id = port.group_id
                    break
            else:
                return
            
            for port in canvas.port_list:
                if port.port_id in self.m_port_id_list:
                    selfgroup_id = port.group_id
                    break
            else:
                return
            
            for portself_id in self.m_port_id_list:
                for porthover_id in hover_port_id_list:
                    for connection in canvas.connection_list:
                        if ( (connection.port_out_id == portself_id and connection.port_in_id == porthover_id) or
                             (connection.port_out_id == porthover_id and connection.port_in_id == portself_id) ):
                            
                            if ( (self.m_port_id_list.index(portself_id) % len(hover_port_id_list)) ==
                                (hover_port_id_list.index(porthover_id) % len(self.m_port_id_list)) ):
                                con_list.append(connection)
                                ports_connected_list.append([portself_id, porthover_id])
                            else:
                                canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                                
            maxportgrp = len(hover_port_id_list) if len(hover_port_id_list) > len(self.m_port_id_list) else len(self.m_port_id_list)
            
            if len(con_list) == maxportgrp:
                for connection in con_list:
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
            else:
                for portself_id in self.m_port_id_list:
                    for porthover_id in hover_port_id_list:
                        if ((self.m_port_id_list.index(portself_id) % len(hover_port_id_list)) == 
                            (hover_port_id_list.index(porthover_id) % len(self.m_port_id_list))):
                            if not [portself_id, porthover_id] in ports_connected_list:
                                if self.m_port_mode == PORT_MODE_OUTPUT:
                                    conn = "%i:%i:%i:%i" % (selfgroup_id, portself_id, hover_portgrp_id, porthover_id)
                                else:
                                    conn = "%i:%i:%i:%i" % (hover_portgrp_id, porthover_id, selfgroup_id, portself_id)
                                canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)
                    
    def resetDotLines(self):
        for connection in self.m_dotcon_list:
            if connection.widget.isReadyToDisc():
                connection.widget.setReadyToDisc(False)
                connection.widget.updateLineGradient()
                
        for line_mov in self.m_line_mov_list:
            line_mov.setReadyToDisc(False)
   
    def resetLineMovPositions(self):
        for i in range(len(self.m_line_mov_list)):
            line_mov = self.m_line_mov_list[i]
            if i < self.getPortLength():
                line_mov.setDestinationPortGroupPosition(i, self.getPortLength())
            else:
                item = line_mov
                canvas.scene.removeItem(item)
                del item
        
        while len(self.m_line_mov_list) < self.getPortLength():
            if options.use_bezier_lines:
                line_mov = CanvasBezierLineMov(self.m_port_mode, self.m_port_type,
                                               len(self.m_line_mov_list), self.getPortLength(), self)
            else:
                line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type,
                            len(self.m_line_mov_list), self.getPortLength(), self)
            self.m_line_mov_list.append(line_mov)
        
        self.m_line_mov_list = self.m_line_mov_list[:self.getPortLength()]
                
    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.m_hover_item = None
            self.m_mouse_down = True
            self.m_cursor_moving = False
                        
        QGraphicsItem.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.m_mouse_down:
            if not self.m_cursor_moving:
                self.setCursor(QCursor(Qt.CrossCursor))
                self.m_cursor_moving = True

                for connection in canvas.connection_list:
                    if connection.port_out_id in self.m_port_id_list or connection.port_in_id in self.m_port_id_list:
                        connection.widget.setLocked(True)

            if not self.m_line_mov_list:
                self.m_r_click_conn = None
                canvas.last_z_value += 1
                self.setZValue(canvas.last_z_value)
                canvas.last_z_value += 1
                
                for port in canvas.port_list:
                    if (port.group_id == self.m_group_id
                            and port.port_id in self.m_port_id_list):
                        port.widget.setZValue(canvas.last_z_value)
                        
                for i in range(len(self.m_port_id_list)):
                    if options.use_bezier_lines:
                        line_mov  = CanvasBezierLineMov(self.m_port_mode, 
                            self.m_port_type, i, len(self.m_port_id_list), self)
                    else:
                        line_mov  = CanvasLineMov(self.m_port_mode, 
                            self.m_port_type, i, len(self.m_port_id_list), self)
                        
                    self.m_line_mov_list.append(line_mov)
                
                canvas.last_z_value += 1
                self.parentItem().setZValue(canvas.last_z_value)

            item = None
            items = canvas.scene.items(event.scenePos(), Qt.ContainsItemShape, Qt.AscendingOrder)
            for i in range(len(items)):
                if items[i].type() == CanvasPortType or items[i].type() == CanvasPortGroupType:
                    if items[i] != self:
                        if not item:
                            item = items[i]
                        elif items[i].parentItem().zValue() > item.parentItem().zValue():
                            item = items[i]

            if self.m_hover_item and self.m_hover_item != item:
                self.m_hover_item.setSelected(False)

            if item:
                if item.getPortMode() != self.m_port_mode and item.getPortType() == self.m_port_type:
                    item.setSelected(True)
                    if item.type() == CanvasPortType:
                        if self.m_hover_item != item:
                            self.m_hover_item = item
                            self.resetDotLines()
                            self.resetLineMovPositions()
                            for line_mov in self.m_line_mov_list:
                                line_mov.setDestinationPortGroupPosition(0, 1)
                            
                            self.m_dotcon_list = []
                            for port_id in self.m_port_id_list:
                                for connection in canvas.connection_list:
                                    if ( (connection.port_out_id == self.m_hover_item.getPortId() and connection.port_in_id == port_id) or
                                        (connection.port_out_id == port_id and connection.port_in_id == self.m_hover_item.getPortId()) ):
                                        self.m_dotcon_list.append(connection)
                                        
                            if len(self.m_dotcon_list) == len(self.m_port_id_list):
                                for connection in self.m_dotcon_list:
                                    connection.widget.setReadyToDisc(True)
                                    connection.widget.updateLineGradient()
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setReadyToDisc(True)
                                    
                    elif item.type() == CanvasPortGroupType:
                        if self.m_hover_item != item:
                            self.m_hover_item = item
                            self.resetDotLines()
                            self.resetLineMovPositions()
                            
                            if self.m_hover_item.getPortLength() <= len(self.m_line_mov_list):
                                for i in range(len(self.m_line_mov_list)):
                                    line_mov = self.m_line_mov_list[i]
                                    line_mov.setDestinationPortGroupPosition(
                                        i % self.m_hover_item.getPortLength(),
                                        self.m_hover_item.getPortLength())
                            else:
                                start_n_linemov = len(self.m_line_mov_list)
                                
                                for i in range(self.m_hover_item.getPortLength()):
                                    if i < start_n_linemov:
                                        line_mov = self.m_line_mov_list[i]
                                        line_mov.setDestinationPortGroupPosition(
                                            i, self.m_hover_item.getPortLength())
                                    else:
                                        port_posinportgrp = i % len(self.m_port_id_list)
                                        if options.use_bezier_lines:
                                            line_mov  = CanvasBezierLineMov(
                                                self.m_port_mode,
                                                self.m_port_type,
                                                port_posinportgrp,
                                                self.m_hover_item.getPortLength(),
                                                self)
                                        else:
                                            line_mov  = CanvasLineMov(
                                                self.m_port_mode, 
                                                self.m_port_type,
                                                port_posinportgrp,
                                                self.m_hover_item.getPortLength(),
                                                self)
                                            
                                        line_mov.setDestinationPortGroupPosition(
                                            i, self.m_hover_item.getPortLength())
                                        
                                        self.m_line_mov_list.append(line_mov)
                            
                            self.m_dotcon_list = []
                            symetric_con_list = []
                            for portself_id in self.m_port_id_list:
                                for porthover_id in self.m_hover_item.getPortsList():
                                    for connection in canvas.connection_list:
                                        if ( (connection.port_out_id == portself_id and connection.port_in_id == porthover_id) or
                                        (connection.port_out_id == porthover_id and connection.port_in_id == portself_id) ):
                                            if (self.m_port_id_list.index(portself_id) % len(self.m_hover_item.getPortsList())) == (self.m_hover_item.m_port_id_list.index(porthover_id) % len(self.m_port_id_list)):
                                                self.m_dotcon_list.append(connection)
                                                symetric_con_list.append(connection)
                                            else:
                                                self.m_dotcon_list.append(connection)
                                                connection.widget.setReadyToDisc(True)
                                                connection.widget.updateLineGradient()
                            
                            if len(self.m_port_id_list) >= len(self.m_hover_item.getPortsList()):
                                biggest_list = self.m_port_id_list
                            else: 
                                biggest_list = self.m_hover_item.getPortsList()
                                
                            if len(symetric_con_list) == len(biggest_list):
                                for connection in self.m_dotcon_list:
                                    connection.widget.setReadyToDisc(True)
                                    connection.widget.updateLineGradient()
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setReadyToDisc(True)
                        
                else:
                    self.m_hover_item = None
                    self.m_r_click_conn = None
                    self.resetDotLines()
                    self.resetLineMovPositions()
                    
            else:
                if self.m_hover_item:
                    self.m_hover_item = None
                    self.m_r_click_conn = None
                    self.resetDotLines()
                    self.resetLineMovPositions()
            
            for line_mov in self.m_line_mov_list:
                line_mov.updateLinePos(event.scenePos())
            return event.accept()

        QGraphicsItem.mouseMoveEvent(self, event)
        
    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            if self.m_mouse_down:
                
                for line_mov in self.m_line_mov_list:
                    item = line_mov
                    canvas.scene.removeItem(item)
                    del item
                self.m_line_mov_list.clear()
                
                for port in canvas.port_list:
                    port.widget.setOpacity(1)
                
                for portgrp in canvas.portgrp_list:
                    portgrp.widget.setOpacity(1)

                for connection in canvas.connection_list:
                    if connection.port_out_id in self.m_port_id_list or connection.port_in_id in self.m_port_id_list:
                        connection.widget.setLocked(False)
                if self.m_hover_item:  
                    #if self.m_r_click_conn != self.m_hover_item:
                        #if time.time() > self.m_r_click_time + 0.3:
                    self.ConnectToHover()
                        
                    canvas.scene.clearSelection()
                else:
                    if self.m_r_click_conn:
                        canvas.scene.clearSelection()

            if self.m_cursor_moving:
                self.setCursor(QCursor(Qt.ArrowCursor))

            self.m_hover_item = None
            self.m_mouse_down = False
            self.m_cursor_moving = False
        QGraphicsItem.mouseReleaseEvent(self, event)

        
    def contextMenuEvent(self, event):
        canvas.scene.clearSelection()
        self.setSelected(True)
        
        menu = QMenu()
        discMenu = QMenu("Disconnect", menu)
        
        #get all connected ports and all connected groups
        all_connected_ports_ids = []
        group_ids_list = []
        for selfport_id in self.m_port_id_list:
            con_list = CanvasGetPortConnectionList(self.m_group_id, selfport_id)
            for con_id, group_id, port_id in con_list:
                for port in canvas.port_list:
                    if port.port_id == port_id:
                        if not port.group_id in group_ids_list:
                            group_ids_list.append(port.group_id)
                                
                if not port_id in all_connected_ports_ids:
                    all_connected_ports_ids.append(port_id)
                    
        #Display 'no connection' action in disconnect menu
        if len(all_connected_ports_ids) == 0:
            action_noconnection = discMenu.addAction('no connection')
            action_noconnection.setEnabled(False)
            
        #check each group to get no trouble with ports in the same portgrp which could have non-consecutive port ids
        for group_id in group_ids_list:
            #get a list of connected port from this group
            gr_port_ids_list = []
            for port in canvas.port_list:
                if port.group_id == group_id:
                    if port.port_id in all_connected_ports_ids:
                        if not port.port_id in gr_port_ids_list:
                            gr_port_ids_list.append(port.port_id)
            
            portgrp_list_ids = []
            portgrp_dirty_connect_list = [] #is used to display individual ports of portgrp or not
            last_is_dirty_portgrp = False #is used to display menu separator or not
            
            #create action for each port of this group connected to the box
            for port in canvas.port_list:
                if port.port_id in gr_port_ids_list:
                    
                    #get port connection list
                    port_con_list = []
                    for connection in canvas.connection_list:
                        if ( (connection.port_out_id in self.m_port_id_list and connection.port_in_id == port.port_id) or
                            (connection.port_out_id == port.port_id and connection.port_in_id in self.m_port_id_list) ):
                            port_con_list.append(connection.connection_id)
                    
                    if port.portgrp_id:
                        port_alone_connected = False
                        
                        if not port.portgrp_id in portgrp_list_ids:
                            portgrp_list_ids.append(port.portgrp_id)
                            
                            for portgrp in canvas.portgrp_list:
                                if portgrp.portgrp_id == port.portgrp_id:
                                    
                                    #count ports from portgrp connected to the box
                                    c = 0
                                    for p in portgrp.port_id_list:
                                        if p in all_connected_ports_ids:
                                            c += 1
                                    
                                    if c == 1:
                                        port_alone_connected = True
                                    
                                    #Set disconnect port action if port is the only one of the portgrp connected to the box
                                    if port_alone_connected:
                                        if last_is_dirty_portgrp:
                                            discMenu.addSeparator()
                                            last_is_dirty_portgrp = False
                                            
                                        full_port_name = CanvasGetFullPortName(port.group_id, port.port_id)
                                        act_x_disc_port = discMenu.addAction(full_port_name)
                                        act_x_disc_port.setData(port_con_list)
                                        act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                                        
                                        
                                    else:
                                        #get portgrp connection list
                                        portgrp_con_list = []
                                        for connection in canvas.connection_list:
                                            if (connection.port_out_id in self.m_port_id_list and connection.port_in_id in portgrp.port_id_list or
                                                connection.port_out_id in portgrp.port_id_list and connection.port_in_id in self.m_port_id_list):
                                                portgrp_con_list.append(connection.connection_id)
                                                if connection.port_out_id in self.m_port_id_list:
                                                    if (self.m_port_id_list.index(connection.port_out_id) % len(portgrp.port_id_list) !=
                                                        portgrp.port_id_list.index(connection.port_in_id) % len(self.m_port_id_list) ):
                                                        portgrp_dirty_connect_list.append(portgrp.portgrp_id)
                                                else:
                                                    if (self.m_port_id_list.index(connection.port_in_id) % len(portgrp.port_id_list) !=
                                                        portgrp.port_id_list.index(connection.port_out_id) % len(self.m_port_id_list) ):
                                                        portgrp_dirty_connect_list.append(portgrp.portgrp_id)
                                                                                                              
                                        if portgrp.portgrp_id in portgrp_dirty_connect_list:
                                            discMenu.addSeparator()
                                            last_is_dirty_portgrp = True
                                        elif last_is_dirty_portgrp:
                                            discMenu.addSeparator()
                                            last_is_dirty_portgrp = False
                                            
                                        
                                        #set portgrp action in submenu
                                        portgrp_print_name = CanvasGetPortGroupFullName(
                                             portgrp.group_id, portgrp.portgrp_id)
                                        act_x_disc_portgrp = discMenu.addAction(portgrp_print_name)        
                                        act_x_disc_portgrp.setData(portgrp_con_list)
                                        act_x_disc_portgrp.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                                        
                                    
                        #set port action in submenu (if port is in a portgrp and isn't the only one connected to the box) 
                        if not port_alone_connected and port.portgrp_id in portgrp_dirty_connect_list:
                            port_print_name = CanvasGetPortPrintName(
                                port.group_id, port.port_id, port.portgrp_id)
                            act_x_disc_port = discMenu.addAction('  → ' + port_print_name)
                            act_x_disc_port.setData(port_con_list)
                            act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                        
                    else:
                        if last_is_dirty_portgrp:
                            discMenu.addSeparator()
                            last_is_dirty_portgrp = False
                            
                        #set port action in submenu (if port is not in a portgrp) 
                        full_port_name = CanvasGetFullPortName(port.group_id, port.port_id)
                        act_x_disc_port = discMenu.addAction(full_port_name)
                        act_x_disc_port.setData(port_con_list)
                        act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                
                
        menu.addMenu(discMenu)
        act_x_disc_alltheportgrp = menu.addAction("Disconnect &All")
        act_x_sep_1 = menu.addSeparator()
        act_x_setasmono = menu.addAction('Split to Monos')

        act_selected = menu.exec_(event.screenPos())
        
        if act_selected == act_x_disc_alltheportgrp:
            for connection in canvas.connection_list:
                if connection.port_out_id in self.m_port_id_list or connection.port_in_id in self.m_port_id_list:
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
            
        elif act_selected == act_x_setasmono:
            self.SplitToMonos()
            
        event.accept()
            
    def boundingRect(self):
        self.m_portgrp_width = self.getPortGroupWidth()
        if self.m_port_mode == PORT_MODE_INPUT:
            return QRectF(canvas.theme.port_in_portgrp_width, 0, self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width, canvas.theme.port_height * len(self.m_port_id_list))
        else:
            return QRectF(0, 0, self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width, canvas.theme.port_height * len(self.m_port_id_list))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))
        
        lineHinting = canvas.theme.port_audio_jack_pen.widthF() / 2

        poly_locx = [0, 0, 0, 0, 0]
        poly_corner_xhinting = (float(canvas.theme.port_height)/2) % floor(float(canvas.theme.port_height)/2)
        if poly_corner_xhinting == 0:
            poly_corner_xhinting = 0.5 * (1 - 7 / (float(canvas.theme.port_height)/2))
        
        if self.m_port_mode == PORT_MODE_INPUT:
            port_width = canvas.theme.port_in_portgrp_width
            
            for port in canvas.port_list:
                if port.port_id in self.m_port_id_list:
                    port_print_name = CanvasGetPortPrintName(port.group_id, 
                                        port.port_id, self.m_portgrp_id)
                    port_in_p_width = QFontMetrics(self.m_portgrp_font).width(port_print_name) + 3
                    port_width = max(port_width, port_in_p_width)

            text_pos = QPointF(port_width + 3, canvas.theme.port_text_ypos + (canvas.theme.port_height * (len(self.m_port_id_list) -1)/2))

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = canvas.theme.port_in_portgrp_width - lineHinting
                poly_locx[1] = self.m_portgrp_width + 5 + lineHinting
                poly_locx[2] = self.m_portgrp_width + 12 + lineHinting
                poly_locx[3] = self.m_portgrp_width + 5 + lineHinting
                poly_locx[4] = canvas.theme.port_in_portgrp_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = canvas.theme.port_in_portgrp_width - lineHinting
                poly_locx[1] = self.m_portgrp_width + 5 + lineHinting
                poly_locx[2] = self.m_portgrp_width + 5 + lineHinting
                poly_locx[3] = self.m_portgrp_width + 5 + lineHinting
                poly_locx[4] = canvas.theme.port_in_portgrp_width - lineHinting
            else:
                qCritical("PatchCanvas::CanvasPortGroup.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        elif self.m_port_mode == PORT_MODE_OUTPUT:
            text_pos = QPointF(9, canvas.theme.port_text_ypos + (canvas.theme.port_height * (len(self.m_port_id_list) -1)/2))

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width - lineHinting
                poly_locx[1] = 7 + lineHinting
                poly_locx[2] = 0 + lineHinting
                poly_locx[3] = 7 + lineHinting
                poly_locx[4] = self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width - lineHinting
                poly_locx[1] = 5 + lineHinting
                poly_locx[2] = 5 + lineHinting
                poly_locx[3] = 5 + lineHinting
                poly_locx[4] = self.m_portgrp_width + 12 - canvas.theme.port_in_portgrp_width - lineHinting
            else:
                qCritical("PatchCanvas::CanvasPortGroup.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        else:
            qCritical("PatchCanvas::CanvasPortGroup.paint() - invalid port mode '%s'" % port_mode2str(self.m_port_mode))
            return

        poly_pen = canvas.theme.portgrp_audio_jack_pen_sel  if self.isSelected() else canvas.theme.portgrp_audio_jack_pen
        text_pen = canvas.theme.port_audio_jack_text_sel if self.isSelected() else canvas.theme.port_audio_jack_text
        
        if self.m_port_mode == PORT_MODE_OUTPUT:
            real_portgrp_width = self.m_portgrp_width - canvas.theme.port_in_portgrp_width
            portgrp_gradient = QLinearGradient(real_portgrp_width -40, 0, real_portgrp_width, 0)
            portgrp_gradient.setColorAt(0, canvas.theme.port_audio_jack_bg_sel if self.isSelected() else canvas.theme.port_audio_jack_bg)
            portgrp_gradient.setColorAt(1, canvas.theme.portgrp_audio_jack_bg_sel if self.isSelected() else canvas.theme.portgrp_audio_jack_bg)
        else:
            portgrp_gradient = QLinearGradient(canvas.theme.port_in_portgrp_width, 0, canvas.theme.port_in_portgrp_width + 40, 0)
            portgrp_gradient.setColorAt(0, canvas.theme.portgrp_audio_jack_bg_sel if self.isSelected() else canvas.theme.portgrp_audio_jack_bg)
            portgrp_gradient.setColorAt(1, canvas.theme.port_audio_jack_bg_sel if self.isSelected() else canvas.theme.port_audio_jack_bg)
            
        
        polygon  = QPolygonF()
        polygon += QPointF(poly_locx[0], lineHinting)
        polygon += QPointF(poly_locx[1], lineHinting)
        polygon += QPointF(poly_locx[2], float(canvas.theme.port_height / 2) )
        polygon += QPointF(poly_locx[2], float(canvas.theme.port_height * (len(self.m_port_id_list) - 1/2)) )
        polygon += QPointF(poly_locx[3], canvas.theme.port_height * len(self.m_port_id_list) - lineHinting)
        polygon += QPointF(poly_locx[4], canvas.theme.port_height * len(self.m_port_id_list) - lineHinting)
            
        if canvas.theme.port_bg_pixmap:
            portRect = polygon.boundingRect()
            portPos  = portRect.topLeft()
            painter.drawTiledPixmap(portRect, canvas.theme.port_bg_pixmap, portPos)
        else:
            painter.setBrush(portgrp_gradient)

        painter.setPen(poly_pen)
        painter.drawPolygon(polygon)

        painter.setPen(text_pen)
        painter.setFont(self.m_portgrp_font)
        portgrp_name = CanvasGetPortGroupName(self.m_group_id,
                                                 self.m_port_id_list)
        painter.drawText(text_pos, portgrp_name)

        if self.isSelected() != self.m_last_selected_state:
            CanvasUpdateSelectedLines()
                    
        self.m_last_selected_state = self.isSelected()

        painter.restore()
 
