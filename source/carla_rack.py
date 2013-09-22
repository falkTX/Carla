#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla rack widget code
# Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

try:
    from PyQt5.QtCore import QSize, QTimer
    from PyQt5.QtWidgets import QApplication, QListWidget, QListWidgetItem
except:
    from PyQt4.QtCore import QSize, QTimer
    from PyQt4.QtGui import QApplication, QListWidget, QListWidgetItem

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_widgets import *

# ------------------------------------------------------------------------------------------------------------
# Rack widget item

class CarlaRackItem(QListWidgetItem):
    RackItemType = QListWidgetItem.UserType + 1
    StaticHeight = 32

    def __init__(self, parent, pluginId):
        QListWidgetItem.__init__(self, parent, self.RackItemType)

        self.fWidget = PluginWidget(parent, pluginId)
        self.fWidget.setFixedHeight(self.StaticHeight)

        self.setSizeHint(QSize(300, self.StaticHeight))

        parent.setItemWidget(self, self.fWidget)

    def close(self):
        self.fWidget.ui.edit_dialog.close()

    def getWidget(self):
        return self.fWidget

    def setId(self, idx):
        self.fWidget.setId(idx)

# ------------------------------------------------------------------------------------------------------------
# Rack widget

class CarlaRackW(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)

        # -------------------------------------------------------------
        # Internal stuff

        self.fPluginCount = 0
        self.fPluginList  = []

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.setFixedWidth(800)
        self.setSortingEnabled(False)

        app = QApplication.instance()
        pal1 = app.palette().base().color()
        pal2 = app.palette().button().color()
        col1 = "stop:0 rgb(%i, %i, %i)" % (pal1.red(), pal1.green(), pal1.blue())
        col2 = "stop:1 rgb(%i, %i, %i)" % (pal2.red(), pal2.green(), pal2.blue())

        self.setStyleSheet("""
          QListWidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                %s,
                %s
            );
          }
        """ % (col1, col2))

        # -------------------------------------------------------------
        # TESTING

        #self.addPlugin(0)
        #self.addPlugin(1)
        #self.addPlugin(2)
        #self.addPlugin(3)
        #self.addPlugin(4)

        #self.removePlugin(3)

        #QTimer.singleShot(3000, self.testRemove)
        #QTimer.singleShot(5000, self.removeAllPlugins)

    def testRemove(self):
        self.removePlugin(0)

    def idleFast(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.fWidget.idleFast()

    def idleSlow(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.fWidget.idleSlow()

    def addPlugin(self, pluginId):
        pitem = CarlaRackItem(self, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        #if not self.fProjectLoading:
            #pwidget.setActive(True, True, True)

    def removePlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        self.takeItem(pluginId)

        pitem.close()
        del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            self.fPluginList[i].setId(i)

    def removeAllPlugins(self):
        while (self.takeItem(0)):
            pass

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.close()
            del pitem

        self.fPluginCount = 0
        self.fPluginList  = []

# ------------------------------------------------------------------------------------------------------------
# TESTING

#from PyQt5.QtWidgets import QApplication
#app = QApplication(sys.argv)
#gui = CarlaRackW(None)
#gui.show()
#app.exec_()
