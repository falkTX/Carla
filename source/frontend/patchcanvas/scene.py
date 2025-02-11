#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from math import floor

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import QT_VERSION, pyqtSignal, pyqtSlot, qFatal, Qt, QPointF, QRectF
    from PyQt5.QtGui import QCursor, QPixmap, QPolygonF
    from PyQt5.QtWidgets import QGraphicsRectItem, QGraphicsScene
elif qt_config == 6:
    from PyQt6.QtCore import QT_VERSION, pyqtSignal, pyqtSlot, qFatal, Qt, QPointF, QRectF
    from PyQt6.QtGui import QCursor, QPixmap, QPolygonF
    from PyQt6.QtWidgets import QGraphicsRectItem, QGraphicsScene

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    CanvasBoxType,
    CanvasIconType,
    CanvasPortType,
    CanvasLineType,
    CanvasBezierLineType,
    CanvasRubberbandType,
    ACTION_BG_RIGHT_CLICK,
    MAX_PLUGIN_ID_ALLOWED,
)

# ------------------------------------------------------------------------------------------------------------

class RubberbandRect(QGraphicsRectItem):
    def __init__(self, scene):
        QGraphicsRectItem.__init__(self, QRectF(0, 0, 0, 0))

        self.setZValue(-1)
        self.hide()

        scene.addItem(self)

    def type(self):
        return CanvasRubberbandType

# ------------------------------------------------------------------------------------------------------------

