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

from PyQt5.QtCore import qCritical, Qt, QPointF, QRectF, QTimer
from PyQt5.QtGui import QCursor, QFont, QFontMetrics, QLinearGradient, QPainter, QPen
from PyQt5.QtWidgets import QGraphicsItem, QMenu

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    features,
    options,
    port_dict_t,
    CanvasBoxType,
    ANTIALIASING_FULL,
    ACTION_PLUGIN_EDIT,
    ACTION_PLUGIN_SHOW_UI,
    ACTION_PLUGIN_CLONE,
    ACTION_PLUGIN_REMOVE,
    ACTION_PLUGIN_RENAME,
    ACTION_PLUGIN_REPLACE,
    ACTION_GROUP_INFO,
    ACTION_GROUP_JOIN,
    ACTION_GROUP_SPLIT,
    ACTION_GROUP_RENAME,
    ACTION_PORTS_DISCONNECT,
    EYECANDY_FULL,
    PORT_MODE_NULL,
    PORT_MODE_INPUT,
    PORT_MODE_OUTPUT,
    PORT_TYPE_NULL,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
    MAX_PLUGIN_ID_ALLOWED,
)

from .canvasboxshadow import CanvasBoxShadow
from .canvasicon import CanvasIcon
from .canvasport import CanvasPort
from .theme import Theme
from .utils import CanvasItemFX, CanvasGetFullPortName, CanvasGetPortConnectionList
#from .canvasportglow import CanvasPortGlow

# ------------------------------------------------------------------------------------------------------------

class cb_line_t(object):
    def __init__(self, line, connection_id):
        self.line = line
        self.connection_id = connection_id

# ------------------------------------------------------------------------------------------------------------

