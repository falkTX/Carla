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

from carla_skin import createPluginSlot

# ------------------------------------------------------------------------------------------------------------
# Rack Widget item

class CarlaRackItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1

    def __init__(self, parent, pluginId, useSkins):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)
        self.host = parent.host

        if False:
            # kdevelop likes this :)
            from carla_backend import CarlaHostMeta
            host = CarlaHostMeta()
            self.host = host

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

    def closeEditDialog(self):
        self.fWidget.fEditDialog.close()

    def getEditDialog(self):
        return self.fWidget.fEditDialog

    def getWidget(self):
        return self.fWidget

    # --------------------------------------------------------------------------------------------------------

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId
        self.fWidget.setPluginId(pluginId)

    # --------------------------------------------------------------------------------------------------------

    def recreateWidget(self):
        if self.fWidget is not None:
            #self.fWidget.fEditDialog.close()
            del self.fWidget

        self.fWidget = createPluginSlot(self.fParent, self.fParent.host, self.fPluginId, self.fUseSkins)
        self.fWidget.setFixedHeight(self.fWidget.getFixedHeight())

        self.setSizeHint(QSize(640, self.fWidget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.fWidget)

# ------------------------------------------------------------------------------------------------------------
# Rack Widget

class RackListWidget(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)

        self.fSupportedExtensions = []
        self.fWasLastDragValid    = False

        self.setMinimumWidth(700)
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

    def paintEvent(self, event):
        painter = QPainter(self.viewport())
        painter.drawTiledPixmap(0, 0, self.fPixmapWidth, self.height(), self.fPixmapL)
        painter.drawTiledPixmap(self.width()-self.fPixmapWidth-2, 0, self.fPixmapWidth, self.height(), self.fPixmapR)
        QListWidget.paintEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
