#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Rack List Widget, a custom Qt4 widget
# Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

from carla_config import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

if config_UseQt5:
    from PyQt5.QtCore import Qt, QSize
    from PyQt5.QtGui import QPainter, QPixmap
    from PyQt5.QtWidgets import QAbstractItemView, QFrame, QListWidget, QListWidgetItem
else:
    from PyQt4.QtCore import Qt, QSize
    from PyQt4.QtGui import QAbstractItemView, QFrame, QListWidget, QListWidgetItem, QPainter, QPixmap

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_skin import *

# ------------------------------------------------------------------------------------------------------------
# Rack Widget item

class RackListItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1
    kMinimumWidth = 620

    def __init__(self, parent, pluginId):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)
        self.host = parent.host

        if False:
            # kdevelop likes this :)
            parent = RackListWidget()
            host = CarlaHostNull()
            self.host = host
            self.fWidget = AbstractPluginSlot()

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fParent   = parent
        self.fPluginId = pluginId
        self.fWidget   = None

        color   = self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaColor")
        skin    = self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaSkin")
        compact = bool(self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaSkinIsCompacted") == "true")

        if color:
            try:
                color = tuple(int(i) for i in color.split(";",3))
            except:
                print("Color value decode failed for", color)
                color = None
        else:
            color = None

        self.fOptions = {
            'color'  : color,
            'skin'   : skin,
            'compact': compact and skin != "classic",
        }

        self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled)
        #self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled|Qt.ItemIsDragEnabled)

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
        del widget.fEditDialog

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

    def isGuiShown(self):
        if self.fWidget is None or self.fWidget.b_gui is not None:
            return None
        return self.fWidget.b_gui.isChecked()

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

        self.setSizeHint(QSize(self.kMinimumWidth, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

        if not firstInit:
            self.host.set_custom_data(self.fPluginId, CUSTOM_DATA_TYPE_PROPERTY,
                                      "CarlaSkinIsCompacted", "true" if self.fOptions['compact'] else "false")

    def recreateWidget2(self, wasCompacted, wasGuiShown):
        self.fOptions['compact'] = wasCompacted

        self.close()

        self.fWidget = createPluginSlot(self.fParent, self.host, self.fPluginId, self.fOptions)
        self.fWidget.setFixedHeight(self.fWidget.getFixedHeight())

        if wasGuiShown and self.fWidget.b_gui is not None:
            self.fWidget.b_gui.setChecked(True)

        self.setSizeHint(QSize(self.kMinimumWidth, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

        self.host.set_custom_data(self.fPluginId, CUSTOM_DATA_TYPE_PROPERTY,
                                  "CarlaSkinIsCompacted", "true" if wasCompacted else "false")

# ------------------------------------------------------------------------------------------------------------
# Rack Widget

class RackListWidget(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)
        self.host = None
        self.fParent = None

        if False:
            # kdevelop likes this :)
            from carla_backend import CarlaHostMeta
            self.host = host = CarlaHostNull()

        exts = gCarla.utils.get_supported_file_extensions()

        self.fSupportedExtensions = tuple(("." + i) for i in exts)
        self.fLastSelectedItem    = None
        self.fWasLastDragValid    = False

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

    # --------------------------------------------------------------------------------------------------------

    def createItem(self, pluginId):
        return RackListItem(self, pluginId)

    def getPluginCount(self):
        return self.fParent.getPluginCount()

    def setHostAndParent(self, host, parent):
        self.host = host
        self.fParent = parent

    # --------------------------------------------------------------------------------------------------------

    def customClearSelection(self):
        self.setCurrentRow(-1)
        self.clearSelection()
        self.clearFocus()

    def isDragUrlValid(self, filename):
        if os.path.isdir(filename):
            #if os.path.exists(os.path.join(filename, "manifest.ttl")):
                #return True
            if MACOS and filename.lower().endswith(".vst"):
                return True

        elif os.path.isfile(filename):
            if filename.lower().endswith(self.fSupportedExtensions):
                return True

        return False

    # --------------------------------------------------------------------------------------------------------

    def dragEnterEvent(self, event):
        urls = event.mimeData().urls()

        for url in urls:
            if self.isDragUrlValid(url.toLocalFile()):
                self.fWasLastDragValid = True
                event.acceptProposedAction()
                return

        self.fWasLastDragValid = False
        QListWidget.dragEnterEvent(self, event)

    def dragMoveEvent(self, event):
        if not self.fWasLastDragValid:
            QListWidget.dragMoveEvent(self, event)
            return

        event.acceptProposedAction()

        tryItem = self.itemAt(event.pos())

        if tryItem is not None:
            self.setCurrentRow(tryItem.getPluginId())
        else:
            self.setCurrentRow(-1)

    def dragLeaveEvent(self, event):
        self.fWasLastDragValid = False
        QListWidget.dragLeaveEvent(self, event)

    # --------------------------------------------------------------------------------------------------------

    # FIXME: this needs some attention

    # if dropping project file over 1 plugin, load it in rack or patchbay
    # if dropping regular files over 1 plugin, keep replacing plugins

    def dropEvent(self, event):
        event.acceptProposedAction()

        urls = event.mimeData().urls()

        if len(urls) == 0:
            return

        tryItem = self.itemAt(event.pos())

        if tryItem is not None:
            pluginId = tryItem.getPluginId()
        else:
            pluginId = -1

        for url in urls:
            if pluginId >= 0:
                self.host.replace_plugin(pluginId)
                pluginId += 1

                if pluginId > self.host.get_current_plugin_count():
                    pluginId = -1

            filename = url.toLocalFile()

            if not self.host.load_file(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        if tryItem is not None:
            self.host.replace_plugin(self.host.get_max_plugin_number())
            #tryItem.widget.setActive(True, True, True)

    # --------------------------------------------------------------------------------------------------------

    def mousePressEvent(self, event):
        if self.itemAt(event.pos()) is None and self.currentRow() != -1:
            event.accept()
            self.customClearSelection()
            return

        QListWidget.mousePressEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self.viewport())
        painter.drawTiledPixmap(0, 0, self.fPixmapWidth, self.height(), self.fPixmapL)
        painter.drawTiledPixmap(self.width()-self.fPixmapWidth, 0, self.fPixmapWidth, self.height(), self.fPixmapR)
        QListWidget.paintEvent(self, event)

    # --------------------------------------------------------------------------------------------------------

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

# ------------------------------------------------------------------------------------------------------------
