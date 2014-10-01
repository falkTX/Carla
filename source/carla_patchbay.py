#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla patchbay widget code
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
    from PyQt5.QtCore import QPointF, QTimer
    from PyQt5.QtGui import QImage
    from PyQt5.QtPrintSupport import QPrinter, QPrintDialog
    from PyQt5.QtWidgets import QFrame, QGraphicsView, QGridLayout
else:
    from PyQt4.QtCore import QPointF, QTimer
    from PyQt4.QtGui import QFrame, QGraphicsView, QGridLayout, QImage, QPrinter, QPrintDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import patchcanvas

from carla_host import *
from digitalpeakmeter import DigitalPeakMeter
from pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------
# Dummy class used in main carla as replacement for PluginEdit

class DummyPluginEdit(object):
    def __init__(self, parent, pluginId):
        object.__init__(self)

        self.fPluginId = pluginId

    #------------------------------------------------------------------

    def clearNotes(self):
        pass

    def noteOn(self, channel, note, velocity):
        pass

    def noteOff(self, channel, note):
        pass

    #------------------------------------------------------------------

    def setPluginId(self, idx):
        self.fPluginId = idx

    #------------------------------------------------------------------

    def close(self):
        pass

    def setName(self, name):
        pass

    def setParameterValue(self, parameterId, value):
        pass

    def reloadAll(self):
        pass

    def idleSlow(self):
        pass

# ------------------------------------------------------------------------------------------------
# Patchbay widget

