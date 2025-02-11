#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, qCritical, qFatal, qWarning, QObject
    from PyQt5.QtCore import QPointF, QRectF, QTimer
    from PyQt5.QtWidgets import QGraphicsObject
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, qCritical, qFatal, qWarning, QObject
    from PyQt6.QtCore import QPointF, QRectF, QTimer
    from PyQt6.QtWidgets import QGraphicsObject

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    features,
    options,
    group_dict_t,
    port_dict_t,
    connection_dict_t,
    bool2str,
    icon2str,
    split2str,
    port_mode2str,
    port_type2str,
    CanvasIconType,
    CanvasRubberbandType,
    ACTION_PORTS_DISCONNECT,
    EYECANDY_FULL,
    ICON_APPLICATION,
    ICON_HARDWARE,
    ICON_LADISH_ROOM,
    PORT_MODE_INPUT,
    PORT_MODE_OUTPUT,
    SPLIT_YES,
    SPLIT_NO,
    SPLIT_UNDEF,
    MAX_PLUGIN_ID_ALLOWED,
)

from .canvasbox import CanvasBox
from .canvasbezierline import CanvasBezierLine
from .canvasline import CanvasLine
from .theme import Theme, getDefaultTheme, getThemeName
from .utils import CanvasCallback, CanvasGetNewGroupPos, CanvasItemFX, CanvasRemoveItemFX

# FIXME
from . import *
from .scene import PatchScene

from utils import QSafeSettings

# ------------------------------------------------------------------------------------------------------------

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
            if item:
                if isinstance(item, QGraphicsObject):
                    item.blockSignals(True)
                    item.hide()
                    item.blockSignals(False)
                else:
                    item.hide()

    @pyqtSlot()
    def AnimationFinishedDestroy(self):
        animation = self.sender()
        if animation:
            animation.forceStop()
            canvas.animation_list.remove(animation)
            item = animation.item()
            if item:
                CanvasRemoveItemFX(item)

    @pyqtSlot()
    def PortContextMenuConnect(self):
        try:
            sources, targets = self.sender().data()
        except:
            return

        for port_type in (PORT_TYPE_AUDIO_JACK, PORT_TYPE_MIDI_JACK, PORT_TYPE_MIDI_ALSA, PORT_TYPE_PARAMETER):
            source_ports = sources[port_type]
            target_ports = targets[port_type]

            source_ports_len = len(source_ports)
            target_ports_len = len(target_ports)

            if source_ports_len == 0 or target_ports_len == 0:
                continue

            for i in range(min(source_ports_len, target_ports_len)):
                data = "%i:%i:%i:%i" % (source_ports[i][0],
                                        source_ports[i][1],
                                        target_ports[i][0],
                                        target_ports[i][1])
                CanvasCallback(ACTION_PORTS_CONNECT, 0, 0, data)

            if source_ports_len == 1 and target_ports_len > 1:
                for i in range(1, target_ports_len):
                    data = "%i:%i:%i:%i" % (source_ports[0][0],
                                            source_ports[0][1],
                                            target_ports[i][0],
                                            target_ports[i][1])
                    CanvasCallback(ACTION_PORTS_CONNECT, 0, 0, data)

    @pyqtSlot()
    def PortContextMenuDisconnect(self):
        try:
            connectionId = int(self.sender().data())
        except:
            return

        CanvasCallback(ACTION_PORTS_DISCONNECT, connectionId, 0, "")

    @pyqtSlot(int, bool, int, int)
    def boxPositionChanged(self, groupId, split, x, y):
        x2 = y2 = 0

        if split:
            for group in canvas.group_list:
                if group.group_id == groupId:
                    if group.split:
                        pos = group.widgets[1].pos()
                        x2  = pos.x()
                        y2  = pos.y()
                    break

        valueStr = "%i:%i:%i:%i" % (x, y, x2, y2)
        CanvasCallback(ACTION_GROUP_POSITION, groupId, 0, valueStr)

    @pyqtSlot(int, bool, int, int)
    def sboxPositionChanged(self, groupId, split, x2, y2):
        x = y = 0

        for group in canvas.group_list:
            if group.group_id == groupId:
                pos = group.widgets[0].pos()
                x = pos.x()
                y = pos.y()
                break

        valueStr = "%i:%i:%i:%i" % (x, y, x2, y2)
        CanvasCallback(ACTION_GROUP_POSITION, groupId, 0, valueStr)

