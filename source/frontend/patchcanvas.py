#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas engine using QGraphicsView/Scene
# Copyright (C) 2010-2018 Filipe Coelho <falktx@falktx.com>
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
# Imports (Config)
import sys
from carla_config import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import floor, log

if config_UseQt5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, qCritical, qFatal, qWarning, Qt, QObject
    from PyQt5.QtCore import QAbstractAnimation, QLineF, QPointF, QRectF, QSizeF, QSettings, QTimer
    from PyQt5.QtGui import QColor, QLinearGradient, QPen, QPolygonF, QPainter, QPainterPath
    from PyQt5.QtGui import QCursor, QFont, QFontMetrics
    from PyQt5.QtSvg import QGraphicsSvgItem, QSvgRenderer
    from PyQt5.QtWidgets import QGraphicsScene, QGraphicsItem, QGraphicsLineItem, QGraphicsPathItem, QGraphicsRectItem
    from PyQt5.QtWidgets import QGraphicsColorizeEffect, QGraphicsDropShadowEffect, QMenu
else:
    from PyQt4.QtCore import pyqtSignal, pyqtSlot, qCritical, qFatal, qWarning, Qt, QObject
    from PyQt4.QtCore import QAbstractAnimation, QLineF, QPointF, QRectF, QSizeF, QSettings, QTimer
    from PyQt4.QtGui import QColor, QLinearGradient, QPen, QPolygonF, QPainter, QPainterPath
    from PyQt4.QtGui import QCursor, QFont, QFontMetrics
    from PyQt4.QtGui import QGraphicsScene, QGraphicsItem, QGraphicsLineItem, QGraphicsPathItem, QGraphicsRectItem
    from PyQt4.QtGui import QGraphicsColorizeEffect, QGraphicsDropShadowEffect, QMenu
    from PyQt4.QtSvg import QGraphicsSvgItem, QSvgRenderer

# ------------------------------------------------------------------------------------------------------------
# Imports (Theme)

from patchcanvas_theme import *

# ------------------------------------------------------------------------------
# patchcanvas-api.h

# Maximum Id for a plugin, treated as invalid/zero if above this value
MAX_PLUGIN_ID_ALLOWED = 0x7FF

# Port Mode
PORT_MODE_NULL   = 0
PORT_MODE_INPUT  = 1
PORT_MODE_OUTPUT = 2

# Port Type
PORT_TYPE_NULL       = 0
PORT_TYPE_AUDIO_JACK = 1
PORT_TYPE_MIDI_JACK  = 2
PORT_TYPE_MIDI_ALSA  = 3
PORT_TYPE_PARAMETER  = 4

#Port Stereo position
PORT_IN_PAIR_POSITION_MONO  = None
PORT_IN_PAIR_POSITION_LEFT  = 0

# Callback Action
ACTION_GROUP_INFO       =  0 # group_id, N, N
ACTION_GROUP_RENAME     =  1 # group_id, N, N
ACTION_GROUP_SPLIT      =  2 # group_id, N, N
ACTION_GROUP_JOIN       =  3 # group_id, N, N
ACTION_PORT_INFO        =  4 # group_id, port_id, N
ACTION_PORT_RENAME      =  5 # group_id, port_id, N
ACTION_PORTS_CONNECT    =  6 # N, N, "outG:outP:inG:inP"
ACTION_PORTS_DISCONNECT =  7 # conn_id, N, N
ACTION_PLUGIN_CLONE     =  8 # plugin_id, N, N
ACTION_PLUGIN_EDIT      =  9 # plugin_id, N, N
ACTION_PLUGIN_RENAME    = 10 # plugin_id, N, N
ACTION_PLUGIN_REPLACE   = 11 # plugin_id, N, N
ACTION_PLUGIN_REMOVE    = 12 # plugin_id, N, N
ACTION_PLUGIN_SHOW_UI   = 13 # plugin_id, N, N
ACTION_BG_RIGHT_CLICK   = 14 # N, N, N

# Icon
ICON_APPLICATION = 0
ICON_HARDWARE    = 1
ICON_DISTRHO     = 2
ICON_FILE        = 3
ICON_PLUGIN      = 4
ICON_LADISH_ROOM = 5

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
        'auto_select_items',
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
CanvasStereoPairType    = QGraphicsItem.UserType + 4
CanvasLineType          = QGraphicsItem.UserType + 5
CanvasBezierLineType    = QGraphicsItem.UserType + 6
CanvasLineMovType       = QGraphicsItem.UserType + 7
CanvasBezierLineMovType = QGraphicsItem.UserType + 8
CanvasRubberbandType    = QGraphicsItem.UserType + 9

# object lists
class group_dict_t(object):
    __slots__ = [
        'group_id',
        'group_name',
        'split',
        'icon',
        'plugin_id',
        'plugin_ui',
        'widgets'
    ]

class port_dict_t(object):
    __slots__ = [
        'group_id',
        'port_id',
        'port_name',
        'port_mode',
        'port_type',
        'pair_id',
        'is_alternate',
        'widget'
    ]

class force_mono_dict_t(object):
    __slots__ = [
        'port_id',
        'port_mode',
        'full_port_name'
    ]
    
class pair_dict_t(object):
    __slots__ = [
        'pair_id',
        'group_id',
        'port_mode',
        'port_type',
        'port_id_list',
        'widget'
    ]

class saved_pair_dict_t(object):
    __slots__ = [
        'port_mode',
        'group_name',
        'port_id_list',
        'port_name_list'
    ]
 

class connection_dict_t(object):
    __slots__ = [
        'connection_id',
        'group_in_id',
        'port_in_id',
        'group_out_id',
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
        'is_line_mov',
        'group_list',
        'port_list',
        'force_mono_list',
        'pair_list',
        'saved_pair_list',
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
    def AnimationFinishedShow(self):
        animation = self.sender()
        if animation:
            animation.forceStop()
            canvas.animation_list.remove(animation)

    @pyqtSlot()
    def AnimationFinishedHide(self):
        animation = self.sender()
        if animation:
            animation.forceStop()
            canvas.animation_list.remove(animation)
            item = animation.item()
            if item: item.hide()

    @pyqtSlot()
    def AnimationFinishedDestroy(self):
        animation = self.sender()
        if animation:
            animation.forceStop()
            canvas.animation_list.remove(animation)
            item = animation.item()
            if item: CanvasRemoveItemFX(item)

    @pyqtSlot()
    def PortContextMenuDisconnect(self):
        try:
            connectionId_list = self.sender().data()
        except:
            return
        
        for connectionId in connectionId_list:
            CanvasCallback(ACTION_PORTS_DISCONNECT, connectionId, 0, "")
        
    @pyqtSlot()
    def SetasStereoWith(self):
        try:
            all_data = self.sender().data()
        except:
            return
        
        port_widget = all_data[0]
        port_id = all_data[1]
        port_widget.SetAsStereo(port_id)

# Global objects
canvas = Canvas()
canvas.qobject    = None
canvas.settings   = None
canvas.theme      = None
canvas.initiated  = False
canvas.is_line_mov = False
canvas.group_list = []
canvas.port_list  = []
canvas.pair_list  = []
canvas.force_mono_list = []
canvas.saved_pair_list = []
canvas.connection_list = []
canvas.animation_list  = []

options = options_t()
options.theme_name = getDefaultThemeName()
options.auto_hide_groups  = False
options.auto_select_items = False
options.use_bezier_lines  = True
options.antialiasing      = ANTIALIASING_SMALL
options.eyecandy          = EYECANDY_SMALL

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
    elif port_type == PORT_TYPE_MIDI_ALSA:
        return "PORT_TYPE_MIDI_ALSA"
    elif port_type == PORT_TYPE_PARAMETER:
        return "PORT_TYPE_MIDI_PARAMETER"
    else:
        return "PORT_TYPE_???"

def icon2str(icon):
    if icon == ICON_APPLICATION:
        return "ICON_APPLICATION"
    elif icon == ICON_HARDWARE:
        return "ICON_HARDWARE"
    elif icon == ICON_DISTRHO:
        return "ICON_DISTRHO"
    elif icon == ICON_FILE:
        return "ICON_FILE"
    elif icon == ICON_PLUGIN:
        return "ICON_PLUGIN"
    elif icon == ICON_LADISH_ROOM:
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

def getStoredCanvasPosition(key, fallback_pos):
    try:
        return canvas.settings.value("CanvasPositions/" + key, fallback_pos, type=QPointF)
    except:
        return fallback_pos

def getStoredCanvasSplit(group_name, fallback_split_mode):
    try:
        return canvas.settings.value("CanvasPositions/%s_SPLIT" % group_name, fallback_split_mode, type=int)
    except:
        return fallback_split_mode

# PatchCanvas API
def setOptions(new_options):
    if canvas.initiated: return
    options.theme_name        = new_options.theme_name
    options.auto_hide_groups  = new_options.auto_hide_groups
    options.auto_select_items = new_options.auto_select_items
    options.use_bezier_lines  = new_options.use_bezier_lines
    options.antialiasing      = new_options.antialiasing
    options.eyecandy          = new_options.eyecandy

def setFeatures(new_features):
    if canvas.initiated: return
    features.group_info   = new_features.group_info
    features.group_rename = new_features.group_rename
    features.port_info    = new_features.port_info
    features.port_rename  = new_features.port_rename
    features.handle_group_pos = new_features.handle_group_pos

def init(appName, scene, callback, debug=False):
    if debug:
        print("PatchCanvas::init(\"%s\", %s, %s, %s)" % (appName, scene, callback, bool2str(debug)))

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
    if not canvas.settings: canvas.settings = QSettings("falkTX", appName)

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
        print("PatchCanvas::clear()")

    group_list_ids = []
    port_list_ids  = []
    connection_list_ids = []

    for group in canvas.group_list:
        group_list_ids.append(group.group_id)

    for port in canvas.port_list:
        port_list_ids.append((port.group_id, port.port_id))

    for connection in canvas.connection_list:
        connection_list_ids.append(connection.connection_id)

    for idx in connection_list_ids:
        disconnectPorts(idx)

    for group_id, port_id in port_list_ids:
        removePort(group_id, port_id)

    for idx in group_list_ids:
        removeGroup(idx)

    canvas.last_z_value = 0
    canvas.last_connection_id = 0

    canvas.group_list = []
    canvas.port_list = []
    canvas.connection_list = []

    canvas.scene.clearSelection()

    animatedItems = []
    for animation in canvas.animation_list:
        animatedItems.append(animation.item())

    for item in canvas.scene.items():
        if item.type() in (CanvasIconType, CanvasRubberbandType) or item in animatedItems:
            continue
        canvas.scene.removeItem(item)
        del item

    canvas.initiated = False

    QTimer.singleShot(0, canvas.scene.update)

def setInitialPos(x, y):
    if canvas.debug:
        print("PatchCanvas::setInitialPos(%i, %i)" % (x, y))

    canvas.initial_pos.setX(x)
    canvas.initial_pos.setY(y)

def setCanvasSize(x, y, width, height):
    if canvas.debug:
        print("PatchCanvas::setCanvasSize(%i, %i, %i, %i)" % (x, y, width, height))

    canvas.size_rect.setX(x)
    canvas.size_rect.setY(y)
    canvas.size_rect.setWidth(width)
    canvas.size_rect.setHeight(height)

def addGroup(group_id, group_name, split=SPLIT_UNDEF, icon=ICON_APPLICATION):
    if canvas.debug:
        print("PatchCanvas::addGroup(%i, %s, %s, %s)" % (group_id, group_name.encode(), split2str(split), icon2str(icon)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            qWarning("PatchCanvas::addGroup(%i, %s, %s, %s) - group already exists" % (group_id, group_name.encode(), split2str(split), icon2str(icon)))
            return

    if split == SPLIT_UNDEF:
        isHardware = bool(icon == ICON_HARDWARE)

        if features.handle_group_pos:
            split = getStoredCanvasSplit(group_name, SPLIT_YES if isHardware else split)
        elif isHardware:
            split = SPLIT_YES

    group_box = CanvasBox(group_id, group_name, icon)

    group_dict = group_dict_t()
    group_dict.group_id = group_id
    group_dict.group_name = group_name
    group_dict.split = bool(split == SPLIT_YES)
    group_dict.icon = icon
    group_dict.plugin_id = -1
    group_dict.plugin_ui = False
    group_dict.widgets = [group_box, None]

    if split == SPLIT_YES:
        group_box.setSplit(True, PORT_MODE_OUTPUT)

        if features.handle_group_pos:
            group_box.setPos(getStoredCanvasPosition(group_name + "_OUTPUT", CanvasGetNewGroupPos(False)))
        else:
            group_box.setPos(CanvasGetNewGroupPos(False))

        group_sbox = CanvasBox(group_id, group_name, icon)
        group_sbox.setSplit(True, PORT_MODE_INPUT)

        group_dict.widgets[1] = group_sbox

        if features.handle_group_pos:
            group_sbox.setPos(getStoredCanvasPosition(group_name + "_INPUT", CanvasGetNewGroupPos(True)))
        else:
            group_sbox.setPos(CanvasGetNewGroupPos(True))

        canvas.last_z_value += 1
        group_sbox.setZValue(canvas.last_z_value)

        if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
            CanvasItemFX(group_sbox, True, False)

        group_sbox.checkItemPos()

    else:
        group_box.setSplit(False)

        if features.handle_group_pos:
            group_box.setPos(getStoredCanvasPosition(group_name, CanvasGetNewGroupPos(False)))
        else:
            # Special ladish fake-split groups
            horizontal = bool(icon == ICON_HARDWARE or icon == ICON_LADISH_ROOM)
            group_box.setPos(CanvasGetNewGroupPos(horizontal))

    group_box.checkItemPos()

    canvas.last_z_value += 1
    group_box.setZValue(canvas.last_z_value)

    canvas.group_list.append(group_dict)

    if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
        return CanvasItemFX(group_box, True, False)

    QTimer.singleShot(0, canvas.scene.update)

def removeGroup(group_id):
    if canvas.debug:
        print("PatchCanvas::removeGroup(%i)" % group_id)

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

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::removeGroup(%i) - unable to find group to remove" % group_id)

def renameGroup(group_id, new_group_name):
    if canvas.debug:
        print("PatchCanvas::renameGroup(%i, %s)" % (group_id, new_group_name.encode()))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.group_name = new_group_name
            group.widgets[0].setGroupName(new_group_name)

            if group.split and group.widgets[1]:
                group.widgets[1].setGroupName(new_group_name)

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::renameGroup(%i, %s) - unable to find group to rename" % (group_id, new_group_name.encode()))

def splitGroup(group_id):
    if canvas.debug:
        print("PatchCanvas::splitGroup(%i)" % group_id)

    item = None
    group_name = ""
    group_icon = ICON_APPLICATION
    plugin_id  = -1
    plugin_ui  = False
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
            plugin_id  = group.plugin_id
            plugin_ui  = group.plugin_ui
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
            port_dict.is_alternate = port.is_alternate
            port_dict.widget = None
            ports_data.append(port_dict)

    for connection in canvas.connection_list:
        if connection.port_out_id in port_list_ids or connection.port_in_id in port_list_ids:
            connection_dict = connection_dict_t()
            connection_dict.connection_id = connection.connection_id
            connection_dict.group_in_id = connection.group_in_id
            connection_dict.port_in_id = connection.port_in_id
            connection_dict.group_out_id = connection.group_out_id
            connection_dict.port_out_id = connection.port_out_id
            connection_dict.widget = None
            conns_data.append(connection_dict)

    # Step 2 - Remove Item and Children
    for conn in conns_data:
        disconnectPorts(conn.connection_id)

    for port_id in port_list_ids:
        removePort(group_id, port_id)

    removeGroup(group_id)

    # Step 3 - Re-create Item, now splitted
    addGroup(group_id, group_name, SPLIT_YES, group_icon)

    if plugin_id >= 0:
        setGroupAsPlugin(group_id, plugin_id, plugin_ui)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type, port.is_alternate)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.group_out_id, conn.port_out_id, conn.group_in_id, conn.port_in_id)

    QTimer.singleShot(0, canvas.scene.update)

def joinGroup(group_id):
    if canvas.debug:
        print("PatchCanvas::joinGroup(%i)" % group_id)

    item   = None
    s_item = None
    group_name = ""
    group_icon = ICON_APPLICATION
    plugin_id  = -1
    plugin_ui  = False
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
            plugin_id  = group.plugin_id
            plugin_ui  = group.plugin_ui
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
            port_dict.is_alternate = port.is_alternate
            port_dict.widget = None
            ports_data.append(port_dict)

    for connection in canvas.connection_list:
        if connection.port_out_id in port_list_ids or connection.port_in_id in port_list_ids:
            connection_dict = connection_dict_t()
            connection_dict.connection_id = connection.connection_id
            connection_dict.group_in_id = connection.group_in_id
            connection_dict.port_in_id = connection.port_in_id
            connection_dict.group_out_id = connection.group_out_id
            connection_dict.port_out_id = connection.port_out_id
            connection_dict.widget = None
            conns_data.append(connection_dict)

    # Step 2 - Remove Item and Children
    for conn in conns_data:
        disconnectPorts(conn.connection_id)

    for port_id in port_list_ids:
        removePort(group_id, port_id)

    removeGroup(group_id)

    # Step 3 - Re-create Item, now together
    addGroup(group_id, group_name, SPLIT_NO, group_icon)

    if plugin_id >= 0:
        setGroupAsPlugin(group_id, plugin_id, plugin_ui)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type, port.is_alternate)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.group_out_id, conn.port_out_id, conn.group_in_id, conn.port_in_id)

    QTimer.singleShot(0, canvas.scene.update)

