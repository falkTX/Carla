#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os

# ------------------------------------------------------------------------------------------------------------
# Imports (PyQt)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import Qt, QSize, QRect, QEvent, QPersistentModelIndex
    from PyQt5.QtGui import QColor, QDrag, QPainter, QPen, QPixmap
    from PyQt5.QtWidgets import QAbstractItemView, QListWidget, QListWidgetItem, QMessageBox
elif qt_config == 6:
    from PyQt6.QtCore import Qt, QSize, QRect, QEvent, QPersistentModelIndex
    from PyQt6.QtGui import QColor, QDrag, QPainter, QPen, QPixmap
    from PyQt6.QtWidgets import QAbstractItemView, QListWidget, QListWidgetItem, QMessageBox

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_backend import CARLA_OS_MAC, CUSTOM_DATA_TYPE_PROPERTY
from carla_shared import gCarla, CustomMessageBox
from carla_skin import createPluginSlot

# ------------------------------------------------------------------------------------------------------------
# Rack Widget item

class RackListItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1
    kMinimumWidth = 620

    def __init__(self, parent, pluginId, useClassicSkin):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)
        self.host = parent.host

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fParent   = parent
        self.fPluginId = pluginId
        self.fWidget   = None

        color   = self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaColor")
        skin    = self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaSkin")
        compact = bool(self.host.get_custom_data_value(pluginId,
                                                       CUSTOM_DATA_TYPE_PROPERTY,
                                                       "CarlaSkinIsCompacted") == "true")

        if color:
            try:
                color = tuple(int(i) for i in color.split(";",3))
            except Exception as e:
                print("Color value decode failed for", color, "error was:", e)
                color = None
        else:
            color = None

        if useClassicSkin and not skin:
            skin = "classic"

        self.fOptions = {
            'color'  : color,
            'skin'   : skin,
            'compact': compact and skin != "classic",
        }

        flags = Qt.ItemIsSelectable | Qt.ItemIsEnabled
        if getattr(self.host, "isFranca", False):
            flags |= Qt.ItemIsDragEnabled
        self.setFlags(flags)

        # ----------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.recreateWidget(firstInit = True)

    # --------------------------------------------------------------------------------------------------------

    def close(self):
        if self.fWidget is None:
            return

        widget = self.fWidget
        self.fWidget = None

        self.fParent.customClearSelection()
        self.fParent.setItemWidget(self, None)

        widget.fEditDialog.close()
        widget.fEditDialog.setParent(None)
        widget.fEditDialog.deleteLater()
        widget.fEditDialog = None

        widget.close()
        widget.setParent(None)
        widget.deleteLater()
        del widget

    def getEditDialog(self):
        if self.fWidget is None:
            return None

        return self.fWidget.fEditDialog

    def getPluginId(self):
        return self.fPluginId

    def getWidget(self):
        return self.fWidget

    def isCompacted(self):
        return self.fOptions['compact']

    # --------------------------------------------------------------------------------------------------------

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId

        if self.fWidget is not None:
            self.fWidget.setPluginId(pluginId)

    def setSelected(self, select):
        if self.fWidget is not None:
            self.fWidget.setSelected(select)

        QListWidgetItem.setSelected(self, select)

    # --------------------------------------------------------------------------------------------------------

    def setCompacted(self, compact):
        self.fOptions['compact'] = compact

    # --------------------------------------------------------------------------------------------------------

    def compact(self):
        if self.fOptions['compact']:
            return
        self.recreateWidget(True)

    def expand(self):
        if not self.fOptions['compact']:
            return
        self.recreateWidget(True)

    def recreateWidget(self, invertCompactOption = False, firstInit = False, newColor = None, newSkin = None):
        if invertCompactOption:
            self.fOptions['compact'] = not self.fOptions['compact']
        if newColor is not None:
            self.fOptions['color'] = newColor
        if newSkin is not None:
            self.fOptions['skin'] = newSkin

        wasGuiShown = None

        if self.fWidget is not None and self.fWidget.b_gui is not None:
            wasGuiShown = self.fWidget.b_gui.isChecked()

        self.close()

        self.fWidget = createPluginSlot(self.fParent, self.host, self.fPluginId, self.fOptions)
        self.fWidget.setFixedHeight(self.fWidget.getFixedHeight())

        if wasGuiShown and self.fWidget.b_gui is not None:
            self.fWidget.b_gui.setChecked(True)

        width = self.fWidget.minimumSizeHint().width() if getattr(self.host, "isFranca", False) else self.kMinimumWidth
        self.setSizeHint(QSize(width, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

        if not firstInit:
            self.host.set_custom_data(self.fPluginId, CUSTOM_DATA_TYPE_PROPERTY,
                                      "CarlaSkinIsCompacted", "true" if self.fOptions['compact'] else "false")

# ------------------------------------------------------------------------------------------------------------
# Rack Widget

class RackListWidget(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)
        self.host = None
        self.fParent = None

        exts = gCarla.utils.get_supported_file_extensions()

        self.fSupportedExtensions = tuple(("." + i) for i in exts)
        self.fLastSelectedItem    = None
        self.fWasLastDragValid    = False
        self.fDragIndex           = QPersistentModelIndex()
        self.fDropPosition        = None

        self.fPixmapL     = QPixmap(":/bitmaps/rack_interior_left.png")
        self.fPixmapR     = QPixmap(":/bitmaps/rack_interior_right.png")
        self.fPixmapWidth = self.fPixmapL.width()

        self.setMinimumWidth(RackListItem.kMinimumWidth)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSortingEnabled(False)

        self.setDragEnabled(True)
        self.setDragDropMode(QAbstractItemView.DropOnly)
        self.setDropIndicatorShown(True)
        self.viewport().setAcceptDrops(True)

        self._updateStyle()

    # --------------------------------------------------------------------------------------------------------

    def createItem(self, pluginId, useClassicSkin):
        return RackListItem(self, pluginId, useClassicSkin)

    def getPluginCount(self):
        return self.fParent.getPluginCount()

    def setHostAndParent(self, host, parent):
        self.host = host
        self.fParent = parent
        if getattr(host, "isFranca", False):
            self.setDragDropMode(QAbstractItemView.DragDropMode.DragDrop)
            self.setDropIndicatorShown(False)

    # --------------------------------------------------------------------------------------------------------

    def customClearSelection(self):
        self.setCurrentRow(-1)
        self.clearSelection()
        self.clearFocus()

    def isDragUrlValid(self, filename):
        if not filename:
            return False

        if filename[-1] == '/':
            filename = filename[:-1]

        lfilename = filename.lower()

        if os.path.isdir(filename):
            #if os.path.exists(os.path.join(filename, "manifest.ttl")):
                #return True
            if CARLA_OS_MAC and lfilename.endswith(".vst"):
                return True
            if lfilename.endswith(".vst3") and ".vst3" in self.fSupportedExtensions:
                return True

        elif os.path.isfile(filename):
            if lfilename.endswith(self.fSupportedExtensions):
                return True

        return False

    # --------------------------------------------------------------------------------------------------------

    def startDrag(self, supportedActions):
        index = self.currentIndex()
        if not index.isValid():
            return

        self.fDragIndex = QPersistentModelIndex(index)
        drag = QDrag(self)
        drag.setMimeData(self.model().mimeData([index]))
        try:
            drag.exec(Qt.DropAction.MoveAction)
        finally:
            self.fDragIndex = QPersistentModelIndex()
            self.fDropPosition = None
            self.viewport().update()
            drag.deleteLater()

    @staticmethod
    def dragPosition(event):
        return event.pos() if qt_config == 5 else event.position().toPoint()

    def rackDropRow(self, position):
        for row in range(self.count()):
            if position.y() <= self.visualItemRect(self.item(row)).center().y():
                return row
        return self.count()

    def dragEnterEvent(self, event):
        if event.source() is self and self.fDragIndex.isValid():
            event.acceptProposedAction()
            return

        urls = event.mimeData().urls()

        for url in urls:
            if self.isDragUrlValid(url.toLocalFile()):
                self.fWasLastDragValid = True
                event.acceptProposedAction()
                return

        self.fWasLastDragValid = False
        QListWidget.dragEnterEvent(self, event)

    def dragMoveEvent(self, event):
        if event.source() is self and self.fDragIndex.isValid():
            # Let Qt handle scrolling near the viewport edges.
            QListWidget.dragMoveEvent(self, event)
            event.acceptProposedAction()
            self.fDropPosition = self.dragPosition(event)
            self.viewport().update()
            return

        if not self.fWasLastDragValid:
            QListWidget.dragMoveEvent(self, event)
            return

        event.acceptProposedAction()

        tryItem = self.itemAt(self.dragPosition(event))

        if tryItem is not None:
            self.setCurrentRow(tryItem.getPluginId())
        else:
            self.setCurrentRow(-1)

    def dragLeaveEvent(self, event):
        self.fDropPosition = None
        self.viewport().update()
        if self.fWasLastDragValid:
            self.fWasLastDragValid = False
        QListWidget.dragLeaveEvent(self, event)

    # --------------------------------------------------------------------------------------------------------

    # FIXME: this needs some attention

    # if dropping project file over 1 plugin, load it in rack or patchbay
    # if dropping regular files over 1 plugin, keep replacing plugins

    def dropEvent(self, event):
        if event.source() is self:
            if self.fDragIndex.isValid():
                source = self.fDragIndex.row()
                gap = self.rackDropRow(self.dragPosition(event))
                self.fDropPosition = None
                self.fParent.movePlugin(source, gap - int(gap > source))
                event.acceptProposedAction()
            return

        event.acceptProposedAction()

        urls = event.mimeData().urls()

        if not urls:
            return

        tryItem = self.itemAt(self.dragPosition(event))

        if tryItem is not None:
            pluginId = tryItem.getPluginId()
        else:
            pluginId = -1

        for url in urls:
            filename = url.toLocalFile()
            if not self.isDragUrlValid(filename):
                continue

            filename = filename.rstrip('/')
            isProject = filename.lower().endswith((".carxp", ".carxs"))
            replacing = pluginId >= 0 and not isProject
            if replacing:
                self.host.replace_plugin(pluginId)
                pluginId += 1

                if pluginId >= self.host.get_current_plugin_count():
                    pluginId = -1

            ok = self.host.load_file(filename)
            if replacing:
                self.host.replace_plugin(self.host.get_max_plugin_number())
            if not ok:
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
                continue

            if filename.lower().endswith(".carxp"):
                gCarla.gui.loadExternalCanvasGroupPositionsIfNeeded(filename)

    # --------------------------------------------------------------------------------------------------------

    def mousePressEvent(self, event):
        if self.itemAt(event.pos()) is None and self.currentRow() != -1:
            event.accept()
            self.customClearSelection()
            return

        QListWidget.mousePressEvent(self, event)

    def changeEvent(self, event):
        if event.type() in (QEvent.StyleChange, QEvent.PaletteChange):
            self._updateStyle()
        QListWidget.changeEvent(self, event)

    def paintEvent(self, event):
        if getattr(self.host, "isFranca", False):
            QListWidget.paintEvent(self, event)
            if self.fDropPosition is not None and self.fDragIndex.isValid():
                row = self.rackDropRow(self.fDropPosition)
                if row not in (self.fDragIndex.row(), self.fDragIndex.row() + 1):
                    last = row == self.count()
                    rect = self.visualItemRect(self.item(row - 1 if last else row))
                    y = rect.bottom() - 1 if last else max(1, rect.top())
                    painter = QPainter(self.viewport())
                    painter.setPen(QPen(self.palette().highlight().color(), 2))
                    painter.drawLine(rect.left(), y, rect.right(), y)
            return

        painter = QPainter(self.viewport())

        width = self.width()
        height = self.height()
        imgL_rect = QRect(0, 0, self.fPixmapWidth, height)
        imgR_rect = QRect(width-self.fPixmapWidth, 0, self.fPixmapWidth, height)

        painter.setBrush(self.rail_col)
        painter.setPen(Qt.NoPen)
        painter.drawRects(imgL_rect, imgR_rect)
        painter.setCompositionMode(QPainter.CompositionMode_Multiply)
        painter.drawTiledPixmap(imgL_rect, self.fPixmapL)
        painter.drawTiledPixmap(imgR_rect, self.fPixmapR)
        painter.setCompositionMode(QPainter.CompositionMode_Plus)
        painter.drawTiledPixmap(imgL_rect, self.fPixmapL)
        painter.drawTiledPixmap(imgR_rect, self.fPixmapR)
        painter.setCompositionMode(QPainter.CompositionMode_SourceOver)

        painter.setPen(self.edge_col)
        painter.setBrush(Qt.NoBrush)
        painter.drawRect(self.fPixmapWidth, 0, width-self.fPixmapWidth*2, height)

        QListWidget.paintEvent(self, event)

    def selectionChanged(self, selected, deselected):
        for index in deselected.indexes():
            item = self.itemFromIndex(index)
            if item is not None:
                item.setSelected(False)

        for index in selected.indexes():
            item = self.itemFromIndex(index)
            if item is not None:
                item.setSelected(True)

        QListWidget.selectionChanged(self, selected, deselected)

    # --------------------------------------------------------------------------------------------------------

    def _updateStyle(self):
        palette = self.palette()

        bg_color = palette.window().color()
        base_color = palette.base().color()
        text_color = palette.text().color()
        r0,g0,b0,_ = bg_color.getRgb()
        r1,g1,b1,_ = text_color.getRgb()

        self.rail_col = QColor(int((r0*3+r1)/4), int((g0*3+g1)/4), int((b0*3+b1)/4))
        self.edge_col = (self.rail_col if self.rail_col.blackF() > base_color.blackF() else base_color).darker(115)

# ------------------------------------------------------------------------------------------------------------
