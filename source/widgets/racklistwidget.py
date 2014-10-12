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

    def __init__(self, parent, pluginId, useSkins):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)
        self.host = parent.host

        if False:
            # kdevelop likes this :)
            parent = RackListWidget()
            host = CarlaHostMeta()
            self.host = host
            self.fWidget = AbstractPluginSlot()

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fParent   = parent
        self.fPluginId = pluginId
        self.fUseSkins = useSkins
        self.fWidget   = None

        self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled)
        #self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled|Qt.ItemIsDragEnabled|Qt.ItemIsDropEnabled)

        # ----------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.recreateWidget()

    # --------------------------------------------------------------------------------------------------------

    def close(self):
        self.fWidget.fEditDialog.close()
        self.fWidget.close()

    def getEditDialog(self):
        return self.fWidget.fEditDialog

    def getPluginId(self):
        return self.fPluginId

    def getWidget(self):
        return self.fWidget

    # --------------------------------------------------------------------------------------------------------

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId
        self.fWidget.setPluginId(pluginId)

    # --------------------------------------------------------------------------------------------------------

    def recreateWidget(self):
        if self.fWidget is not None:
            self.fWidget.fEditDialog.close()
            self.fWidget.close()
            del self.fWidget.fEditDialog
            del self.fWidget

        self.fWidget = createPluginSlot(self.fParent, self.host, self.fPluginId, self.fUseSkins)
        self.fWidget.setFixedHeight(self.fWidget.getFixedHeight())

        self.setSizeHint(QSize(700, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

# ------------------------------------------------------------------------------------------------------------
# Rack Widget

class RackListWidget(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)
        self.host = None

        if False:
            # kdevelop likes this :)
            from carla_backend import CarlaHostMeta
            host = CarlaHostMeta()
            self.host = host

        self.fSupportedExtensions = []
        self.fWasLastDragValid    = False

        self.setMinimumWidth(740)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSortingEnabled(False)

        self.setDragEnabled(True)
        self.setDragDropMode(QAbstractItemView.DropOnly)
        self.setDropIndicatorShown(True)
        self.viewport().setAcceptDrops(True)

        self.setFrameShape(QFrame.NoFrame)
        self.setFrameShadow(QFrame.Plain)

        self.fPixmapL = QPixmap(":/bitmaps/rack_interior_left.png")
        self.fPixmapR = QPixmap(":/bitmaps/rack_interior_right.png")

        self.fPixmapWidth = self.fPixmapL.width()

    # --------------------------------------------------------------------------------------------------------

    def createItem(self, pluginId, useSkins):
        return RackListItem(self, pluginId, useSkins)

    def setHost(self, host):
        self.host = host

        exts = host.get_supported_file_extensions().split(";")

        exts.append(".dll")

        if MACOS:
            exts.append(".dylib")
        if not WINDOWS:
            exts.append(".so")

        self.fSupportedExtensions = tuple(i.replace("*","") for i in exts)

    # --------------------------------------------------------------------------------------------------------

    def isDragUrlValid(self, url):
        filename = url.toLocalFile()

        if os.path.isdir(filename):
            if os.path.exists(os.path.join(filename, "manifest.ttl")):
                return True
            if filename.lower().endswith((".vst", ".vst3")):
                return True

        elif os.path.isfile(filename):
            if filename.lower().endswith(self.fSupportedExtensions):
                return True

        return False

    # --------------------------------------------------------------------------------------------------------

    def dragEnterEvent(self, event):
        urls = event.mimeData().urls()

        for url in urls:
            if self.isDragUrlValid(url):
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
        if self.itemAt(event.pos()) is None:
            event.accept()
            self.setCurrentRow(-1)
            return

        QListWidget.mousePressEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self.viewport())
        painter.drawTiledPixmap(0, 0, self.fPixmapWidth, self.height(), self.fPixmapL)
        painter.drawTiledPixmap(self.width()-self.fPixmapWidth-2, 0, self.fPixmapWidth, self.height(), self.fPixmapR)
        QListWidget.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