# ------------------------------------------------------------------------------------------------------------

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

# ------------------------------------------------------------------------------------------------------------

def init(appName, scene, callback, debug=False):
    if debug:
        print("PatchCanvas::init(\"%s\", %s, %s, %s)" % (appName, scene, callback, bool2str(debug)))

    if canvas.initiated:
        qCritical("PatchCanvas::init() - already initiated")
        return

    if not callback:
        qFatal("PatchCanvas::init() - fatal error: callback not set")
        return

    canvas.callback = callback
    canvas.debug = debug
    canvas.scene = scene

    canvas.last_z_value = 0
    canvas.last_connection_id = 0
    canvas.initial_pos = QPointF(0, 0)
    canvas.size_rect = QRectF()

    if not canvas.qobject:
        canvas.qobject = CanvasObject()
    if not canvas.settings:
        canvas.settings = QSafeSettings("falkTX", appName)

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

    group_pos = {}
    group_list_ids = []
    port_list_ids = []
    connection_list_ids = []

    for group in canvas.group_list:
        group_pos[group.group_name] = (
            group.split,
            group.widgets[0].pos(),
            group.widgets[1].pos() if group.split else None,
        )
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
    canvas.group_plugin_map = {}
    canvas.old_group_pos = group_pos

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

# ------------------------------------------------------------------------------------------------------------

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
    canvas.scene.updateLimits()
    canvas.scene.fixScaleFactor()

