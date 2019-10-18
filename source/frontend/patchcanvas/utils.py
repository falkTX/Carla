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

from PyQt5.QtCore import qCritical, QPointF, QTimer

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import bool2str, canvas, CanvasBoxType
from .canvasfadeanimation import CanvasFadeAnimation

# ------------------------------------------------------------------------------------------------------------

def CanvasGetNewGroupPos(horizontal):
    if canvas.debug:
        print("PatchCanvas::CanvasGetNewGroupPos(%s)" % bool2str(horizontal))

    new_pos = QPointF(canvas.initial_pos)
    items = canvas.scene.items()

    break_loop = False
    while not break_loop:
        break_for = False
        for i, item in enumerate(items):
            if item and item.type() == CanvasBoxType:
                if item.sceneBoundingRect().contains(new_pos):
                    if horizontal:
                        new_pos += QPointF(item.boundingRect().width() + 15, 0)
                    else:
                        new_pos += QPointF(0, item.boundingRect().height() + 15)
                    break_for = True
                    break

            if i >= len(items) - 1 and not break_for:
                break_loop = True

    return new_pos

def CanvasGetFullPortName(group_id, port_id):
    if canvas.debug:
        print("PatchCanvas::CanvasGetFullPortName(%i, %i)" % (group_id, port_id))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            group_id = port.group_id
            for group in canvas.group_list:
                if group.group_id == group_id:
                    return group.group_name + ":" + port.port_name
            break

    qCritical("PatchCanvas::CanvasGetFullPortName(%i, %i) - unable to find port" % (group_id, port_id))
    return ""

def CanvasGetPortConnectionList(group_id, port_id):
    if canvas.debug:
        print("PatchCanvas::CanvasGetPortConnectionList(%i, %i)" % (group_id, port_id))

    conn_list = []

    for connection in canvas.connection_list:
        if connection.group_out_id == group_id and connection.port_out_id == port_id:
            conn_list.append((connection.connection_id, connection.group_in_id, connection.port_in_id))
        elif connection.group_in_id == group_id and connection.port_in_id == port_id:
            conn_list.append((connection.connection_id, connection.group_out_id, connection.port_out_id))

    return conn_list

def CanvasGetPortGroupPosition(group_id, port_id, portgrp_id):
    if portgrp_id < 0:
        return (0, 1)
    
    #print('eokfc', group_id, port_id, portgrp_id)
    #for portgrp in canvas.portgrp_list:
        #print(portgrp.group_id, portgrp.portgrp_id, portgrp.port_id_list)
    
    for portgrp in canvas.portgrp_list:
        if (portgrp.group_id == group_id
                and portgrp.portgrp_id == portgrp_id):
            for i in range(len(portgrp.port_id_list)):
                if port_id == portgrp.port_id_list[i]:
                    return (i, len(portgrp.port_id_list))
    return (0, 1)

def CanvasGetPortGroupName(group_id, ports_ids_list):
    ports_names = []
    
    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id in ports_ids_list:
            ports_names.append(port.port_name)
    
    if len(ports_names) < 2:
        return ''
    
    portgrp_name_ends = ( ' ', '_', '.', '-', '#', ':', 'out', 'in', 'Out',
                            'In', 'Output', 'Input', 'output', 'input' )
    
    # set portgrp name
    portgrp_name = ''
    checkCharacter = True
    
    for c in ports_names[0]:        
        for eachname in ports_names:
            if not eachname.startswith(portgrp_name + c):
                checkCharacter = False
                break
        if not checkCharacter:
            break
        portgrp_name += c
    
    # reduce portgrp name until it ends with one of the characters
    # in portgrp_name_ends
    check = False
    while not check:
        for x in portgrp_name_ends:
            if portgrp_name.endswith(x):
                check = True
                break
        
        if len(portgrp_name) == 0 or portgrp_name in ports_names:
                check = True
            
        if not check:
            portgrp_name = portgrp_name[:-1]
    
    return portgrp_name

def CanvasGetPortPrintName(group_id, port_id, portgrp_id):
    for portgrp in canvas.portgrp_list:
        if (portgrp.group_id == group_id
                and portgrp.portgrp_id == portgrp_id):
            portgrp_name = CanvasGetPortGroupName(group_id,
                                                     portgrp.port_id_list)
            for port in canvas.port_list:
                if port.group_id == group_id and port.port_id == port_id:
                    return port.port_name.replace(portgrp_name, '', 1)

def CanvasGetPortGroupFullName(group_id, portgrp_id):
    for portgrp in canvas.portgrp_list:
        if (portgrp.group_id == group_id
                and portgrp.portgrp_id == portgrp_id):
            group_name = CanvasGetGroupName(portgrp.group_id)
            
            endofname = ''
            for port_id in portgrp.port_id_list:
                endofname += "%s/" % CanvasGetPortPrintName(group_id, port_id,
                                                     portgrp.portgrp_id)
            portgrp_name = CanvasGetPortGroupName(group_id, 
                                                     portgrp.port_id_list)
            
            return "%s:%s %s" % (group_name, portgrp_name, endofname[:-1])
    
    return ""