def getGroupPos(group_id, port_mode=PORT_MODE_OUTPUT):
    if canvas.debug:
        print("PatchCanvas::getGroupPos(%i, %s)" % (group_id, port_mode2str(port_mode)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            return group.widgets[1 if (group.split and port_mode == PORT_MODE_INPUT) else 0].pos()

    qCritical("PatchCanvas::getGroupPos(%i, %s) - unable to find group" % (group_id, port_mode2str(port_mode)))
    return QPointF(0, 0)

def saveGroupPositions():
    if canvas.debug:
        print("PatchCanvas::getGroupPositions()")

    ret = []

    for group in canvas.group_list:
        if group.split:
            pos1 = group.widgets[0].pos()
            pos2 = group.widgets[1].pos()
        else:
            pos1 = group.widgets[0].pos()
            pos2 = QPointF(0, 0)

        ret.append({
            "name" : group.group_name,
            "pos1x": pos1.x(),
            "pos1y": pos1.y(),
            "pos2x": pos2.x(),
            "pos2y": pos2.y(),
            "split": group.split,
        })

    return ret

def restoreGroupPositions(dataList):
    if canvas.debug:
        print("PatchCanvas::restoreGroupPositions(...)")

    map = {}

    for group in canvas.group_list:
        map[group.group_name] = group

    for data in dataList:
        name  = data['name']
        group = map.get(name, None)

        if group is None:
            continue

        group.widgets[0].setPos(data['pos1x'], data['pos1y'])

        if group.split and group.widgets[1]:
            group.widgets[1].setPos(data['pos2x'], data['pos2y'])

def setGroupPos(group_id, group_pos_x, group_pos_y):
    setGroupPosFull(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y)

def setGroupPosFull(group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i):
    if canvas.debug:
        print("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)" % (group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.widgets[0].setPos(group_pos_x_o, group_pos_y_o)

            if group.split and group.widgets[1]:
                group.widgets[1].setPos(group_pos_x_i, group_pos_y_i)

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i) - unable to find group to reposition" % (group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

def setGroupIcon(group_id, icon):
    if canvas.debug:
        print("PatchCanvas::setGroupIcon(%i, %s)" % (group_id, icon2str(icon)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.icon = icon
            group.widgets[0].setIcon(icon)

            if group.split and group.widgets[1]:
                group.widgets[1].setIcon(icon)

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::setGroupIcon(%i, %s) - unable to find group to change icon" % (group_id, icon2str(icon)))

def setGroupAsPlugin(group_id, plugin_id, hasUi):
    if canvas.debug:
        print("PatchCanvas::setGroupAsPlugin(%i, %i, %s)" % (group_id, plugin_id, bool2str(hasUi)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.plugin_id = plugin_id
            group.plugin_ui = hasUi
            group.widgets[0].setAsPlugin(plugin_id, hasUi)

            if group.split and group.widgets[1]:
                group.widgets[1].setAsPlugin(plugin_id, hasUi)
            return

    qCritical("PatchCanvas::setGroupAsPlugin(%i, %i, %s) - unable to find group to set as plugin" % (group_id, plugin_id, bool2str(hasUi)))

def addPort(group_id, port_id, port_name, port_mode, port_type, is_alternate=False):
    if canvas.debug:
        print("PatchCanvas::addPort(%i, %i, %s, %s, %s, %s)" % (group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type), bool2str(is_alternate)))

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
            port_widget = box_widget.addPortFromGroup(port_id, port_mode, port_type, port_name, is_alternate)
            break

    if not (box_widget and port_widget):
        qCritical("PatchCanvas::addPort(%i, %i, %s, %s, %s) - Unable to find parent group" % (group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))
        return

    port_dict = port_dict_t()
    port_dict.group_id  = group_id
    port_dict.port_id   = port_id
    port_dict.port_name = port_name
    port_dict.port_mode = port_mode
    port_dict.port_type = port_type
    port_dict.pair_id   = None
    port_dict.is_alternate = is_alternate
    port_dict.widget = port_widget
    canvas.port_list.append(port_dict)

    box_widget.updatePositions()
    
    pair_port_id_list = CanvasGetNewPairPortIdList(group_id, port_id, port_name, port_mode, port_type)
    if pair_port_id_list:
        CanvasAddPair(group_id, port_mode, port_type, pair_port_id_list)
    box_widget.updatePositions()
    
    if options.eyecandy == EYECANDY_FULL:
        return CanvasItemFX(port_widget, True, False)

    QTimer.singleShot(0, canvas.scene.update)

def removePort(group_id, port_id):
    if canvas.debug:
        print("PatchCanvas::removePort(%i, %i)" % (group_id, port_id))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            if port.pair_id:
                CanvasRemovePair(port.pair_id)
            item = port.widget
            item.parentItem().removePortFromGroup(port_id)
            canvas.scene.removeItem(item)
            canvas.port_list.remove(port)
            del item

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::removePort(%i, %i) - Unable to find port to remove" % (group_id, port_id))

def renamePort(group_id, port_id, new_port_name):
    if canvas.debug:
        print("PatchCanvas::renamePort(%i, %i, %s)" % (group_id, port_id, new_port_name.encode()))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            port.port_name = new_port_name
            port.widget.setPortName(new_port_name)
            port.widget.parentItem().updatePositions()

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::renamePort(%i, %i, %s) - Unable to find port to rename" % (group_id, port_id, new_port_name.encode()))

def connectPorts(connection_id, group_out_id, port_out_id, group_in_id, port_in_id):
    if canvas.debug:
        print("PatchCanvas::connectPorts(%i, %i, %i, %i, %i)" % (connection_id, group_out_id, port_out_id, group_in_id, port_in_id))

    port_out = None
    port_in  = None
    port_out_parent = None
    port_in_parent  = None

    for port in canvas.port_list:
        if port.group_id == group_out_id and port.port_id == port_out_id:
            port_out = port.widget
            port_out_parent = port_out.parentItem()
        elif port.group_id == group_in_id and port.port_id == port_in_id:
            port_in = port.widget
            port_in_parent = port_in.parentItem()

    # FIXME
    if not (port_out and port_in):
        qCritical("PatchCanvas::connectPorts(%i, %i, %i, %i, %i) - unable to find ports to connect" % (connection_id, group_out_id, port_out_id, group_in_id, port_in_id))
        return

    connection_dict = connection_dict_t()
    connection_dict.connection_id = connection_id
    connection_dict.group_in_id = group_in_id
    connection_dict.port_in_id = port_in_id
    connection_dict.group_out_id = group_out_id
    connection_dict.port_out_id = port_out_id

    if options.use_bezier_lines:
        connection_dict.widget = CanvasBezierLine(port_out, port_in, None)
    else:
        connection_dict.widget = CanvasLine(port_out, port_in, None)

    canvas.scene.addItem(connection_dict.widget)

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
        return CanvasItemFX(item, True, False)

    QTimer.singleShot(0, canvas.scene.update)

def disconnectPorts(connection_id):
    if canvas.debug:
        print("PatchCanvas::disconnectPorts(%i)" % connection_id)

    line  = None
    item1 = None
    item2 = None
    group1id = port1id = 0
    group2id = port2id = 0

    for connection in canvas.connection_list:
        if connection.connection_id == connection_id:
            group1id = connection.group_out_id
            group2id = connection.group_in_id
            port1id = connection.port_out_id
            port2id = connection.port_in_id
            line = connection.widget
            canvas.connection_list.remove(connection)
            break

    if not line:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find connection ports" % connection_id)
        return

    for port in canvas.port_list:
        if port.group_id == group1id and port.port_id == port1id:
            item1 = port.widget
            break

    if not item1:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find output port" % connection_id)
        return

    for port in canvas.port_list:
        if port.group_id == group2id and port.port_id == port2id:
            item2 = port.widget
            break

    if not item2:
        qCritical("PatchCanvas::disconnectPorts(%i) - unable to find input port" % connection_id)
        return

    item1.parentItem().removeLineFromGroup(connection_id)
    item2.parentItem().removeLineFromGroup(connection_id)

    if options.eyecandy == EYECANDY_FULL:
        return CanvasItemFX(line, False, True)

    canvas.scene.removeItem(line)
    del line

    QTimer.singleShot(0, canvas.scene.update)

def arrange():
    if canvas.debug:
        print("PatchCanvas::arrange()")

def updateZValues():
    if canvas.debug:
        print("PatchCanvas::updateZValues()")

    for group in canvas.group_list:
        group.widgets[0].resetLinesZValue()

        if group.split and group.widgets[1]:
            group.widgets[1].resetLinesZValue()

def handlePluginRemoved(plugin_id):
    if canvas.debug:
        print("PatchCanvas::handlePluginRemoved(%i)" % plugin_id)

    for group in canvas.group_list:
        if group.plugin_id < plugin_id or group.plugin_id > MAX_PLUGIN_ID_ALLOWED:
            continue

        group.plugin_id -= 1
        group.widgets[0].m_plugin_id -= 1

        if group.split and group.widgets[1]:
            group.widgets[1].m_plugin_id -= 1

def handleAllPluginsRemoved():
    if canvas.debug:
        print("PatchCanvas::handleAllPluginsRemoved()")

    for group in canvas.group_list:
        if group.plugin_id > MAX_PLUGIN_ID_ALLOWED:
            continue

        group.plugin_id = -1
        group.plugin_ui = False
        group.widgets[0].m_plugin_id = -1
        group.widgets[0].m_plugin_ui = False

        if group.split and group.widgets[1]:
            group.widgets[1].m_plugin_id = -1
            group.widgets[1].m_plugin_ui = False

# Extra Internal functions
def CanvasGetGroupName(group_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasGetGroupName(%i)" % group_id)

    for group in canvas.group_list:
        if group.group_id == group_id:
            return group.group_name

    qCritical("PatchCanvas::CanvasGetGroupName(%i) - unable to find group" % group_id)
    return ""

def CanvasGetNewGroupPos(horizontal):
    if canvas.debug:
        print("PatchCanvas::CanvasGetNewGroupPos(%s)" % bool2str(horizontal))

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

def CanvasIsPortMonoForced(port_id, port_mode, full_port_name):
    for forced_port in canvas.force_mono_list:
        if forced_port.port_id == port_id:
            return True
        elif (forced_port.port_mode == port_mode and
              forced_port.full_port_name == full_port_name):
            return True
        
    if canvas.settings.value("ForcedMonoPorts/%s" % full_port_name, PORT_MODE_NULL, type=int) == port_mode:
        return True
    
    return False

def CanvasGetNewPairPortIdList(group_id, port_id, port_name, port_mode, port_type):
    pair_port_id_list = []
    port_cousin_list  = []
    group_name = CanvasGetGroupName(group_id)
    
    if port_type != PORT_TYPE_AUDIO_JACK:
        return
    
    #get port_cousin_list, ports able to be in the same pair than pair_id
    for port in canvas.port_list:
        if port.group_id == group_id and port.port_mode == port_mode and port.port_type == port_type:
            if port.port_id != port_id:
                port_cousin_list.append(port)
            
    if len(port_cousin_list) < 1:
        return
    
    #check in canvas.saved_pair_list if a pair shall be added
    for saved_pair in canvas.saved_pair_list:
        mv_after_port_list = []
        n = 0
        #if port to add is the last of saved pair
        if len(saved_pair.port_id_list) >= 2 and port_id == saved_pair.port_id_list[-1]:
            for port in port_cousin_list:
                if port.port_id == saved_pair.port_id_list[n]:
                    n += 1
                    pair_port_id_list.append(port.port_id)
                elif n > 0:
                    mv_after_port_list.append(port)
            if len(saved_pair.port_id_list) == n+1:
                #re-order canvas.port_list to get ports between the paired ports after the pair
                for port in mv_after_port_list:
                    canvas.port_list.remove(port)
                    canvas.port_list.append(port)
                
                #add port to add to pair_port_id_list and send pair_port_id_list to addPort()
                pair_port_id_list.append(port_id)
                return pair_port_id_list
            break
        
        elif (saved_pair.group_name == group_name and
              saved_pair.port_mode  == port_mode  and
              len(saved_pair.port_name_list) >= 2 and 
              port_name == saved_pair.port_name_list[-1]):
                for port in port_cousin_list:
                    if len(saved_pair.port_name_list) > n and port.port_name == saved_pair.port_name_list[n]:
                        n += 1
                        pair_port_id_list.append(port.port_id)
                    elif n > 0:
                        mv_after_port_list.append(port)
                if len(saved_pair.port_id_list) == n+1:
                    for port in mv_after_port_list:
                        canvas.port_list.remove(port)
                        canvas.port_list.append(port)
                    pair_port_id_list.append(port_id)
                    return pair_port_id_list
                break
            
    pair_port_id_list = []
    
    #check in config_file if a pair shall be added
    canvas.settings.beginGroup("CanvasPairs")
    all_pair_nums = canvas.settings.childKeys()
    canvas.settings.endGroup()
    
    for pair_num_name in all_pair_nums:
        pair_tab = canvas.settings.value("CanvasPairs/%s" % pair_num_name, [])
        if (pair_tab.__class__() == [] and
            len(pair_tab) >= 4 and
            group_name == pair_tab[0] and
            port_mode  == int(pair_tab[1]) and
            port_name  == pair_tab[-1]):
                port_name_list = pair_tab[2:]
                mv_after_port_list = []
                n = 0
                for port in port_cousin_list:
                    if len(port_name_list) > n and port.port_name == port_name_list[n]:
                        n += 1
                        pair_port_id_list.append(port.port_id)
                    elif n > 0:
                        mv_after_port_list.append(port)
                if len(port_name_list) == n+1:
                    pair_port_id_list.append(port_id)
                    for port in mv_after_port_list:
                        canvas.port_list.remove(port)
                        canvas.port_list.append(port)
                    return pair_port_id_list
                break
        
    #make stereo_detection
    #if options.stereo_detection:
    if 0 == 1:
        if len(port_name) < 1:
            return
        
        #if port_cousin_list[-1].pair_id:
            #return
        
        may_match_list   = []
        
        #check if port to add has been forced as mono
        if CanvasIsPortMonoForced(port_id, port_mode, group_name + ':' + port_name):
            return
        
        #check if possibly left port has been forced as mono
        if CanvasIsPortMonoForced(port_cousin_list[-1].port_id, 
                                  port_cousin_list[-1].port_mode, 
                                  group_name + ':' + port_cousin_list[-1].port_name):
            return

        #if Port ends with digit
        if port_name[-1].isdigit():
            BasePort = port_name[:-1]
            InNum    = port_name[-1]

            while BasePort[-1].isdigit():
                InNum = BasePort[-1] + InNum
                BasePort = BasePort[:-1]
            
            #if Port ends with Ldigits or Rdigits
            if BasePort.endswith('R'):
                may_match_list.append(BasePort[:-1] + 'L' + InNum)
            else:
                may_match_list.append(BasePort + str(int(InNum) -1))
                
                if int(InNum) == 1 or int(InNum) == 2:
                    if BasePort.endswith(' ') or BasePort.endswith('_'):
                        may_match_list.append(BasePort[:-1])
                    else:
                        may_match_list.append(BasePort)
                        
        else:
            
            #Port ends with 'R'
            if port_name.endswith('R'):
                may_match_list.append(port_name[:-1] + 'L')
                if len(port_name) >= 2:
                    if port_name[-2] == ' ':
                        may_match_list.append(port_name[:-2])
                    else:
                        may_match_list.append(port_name[:-1])
            
            #Port ends with 'right'
            elif port_name.endswith('right'):
                may_match_list.append(port_name[:-5] + 'left')
            
            #Port ends with 'Right'
            elif port_name.endswith('Right'):
                may_match_list.append(port_name[:-5] + 'Left')
                
            elif port_name.endswith('(Right)'):
                may_match_list.append(port_name[:-7] + '(Left)')
                
            elif port_name.endswith('.r'):
                may_match_list.append(port_name[:-2] + '.l')
                
            for x in 'out', 'Out', 'output', 'Output', 'in', 'In', 'input', 'Input', 'audio input', 'audio output':
                if port_name.endswith('R ' + x):
                    may_match_list.append('L ' + x)
                elif port_name.endswith('right ' + x):
                    may_match_list.append('left ' + x)
                elif port_name.endswith('Right ' + x):
                    may_match_list.append('Left ' + x)
                    
        #for may_match in may_match_list:
            #if port_cousin_list[-1].port_name == may_match:
                #return [ port_cousin_list[-1].port_id, port_id ]
                
        for may_match in may_match_list:
            for port in port_cousin_list:
                if port.port_name == may_match:
                    if port.pair_id == None:
                        if port == port_cousin_list[-1]:
                            return [ port.port_id, port_id ]
                        else:
                            last_port = canvas.port_list[-1]
                            canvas.port_list = canvas.port_list[:-1]
                            index = canvas.port_list.index(port)
                            canvas.port_list.insert(index+1, last_port)
                            return [ port.port_id, port_id ]
                            
def CanvasAddPair(group_id, port_mode, port_type, port_id_list):
    box_widget  = None
    pair_widget = None
    pair_id = 0
    
    #if port in port_id_list is currently in a pair, then remove this pair. 
    for port in canvas.port_list:
        if port.port_id in port_id_list:
            if port.pair_id:
                if canvas.debug:
                    qDebug("PatchCanvas::CanvasAddPair RemoveAPair(%i:%i)" % (port.pair_id, port.port_id))
                CanvasRemovePair(port.pair_id)
                break
    
    #get new pair_id
    pair_list_ids = []
    for pair in canvas.pair_list:
        pair_list_ids.append(pair.pair_id)
        
    pair_list_ids.sort()
    
    if len(pair_list_ids) == 0:
        pair_id = 1
    else:
        pair_id = pair_list_ids[-1] +1
    
    if canvas.debug:
        qDebug("PatchCanvas::CanvasAddPair(%i)" % pair_id)
    
    #add pair widget
    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split and group.widgets[0].getSplittedMode() != port_mode and group.widgets[1]:
                n = 1
            else:
                n = 0
            box_widget  = group.widgets[n]
            pair_widget = box_widget.addPairFromGroup(pair_id, port_mode, port_type, port_id_list)
            break
    
    pair_dict = pair_dict_t()
    pair_dict.pair_id      = pair_id
    pair_dict.group_id     = group_id
    pair_dict.port_mode    = port_mode
    pair_dict.port_type    = port_type
    pair_dict.port_id_list = port_id_list
    pair_dict.widget       = pair_widget
    canvas.pair_list.append(pair_dict)
    
    zpair = pair_widget.zValue()
    
    for port in canvas.port_list:
        if port.port_id in port_id_list:
            port.pair_id = pair_id
            port.widget.setPairId(pair_id)
            port.widget.setZValue(zpair +1)
    box_widget.updatePositions()

    QTimer.singleShot(0, canvas.scene.update)

def CanvasRemovePair(pair_id):
    if canvas.debug:
        qDebug("PatchCanvas::CanvasRemovePair(%i)" % pair_id)

    for pair in canvas.pair_list:
        if pair.pair_id == pair_id:
            for port in canvas.port_list:
                if port.port_id in pair.port_id_list:
                    port.pair_id = None
                    port.widget.setPairId(None)
            
            item = pair.widget
            item.parentItem().updatePositions()
            canvas.scene.removeItem(item)
            del item

            canvas.pair_list.remove(pair)
            
            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::CanvasRemovePair(%i) - Unable to find pair to remove" % pair_id)
    
def CanvasGetPortsOfPairList(pair_id):
    for pair in canvas.pair_list:
        if pair.pair_id == pair_id:
            return pair.port_id_list
    qCritical("PatchCanvas::CanvasGetPortsOfPairList(%i) - unable to find pair" % pair_id)

def CanvasGetPortPositionAndPairLenght(port_id, pair_id):
    if pair_id:
        for pair in canvas.pair_list:
            if pair.pair_id == pair_id:
                if port_id in pair.port_id_list:
                    return ( pair.port_id_list.index(port_id), len(pair.port_id_list) )
    return ( None, 0 )
                
def CanvasGetPairName(ports_ids_list):
    ports_names = []
    
    for port in canvas.port_list:
        if port.port_id in ports_ids_list:
            ports_names.append(port.port_name)
    
    if len(ports_names) < 2:
        return ''
    
    pair_name_ends = ( ' ', '_', '.', '-', '#', ':', 'out', 'in', 'Out', 'In', 'Output', 'Input', 'output', 'input' )
    
    #set pair name
    pair_name = ''
    checkCharacter = True
    
    for c in ports_names[0]:        
        for eachname in ports_names:
            if not eachname.startswith(pair_name + c):
                checkCharacter = False
                break
        if not checkCharacter:
            break
        pair_name += c
    
    #reduce pair name until it ends with one of the characters in pair_name_ends
    check = False
    while not check:
        for x in pair_name_ends:
            if pair_name.endswith(x):
                check = True
                break
        
        if len(pair_name) == 0 or pair_name in ports_names:
                check = True
            
        if not check:
            pair_name = pair_name[:-1]
    
    return pair_name
        
def CanvasGetPortPrintName(port_id, pair_id):
    for pair in canvas.pair_list:
        if pair.pair_id == pair_id:
            pair_name = CanvasGetPairName(pair.port_id_list)
            for port in canvas.port_list:
                if port.port_id == port_id:
                    return port.port_name.replace(pair_name, '', 1)

def CanvasGetPairFullName(pair_id):
    for pair in canvas.pair_list:
        if pair.pair_id == pair_id:
            group_name       = CanvasGetGroupName(pair.group_id)
            
            endofname = ''
            for port_id in pair.port_id_list:
                endofname += CanvasGetPortPrintName(port_id, pair.pair_id) + '/'
            pair_name = CanvasGetPairName(pair.port_id_list)
            
            return group_name + ':' + pair_name + ' ' + endofname[:-1]

def CanvasSplitPair(pair_id):
    pair_port_id_list = CanvasGetPortsOfPairList(pair_id)
    pair_ports_names = []
    
    for port in canvas.port_list:
        if port.port_id in pair_port_id_list:
            pair_ports_names.append(port.port_name)
            #full_port_name = CanvasGetFullPortName(port.port_id)
            group_name = CanvasGetGroupName(port.group_id)
            #if options.stereo_detection:
                #canvas.settings.setValue("ForcedMonoPorts/%s:%s" % (group_name, port.port_name), port.port_mode)
            #canvas.callback(ACTION_SAVE_MONO_PORT, port.port_id, port.port_mode, group_name + ':' + port.port_name)
            
            canvas.settings.beginGroup("CanvasPairs")
            all_pair_nums = canvas.settings.childKeys()
            canvas.settings.endGroup()
            
            for pair_num_name in all_pair_nums:
                pair_tab = canvas.settings.value("CanvasPairs/%s" % pair_num_name, [])
                if (len(pair_tab) >= 3 and 
                    group_name == pair_tab[0] and 
                    port.port_mode == int(pair_tab[1]) and 
                    port.port_name in pair_tab[2:]):
                        canvas.settings.remove("CanvasPairs/%s" % pair_num_name)
                        break
                
    CanvasRemovePair(pair_id)
    
def CanvasGetPairName(ports_ids_list):
    ports_names = []
    
    for port in canvas.port_list:
        if port.port_id in ports_ids_list:
            ports_names.append(port.port_name)
    
    if len(ports_names) < 2:
        return ''
    
    pair_name_ends = ( ' ', '_', '.', '-', '#', ':', 'out', 'in', 'Out', 'In', 'Output', 'Input', 'output', 'input' )
    
    #set pair name
    pair_name = ''
    checkCharacter = True
    
    for c in ports_names[0]:        
        for eachname in ports_names:
            if not eachname.startswith(pair_name + c):
                checkCharacter = False
                break
        if not checkCharacter:
            break
        pair_name += c
    
    #reduce pair name until it ends with one of the characters in pair_name_ends
    check = False
    while not check:
        for x in pair_name_ends:
            if pair_name.endswith(x):
                check = True
                break
        
        if len(pair_name) == 0 or pair_name in ports_names:
                check = True
            
        if not check:
            pair_name = pair_name[:-1]
    
    return pair_name

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
    
    for pair in canvas.pair_list:
        if pair.widget.isSelected():
            for connection in canvas.connection_list:
                if connection.port_out_id in pair.port_id_list or connection.port_in_id in pair.port_id_list:
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

# ------------------------------------------------------------------------------
# rubberbandrect.cpp

class RubberbandRect(QGraphicsRectItem):
    def __init__(self, scene):
        QGraphicsRectItem.__init__(self, QRectF(0, 0, 0, 0))

        self.setZValue(-1)
        self.hide()

        scene.addItem(self)

    def type(self):
        return CanvasRubberbandType

# ------------------------------------------------------------------------------
# patchscene.cpp

class PatchScene(QGraphicsScene):
    scaleChanged    = pyqtSignal(float)
    sceneGroupMoved = pyqtSignal(int, int, QPointF)
    pluginSelected  = pyqtSignal(list)

    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_ctrl_down = False
        self.m_mouse_down_init = False
        self.m_mouse_rubberband = False
        self.m_mid_button_down = False
        self.m_pointer_border = QRectF(0.0, 0.0, 1.0, 1.0)

        self.m_rubberband = RubberbandRect(self)
        self.m_rubberband_selection = False
        self.m_rubberband_orig_point = QPointF(0, 0)

        self.m_view = view
        if not self.m_view:
            qFatal("PatchCanvas::PatchScene() - invalid view")

        self.selectionChanged.connect(self.slot_selectionChanged)

    def fixScaleFactor(self):
        scale = self.m_view.transform().m11()
        if scale > 3.0:
            self.m_view.resetTransform()
            self.m_view.scale(3.0, 3.0)
        elif scale < 0.2:
            self.m_view.resetTransform()
            self.m_view.scale(0.2, 0.2)
        self.scaleChanged.emit(self.m_view.transform().m11())

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
                        first_value = False
                        min_x = pos.x()
                        min_y = pos.y()
                        max_x = pos.x() + rect.width()
                        max_y = pos.y() + rect.height()
                    else:
                        min_x = min(min_x, pos.x())
                        min_y = min(min_y, pos.y())
                        max_x = max(max_x, pos.x() + rect.width())
                        max_y = max(max_y, pos.y() + rect.height())

            if not first_value:
                self.m_view.fitInView(min_x, min_y, abs(max_x - min_x), abs(max_y - min_y), Qt.KeepAspectRatio)
                self.fixScaleFactor()

    def zoom_in(self):
        if self.m_view.transform().m11() < 3.0:
            self.m_view.scale(1.2, 1.2)
        self.scaleChanged.emit(self.m_view.transform().m11())

    def zoom_out(self):
        if self.m_view.transform().m11() > 0.2:
            self.m_view.scale(0.8, 0.8)
        self.scaleChanged.emit(self.m_view.transform().m11())

    def zoom_reset(self):
        self.m_view.resetTransform()
        self.scaleChanged.emit(1.0)

    @pyqtSlot()
    def slot_selectionChanged(self):
        items_list = self.selectedItems()

        if len(items_list) == 0:
            self.pluginSelected.emit([])
            return

        plugin_list = []

        for item in items_list:
            if item and item.isVisible():
                group_item = None

                if item.type() == CanvasBoxType:
                    group_item = item
                elif item.type() == CanvasPortType:
                    group_item = item.parentItem()
                elif item.type() == CanvasStereoPairType:
                    group_item = item.parentItem()
                #elif item.type() in (CanvasLineType, CanvasBezierLineType, CanvasLineMovType, CanvasBezierLineMovType):
                    #plugin_list = []
                    #break

                if group_item is not None and group_item.m_plugin_id >= 0:
                    plugin_id = group_item.m_plugin_id
                    if plugin_id > MAX_PLUGIN_ID_ALLOWED:
                        plugin_id = 0
                    plugin_list.append(plugin_id)

        self.pluginSelected.emit(plugin_list)

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
        if event.button() == Qt.MidButton and self.m_ctrl_down:
            self.m_mid_button_down = True
            pos = event.scenePos()
            self.m_pointer_border.moveTo(floor(pos.x()), floor(pos.y()))

            items = self.items(self.m_pointer_border)
            for item in items:
                if item and item.type() in [CanvasLineType, CanvasBezierLineType]:
                    item.triggerDisconnect()
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
            x = min(pos.x(), self.m_rubberband_orig_point.x())
            y = min(pos.y(), self.m_rubberband_orig_point.y())

            lineHinting = canvas.theme.rubberband_pen.widthF() / 2
            self.m_rubberband.setRect(x+lineHinting,
                                      y+lineHinting,
                                      abs(pos.x() - self.m_rubberband_orig_point.x()),
                                      abs(pos.y() - self.m_rubberband_orig_point.y()))
            return event.accept()

        if self.m_mid_button_down and self.m_ctrl_down:
            trail = QPolygonF([event.scenePos(), event.lastScenePos(), event.scenePos()])
            items = self.items(trail)
            for item in items:
                if item and item.type() in [CanvasLineType, CanvasBezierLineType]:
                    item.triggerDisconnect()

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
                    self.sceneGroupMoved.emit(item.getGroupId(), item.getSplittedMode(), item.scenePos())

            if len(items_list) > 1:
                canvas.scene.update()

        self.m_mouse_down_init  = False
        self.m_mouse_rubberband = False

        if event.button() == Qt.MidButton:
            self.m_mid_button_down = False
            return event.accept()

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

    def contextMenuEvent(self, event):
        if len(self.selectedItems()) == 0:
            event.accept()
            canvas.callback(ACTION_BG_RIGHT_CLICK, 0, 0, "")
        else:
            QGraphicsScene.contextMenuEvent(self, event)

# ------------------------------------------------------------------------------
# canvasfadeanimation.cpp

class CanvasFadeAnimation(QAbstractAnimation):
    def __init__(self, item, show):
        QAbstractAnimation.__init__(self)

        self.m_show = show
        self.m_duration = 0
        self.m_item = item

    def item(self):
        return self.m_item

    def forceStop(self):
        self.blockSignals(True)
        self.stop()

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

        try:
            self.m_item.setOpacity(value)
        except:
            print("Error: failed to animate canvas item, already destroyed?")
            self.forceStop()
            canvas.animation_list.remove(self)
            return

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
        QGraphicsLineItem.__init__(self, parent)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False
        self.m_ready_to_disc = False

        self.setGraphicsEffect(None)
        self.updateLinePos()
    
    def isReadyToDisc(self):
        return self.m_ready_to_disc
    
    def setReadyToDisc(self, yesno):
        self.m_ready_to_disc = yesno
    
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

    def triggerDisconnect(self):
        for connection in canvas.connection_list:
            if (connection.port_out_id == self.item1.getPortId() and connection.port_in_id == self.item2.getPortId()):
                canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                break

    def updateLinePos(self):
        if self.item1.getPortMode() == PORT_MODE_OUTPUT:
            pppl1 = CanvasGetPortPositionAndPairLenght(self.item1.getPortId(), self.item1.getPairId())
            port_posinpair1, pair_lenght1 = pppl1[0], pppl1[1]
            
            if pair_lenght1:
                if pair_lenght1 > 2:
                    phi = 0.75
                else:
                    phi = 0.62
            
            if pair_lenght1 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (pair_lenght1 - phi)
                delta = (last_old_y - first_old_y) / (pair_lenght1 -1)
                Y1 = first_old_y + (port_posinpair1 * delta) - (canvas.theme.port_height * port_posinpair1)
            else:
                Y1 = canvas.theme.port_height / 2
                
            pppl2 = CanvasGetPortPositionAndPairLenght(self.item2.getPortId(), self.item2.getPairId())
            port_posinpair2, pair_lenght2 = pppl2[0], pppl2[1]
            
            if pair_lenght2:
                if pair_lenght2 > 2:
                    phi = 0.75
                else:
                    phi = 0.62
            
            if pair_lenght2 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (pair_lenght2 - phi)
                delta = (last_old_y - first_old_y) / (pair_lenght2 -1)
                Y2 = first_old_y + (port_posinpair2 * delta) - (canvas.theme.port_height * port_posinpair2)
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
        QGraphicsPathItem.__init__(self, parent)

        self.item1 = item1
        self.item2 = item2

        self.m_locked = False
        self.m_lineSelected = False
        self.m_ready_to_disc = False

        self.setBrush(QColor(0, 0, 0, 0))
        self.setGraphicsEffect(None)
        self.updateLinePos()
    
    
    def isReadyToDisc(self):
        return self.m_ready_to_disc
    
    def setReadyToDisc(self, yesno):
        self.m_ready_to_disc = yesno
    
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

    def triggerDisconnect(self):
        for connection in canvas.connection_list:
            if (connection.port_out_id == self.item1.getPortId() and connection.port_in_id == self.item2.getPortId()):
                canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                break

    def updateLinePos(self):
        if self.item1.getPortMode() == PORT_MODE_OUTPUT:
            item1_x = self.item1.scenePos().x() + self.item1.getPortWidth() + 12
            
            port_posinpair1, pair_lenght1 = CanvasGetPortPositionAndPairLenght(self.item1.getPortId(), self.item1.getPairId())
            
            if pair_lenght1 > 2:
                phi = 0.75
            else:
                phi = 0.62
            
            if pair_lenght1 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (pair_lenght1 - phi)
                delta = (last_old_y - first_old_y) / (pair_lenght1 -1)
                old_y1 = first_old_y + (port_posinpair1 * delta) - (canvas.theme.port_height * port_posinpair1)
            else:
                old_y1 = canvas.theme.port_height / 2
            
            item1_y = self.item1.scenePos().y() + old_y1
            
            item2_x = self.item2.scenePos().x()
            
            port_posinpair2, pair_lenght2 = CanvasGetPortPositionAndPairLenght(self.item2.getPortId(), self.item2.getPairId())
            
            if pair_lenght2 > 2:
                phi = 0.75
            else:
                phi = 0.62
            
            if pair_lenght2 > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (pair_lenght2 - phi)
                delta = (last_old_y - first_old_y) / (pair_lenght2 -1)
                old_y2 = first_old_y + (port_posinpair2 * delta) - (canvas.theme.port_height * port_posinpair2)
            else:
                old_y2 = canvas.theme.port_height / 2
                
            item2_y = self.item2.scenePos().y() + old_y2

            item1_mid_x = abs(item1_x - item2_x) / 2
            item1_new_x = item1_x + item1_mid_x

            item2_mid_x = abs(item1_x - item2_x) / 2
            item2_new_x = item2_x - item2_mid_x
            
            #if 0 == 0:
                #item1_new_y, item2_new_y = item1_y, item2_y
                #addLine = False
                
                #diffxy = abs(item1_y - item2_y) - 1 * (abs(item1_x - item2_x))
                #if diffxy > 0:
                    #if abs(item1_y - item2_y) > 50:
                        #item1_new_x += abs(diffxy)
                        #item2_new_x -= abs(diffxy)

                    #if abs(item1_y - item2_y) > 50:
                        #while abs(item1_new_x - item2_x) < 50:
                            #item1_new_x += 10
                            #item2_new_x -= 10
                    
                    
                    
                #if item1_x - item2_x > 0:
                    #if item1_new_x - item1_x < 80:
                        #item1_new_x = item1_x + 80
                        #item2_new_x = item2_x - 80
                    #elif item1_new_x - item1_x > 300:
                        #item1_new_x = item1_x + 300
                        #item2_new_x = item2_x - 300
                        
                    ##correction de la ligne droite
                    #diff_y = abs(item1_y - item2_y)
                    #if diff_y <= 200:
                        #if diff_y < 5:
                            #diff_y = 5
                        #item1_new_y -= 50 / (log(diff_y) *log(diff_y))
                        #item2_new_y += 50 / (log(diff_y) * log(diff_y))
            
            path = QPainterPath(QPointF(item1_x, item1_y))
            #path.cubicTo(item1_new_x, item1_new_y, item2_new_x, item2_new_y, item2_x, item2_y)
            path.cubicTo(item1_new_x, item1_y, item2_new_x, item2_y, item2_x, item2_y)
            self.setPath(path)

            self.m_lineSelected = False
            #self.setOpacity(0.)
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

        self.setPen(QPen(port_gradient, 2))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsPathItem.paint(self, painter, option, widget)
        painter.restore()

# ------------------------------------------------------------------------------
# canvaslivemov.cpp

class CanvasLineMov(QGraphicsLineItem):
    def __init__(self, port_mode, port_type, port_posinpair, pair_lenght, parent):
        QGraphicsLineItem.__init__(self)
        self.setParentItem(parent)

        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_posinpair    = port_posinpair
        self.m_port_posinpair_to = port_posinpair
        self.m_pair_lenght       = pair_lenght
        self.m_pair_lenght_to    = pair_lenght
        self.m_ready_to_disc  = False
        
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
            qWarning("PatchCanvas::CanvasLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)
        
        self.setPen(pen)
    
    def setPortPosInPairTo(self, port_posinpair_to):
        self.m_port_posinpair_to = port_posinpair_to
        
    def setPairLenghtTo(self, pair_lenght):
        self.m_pair_lenght_to = pair_lenght
    
    def isReadyToDisc(self):
        return self.m_ready_to_disc
    
    def setReadyToDisc(self, yesno):
        self.m_ready_to_disc = yesno
    
    def toggleReadyToDisc(self):
        self.m_ready_to_disc = not bool(self.m_ready_to_disc)
    
    def updateLinePos(self, scenePos):
        if self.m_ready_to_disc:
            if self.m_port_type == PORT_TYPE_AUDIO_JACK:
                pen = QPen(canvas.theme.line_audio_jack, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_JACK:
                pen = QPen(canvas.theme.line_midi_jack, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_A2J:
                pen = QPen(canvas.theme.line_midi_a2j, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
                pen = QPen(canvas.theme.line_midi_alsa, 2, Qt.DotLine)
            else:
                qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
                pen = QPen(Qt.black)
        else:
            if self.m_port_type == PORT_TYPE_AUDIO_JACK:
                pen = QPen(canvas.theme.line_audio_jack, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_JACK:
                pen = QPen(canvas.theme.line_midi_jack, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_A2J:
                pen = QPen(canvas.theme.line_midi_a2j, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
                pen = QPen(canvas.theme.line_midi_alsa, 2)
            else:
                qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
                pen = QPen(Qt.black)
        self.setPen(pen)
        
        phi = 0.75 if self.m_pair_lenght > 2 else 0.62
        phito = 0.75 if self.m_pair_lenght_to > 2 else 0.62
        
        item_pos = [0, 0]

        if self.m_port_mode == PORT_MODE_INPUT:
            item_pos[0] = 0
        elif self.m_port_mode == PORT_MODE_OUTPUT:
            item_pos[0] = self.p_width + 12
        else:
            return
        
        if self.parentItem().type() == CanvasPortType:
            if self.m_pair_lenght:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (self.m_pair_lenght - phi)
                delta = (last_old_y - first_old_y) / (self.m_pair_lenght -1)
                item_pos[1] = first_old_y + (self.m_port_posinpair * delta) - (canvas.theme.port_height * self.m_port_posinpair)
            else:
                item_pos[1] = float(canvas.theme.port_height)/2
        
        elif self.parentItem().type() == CanvasStereoPairType:
            first_old_y = canvas.theme.port_height * phi
            last_old_y  = canvas.theme.port_height * (self.m_pair_lenght - phi)
            delta = (last_old_y - first_old_y) / (self.m_pair_lenght -1)
            item_pos[1] = first_old_y + (self.m_port_posinpair * delta)
            
        if self.m_port_posinpair_to == PORT_IN_PAIR_POSITION_MONO:
                mouse_y_offset = 0
        else:
            first_new_y = canvas.theme.port_height * phito
            last_new_y  = canvas.theme.port_height * (self.m_pair_lenght_to - phito)
            delta = (last_new_y - first_new_y) / (self.m_pair_lenght_to -1)
            new_y1 = first_new_y + (self.m_port_posinpair_to * delta)
            mouse_y_offset = new_y1 - ( (last_new_y - first_new_y) / 2 ) - (canvas.theme.port_height * phito)
        
        line = QLineF(item_pos[0], item_pos[1], scenePos.x() - self.p_lineX, scenePos.y() - self.p_lineY + mouse_y_offset)
        self.setLine(line)

    def type(self):
        return CanvasLineMovType

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing))
        QGraphicsLineItem.paint(self, painter, option, widget)
        painter.restore()
        
    def setPortPosInPairTo(self, port_posinpair_to):
        self.m_port_posinpair_to = port_posinpair_to
        
    def setPairLenghtTo(self, pair_lenght):
        self.m_pair_lenght_to = pair_lenght

# ------------------------------------------------------------------------------
# canvasbezierlinemov.cpp

class CanvasBezierLineMov(QGraphicsPathItem):
    def __init__(self, port_mode, port_type, port_posinpair, pair_lenght, parent):
        QGraphicsPathItem.__init__(self)
        self.setParentItem(parent)

        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_posinpair    = port_posinpair
        self.m_port_posinpair_to = port_posinpair
        self.m_pair_lenght       = pair_lenght
        self.m_pair_lenght_to    = pair_lenght
        self.m_ready_to_disc = False
        
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
            qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
            pen = QPen(Qt.black)

        self.setBrush(QColor(0, 0, 0, 0))
        self.setPen(pen)
    
    def setPortPosInPairTo(self, port_posinpair_to):
        self.m_port_posinpair_to = port_posinpair_to
        
    def setPairLenghtTo(self, pair_lenght):
        self.m_pair_lenght_to = pair_lenght
    
    def isReadyToDisc(self):
        return self.m_ready_to_disc
    
    def setReadyToDisc(self, yesno):
        self.m_ready_to_disc = yesno
    
    def toggleReadyToDisc(self):
        self.m_ready_to_disc = not bool(self.m_ready_to_disc)
    
    def setPortPosInPairTo(self, port_posinpair_to):
        self.m_port_posinpair_to = port_posinpair_to
      
    def setPairLenghtTo(self, pair_lenght):
        self.m_pair_lenght_to = pair_lenght
    
    def updateLinePos(self, scenePos):
        if self.m_ready_to_disc:
            if self.m_port_type == PORT_TYPE_AUDIO_JACK:
                pen = QPen(canvas.theme.line_audio_jack, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_JACK:
                pen = QPen(canvas.theme.line_midi_jack, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_A2J:
                pen = QPen(canvas.theme.line_midi_a2j, 2, Qt.DotLine)
            elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
                pen = QPen(canvas.theme.line_midi_alsa, 2, Qt.DotLine)
            else:
                qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
                pen = QPen(Qt.black)
        else:
            if self.m_port_type == PORT_TYPE_AUDIO_JACK:
                pen = QPen(canvas.theme.line_audio_jack, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_JACK:
                pen = QPen(canvas.theme.line_midi_jack, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_A2J:
                pen = QPen(canvas.theme.line_midi_a2j, 2)
            elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
                pen = QPen(canvas.theme.line_midi_alsa, 2)
            else:
                qWarning("PatchCanvas::CanvasBezierLineMov(%s, %s, %s) - invalid port type" % (port_mode2str(port_mode), port_type2str(port_type), parent))
                pen = QPen(Qt.black)
        self.setPen(pen)
        
        phi = 0.75 if self.m_pair_lenght > 2 else 0.62
        phito = 0.75 if self.m_pair_lenght_to > 2 else 0.62
                
        if self.parentItem().type() == CanvasPortType:
            if self.m_pair_lenght > 1:
                first_old_y = canvas.theme.port_height * phi
                last_old_y  = canvas.theme.port_height * (self.m_pair_lenght - phi)
                delta = (last_old_y - first_old_y) / (self.m_pair_lenght -1)
                old_y = first_old_y + (self.m_port_posinpair * delta) - (canvas.theme.port_height * self.m_port_posinpair)
            else:
                old_y = canvas.theme.port_height / 2
                
            if self.m_port_posinpair_to == PORT_IN_PAIR_POSITION_MONO:
                new_y = 0
            else:
                first_new_y = canvas.theme.port_height * phito
                last_new_y  = canvas.theme.port_height * (self.m_pair_lenght_to - phito)
                delta = (last_new_y - first_new_y) / (self.m_pair_lenght_to -1)
                new_y1 = first_new_y + (self.m_port_posinpair_to * delta)
                new_y = new_y1 - ( (last_new_y - first_new_y) / 2 ) - (canvas.theme.port_height * phito)
                
                
            if self.m_port_mode == PORT_MODE_INPUT:
                old_x = 0
                mid_x = abs(scenePos.x() - self.p_itemX) / 2
                new_x = old_x - mid_x
            elif self.m_port_mode == PORT_MODE_OUTPUT:
                old_x = self.p_width + 12
                mid_x = abs(scenePos.x() - (self.p_itemX + old_x)) / 2
                new_x = old_x + mid_x
            else:
                return

            final_x = scenePos.x() - self.p_itemX
            final_y = scenePos.y() - self.p_itemY + new_y

            path = QPainterPath(QPointF(old_x, old_y))
            path.cubicTo(new_x, old_y, new_x, final_y, final_x, final_y)
            self.setPath(path)
            
        elif self.parentItem().type() == CanvasStereoPairType:
            first_old_y = canvas.theme.port_height * phi
            last_old_y  = canvas.theme.port_height * (self.m_pair_lenght - phi)
            delta = (last_old_y - first_old_y) / (self.m_pair_lenght -1)
            old_y = first_old_y + (self.m_port_posinpair * delta)
            
            if self.m_port_posinpair_to == PORT_IN_PAIR_POSITION_MONO:
                new_y = 0
            elif self.m_port_posinpair_to == self.m_port_posinpair and self.m_pair_lenght == self.m_pair_lenght_to:
                new_y = old_y - ( (last_old_y - first_old_y) / 2 ) - (canvas.theme.port_height * phi)
            else:
                first_new_y = canvas.theme.port_height * phito
                last_new_y  = canvas.theme.port_height * (self.m_pair_lenght_to - phito)
                delta = (last_new_y - first_new_y) / (self.m_pair_lenght_to -1)
                new_y1 = first_new_y + (self.m_port_posinpair_to * delta)
                new_y = new_y1 - ( (last_new_y - first_new_y) / 2 ) - (canvas.theme.port_height * phito)
            
            if self.m_port_mode == PORT_MODE_INPUT:
                old_x = 0
                mid_x = abs(scenePos.x() - self.p_itemX) / 2
                new_x = old_x - mid_x
            elif self.m_port_mode == PORT_MODE_OUTPUT:
                old_x = self.p_width + 12
                mid_x = abs(scenePos.x() - (self.p_itemX + old_x)) / 2
                new_x = old_x + mid_x
            else:
                return
            
            final_x = scenePos.x() - self.p_itemX - 1
            final_y = scenePos.y() - self.p_itemY + new_y

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
    def __init__(self, group_id, port_id, port_name, port_mode, port_type, is_alternate, parent):
        QGraphicsItem.__init__(self, parent)

        # Save Variables, useful for later
        self.m_group_id  = group_id
        self.m_port_id   = port_id
        self.m_port_mode = port_mode
        self.m_port_type = port_type
        self.m_port_name = port_name
        self.m_pair_id   = None
        self.m_is_alternate = is_alternate

        # Base Variables
        self.m_port_width  = 15
        self.m_port_height = canvas.theme.port_height
        self.m_port_font = QFont()
        self.m_port_font.setFamily(canvas.theme.port_font_name)
        self.m_port_font.setPixelSize(canvas.theme.port_font_size)
        self.m_port_font.setWeight(canvas.theme.port_font_state)

        self.m_line_mov_list = []
        self.m_dotcon_list = []
        self.m_hover_item = None
        self.m_last_selected_state = False

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
    
    def getPairId(self):
        return self.m_pair_id
    
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
    
    def setPairId(self, pair_id):
        self.m_pair_id = pair_id
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
    
    def deleteLine2(self):
        for line_mov in self.m_line_mov_list:
            if self.m_line_mov_list.index(line_mov) > 0:
                canvas.scene.removeItem(line_mov)
            else:
                line_mov.setPortPosInPairTo(PORT_IN_PAIR_POSITION_MONO)
        self.m_line_mov_list = self.m_line_mov_list[:1]
    
    def resetDotLines(self):
        for connection in self.m_dotcon_list:
            if connection.widget.isReadyToDisc():
                connection.widget.setReadyToDisc(False)
                connection.widget.updateLineGradient()
                
        for line_mov in self.m_line_mov_list:
            line_mov.setReadyToDisc(False)
        self.m_dotcon_list.clear()
    
    def SetAsStereo(self, port_id):
        port_id_list = []
        for port in canvas.port_list:
            if port.port_id in (self.m_port_id, port_id):
                port_id_list.append(port.port_id)
        
        CanvasAddPair(self.m_group_id, self.m_port_mode, self.m_port_type, port_id_list )
        group_name = CanvasGetGroupName(self.m_group_id)
        port_name_list = []
        
        # get new pair_num
        canvas.settings.beginGroup("CanvasPairs")
        all_pair_nums = canvas.settings.childKeys()
        canvas.settings.endGroup()
        
        n = 1
        new_pair_num_name = 'Pair_%i' % n
        while new_pair_num_name in all_pair_nums:
            n +=1
            new_pair_num_name = 'Pair_%i' % n
            
        port_name_list = []
        for port in canvas.port_list:
            if port.port_id in port_id_list:
                port_name_list.append(port.port_name)
                canvas.settings.remove("ForcedMonoPorts/%s:%s" % (group_name, port.port_name))
                
        pair_tab = [ group_name, self.m_port_mode ] + port_name_list
        canvas.settings.setValue("CanvasPairs/%s" % new_pair_num_name, pair_tab)
        
        #canvas.callback(ACTION_SAVE_PAIR, port_id_list.copy(), (self.m_port_mode, group_name, port_name_list), '')
        
        self.parentItem().updatePositions()
    
    def connectToHover(self):
        if self.m_hover_item:
            if self.m_hover_item.type() == CanvasPortType:
                hover_port_id_list = [ self.m_hover_item.getPortId() ]
            elif self.m_hover_item.type() == CanvasStereoPairType:
                hover_port_id_list = self.m_hover_item.getPortsList()
                
            con_list = []
            ports_connected_list = []
            
            for port in canvas.port_list:
                if port.port_id in hover_port_id_list:
                    hover_group_id = port.group_id
                    break
            else:
                return
                    
            for porthover_id in hover_port_id_list:
                for connection in canvas.connection_list:
                    if ( (connection.port_out_id == self.m_port_id and connection.port_in_id == porthover_id) or
                         (connection.port_out_id == porthover_id and connection.port_in_id == self.m_port_id) ):
                            con_list.append(connection)
                            ports_connected_list.append(porthover_id)
            
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
        if self.m_mouse_down:
            if not self.m_cursor_moving:
                self.setCursor(QCursor(Qt.CrossCursor))
                self.m_cursor_moving = True

                for connection in canvas.connection_list:
                    if ((connection.group_out_id == self.m_group_id and connection.port_out_id == self.m_port_id) or
                        (connection.group_in_id  == self.m_group_id and connection.port_in_id  == self.m_port_id)):
                        connection.widget.setLocked(True)
            
            if not self.m_line_mov_list:
                port_posinpair, pair_lenght = CanvasGetPortPositionAndPairLenght(self.m_port_id, self.m_pair_id)
                
                if options.use_bezier_lines:
                    line_mov = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                else:
                    line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                
                line_mov.setPortPosInPairTo(PORT_IN_PAIR_POSITION_MONO)
                self.m_line_mov_list.append(line_mov)
                canvas.is_line_mov = True
                line_mov.setZValue(canvas.last_z_value)
                canvas.last_z_value += 1
                self.parentItem().setZValue(canvas.last_z_value)

            item = None
            items = canvas.scene.items(event.scenePos(), Qt.ContainsItemShape, Qt.AscendingOrder)
            for i in range(len(items)):
                if items[i].type() in (CanvasPortType, CanvasStereoPairType):
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
                    if item.type() == CanvasStereoPairType:
                        if self.m_hover_item != item:
                            self.m_hover_item = item
                            self.resetDotLines()
                            
                            if len(self.m_line_mov_list) <= 1:
                                #make original line going to first port of the hover pair
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setPortPosInPairTo(PORT_IN_PAIR_POSITION_LEFT)
                                    line_mov.setPairLenghtTo(len(self.m_hover_item.m_port_id_list))
                                    
                                port_posinpair, pair_lenght = CanvasGetPortPositionAndPairLenght(self.m_port_id, self.m_pair_id)
                                
                                #create one line for each port of the hover pair
                                for x in range(1, len(self.m_hover_item.m_port_id_list)):
                                    if options.use_bezier_lines:
                                        line_mov = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                                    else:
                                        line_mov = CanvasLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                                    line_mov.setPortPosInPairTo(x)
                                    line_mov.setPairLenghtTo(len(self.m_hover_item.m_port_id_list))
                                    self.m_line_mov_list.append(line_mov)
                                
                            for port_id in self.m_hover_item.getPortsList():
                                for connection in canvas.connection_list:
                                    if ( (connection.port_out_id == self.m_port_id and connection.port_in_id == port_id) or
                                        (connection.port_out_id == port_id and connection.port_in_id == self.m_port_id) ):
                                        self.m_dotcon_list.append(connection)
                                        
                            if len(self.m_dotcon_list) == len(self.m_hover_item.getPortsList()):
                                for connection in self.m_dotcon_list:
                                    connection.widget.setReadyToDisc(True)
                                    connection.widget.updateLineGradient()
                                
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setReadyToDisc(True)
                        
                    elif item.type() == CanvasPortType:
                        if self.m_hover_item !=  item:
                            self.m_hover_item = item
                            self.deleteLine2()
                            self.resetDotLines()
                            
                            for connection in canvas.connection_list:
                                if ( (connection.port_out_id == self.m_port_id and connection.port_in_id == self.m_hover_item.getPortId()) or
                                    (connection.port_out_id == self.m_hover_item.getPortId() and connection.port_in_id == self.m_port_id) ):
                                    for line_mov in self.m_line_mov_list:
                                        line_mov.setReadyToDisc(True)
                                    connection.widget.setReadyToDisc(True)
                                    connection.widget.updateLineGradient()
                                    self.m_dotcon_list.append(connection)
                                elif connection.widget.isReadyToDisc():
                                    connection.widget.setReadyToDisc(False)
                                    connection.widget.updateLineGradient()
                else:
                    self.m_hover_item = None
                    self.deleteLine2()
                    self.resetDotLines()
            else:
                self.m_hover_item = None
                self.deleteLine2()
                self.resetDotLines()
            
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
                if ((connection.group_out_id == self.m_group_id and connection.port_out_id == self.m_port_id) or
                    (connection.group_in_id  == self.m_group_id and connection.port_in_id  == self.m_port_id)):
                    connection.widget.setLocked(False)

            if self.m_hover_item:
                ## TODO: a better way to check already existing connection
                #for connection in canvas.connection_list:
                    #hover_group_id = self.m_hover_item.getGroupId()
                    #hover_port_id  = self.m_hover_item.getPortId()

                    #if (
                        #(connection.group_out_id == self.m_group_id and connection.port_out_id == self.m_port_id and
                         #connection.group_in_id  == hover_group_id  and connection.port_in_id  == hover_port_id) or
                        #(connection.group_out_id == hover_group_id  and connection.port_out_id == hover_port_id and
                         #connection.group_in_id  == self.m_group_id and connection.port_in_id  == self.m_port_id)
                       #):
                        #canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
                        #break
                #else:
                    #if self.m_port_mode == PORT_MODE_OUTPUT:
                        #conn = "%i:%i:%i:%i" % (self.m_group_id, self.m_port_id, self.m_hover_item.getGroupId(), self.m_hover_item.getPortId())
                        #canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)
                    #else:
                        #conn = "%i:%i:%i:%i" % (self.m_hover_item.getGroupId(), self.m_hover_item.getPortId(), self.m_group_id, self.m_port_id)
                        #canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)
                self.connectToHover()

                canvas.scene.clearSelection()

        if self.m_cursor_moving:
            self.setCursor(QCursor(Qt.ArrowCursor))

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
        
        if self.m_port_type == PORT_TYPE_AUDIO_JACK and not self.m_pair_id:
            StereoMenu = QMenu('Set as Stereo with', menu)
            menu.addMenu(StereoMenu)
            
            #get list of available mono ports settables as stereo with port
            port_cousin_list = []
            for port in canvas.port_list:
                if port.port_type == PORT_TYPE_AUDIO_JACK and port.group_id == self.m_group_id and port.port_mode == self.m_port_mode:
                    port_cousin_list.append(port.port_id)
            
            selfport_index = port_cousin_list.index(self.m_port_id)
            stereo_able_ids_list = []
            if selfport_index > 0:
                stereo_able_ids_list.append(port_cousin_list[selfport_index -1])
            if selfport_index < len(port_cousin_list) -1:
                stereo_able_ids_list.append(port_cousin_list[selfport_index +1])
                
            at_least_one = False
            for port in canvas.port_list:
                if port.port_id in stereo_able_ids_list and not port.pair_id:
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

    def boundingRect(self):
        if self.m_pair_id:
            if self.m_port_mode == PORT_MODE_INPUT:
                return QRectF(0, 0, canvas.theme.port_in_pair_width, self.m_port_height)
            else:
                return QRectF(self.m_port_width +12 - canvas.theme.port_in_pair_width,
                              0, canvas.theme.port_in_pair_width, self.m_port_height)
        else:
            return QRectF(0, 0, self.m_port_width + 12, self.m_port_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))

        # FIXME: would be more correct to take line width from Pen, loaded to painter,
        # but this needs some code rearrangement
        lineHinting = canvas.theme.port_audio_jack_pen.widthF() / 2

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
                poly_locx[5] = canvas.theme.port_in_pair_width
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = lineHinting
                poly_locx[1] = self.m_port_width + 5 - lineHinting
                poly_locx[2] = self.m_port_width + 5 - lineHinting
                poly_locx[3] = self.m_port_width + 5 - lineHinting
                poly_locx[4] = lineHinting
                poly_locx[5] = canvas.theme.port_in_pair_width
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
                poly_locx[5] = self.m_port_width + 12 - canvas.theme.port_in_pair_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_port_width + 12 - lineHinting
                poly_locx[1] = 5 + lineHinting
                poly_locx[2] = 5 + lineHinting
                poly_locx[3] = 5 + lineHinting
                poly_locx[4] = self.m_port_width + 12 - lineHinting
                poly_locx[5] = self.m_port_width + 12 -canvas.theme.port_in_pair_width - lineHinting
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
        elif self.m_port_type == PORT_TYPE_MIDI_ALSA:
            poly_color = canvas.theme.port_midi_alsa_bg_sel if self.isSelected() else canvas.theme.port_midi_alsa_bg
            poly_pen = canvas.theme.port_midi_alsa_pen_sel  if self.isSelected() else canvas.theme.port_midi_alsa_pen
            text_pen = canvas.theme.port_midi_alsa_text_sel if self.isSelected() else canvas.theme.port_midi_alsa_text
            conn_pen = canvas.theme.port_midi_alsa_pen_sel
        elif self.m_port_type == PORT_TYPE_PARAMETER:
            poly_color = canvas.theme.port_parameter_bg_sel if self.isSelected() else canvas.theme.port_parameter_bg
            poly_pen = canvas.theme.port_parameter_pen_sel  if self.isSelected() else canvas.theme.port_parameter_pen
            text_pen = canvas.theme.port_parameter_text_sel if self.isSelected() else canvas.theme.port_parameter_text
            conn_pen = canvas.theme.port_parameter_pen_sel
        else:
            qCritical("PatchCanvas::CanvasPort.paint() - invalid port type '%s'" % port_type2str(self.m_port_type))
            return

        if self.m_is_alternate:
            poly_color = poly_color.darker(180)
            #poly_pen.setColor(poly_pen.color().darker(110))
            #text_pen.setColor(text_pen.color()) #.darker(150))
            #conn_pen.setColor(conn_pen.color()) #.darker(150))
        
        polygon  = QPolygonF()
        
        if self.m_pair_id:
            first_of_pair = False
            last_of_pair = False
            
            for pair in canvas.pair_list:
                if pair.pair_id == self.m_pair_id:
                    if self.m_port_id == pair.port_id_list[0]:
                        first_of_pair = True
                    if self.m_port_id == pair.port_id_list[-1]:
                        last_of_pair = True
                    break
                
            if first_of_pair:
                polygon += QPointF(poly_locx[0] , lineHinting)
                polygon += QPointF(poly_locx[5] , lineHinting)
            else:
                polygon += QPointF(poly_locx[0] , 0)
                polygon += QPointF(poly_locx[5] , 0)
                
            if last_of_pair:
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
            portRect = polygon.boundingRect()
            portPos  = portRect.topLeft()
            painter.drawTiledPixmap(portRect, canvas.theme.port_bg_pixmap, portPos)
        else:
            painter.setBrush(poly_color) #.lighter(200))

        painter.setPen(poly_pen)
        painter.drawPolygon(polygon)

        painter.setPen(text_pen)
        painter.setFont(self.m_port_font)
        
        if self.m_pair_id:
            print_name = CanvasGetPortPrintName(self.m_port_id, self.m_pair_id)
            print_name_size = QFontMetrics(self.m_port_font).width(print_name)    
            if self.m_port_mode == PORT_MODE_OUTPUT:
                
                text_pos = QPointF(self.m_port_width + 9 - print_name_size, canvas.theme.port_text_ypos) 

            if print_name_size > (canvas.theme.port_in_pair_width - 6):
                painter.setPen(QPen(poly_color, 3))
                painter.drawLine(poly_locx[5], 3, poly_locx[5], canvas.theme.port_height - 3)
                painter.setPen(text_pen)
                painter.setFont(self.m_port_font)
            painter.drawText(text_pos, print_name)
            
        else:
            painter.drawText(text_pos, self.m_port_name)

        if self.isSelected() != self.m_last_selected_state:
            for connection in canvas.connection_list:
                if ((connection.group_out_id == self.m_group_id and connection.port_out_id == self.m_port_id) or
                    (connection.group_in_id  == self.m_group_id and connection.port_in_id  == self.m_port_id)):
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


class CanvasStereoPair(QGraphicsItem):
    def __init__(self, pair_id, port_mode, port_type, port_id_list, parent):
        QGraphicsItem.__init__(self, parent)

        # Save Variables, useful for later
        self.m_pair_id    = pair_id
        self.m_port_mode  = port_mode
        self.m_port_type  = port_type
        self.m_port_id_list = port_id_list
        self.m_group_id = parent.getGroupId()
        
        # Base Variables
        self.m_pair_width  = 15
        self.m_pair_height = canvas.theme.port_height
        self.m_pair_font = QFont()
        self.m_pair_font.setFamily(canvas.theme.port_font_name)
        self.m_pair_font.setPixelSize(canvas.theme.port_font_size)
        self.m_pair_font.setWeight(canvas.theme.port_font_state)

        self.m_line_mov_list = []
        self.m_r_click_conn = None
        self.m_r_click_time = 0
        self.m_dotcon_list = []
        self.m_hover_item = None
        self.m_last_selected_state = False

        self.m_mouse_down = False
        self.m_cursor_moving = False
        self.setFlags(QGraphicsItem.ItemIsSelectable)
      
    def getPairId(self):
        return self.m_pair_id

    def getPortMode(self):
        return self.m_port_mode

    def getPortType(self):
        return self.m_port_type
    
    def getGroupId(self):
        return self.m_group_id
    
    def getPortWidth(self):
        return self.m_pair_width
    
    def getPairWidth(self):
        return self.m_pair_width

    def getPairHeight(self):
        return self.m_port_height
    
    def getPortsList(self):
        return self.m_port_id_list
        
    def setPortMode(self, port_mode):
        self.m_port_mode = port_mode
        self.update()

    def type(self):
        return CanvasStereoPairType

    def setPairWidth(self, pair_width):
        if pair_width < self.m_pair_width:
            QTimer.singleShot(0, canvas.scene.update)

        self.m_pair_width = pair_width
        self.update()

    def SplitToMonos(self):
        groupItem = self.parentItem()
        CanvasSplitPair(self.m_pair_id)
        groupItem.updatePositions()
        
    def ConnectToHover(self):
        if self.m_hover_item:
            if self.m_hover_item.type() == CanvasPortType:
                hover_port_id_list = [ self.m_hover_item.getPortId() ]
            elif self.m_hover_item.type() == CanvasStereoPairType:
                hover_port_id_list = self.m_hover_item.getPortsList()
                
            con_list = []
            ports_connected_list = []
            
            for port in canvas.port_list:
                if port.port_id == hover_port_id_list[0]:
                    hover_port_group_id = port.group_id
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
                                
            maxpair = len(hover_port_id_list) if len(hover_port_id_list) > len(self.m_port_id_list) else len(self.m_port_id_list)
            
            if len(con_list) == maxpair:
                for connection in con_list:
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
            else:
                for portself_id in self.m_port_id_list:
                    for porthover_id in hover_port_id_list:
                        if ((self.m_port_id_list.index(portself_id) % len(hover_port_id_list)) == 
                            (hover_port_id_list.index(porthover_id) % len(self.m_port_id_list))):
                            if not [portself_id, porthover_id] in ports_connected_list:
                                if self.m_port_mode == PORT_MODE_OUTPUT:
                                    conn = "%i:%i:%i:%i" % (selfgroup_id, portself_id, hover_port_group_id, porthover_id)
                                else:
                                    conn = "%i:%i:%i:%i" % (hover_port_group_id, porthover_id, selfgroup_id, portself_id)
                                canvas.callback(ACTION_PORTS_CONNECT, 0, 0, conn)
                    
    def resetDotLines(self):
        for connection in self.m_dotcon_list:
            if connection.widget.isReadyToDisc():
                connection.widget.setReadyToDisc(False)
                connection.widget.updateLineGradient()
                
        for line_mov in self.m_line_mov_list:
            line_mov.setReadyToDisc(False)
   
    def resetLineMovPositions(self):
        for line_mov in self.m_line_mov_list:
            if self.m_line_mov_list.index(line_mov) < len(self.m_port_id_list):
                line_mov.setPortPosInPairTo(line_mov.m_port_posinpair)
                line_mov.setPairLenghtTo(line_mov.m_pair_lenght)
            else:
                item = line_mov
                canvas.scene.removeItem(item)
                del item
                
        self.m_line_mov_list = self.m_line_mov_list[:len(self.m_port_id_list)]
                
            
    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.m_hover_item = None
            self.m_mouse_down = True
            self.m_cursor_moving = False
            
        elif event.button() == Qt.RightButton:
            if canvas.is_line_mov:
                if self.m_hover_item:
                    self.ConnectToHover()
                    #self.m_r_click_conn = self.m_hover_item
                    #self.m_r_click_time = time.time()
                    
                    for line_mov in self.m_line_mov_list:
                        line_mov.toggleReadyToDisc()
                        line_mov.updateLinePos(event.scenePos())
                        
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
                    if port.port_id in self.m_port_id_list:
                        port.widget.setZValue(canvas.last_z_value)
                
                #for port in canvas.port_list:
                    #if (not port.port_id in self.m_port_id_list and
                        #(port.port_type != self.m_port_type or 
                        #port.port_mode == self.m_port_mode)):
                        #port.widget.setOpacity(0.35)
                        
                #for pair in canvas.pair_list:
                    #if (pair.pair_id != self.m_pair_id and
                        #(pair.port_type != self.m_port_type or
                        #pair.port_mode == self.m_port_mode)):
                        #pair.widget.setOpacity(0.35)
                
                for port_id in self.m_port_id_list:
                    port_posinpair, pair_lenght = CanvasGetPortPositionAndPairLenght(port_id, self.m_pair_id)
                    
                    if options.use_bezier_lines:
                        line_mov  = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                    else:
                        line_mov  = CanvasLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                    #line_mov.setZValue(canvas.last_z_value)
                    self.m_line_mov_list.append(line_mov)
                
                canvas.is_line_mov = True
                canvas.last_z_value += 1
                self.parentItem().setZValue(canvas.last_z_value)

            item = None
            items = canvas.scene.items(event.scenePos(), Qt.ContainsItemShape, Qt.AscendingOrder)
            for i in range(len(items)):
                if items[i].type() == CanvasPortType or items[i].type() == CanvasStereoPairType:
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
                                line_mov.setPortPosInPairTo(PORT_IN_PAIR_POSITION_MONO)
                            
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
                                    
                    elif item.type() == CanvasStereoPairType:
                        if self.m_hover_item != item:
                            self.m_hover_item = item
                            self.resetDotLines()
                            self.resetLineMovPositions()
                            
                            if len(self.m_hover_item.m_port_id_list) <= len(self.m_line_mov_list):
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setPortPosInPairTo(line_mov.m_port_posinpair % len(self.m_hover_item.m_port_id_list))
                                    line_mov.setPairLenghtTo(len(self.m_hover_item.m_port_id_list))
                            else:
                                
                                for line_mov in self.m_line_mov_list:
                                    line_mov.setPairLenghtTo(len(self.m_hover_item.m_port_id_list))
                                
                                #create new lines when line is hover a biggest pair
                                for x in range(len(self.m_port_id_list), len(self.m_hover_item.m_port_id_list)):
                                    port_posinpair = x % len(self.m_port_id_list)
                                    pair_lenght = len(self.m_port_id_list)
                                    if options.use_bezier_lines:
                                        line_mov  = CanvasBezierLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                                    else:
                                        line_mov  = CanvasLineMov(self.m_port_mode, self.m_port_type, port_posinpair, pair_lenght, self)
                                        
                                    line_mov.setPortPosInPairTo(x)
                                    line_mov.setPairLenghtTo(len(self.m_hover_item.m_port_id_list))
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
                    for port_id in self.m_port_id_list:
                        port_posinpair = self.m_port_id_list.index(port_id)
                        self.m_line_mov_list[port_posinpair].setPortPosInPairTo(port_posinpair)
                    
            else:
                if self.m_hover_item:
                    self.m_hover_item = None
                    self.m_r_click_conn = None
                    self.resetDotLines()
                    self.resetLineMovPositions()
                    for port_id in self.m_port_id_list:
                        port_posinpair = self.m_port_id_list.index(port_id)
                        self.m_line_mov_list[port_posinpair].setPortPosInPairTo(port_posinpair)
            
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
                
                for pair in canvas.pair_list:
                    pair.widget.setOpacity(1)

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
            canvas.is_line_mov = False
        QGraphicsItem.mouseReleaseEvent(self, event)

        
    def contextMenuEvent(self, event):
        if canvas.is_line_mov:
            return event.ignore()
        
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
            
        #check each group to get no trouble with ports in the same pair which could have non-consecutive port ids
        for group_id in group_ids_list:
            #get a list of connected port from this group
            gr_port_ids_list = []
            for port in canvas.port_list:
                if port.group_id == group_id:
                    if port.port_id in all_connected_ports_ids:
                        if not port.port_id in gr_port_ids_list:
                            gr_port_ids_list.append(port.port_id)
            
            pair_list_ids = []
            pair_dirty_connect_list = [] #is used to display individual ports of pair or not
            last_is_dirty_pair = False #is used to display menu separator or not
            
            #create action for each port of this group connected to the box
            for port in canvas.port_list:
                if port.port_id in gr_port_ids_list:
                    
                    #get port connection list
                    port_con_list = []
                    for connection in canvas.connection_list:
                        if ( (connection.port_out_id in self.m_port_id_list and connection.port_in_id == port.port_id) or
                            (connection.port_out_id == port.port_id and connection.port_in_id in self.m_port_id_list) ):
                            port_con_list.append(connection.connection_id)
                    
                    if port.pair_id:
                        port_alone_connected = False
                        
                        if not port.pair_id in pair_list_ids:
                            pair_list_ids.append(port.pair_id)
                            
                            for pair in canvas.pair_list:
                                if pair.pair_id == port.pair_id:
                                    
                                    #count ports from pair connected to the box
                                    c = 0
                                    for p in pair.port_id_list:
                                        if p in all_connected_ports_ids:
                                            c += 1
                                    
                                    if c == 1:
                                        port_alone_connected = True
                                    
                                    #Set disconnect port action if port is the only one of the pair connected to the box
                                    if port_alone_connected:
                                        if last_is_dirty_pair:
                                            discMenu.addSeparator()
                                            last_is_dirty_pair = False
                                            
                                        full_port_name = CanvasGetFullPortName(port.port_id)
                                        act_x_disc_port = discMenu.addAction(full_port_name)
                                        act_x_disc_port.setData(port_con_list)
                                        act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                                        
                                        
                                    else:
                                        #get pair connection list
                                        pair_con_list = []
                                        for connection in canvas.connection_list:
                                            if (connection.port_out_id in self.m_port_id_list and connection.port_in_id in pair.port_id_list or
                                                connection.port_out_id in pair.port_id_list and connection.port_in_id in self.m_port_id_list):
                                                pair_con_list.append(connection.connection_id)
                                                if connection.port_out_id in self.m_port_id_list:
                                                    if (self.m_port_id_list.index(connection.port_out_id) % len(pair.port_id_list) !=
                                                        pair.port_id_list.index(connection.port_in_id) % len(self.m_port_id_list) ):
                                                        pair_dirty_connect_list.append(pair.pair_id)
                                                else:
                                                    if (self.m_port_id_list.index(connection.port_in_id) % len(pair.port_id_list) !=
                                                        pair.port_id_list.index(connection.port_out_id) % len(self.m_port_id_list) ):
                                                        pair_dirty_connect_list.append(pair.pair_id)
                                                                                                              
                                        if pair.pair_id in pair_dirty_connect_list:
                                            discMenu.addSeparator()
                                            last_is_dirty_pair = True
                                        elif last_is_dirty_pair:
                                            discMenu.addSeparator()
                                            last_is_dirty_pair = False
                                            
                                        
                                        #set pair action in submenu
                                        pair_print_name = CanvasGetPairFullName(pair.pair_id)
                                        act_x_disc_pair = discMenu.addAction(pair_print_name)        
                                        act_x_disc_pair.setData(pair_con_list)
                                        act_x_disc_pair.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                                        
                                    
                        #set port action in submenu (if port is in a pair and isn't the only one connected to the box) 
                        if not port_alone_connected and port.pair_id in pair_dirty_connect_list:
                            port_print_name = CanvasGetPortPrintName(port.port_id, port.pair_id)
                            act_x_disc_port = discMenu.addAction('  → ' + port_print_name)
                            act_x_disc_port.setData(port_con_list)
                            act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                        
                    else:
                        if last_is_dirty_pair:
                            discMenu.addSeparator()
                            last_is_dirty_pair = False
                            
                        #set port action in submenu (if port is not in a pair) 
                        full_port_name = CanvasGetFullPortName(port.group_id, port.port_id)
                        act_x_disc_port = discMenu.addAction(full_port_name)
                        act_x_disc_port.setData(port_con_list)
                        act_x_disc_port.triggered.connect(canvas.qobject.PortContextMenuDisconnect)
                
                
        menu.addMenu(discMenu)
        act_x_disc_allthepair = menu.addAction("Disconnect &All")
        act_x_sep_1 = menu.addSeparator()
        act_x_setasmono = menu.addAction('Split to Monos')

        act_selected = menu.exec_(event.screenPos())
        
        if act_selected == act_x_disc_allthepair:
            for connection in canvas.connection_list:
                if connection.port_out_id in self.m_port_id_list or connection.port_in_id in self.m_port_id_list:
                    canvas.callback(ACTION_PORTS_DISCONNECT, connection.connection_id, 0, "")
            
        elif act_selected == act_x_setasmono:
            self.SplitToMonos()
            
        event.accept()
            
    def boundingRect(self):
        self.m_pair_width = self.getPairWidth()
        if self.m_port_mode == PORT_MODE_INPUT:
            return QRectF(canvas.theme.port_in_pair_width, 0, self.m_pair_width + 12 - canvas.theme.port_in_pair_width, canvas.theme.port_height * len(self.m_port_id_list))
        else:
            return QRectF(0, 0, self.m_pair_width + 12 - canvas.theme.port_in_pair_width, canvas.theme.port_height * len(self.m_port_id_list))

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))
        
        lineHinting = canvas.theme.port_audio_jack_pen.widthF() / 2

        poly_locx = [0, 0, 0, 0, 0]
        poly_corner_xhinting = (float(canvas.theme.port_height)/2) % floor(float(canvas.theme.port_height)/2)
        if poly_corner_xhinting == 0:
            poly_corner_xhinting = 0.5 * (1 - 7 / (float(canvas.theme.port_height)/2))
        
        if self.m_port_mode == PORT_MODE_INPUT:
            port_width = canvas.theme.port_in_pair_width
            
            for port in canvas.port_list:
                if port.port_id in self.m_port_id_list:
                    port_print_name = CanvasGetPortPrintName(port.port_id, self.m_pair_id)
                    port_in_p_width = QFontMetrics(self.m_pair_font).width(port_print_name) + 3
                    port_width = max(port_width, port_in_p_width)

            text_pos = QPointF(port_width + 3, canvas.theme.port_text_ypos + (canvas.theme.port_height * (len(self.m_port_id_list) -1)/2))

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = canvas.theme.port_in_pair_width - lineHinting
                poly_locx[1] = self.m_pair_width + 5 + lineHinting
                poly_locx[2] = self.m_pair_width + 12 + lineHinting
                poly_locx[3] = self.m_pair_width + 5 + lineHinting
                poly_locx[4] = canvas.theme.port_in_pair_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = canvas.theme.port_in_pair_width - lineHinting
                poly_locx[1] = self.m_pair_width + 5 + lineHinting
                poly_locx[2] = self.m_pair_width + 5 + lineHinting
                poly_locx[3] = self.m_pair_width + 5 + lineHinting
                poly_locx[4] = canvas.theme.port_in_pair_width - lineHinting
            else:
                qCritical("PatchCanvas::CanvasStereoPair.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        elif self.m_port_mode == PORT_MODE_OUTPUT:
            text_pos = QPointF(9, canvas.theme.port_text_ypos + (canvas.theme.port_height * (len(self.m_port_id_list) -1)/2))

            if canvas.theme.port_mode == Theme.THEME_PORT_POLYGON:
                poly_locx[0] = self.m_pair_width + 12 - canvas.theme.port_in_pair_width - lineHinting
                poly_locx[1] = 7 + lineHinting
                poly_locx[2] = 0 + lineHinting
                poly_locx[3] = 7 + lineHinting
                poly_locx[4] = self.m_pair_width + 12 - canvas.theme.port_in_pair_width - lineHinting
            elif canvas.theme.port_mode == Theme.THEME_PORT_SQUARE:
                poly_locx[0] = self.m_pair_width + 12 - canvas.theme.port_in_pair_width - lineHinting
                poly_locx[1] = 5 + lineHinting
                poly_locx[2] = 5 + lineHinting
                poly_locx[3] = 5 + lineHinting
                poly_locx[4] = self.m_pair_width + 12 - canvas.theme.port_in_pair_width - lineHinting
            else:
                qCritical("PatchCanvas::CanvasStereoPair.paint() - invalid theme port mode '%s'" % canvas.theme.port_mode)
                return

        else:
            qCritical("PatchCanvas::CanvasStereoPair.paint() - invalid port mode '%s'" % port_mode2str(self.m_port_mode))
            return

        poly_pen = canvas.theme.pair_audio_jack_pen_sel  if self.isSelected() else canvas.theme.pair_audio_jack_pen
        text_pen = canvas.theme.port_audio_jack_text_sel if self.isSelected() else canvas.theme.port_audio_jack_text
        
        if self.m_port_mode == PORT_MODE_OUTPUT:
            real_pair_width = self.m_pair_width - canvas.theme.port_in_pair_width
            pair_gradient = QLinearGradient(real_pair_width -40, 0, real_pair_width, 0)
            pair_gradient.setColorAt(0, canvas.theme.port_audio_jack_bg_sel if self.isSelected() else canvas.theme.port_audio_jack_bg)
            pair_gradient.setColorAt(1, canvas.theme.pair_audio_jack_bg_sel if self.isSelected() else canvas.theme.pair_audio_jack_bg)
        else:
            pair_gradient = QLinearGradient(canvas.theme.port_in_pair_width, 0, canvas.theme.port_in_pair_width + 40, 0)
            pair_gradient.setColorAt(0, canvas.theme.pair_audio_jack_bg_sel if self.isSelected() else canvas.theme.pair_audio_jack_bg)
            pair_gradient.setColorAt(1, canvas.theme.port_audio_jack_bg_sel if self.isSelected() else canvas.theme.port_audio_jack_bg)
            
        
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
            painter.setBrush(pair_gradient)

        painter.setPen(poly_pen)
        painter.drawPolygon(polygon)

        painter.setPen(text_pen)
        painter.setFont(self.m_pair_font)
        pair_name = CanvasGetPairName(self.m_port_id_list)
        painter.drawText(text_pos, pair_name)

        if self.isSelected() != self.m_last_selected_state:
            CanvasUpdateSelectedLines()
                    
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
        QGraphicsItem.__init__(self, parent)

        # Save Variables, useful for later
        self.m_group_id   = group_id
        self.m_group_name = group_name

        # plugin Id, < 0 if invalid
        self.m_plugin_id = -1
        self.m_plugin_ui = False

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
        self.m_font_name = QFont()
        self.m_font_name.setFamily(canvas.theme.box_font_name)
        self.m_font_name.setPixelSize(canvas.theme.box_font_size)
        self.m_font_name.setWeight(canvas.theme.box_font_state)

        self.m_font_port = QFont()
        self.m_font_port.setFamily(canvas.theme.port_font_name)
        self.m_font_port.setPixelSize(canvas.theme.port_font_size)
        self.m_font_port.setWeight(canvas.theme.port_font_state)

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

        self.setFlag(QGraphicsItem.ItemIsFocusable, True)

        if options.auto_select_items:
            self.setAcceptHoverEvents(True)

        self.updatePositions()

        canvas.scene.addItem(self)

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

    def setAsPlugin(self, plugin_id, hasUi):
        self.m_plugin_id = plugin_id
        self.m_plugin_ui = hasUi

    def setIcon(self, icon):
        if self.icon_svg is not None:
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

    def addPortFromGroup(self, port_id, port_mode, port_type, port_name, is_alternate):
        if len(self.m_port_list_ids) == 0:
            if options.auto_hide_groups:
                if options.eyecandy == EYECANDY_FULL:
                    CanvasItemFX(self, True, False)
                self.setVisible(True)

        new_widget = CanvasPort(self.m_group_id, port_id, port_name, port_mode, port_type, is_alternate, self)

        port_dict = port_dict_t()
        port_dict.group_id = self.m_group_id
        port_dict.port_id  = port_id
        port_dict.port_name = port_name
        port_dict.port_mode = port_mode
        port_dict.port_type = port_type
        port_dict.is_alternate = is_alternate
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
                    CanvasItemFX(self, False, False)
                else:
                    self.setVisible(False)
    
    def addPairFromGroup(self, pair_id, port_mode, port_type, port_id_list):
        new_widget = CanvasStereoPair(pair_id, port_mode, port_type, port_id_list, self)
        return new_widget
    
    def removePairFromGroup(self, pair_id):
        self.updatePositions()
    
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
        if self.icon_svg is None:
            return

        item = self.icon_svg
        self.icon_svg = None
        canvas.scene.removeItem(item)
        del item

    def updatePositions(self):
        self.prepareGeometryChange()

        # Check Text Name size
        app_name_size = QFontMetrics(self.m_font_name).width(self.m_group_name) + 30
        self.p_width = max( 50, app_name_size )

        # Get Port List
        port_list = []
        for port in canvas.port_list:
            if port.group_id == self.m_group_id and port.port_id in self.m_port_list_ids:
                port_list.append(port)

        if len(port_list) == 0:
            self.p_height = canvas.theme.box_header_height
        else:
            max_in_width = max_out_width = 0
            port_spacing = canvas.theme.port_height + canvas.theme.port_spacing

            # Get Max Box Width, vertical ports re-positioning
            port_types   = [PORT_TYPE_AUDIO_JACK, PORT_TYPE_MIDI_JACK, PORT_TYPE_MIDI_ALSA, PORT_TYPE_PARAMETER]
            last_in_type = last_out_type = PORT_TYPE_NULL
            last_in_pos  = last_out_pos  = canvas.theme.box_header_height + canvas.theme.box_header_spacing
            last_of_pair = True
            
            for port_type in port_types:
                for port in port_list:
                    if port.port_type != port_type:
                        continue
                    
                    last_of_pair = True
                    if port.pair_id:
                        for pair in canvas.pair_list:
                            if pair.group_id == self.m_group_id and port.port_id in pair.port_id_list:
                                if pair.port_id_list[-1] != port.port_id:
                                    last_of_pair = False
                                    break
                    
                    size = QFontMetrics(self.m_font_port).width(port.port_name)
                    if port.pair_id:
                        size = 0
                        totsize = QFontMetrics(self.m_font_port).width(port.port_name) + 3
                        for pair in canvas.pair_list:
                            if pair.pair_id == port.pair_id:
                                pair_name = CanvasGetPairName(pair.port_id_list)
                                size = QFontMetrics(self.m_font_port).width(pair_name) + canvas.theme.port_in_pair_width
                                break
                        size = max(size, totsize)
                    
                    if port.port_mode == PORT_MODE_INPUT:
                        max_in_width = max( max_in_width, size )
                        if port.port_type != last_in_type:
                            if last_in_type != PORT_TYPE_NULL:
                                last_in_pos += canvas.theme.port_spacingT
                            last_in_type = port.port_type
                        port.widget.setY(last_in_pos)
                        
                        for pair in canvas.pair_list:
                            if pair.group_id == self.m_group_id and pair.port_id_list[0] == port.port_id:
                                pair.widget.setY(last_in_pos)
                                break
                        
                        if last_of_pair:
                            last_in_pos += port_spacing
                        else:
                            last_in_pos += canvas.theme.port_height

                    elif port.port_mode == PORT_MODE_OUTPUT:
                        max_out_width = max( max_out_width, size )
                        if port.port_type != last_out_type:
                            if last_out_type != PORT_TYPE_NULL:
                                last_out_pos += canvas.theme.port_spacingT
                            last_out_type = port.port_type
                        port.widget.setY(last_out_pos)
                        
                        for pair in canvas.pair_list:
                            if pair.group_id == self.m_group_id and pair.port_id_list[0] == port.port_id:
                                pair.widget.setY(last_out_pos)
                                break
                        
                        if last_of_pair:
                            last_out_pos += port_spacing
                        else:
                            last_out_pos += canvas.theme.port_height

            self.p_width = max(self.p_width, 30 + max_in_width + max_out_width)

            # Horizontal ports re-positioning
            inX = 1 + canvas.theme.port_offset
            outX = self.p_width - max_out_width - canvas.theme.port_offset - 13
            out_in_pairX = self.p_width - canvas.theme.port_offset - 13 - canvas.theme.port_in_pair_width
            
            for port_type in port_types:
                for port in port_list:
                    if port.port_mode == PORT_MODE_INPUT:
                        port.widget.setX(inX)
                        if port.pair_id:
                            port.widget.setPortWidth(canvas.theme.port_in_pair_width)
                            
                            for pair in canvas.pair_list:
                                if pair.group_id == self.m_group_id and pair.port_id_list[0] == port.port_id:
                                    pair.widget.setPairWidth(max_in_width)
                                    pair.widget.setX(canvas.theme.port_offset +1)
                                    break
                            
                        else:
                            port.widget.setPortWidth(max_in_width)

                    elif port.port_mode == PORT_MODE_OUTPUT:
                        if port.pair_id:
                            port.widget.setX(out_in_pairX)
                            port.widget.setPortWidth(canvas.theme.port_in_pair_width)
                            
                            for pair in canvas.pair_list:
                                if pair.group_id == self.m_group_id and pair.port_id_list[0] == port.port_id:
                                    pair.widget.setX(outX)
                                    pair.widget.setPairWidth(max_out_width)
                                    break
                        else:
                            port.widget.setX(outX)
                            port.widget.setPortWidth(max_out_width)

            self.p_height  = max(last_in_pos, last_out_pos)
            self.p_height += max(canvas.theme.port_spacing, canvas.theme.port_spacingT) - canvas.theme.port_spacing
            self.p_height += canvas.theme.box_pen.widthF()

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
        event.accept()

        menu = QMenu()
        discMenu = QMenu("Disconnect", menu)

        conn_list     = []
        conn_list_ids = []

        for port_id in self.m_port_list_ids:
            tmp_conn_list = CanvasGetPortConnectionList(self.m_group_id, port_id)
            for conn_id, group_id, port_id in tmp_conn_list:
                if conn_id not in conn_list_ids:
                    conn_list.append((conn_id, group_id, port_id))
                    conn_list_ids.append(conn_id)

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
        act_x_sep1 = menu.addSeparator()
        act_x_info = menu.addAction("Info")
        act_x_rename = menu.addAction("Rename")
        act_x_sep2 = menu.addSeparator()
        act_x_split_join = menu.addAction("Join" if self.m_splitted else "Split")

        if not features.group_info:
            act_x_info.setVisible(False)

        if not features.group_rename:
            act_x_rename.setVisible(False)

        if not (features.group_info and features.group_rename):
            act_x_sep1.setVisible(False)

        if self.m_plugin_id >= 0 and self.m_plugin_id <= MAX_PLUGIN_ID_ALLOWED:
            menu.addSeparator()
            act_p_edit = menu.addAction("Edit")
            act_p_ui   = menu.addAction("Show Custom UI")
            menu.addSeparator()
            act_p_clone   = menu.addAction("Clone")
            act_p_rename  = menu.addAction("Rename...")
            act_p_replace = menu.addAction("Replace...")
            act_p_remove  = menu.addAction("Remove")

            if not self.m_plugin_ui:
                act_p_ui.setVisible(False)

        else:
            act_p_edit    = act_p_ui     = None
            act_p_clone   = act_p_rename = None
            act_p_replace = act_p_remove = None

        haveIns = haveOuts = False
        for port in canvas.port_list:
            if port.group_id == self.m_group_id and port.port_id in self.m_port_list_ids:
                if port.port_mode == PORT_MODE_INPUT:
                    haveIns = True
                elif port.port_mode == PORT_MODE_OUTPUT:
                    haveOuts = True

        if not (self.m_splitted or bool(haveIns and haveOuts)):
            act_x_sep2.setVisible(False)
            act_x_split_join.setVisible(False)

        act_selected = menu.exec_(event.screenPos())

        if act_selected is None:
            pass

        elif act_selected == act_x_disc_all:
            for conn_id in conn_list_ids:
                canvas.callback(ACTION_PORTS_DISCONNECT, conn_id, 0, "")

        elif act_selected == act_x_info:
            canvas.callback(ACTION_GROUP_INFO, self.m_group_id, 0, "")

        elif act_selected == act_x_rename:
            canvas.callback(ACTION_GROUP_RENAME, self.m_group_id, 0, "")

        elif act_selected == act_x_split_join:
            if self.m_splitted:
                canvas.callback(ACTION_GROUP_JOIN, self.m_group_id, 0, "")
            else:
                canvas.callback(ACTION_GROUP_SPLIT, self.m_group_id, 0, "")

        elif act_selected == act_p_edit:
            canvas.callback(ACTION_PLUGIN_EDIT, self.m_plugin_id, 0, "")

        elif act_selected == act_p_ui:
            canvas.callback(ACTION_PLUGIN_SHOW_UI, self.m_plugin_id, 0, "")

        elif act_selected == act_p_clone:
            canvas.callback(ACTION_PLUGIN_CLONE, self.m_plugin_id, 0, "")

        elif act_selected == act_p_rename:
            canvas.callback(ACTION_PLUGIN_RENAME, self.m_plugin_id, 0, "")

        elif act_selected == act_p_replace:
            canvas.callback(ACTION_PLUGIN_REPLACE, self.m_plugin_id, 0, "")

        elif act_selected == act_p_remove:
            canvas.callback(ACTION_PLUGIN_REMOVE, self.m_plugin_id, 0, "")

    def keyPressEvent(self, event):
        if self.m_plugin_id >= 0 and event.key() == Qt.Key_Delete:
            canvas.callback(ACTION_PLUGIN_REMOVE, self.m_plugin_id, 0, "")
            return event.accept()
        QGraphicsItem.keyPressEvent(self, event)

    def hoverEnterEvent(self, event):
        if options.auto_select_items:
            if len(canvas.scene.selectedItems()) > 0:
                canvas.scene.clearSelection()
            self.setSelected(True)
        QGraphicsItem.hoverEnterEvent(self, event)

    def mouseDoubleClickEvent(self, event):
        if self.m_plugin_id >= 0:
            canvas.callback(ACTION_PLUGIN_SHOW_UI if self.m_plugin_ui else ACTION_PLUGIN_EDIT, self.m_plugin_id, 0, "")
            return event.accept()
        QGraphicsItem.mouseDoubleClickEvent(self, event)

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
            self.setX(round(self.x()))
            self.setY(round(self.y()))
        self.m_mouse_down = False
        self.m_cursor_moving = False
        QGraphicsItem.mouseReleaseEvent(self, event)

    def boundingRect(self):
        return QRectF(0, 0, self.p_width, self.p_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))
        rect = QRectF(0, 0, self.p_width, self.p_height)

        # Draw rectangle
        pen = canvas.theme.box_pen_sel if self.isSelected() else canvas.theme.box_pen
        painter.setPen(pen)
        lineHinting = pen.widthF() / 2

        if canvas.theme.box_bg_type == Theme.THEME_BG_GRADIENT:
            box_gradient = QLinearGradient(0, 0, 0, self.p_height)
            box_gradient.setColorAt(0, canvas.theme.box_bg_1)
            box_gradient.setColorAt(1, canvas.theme.box_bg_2)
            painter.setBrush(box_gradient)
        else:
            painter.setBrush(canvas.theme.box_bg_1)

        rect.adjust(lineHinting, lineHinting, -lineHinting, -lineHinting)
        painter.drawRect(rect)

        # Draw pixmap header
        rect.setHeight(canvas.theme.box_header_height)
        if canvas.theme.box_header_pixmap:
            painter.setPen(Qt.NoPen)
            painter.setBrush(canvas.theme.box_bg_2)

            # outline
            rect.adjust(lineHinting, lineHinting, -lineHinting, -lineHinting)
            painter.drawRect(rect)

            rect.adjust(1, 1, -1, 0)
            painter.drawTiledPixmap(rect, canvas.theme.box_header_pixmap, rect.topLeft())

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
            elif "distrho" in name:
                icon_path = ":/scalable/pb_distrho.svg"
                self.p_size = QRectF(5, 4, 14, 14)
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
                self.p_size = QRectF(4, 3, 16, 16)

        elif icon == ICON_HARDWARE:
            icon_path = ":/scalable/pb_hardware.svg"
            self.p_size = QRectF(5, 2, 16, 16)

        elif icon == ICON_DISTRHO:
            icon_path = ":/scalable/pb_distrho.svg"
            self.p_size = QRectF(5, 4, 14, 14)

        elif icon == ICON_FILE:
            icon_path = ":/scalable/pb_file.svg"
            self.p_size = QRectF(5, 4, 12, 14)

        elif icon == ICON_PLUGIN:
            icon_path = ":/scalable/pb_plugin.svg"
            self.p_size = QRectF(5, 4, 14, 14)

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
        if not self.m_renderer:
            return QGraphicsSvgItem.paint(self, painter, option, widget)

        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, False)
        painter.setRenderHint(QPainter.TextAntialiasing, False)
        self.m_renderer.render(painter, self.p_size)
        painter.restore()

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
        elif port_type == PORT_TYPE_MIDI_ALSA:
            self.setColor(canvas.theme.line_midi_alsa_glow)
        elif port_type == PORT_TYPE_PARAMETER:
            self.setColor(canvas.theme.line_parameter_glow)

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