def addGroup(group_id, group_name, split=SPLIT_UNDEF, icon=ICON_APPLICATION):
    if canvas.debug:
        print("PatchCanvas::addGroup(%i, %s, %s, %s)" % (
              group_id, group_name.encode(), split2str(split), icon2str(icon)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            qWarning("PatchCanvas::addGroup(%i, %s, %s, %s) - group already exists" % (
                     group_id, group_name.encode(), split2str(split), icon2str(icon)))
            return None

    old_matching_group = canvas.old_group_pos.pop(group_name, None)

    if split == SPLIT_UNDEF:
        isHardware = bool(icon == ICON_HARDWARE)

        if features.handle_group_pos:
            split = getStoredCanvasSplit(group_name, SPLIT_YES if isHardware else split)
        elif isHardware:
            split = SPLIT_YES
        elif old_matching_group is not None and old_matching_group[0]:
            split = SPLIT_YES

    group_box = CanvasBox(group_id, group_name, icon)
    group_box.positionChanged.connect(canvas.qobject.boxPositionChanged)
    group_box.blockSignals(True)

    group_dict = group_dict_t()
    group_dict.group_id = group_id
    group_dict.group_name = group_name
    group_dict.split = bool(split == SPLIT_YES)
    group_dict.icon = icon
    group_dict.plugin_id = -1
    group_dict.plugin_ui = False
    group_dict.plugin_inline = False
    group_dict.widgets = [group_box, None]

    if split == SPLIT_YES:
        group_box.setSplit(True, PORT_MODE_OUTPUT)

        if features.handle_group_pos:
            group_box.setPos(getStoredCanvasPosition(group_name + "_OUTPUT", CanvasGetNewGroupPos(False)))
        elif old_matching_group is not None:
            group_box.setPos(old_matching_group[1])
        else:
            group_box.setPos(CanvasGetNewGroupPos(False))

        group_sbox = CanvasBox(group_id, group_name, icon)
        group_sbox.positionChanged.connect(canvas.qobject.sboxPositionChanged)
        group_sbox.blockSignals(True)
        group_sbox.setSplit(True, PORT_MODE_INPUT)

        group_dict.widgets[1] = group_sbox

        if features.handle_group_pos:
            group_sbox.setPos(getStoredCanvasPosition(group_name + "_INPUT", CanvasGetNewGroupPos(True)))
        elif old_matching_group is not None and old_matching_group[0]:
            group_sbox.setPos(old_matching_group[2])
        else:
            group_sbox.setPos(group_box.x() + group_box.boundingRect().width() + 300, group_box.y())

        canvas.last_z_value += 1
        group_sbox.setZValue(canvas.last_z_value)

        if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
            CanvasItemFX(group_sbox, True, False)

        group_sbox.checkItemPos()
        group_sbox.blockSignals(False)

    else:
        group_box.setSplit(False)

        if features.handle_group_pos:
            group_box.setPos(getStoredCanvasPosition(group_name, CanvasGetNewGroupPos(False)))
        elif old_matching_group is not None:
            group_box.setPos(old_matching_group[1])
        else:
            # Special ladish fake-split groups
            horizontal = bool(icon == ICON_HARDWARE or icon == ICON_LADISH_ROOM)
            group_box.setPos(CanvasGetNewGroupPos(horizontal))

    canvas.last_z_value += 1
    group_box.setZValue(canvas.last_z_value)

    group_box.checkItemPos()
    group_box.blockSignals(False)

    canvas.group_list.append(group_dict)

    if options.eyecandy == EYECANDY_FULL and not options.auto_hide_groups:
        CanvasItemFX(group_box, True, False)
    else:
        QTimer.singleShot(0, canvas.scene.update)

    return group_dict

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
            canvas.group_plugin_map.pop(group.plugin_id, None)

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
    group_pos = None
    plugin_id = -1
    plugin_ui = False
    plugin_inline = False
    ports_data = []
    conns_data = []

    # Step 1 - Store all Item data
    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split:
                if canvas.debug:
                    print("PatchCanvas::splitGroup(%i) - group is already split" % group_id)
                return

            item = group.widgets[0]
            group_name = group.group_name
            group_icon = group.icon
            group_pos = item.pos()
            plugin_id = group.plugin_id
            plugin_ui = group.plugin_ui
            plugin_inline = group.plugin_inline
            break

    if not item:
        qCritical("PatchCanvas::splitGroup(%i) - unable to find group to split" % group_id)
        return

    port_list_ids = list(item.getPortList())

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id in port_list_ids:
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
        if (connection.group_in_id == group_id and connection.port_in_id in port_list_ids) or \
           (connection.group_out_id == group_id and connection.port_out_id in port_list_ids):
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

    # Step 3 - Re-create Item, now split
    group = addGroup(group_id, group_name, SPLIT_YES, group_icon)

    if plugin_id >= 0:
        setGroupAsPlugin(group_id, plugin_id, plugin_ui, plugin_inline)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type, port.is_alternate)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.group_out_id, conn.port_out_id, conn.group_in_id, conn.port_in_id, True)

    if group is not None:
        pos1 = group.widgets[0].pos()
        pos2 = group.widgets[1].pos()
        group2_pos = QPointF(group_pos.x() + group.widgets[1].boundingRect().width() * 3/2, group_pos.y())
        group.widgets[0].blockSignals(True)
        group.widgets[0].setPos(group_pos)
        group.widgets[0].blockSignals(False)
        group.widgets[1].blockSignals(True)
        group.widgets[1].setPos(group2_pos)
        group.widgets[1].checkItemPos()
        group.widgets[1].blockSignals(False)
        valueStr = "%i:%i:%i:%i" % (group_pos.x(), group_pos.y(), group2_pos.x(), group2_pos.y())
        CanvasCallback(ACTION_GROUP_POSITION, group_id, 0, valueStr)

    QTimer.singleShot(0, canvas.scene.update)

