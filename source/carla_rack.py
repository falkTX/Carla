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

    # -----------------------------------------------------------------

    def close(self):
        self.fWidget.ui.edit_dialog.close()

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

        #self.setMnimumWidth(800)
        self.setSortingEnabled(False)

        app  = QApplication.instance()
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

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return self.fPluginCount

    def getPlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None

        return pitem

    # -----------------------------------------------------------------

    def addPlugin(self, pluginId, isProjectLoading):
        pitem = CarlaRackItem(self, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        if not isProjectLoading:
            pitem.fWidget.setActive(True, True, True)

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

    def renamePlugin(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.ui.label_name.setText(name)
        pitem.fWidget.ui.edit_dialog.setName(name)

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

    # -----------------------------------------------------------------

    def setParameterValue(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setParameterValue(index, value)

    def setParameterDefault(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setParameterDefault(index, value)

    def setParameterMidiChannel(self, pluginId, index, channel):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setParameterMidiChannel(index, channel)

    def setParameterMidiCC(self, pluginId, index, cc):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setParameterMidiControl(index, cc)

    # -----------------------------------------------------------------

    def setProgram(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setProgram(index)

    def setMidiProgram(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.setMidiProgram(index)

    # -----------------------------------------------------------------

    def noteOn(self, pluginId, channel, note, velocity):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.sendNoteOn(channel, note)

    def noteOff(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.fWidget.sendNoteOff(channel, note)

    # -----------------------------------------------------------------

    def setGuiState(self, pluginId, state):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        if state == 0:
            pitem.fWidget.ui.b_gui.setChecked(False)
            pitem.fWidget.ui.b_gui.setEnabled(True)
        elif state == 1:
            pitem.fWidget.ui.b_gui.setChecked(True)
            pitem.fWidget.ui.b_gui.setEnabled(True)
        elif state == -1:
            pitem.fWidget.ui.b_gui.setChecked(False)
            pitem.fWidget.ui.b_gui.setEnabled(False)

    # -----------------------------------------------------------------

    def updateInfo(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.ui.edit_dialog.updateInfo()

    def reloadInfo(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.ui.edit_dialog.reloadInfo()

    def reloadParameters(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.ui.edit_dialog.reloadParameters()

    def reloadPrograms(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.ui.edit_dialog.reloadPrograms()

    def reloadAll(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.ui.edit_dialog.reloadAll()

    # -----------------------------------------------------------------

    def patchbayClientAdded(self, clientId, clientIcon, clientName):
        pass

    def patchbayClientRemoved(self, clientId, clientName):
        pass

    def patchbayClientRenamed(self, clientId, newClientName):
        pass

    def patchbayPortAdded(self, clientId, portId, portFlags, portName):
        pass

    def patchbayPortRemoved(self, groupId, portId, fullPortName):
        pass

    def patchbayPortRenamed(self, groupId, portId, newPortName):
        pass

    def patchbayConnectionAdded(self, connectionId, portOutId, portInId):
        pass

    def patchbayConnectionRemoved(self, connectionId):
        pass

    def patchbayIconChanged(self, clientId, clientIcon):
        pass

    # -----------------------------------------------------------------

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

# ------------------------------------------------------------------------------------------------------------
# TESTING

#from PyQt5.QtWidgets import QApplication
#app = QApplication(sys.argv)
#gui = CarlaRackW(None)
#gui.show()
#app.exec_()
