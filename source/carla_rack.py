#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla rack widget code
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
    from PyQt5.QtCore import Qt, QSize, QTimer
    from PyQt5.QtGui import QPixmap
    from PyQt5.QtWidgets import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem
else:
    from PyQt4.QtCore import Qt, QSize, QTimer
    from PyQt4.QtGui import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPixmap

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import *
from carla_skin import *

# ------------------------------------------------------------------------------------------------------------
# Rack widget item

class CarlaRackItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1

    def __init__(self, parent, pluginId, useSkins):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)

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

        if False:
            self.fWidget = AbstractPluginSlot(parent, parent.host, pluginId)

    # --------------------------------------------------------------------------------------------------------

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId
        self.fWidget.setPluginId(pluginId)

    # --------------------------------------------------------------------------------------------------------

    def getEditDialog(self):
        return self.fWidget.fEditDialog

    def closeEditDialog(self):
        self.fWidget.fEditDialog.close()

    # --------------------------------------------------------------------------------------------------------

    def getWidget(self):
        return self.fWidget

    def recreateWidget(self):
        if self.fWidget is not None:
            #self.fWidget.fEditDialog.close()
            del self.fWidget

        self.fWidget = createPluginSlot(self.fParent, self.fParent.host, self.fPluginId, self.fUseSkins)
        self.fWidget.setFixedHeight(self.fWidget.getFixedHeight())

        self.setSizeHint(QSize(640, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

# ------------------------------------------------------------------------------------------------------------
# Rack widget list

class CarlaRackList(QListWidget):
    def __init__(self, parent, host):
        QListWidget.__init__(self, parent)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostMeta()
            self.host = host

        # -------------------------------------------------------------

        exts = host.get_supported_file_extensions().split(";")

        # plugin files
        exts.append("dll")

        if MACOS:
            exts.append("dylib")
        if not WINDOWS:
            exts.append("so")

        self.fSupportedExtensions = tuple(i.replace("*.","") for i in exts)
        self.fWasLastDragValid    = False

        self.setMinimumWidth(640+20) # required by zita, 591 was old value
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSortingEnabled(False)
        #self.setSortingEnabled(True)

        self.setDragEnabled(True)
        self.setDragDropMode(QAbstractItemView.DropOnly)
        self.setDropIndicatorShown(True)
        self.viewport().setAcceptDrops(True)

        self.setFrameShape(QFrame.NoFrame)
        self.setFrameShadow(QFrame.Plain)

        self.fPixmapL = QPixmap(":/bitmaps/rack_interior_left.png")
        self.fPixmapR = QPixmap(":/bitmaps/rack_interior_right.png")

        self.fPixmapWidth = self.fPixmapL.width()

    def isDragEventValid(self, urls):
        for url in urls:
            filename = url.toLocalFile()

            if os.path.isdir(filename):
                if os.path.exists(os.path.join(filename, "manifest.ttl")):
                    return True

            elif os.path.isfile(filename):
                if filename.lower().endswith(self.fSupportedExtensions):
                    return True

        return False

    def dragEnterEvent(self, event):
        if self.isDragEventValid(event.mimeData().urls()):
            self.fWasLastDragValid = True
            event.acceptProposedAction()
            return

        self.fWasLastDragValid = False
        QListWidget.dragEnterEvent(self, event)

    def dragMoveEvent(self, event):
        if self.fWasLastDragValid:
            event.acceptProposedAction()

            tryItem = self.itemAt(event.pos())

            if tryItem is not None:
                self.setCurrentRow(tryItem.widget.getPluginId())
            else:
                self.setCurrentRow(-1)
            return

        QListWidget.dragMoveEvent(self, event)

    #def dragLeaveEvent(self, event):
        #self.fWasLastDragValid = False
        #QListWidget.dragLeaveEvent(self, event)

    def dropEvent(self, event):
        event.acceptProposedAction()

        urls = event.mimeData().urls()

        if len(urls) == 0:
            return

        tryItem = self.itemAt(event.pos())

        if tryItem is not None:
            pluginId = tryItem.widget.getPluginId()
            self.host.replace_plugin(pluginId)

        for url in urls:
            filename = url.toLocalFile()

            if not self.host.load_file(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        if tryItem is not None:
            self.host.replace_plugin(self.parent().fPluginCount)
            #tryItem.widget.setActive(True, True, True)

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
# Rack widget

class CarlaRackW(QFrame):
#class CarlaRackW(QFrame, HostWidgetMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, doSetup = True):
        QFrame.__init__(self, parent)
        self.host = host
