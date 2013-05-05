#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas engine using QGraphicsView/Scene
# Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# TODO - apply last changes to c++ (2 commits + theme changes)

# Imports (Global)
from PyQt4.QtCore import pyqtSlot, qDebug, qCritical, qFatal, qWarning, Qt, QObject, SIGNAL, SLOT
from PyQt4.QtCore import QAbstractAnimation, QLineF, QPointF, QRectF, QSizeF, QSettings, QTimer
from PyQt4.QtGui import QColor, QLinearGradient, QPen, QPolygonF, QPainter, QPainterPath
from PyQt4.QtGui import QCursor, QFont, QFontMetrics, QInputDialog, QLineEdit, QMenu
from PyQt4.QtGui import QGraphicsScene, QGraphicsItem, QGraphicsLineItem, QGraphicsPathItem
from PyQt4.QtGui import QGraphicsColorizeEffect, QGraphicsDropShadowEffect
from PyQt4.QtSvg import QGraphicsSvgItem, QSvgRenderer

# Imports (Theme)
from patchcanvas_theme import *

# ------------------------------------------------------------------------------
# patchcanvas-api.h

# Port Mode
PORT_MODE_NULL   = 0
PORT_MODE_INPUT  = 1
PORT_MODE_OUTPUT = 2

# Port Type
PORT_TYPE_NULL       = 0
PORT_TYPE_AUDIO_JACK = 1
PORT_TYPE_MIDI_JACK  = 2
PORT_TYPE_MIDI_A2J   = 3
PORT_TYPE_MIDI_ALSA  = 4

# Callback Action
ACTION_GROUP_INFO       = 0 # group_id, N, N
ACTION_GROUP_RENAME     = 1 # group_id, N, new_name
ACTION_GROUP_SPLIT      = 2 # group_id, N, N
ACTION_GROUP_JOIN       = 3 # group_id, N, N
ACTION_PORT_INFO        = 4 # port_id, N, N
ACTION_PORT_RENAME      = 5 # port_id, N, new_name
ACTION_PORTS_CONNECT    = 6 # out_id, in_id, N
ACTION_PORTS_DISCONNECT = 7 # conn_id, N, N

# Icon
ICON_HARDWARE    = 0
ICON_APPLICATION = 1
ICON_LADISH_ROOM = 2

# Split Option
SPLIT_UNDEF = 0
SPLIT_NO    = 1
SPLIT_YES   = 2

# Antialiasing Option
ANTIALIASING_NONE  = 0
ANTIALIASING_SMALL = 1
ANTIALIASING_FULL  = 2

# Eye-Candy Option
EYECANDY_NONE  = 0
EYECANDY_SMALL = 1
EYECANDY_FULL  = 2

# Canvas options
class options_t(object):
    __slots__ = [
        'theme_name',
        'auto_hide_groups',
        'use_bezier_lines',
        'antialiasing',
        'eyecandy'
    ]

# Canvas features
class features_t(object):
    __slots__ = [
        'group_info',
        'group_rename',
        'port_info',
        'port_rename',
        'handle_group_pos'
    ]

# ------------------------------------------------------------------------------
# patchcanvas.h

# object types
CanvasBoxType           = QGraphicsItem.UserType + 1
CanvasIconType          = QGraphicsItem.UserType + 2
CanvasPortType          = QGraphicsItem.UserType + 3
CanvasLineType          = QGraphicsItem.UserType + 4
CanvasBezierLineType    = QGraphicsItem.UserType + 5
CanvasLineMovType       = QGraphicsItem.UserType + 6
CanvasBezierLineMovType = QGraphicsItem.UserType + 7

# object lists
class group_dict_t(object):
    __slots__ = [
        'group_id',
        'group_name',
        'split',
        'icon',
        'widgets'
    ]

class port_dict_t(object):
    __slots__ = [
        'group_id',
        'port_id',
        'port_name',
        'port_mode',
        'port_type',
        'widget'
    ]

class connection_dict_t(object):
    __slots__ = [
        'connection_id',
        'port_in_id',
        'port_out_id',
        'widget'
    ]

class animation_dict_t(object):
    __slots__ = [
        'animation',
        'item'
    ]

# Main Canvas object
class Canvas(object):
    __slots__ = [
        'scene',
        'callback',
        'debug',
        'last_z_value',
        'last_connection_id',
        'initial_pos',
        'size_rect',
        'group_list',
        'port_list',
        'connection_list',
        'animation_list',
        'qobject',
        'settings',
        'theme',
        'initiated'
    ]

# ------------------------------------------------------------------------------
# patchcanvas.cpp

class CanvasObject(QObject):
    def __init__(self, parent=None):
        QObject.__init__(self, parent)

    @pyqtSlot()
    def AnimationIdle(self):
        animation = self.sender()
        if animation:
            CanvasRemoveAnimation(animation)

    @pyqtSlot()
    def AnimationHide(self):
        animation = self.sender()
        if animation:
            item = animation.item()
            if item:
                item.hide()
            CanvasRemoveAnimation(animation)

    @pyqtSlot()
    def AnimationDestroy(self):
        animation = self.sender()
        if animation:
            item = animation.item()
            if item:
                CanvasRemoveItemFX(item)
            CanvasRemoveAnimation(animation)

    @pyqtSlot()
    def CanvasPostponedGroups(self):
        CanvasPostponedGroups()

    @pyqtSlot()
    def PortContextMenuDisconnect(self):
        try:
            connectionId = int(self.sender().data())
        except:
            return

        CanvasCallback(ACTION_PORTS_DISCONNECT, connectionId, 0, "")

# Global objects
canvas = Canvas()
canvas.qobject    = None
canvas.settings   = None
canvas.theme      = None
canvas.initiated  = False
canvas.group_list = []
canvas.port_list  = []
canvas.connection_list = []
canvas.animation_list  = []

options = options_t()
options.theme_name = getDefaultThemeName()
options.auto_hide_groups = False
options.use_bezier_lines = True
options.antialiasing = ANTIALIASING_SMALL
options.eyecandy     = EYECANDY_SMALL

features = features_t()
features.group_info   = False
features.group_rename = False
features.port_info    = False
features.port_rename  = False
features.handle_group_pos = False

# Internal functions
def bool2str(check):
    return "True" if check else "False"

def port_mode2str(port_mode):
    if port_mode == PORT_MODE_NULL:
        return "PORT_MODE_NULL"
    elif port_mode == PORT_MODE_INPUT:
        return "PORT_MODE_INPUT"
    elif port_mode == PORT_MODE_OUTPUT:
        return "PORT_MODE_OUTPUT"
    else:
        return "PORT_MODE_???"

def port_type2str(port_type):
    if port_type == PORT_TYPE_NULL:
        return "PORT_TYPE_NULL"
    elif port_type == PORT_TYPE_AUDIO_JACK:
        return "PORT_TYPE_AUDIO_JACK"
    elif port_type == PORT_TYPE_MIDI_JACK:
        return "PORT_TYPE_MIDI_JACK"
    elif port_type == PORT_TYPE_MIDI_A2J:
        return "PORT_TYPE_MIDI_A2J"
    elif port_type == PORT_TYPE_MIDI_ALSA:
        return "PORT_TYPE_MIDI_ALSA"
    else:
        return "PORT_TYPE_???"

def icon2str(icon):
    if icon == ICON_HARDWARE:
        return "ICON_HARDWARE"
    elif ICON_APPLICATION:
        return "ICON_APPLICATION"
    elif ICON_LADISH_ROOM:
        return "ICON_LADISH_ROOM"
    else:
        return "ICON_???"

def split2str(split):
    if split == SPLIT_UNDEF:
        return "SPLIT_UNDEF"
    elif split == SPLIT_NO:
        return "SPLIT_NO"
    elif split == SPLIT_YES:
        return "SPLIT_YES"
    else:
        return "SPLIT_???"

# PatchCanvas API
def setOptions(new_options):
    if canvas.initiated: return
    options.theme_name       = new_options.theme_name
    options.auto_hide_groups = new_options.auto_hide_groups
    options.use_bezier_lines = new_options.use_bezier_lines
    options.antialiasing = new_options.antialiasing
    options.eyecandy     = new_options.eyecandy

def setFeatures(new_features):
    if canvas.initiated: return
    features.group_info   = new_features.group_info
    features.group_rename = new_features.group_rename
    features.port_info    = new_features.port_info
    features.port_rename  = new_features.port_rename
    features.handle_group_pos = new_features.handle_group_pos

def init(appName, scene, callback, debug=False):
    if debug:
        qDebug("PatchCanvas::init(\"%s\", %s, %s, %s)" % (appName, scene, callback, bool2str(debug)))

    if canvas.initiated:
        qCritical("PatchCanvas::init() - already initiated")
        return

    if not callback:
        qFatal("PatchCanvas::init() - fatal error: callback not set")
        return

    canvas.scene    = scene
    canvas.callback = callback
    canvas.debug    = debug

    canvas.last_z_value = 0
    canvas.last_connection_id = 0
    canvas.initial_pos = QPointF(0, 0)
    canvas.size_rect = QRectF()

    if not canvas.qobject:  canvas.qobject = CanvasObject()
    if not canvas.settings: canvas.settings = QSettings("Cadence", appName)

    if canvas.theme:
        del canvas.theme
        canvas.theme = None

    for i in range(Theme.THEME_MAX):
        this_theme_name = getThemeName(i)
        if this_theme_name == options.theme_name:
            canvas.theme = Theme(i)
            break

    if not canvas.theme:
        canvas.theme = Theme(getDefaultTheme())

    canvas.scene.updateTheme()

    canvas.initiated = True