def CanvasAddPortToPortGroup(group_id, port_id, portgrp_id):
    # returns the portgrp_id if portgrp is active, else 0
    for portgrp in canvas.portgrp_list:
        if (portgrp.group_id == group_id
                and portgrp.portgrp_id == portgrp_id):
            if not port_id in portgrp.port_id_list:
                portgrp.port_id_list.append(port_id)
                
            if len(portgrp.port_id_list) >= 2:
                box_widget = None
                
                for port in canvas.port_list:
                    if (port.group_id == group_id
                            and port.port_id in portgrp.port_id_list):
                        port.portgrp_id = portgrp_id
                        if port.widget:
                            port.widget.setPortGroupId(portgrp_id)
                            box_widget = port.widget.parentItem()
                            
                if box_widget and portgrp.widget is None:
                    portgrp.widget = box_widget.addPortGroupFromGroup(
                            portgrp_id, portgrp.port_mode, 
                            portgrp.port_type, portgrp.port_id_list)
                    
                    # Put ports widget ahead the portgrp widget
                    for port in canvas.port_list:
                        if (port.group_id == group_id
                            and port.port_id in portgrp.port_id_list):
                                if port.widget:
                                    canvas.last_z_value += 1
                                    port.widget.setZValue(canvas.last_z_value)
            else:
                portgrp_id = 0
            
            return portgrp_id
    return 0
    
def CanvasRemovePortFromPortGroup(group_id, port_id, portgrp_id):
    for portgrp in canvas.portgrp_list:
        if (portgrp.group_id == group_id
            and portgrp.portgrp_id == portgrp_id):
            # clear the portgrp when removing one port from it
            for port in canvas.port_list:
                if (port.group_id == group_id
                        and port.port_id in portgrp.port_id_list):
                    port.portgrp_id = 0
                    if port.widget:
                        port.widget.setPortGroupId(0)
                
            item = portgrp.widget
            if item:
                canvas.scene.removeItem(item)
                del item
            portgrp.widget = None
            
            portgrp.port_id_list.clear()
            
            #if port_id in portgrp.port_id_list:
                #portgrp.port_id_list.remove(port_id)
                
            #if len(portgrp.port_id_list) < 2:
                #for port in canvas.port_list:
                    #if (port.group_id == group_id
                            #and port.port_id in portgrp.port_id_list):
                        #port.portgrp_id = 0
                        #if port.widget:
                            #port.widget.setPortGroupId(0)
                
                #item = portgrp.widget
                #if item:
                    #canvas.scene.removeItem(item)
                    #del item
                #portgrp.widget = None
            break

def CanvasUpdateSelectedLines():
    sel_con_list = []
    upper_line_z_value = 0
    
    for connection in canvas.connection_list:
        line_z_value = connection.widget.zValue()
        if int(line_z_value) != line_z_value:
            connection.widget.setZValue(int(line_z_value))
        if int(line_z_value) > upper_line_z_value:
            upper_line_z_value = int(line_z_value)
    
    for port in canvas.port_list:
        if port.widget.isSelected():
            for connection in canvas.connection_list:
                if connection.port_out_id == port.port_id or connection.port_in_id == port.port_id:
                    sel_con_list.append(connection)
    
    for portgrp in canvas.portgrp_list:
        if portgrp.widget.isSelected():
            for connection in canvas.connection_list:
                if connection.port_out_id in portgrp.port_id_list or connection.port_in_id in portgrp.port_id_list:
                    sel_con_list.append(connection)
                    
    for connection in canvas.connection_list:
        if connection in sel_con_list:
            connection.widget.setZValue(upper_line_z_value + 0.1)
            connection.widget.setLineSelected(True)
        else:
            connection.widget.setLineSelected(False)
    
    del sel_con_list

def CanvasCallback(action, value1, value2, value_str):
    if canvas.debug:
        print("PatchCanvas::CanvasCallback(%i, %i, %i, %s)" % (action, value1, value2, value_str.encode()))

    canvas.callback(action, value1, value2, value_str)

def CanvasItemFX(item, show, destroy):
    if canvas.debug:
        print("PatchCanvas::CanvasItemFX(%s, %s, %s)" % (item, bool2str(show), bool2str(destroy)))

    # Check if the item already has an animation
    for animation in canvas.animation_list:
        if animation.item() == item:
            animation.forceStop()
            canvas.animation_list.remove(animation)
            del animation
            break

    animation = CanvasFadeAnimation(item, show)
    animation.setDuration(750 if show else 500)

    if show:
        animation.finished.connect(canvas.qobject.AnimationFinishedShow)
    else:
        if destroy:
            animation.finished.connect(canvas.qobject.AnimationFinishedDestroy)
        else:
            animation.finished.connect(canvas.qobject.AnimationFinishedHide)

    canvas.animation_list.append(animation)

    animation.start()

def CanvasRemoveItemFX(item):
    if canvas.debug:
        print("PatchCanvas::CanvasRemoveItemFX(%s)" % item)

    if item.type() == CanvasBoxType:
        item.removeIconFromScene()

    canvas.scene.removeItem(item)
    del item

    QTimer.singleShot(0, canvas.scene.update)

# ------------------------------------------------------------------------------------------------------------
