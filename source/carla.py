#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
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
# For a full copy of the GNU General Public License see the GPL.txt file

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from time import sleep
from PyQt5.QtCore import Qt, QModelIndex, QPointF, QSize
from PyQt5.QtGui import QImage, QPalette, QResizeEvent, QSyntaxHighlighter
from PyQt5.QtPrintSupport import QPrinter, QPrintDialog
from PyQt5.QtWidgets import QApplication, QDialogButtonBox, QFileSystemModel, QLabel, QMainWindow

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_carla

from carla_shared import *

# ------------------------------------------------------------------------------------------------------------
# Global Variables

libPrefix = None
projectFilename = None

# ------------------------------------------------------------------------------------------------------------
# Log Syntax Highlighter

class LogSyntaxHighlighter(QSyntaxHighlighter):
    def __init__(self, parent):
        QSyntaxHighlighter.__init__(self, parent)

        palette = parent.palette()

        self.fColorDebug = palette.color(QPalette.Disabled, QPalette.WindowText)
        self.fColorError = Qt.red

        # -------------------------------------------------------------

    def highlightBlock(self, text):
        if text.startswith("DEBUG:"):
            self.setFormat(0, len(text), self.fColorDebug)
        elif text.startswith("ERROR:"):
            self.setFormat(0, len(text), self.fColorError)

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CarlaMainW(QMainWindow):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.ui =  ui_carla.Ui_CarlaMainW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fFirstEngineInit = True

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.fInfoLabel = QLabel(self)
        self.fInfoLabel.setText("")
        self.fInfoText = ""

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setNameFilters(cString(Carla.host.get_supported_file_types()).split(";"))
        self.fDirModel.setRootPath(HOME)

        if not WINDOWS:
            self.fSyntaxLog = LogSyntaxHighlighter(self.ui.pte_log)
            self.fSyntaxLog.setDocument(self.ui.pte_log.document())

            if LADISH_APP_NAME:
                self.ui.miniCanvasPreview.setVisible(False)
                self.ui.tabMain.removeTab(1)
        else:
            self.ui.tabMain.removeTab(2)

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        self.ui.act_engine_start.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        # FIXME: Qt4 needs this so it properly creates & resizes the canvas
        self.ui.tabMain.setCurrentIndex(1)
        self.ui.tabMain.setCurrentIndex(0)

        # -------------------------------------------------------------
        # Set-up Canvas Preview

        self.ui.miniCanvasPreview.setRealParent(self)
        self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.ui.miniCanvasPreview.init(self.scene, DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT, self.fSavedSettings["UseCustomMiniCanvasPaint"])
        QTimer.singleShot(100, self, SLOT("slot_miniCanvasInit()"))

    def updateInfoLabelPos(self):
        tabBar = self.ui.tabMain.tabBar()
        y = tabBar.mapFromParent(self.ui.centralwidget.pos()).y()+tabBar.height()/4

        if not self.ui.toolBar.isVisible():
            y -= self.ui.toolBar.height()

        self.fInfoLabel.move(self.fInfoLabel.x(), y)

    def updateInfoLabelSize(self):
        tabBar = self.ui.tabMain.tabBar()
        self.fInfoLabel.resize(self.ui.tabMain.width()-tabBar.width()-20, self.fInfoLabel.height())

    @pyqtSlot()
    def slot_fileExportLv2Preset(self):
        fileFilter  = self.tr("LV2 Preset (*.lv2)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if not filenameTry:
            return

        if not filenameTry.endswith(".lv2"):
            filenameTry += ".lv2"

        if os.path.exists(filenameTry) and not os.path.isdir(filenameTry):
            # TODO - error
            return

        # Save current project to a tmp file, and read it
        tmpFile = os.path.join(TMP, "carla-plugin-export.carxp")

        if not Carla.host.save_project(tmpFile):
            # TODO - error
            return

        tmpFileFd = open(tmpFile, "r")
        presetContents = tmpFileFd.read()
        tmpFileFd.close()
        os.remove(tmpFile)

        # Create LV2 Preset
        os.mkdir(filenameTry)

        manifestPath = os.path.join(filenameTry, "manifest.ttl")

        manifestFd = open(manifestPath, "w")
        manifestFd.write("""# LV2 Preset for the Carla LV2 Plugin
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .
@prefix pset:  <http://lv2plug.in/ns/ext/presets#> .
@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
@prefix state: <http://lv2plug.in/ns/ext/state#> .

<file://%s>
    a pset:Preset ;
    lv2:appliesTo <http://kxstudio.sf.net/carla> ;
    rdfs:label "%s" ;
    state:state [
        <http://kxstudio.sf.net/ns/carla/string>
\"\"\"
%s
\"\"\"
    ] .
""" % (manifestPath, os.path.basename(filenameTry), presetContents))
        manifestFd.close()

    @pyqtSlot()
    def slot_toolbarShown(self):
        self.updateInfoLabelPos()

    @pyqtSlot()
    def slot_splitterMoved(self):
        self.updateInfoLabelSize()

    @pyqtSlot(int)
    def slot_diskFolderChanged(self, index):
        if index < 0:
            return
        elif index == 0:
            filename = HOME
            self.ui.b_disk_remove.setEnabled(False)
        else:
            filename = self.ui.cb_disk.itemData(index)
            self.ui.b_disk_remove.setEnabled(True)

        self.fDirModel.setRootPath(filename)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(filename))

    @pyqtSlot()
    def slot_diskFolderAdd(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("New Folder"), "", QFileDialog.ShowDirsOnly)

        if newPath:
            if newPath[-1] == os.sep:
                newPath = newPath[:-1]
            self.ui.cb_disk.addItem(os.path.basename(newPath), newPath)
            self.ui.cb_disk.setCurrentIndex(self.ui.cb_disk.count()-1)
            self.ui.b_disk_remove.setEnabled(True)

    @pyqtSlot()
    def slot_diskFolderRemove(self):
        index = self.ui.cb_disk.currentIndex()

        if index <= 0:
            return

        self.ui.cb_disk.removeItem(index)

        if self.ui.cb_disk.currentIndex() == 0:
            self.ui.b_disk_remove.setEnabled(False)

    @pyqtSlot(QModelIndex)
    def slot_fileTreeDoubleClicked(self, modelIndex):
        filename = self.fDirModel.filePath(modelIndex)

        if not Carla.host.load_filename(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        self.ui.graphicsView.horizontalScrollBar().setValue(xp * DEFAULT_CANVAS_WIDTH)
        self.ui.graphicsView.verticalScrollBar().setValue(yp * DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosX(xp)

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosY(yp)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        self.ui.miniCanvasPreview.update()

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.ui.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot()
    def slot_miniCanvasInit(self):
        settings = QSettings()
        self.ui.graphicsView.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", DEFAULT_CANVAS_WIDTH / 3, type=int))
        self.ui.graphicsView.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", DEFAULT_CANVAS_HEIGHT * 3 / 8, type=int))

        tabBar = self.ui.tabMain.tabBar()
        x = tabBar.width()+20
        y = tabBar.mapFromParent(self.ui.centralwidget.pos()).y()+tabBar.height()/4
        self.fInfoLabel.move(x, y)
        self.fInfoLabel.resize(self.ui.tabMain.width()-x, tabBar.height())

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.ui.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.ui.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        self.ui.miniCanvasPreview.setViewSize(float(self.ui.graphicsView.width()) / DEFAULT_CANVAS_WIDTH, float(self.ui.graphicsView.height()) / DEFAULT_CANVAS_HEIGHT)

    @pyqtSlot(str)
    def slot_handleNSM_AnnounceCallback(self, smName):
        self.fSessionManagerName = smName
        self.ui.act_file_new.setEnabled(False)
        self.ui.act_file_open.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_start.setEnabled(True)
        self.ui.act_engine_stop.setEnabled(False)

    @pyqtSlot(str)
    def slot_handleNSM_OpenCallback(self, data):
        projectPath, clientId = data.rsplit(":", 1)
        self.fClientName = clientId

        # restart engine
        if self.fEngineStarted:
            self.stopEngine()

        self.slot_engineStart()

        if self.fEngineStarted:
            self.loadProject(projectPath)

        Carla.host.nsm_reply_open()

    @pyqtSlot()
    def slot_handleNSM_SaveCallback(self):
        self.saveProject(self.fProjectFilename)
        Carla.host.nsm_reply_save()

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, self.tr("Error"), error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("Engine has been stopped or crashed.\nPlease restart Carla"),
            self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    def dragEnterEvent(self, event):
        if event.source() == self.ui.fileTreeView:
            event.accept()
        elif self.ui.tabMain.contentsRect().contains(event.pos()):
            event.accept()
        else:
            QMainWindow.dragEnterEvent(self, event)

    def dropEvent(self, event):
        event.accept()

        urls = event.mimeData().urls()

        for url in urls:
            filename = url.toLocalFile()

            if not Carla.host.load_filename(filename):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                                 self.tr("Failed to load file"),
                                 cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

    def resizeEvent(self, event):
        if self.ui.tabMain.currentIndex() == 0:
            # Force update of 2nd tab
            width  = self.ui.tab_plugins.width()-4
            height = self.ui.tab_plugins.height()-4
            self.ui.miniCanvasPreview.setViewSize(float(width) / DEFAULT_CANVAS_WIDTH, float(height) / DEFAULT_CANVAS_HEIGHT)
        else:
            QTimer.singleShot(0, self, SLOT("slot_miniCanvasCheckSize()"))

        self.updateInfoLabelSize()

        QMainWindow.resizeEvent(self, event)