def clear():
    if canvas.debug:
        qDebug("PatchCanvas::clear()")

    group_list_ids = []
    port_list_ids  = []
    connection_list_ids = []

    for group in canvas.group_list:
        group_list_ids.append(group.group_id)

    for port in canvas.port_list:
        port_list_ids.append(port.port_id)

    for connection in canvas.connection_list:
        connection_list_ids.append(connection.connection_id)

    for idx in connection_list_ids:
        disconnectPorts(idx)

    for idx in port_list_ids:
        removePort(idx)

    for idx in group_list_ids:
        removeGroup(idx)

    canvas.last_z_value = 0
    canvas.last_connection_id = 0

    canvas.group_list = []
    canvas.port_list = []
    canvas.connection_list = []

    canvas.initiated = False

def setInitialPos(x, y):
    if canvas.debug:
        qDebug("PatchCanvas::setInitialPos(%i, %i)" % (x, y))

    canvas.initial_pos.setX(x)
    canvas.initial_pos.setY(y)

def setCanvasSize(x, y, width, height):
    if canvas.debug:
        qDebug("PatchCanvas::setCanvasSize(%i, %i, %i, %i)" % (x, y, width, height))

    canvas.size_rect.setX(x)
    canvas.size_rect.setY(y)
    canvas.size_rect.setWidth(width)
    canvas.size_rect.setHeight(height)