class CarlaPatchbayW(QFrame, PluginEditParentMeta, HostWidgetMeta):
#class CarlaPatchbayW(QFrame, PluginEditParentMeta, HostWidgetMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, doSetup = True, onlyPatchbay = True, is3D = False):
        QFrame.__init__(self, parent)
        self.host = host

        # -------------------------------------------------------------
        # Connect actions to functions

        #self.fKeys.keyboard.noteOn.connect(self.slot_noteOn)
        #self.fKeys.keyboard.noteOff.connect(self.slot_noteOff)

        # -------------------------------------------------------------
        # Connect actions to functions (part 2)

        #host.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        #host.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        #host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        #host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)

        #if not doSetup: return

        #parent.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        #parent.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        #parent.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        #parent.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        #parent.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        #parent.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        #parent.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        #parent.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)

        #parent.ui.act_canvas_show_internal.triggered.connect(self.slot_canvasShowInternal)
        #parent.ui.act_canvas_show_external.triggered.connect(self.slot_canvasShowExternal)
        #parent.ui.act_canvas_arrange.setEnabled(False) # TODO, later
        #parent.ui.act_canvas_arrange.triggered.connect(self.slot_canvasArrange)
        #parent.ui.act_canvas_refresh.triggered.connect(self.slot_canvasRefresh)
        #parent.ui.act_canvas_zoom_fit.triggered.connect(self.slot_canvasZoomFit)
        #parent.ui.act_canvas_zoom_in.triggered.connect(self.slot_canvasZoomIn)
        #parent.ui.act_canvas_zoom_out.triggered.connect(self.slot_canvasZoomOut)
        #parent.ui.act_canvas_zoom_100.triggered.connect(self.slot_canvasZoomReset)
        #parent.ui.act_canvas_print.triggered.connect(self.slot_canvasPrint)
        #parent.ui.act_canvas_save_image.triggered.connect(self.slot_canvasSaveImage)

        #parent.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

    # -----------------------------------------------------------------

    def getPluginEditDialog(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pedit = self.fPluginList[pluginId]
        if pedit is None:
            return None
        if False:
            pedit = DummyPluginEdit(self, 0)

        return pedit

    # -----------------------------------------------------------------

    @pyqtSlot(int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName):
        if self.fIsOnlyPatchbay:
            pedit = PluginEdit(self, self.host, pluginId)
        else:
            pedit = DummyPluginEdit(self, pluginId)

        self.fPluginList.append(pedit)
        self.fPluginCount += 1

        if self.fIsOnlyPatchbay and not self.fParent.isProjectLoading():
            self.host.set_active(pluginId, True)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        patchcanvas.handlePluginRemoved(pluginId)

        if pluginId in self.fSelectedPlugins:
            self.clearSideStuff()

        pedit = self.getPluginEditDialog(pluginId)

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)

        if pedit is not None:
            pedit.close()
            del pedit

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pedit = self.fPluginList[i]
            pedit.setPluginId(i)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velocity):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOff(note, False)

    # -----------------------------------------------------------------
    # HostWidgetMeta methods

    def removeAllPlugins(self):
        #patchcanvas.handleAllPluginsRemoved()

        for pedit in self.fPluginList:
            if pedit is None:
                break
            pedit.close()
            del pedit

        self.fPluginCount = 0
        self.fPluginList  = []

        self.clearSideStuff()

    def engineStarted(self):
        pass

    def engineStopped(self):
        pass
        #patchcanvas.clear()

    def projectLoadingStarted(self):
        pass
        #self.fView.setEnabled(False)

    def projectLoadingFinished(self):
        pass
        #self.fView.setEnabled(True)
        #QTimer.singleShot(1000, self.slot_canvasRefresh)

    def saveSettings(self, settings):
        pass

    def showEditDialog(self, pluginId):
        pedit = self.getPluginEditDialog(pluginId)

        if pedit:
            pedit.show()

    # -----------------------------------------------------------------
    # PluginEdit callbacks

    def editDialogVisibilityChanged(self, pluginId, visible):
        pass

    def editDialogPluginHintsChanged(self, pluginId, hints):
        pass

    def editDialogParameterValueChanged(self, pluginId, parameterId, value):
        pass

    def editDialogProgramChanged(self, pluginId, index):
        pass

    def editDialogMidiProgramChanged(self, pluginId, index):
        pass

    def editDialogNotePressed(self, pluginId, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOn(note, False)

    def editDialogNoteReleased(self, pluginId, note):
        if pluginId in self.fSelectedPlugins:
            self.fKeys.keyboard.sendNoteOff(note, False)

    def editDialogMidiActivityChanged(self, pluginId, onOff):
        pass

    # -----------------------------------------------------------------

    def clearSideStuff(self):
        #self.scene.clearSelection()

        self.fSelectedPlugins = []

        #self.fKeys.keyboard.allNotesOff(False)
        #self.fKeys.setEnabled(False)

        #self.fPeaksCleared = True
        #self.fPeaksIn.displayMeter(1, 0.0, True)
        #self.fPeaksIn.displayMeter(2, 0.0, True)
        #self.fPeaksOut.displayMeter(1, 0.0, True)
        #self.fPeaksOut.displayMeter(2, 0.0, True)

    # -----------------------------------------------------------------

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        for pluginId in self.fSelectedPlugins:
            self.host.send_midi_note(pluginId, 0, note, 100)

            pedit = self.getPluginEditDialog(pluginId)
            pedit.noteOn(0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        for pluginId in self.fSelectedPlugins:
            self.host.send_midi_note(pluginId, 0, note, 0)

            pedit = self.getPluginEditDialog(pluginId)
            pedit.noteOff(0, note)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_pluginsEnable(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            self.host.set_active(i, True)

    @pyqtSlot()
    def slot_pluginsDisable(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            self.host.set_active(i, False)

    @pyqtSlot()
    def slot_pluginsVolume100(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pedit = self.fPluginList[i]
            if pedit is None:
                break

            if pedit.getHints() & PLUGIN_CAN_VOLUME:
                pedit.setParameterValue(PARAMETER_VOLUME, 1.0)
                self.host.set_volume(i, 1.0)

    @pyqtSlot()
    def slot_pluginsMute(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pedit = self.fPluginList[i]
            if pedit is None:
                break

            if pedit.getHints() & PLUGIN_CAN_VOLUME:
                pedit.setParameterValue(PARAMETER_VOLUME, 0.0)
                self.host.set_volume(i, 0.0)

    @pyqtSlot()
    def slot_pluginsWet100(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pedit = self.fPluginList[i]
            if pedit is None:
                break

            if pedit.getHints() & PLUGIN_CAN_DRYWET:
                pedit.setParameterValue(PARAMETER_DRYWET, 1.0)
                self.host.set_drywet(i, 1.0)

    @pyqtSlot()
    def slot_pluginsBypass(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pedit = self.fPluginList[i]
            if pedit is None:
                break

            if pedit.getHints() & PLUGIN_CAN_DRYWET:
                pedit.setParameterValue(PARAMETER_DRYWET, 0.0)
                self.host.set_drywet(i, 0.0)

    @pyqtSlot()
    def slot_pluginsCenter(self):
        if not self.host.is_engine_running():
            return

        for i in range(self.fPluginCount):
            pedit = self.fPluginList[i]
            if pedit is None:
                break

            if pedit.getHints() & PLUGIN_CAN_BALANCE:
                pedit.setParameterValue(PARAMETER_BALANCE_LEFT, -1.0)
                pedit.setParameterValue(PARAMETER_BALANCE_RIGHT, 1.0)
                self.host.set_balance_left(i, -1.0)
                self.host.set_balance_right(i, 1.0)

            if pedit.getHints() & PLUGIN_CAN_PANNING:
                pedit.setParameterValue(PARAMETER_PANNING, 0.0)
                self.host.set_panning(i, 0.0)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self, self.host, True, hasGL)
        if not dialog.exec_():
            return

        self.fParent.loadSettings(False)

        patchcanvas.clear()

        self.setupCanvas()
        self.fParent.updateContainer(self.themeData)
        #self.slot_miniCanvasCheckAll()

        if self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

    # -----------------------------------------------------------------

    @pyqtSlot()
    def slot_canvasArrange(self):
        patchcanvas.arrange()

    @pyqtSlot()
    def slot_canvasShowInternal(self):
        self.fExternalPatchbay = False
        self.fParent.ui.act_canvas_show_internal.blockSignals(True)
        self.fParent.ui.act_canvas_show_external.blockSignals(True)
        self.fParent.ui.act_canvas_show_internal.setChecked(True)
        self.fParent.ui.act_canvas_show_external.setChecked(False)
        self.fParent.ui.act_canvas_show_internal.blockSignals(False)
        self.fParent.ui.act_canvas_show_external.blockSignals(False)
        self.slot_canvasRefresh()

    @pyqtSlot()
    def slot_canvasShowExternal(self):
        self.fExternalPatchbay = True
        self.fParent.ui.act_canvas_show_internal.blockSignals(True)
        self.fParent.ui.act_canvas_show_external.blockSignals(True)
        self.fParent.ui.act_canvas_show_internal.setChecked(False)
        self.fParent.ui.act_canvas_show_external.setChecked(True)
        self.fParent.ui.act_canvas_show_internal.blockSignals(False)
        self.fParent.ui.act_canvas_show_external.blockSignals(False)
        self.slot_canvasRefresh()

    @pyqtSlot()
    def slot_canvasRefresh(self):
        #patchcanvas.clear()

        if self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

            for pedit in self.fPluginList:
                if pedit is None:
                    break
                pedit.reloadAll()

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

        if config_UseQt5:
            newPath = newPath[0]
        if not newPath:
            return

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
