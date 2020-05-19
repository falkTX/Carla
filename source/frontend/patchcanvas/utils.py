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

    #break_loop = False
    while True:
        break_for = False
        for i, item in enumerate(items):
            if item and item.type() == CanvasBoxType:
                if item.sceneBoundingRect().adjusted(-5, -5, 5, 5).contains(new_pos):
                    itemRect = item.boundingRect()
                    if horizontal:
                        new_pos += QPointF(itemRect.width() + 50, 0)
                    else:
                        itemHeight = itemRect.height()
                        if itemHeight < 30:
                            new_pos += QPointF(0, itemHeight + 50)
                        else:
                            new_pos.setY(item.scenePos().y() + itemHeight + 20)
                    break_for = True
                    break
        else:
            if not break_for:
                break
            #break_loop = True

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