def addGroup(group_id, group_name, split=SPLIT_UNDEF, icon=ICON_APPLICATION):
    if canvas.debug:
        qDebug("PatchCanvas::addGroup(%i, %s, %s, %s)" % (group_id, group_name.encode(), split2str(split), icon2str(icon)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            qWarning("PatchCanvas::addGroup(%i, %s, %s, %s) - group already exists" % (group_id, group_name.encode(), split2str(split), icon2str(icon)))
            return

    if split == SPLIT_UNDEF and features.handle_group_pos:
        split = canvas.settings.value("CanvasPositions/%s_SPLIT" % group_name, split, type=int)

    group_box = CanvasBox(group_id, group_name, icon)

    group_dict = group_dict_t()
    group_dict.group_id = group_id
    group_dict.group_name = group_name
    group_dict.split = bool(split == SPLIT_YES)
    group_dict.icon = icon
    group_dict.widgets = [group_box, None]

    if split == SPLIT_YES:
        group_box.setSplit(True, PORT_MODE_OUTPUT)

        if features.handle_group_pos:
            group_box.setPos(canvas.settings.value("CanvasPositions/%s_OUTPUT" % group_name, CanvasGetNewGroupPos(), type=QPointF))
        else:
            group_box.setPos(CanvasGetNewGroupPos())

        group_sbox = CanvasBox(group_id, group_name, icon)
        group_sbox.setSplit(True, PORT_MODE_INPUT)

        group_dict.widgets[1] = group_sbox

        if features.handle_group_pos:
            group_sbox.setPos(canvas.settings.value("CanvasPositions/%s_INPUT" % group_name, CanvasGetNewGroupPos(True), type=QPointF))
        else:
            group_sbox.setPos(CanvasGetNewGroupPos(True))

        canvas.last_z_value += 1
        group_sbox.setZValue(canvas.last_z_value)

        if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
            CanvasItemFX(group_sbox, True)

    else:
        group_box.setSplit(False)

        if features.handle_group_pos:
            group_box.setPos(canvas.settings.value("CanvasPositions/%s" % group_name, CanvasGetNewGroupPos(), type=QPointF))
        else:
            # Special ladish fake-split groups
            horizontal = bool(icon == ICON_HARDWARE or icon == ICON_LADISH_ROOM)
            group_box.setPos(CanvasGetNewGroupPos(horizontal))

    group_box.checkItemPos()

    canvas.last_z_value += 1
    group_box.setZValue(canvas.last_z_value)

    canvas.group_list.append(group_dict)

    if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
        CanvasItemFX(group_box, True)

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def removeGroup(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::removeGroup(%i)" % group_id)

    for group in canvas.group_list:
        if group.group_id == group_id:
            item = group.widgets[0]
            group_name = group.group_name

            if group.split:
                s_item = group.widgets[1]

                if features.handle_group_pos:
                    canvas.settings.setValue("CanvasPositions/%s_OUTPUT" % group_name, item.pos())
                    canvas.settings.setValue("CanvasPositions/%s_INPUT" % group_name, s_item.pos())
                    canvas.settings.setValue("CanvasPositions/%s_SPLIT" % group_name, SPLIT_YES)

                if options.eyecandy == EYECANDY_FULL:
                    CanvasItemFX(s_item, False, True)
                else:
                    s_item.removeIconFromScene()
                    canvas.scene.removeItem(s_item)
                    del s_item

            else:
                if features.handle_group_pos:
                    canvas.settings.setValue("CanvasPositions/%s" % group_name, item.pos())
                    canvas.settings.setValue("CanvasPositions/%s_SPLIT" % group_name, SPLIT_NO)

            if options.eyecandy == EYECANDY_FULL:
                CanvasItemFX(item, False, True)
            else:
                item.removeIconFromScene()
                canvas.scene.removeItem(item)
                del item

            canvas.group_list.remove(group)

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::removeGroup(%i) - unable to find group to remove" % group_id)

def renameGroup(group_id, new_group_name):
    if canvas.debug:
        qDebug("PatchCanvas::renameGroup(%i, %s)" % (group_id, new_group_name.encode()))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.group_name = new_group_name
            group.widgets[0].setGroupName(new_group_name)

            if group.split and group.widgets[1]:
                group.widgets[1].setGroupName(new_group_name)

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::renameGroup(%i, %s) - unable to find group to rename" % (group_id, new_group_name.encode()))

def splitGroup(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::splitGroup(%i)" % group_id)

    item = None
    group_name = ""
    group_icon = ICON_APPLICATION
    ports_data = []
    conns_data = []

    # Step 1 - Store all Item data
    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split:
                qCritical("PatchCanvas::splitGroup(%i) - group is already splitted" % group_id)
                return

            item = group.widgets[0]
            group_name = group.group_name
            group_icon = group.icon
            break

    if not item:
        qCritical("PatchCanvas::splitGroup(%i) - unable to find group to split" % group_id)
        return

    port_list_ids = list(item.getPortList())

    for port in canvas.port_list:
        if port.port_id in port_list_ids:
            port_dict = port_dict_t()
            port_dict.group_id = port.group_id
            port_dict.port_id = port.port_id
            port_dict.port_name = port.port_name
            port_dict.port_mode = port.port_mode
            port_dict.port_type = port.port_type
            port_dict.widget = None
            ports_data.append(port_dict)

    for connection in canvas.connection_list:
        if connection.port_out_id in port_list_ids or connection.port_in_id in port_list_ids:
            connection_dict = connection_dict_t()
            connection_dict.connection_id = connection.connection_id
            connection_dict.port_in_id = connection.port_in_id
            connection_dict.port_out_id = connection.port_out_id
            connection_dict.widget = None
            conns_data.append(connection_dict)

    # Step 2 - Remove Item and Children
    for conn in conns_data:
        disconnectPorts(conn.connection_id)

    for port_id in port_list_ids:
        removePort(port_id)

    removeGroup(group_id)

    # Step 3 - Re-create Item, now splitted
    addGroup(group_id, group_name, SPLIT_YES, group_icon)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.port_out_id, conn.port_in_id)

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def joinGroup(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::joinGroup(%i)" % group_id)

    item   = None
    s_item = None
    group_name = ""
    group_icon = ICON_APPLICATION
    ports_data = []
    conns_data = []

    # Step 1 - Store all Item data
    for group in canvas.group_list:
        if group.group_id == group_id:
            if not group.split:
                qCritical("PatchCanvas::joinGroup(%i) - group is not splitted" % group_id)
                return

            item = group.widgets[0]
            s_item = group.widgets[1]
            group_name = group.group_name
            group_icon = group.icon
            break

    # FIXME
    if not (item and s_item):
        qCritical("PatchCanvas::joinGroup(%i) - unable to find groups to join" % group_id)
        return

    port_list_ids = list(item.getPortList())
    port_list_idss = s_item.getPortList()

    for port_id in port_list_idss:
        if port_id not in port_list_ids:
            port_list_ids.append(port_id)

    for port in canvas.port_list:
        if port.port_id in port_list_ids:
            port_dict = port_dict_t()
            port_dict.group_id = port.group_id
            port_dict.port_id = port.port_id
            port_dict.port_name = port.port_name
            port_dict.port_mode = port.port_mode
            port_dict.port_type = port.port_type
            port_dict.widget = None
            ports_data.append(port_dict)

    for connection in canvas.connection_list:
        if connection.port_out_id in port_list_ids or connection.port_in_id in port_list_ids:
            connection_dict = connection_dict_t()
            connection_dict.connection_id = connection.connection_id
            connection_dict.port_in_id = connection.port_in_id
            connection_dict.port_out_id = connection.port_out_id
            connection_dict.widget = None
            conns_data.append(connection_dict)

    # Step 2 - Remove Item and Children
    for conn in conns_data:
        disconnectPorts(conn.connection_id)

    for port_id in port_list_ids:
        removePort(port_id)

    removeGroup(group_id)

    # Step 3 - Re-create Item, now together
    addGroup(group_id, group_name, SPLIT_NO, group_icon)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.port_out_id, conn.port_in_id)

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def getGroupPos(group_id, port_mode=PORT_MODE_OUTPUT):
    if canvas.debug:
        qDebug("PatchCanvas::getGroupPos(%i, %s)" % (group_id, port_mode2str(port_mode)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split:
                if port_mode == PORT_MODE_OUTPUT:
                    return group.widgets[0].pos()
                elif port_mode == PORT_MODE_INPUT:
                    return group.widgets[1].pos()
                else:
                    return QPointF(0, 0)
            else:
                return group.widgets[0].pos()

    qCritical("PatchCanvas::getGroupPos(%i, %s) - unable to find group" % (group_id, port_mode2str(port_mode)))
    return QPointF(0, 0)

def setGroupPos(group_id, group_pos_x, group_pos_y):
    setGroupPosFull(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y)

def setGroupPosFull(group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i):
    if canvas.debug:
        qDebug("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)" % (group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.widgets[0].setPos(group_pos_x_o, group_pos_y_o)

            if group.split and group.widgets[1]:
                group.widgets[1].setPos(group_pos_x_i, group_pos_y_i)

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i) - unable to find group to reposition" % (group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

def setGroupIcon(group_id, icon):
    if canvas.debug:
        qDebug("PatchCanvas::setGroupIcon(%i, %s)" % (group_id, icon2str(icon)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.icon = icon
            group.widgets[0].setIcon(icon)

            if group.split and group.widgets[1]:
                group.widgets[1].setIcon(icon)

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::setGroupIcon(%i, %s) - unable to find group to change icon" % (group_id, icon2str(icon)))

def addPort(group_id, port_id, port_name, port_mode, port_type):
    if canvas.debug:
        qDebug("PatchCanvas::addPort(%i, %i, %s, %s, %s)" % (group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            qWarning("PatchCanvas::addPort(%i, %i, %s, %s, %s) - port already exists" % (group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))
            return

    box_widget  = None
    port_widget = None

    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split and group.widgets[0].getSplittedMode() != port_mode and group.widgets[1]:
                n = 1
            else:
                n = 0
            box_widget  = group.widgets[n]
            port_widget = box_widget.addPortFromGroup(port_id, port_mode, port_type, port_name)
            break

    if not (box_widget and port_widget):
        qCritical("PatchCanvas::addPort(%i, %i, %s, %s, %s) - Unable to find parent group" % (group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))
        return

    if options.eyecandy == EYECANDY_FULL:
        CanvasItemFX(port_widget, True)

    port_dict = port_dict_t()
    port_dict.group_id  = group_id
    port_dict.port_id   = port_id
    port_dict.port_name = port_name
    port_dict.port_mode = port_mode
    port_dict.port_type = port_type
    port_dict.widget = port_widget
    canvas.port_list.append(port_dict)

    box_widget.updatePositions()

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def removePort(port_id):
    if canvas.debug:
        qDebug("PatchCanvas::removePort(%i)" % port_id)

    for port in canvas.port_list:
        if port.port_id == port_id:
            item = port.widget
            item.parentItem().removePortFromGroup(port_id)
            canvas.scene.removeItem(item)
            del item

            canvas.port_list.remove(port)

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::removePort(%i) - Unable to find port to remove" % port_id)

def renamePort(port_id, new_port_name):
    if canvas.debug:
        qDebug("PatchCanvas::renamePort(%i, %s)" % (port_id, new_port_name.encode()))

    for port in canvas.port_list:
        if port.port_id == port_id:
            port.port_name = new_port_name
            port.widget.setPortName(new_port_name)
            port.widget.parentItem().updatePositions()

            QTimer.singleShot(0, canvas.scene, SLOT("update()"))
            return

    qCritical("PatchCanvas::renamePort(%i, %s) - Unable to find port to rename" % (port_id, new_port_name.encode()))

def connectPorts(connection_id, port_out_id, port_in_id):
    if canvas.debug:
        qDebug("PatchCanvas::connectPorts(%i, %i, %i)" % (connection_id, port_out_id, port_in_id))

    port_out = None
    port_in  = None
    port_out_parent = None
    port_in_parent  = None

    for port in canvas.port_list:
        if port.port_id == port_out_id:
            port_out = port.widget
            port_out_parent = port_out.parentItem()
        elif port.port_id == port_in_id:
            port_in = port.widget
            port_in_parent = port_in.parentItem()

    # FIXME
    if not (port_out and port_in):
        qCritical("PatchCanvas::connectPorts(%i, %i, %i) - unable to find ports to connect" % (connection_id, port_out_id, port_in_id))
        return

    connection_dict = connection_dict_t()
    connection_dict.connection_id = connection_id
    connection_dict.port_out_id = port_out_id
    connection_dict.port_in_id = port_in_id

    if options.use_bezier_lines:
        connection_dict.widget = CanvasBezierLine(port_out, port_in, None)
    else:
        connection_dict.widget = CanvasLine(port_out, port_in, None)

    port_out_parent.addLineFromGroup(connection_dict.widget, connection_id)
    port_in_parent.addLineFromGroup(connection_dict.widget, connection_id)

    canvas.last_z_value += 1
    port_out_parent.setZValue(canvas.last_z_value)
    port_in_parent.setZValue(canvas.last_z_value)

    canvas.last_z_value += 1
    connection_dict.widget.setZValue(canvas.last_z_value)

    canvas.connection_list.append(connection_dict)

    if options.eyecandy == EYECANDY_FULL:
        item = connection_dict.widget
        CanvasItemFX(item, True)

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def disconnectPorts(connection_id):
    if canvas.debug:
        qDebug("PatchCanvas::disconnectPorts(%i)" % connection_id)

    port_1_id = port_2_id = 0
    line = None
    item1 = None
    item2 = None

    for connection in canvas.connection_list:
        if connection.connection_id == connection_id:
            port_1_id = connection.port_out_id
            port_2_id = connection.port_in_id
            line = connection.widget
            canvas.connection_list.remove(connection)
            break

    if not line:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find connection ports" % connection_id)
        return

    for port in canvas.port_list:
        if port.port_id == port_1_id:
            item1 = port.widget
            break

    if not item1:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find output port" % connection_id)
        return

    for port in canvas.port_list:
        if port.port_id == port_2_id:
            item2 = port.widget
            break

    if not item2:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find input port" % connection_id)
        return

    item1.parentItem().removeLineFromGroup(connection_id)
    item2.parentItem().removeLineFromGroup(connection_id)

    if options.eyecandy == EYECANDY_FULL:
        CanvasItemFX(line, False, True)
    else:
        line.deleteFromScene()

    QTimer.singleShot(0, canvas.scene, SLOT("update()"))

def arrange():
    if canvas.debug:
        qDebug("PatchCanvas::arrange()")

def updateZValues():
    if canvas.debug:
        qDebug("PatchCanvas::updateZValues()")

    for group in canvas.group_list:
        group.widgets[0].resetLinesZValue()

        if group.split and group.widgets[1]:
            group.widgets[1].resetLinesZValue()

# Extra Internal functions

def CanvasGetGroupName(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetGroupName(%i)" % group_id)

    for group in canvas.group_list:
        if group.group_id == group_id:
            return group.group_name

    qCritical("PatchCanvas::CanvasGetGroupName(%i) - unable to find group" % group_id)
    return ""

def CanvasGetGroupPortCount(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetGroupPortCount(%i)" % group_id)

    port_count = 0
    for port in canvas.port_list:
        if port.group_id == group_id:
            port_count += 1

    return port_count

def CanvasGetNewGroupPos(horizontal=False):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetNewGroupPos(%s)" % bool2str(horizontal))

    new_pos = QPointF(canvas.initial_pos.x(), canvas.initial_pos.y())
    items = canvas.scene.items()

    break_loop = False
    while not break_loop:
        break_for = False
        for i in range(len(items)):
            item = items[i]
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

def CanvasGetFullPortName(port_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetFullPortName(%i)" % port_id)

    for port in canvas.port_list:
        if port.port_id == port_id:
            group_id = port.group_id
            for group in canvas.group_list:
                if group.group_id == group_id:
                    return group.group_name + ":" + port.port_name
            break

    qCritical("PatchCanvas::CanvasGetFullPortName(%i) - unable to find port" % port_id)
    return ""

def CanvasGetPortConnectionList(port_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetPortConnectionList(%i)" % port_id)

    port_con_list = []

    for connection in canvas.connection_list:
        if connection.port_out_id == port_id or connection.port_in_id == port_id:
            port_con_list.append(connection.connection_id)

    return port_con_list

def CanvasGetConnectedPort(connection_id, port_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetConnectedPort(%i, %i)" % (connection_id, port_id))

    for connection in canvas.connection_list:
        if connection.connection_id == connection_id:
            if connection.port_out_id == port_id:
                return connection.port_in_id
            else:
                return connection.port_out_id

    qCritical("PatchCanvas::CanvasGetConnectedPort(%i, %i) - unable to find connection" % (connection_id, port_id))
    return 0

def CanvasRemoveAnimation(f_animation):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasRemoveAnimation(%s)" % f_animation)

    for animation in canvas.animation_list:
        if animation.animation == f_animation:
            canvas.animation_list.remove(animation)
            del animation.animation
            break

def CanvasPostponedGroups():
    if canvas.debug:
        qDebug("PatchCanvas::CanvasPostponedGroups()")

def CanvasCallback(action, value1, value2, value_str):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasCallback(%i, %i, %i, %s)" % (action, value1, value2, value_str.encode()))

    canvas.callback(action, value1, value2, value_str)

def CanvasItemFX(item, show, destroy=False):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasItemFX(%s, %s, %s)" % (item, bool2str(show), bool2str(destroy)))

    # Check if item already has an animationItemFX
    for animation in canvas.animation_list:
        if animation.item == item:
            animation.animation.stop()
            canvas.animation_list.remove(animation)
            del animation.animation
            break

    animation = CanvasFadeAnimation(item, show)
    animation.setDuration(750 if show else 500)

    animation_dict = animation_dict_t()
    animation_dict.animation = animation
    animation_dict.item = item
    canvas.animation_list.append(animation_dict)

    if show:
        canvas.qobject.connect(animation, SIGNAL("finished()"), SLOT("AnimationIdle()"))
    else:
        if destroy:
            canvas.qobject.connect(animation, SIGNAL("finished()"), SLOT("AnimationDestroy()"))
        else:
            canvas.qobject.connect(animation, SIGNAL("finished()"), SLOT("AnimationHide()"))

    animation.start()

def CanvasRemoveItemFX(item):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasRemoveItemFX(%s)" % item)

    if item.type() == CanvasBoxType:
        item.removeIconFromScene()
        canvas.scene.removeItem(item)

    elif item.type() == CanvasPortType:
        canvas.scene.removeItem(item)

    elif item.type() in (CanvasLineType, CanvasBezierLineType):
        pass #item.deleteFromScene()

    # Force deletion of item if needed
    if item.type() in (CanvasBoxType, CanvasPortType):
        del item

# ------------------------------------------------------------------------------
# patchscene.cpp

class PatchScene(QGraphicsScene):
    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_ctrl_down = False
        self.m_mouse_down_init = False
        self.m_mouse_rubberband = False

        self.m_rubberband = self.addRect(QRectF(0, 0, 0, 0))
        self.m_rubberband.setZValue(-1)
        self.m_rubberband.hide()
        self.m_rubberband_selection = False
        self.m_rubberband_orig_point = QPointF(0, 0)

        self.m_view = view
        if not self.m_view:
            qFatal("PatchCanvas::PatchScene() - invalid view")

    def fixScaleFactor(self):
        scale = self.m_view.transform().m11()
        if scale > 3.0:
            self.m_view.resetTransform()
            self.m_view.scale(3.0, 3.0)
        elif scale < 0.2:
            self.m_view.resetTransform()
            self.m_view.scale(0.2, 0.2)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def updateTheme(self):
        self.setBackgroundBrush(canvas.theme.canvas_bg)
        self.m_rubberband.setPen(canvas.theme.rubberband_pen)
        self.m_rubberband.setBrush(canvas.theme.rubberband_brush)

    def zoom_fit(self):
        min_x = min_y = max_x = max_y = None
        first_value = True

        items_list = self.items()

        if len(items_list) > 0:
            for item in items_list:
                if item and item.isVisible() and item.type() == CanvasBoxType:
                    pos = item.scenePos()
                    rect = item.boundingRect()

                    if first_value:
                        min_x = pos.x()
                    elif pos.x() < min_x:
                        min_x = pos.x()

                    if first_value:
                        min_y = pos.y()
                    elif pos.y() < min_y:
                        min_y = pos.y()

                    if first_value:
                        max_x = pos.x() + rect.width()
                    elif pos.x() + rect.width() > max_x:
                        max_x = pos.x() + rect.width()

                    if first_value:
                        max_y = pos.y() + rect.height()
                    elif pos.y() + rect.height() > max_y:
                        max_y = pos.y() + rect.height()

                    first_value = False

            if not first_value:
                self.m_view.fitInView(min_x, min_y, abs(max_x - min_x), abs(max_y - min_y), Qt.KeepAspectRatio)
                self.fixScaleFactor()

    def zoom_in(self):
        if self.m_view.transform().m11() < 3.0:
            self.m_view.scale(1.2, 1.2)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def zoom_out(self):
        if self.m_view.transform().m11() > 0.2:
            self.m_view.scale(0.8, 0.8)
        self.emit(SIGNAL("scaleChanged(double)"), self.m_view.transform().m11())

    def zoom_reset(self):
        self.m_view.resetTransform()
        self.emit(SIGNAL("scaleChanged(double)"), 1.0)

    def keyPressEvent(self, event):
        if not self.m_view:
            return event.ignore()

        if event.key() == Qt.Key_Control:
            self.m_ctrl_down = True

        elif event.key() == Qt.Key_Home:
            self.zoom_fit()
            return event.accept()

        elif self.m_ctrl_down:
            if event.key() == Qt.Key_Plus:
                self.zoom_in()
                return event.accept()
            elif event.key() == Qt.Key_Minus:
                self.zoom_out()
                return event.accept()
            elif event.key() == Qt.Key_1:
                self.zoom_reset()
                return event.accept()

        QGraphicsScene.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key_Control:
            self.m_ctrl_down = False
        QGraphicsScene.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        self.m_mouse_down_init  = bool(event.button() == Qt.LeftButton)
        self.m_mouse_rubberband = False
        QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.m_mouse_down_init:
            self.m_mouse_down_init  = False
            self.m_mouse_rubberband = bool(len(self.selectedItems()) == 0)

        if self.m_mouse_rubberband:
            if not self.m_rubberband_selection:
                self.m_rubberband.show()
                self.m_rubberband_selection = True
                self.m_rubberband_orig_point = event.scenePos()

            pos = event.scenePos()

            if pos.x() > self.m_rubberband_orig_point.x():
                x = self.m_rubberband_orig_point.x()
            else:
                x = pos.x()

            if pos.y() > self.m_rubberband_orig_point.y():
                y = self.m_rubberband_orig_point.y()
            else:
                y = pos.y()

            self.m_rubberband.setRect(x, y, abs(pos.x() - self.m_rubberband_orig_point.x()), abs(pos.y() - self.m_rubberband_orig_point.y()))
            return event.accept()

        QGraphicsScene.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.m_rubberband_selection:
            items_list = self.items()
            if len(items_list) > 0:
                for item in items_list:
                    if item and item.isVisible() and item.type() == CanvasBoxType:
                        item_rect = item.sceneBoundingRect()
                        item_top_left = QPointF(item_rect.x(), item_rect.y())
                        item_bottom_right = QPointF(item_rect.x() + item_rect.width(), item_rect.y() + item_rect.height())

                        if self.m_rubberband.contains(item_top_left) and self.m_rubberband.contains(item_bottom_right):
                            item.setSelected(True)

                self.m_rubberband.hide()
                self.m_rubberband.setRect(0, 0, 0, 0)
                self.m_rubberband_selection = False

        else:
            items_list = self.selectedItems()
            for item in items_list:
                if item and item.isVisible() and item.type() == CanvasBoxType:
                    item.checkItemPos()
                    self.emit(SIGNAL("sceneGroupMoved(int, int, QPointF)"), item.getGroupId(), item.getSplittedMode(), item.scenePos())

            if len(items_list) > 1:
                canvas.scene.update()

        self.m_mouse_down_init  = False
        self.m_mouse_rubberband = False
        QGraphicsScene.mouseReleaseEvent(self, event)

    def wheelEvent(self, event):
        if not self.m_view:
            return event.ignore()

        if self.m_ctrl_down:
            factor = 1.41 ** (event.delta() / 240.0)
            self.m_view.scale(factor, factor)

            self.fixScaleFactor()
            return event.accept()

        QGraphicsScene.wheelEvent(self, event)

# ------------------------------------------------------------------------------
# canvasfadeanimation.cpp

class CanvasFadeAnimation(QAbstractAnimation):
    def __init__(self, item, show, parent=None):
        QAbstractAnimation.__init__(self, parent)

        self.m_show = show
        self.m_duration = 0
        self.m_item = item

    def item(self):
        return self.m_item

    def setDuration(self, time):
        if self.m_item.opacity() == 0 and not self.m_show:
            self._duration = 0
        else:
            self.m_item.show()
            self.m_duration = time

    def duration(self):
        return self.m_duration

    def updateCurrentTime(self, time):
        if self.m_duration == 0:
            return

        if self.m_show:
            value = float(time) / self.m_duration
        else:
            value = 1.0 - (float(time) / self.m_duration)

        self.m_item.setOpacity(value)

        if self.m_item.type() == CanvasBoxType:
            self.m_item.setShadowOpacity(value)

    def updateDirection(self, direction):
        pass

    def updateState(self, oldState, newState):
        pass

# ------------------------------------------------------------------------------
# canvasline.cpp

class CanvasLine(QGraphicsLineItem):
    def __init__(self, item1, item2, parent):
        QGraphicsLineItem.__init__(self, parent, canvas.scene)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False

        self.setGraphicsEffect(None)
        self.updateLinePos()

    def deleteFromScene(self):
        canvas.scene.removeItem(self)
        del self

    def isLocked(self):
        return self.m_locked

    def setLocked(self, yesno):
        self.m_locked = yesno

    def isLineSelected(self):
        return self.m_lineSelected

    def setLineSelected(self, yesno):
        if self.m_locked:
            return

        if options.eyecandy == EYECANDY_FULL:
            if yesno:
                self.setGraphicsEffect(CanvasPortGlow(self.item1.getPortType(), self.toGraphicsObject()))
            else:
                self.setGraphicsEffect(None)

        self.m_lineSelected = yesno
        self.updateLineGradient()

    def updateLinePos(self):
        if self.item1.getPortMode() == PORT_MODE_OUTPUT:
            line = QLineF(self.item1.scenePos().x() + self.item1.getPortWidth() + 12,
                          self.item1.scenePos().y() + float(canvas.theme.port_height)/2,
                          self.item2.scenePos().x(),
                          self.item2.scenePos().y() + float(canvas.theme.port_height)/2)
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
        elif port_type1 == PORT_TYPE_MIDI_A2J:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_a2j_sel if self.m_lineSelected else canvas.theme.line_midi_a2j)
        elif port_type1 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)

        if port_type2 == PORT_TYPE_AUDIO_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_audio_jack_sel if self.m_lineSelected else canvas.theme.line_audio_jack)
        elif port_type2 == PORT_TYPE_MIDI_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_jack_sel if self.m_lineSelected else canvas.theme.line_midi_jack)
        elif port_type2 == PORT_TYPE_MIDI_A2J:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_a2j_sel if self.m_lineSelected else canvas.theme.line_midi_a2j)
        elif port_type2 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)

        self.setPen(QPen(port_gradient, 2))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsLineItem.paint(self, painter, option, widget)
        painter.restore()

# ------------------------------------------------------------------------------
# canvasbezierline.cpp

class CanvasBezierLine(QGraphicsPathItem):
    def __init__(self, item1, item2, parent):
        QGraphicsPathItem.__init__(self, parent, canvas.scene)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False

        self.setBrush(QColor(0, 0, 0, 0))
        self.setGraphicsEffect(None)
        self.updateLinePos()

    def deleteFromScene(self):
        canvas.scene.removeItem(self)
        del self

    def isLocked(self):
        return self.m_locked

    def setLocked(self, yesno):
        self.m_locked = yesno

    def isLineSelected(self):
        return self.m_lineSelected

    def setLineSelected(self, yesno):
        if self.m_locked:
            return

        if options.eyecandy == EYECANDY_FULL:
            if yesno:
                self.setGraphicsEffect(CanvasPortGlow(self.item1.getPortType(), self.toGraphicsObject()))
            else:
                self.setGraphicsEffect(None)

        self.m_lineSelected = yesno
        self.updateLineGradient()

    def updateLinePos(self):
        if self.item1.getPortMode() == PORT_MODE_OUTPUT:
            item1_x = self.item1.scenePos().x() + self.item1.getPortWidth() + 12
            item1_y = self.item1.scenePos().y() + float(canvas.theme.port_height)/2

            item2_x = self.item2.scenePos().x()
            item2_y = self.item2.scenePos().y() + float(canvas.theme.port_height)/2

            item1_mid_x = abs(item1_x - item2_x) / 2
            item1_new_x = item1_x + item1_mid_x

            item2_mid_x = abs(item1_x - item2_x) / 2
            item2_new_x = item2_x - item2_mid_x

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
        elif port_type1 == PORT_TYPE_MIDI_A2J:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_a2j_sel if self.m_lineSelected else canvas.theme.line_midi_a2j)
        elif port_type1 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos1, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)

        if port_type2 == PORT_TYPE_AUDIO_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_audio_jack_sel if self.m_lineSelected else canvas.theme.line_audio_jack)
        elif port_type2 == PORT_TYPE_MIDI_JACK:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_jack_sel if self.m_lineSelected else canvas.theme.line_midi_jack)
        elif port_type2 == PORT_TYPE_MIDI_A2J:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_a2j_sel if self.m_lineSelected else canvas.theme.line_midi_a2j)
        elif port_type2 == PORT_TYPE_MIDI_ALSA:
            port_gradient.setColorAt(pos2, canvas.theme.line_midi_alsa_sel if self.m_lineSelected else canvas.theme.line_midi_alsa)

        self.setPen(QPen(port_gradient, 2))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsPathItem.paint(self, painter, option, widget)
        painter.restore()

# ------------------------------------------------------------------------------
# canvaslivemov.cpp

class CanvasLineMov(QGraphicsLineItem):
    def __init__(self, port_mode, port_type, parent):
        QGraphicsLineItem.__init__(self, parent, canvas.scene)

        self.m_port_mode = port_mode
        self.m_port_type = port_type

        # Port position doesn't change while moving around line
        self.p_lineX = self.scenePos().x()
        self.p_lineY = self.scenePos().y()
        self.p_width = self.parentItem().getPortWidth()

        if port_type == PORT_TYPE_AUDIO_JACK:
            pen = QPen(canvas.theme.line_audio_jack, 2)
        elif port_type == PORT_TYPE_MIDI_JACK:
            pen = QPen(canvas.theme.line_midi_jack, 2)
        elif port_type == PORT_TYPE_MIDI_A2J:
            pen = QPen(canvas.theme.line_midi_a2j, 2)
        elif port_type == PORT_TYPE_MIDI_ALSA:
            pen = QPen(canvas.theme.line_midi_alsa, 2)
        else:
            qWarning("PatchCanvas::CanvasLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)

        self.setPen(pen)

    def deleteFromScene(self):
        canvas.scene.removeItem(self)
        del self

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

# ------------------------------------------------------------------------------
# canvasbezierlinemov.cpp

class CanvasBezierLineMov(QGraphicsPathItem):
    def __init__(self, port_mode, port_type, parent):
        QGraphicsPathItem.__init__(self, parent, canvas.scene)

        self.m_port_mode = port_mode
        self.m_port_type = port_type

        # Port position doesn't change while moving around line
        self.p_itemX = self.scenePos().x()
        self.p_itemY = self.scenePos().y()
        self.p_width = self.parentItem().getPortWidth()

        if port_type == PORT_TYPE_AUDIO_JACK:
            pen = QPen(canvas.theme.line_audio_jack, 2)
        elif port_type == PORT_TYPE_MIDI_JACK:
            pen = QPen(canvas.theme.line_midi_jack, 2)
        elif port_type == PORT_TYPE_MIDI_A2J:
            pen = QPen(canvas.theme.line_midi_a2j, 2)
        elif port_type == PORT_TYPE_MIDI_ALSA:
            pen = QPen(canvas.theme.line_midi_alsa, 2)
        else:
            qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)

        self.setBrush(QColor(0, 0, 0, 0))
        self.setPen(pen)

    def deleteFromScene(self):
        canvas.scene.removeItem(self)
        del self

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

# ------------------------------------------------------------------------------
# canvasport.cpp

class CanvasPort(QGraphicsItem):
    def __init__(self, port_id, port_name, port_mode, port_type, parent):
        QGraphicsItem.__init__(self, parent, canvas.scene)

        # Save Variables, useful for later
        self.m_port_id = port_id
        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_name = port_name

        # Base Variables
        self.m_port_width  = 15
        self.m_port_height = canvas.theme.port_height
        self.m_port_font = QFont(canvas.theme.port_font_name, canvas.theme.port_font_size, canvas.theme.port_font_state)

        self.m_line_mov = None
        self.m_hover_item = None
        self.m_last_selected_state = False

        self.m_mouse_down = False
        self.m_cursor_moving = False

        self.setFlags(QGraphicsItem.ItemIsSelectable)

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
            QTimer.singleShot(0, canvas.scene, SLOT("update()"))

        self.m_port_name = port_name
        self.update()

    def setPortWidth(self, port_width):
        if port_width < self.m_port_width:
            QTimer.singleShot(0, canvas.scene, SLOT("update()"))

        self.m_port_width = port_width
        self.update()

    def type(self):
        return CanvasPortType

    def mousePressEvent(self, event):
        self.m_hover_item = None
        self.m_mouse_down = bool(event.button() == Qt.LeftButton)
        self.m_cursor_moving = False
        QGraphicsItem.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.m_mouse_down:
            if not self.m_cursor_moving:
                self.setCursor(QCursor(Qt.CrossCursor))
                self.m_cursor_moving = True

                for connection in canvas.connection_list:
                    if connection.port_out_id == self.m_port_id or connection.port_in_id == self.m_port_id:
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
            for i in range(len(items)):
                if items[i].type() == CanvasPortType:
                    if items[i] != self:
                        if not item:
                            item = items[i]
                        elif items[i].parentItem().zValue() > item.parentItem().zValue():
                            item = items[i]

            if self.m_hover_item and self.m_hover_item != item:
                self.m_hover_item.setSelected(False)

            if item:
                a2j_connection = bool(item.getPortType() == PORT_TYPE_MIDI_JACK and self.m_port_type == PORT_TYPE_MIDI_A2J) or (item.getPortType() == PORT_TYPE_MIDI_A2J and self.m_port_type == PORT_TYPE_MIDI_JACK)

                if item.getPortMode() != self.m_port_mode and (item.getPortType() == self.m_port_type or a2j_connection):
                    item.setSelected(True)
                    self.m_hover_item = item
                else:
                    self.m_hover_item = None
            else:
                self.m_hover_item = None

            self.m_line_mov.updateLinePos(event.scenePos())
            return event.accept()

        QGraphicsItem.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.m_mouse_down:
            if self.m_line_mov:
                self.m_line_mov.deleteFromScene()
                self.m_line_mov = None

            for connection in canvas.connection_list:
                if connection.port_out_id == self.m_port_id or connection.port_in_id == self.m_port_id:
                    connection.widget.setLocked(False)

            if self.m_hover_item:
                check = False
                for connection in canvas.connection_list:
                    if ( (connection.port_out_id == self.m_port_id and connection.port_in_id == self.m_hover_item.getPortId()) or
                         (connection.port_out_id == self.m_hover_item.getPortId() and connection.port_in_id == self.m_port_id) ):
                        canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                        check = True
                        break

                if not check:
                    if self.m_port_mode == PORT_MODE_OUTPUT:
                        canvas.callback(ACTION_PORTS_CONNECT, self.m_port_id, self.m_hover_item.getPortId(), "")
                    else:
                        canvas.callback(ACTION_PORTS_CONNECT, self.m_hover_item.getPortId(), self.m_port_id, "")

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

        port_con_list = CanvasGetPortConnectionList(self.m_port_id)

        if len(port_con_list) > 0:
            for port_id in port_con_list:
                port_con_id = CanvasGetConnectedPort(port_id, self.m_port_id)
                act_x_disc = discMenu.addAction(CanvasGetFullPortName(port_con_id))
                act_x_disc.setData(port_id)
                QObject.connect(act_x_disc, SIGNAL("triggered()"), canvas.qobject, SLOT("PortContextMenuDisconnect()"))
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
            for port_id in port_con_list:
                canvas.callback(ACTION_PORTS_DISCONNECT, port_id, 0, "")

        elif act_selected == act_x_info:
            canvas.callback(ACTION_PORT_INFO, self.m_port_id, 0, "")

        elif act_selected == act_x_rename:
            new_name_try = QInputDialog.getText(None, "Rename Port", "New name:", QLineEdit.Normal, self.m_port_name)
            if new_name_try[1] and new_name_try[0]: # 1 - bool ok, 0 - return text
                canvas.callback(ACTION_PORT_RENAME, self.m_port_id, 0, new_name_try[0])

        event.accept()

    def boundingRect(self):
        return QRectF(0, 0, self.m_port_width + 12, self.m_port_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))

        poly_locx = [0, 0, 0, 0, 0]

        if self.m_port_mode == PORT_MODE_INPUT:
            text_pos = QPointF(3, canvas.theme.port_text_ypos)

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = 0
                poly_locx[1] = self.m_port_width + 5
                poly_locx[2] = self.m_port_width + 12
                poly_locx[3] = self.m_port_width + 5
                poly_locx[4] = 0
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = 0
                poly_locx[1] = self.m_port_width + 5
                poly_locx[2] = self.m_port_width + 5
                poly_locx[3] = self.m_port_width + 5
                poly_locx[4] = 0
            else:
                qCritical("PatchCanvas::CanvasPort.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        elif self.m_port_mode == PORT_MODE_OUTPUT:
            text_pos = QPointF(9, canvas.theme.port_text_ypos)

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = self.m_port_width + 12
                poly_locx[1] = 7
                poly_locx[2] = 0
                poly_locx[3] = 7
                poly_locx[4] = self.m_port_width + 12
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_port_width + 12
                poly_locx[1] = 5
                poly_locx[2] = 5
                poly_locx[3] = 5
                poly_locx[4] = self.m_port_width + 12
            else:
                qCritical("PatchCanvas::CanvasPort.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port mode '%s'" % port_mode2str(self.m_port_mode))
            return

        if self.m_port_type == PORT_TYPE_AUDIO_JACK:
            poly_color = canvas.theme.port_audio_jack_bg_sel if self.isSelected() else canvas.theme.port_audio_jack_bg
            poly_pen = canvas.theme.port_audio_jack_pen_sel  if self.isSelected() else canvas.theme.port_audio_jack_pen
            text_pen = canvas.theme.port_audio_jack_text_sel if self.isSelected() else canvas.theme.port_audio_jack_text
            conn_pen = canvas.theme.port_audio_jack_pen_sel
        elif self.m_port_type == PORT_TYPE_MIDI_JACK:
            poly_color = canvas.theme.port_midi_jack_bg_sel if self.isSelected() else canvas.theme.port_midi_jack_bg
            poly_pen = canvas.theme.port_midi_jack_pen_sel  if self.isSelected() else canvas.theme.port_midi_jack_pen
            text_pen = canvas.theme.port_midi_jack_text_sel if self.isSelected() else canvas.theme.port_midi_jack_text
            conn_pen = canvas.theme.port_midi_jack_pen_sel
        elif self.m_port_type == PORT_TYPE_MIDI_A2J:
            poly_color = canvas.theme.port_midi_a2j_bg_sel if self.isSelected() else canvas.theme.port_midi_a2j_bg
            poly_pen = canvas.theme.port_midi_a2j_pen_sel  if self.isSelected() else canvas.theme.port_midi_a2j_pen
            text_pen = canvas.theme.port_midi_a2j_text_sel if self.isSelected() else canvas.theme.port_midi_a2j_text
            conn_pen = canvas.theme.port_midi_a2j_pen_sel
        elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
            poly_color = canvas.theme.port_midi_alsa_bg_sel if self.isSelected() else canvas.theme.port_midi_alsa_bg
            poly_pen = canvas.theme.port_midi_alsa_pen_sel  if self.isSelected() else canvas.theme.port_midi_alsa_pen
            text_pen = canvas.theme.port_midi_alsa_text_sel if self.isSelected() else canvas.theme.port_midi_alsa_text
            conn_pen = canvas.theme.port_midi_alsa_pen_sel
        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port type '%s'" % port_type2str(self.m_port_type))
            return

        polygon  = QPolygonF()
        polygon += QPointF(poly_locx[0], 0)
        polygon += QPointF(poly_locx[1], 0)
        polygon += QPointF(poly_locx[2], float(canvas.theme.port_height)/2)
        polygon += QPointF(poly_locx[3], canvas.theme.port_height)
        polygon += QPointF(poly_locx[4], canvas.theme.port_height)

        if canvas.theme.port_bg_pixmap:
            portRect = polygon.boundingRect()
            portPos  = portRect.topLeft()
            painter.drawTiledPixmap(portRect, canvas.theme.port_bg_pixmap, portPos)
        else:
            painter.setBrush(poly_color)

        painter.setPen(poly_pen)
        painter.drawPolygon(polygon)

        painter.setPen(text_pen)
        painter.setFont(self.m_port_font)
        painter.drawText(text_pos, self.m_port_name)

        if self.isSelected() != self.m_last_selected_state:
            for connection in canvas.connection_list:
                if connection.port_out_id == self.m_port_id or connection.port_in_id == self.m_port_id:
                    connection.widget.setLineSelected(self.isSelected())

        if canvas.theme.idx == Theme.THEME_OOSTUDIO and canvas.theme.port_bg_pixmap:
            painter.setPen(Qt.NoPen)
            painter.setBrush(conn_pen.brush())

            if self.m_port_mode == PORT_MODE_INPUT:
                connRect = QRectF(portRect.topLeft(), QSizeF(2, portRect.height()))
            else:
                connRect = QRectF(QPointF(portRect.right()-2, portRect.top()), QSizeF(2, portRect.height()))

            painter.drawRect(connRect)

        self.m_last_selected_state = self.isSelected()

        painter.restore()

# ------------------------------------------------------------------------------
# canvasbox.cpp

class cb_line_t(object):
    __slots__ = [
        'line',
        'connection_id'
    ]

class CanvasBox(QGraphicsItem):
    def __init__(self, group_id, group_name, icon, parent=None):
        QGraphicsItem.__init__(self, parent, canvas.scene)

        # Save Variables, useful for later
        self.m_group_id   = group_id
        self.m_group_name = group_name

        # Base Variables
        self.p_width  = 50
        self.p_height = canvas.theme.box_header_height + canvas.theme.box_header_spacing + 1

        self.m_last_pos = QPointF()
        self.m_splitted = False
        self.m_splitted_mode = PORT_MODE_NULL

        self.m_cursor_moving = False
        self.m_forced_split = False
        self.m_mouse_down = False

        self.m_port_list_ids = []
        self.m_connection_lines = []

        # Set Font
        self.m_font_name = QFont(canvas.theme.box_font_name, canvas.theme.box_font_size, canvas.theme.box_font_state)
        self.m_font_port = QFont(canvas.theme.port_font_name, canvas.theme.port_font_size, canvas.theme.port_font_state)

        # Icon
        if canvas.theme.box_use_icon:
            self.icon_svg = CanvasIcon(icon, self.m_group_name, self)
        else:
            self.icon_svg = None

        # Shadow
        if options.eyecandy:
            self.shadow = CanvasBoxShadow(self.toGraphicsObject())
            self.shadow.setFakeParent(self)
            self.setGraphicsEffect(self.shadow)
        else:
            self.shadow = None

        # Final touches
        self.setFlags(QGraphicsItem.ItemIsMovable | QGraphicsItem.ItemIsSelectable)

        # Wait for at least 1 port
        if options.auto_hide_groups:
            self.setVisible(False)

        self.updatePositions()

    def getGroupId(self):
        return self.m_group_id

    def getGroupName(self):
        return self.m_group_name

    def isSplitted(self):
        return self.m_splitted

    def getSplittedMode(self):
        return self.m_splitted_mode

    def getPortCount(self):
        return len(self.m_port_list_ids)

    def getPortList(self):
        return self.m_port_list_ids

    def setIcon(self, icon):
        if self.icon_svg:
            self.icon_svg.setIcon(icon, self.m_group_name)

    def setSplit(self, split, mode=PORT_MODE_NULL):
        self.m_splitted = split
        self.m_splitted_mode = mode

    def setGroupName(self, group_name):
        self.m_group_name = group_name
        self.updatePositions()

    def setShadowOpacity(self, opacity):
        if self.shadow:
            self.shadow.setOpacity(opacity)

    def addPortFromGroup(self, port_id, port_mode, port_type, port_name):
        if len(self.m_port_list_ids) == 0:
            if options.auto_hide_groups:
                if options.eyecandy == EYECANDY_FULL:
                    CanvasItemFX(self, True)
                self.setVisible(True)

        new_widget = CanvasPort(port_id, port_name, port_mode, port_type, self)

        port_dict = port_dict_t()
        port_dict.group_id = self.m_group_id
        port_dict.port_id = port_id
        port_dict.port_name = port_name
        port_dict.port_mode = port_mode
        port_dict.port_type = port_type
        port_dict.widget = new_widget

        self.m_port_list_ids.append(port_id)

        return new_widget

    def removePortFromGroup(self, port_id):
        if port_id in self.m_port_list_ids:
            self.m_port_list_ids.remove(port_id)
        else:
            qCritical("PatchCanvas::CanvasBox.removePort(%i) - unable to find port to remove" % port_id)
            return

        if len(self.m_port_list_ids) > 0:
            self.updatePositions()

        elif self.isVisible():
            if options.auto_hide_groups:
                if options.eyecandy == EYECANDY_FULL:
                    CanvasItemFX(self, False)
                else:
                    self.setVisible(False)

    def addLineFromGroup(self, line, connection_id):
        new_cbline = cb_line_t()
        new_cbline.line = line
        new_cbline.connection_id = connection_id
        self.m_connection_lines.append(new_cbline)

    def removeLineFromGroup(self, connection_id):
        for connection in self.m_connection_lines:
            if connection.connection_id == connection_id:
                self.m_connection_lines.remove(connection)
                return
        qCritical("PatchCanvas::CanvasBox.removeLineFromGroup(%i) - unable to find line to remove" % connection_id)

    def checkItemPos(self):
        if not canvas.size_rect.isNull():
            pos = self.scenePos()
            if not (canvas.size_rect.contains(pos) and canvas.size_rect.contains(pos + QPointF(self.p_width, self.p_height))):
                if pos.x() < canvas.size_rect.x():
                    self.setPos(canvas.size_rect.x(), pos.y())
                elif pos.x() + self.p_width > canvas.size_rect.width():
                    self.setPos(canvas.size_rect.width() - self.p_width, pos.y())

                pos = self.scenePos()
                if pos.y() < canvas.size_rect.y():
                    self.setPos(pos.x(), canvas.size_rect.y())
                elif pos.y() + self.p_height > canvas.size_rect.height():
                    self.setPos(pos.x(), canvas.size_rect.height() - self.p_height)

    def removeIconFromScene(self):
        if self.icon_svg:
            canvas.scene.removeItem(self.icon_svg)

    def updatePositions(self):
        self.prepareGeometryChange()

        max_in_width  = 0
        max_in_height = canvas.theme.box_header_height + canvas.theme.box_header_spacing
        max_out_width  = 0
        max_out_height = canvas.theme.box_header_height + canvas.theme.box_header_spacing
        port_spacing   = canvas.theme.port_height + canvas.theme.port_spacing

        have_audio_jack_in = have_midi_jack_in = have_midi_a2j_in = have_midi_alsa_in = False
        have_audio_jack_out = have_midi_jack_out = have_midi_a2j_out = have_midi_alsa_out = False

        # reset box size
        self.p_width  = 50
        self.p_height = canvas.theme.box_header_height + canvas.theme.box_header_spacing + 1

        # Check Text Name size
        app_name_size = QFontMetrics(self.m_font_name).width(self.m_group_name) + 30
        if app_name_size > self.p_width:
            self.p_width = app_name_size

        # Get Port List
        port_list = []
        for port in canvas.port_list:
            if port.port_id in self.m_port_list_ids:
                port_list.append(port)

        # Get Max Box Width/Height
        for port in port_list:
            if port.port_mode == PORT_MODE_INPUT:
                max_in_height += port_spacing

                size = QFontMetrics(self.m_font_port).width(port.port_name)
                if size > max_in_width:
                    max_in_width = size

                if port.port_type == PORT_TYPE_AUDIO_JACK and not have_audio_jack_in:
                    have_audio_jack_in = True
                    max_in_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_JACK and not have_midi_jack_in:
                    have_midi_jack_in = True
                    max_in_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_A2J and not have_midi_a2j_in:
                    have_midi_a2j_in = True
                    max_in_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_ALSA and not have_midi_alsa_in:
                    have_midi_alsa_in = True
                    max_in_height += canvas.theme.port_spacingT

            elif port.port_mode == PORT_MODE_OUTPUT:
                max_out_height += port_spacing

                size = QFontMetrics(self.m_font_port).width(port.port_name)
                if size > max_out_width:
                    max_out_width = size

                if port.port_type == PORT_TYPE_AUDIO_JACK and not have_audio_jack_out:
                    have_audio_jack_out = True
                    max_out_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_JACK and not have_midi_jack_out:
                    have_midi_jack_out = True
                    max_out_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_A2J and not have_midi_a2j_out:
                    have_midi_a2j_out = True
                    max_out_height += canvas.theme.port_spacingT
                elif port.port_type == PORT_TYPE_MIDI_ALSA and not have_midi_alsa_out:
                    have_midi_alsa_out = True
                    max_out_height += canvas.theme.port_spacingT

        if canvas.theme.port_spacingT == 0:
            max_in_height  += 2
            max_out_height += 2

        final_width = 30 + max_in_width + max_out_width
        if final_width > self.p_width:
            self.p_width = final_width

        if max_in_height > self.p_height:
            self.p_height = max_in_height

        if max_out_height > self.p_height:
            self.p_height = max_out_height

        # Remove bottom space
        self.p_height -= canvas.theme.port_spacingT

        if canvas.theme.box_header_spacing > 0:
            if len(port_list) == 0:
                self.p_height -= canvas.theme.box_header_spacing
            else:
                self.p_height -= canvas.theme.box_header_spacing/2

        last_in_pos  = canvas.theme.box_header_height + canvas.theme.box_header_spacing
        last_out_pos = canvas.theme.box_header_height + canvas.theme.box_header_spacing
        last_in_type  = PORT_TYPE_NULL
        last_out_type = PORT_TYPE_NULL

        # Re-position ports, AUDIO_JACK
        for port in port_list:
            if port.port_type == PORT_TYPE_AUDIO_JACK:
                if port.port_mode == PORT_MODE_INPUT:
                    port.widget.setPos(QPointF(1 + canvas.theme.port_offset, last_in_pos))
                    port.widget.setPortWidth(max_in_width)

                    last_in_pos += port_spacing
                    last_in_type = port.port_type

                elif port.port_mode == PORT_MODE_OUTPUT:
                    port.widget.setPos(QPointF(self.p_width - max_out_width - canvas.theme.port_offset - 13, last_out_pos))
                    port.widget.setPortWidth(max_out_width)

                    last_out_pos += port_spacing
                    last_out_type = port.port_type

        # Re-position ports, MIDI_JACK
        for port in port_list:
            if port.port_type == PORT_TYPE_MIDI_JACK:
                if port.port_mode == PORT_MODE_INPUT:
                    if last_in_type != PORT_TYPE_NULL and port.port_type != last_in_type:
                        last_in_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(1 + canvas.theme.port_offset, last_in_pos))
                    port.widget.setPortWidth(max_in_width)

                    last_in_pos += port_spacing
                    last_in_type = port.port_type

                elif port.port_mode == PORT_MODE_OUTPUT:
                    if last_out_type != PORT_TYPE_NULL and port.port_type != last_out_type:
                        last_out_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(self.p_width - max_out_width - canvas.theme.port_offset - 13, last_out_pos))
                    port.widget.setPortWidth(max_out_width)

                    last_out_pos += port_spacing
                    last_out_type = port.port_type

        # Re-position ports, MIDI_A2J
        for port in port_list:
            if port.port_type == PORT_TYPE_MIDI_A2J:
                if port.port_mode == PORT_MODE_INPUT:
                    if last_in_type != PORT_TYPE_NULL and port.port_type != last_in_type:
                        last_in_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(1 + canvas.theme.port_offset, last_in_pos))
                    port.widget.setPortWidth(max_in_width)

                    last_in_pos += port_spacing
                    last_in_type = port.port_type

                elif port.port_mode == PORT_MODE_OUTPUT:
                    if last_out_type != PORT_TYPE_NULL and port.port_type != last_out_type:
                        last_out_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(self.p_width - max_out_width - canvas.theme.port_offset - 13, last_out_pos))
                    port.widget.setPortWidth(max_out_width)

                    last_out_pos += port_spacing
                    last_out_type = port.port_type

        # Re-position ports, MIDI_ALSA
        for port in port_list:
            if port.port_type == PORT_TYPE_MIDI_ALSA:
                if port.port_mode == PORT_MODE_INPUT:
                    if last_in_type != PORT_TYPE_NULL and port.port_type != last_in_type:
                        last_in_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(1 + canvas.theme.port_offset, last_in_pos))
                    port.widget.setPortWidth(max_in_width)

                    last_in_pos += port_spacing
                    last_in_type = port.port_type

                elif port.port_mode == PORT_MODE_OUTPUT:
                    if last_out_type != PORT_TYPE_NULL and port.port_type != last_out_type:
                        last_out_pos += canvas.theme.port_spacingT

                    port.widget.setPos(QPointF(self.p_width - max_out_width - canvas.theme.port_offset - 13, last_out_pos))
                    port.widget.setPortWidth(max_out_width)

                    last_out_pos += port_spacing
                    last_out_type = port.port_type

        self.repaintLines(True)
        self.update()

    def repaintLines(self, forced=False):
        if self.pos() != self.m_last_pos or forced:
            for connection in self.m_connection_lines:
                connection.line.updateLinePos()

        self.m_last_pos = self.pos()

    def resetLinesZValue(self):
        for connection in canvas.connection_list:
            if connection.port_out_id in self.m_port_list_ids and connection.port_in_id in self.m_port_list_ids:
                z_value = canvas.last_z_value
            else:
                z_value = canvas.last_z_value - 1

            connection.widget.setZValue(z_value)

    def type(self):
        return CanvasBoxType

    def contextMenuEvent(self, event):
        menu = QMenu()
        discMenu = QMenu("Disconnect", menu)

        port_con_list = []
        port_con_list_ids = []

        for port_id in self.m_port_list_ids:
            tmp_port_con_list = CanvasGetPortConnectionList(port_id)
            for port_con_id in tmp_port_con_list:
                if port_con_id not in port_con_list:
                    port_con_list.append(port_con_id)
                    port_con_list_ids.append(port_id)

        if len(port_con_list) > 0:
            for i in range(len(port_con_list)):
                port_con_id = CanvasGetConnectedPort(port_con_list[i], port_con_list_ids[i])
                act_x_disc = discMenu.addAction(CanvasGetFullPortName(port_con_id))
                act_x_disc.setData(port_con_list[i])
                QObject.connect(act_x_disc, SIGNAL("triggered()"), canvas.qobject, SLOT("PortContextMenuDisconnect()"))
        else:
            act_x_disc = discMenu.addAction("No connections")
            act_x_disc.setEnabled(False)

        menu.addMenu(discMenu)
        act_x_disc_all = menu.addAction("Disconnect &All")
        act_x_sep1 = menu.addSeparator()
        act_x_info = menu.addAction("&Info")
        act_x_rename = menu.addAction("&Rename")
        act_x_sep2 = menu.addSeparator()
        act_x_split_join = menu.addAction("Join" if self.m_splitted else "Split")

        if not features.group_info:
            act_x_info.setVisible(False)

        if not features.group_rename:
            act_x_rename.setVisible(False)

        if not (features.group_info and features.group_rename):
            act_x_sep1.setVisible(False)

        haveIns = haveOuts = False
        for port in canvas.port_list:
            if port.port_id in self.m_port_list_ids:
                if port.port_mode == PORT_MODE_INPUT:
                    haveIns = True
                elif port.port_mode == PORT_MODE_OUTPUT:
                    haveOuts = True

        if not (self.m_splitted or bool(haveIns and haveOuts)):
            act_x_sep2.setVisible(False)
            act_x_split_join.setVisible(False)

        act_selected = menu.exec_(event.screenPos())

        if act_selected == act_x_disc_all:
            for port_id in port_con_list:
                canvas.callback(ACTION_PORTS_DISCONNECT, port_id, 0, "")

        elif act_selected == act_x_info:
            canvas.callback(ACTION_GROUP_INFO, self.m_group_id, 0, "")

        elif act_selected == act_x_rename:
            new_name_try = QInputDialog.getText(None, "Rename Group", "New name:", QLineEdit.Normal, self.m_group_name)
            if new_name_try[1] and new_name_try[0]: # 1 - bool ok, 0 - return text
                canvas.callback(ACTION_GROUP_RENAME, self.m_group_id, 0, new_name_try[0])

        elif act_selected == act_x_split_join:
            if self.m_splitted:
                canvas.callback(ACTION_GROUP_JOIN, self.m_group_id, 0, "")
            else:
                canvas.callback(ACTION_GROUP_SPLIT, self.m_group_id, 0, "")

        event.accept()

    def mousePressEvent(self, event):
        canvas.last_z_value += 1
        self.setZValue(canvas.last_z_value)
        self.resetLinesZValue()
        self.m_cursor_moving = False

        if event.button() == Qt.RightButton:
            canvas.scene.clearSelection()
            self.setSelected(True)
            self.m_mouse_down = False
            return event.accept()

        elif event.button() == Qt.LeftButton:
            if self.sceneBoundingRect().contains(event.scenePos()):
                self.m_mouse_down = True
            else:
                # Fix a weird Qt behaviour with right-click mouseMove
                self.m_mouse_down = False
                return event.ignore()

        else:
            self.m_mouse_down = False

        QGraphicsItem.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.m_mouse_down:
            if not self.m_cursor_moving:
                self.setCursor(QCursor(Qt.SizeAllCursor))
                self.m_cursor_moving = True
            self.repaintLines()
        QGraphicsItem.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self.m_cursor_moving:
            self.setCursor(QCursor(Qt.ArrowCursor))
        self.m_mouse_down = False
        self.m_cursor_moving = False
        QGraphicsItem.mouseReleaseEvent(self, event)

    def boundingRect(self):
        return QRectF(0, 0, self.p_width, self.p_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, False)

        # Draw rectangle
        if self.isSelected():
            painter.setPen(canvas.theme.box_pen_sel)
        else:
            painter.setPen(canvas.theme.box_pen)

        if canvas.theme.box_bg_type == Theme.THEME_BG_GRADIENT:
            box_gradient = QLinearGradient(0, 0, 0, self.p_height)
            box_gradient.setColorAt(0, canvas.theme.box_bg_1)
            box_gradient.setColorAt(1, canvas.theme.box_bg_2)
            painter.setBrush(box_gradient)
        else:
            painter.setBrush(canvas.theme.box_bg_1)

        painter.drawRect(0, 0, self.p_width, self.p_height)

        # Draw pixmap header
        if canvas.theme.box_header_pixmap:
            painter.setPen(Qt.NoPen)
            painter.setBrush(canvas.theme.box_bg_2)
            painter.drawRect(1, 1, self.p_width-2, canvas.theme.box_header_height)

            headerPos  = QPointF(1, 1)
            headerRect = QRectF(2, 2, self.p_width-4, canvas.theme.box_header_height-3)
            painter.drawTiledPixmap(headerRect, canvas.theme.box_header_pixmap, headerPos)

        # Draw text
        painter.setFont(self.m_font_name)

        if self.isSelected():
            painter.setPen(canvas.theme.box_text_sel)
        else:
            painter.setPen(canvas.theme.box_text)

        if canvas.theme.box_use_icon:
            textPos = QPointF(25, canvas.theme.box_text_ypos)
        else:
            appNameSize = QFontMetrics(self.m_font_name).width(self.m_group_name)
            rem = self.p_width - appNameSize
            textPos = QPointF(rem/2, canvas.theme.box_text_ypos)

        painter.drawText(textPos, self.m_group_name)

        self.repaintLines()

        painter.restore()