def joinGroup(group_id):
    if canvas.debug:
        print("PatchCanvas::joinGroup(%i)" % group_id)

    item = None
    s_item = None
    group_name = ""
    group_icon = ICON_APPLICATION
    group_pos = None
    plugin_id = -1
    plugin_ui = False
    plugin_inline = False
    ports_data = []
    conns_data = []

    # Step 1 - Store all Item data
    for group in canvas.group_list:
        if group.group_id == group_id:
            if not group.split:
                if canvas.debug:
                    print("PatchCanvas::joinGroup(%i) - group is not split" % group_id)
                return

            item = group.widgets[0]
            s_item = group.widgets[1]
            group_name = group.group_name
            group_icon = group.icon
            group_pos = item.pos()
            plugin_id = group.plugin_id
            plugin_ui = group.plugin_ui
            plugin_inline = group.plugin_inline
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
        if port.group_id == group_id and port.port_id in port_list_ids:
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
        if (connection.group_in_id == group_id and connection.port_in_id in port_list_ids) or \
           (connection.group_out_id == group_id and connection.port_out_id in port_list_ids):
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
    group = addGroup(group_id, group_name, SPLIT_NO, group_icon)

    if plugin_id >= 0:
        setGroupAsPlugin(group_id, plugin_id, plugin_ui, plugin_inline)

    for port in ports_data:
        addPort(group_id, port.port_id, port.port_name, port.port_mode, port.port_type, port.is_alternate)

    for conn in conns_data:
        connectPorts(conn.connection_id, conn.group_out_id, conn.port_out_id, conn.group_in_id, conn.port_in_id, True)

    if group is not None:
        group.widgets[0].blockSignals(True)
        group.widgets[0].setPos(group_pos)
        group.widgets[0].checkItemPos()
        group.widgets[0].blockSignals(False)
        valueStr = "%i:%i:%i:%i" % (group_pos.x(), group_pos.y(), 0, 0)
        CanvasCallback(ACTION_GROUP_POSITION, group_id, 0, valueStr)

    QTimer.singleShot(0, canvas.scene.update)

# ------------------------------------------------------------------------------------------------------------

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

    mapping = {}

    for group in canvas.group_list:
        mapping[group.group_name] = group

    for data in dataList:
        name = data['name']
        group = mapping.get(name, None)

        if group is None:
            continue

        group.widgets[0].blockSignals(True)
        group.widgets[0].setPos(data['pos1x'], data['pos1y'])
        group.widgets[0].blockSignals(False)

        if group.split and group.widgets[1]:
            group.widgets[1].blockSignals(True)
            group.widgets[1].setPos(data['pos2x'], data['pos2y'])
            group.widgets[1].blockSignals(False)

def setGroupPos(group_id, group_pos_x, group_pos_y):
    setGroupPosFull(group_id, group_pos_x, group_pos_y, group_pos_x, group_pos_y)

