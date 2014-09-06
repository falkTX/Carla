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
    from PyQt5.QtWidgets import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QScrollBar
else:
    from PyQt4.QtCore import Qt, QSize, QTimer
    from PyQt4.QtGui import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPixmap, QScrollBar

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

class CarlaRackW(QFrame, HostWidgetMeta):
#class CarlaRackW(QFrame, HostWidgetMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, doSetup = True):
        QFrame.__init__(self, parent)
        self.host = host

        if False:
            # kdevelop likes this :)
            host = CarlaHostMeta()
            self.host = host

        # -------------------------------------------------------------

        self.fLayout = QHBoxLayout(self)
        self.fLayout.setContentsMargins(0, 0, 0, 0)
        self.fLayout.setSpacing(0)
        self.setLayout(self.fLayout)

        self.fPadLeft  = QLabel(self)
        self.fPadLeft.setFixedWidth(25)
        self.fPadLeft.setObjectName("PadLeft")
        self.fPadLeft.setText("")

        self.fPadRight = QLabel(self)
        self.fPadRight.setFixedWidth(25)
        self.fPadRight.setObjectName("PadRight")
        self.fPadRight.setText("")

        self.fRack = CarlaRackList(self, host)
        self.fRack.setObjectName("CarlaRackList")
        self.fRack.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.fRack.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.fRack.currentRowChanged.connect(self.slot_currentRowChanged)

        sb = self.fRack.verticalScrollBar()
        self.fScrollBar = QScrollBar(Qt.Vertical, self)
        self.fScrollBar.setMinimum(sb.minimum())
        self.fScrollBar.setMaximum(sb.maximum())
        self.fScrollBar.setValue(sb.value())

        #sb.actionTriggered.connect(self.fScrollBar.triggerAction)
        #sb.sliderMoved.connect(self.fScrollBar.)
        #sb.sliderPressed.connect(self.fScrollBar.)
        #sb.sliderReleased.connect(self.fScrollBar.)
        sb.rangeChanged.connect(self.fScrollBar.setRange)
        sb.valueChanged.connect(self.fScrollBar.setValue)
        self.fScrollBar.rangeChanged.connect(sb.setRange)
        self.fScrollBar.valueChanged.connect(sb.setValue)

        self.fLayout.addWidget(self.fPadLeft)
        self.fLayout.addWidget(self.fRack)
        self.fLayout.addWidget(self.fPadRight)
        self.fLayout.addWidget(self.fScrollBar)

        # -------------------------------------------------------------
        # Internal stuff

        self.fParent      = parent
        self.fPluginCount = 0
        self.fPluginList  = []

        self.fCurrentRow = -1
        self.fLastSelectedItem = None

        # -------------------------------------------------------------
        # Set-up GUI stuff

        #app  = QApplication.instance()
        #pal1 = app.palette().base().color()
        #pal2 = app.palette().button().color()
        #col1 = "stop:0 rgb(%i, %i, %i)" % (pal1.red(), pal1.green(), pal1.blue())
        #col2 = "stop:1 rgb(%i, %i, %i)" % (pal2.red(), pal2.green(), pal2.blue())

        self.setStyleSheet("""
          QLabel#PadLeft {
            background-image: url(:/bitmaps/rack_padding_left.png);
            background-repeat: repeat-y;
          }
          QLabel#PadRight {
            background-image: url(:/bitmaps/rack_padding_right.png);
            background-repeat: repeat-y;
          }
          CarlaRackList#CarlaRackList {
            background-color: black;
          }
        """)

        # -------------------------------------------------------------
        # Connect actions to functions

        host.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        host.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        host.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

        if not doSetup: return

        parent.ui.menu_Canvas.hide()

        parent.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        parent.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        parent.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        parent.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        parent.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        parent.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        parent.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        parent.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)

        parent.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

    # -----------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName):
        pitem = CarlaRackItem(self.fRack, pluginId, self.fParent.getSavedSettings()[CARLA_KEY_MAIN_USE_CUSTOM_SKINS])

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        if not self.fParent.isProjectLoading():
            pitem.getWidget().setActive(True, True, True)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        pitem = self.getPluginItem(pluginId)

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)
        self.fRack.takeItem(pluginId)

        if pitem is not None:
            pitem.closeEditDialog()
            del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pitem = self.fPluginList[i]
            pitem.setPluginId(i)

    # -----------------------------------------------------------------
    # HostWidgetMeta methods

    def removeAllPlugins(self):
        while self.fRack.takeItem(0):
            pass

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.closeEditDialog()
            del pitem

        self.fPluginCount = 0
        self.fPluginList  = []

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass

    def idleFast(self):
        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().idleFast()

    def idleSlow(self):
        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().idleSlow()

    def projectLoadingStarted(self):
        self.fRack.setEnabled(False)

    def projectLoadingFinished(self):
        self.fRack.setEnabled(True)

    def saveSettings(self, settings):
        pass

    def showEditDialog(self, pluginId):
        dialog = self.getPluginEditDialog(pluginId)

        if dialog is None:
            return

        dialog.show()

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setActive(True, True, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setActive(False, True, True)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setInternalParameter(PLUGIN_CAN_VOLUME, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setInternalParameter(PLUGIN_CAN_VOLUME, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setInternalParameter(PLUGIN_CAN_DRYWET, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setInternalParameter(PLUGIN_CAN_DRYWET, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not self.host.is_engine_running():
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().setInternalParameter(PARAMETER_BALANCE_LEFT, -1.0)
            pitem.getWidget().setInternalParameter(PARAMETER_BALANCE_RIGHT, 1.0)
            pitem.getWidget().setInternalParameter(PARAMETER_PANNING, 0.0)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self, self.host, False, False)
        if not dialog.exec_():
            return

        self.fParent.loadSettings(False)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        self.fRack.setCurrentRow(-1)
        self.fCurrentRow = -1
        self.fLastSelectedItem = None

        pitem.recreateWidget()

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_currentRowChanged(self, row):
        self.fCurrentRow = row

        if self.fLastSelectedItem is not None:
            self.fLastSelectedItem.setSelected(False)

        if row < 0 or row >= self.fPluginCount or self.fPluginList[row] is None:
            self.fLastSelectedItem = None
            return

        pitem = self.fPluginList[row]
        pitem.getWidget().setSelected(True)
        self.fLastSelectedItem = pitem.getWidget()

    # -----------------------------------------------------------------

    def getPluginItem(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        if False:
            pitem = CarlaRackItem(self, 0, False)

        return pitem

    def getPluginEditDialog(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        if False:
            pitem = CarlaRackItem(self, 0, False)

        return pitem.getEditDialog()

    def getPluginSlotWidget(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        if False:
            pitem = CarlaRackItem(self, 0, False)

        return pitem.getWidget()

    # -----------------------------------------------------------------