# ------------------------------------------------------------------------------
# canvasicon.cpp

class CanvasIcon(QGraphicsSvgItem):
    def __init__(self, icon, name, parent):
        QGraphicsSvgItem.__init__(self, parent)

        self.m_renderer = None
        self.p_size = QRectF(0, 0, 0, 0)

        self.m_colorFX = QGraphicsColorizeEffect(self)
        self.m_colorFX.setColor(canvas.theme.box_text.color())

        self.setGraphicsEffect(self.m_colorFX)
        self.setIcon(icon, name)

    def setIcon(self, icon, name):
        name = name.lower()
        icon_path = ""

        if icon == ICON_APPLICATION:
            self.p_size = QRectF(3, 2, 19, 18)

            if "audacious" in name:
                icon_path   = ":/scalable/pb_audacious.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "clementine" in name:
                icon_path = ":/scalable/pb_clementine.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "jamin" in name:
                icon_path = ":/scalable/pb_jamin.svg"
                self.p_size = QRectF(5, 3, 16, 16)
            elif "mplayer" in name:
                icon_path = ":/scalable/pb_mplayer.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "vlc" in name:
                icon_path = ":/scalable/pb_vlc.svg"
                self.p_size = QRectF(5, 3, 16, 16)

            else:
                icon_path = ":/scalable/pb_generic.svg"
                self.p_size = QRectF(5, 3, 16, 16)

        elif icon == ICON_HARDWARE:
            icon_path = ":/scalable/pb_hardware.svg"
            self.p_size = QRectF(5, 2, 16, 16)

        elif icon == ICON_LADISH_ROOM:
            # TODO - make a unique ladish-room icon
            icon_path = ":/scalable/pb_hardware.svg"
            self.p_size = QRectF(5, 2, 16, 16)

        else:
            self.p_size = QRectF(0, 0, 0, 0)
            qCritical("PatchCanvas::CanvasIcon.setIcon(%s, %s) - unsupported icon requested" % (icon2str(icon), name.encode()))
            return

        self.m_renderer = QSvgRenderer(icon_path, canvas.scene)
        self.setSharedRenderer(self.m_renderer)
        self.update()

    def type(self):
        return CanvasIconType

    def boundingRect(self):
        return self.p_size

    def paint(self, painter, option, widget):
        if self.m_renderer:
            painter.save()
            painter.setRenderHint(QPainter.Antialiasing, False)
            painter.setRenderHint(QPainter.TextAntialiasing, False)
            self.m_renderer.render(painter, self.p_size)
            painter.restore()
        else:
            QGraphicsSvgItem.paint(self, painter, option, widget)