class PatchScene(QGraphicsScene):
    scaleChanged = pyqtSignal(float)
    pluginSelected = pyqtSignal(list)

    def __init__(self, parent, view):
        QGraphicsScene.__init__(self, parent)

        self.m_connection_cut_mode = False
        self.m_scale_area = False
        self.m_mouse_down_init = False
        self.m_mouse_rubberband = False
        self.m_scale_min = 0.1
        self.m_scale_max = 4.0

        self.m_rubberband = RubberbandRect(self)
        self.m_rubberband_selection = False
        self.m_rubberband_orig_point = QPointF(0, 0)

        self.m_view = view
        if not self.m_view:
            qFatal("PatchCanvas::PatchScene() - invalid view")

        self.m_cursor_cut = None
        self.m_cursor_zoom = None

        self.setItemIndexMethod(QGraphicsScene.NoIndex)
        self.selectionChanged.connect(self.slot_selectionChanged)

    def getDevicePixelRatioF(self):
        if QT_VERSION < 0x50600:
            return 1.0

        return self.m_view.devicePixelRatioF()

    def getScaleFactor(self):
        return self.m_view.transform().m11()

    def getView(self):
        return self.m_view

    def fixScaleFactor(self, transform=None):
        fix, set_view = False, False
        if not transform:
            set_view = True
            view = self.m_view
            transform = view.transform()

        scale = transform.m11()
        if scale > self.m_scale_max:
            fix = True
            transform.reset()
            transform.scale(self.m_scale_max, self.m_scale_max)
        elif scale < self.m_scale_min:
            fix = True
            transform.reset()
            transform.scale(self.m_scale_min, self.m_scale_min)

        if set_view:
            if fix:
                view.setTransform(transform)
            self.scaleChanged.emit(transform.m11())

        return fix

    def updateLimits(self):
        w0 = canvas.size_rect.width()
        h0 = canvas.size_rect.height()
        w1 = self.m_view.width()
        h1 = self.m_view.height()
        self.m_scale_min = w1/w0 if w0/h0 > w1/h1 else h1/h0

    def updateTheme(self):
        self.setBackgroundBrush(canvas.theme.canvas_bg)
        self.m_rubberband.setPen(canvas.theme.rubberband_pen)
        self.m_rubberband.setBrush(canvas.theme.rubberband_brush)

        cur_color = "black" if canvas.theme.canvas_bg.blackF() < 0.5 else "white"
        self.m_cursor_cut = QCursor(QPixmap(":/cursors/cut_"+cur_color+".png"), 1, 1)
        self.m_cursor_zoom = QCursor(QPixmap(":/cursors/zoom-area_"+cur_color+".png"), 8, 7)

    def zoom_fit(self):
        min_x = min_y = max_x = max_y = None
        first_value = True

        items_list = self.items()

        if len(items_list) > 0:
            for item in items_list:
                if item and item.isVisible() and item.type() == CanvasBoxType:
                    pos = item.scenePos()
                    rect = item.boundingRect()

                    x = pos.x()
                    y = pos.y()
                    if first_value:
                        first_value = False
                        min_x, min_y = x, y
                        max_x = x + rect.width()
                        max_y = y + rect.height()
                    else:
                        min_x = min(min_x, x)
                        min_y = min(min_y, y)
                        max_x = max(max_x, x + rect.width())
                        max_y = max(max_y, y + rect.height())

            if not first_value:
                self.m_view.fitInView(min_x, min_y, abs(max_x - min_x), abs(max_y - min_y), Qt.KeepAspectRatio)
                self.fixScaleFactor()

    def zoom_in(self):
        view = self.m_view
        transform = view.transform()
        if transform.m11() < self.m_scale_max:
            transform.scale(1.2, 1.2)
            if transform.m11() > self.m_scale_max:
                transform.reset()
                transform.scale(self.m_scale_max, self.m_scale_max)
            view.setTransform(transform)
        self.scaleChanged.emit(transform.m11())

    def zoom_out(self):
        view = self.m_view
        transform = view.transform()
        if transform.m11() > self.m_scale_min:
            transform.scale(0.833333333333333, 0.833333333333333)
            if transform.m11() < self.m_scale_min:
                transform.reset()
                transform.scale(self.m_scale_min, self.m_scale_min)
            view.setTransform(transform)
        self.scaleChanged.emit(transform.m11())

    def zoom_reset(self):
        self.m_view.resetTransform()
        self.scaleChanged.emit(1.0)

    def handleMouseRelease(self):
        rubberband_active = self.m_rubberband_selection

        if self.m_scale_area and not self.m_rubberband_selection:
            self.m_scale_area = False
            self.m_view.viewport().unsetCursor()

        if self.m_rubberband_selection:
            if self.m_scale_area:
                self.m_scale_area = False
                self.m_view.viewport().unsetCursor()

                rect = self.m_rubberband.rect()
                self.m_view.fitInView(rect.x(), rect.y(), rect.width(), rect.height(), Qt.KeepAspectRatio)
                self.fixScaleFactor()

            else:
                items_list = self.items()
                for item in items_list:
                    if item and item.isVisible() and item.type() == CanvasBoxType:
                        item_rect = item.sceneBoundingRect()
                        item_top_left = QPointF(item_rect.x(), item_rect.y())
                        item_bottom_right = QPointF(item_rect.x() + item_rect.width(),
                                                    item_rect.y() + item_rect.height())

                        if self.m_rubberband.contains(item_top_left) and self.m_rubberband.contains(item_bottom_right):
                            item.setSelected(True)

            self.m_rubberband.hide()
            self.m_rubberband.setRect(0, 0, 0, 0)
            self.m_rubberband_selection = False

        self.m_mouse_rubberband = False

        self.stopConnectionCut()

        return rubberband_active

    def startConnectionCut(self):
        if self.m_cursor_cut:
            self.m_connection_cut_mode = True
            self.m_view.viewport().setCursor(self.m_cursor_cut)

    def stopConnectionCut(self):
        if self.m_connection_cut_mode:
            self.m_connection_cut_mode = False
            self.m_view.viewport().unsetCursor()

    def triggerRubberbandScale(self):
        self.m_scale_area = True
        if self.m_cursor_zoom:
            self.m_view.viewport().setCursor(self.m_cursor_zoom)

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
            event.ignore()
            return

        if event.key() == Qt.Key_Home:
            event.accept()
            self.zoom_fit()
            return

        if event.modifiers() & Qt.ControlModifier:
            if event.key() == Qt.Key_Plus:
                event.accept()
                self.zoom_in()
                return

            if event.key() == Qt.Key_Minus:
                event.accept()
                self.zoom_out()
                return

            if event.key() == Qt.Key_1:
                event.accept()
                self.zoom_reset()
                return

        QGraphicsScene.keyPressEvent(self, event)

    def keyReleaseEvent(self, event):
        self.stopConnectionCut()
        QGraphicsScene.keyReleaseEvent(self, event)

    def mousePressEvent(self, event):
        ctrlDown = bool(event.modifiers() & Qt.ControlModifier)

        self.m_mouse_down_init = (
            (event.button() == Qt.LeftButton) or ((event.button() == Qt.RightButton) and ctrlDown)
        )
        self.m_mouse_rubberband = False

        if event.button() == Qt.MiddleButton and ctrlDown:
            self.startConnectionCut()
            items = self.items(event.scenePos())
            for item in items:
                if item and item.type() in (CanvasLineType, CanvasBezierLineType, CanvasPortType):
                    item.triggerDisconnect()

        QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.m_mouse_down_init:
            self.m_mouse_down_init = False
            items = self.items(event.scenePos())
            for item in items:
                if item and item.type() in (CanvasBoxType, CanvasIconType, CanvasPortType):
                    self.m_mouse_rubberband = False
                    break
            else:
                self.m_mouse_rubberband = True

        if self.m_mouse_rubberband:
            event.accept()
            pos = event.scenePos()
            pos_x = pos.x()
            pos_y = pos.y()
            if not self.m_rubberband_selection:
                self.m_rubberband.show()
                self.m_rubberband_selection = True
                self.m_rubberband_orig_point = pos
            rubberband_orig_point = self.m_rubberband_orig_point

            x = min(pos_x, rubberband_orig_point.x())
            y = min(pos_y, rubberband_orig_point.y())

            lineHinting = canvas.theme.rubberband_pen.widthF() / 2
            self.m_rubberband.setRect(x+lineHinting,
                                      y+lineHinting,
                                      abs(pos_x - rubberband_orig_point.x()),
                                      abs(pos_y - rubberband_orig_point.y()))
            return

        if self.m_connection_cut_mode:
            trail = QPolygonF([event.scenePos(), event.lastScenePos(), event.scenePos()])
            items = self.items(trail)
            for item in items:
                if item and item.type() in (CanvasLineType, CanvasBezierLineType):
                    item.triggerDisconnect()

        QGraphicsScene.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        self.m_mouse_down_init = False

        if not self.handleMouseRelease():
            items_list = self.selectedItems()
            needs_update = False
            for item in items_list:
                if item and item.isVisible() and item.type() == CanvasBoxType:
                    item.checkItemPos()
                    needs_update = True

            if needs_update:
                canvas.scene.update()

        QGraphicsScene.mouseReleaseEvent(self, event)

    def zoom_wheel(self, delta):
        transform = self.m_view.transform()
        scale = transform.m11()

        if (delta > 0 and scale < self.m_scale_max) or (delta < 0 and scale > self.m_scale_min):
            factor = 1.41 ** (delta / 240.0)
            transform.scale(factor, factor)
            self.fixScaleFactor(transform)
            self.m_view.setTransform(transform)
            self.scaleChanged.emit(transform.m11())

    def wheelEvent(self, event):
        if not self.m_view:
            event.ignore()
            return

        if event.modifiers() & Qt.ControlModifier:
            event.accept()
            self.zoom_wheel(event.delta())
            return

        QGraphicsScene.wheelEvent(self, event)

    def contextMenuEvent(self, event):
        if self.handleMouseRelease():
            self.m_mouse_down_init = False
            QGraphicsScene.contextMenuEvent(self, event)
            return

        if event.modifiers() & Qt.ControlModifier:
            event.accept()
            self.triggerRubberbandScale()
            return

        if len(self.selectedItems()) == 0:
            self.m_mouse_down_init = False
            event.accept()
            canvas.callback(ACTION_BG_RIGHT_CLICK, 0, 0, "")
            return

        self.m_mouse_down_init = False
        QGraphicsScene.contextMenuEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