def setGroupPosFull(group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i):
    if canvas.debug:
        print("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i)" % (
              group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.widgets[0].blockSignals(True)
            group.widgets[0].setPos(group_pos_x_o, group_pos_y_o)
            group.widgets[0].checkItemPos()
            group.widgets[0].blockSignals(False)

            if group.split and group.widgets[1]:
                group.widgets[1].blockSignals(True)
                group.widgets[1].setPos(group_pos_x_i, group_pos_y_i)
                group.widgets[1].checkItemPos()
                group.widgets[1].blockSignals(False)

            QTimer.singleShot(0, canvas.scene.update)
            return

    qCritical("PatchCanvas::setGroupPos(%i, %i, %i, %i, %i) - unable to find group to reposition" % (
              group_id, group_pos_x_o, group_pos_y_o, group_pos_x_i, group_pos_y_i))

# ------------------------------------------------------------------------------------------------------------

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

def setGroupAsPlugin(group_id, plugin_id, hasUI, hasInlineDisplay):
    if canvas.debug:
        print("PatchCanvas::setGroupAsPlugin(%i, %i, %s, %s)" % (
              group_id, plugin_id, bool2str(hasUI), bool2str(hasInlineDisplay)))

    for group in canvas.group_list:
        if group.group_id == group_id:
            group.plugin_id = plugin_id
            group.plugin_ui = hasUI
            group.plugin_inline = hasInlineDisplay
            group.widgets[0].setAsPlugin(plugin_id, hasUI, hasInlineDisplay)

            if group.split and group.widgets[1]:
                group.widgets[1].setAsPlugin(plugin_id, hasUI, hasInlineDisplay)

            canvas.group_plugin_map[plugin_id] = group
            return

    qCritical("PatchCanvas::setGroupAsPlugin(%i, %i, %s, %s) - unable to find group to set as plugin" % (
              group_id, plugin_id, bool2str(hasUI), bool2str(hasInlineDisplay)))

# ------------------------------------------------------------------------------------------------------------

def focusGroupUsingPluginId(plugin_id):
    if canvas.debug:
        print("PatchCanvas::focusGroupUsingPluginId(%i)" % (plugin_id,))

    if plugin_id < 0 or plugin_id >= MAX_PLUGIN_ID_ALLOWED:
        return False

    for group in canvas.group_list:
        if group.plugin_id == plugin_id:
            item = group.widgets[0]
            canvas.scene.clearSelection()
            canvas.scene.getView().centerOn(item)
            item.setSelected(True)
            return True

def focusGroupUsingGroupName(group_name):
    if canvas.debug:
        print("PatchCanvas::focusGroupUsingGroupName(%s)" % (group_name,))

    for group in canvas.group_list:
        if group.group_name == group_name:
            item = group.widgets[0]
            canvas.scene.clearSelection()
            canvas.scene.getView().centerOn(item)
            item.setSelected(True)
            return True

# ------------------------------------------------------------------------------------------------------------

def addPort(group_id, port_id, port_name, port_mode, port_type, is_alternate=False):
    if canvas.debug:
        print("PatchCanvas::addPort(%i, %i, %s, %s, %s, %s)" % (
              group_id, port_id, port_name.encode(),
              port_mode2str(port_mode), port_type2str(port_type), bool2str(is_alternate)))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            qWarning("PatchCanvas::addPort(%i, %i, %s, %s, %s) - port already exists" % (
                     group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))
            return

    box_widget = None
    port_widget = None

    for group in canvas.group_list:
        if group.group_id == group_id:
            if group.split and group.widgets[0].getSplitMode() != port_mode and group.widgets[1]:
                n = 1
            else:
                n = 0
            box_widget = group.widgets[n]
            port_widget = box_widget.addPortFromGroup(port_id, port_mode, port_type, port_name, is_alternate)
            break

    if not (box_widget and port_widget):
        qCritical("PatchCanvas::addPort(%i, %i, %s, %s, %s) - Unable to find parent group" % (
                  group_id, port_id, port_name.encode(), port_mode2str(port_mode), port_type2str(port_type)))
        return

    port_dict = port_dict_t()
    port_dict.group_id = group_id
    port_dict.port_id = port_id
    port_dict.port_name = port_name
    port_dict.port_mode = port_mode
    port_dict.port_type = port_type
    port_dict.is_alternate = is_alternate
    port_dict.widget = port_widget
    canvas.port_list.append(port_dict)

    box_widget.updatePositions()

    if options.eyecandy == EYECANDY_FULL:
        CanvasItemFX(port_widget, True, False)
        return

    QTimer.singleShot(0, canvas.scene.update)

def removePort(group_id, port_id):
    if canvas.debug:
        print("PatchCanvas::removePort(%i, %i)" % (group_id, port_id))

    for port in canvas.port_list:
        if port.group_id == group_id and port.port_id == port_id:
            item = port.widget
            try:
                pitem = item.parentItem()
                canvas.scene.removeItem(item)
            except RuntimeError:
                pass
            else:
                pitem.removePortFromGroup(port_id)
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

    qCritical("PatchCanvas::renamePort(%i, %i, %s) - Unable to find port to rename" % (
              group_id, port_id, new_port_name.encode()))

def connectPorts(connection_id, group_out_id, port_out_id, group_in_id, port_in_id, fromSplitOrJoin = False):
    if canvas.last_connection_id >= connection_id and not fromSplitOrJoin:
        print("PatchCanvas::connectPorts(%i, %i, %i, %i, %i) - invalid connection id received (last: %i)" % (
              connection_id, group_out_id, port_out_id, group_in_id, port_in_id, canvas.last_connection_id))
        return

    canvas.last_connection_id = connection_id

    if canvas.debug:
        print("PatchCanvas::connectPorts(%i, %i, %i, %i, %i)" % (
              connection_id, group_out_id, port_out_id, group_in_id, port_in_id))

    port_out = None
    port_in = None
    port_out_parent = None
    port_in_parent = None

    for port in canvas.port_list:
        if port.group_id == group_out_id and port.port_id == port_out_id:
            port_out = port.widget
            port_out_parent = port_out.parentItem()
        elif port.group_id == group_in_id and port.port_id == port_in_id:
            port_in = port.widget
            port_in_parent = port_in.parentItem()

    # FIXME
    if not (port_out and port_in):
        qCritical("PatchCanvas::connectPorts(%i, %i, %i, %i, %i) - unable to find ports to connect" % (
                  connection_id, group_out_id, port_out_id, group_in_id, port_in_id))
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
        CanvasItemFX(item, True, False)
        return

    QTimer.singleShot(0, canvas.scene.update)

def disconnectPorts(connection_id):
    if canvas.debug:
        print("PatchCanvas::disconnectPorts(%i)" % connection_id)

    line = None
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

    item1p = item1.parentItem()
    item2p = item2.parentItem()
    if item1p:
        item1p.removeLineFromGroup(connection_id)
    if item2p:
        item2p.removeLineFromGroup(connection_id)

    if options.eyecandy == EYECANDY_FULL:
        CanvasItemFX(line, False, True)
        return

    canvas.scene.removeItem(line)
    del line

    QTimer.singleShot(0, canvas.scene.update)

# ------------------------------------------------------------------------------------------------------------

def arrange():
    if canvas.debug:
        print("PatchCanvas::arrange()")

# ------------------------------------------------------------------------------------------------------------

def updateZValues():
    if canvas.debug:
        print("PatchCanvas::updateZValues()")

    for group in canvas.group_list:
        group.widgets[0].resetLinesZValue()

        if group.split and group.widgets[1]:
            group.widgets[1].resetLinesZValue()

# ------------------------------------------------------------------------------------------------------------

def redrawPluginGroup(plugin_id):
    group = canvas.group_plugin_map.get(plugin_id, None)

    if group is None:
        #qCritical("PatchCanvas::redrawPluginGroup(%i) - unable to find group" % plugin_id)
        return

    group.widgets[0].redrawInlineDisplay()

    if group.split and group.widgets[1]:
        group.widgets[1].redrawInlineDisplay()

def handlePluginRemoved(plugin_id):
    if canvas.debug:
        print("PatchCanvas::handlePluginRemoved(%i)" % plugin_id)

    canvas.scene.clearSelection()

    group = canvas.group_plugin_map.pop(plugin_id, None)

    if group is not None:
        group.plugin_id = -1
        group.plugin_ui = False
        group.plugin_inline = False
        group.widgets[0].removeAsPlugin()

        if group.split and group.widgets[1]:
            group.widgets[1].removeAsPlugin()

    for group in canvas.group_list:
        if group.plugin_id < plugin_id or group.plugin_id > MAX_PLUGIN_ID_ALLOWED:
            continue

        group.plugin_id -= 1
        group.widgets[0].m_plugin_id -= 1

        if group.split and group.widgets[1]:
            group.widgets[1].m_plugin_id -= 1

        canvas.group_plugin_map[plugin_id] = group

def handleAllPluginsRemoved():
    if canvas.debug:
        print("PatchCanvas::handleAllPluginsRemoved()")

    canvas.group_plugin_map = {}

    for group in canvas.group_list:
        if group.plugin_id < 0:
            continue
        if group.plugin_id > MAX_PLUGIN_ID_ALLOWED:
            continue

        group.plugin_id = -1
        group.plugin_ui = False
        group.plugin_inline = False
        group.widgets[0].removeAsPlugin()

        if group.split and group.widgets[1]:
            group.widgets[1].removeAsPlugin()

# ------------------------------------------------------------------------------------------------------------
