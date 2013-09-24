#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla patchbay widget code
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
    from PyQt5.QtWidgets import QGraphicsView
except:
    from PyQt4.QtGui import QGraphicsView

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas

from carla_widgets import *

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    #try:
    from PyQt5.QtOpenGL import QGLWidget
    #except:
        #from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------
# Patchbay widget

class CarlaPatchbayW(QGraphicsView):
    def __init__(self, parent):
        QGraphicsView.__init__(self, parent)

        # -------------------------------------------------------------
        # Internal stuff

        self.fPluginCount = 0
        self.fPluginList  = []

        # -------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self) # FIXME?
        self.setScene(self.scene)
        #self.setRenderHint(QPainter.Antialiasing, bool(self.fSavedSettings["Canvas/Antialiasing"] == patchcanvas.ANTIALIASING_FULL))
        #if self.fSavedSettings["Canvas/UseOpenGL"] and hasGL:
            #self.setViewport(QGLWidget(self))
            #self.setRenderHint(QPainter.HighQualityAntialiasing, self.fSavedSettings["Canvas/HighQualityAntialiasing"])

        #pOptions = patchcanvas.options_t()
        #pOptions.theme_name       = self.fSavedSettings["Canvas/Theme"]
        #pOptions.auto_hide_groups = self.fSavedSettings["Canvas/AutoHideGroups"]
        #pOptions.use_bezier_lines = self.fSavedSettings["Canvas/UseBezierLines"]
        #pOptions.antialiasing     = self.fSavedSettings["Canvas/Antialiasing"]
        #pOptions.eyecandy         = self.fSavedSettings["Canvas/EyeCandy"]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = False
        pFeatures.port_rename  = False
        pFeatures.handle_group_pos = True

        #patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Carla", self.scene, CanvasCallback, True)

        #patchcanvas.setCanvasSize(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)
        #patchcanvas.setInitialPos(DEFAULT_CANVAS_WIDTH / 2, DEFAULT_CANVAS_HEIGHT / 2)
        #self.setSceneRect(0, 0, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT)

    # -----------------------------------------------------------------

    def recheckPluginHints(self, hints):
        pass

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
        pitem = PluginEdit(self, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

    def removePlugin(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        pitem.close()
        del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            self.fPluginList[i].fPluginId = i # FIXME ?

    def renamePlugin(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setName(name)

    def removeAllPlugins(self):
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

        pitem.setParameterValue(index, value)

    def setParameterDefault(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterDefault(index, value)

    def setParameterMidiChannel(self, pluginId, index, channel):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterMidiChannel(index, channel)

    def setParameterMidiCC(self, pluginId, index, cc):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterMidiControl(index, cc)

    # -----------------------------------------------------------------

    def setProgram(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setProgram(index)

    def setMidiProgram(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setMidiProgram(index)

    # -----------------------------------------------------------------

    def noteOn(self, pluginId, channel, note, velocity):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.sendNoteOn(channel, note)

    def noteOff(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.sendNoteOff(channel, note)

    # -----------------------------------------------------------------

    def setGuiState(self, pluginId, state):
        pass

    # -----------------------------------------------------------------

    def updateInfo(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.updateInfo()

    def reloadInfo(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadInfo()

    def reloadParameters(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadParameters()

    def reloadPrograms(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadPrograms()

    def reloadAll(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadAll()

    # -----------------------------------------------------------------

    def patchbayClientAdded(self, clientId, clientIcon, clientName):
        pcSplit = patchcanvas.SPLIT_UNDEF
        pcIcon  = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcSplit = patchcanvas.SPLIT_YES
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE
        elif clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN

        patchcanvas.addGroup(clientId, clientName, pcSplit, pcIcon)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayClientRemoved(self, clientId, clientName):
        #if not self.fEngineStarted: return
        patchcanvas.removeGroup(clientId)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayClientRenamed(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayPortAdded(self, clientId, portId, portFlags, portName):
        if (portFlags & PATCHBAY_PORT_IS_INPUT):
            portMode = patchcanvas.PORT_MODE_INPUT
        elif (portFlags & PATCHBAY_PORT_IS_OUTPUT):
            portMode = patchcanvas.PORT_MODE_OUTPUT
        else:
            portMode = patchcanvas.PORT_MODE_NULL

        if (portFlags & PATCHBAY_PORT_IS_AUDIO):
            portType = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif (portFlags & PATCHBAY_PORT_IS_MIDI):
            portType = patchcanvas.PORT_TYPE_MIDI_JACK
        else:
            portType = patchcanvas.PORT_TYPE_NULL

        patchcanvas.addPort(clientId, portId, portName, portMode, portType)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayPortRemoved(self, groupId, portId, fullPortName):
        #if not self.fEngineStarted: return
        patchcanvas.removePort(portId)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayPortRenamed(self, groupId, portId, newPortName):
        patchcanvas.renamePort(portId, newPortName)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayConnectionAdded(self, connectionId, portOutId, portInId):
        patchcanvas.connectPorts(connectionId, portOutId, portInId)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayConnectionRemoved(self, connectionId):
        #if not self.fEngineStarted: return
        patchcanvas.disconnectPorts(connectionId)
        #QTimer.singleShot(0, self.ui.miniCanvasPreview, SLOT("update()"))

    def patchbayIconChanged(self, clientId, clientIcon):
        pcIcon = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE
        elif clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN

        patchcanvas.setGroupIcon(clientId, pcIcon)

    # -----------------------------------------------------------------

    def idleFast(self):
        pass

    def idleSlow(self):
        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]

            if pitem is None:
                break

            pitem.idleSlow()

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_canvasArrange(self):
        patchcanvas.arrange()

    @pyqtSlot()
    def slot_canvasRefresh(self):
        patchcanvas.clear()
        if Carla.host.is_engine_running():
            Carla.host.patchbay_refresh()
        QTimer.singleShot(1000 if self.fSavedSettings['Canvas/EyeCandy'] else 0, self.ui.miniCanvasPreview, SLOT("update()"))

    @pyqtSlot()
    def slot_canvasZoomFit(self):
        self.scene.zoom_fit()

    @pyqtSlot()
    def slot_canvasZoomIn(self):
        self.scene.zoom_in()

    @pyqtSlot()
    def slot_canvasZoomOut(self):
        self.scene.zoom_out()

    @pyqtSlot()
    def slot_canvasZoomReset(self):
        self.scene.zoom_reset()

    @pyqtSlot()
    def slot_canvasPrint(self):
        self.scene.clearSelection()
        self.fExportPrinter = QPrinter()
        dialog = QPrintDialog(self.fExportPrinter, self)

        if dialog.exec_():
            painter = QPainter(self.fExportPrinter)
            painter.save()
            painter.setRenderHint(QPainter.Antialiasing)
            painter.setRenderHint(QPainter.TextAntialiasing)
            self.scene.render(painter)
            painter.restore()

    @pyqtSlot()
    def slot_canvasSaveImage(self):
        newPath = QFileDialog.getSaveFileName(self, self.tr("Save Image"), filter=self.tr("PNG Image (*.png);;JPEG Image (*.jpg)"))

        if newPath:
            self.scene.clearSelection()

            # FIXME - must be a better way...
            if newPath.endswith((".jpg", ".jpG", ".jPG", ".JPG", ".JPg", ".Jpg")):
                imgFormat = "JPG"
            elif newPath.endswith((".png", ".pnG", ".pNG", ".PNG", ".PNg", ".Png")):
                imgFormat = "PNG"
            else:
                # File-dialog may not auto-add the extension
                imgFormat = "PNG"
                newPath  += ".png"

            self.fExportImage = QImage(self.scene.sceneRect().width(), self.scene.sceneRect().height(), QImage.Format_RGB32)
            painter = QPainter(self.fExportImage)
            painter.save()
            painter.setRenderHint(QPainter.Antialiasing) # TODO - set true, cleanup this
            painter.setRenderHint(QPainter.TextAntialiasing)
            self.scene.render(painter)
            self.fExportImage.save(newPath, imgFormat, 100)
            painter.restore()

# ------------------------------------------------------------------------------------------------
# Canvas callback

def CanvasCallback(action, value1, value2, valueStr):
    if action == patchcanvas.ACTION_GROUP_INFO:
        pass

    elif action == patchcanvas.ACTION_GROUP_RENAME:
        pass

    elif action == patchcanvas.ACTION_GROUP_SPLIT:
        groupId = value1
        patchcanvas.splitGroup(groupId)
        Carla.gui.ui.miniCanvasPreview.update()

    elif action == patchcanvas.ACTION_GROUP_JOIN:
        groupId = value1
        patchcanvas.joinGroup(groupId)
        Carla.gui.ui.miniCanvasPreview.update()

    elif action == patchcanvas.ACTION_PORT_INFO:
        pass

    elif action == patchcanvas.ACTION_PORT_RENAME:
        pass

    elif action == patchcanvas.ACTION_PORTS_CONNECT:
        portIdA = value1
        portIdB = value2

        if not Carla.host.patchbay_connect(portIdA, portIdB):
            print("Connection failed:", cString(Carla.host.get_last_error()))

    elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
        connectionId = value1

        if not Carla.host.patchbay_disconnect(connectionId):
            print("Disconnect failed:", cString(Carla.host.get_last_error()))