class CanvasBox(QGraphicsItem):
    def __init__(self, group_id, group_name, icon, parent=None):
        QGraphicsItem.__init__(self, parent)

        # Save Variables, useful for later
        self.m_group_id = group_id
        self.m_group_name = group_name

        # plugin Id, < 0 if invalid
        self.m_plugin_id = -1
        self.m_plugin_ui = False

        # Base Variables
        self.p_width = 50
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
        QTimer.singleShot(0, self.fixPos)

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
        port_dict.port_id = port_id
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

    def addLineFromGroup(self, line, connection_id):
        new_cbline = cb_line_t(line, connection_id)
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
            if not (canvas.size_rect.contains(pos) and
                    canvas.size_rect.contains(pos + QPointF(self.p_width, self.p_height))):
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
        self.p_width = max(50, app_name_size)

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
            port_types = [PORT_TYPE_AUDIO_JACK, PORT_TYPE_MIDI_JACK, PORT_TYPE_MIDI_ALSA, PORT_TYPE_PARAMETER]
            last_in_type = last_out_type = PORT_TYPE_NULL
            last_in_pos = last_out_pos = canvas.theme.box_header_height + canvas.theme.box_header_spacing

            for port_type in port_types:
                for port in port_list:
                    if port.port_type != port_type:
                        continue

                    size = QFontMetrics(self.m_font_port).width(port.port_name)

                    if port.port_mode == PORT_MODE_INPUT:
                        max_in_width = max(max_in_width, size)
                        if port.port_type != last_in_type:
                            if last_in_type != PORT_TYPE_NULL:
                                last_in_pos += canvas.theme.port_spacingT
                            last_in_type = port.port_type
                        port.widget.setY(last_in_pos)
                        last_in_pos += port_spacing

                    elif port.port_mode == PORT_MODE_OUTPUT:
                        max_out_width = max(max_out_width, size)
                        if port.port_type != last_out_type:
                            if last_out_type != PORT_TYPE_NULL:
                                last_out_pos += canvas.theme.port_spacingT
                            last_out_type = port.port_type
                        port.widget.setY(last_out_pos)
                        last_out_pos += port_spacing

            self.p_width = max(self.p_width, 30 + max_in_width + max_out_width)

            # Horizontal ports re-positioning
            inX = canvas.theme.port_offset
            outX = self.p_width - max_out_width - canvas.theme.port_offset - 12
            for port_type in port_types:
                for port in port_list:
                    if port.port_mode == PORT_MODE_INPUT:
                        port.widget.setX(inX)
                        port.widget.setPortWidth(max_in_width)

                    elif port.port_mode == PORT_MODE_OUTPUT:
                        port.widget.setX(outX)
                        port.widget.setPortWidth(max_out_width)

            self.p_height = max(last_in_pos, last_out_pos)
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

        conn_list = []
        conn_list_ids = []

        for port_id in self.m_port_list_ids:
            tmp_conn_list = CanvasGetPortConnectionList(self.m_group_id, port_id)
            for tmp_conn_id, tmp_group_id, tmp_port_id in tmp_conn_list:
                if tmp_conn_id not in conn_list_ids:
                    conn_list.append((tmp_conn_id, tmp_group_id, tmp_port_id))
                    conn_list_ids.append(tmp_conn_id)

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
            act_p_ui = menu.addAction("Show Custom UI")
            menu.addSeparator()
            act_p_clone = menu.addAction("Clone")
            act_p_rename = menu.addAction("Rename...")
            act_p_replace = menu.addAction("Replace...")
            act_p_remove = menu.addAction("Remove")

            if not self.m_plugin_ui:
                act_p_ui.setVisible(False)

        else:
            act_p_edit = act_p_ui = None
            act_p_clone = act_p_rename = None
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
            event.accept()
            canvas.callback(ACTION_PLUGIN_REMOVE, self.m_plugin_id, 0, "")
            return
        QGraphicsItem.keyPressEvent(self, event)

    def hoverEnterEvent(self, event):
        if options.auto_select_items:
            if len(canvas.scene.selectedItems()) > 0:
                canvas.scene.clearSelection()
            self.setSelected(True)
        QGraphicsItem.hoverEnterEvent(self, event)

    def mouseDoubleClickEvent(self, event):
        if self.m_plugin_id >= 0:
            event.accept()
            canvas.callback(ACTION_PLUGIN_SHOW_UI if self.m_plugin_ui else ACTION_PLUGIN_EDIT, self.m_plugin_id, 0, "")
            return

        QGraphicsItem.mouseDoubleClickEvent(self, event)

    def mousePressEvent(self, event):
        canvas.last_z_value += 1
        self.setZValue(canvas.last_z_value)
        self.resetLinesZValue()
        self.m_cursor_moving = False

        if event.button() == Qt.RightButton:
            event.accept()
            canvas.scene.clearSelection()
            self.setSelected(True)
            self.m_mouse_down = False
            return

        elif event.button() == Qt.LeftButton:
            if self.sceneBoundingRect().contains(event.scenePos()):
                self.m_mouse_down = True
            else:
                # FIXME: Check if still valid: Fix a weird Qt behaviour with right-click mouseMove
                self.m_mouse_down = False
                event.ignore()
                return

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
            self.unsetCursor()
            self.fixPos()
        self.m_mouse_down = False
        self.m_cursor_moving = False
        QGraphicsItem.mouseReleaseEvent(self, event)

    def moveEvent(self, event):
        if not self.m_mouse_down:
            self.fixPos()

    def fixPos(self):
        self.setX(round(self.x()))
        self.setY(round(self.y()))

    def boundingRect(self):
        return QRectF(0, 0, self.p_width, self.p_height)

    def paint(self, painter, option, widget):
        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, bool(options.antialiasing == ANTIALIASING_FULL))
        rect = QRectF(0, 0, self.p_width, self.p_height)

        # Draw rectangle
        pen = QPen(canvas.theme.box_pen_sel if self.isSelected() else canvas.theme.box_pen)
        pen.setWidthF(pen.widthF() + 0.00001)
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

# ------------------------------------------------------------------------------------------------------------