# ------------------------------------------------------------------------------
# canvasportglow.cpp

class CanvasPortGlow(QGraphicsDropShadowEffect):
    def __init__(self, port_type, parent):
        QGraphicsDropShadowEffect.__init__(self, parent)

        self.setBlurRadius(12)
        self.setOffset(0, 0)

        if port_type == PORT_TYPE_AUDIO_JACK:
            self.setColor(canvas.theme.line_audio_jack_glow)
        elif port_type == PORT_TYPE_MIDI_JACK:
            self.setColor(canvas.theme.line_midi_jack_glow)
        elif port_type == PORT_TYPE_MIDI_A2J:
            self.setColor(canvas.theme.line_midi_a2j_glow)
        elif port_type == PORT_TYPE_MIDI_ALSA:
            self.setColor(canvas.theme.line_midi_alsa_glow)

# ------------------------------------------------------------------------------
# canvasboxshadow.cpp

class CanvasBoxShadow(QGraphicsDropShadowEffect):
    def __init__(self, parent):
        QGraphicsDropShadowEffect.__init__(self, parent)

        self.m_fakeParent = None

        self.setBlurRadius(20)
        self.setColor(canvas.theme.box_shadow)
        self.setOffset(0, 0)

    def setFakeParent(self, fakeParent):
        self.m_fakeParent = fakeParent

    def setOpacity(self, opacity):
        color = QColor(canvas.theme.box_shadow)
        color.setAlphaF(opacity)
        self.setColor(color)

    def draw(self, painter):
        if self.m_fakeParent:
            self.m_fakeParent.repaintLines()
        QGraphicsDropShadowEffect.draw(self, painter)
