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

from carla_skin import *

# ------------------------------------------------------------------------------------------------------------
# Rack widget item

class CarlaRackItem(QListWidgetItem):
    kRackItemType = QListWidgetItem.UserType + 1

    def __init__(self, parent, pluginId):
        QListWidgetItem.__init__(self, parent, self.kRackItemType)

        self.fParent = parent

        self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled)
        #self.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled|Qt.ItemIsDragEnabled|Qt.ItemIsDropEnabled)

        self.createWidget(pluginId)

    # -----------------------------------------------------------------

    def createWidget(self, pluginId):
        self.widget = createPluginSlot(self.fParent, pluginId)
        self.widget.setFixedHeight(self.widget.getFixedHeight())

        self.setSizeHint(QSize(640, self.widget.getFixedHeight()))

        self.fParent.setItemWidget(self, self.widget)

    # -----------------------------------------------------------------

    def close(self):
        self.widget.fEditDialog.close()

    def reloadAll(self, pluginId):
        self.widget.fEditDialog.close()
        del self.widget
        self.createWidget(pluginId)

# ------------------------------------------------------------------------------------------------------------
# Rack widget list

class CarlaRackList(QListWidget):
    def __init__(self, parent):
        QListWidget.__init__(self, parent)

        exts = gCarla.host.get_supported_file_extensions().split(";") if gCarla.host is not None else ["wav",]

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
            gCarla.host.replace_plugin(pluginId)

        for url in urls:
            filename = url.toLocalFile()

            if not gCarla.host.load_file(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 gCarla.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        if tryItem is not None:
            gCarla.host.replace_plugin(self.parent().fPluginCount)
            tryItem.widget.setActive(True, True, True)

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
    def __init__(self, parent, doSetup = True):
        QFrame.__init__(self, parent)

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

        self.fRack = CarlaRackList(self)
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
          QListWidget {
            background-color: black;
          }
        """)

        # -------------------------------------------------------------
        # Connect actions to functions

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

        parent.ParameterValueChangedCallback.connect(self.slot_handleParameterValueChangedCallback)
        parent.ParameterDefaultChangedCallback.connect(self.slot_handleParameterDefaultChangedCallback)
        parent.ParameterMidiChannelChangedCallback.connect(self.slot_handleParameterMidiChannelChangedCallback)
        parent.ParameterMidiCcChangedCallback.connect(self.slot_handleParameterMidiCcChangedCallback)
        parent.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        parent.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        parent.UiStateChangedCallback.connect(self.slot_handleUiStateChangedCallback)
        parent.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        parent.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        parent.UpdateCallback.connect(self.slot_handleUpdateCallback)
        parent.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        parent.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        parent.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        parent.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return self.fPluginCount

    # -----------------------------------------------------------------

    def addPlugin(self, pluginId, isProjectLoading):
        pitem = CarlaRackItem(self.fRack, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        if not isProjectLoading:
            pitem.widget.setActive(True, True, True)

    def removePlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        self.fRack.takeItem(pluginId)

        pitem.close()
        del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pitem = self.fPluginList[i]
            pitem.widget.setId(i)

    def renamePlugin(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setName(newName)

    def disablePlugin(self, pluginId, errorMsg):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

    def removeAllPlugins(self):
        while self.fRack.takeItem(0):
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

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass

    def engineChanged(self):
        pass

    # -----------------------------------------------------------------

    def idleFast(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.widget.idleFast()

    def idleSlow(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.widget.idleSlow()

    # -----------------------------------------------------------------

    def projectLoadingStarted(self):
        self.fRack.setEnabled(False)

    def projectLoadingFinished(self):
        self.fRack.setEnabled(True)

    # -----------------------------------------------------------------

    def saveSettings(self, settings):
        pass

    def showEditDialog(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.slot_showEditDialog(True)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setActive(True, True, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setActive(False, True, True)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_VOLUME, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_VOLUME, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_DRYWET, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PLUGIN_CAN_DRYWET, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not gCarla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            pitem.widget.setInternalParameter(PARAMETER_BALANCE_LEFT, -1.0)
            pitem.widget.setInternalParameter(PARAMETER_BALANCE_RIGHT, 1.0)
            pitem.widget.setInternalParameter(PARAMETER_PANNING, 0.0)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_configureCarla(self):
        if self.fParent is None or not self.fParent.openSettingsWindow(False, False):
            return

        self.fParent.loadSettings(False)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterValue(index, value, True)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterDefault(index, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, index, cc):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterMidiControl(index, cc)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, index, channel):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setParameterMidiChannel(index, channel)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setProgram(index, True)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.setMidiProgram(index, True)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_handleUiStateChangedCallback(self, pluginId, state):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.customUiStateChanged(state)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.sendNoteOn(channel, note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.sendNoteOff(channel, note)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.widget.fEditDialog.reloadPrograms()

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

        pitem.reloadAll(pluginId)

    # -----------------------------------------------------------------

    def slot_currentRowChanged(self, row):
        self.fCurrentRow = row

        if self.fLastSelectedItem is not None:
            self.fLastSelectedItem.setSelected(False)

        if row < 0 or row >= self.fPluginCount or self.fPluginList[row] is None:
            self.fLastSelectedItem = None
            return

        pitem = self.fPluginList[row]
        pitem.widget.setSelected(True)
        self.fLastSelectedItem = pitem.widget

    # -----------------------------------------------------------------
