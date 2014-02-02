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

from PyQt4.QtCore import QPointF, QTimer
from PyQt4.QtGui import QGraphicsView, QImage, QPrinter, QPrintDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas

from carla_widgets import *

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Carla Canvas defaults

CARLA_DEFAULT_CANVAS_SIZE_WIDTH  = 3100
CARLA_DEFAULT_CANVAS_SIZE_HEIGHT = 2400

# ------------------------------------------------------------------------------------------------
# Patchbay widget

class CarlaPatchbayW(QGraphicsView):
    def __init__(self, parent, doSetup = True):
        QGraphicsView.__init__(self, parent)

        # -------------------------------------------------------------
        # Internal stuff

        self.fParent      = parent
        self.fPluginCount = 0
        self.fPluginList  = []

        self.fCanvasWidth  = 0
        self.fCanvasHeight = 0

        # -------------------------------------------------------------
        # Set-up Canvas Preview

        self.fMiniCanvasPreview = self.fParent.ui.miniCanvasPreview
        self.fMiniCanvasPreview.setRealParent(self)
        self.fMovingViaMiniCanvas = False

        # -------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self) # FIXME?
        self.setScene(self.scene)
        self.setRenderHint(QPainter.Antialiasing, bool(parent.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING] == patchcanvas.ANTIALIASING_FULL))

        if parent.fSavedSettings[CARLA_KEY_CANVAS_USE_OPENGL] and hasGL:
            self.setViewport(QGLWidget(self))
            self.setRenderHint(QPainter.HighQualityAntialiasing, parent.fSavedSettings[CARLA_KEY_CANVAS_HQ_ANTIALIASING])

        self.setupCanvas()

        QTimer.singleShot(100, self.slot_restoreScrollbarValues)

        # -------------------------------------------------------------
        # Connect actions to functions

        self.horizontalScrollBar().valueChanged.connect(self.slot_horizontalScrollBarChanged)
        self.verticalScrollBar().valueChanged.connect(self.slot_verticalScrollBarChanged)

        self.scene.scaleChanged.connect(self.slot_canvasScaleChanged)
        self.scene.sceneGroupMoved.connect(self.slot_canvasItemMoved)

        self.fMiniCanvasPreview.miniCanvasMoved.connect(self.slot_miniCanvasMoved)

        if not doSetup: return

        parent.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        parent.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        parent.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        parent.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        parent.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        parent.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        parent.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        parent.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)

        parent.ui.act_canvas_arrange.setEnabled(False) # TODO, later
        parent.ui.act_canvas_arrange.triggered.connect(self.slot_canvasArrange)
        parent.ui.act_canvas_refresh.triggered.connect(self.slot_canvasRefresh)
        parent.ui.act_canvas_zoom_fit.triggered.connect(self.slot_canvasZoomFit)
        parent.ui.act_canvas_zoom_in.triggered.connect(self.slot_canvasZoomIn)
        parent.ui.act_canvas_zoom_out.triggered.connect(self.slot_canvasZoomOut)
        parent.ui.act_canvas_zoom_100.triggered.connect(self.slot_canvasZoomReset)
        parent.ui.act_canvas_print.triggered.connect(self.slot_canvasPrint)
        parent.ui.act_canvas_save_image.triggered.connect(self.slot_canvasSaveImage)

        parent.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

        parent.ParameterValueChangedCallback.connect(self.slot_handleParameterValueChangedCallback)
        parent.ParameterDefaultChangedCallback.connect(self.slot_handleParameterDefaultChangedCallback)
        parent.ParameterMidiChannelChangedCallback.connect(self.slot_handleParameterMidiChannelChangedCallback)
        parent.ParameterMidiCcChangedCallback.connect(self.slot_handleParameterMidiCcChangedCallback)
        parent.ProgramChangedCallback.connect(self.slot_handleProgramChangedCallback)
        parent.MidiProgramChangedCallback.connect(self.slot_handleMidiProgramChangedCallback)
        parent.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        parent.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        parent.UpdateCallback.connect(self.slot_handleUpdateCallback)
        parent.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        parent.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        parent.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        parent.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)
        parent.PatchbayClientAddedCallback.connect(self.slot_handlePatchbayClientAddedCallback)
        parent.PatchbayClientRemovedCallback.connect(self.slot_handlePatchbayClientRemovedCallback)
        parent.PatchbayClientRenamedCallback.connect(self.slot_handlePatchbayClientRenamedCallback)
        parent.PatchbayClientDataChangedCallback.connect(self.slot_handlePatchbayClientDataChangedCallback)
        parent.PatchbayPortAddedCallback.connect(self.slot_handlePatchbayPortAddedCallback)
        parent.PatchbayPortRemovedCallback.connect(self.slot_handlePatchbayPortRemovedCallback)
        parent.PatchbayPortRenamedCallback.connect(self.slot_handlePatchbayPortRenamedCallback)
        parent.PatchbayConnectionAddedCallback.connect(self.slot_handlePatchbayConnectionAddedCallback)
        parent.PatchbayConnectionRemovedCallback.connect(self.slot_handlePatchbayConnectionRemovedCallback)

    # -----------------------------------------------------------------

    def getPluginCount(self):
        return self.fPluginCount

    # -----------------------------------------------------------------

    def addPlugin(self, pluginId, isProjectLoading):
        pitem = PluginEdit(self, pluginId)

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        if not isProjectLoading:
            Carla.host.set_active(pluginId, True)

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
            pitem = self.fPluginList[i]
            pitem.setId(i)

    def renamePlugin(self, pluginId, newName):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setName(newName)

    def disablePlugin(self, pluginId, errorMsg):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

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

    def engineStarted(self):
        pass

    def engineStopped(self):
        patchcanvas.clear()

    def engineChanged(self):
        pass

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

    def saveSettings(self, settings):
        settings.setValue("HorizontalScrollBarValue", self.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.verticalScrollBar().value())

    # -----------------------------------------------------------------
    # called by PluginEdit, ignored here

    def recheckPluginHints(self, hints):
        pass

    # -----------------------------------------------------------------

    def setupCanvas(self):
        pOptions = patchcanvas.options_t()
        pOptions.theme_name       = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_THEME]
        pOptions.auto_hide_groups = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS]
        pOptions.use_bezier_lines = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_USE_BEZIER_LINES]
        pOptions.antialiasing     = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING]
        pOptions.eyecandy         = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_EYE_CANDY]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = False
        pFeatures.port_rename  = False
        pFeatures.handle_group_pos = True

        patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Carla2", self.scene, canvasCallback, False)

        tryCanvasSize = self.fParent.fSavedSettings[CARLA_KEY_CANVAS_SIZE].split("x")

        if len(tryCanvasSize) == 2 and tryCanvasSize[0].isdigit() and tryCanvasSize[1].isdigit():
            self.fCanvasWidth  = int(tryCanvasSize[0])
            self.fCanvasHeight = int(tryCanvasSize[1])
        else:
            self.fCanvasWidth  = CARLA_DEFAULT_CANVAS_SIZE_WIDTH
            self.fCanvasHeight = CARLA_DEFAULT_CANVAS_SIZE_HEIGHT

        patchcanvas.setCanvasSize(0, 0, self.fCanvasWidth, self.fCanvasHeight)
        patchcanvas.setInitialPos(self.fCanvasWidth / 2, self.fCanvasHeight / 2)
        self.setSceneRect(0, 0, self.fCanvasWidth, self.fCanvasHeight)

        self.themeData = [self.fCanvasWidth, self.fCanvasHeight, patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color()]

    def updateCanvasInitialPos(self):
        x = self.horizontalScrollBar().value() + self.width()/4
        y = self.verticalScrollBar().value() + self.height()/4
        patchcanvas.setInitialPos(x, y)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.fMiniCanvasPreview.setViewSize(float(self.width()) / self.fCanvasWidth, float(self.height()) / self.fCanvasHeight)

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        if self.fMovingViaMiniCanvas: return

        maximum = self.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.fMiniCanvasPreview.setViewPosX(xp)
        self.updateCanvasInitialPos()

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        if self.fMovingViaMiniCanvas: return

        maximum = self.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.fMiniCanvasPreview.setViewPosY(yp)
        self.updateCanvasInitialPos()

    @pyqtSlot()
    def slot_restoreScrollbarValues(self):
        settings = QSettings()
        self.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", self.horizontalScrollBar().maximum()/2, type=int))
        self.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", self.verticalScrollBar().maximum()/2, type=int))

    # -----------------------------------------------------------------

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.fMiniCanvasPreview.setViewScale(scale)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        self.fMiniCanvasPreview.update()

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.fMovingViaMiniCanvas = True
        self.horizontalScrollBar().setValue(xp * self.horizontalScrollBar().maximum())
        self.verticalScrollBar().setValue(yp * self.verticalScrollBar().maximum())
        self.fMovingViaMiniCanvas = False
        self.updateCanvasInitialPos()

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            Carla.host.set_active(i, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            Carla.host.set_active(i, False)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            if pitem.getHints() & PLUGIN_CAN_VOLUME:
                pitem.setParameterValue(PARAMETER_VOLUME, 1.0)
                Carla.host.set_volume(i, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            if pitem.getHints() & PLUGIN_CAN_VOLUME:
                pitem.setParameterValue(PARAMETER_VOLUME, 0.0)
                Carla.host.set_volume(i, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            if pitem.getHints() & PLUGIN_CAN_DRYWET:
                pitem.setParameterValue(PARAMETER_DRYWET, 1.0)
                Carla.host.set_drywet(i, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            if pitem.getHints() & PLUGIN_CAN_DRYWET:
                pitem.setParameterValue(PARAMETER_DRYWET, 0.0)
                Carla.host.set_drywet(i, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not Carla.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pitem = self.fPluginList[i]
            if pitem is None:
                break

            if pitem.getHints() & PLUGIN_CAN_BALANCE:
                pitem.setParameterValue(PARAMETER_BALANCE_LEFT, -1.0)
                pitem.setParameterValue(PARAMETER_BALANCE_RIGHT, 1.0)
                Carla.host.set_balance_left(i, -1.0)
                Carla.host.set_balance_right(i, 1.0)

            if pitem.getHints() & PLUGIN_CAN_PANNING:
                pitem.setParameterValue(PARAMETER_PANNING, 0.0)
                Carla.host.set_panning(i, 0.0)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_configureCarla(self):
        if self.fParent is None or not self.fParent.openSettingsWindow(True, hasGL):
            return

        self.fParent.loadSettings(False)

        patchcanvas.clear()

        self.setupCanvas()
        self.fParent.updateContainer(self.themeData)
        self.slot_miniCanvasCheckAll()

        if Carla.host.is_engine_running():
            Carla.host.patchbay_refresh()

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterValue(index, value)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, index, value):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterDefault(index, value)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, index, cc):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterMidiControl(index, cc)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, index, channel):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setParameterMidiChannel(index, channel)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setProgram(index)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, index):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.setMidiProgram(index)

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.sendNoteOn(channel, note)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.sendNoteOff(channel, note)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.reloadAll()

    # -----------------------------------------------------------------

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayClientAddedCallback(self, clientId, clientIcon, pluginId, clientName):
        pcSplit = patchcanvas.SPLIT_UNDEF
        pcIcon  = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN
        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_CARLA:
            pass
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE

        patchcanvas.addGroup(clientId, clientName, pcSplit, pcIcon)

        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

        if pluginId < 0:
            return
        if pluginId >= self.getPluginCount():
            print("sorry, can't map this plugin to canvas client", pluginId, self.getPluginCount())
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        patchcanvas.setGroupAsPlugin(clientId, pluginId, bool(pitem.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI))

    @pyqtSlot(int)
    def slot_handlePatchbayClientRemovedCallback(self, clientId):
        #if not self.fEngineStarted: return
        patchcanvas.removeGroup(clientId)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayClientDataChangedCallback(self, clientId, clientIcon, pluginId):
        pcIcon = patchcanvas.ICON_APPLICATION

        if clientIcon == PATCHBAY_ICON_PLUGIN:
            pcIcon = patchcanvas.ICON_PLUGIN
        if clientIcon == PATCHBAY_ICON_HARDWARE:
            pcIcon = patchcanvas.ICON_HARDWARE
        elif clientIcon == PATCHBAY_ICON_CARLA:
            pass
        elif clientIcon == PATCHBAY_ICON_DISTRHO:
            pcIcon = patchcanvas.ICON_DISTRHO
        elif clientIcon == PATCHBAY_ICON_FILE:
            pcIcon = patchcanvas.ICON_FILE

        patchcanvas.setGroupIcon(clientId, pcIcon)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

        if pluginId < 0:
            return
        if pluginId >= self.getPluginCount():
            print("sorry, can't map this plugin to canvas client", pluginId, self.getPluginCount())
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        patchcanvas.setGroupAsPlugin(clientId, pluginId, bool(pitem.fPluginInfo['hints'] & PLUGIN_HAS_CUSTOM_UI))

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortAddedCallback(self, clientId, portId, portFlags, portName):
        if (portFlags & PATCHBAY_PORT_IS_INPUT):
            portMode = patchcanvas.PORT_MODE_INPUT
        else:
            portMode = patchcanvas.PORT_MODE_OUTPUT

        if (portFlags & PATCHBAY_PORT_TYPE_AUDIO):
            portType = patchcanvas.PORT_TYPE_AUDIO_JACK
        elif (portFlags & PATCHBAY_PORT_TYPE_CV):
            portType = patchcanvas.PORT_TYPE_AUDIO_JACK # TODO
        elif (portFlags & PATCHBAY_PORT_TYPE_MIDI):
            portType = patchcanvas.PORT_TYPE_MIDI_JACK
        else:
            portType = patchcanvas.PORT_TYPE_NULL

        patchcanvas.addPort(clientId, portId, portName, portMode, portType)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, int)
    def slot_handlePatchbayPortRemovedCallback(self, groupId, portId):
        #if not self.fEngineStarted: return
        patchcanvas.removePort(portId)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRenamedCallback(self, groupId, portId, newPortName):
        patchcanvas.renamePort(portId, newPortName)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, portOutId, portInId):
        patchcanvas.connectPorts(connectionId, portOutId, portInId)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId, portOutId, portInId):
        #if not self.fEngineStarted: return
        patchcanvas.disconnectPorts(connectionId)
        QTimer.singleShot(0, self.fMiniCanvasPreview.update)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_canvasArrange(self):
        patchcanvas.arrange()

    @pyqtSlot()
    def slot_canvasRefresh(self):
        patchcanvas.clear()
        if Carla.host.is_engine_running():
            Carla.host.patchbay_refresh()
        QTimer.singleShot(1000 if self.fParent.fSavedSettings[CARLA_KEY_CANVAS_EYE_CANDY] else 0, self.fMiniCanvasPreview.update)

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

            if newPath.lower().endswith((".jpg",)):
                imgFormat = "JPG"
            elif newPath.lower().endswith((".png",)):
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

    # -----------------------------------------------------------------

    def resizeEvent(self, event):
        QGraphicsView.resizeEvent(self, event)
        self.slot_miniCanvasCheckSize()

# ------------------------------------------------------------------------------------------------
# Canvas callback

def canvasCallback(action, value1, value2, valueStr):
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
            print("Connection failed:", Carla.host.get_last_error())

    elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
        connectionId = value1

        if not Carla.host.patchbay_disconnect(connectionId):
            print("Disconnect failed:", Carla.host.get_last_error())

    elif action == patchcanvas.ACTION_PLUGIN_REMOVE:
        pluginId = value1

        Carla.host.remove_plugin(pluginId)
