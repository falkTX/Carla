#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla host code
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
    from PyQt5.QtCore import qCritical, QFileInfo, QModelIndex, QPointF, QTimer
    from PyQt5.QtGui import QImage, QPalette
    from PyQt5.QtPrintSupport import QPrinter, QPrintDialog
    from PyQt5.QtWidgets import QAction, QApplication, QFileSystemModel, QListWidgetItem, QMainWindow
else:
    from PyQt4.QtCore import qCritical, QFileInfo, QModelIndex, QPointF, QTimer
    from PyQt4.QtGui import QAction, QApplication, QFileSystemModel, QImage, QListWidgetItem, QMainWindow, QPalette, QPrinter, QPrintDialog

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import patchcanvas
import ui_carla_host

from carla_app import *
from carla_database import *
from carla_panels import *
from carla_settings import *
from carla_utils import *
from carla_widgets import *

from digitalpeakmeter import DigitalPeakMeter
from pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    if config_UseQt5:
        from PyQt5.QtOpenGL import QGLWidget
    else:
        from PyQt4.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Session Management support

CARLA_CLIENT_NAME = os.getenv("CARLA_CLIENT_NAME")
LADISH_APP_NAME   = os.getenv("LADISH_APP_NAME")
NSM_URL           = os.getenv("NSM_URL")

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
#class HostWindow(QMainWindow, PluginEditParentMeta, metaclass=PyQtMetaClass):
    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    # --------------------------------------------------------------------------------------------------------

    def __init__(self, host, withCanvas, parent=None):
        QMainWindow.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_host.Ui_CarlaHostW()
        self.ui.setupUi(self)
        gCarla.gui = self

        if False:
            # kdevelop likes this :)
            host = CarlaHostNull()
            self.host = host

        self._true = c_char_p("true".encode("utf-8"))

        self.fParentOrSelf = parent or self

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fPluginCount = 0
        self.fPluginList  = []

        self.fProjectFilename  = ""
        self.fIsProjectLoading = False
        self.fCurrentlyRemovingAllPlugins = False

        # first attempt of auto-start engine doesn't show an error
        self.fFirstEngineInit = True

        # to be filled with key-value pairs of current settings
        self.fSavedSettings = {}

        if host.isControl:
            self.fClientName         = "Carla-Control"
            self.fSessionManagerName = "Control"
        elif host.isPlugin:
            self.fClientName         = "Carla-Plugin"
            self.fSessionManagerName = "Plugin"
        elif LADISH_APP_NAME:
            self.fClientName         = LADISH_APP_NAME
            self.fSessionManagerName = "LADISH"
        elif NSM_URL and host.nsmOK:
            self.fClientName         = "Carla.tmp"
            self.fSessionManagerName = "Non Session Manager TMP"
        else:
            self.fClientName         = CARLA_CLIENT_NAME or "Carla"
            self.fSessionManagerName = ""

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff (patchbay)

        self.fExportImage   = QImage()
        self.fExportPrinter = None

        self.fPeaksCleared = True

        self.fExternalPatchbay = False
        self.fSelectedPlugins  = []

        self.fCanvasWidth  = 0
        self.fCanvasHeight = 0
        self.fMiniCanvasUpdateTimeout = 0

        self.fWithCanvas = withCanvas

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (engine stopped)

        if self.host.isPlugin or self.host.isControl:
            self.ui.act_file_save.setVisible(False)
            self.ui.act_engine_start.setEnabled(False)
            self.ui.act_engine_start.setVisible(False)
            self.ui.act_engine_stop.setEnabled(False)
            self.ui.act_engine_stop.setVisible(False)
            self.ui.act_settings_show_time_panel.setEnabled(False)
            self.ui.act_settings_show_time_panel.setVisible(False)
            self.ui.menu_Engine.setEnabled(False)
            self.ui.menu_Engine.setVisible(False)
            self.ui.menu_Engine.menuAction().setVisible(False)

            if self.host.isControl:
                self.ui.act_file_new.setVisible(False)
                self.ui.act_file_open.setVisible(False)
                self.ui.act_file_save.setVisible(False)
                self.ui.act_file_save_as.setVisible(False)
                self.ui.act_plugin_add.setVisible(False)
                self.ui.act_plugin_add2.setVisible(False)
                self.ui.act_plugin_remove_all.setVisible(False)
                self.ui.menu_Plugin.setEnabled(False)
                self.ui.menu_Plugin.setVisible(False)
                self.ui.menu_Plugin.menuAction().setVisible(False)
            else:
                self.ui.act_file_save_as.setText(self.tr("Export as..."))

        else:
            self.ui.act_engine_start.setEnabled(True)

        if not self.host.isControl:
            self.ui.act_file_connect.setEnabled(False)
            self.ui.act_file_connect.setVisible(False)
            self.ui.act_file_refresh.setEnabled(False)
            self.ui.act_file_refresh.setVisible(False)

        if self.fSessionManagerName and not self.host.isPlugin:
            self.ui.act_file_new.setEnabled(False)

        self.ui.act_file_open.setEnabled(False)
        self.ui.act_file_save.setEnabled(False)
        self.ui.act_file_save_as.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        self.ui.act_canvas_show_internal.setChecked(False)
        self.ui.act_canvas_show_internal.setVisible(False)
        self.ui.act_canvas_show_external.setChecked(False)
        self.ui.act_canvas_show_external.setVisible(False)

        self.ui.menu_PluginMacros.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)

        self.ui.dockWidgetTitleBar = QWidget(self)
        self.ui.dockWidget.setTitleBarWidget(self.ui.dockWidgetTitleBar)

        if not withCanvas:
            self.ui.act_canvas_show_internal.setVisible(False)
            self.ui.act_canvas_show_external.setVisible(False)
            self.ui.act_canvas_arrange.setVisible(False)
            self.ui.act_canvas_print.setVisible(False)
            self.ui.act_canvas_refresh.setVisible(False)
            self.ui.act_canvas_save_image.setVisible(False)
            self.ui.act_canvas_zoom_100.setVisible(False)
            self.ui.act_canvas_zoom_fit.setVisible(False)
            self.ui.act_canvas_zoom_in.setVisible(False)
            self.ui.act_canvas_zoom_out.setVisible(False)
            self.ui.act_settings_show_meters.setVisible(False)
            self.ui.act_settings_show_keyboard.setVisible(False)
            self.ui.menu_Canvas_Zoom.setEnabled(False)
            self.ui.menu_Canvas_Zoom.setVisible(False)
            self.ui.menu_Canvas_Zoom.menuAction().setVisible(False)
            self.ui.menu_Canvas.setEnabled(False)
            self.ui.menu_Canvas.setVisible(False)
            self.ui.menu_Canvas.menuAction().setVisible(False)
            self.ui.miniCanvasPreview.hide()
            self.ui.tabWidget.removeTab(1)
            self.ui.tabWidget.tabBar().hide()

            if host.isControl:
                self.ui.dockWidget.hide()

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (disk)

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setRootPath(HOME)
        self.fDirModel.setNameFilters(gCarla.utils.get_supported_file_extensions().split(";"))

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (panels)

        self.ui.panelTime = CarlaPanelTime(host, self)
        self.ui.panelTime.setEnabled(False)
        self.ui.panelTime.adjustSize()

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (rack)

        self.ui.listWidget.setHost(self.host)

        sb = self.ui.listWidget.verticalScrollBar()
        self.ui.rackScrollBar.setMinimum(sb.minimum())
        self.ui.rackScrollBar.setMaximum(sb.maximum())
        self.ui.rackScrollBar.setValue(sb.value())

        sb.rangeChanged.connect(self.ui.rackScrollBar.setRange)
        sb.valueChanged.connect(self.ui.rackScrollBar.setValue)
        self.ui.rackScrollBar.rangeChanged.connect(sb.setRange)
        self.ui.rackScrollBar.valueChanged.connect(sb.setValue)

        self.ui.rack.setStyleSheet("""
          QLabel#pad_left {
            background-image: url(:/bitmaps/rack_padding_left.png);
            background-repeat: repeat-y;
          }
          QLabel#pad_right {
            background-image: url(:/bitmaps/rack_padding_right.png);
            background-repeat: repeat-y;
          }
          CarlaRackList#CarlaRackList {
            background-color: black;
          }
        """)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (patchbay)

        self.ui.peak_in.setChannelCount(2)
        self.ui.peak_in.setMeterColor(DigitalPeakMeter.COLOR_BLUE)
        self.ui.peak_in.setMeterOrientation(DigitalPeakMeter.VERTICAL)
        self.ui.peak_in.setFixedWidth(25)

        self.ui.peak_out.setChannelCount(2)
        self.ui.peak_out.setMeterColor(DigitalPeakMeter.COLOR_GREEN)
        self.ui.peak_out.setMeterOrientation(DigitalPeakMeter.VERTICAL)
        self.ui.peak_out.setFixedWidth(25)

        self.ui.scrollArea = PixmapKeyboardHArea(self.ui.patchbay)
        self.ui.keyboard   = self.ui.scrollArea.keyboard
        self.ui.patchbay.layout().addWidget(self.ui.scrollArea, 1, 0, 1, 0)

        self.ui.scrollArea.setEnabled(False)

        self.ui.miniCanvasPreview.setRealParent(self)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (special stuff for Mac OS)

        if MACOS:
            self.ui.act_file_quit.setMenuRole(QAction.QuitRole)
            self.ui.act_settings_configure.setMenuRole(QAction.PreferencesRole)
            self.ui.act_help_about.setMenuRole(QAction.AboutRole)
            self.ui.act_help_about_juce.setMenuRole(QAction.AboutQtRole)
            self.ui.act_help_about_qt.setMenuRole(QAction.AboutQtRole)
            self.ui.menu_Settings.setTitle("Panels")
            #self.ui.menu_Help.hide()

        # ----------------------------------------------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)

        # ----------------------------------------------------------------------------------------------------
        # Set-up Canvas

        self.scene = patchcanvas.PatchScene(self, self.ui.graphicsView)
        self.ui.graphicsView.setScene(self.scene)
        self.ui.graphicsView.setRenderHint(QPainter.Antialiasing, bool(self.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING] == patchcanvas.ANTIALIASING_FULL))

        if self.fSavedSettings[CARLA_KEY_CANVAS_USE_OPENGL] and hasGL:
            self.ui.glView = QGLWidget(self)
            self.ui.graphicsView.setViewport(self.ui.glView)
            self.ui.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.fSavedSettings[CARLA_KEY_CANVAS_HQ_ANTIALIASING])

        self.setupCanvas()

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_new.triggered.connect(self.slot_fileNew)
        self.ui.act_file_open.triggered.connect(self.slot_fileOpen)
        self.ui.act_file_save.triggered.connect(self.slot_fileSave)
        self.ui.act_file_save_as.triggered.connect(self.slot_fileSaveAs)

        self.ui.act_engine_start.triggered.connect(self.slot_engineStart)
        self.ui.act_engine_stop.triggered.connect(self.slot_engineStop)

        self.ui.act_plugin_add.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_add2.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_remove_all.triggered.connect(self.slot_pluginRemoveAll)

        self.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        self.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        self.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        self.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        self.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        self.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        self.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        self.ui.act_plugins_compact.triggered.connect(self.slot_pluginsCompact)
        self.ui.act_plugins_expand.triggered.connect(self.slot_pluginsExpand)
        self.ui.act_plugins_panic.triggered.connect(self.slot_pluginsDisable)

        self.ui.act_canvas_show_internal.triggered.connect(self.slot_canvasShowInternal)
        self.ui.act_canvas_show_external.triggered.connect(self.slot_canvasShowExternal)
        self.ui.act_canvas_arrange.triggered.connect(self.slot_canvasArrange)
        self.ui.act_canvas_refresh.triggered.connect(self.slot_canvasRefresh)
        self.ui.act_canvas_zoom_fit.triggered.connect(self.slot_canvasZoomFit)
        self.ui.act_canvas_zoom_in.triggered.connect(self.slot_canvasZoomIn)
        self.ui.act_canvas_zoom_out.triggered.connect(self.slot_canvasZoomOut)
        self.ui.act_canvas_zoom_100.triggered.connect(self.slot_canvasZoomReset)
        self.ui.act_canvas_print.triggered.connect(self.slot_canvasPrint)
        self.ui.act_canvas_save_image.triggered.connect(self.slot_canvasSaveImage)
        self.ui.act_canvas_arrange.setEnabled(False) # TODO, later

        self.ui.act_settings_show_time_panel.toggled.connect(self.slot_showTimePanel)
        self.ui.act_settings_show_toolbar.toggled.connect(self.slot_showToolbar)
        self.ui.act_settings_show_meters.toggled.connect(self.slot_showCanvasMeters)
        self.ui.act_settings_show_keyboard.toggled.connect(self.slot_showCanvasKeyboard)
        self.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

        self.ui.act_help_about.triggered.connect(self.slot_aboutCarla)
        self.ui.act_help_about_juce.triggered.connect(self.slot_aboutJuce)
        self.ui.act_help_about_qt.triggered.connect(self.slot_aboutQt)

        self.ui.cb_disk.currentIndexChanged.connect(self.slot_diskFolderChanged)
        self.ui.b_disk_add.clicked.connect(self.slot_diskFolderAdd)
        self.ui.b_disk_remove.clicked.connect(self.slot_diskFolderRemove)
        self.ui.fileTreeView.doubleClicked.connect(self.slot_fileTreeDoubleClicked)

        self.ui.graphicsView.horizontalScrollBar().valueChanged.connect(self.slot_horizontalScrollBarChanged)
        self.ui.graphicsView.verticalScrollBar().valueChanged.connect(self.slot_verticalScrollBarChanged)

        self.ui.keyboard.noteOn.connect(self.slot_noteOn)
        self.ui.keyboard.noteOff.connect(self.slot_noteOff)

        self.ui.miniCanvasPreview.miniCanvasMoved.connect(self.slot_miniCanvasMoved)

        self.ui.panelTime.finished.connect(self.slot_timePanelClosed)
        self.ui.tabWidget.currentChanged.connect(self.slot_tabChanged)

        self.scene.scaleChanged.connect(self.slot_canvasScaleChanged)
        self.scene.sceneGroupMoved.connect(self.slot_canvasItemMoved)
        self.scene.pluginSelected.connect(self.slot_canvasPluginSelected)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

        host.EngineStartedCallback.connect(self.slot_handleEngineStartedCallback)
        host.EngineStoppedCallback.connect(self.slot_handleEngineStoppedCallback)

        host.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        host.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        host.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

        host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)

        host.UpdateCallback.connect(self.slot_handleUpdateCallback)

        host.PatchbayClientAddedCallback.connect(self.slot_handlePatchbayClientAddedCallback)
        host.PatchbayClientRemovedCallback.connect(self.slot_handlePatchbayClientRemovedCallback)
        host.PatchbayClientRenamedCallback.connect(self.slot_handlePatchbayClientRenamedCallback)
        host.PatchbayClientDataChangedCallback.connect(self.slot_handlePatchbayClientDataChangedCallback)
        host.PatchbayPortAddedCallback.connect(self.slot_handlePatchbayPortAddedCallback)
        host.PatchbayPortRemovedCallback.connect(self.slot_handlePatchbayPortRemovedCallback)
        host.PatchbayPortRenamedCallback.connect(self.slot_handlePatchbayPortRenamedCallback)
        host.PatchbayConnectionAddedCallback.connect(self.slot_handlePatchbayConnectionAddedCallback)
        host.PatchbayConnectionRemovedCallback.connect(self.slot_handlePatchbayConnectionRemovedCallback)

        host.NSMCallback.connect(self.slot_handleNSMCallback)

        host.DebugCallback.connect(self.slot_handleDebugCallback)
        host.InfoCallback.connect(self.slot_handleInfoCallback)
        host.ErrorCallback.connect(self.slot_handleErrorCallback)
        host.QuitCallback.connect(self.slot_handleQuitCallback)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        self.setProperWindowTitle()

        # Qt needs this so it properly creates & resizes the canvas
        self.ui.tabWidget.blockSignals(True)
        self.ui.tabWidget.setCurrentIndex(1)
        self.ui.tabWidget.setCurrentIndex(0)
        self.ui.tabWidget.blockSignals(False)

        # Plugin needs to have timers always running so it receives messages
        if self.host.isPlugin:
            self.startTimers()

        # Start in patchbay tab if using forced patchbay mode
        if host.processModeForced and host.processMode == ENGINE_PROCESS_MODE_PATCHBAY and not host.isControl:
            self.ui.tabWidget.setCurrentIndex(1)

        # Load initial project file if set
        if not (self.host.isControl or self.host.isPlugin):
            projectFile = getInitialProjectFile(QApplication.instance())

            if projectFile:
                self.loadProjectLater(projectFile)

        # For NSM we wait for the open message
        if NSM_URL and host.nsmOK:
            host.nsm_ready(-1)
            return

        QTimer.singleShot(0, self.slot_engineStart)

    # --------------------------------------------------------------------------------------------------------
    # Setup

    def compactPlugin(self, pluginId):
        if pluginId > self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]

        if pitem is None:
            return

        pitem.recreateWidget(True)

    def setLoadRDFsNeeded(self):
        self.fLadspaRdfNeedsUpdate = True

    def setProperWindowTitle(self):
        title = self.fClientName

        if self.fProjectFilename and not self.host.nsmOK:
            title += " - %s" % os.path.basename(self.fProjectFilename)
        if self.fSessionManagerName:
            title += " (%s)" % self.fSessionManagerName

        self.setWindowTitle(title)

    # --------------------------------------------------------------------------------------------------------
    # Files

    def loadProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: loading project without filename set")

        self.projectLoadingStarted()
        self.fIsProjectLoading = True

        self.host.load_project(self.fProjectFilename)

        self.fIsProjectLoading = False
        self.projectLoadingFinished()

    def loadProjectLater(self, filename):
        self.fProjectFilename = QFileInfo(filename).absoluteFilePath()
        self.setProperWindowTitle()
        QTimer.singleShot(1, self.slot_loadProjectNow)

    def saveProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: saving project without filename set")

        self.host.save_project(self.fProjectFilename)

    def projectLoadingStarted(self):
        self.ui.rack.setEnabled(False)
        self.ui.graphicsView.setEnabled(False)

    def projectLoadingFinished(self):
        self.ui.rack.setEnabled(True)
        self.ui.graphicsView.setEnabled(True)
        QTimer.singleShot(1000, self.slot_canvasRefresh)

    # --------------------------------------------------------------------------------------------------------
    # Files (menu actions)

    @pyqtSlot()
    def slot_fileNew(self):
        self.slot_pluginRemoveAll()
        self.fProjectFilename = ""
        self.setProperWindowTitle()

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter = self.tr("Carla Project File (*.carxp);;Carla Preset File (*.carxs)")
        filename   = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        newFile = True

        if self.fPluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Question"), self.tr("There are some plugins loaded, do you want to remove them now?"),
                                                                          QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            newFile = (ask == QMessageBox.Yes)

        if newFile:
            self.slot_pluginRemoveAll()
            self.fProjectFilename = filename
            self.setProperWindowTitle()
            self.loadProjectNow()
        else:
            filenameOld = self.fProjectFilename
            self.fProjectFilename = filename
            self.loadProjectNow()
            self.fProjectFilename = filenameOld

    @pyqtSlot()
    def slot_fileSave(self, saveAs=False):
        if self.fProjectFilename and not saveAs:
            return self.saveProjectNow()

        fileFilter = self.tr("Carla Project File (*.carxp)")
        filename   = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        if config_UseQt5:
            filename = filename[0]
        if not filename:
            return

        if not filename.lower().endswith(".carxp"):
            filename += ".carxp"

        if self.fProjectFilename != filename:
            self.fProjectFilename = filename
            self.setProperWindowTitle()

        self.saveProjectNow()

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    @pyqtSlot()
    def slot_loadProjectNow(self):
        self.loadProjectNow()

    # --------------------------------------------------------------------------------------------------------
    # Engine (menu actions)

    @pyqtSlot()
    def slot_engineStart(self):
        audioDriver = setEngineSettings(self.host)
        firstInit   = self.fFirstEngineInit

        self.fFirstEngineInit = False

        if self.host.engine_init(audioDriver, self.fClientName) or firstInit:
            return

        audioError = self.host.get_last_error()

        if audioError:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
        else:
            QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))

    @pyqtSlot()
    def slot_engineStop(self, forced = False):
        if self.fPluginCount > 0:
            if not forced:
                ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                            "Do you want to do this now?"),
                                                                            QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
                if ask != QMessageBox.Yes:
                    return

            self.killTimers()
            self.removeAllPlugins()
            self.host.set_engine_about_to_close()
            self.host.remove_all_plugins()

        if self.host.is_engine_running() and not self.host.engine_close():
            print(self.host.get_last_error())

    # --------------------------------------------------------------------------------------------------------
    # Engine (host callbacks)

    @pyqtSlot(str)
    def slot_handleEngineStartedCallback(self, processMode, transportMode, driverName):
        self.ui.menu_PluginMacros.setEnabled(True)
        self.ui.menu_Canvas.setEnabled(True)

        self.ui.act_canvas_show_internal.blockSignals(True)
        self.ui.act_canvas_show_external.blockSignals(True)

        if processMode == ENGINE_PROCESS_MODE_PATCHBAY and not (self.host.isControl or self.host.isPlugin):
            self.ui.act_canvas_show_internal.setChecked(True)
            self.ui.act_canvas_show_internal.setVisible(True)
            self.ui.act_canvas_show_external.setChecked(False)
            self.ui.act_canvas_show_external.setVisible(True)
        else:
            self.ui.act_canvas_show_internal.setChecked(False)
            self.ui.act_canvas_show_internal.setVisible(False)
            self.ui.act_canvas_show_external.setChecked(False)
            self.ui.act_canvas_show_external.setVisible(False)

        self.ui.act_canvas_show_internal.blockSignals(False)
        self.ui.act_canvas_show_external.blockSignals(False)

        if not (self.host.isControl or self.host.isPlugin):
            canSave = (self.fProjectFilename and os.path.exists(self.fProjectFilename)) or not self.fSessionManagerName
            self.ui.act_file_save.setEnabled(canSave)
            self.ui.act_engine_start.setEnabled(False)
            self.ui.act_engine_stop.setEnabled(True)
            self.ui.panelTime.setEnabled(True)

        if self.host.isPlugin or not self.fSessionManagerName:
            self.ui.act_file_open.setEnabled(True)
            self.ui.act_file_save_as.setEnabled(True)

        self.startTimers()

    @pyqtSlot()
    def slot_handleEngineStoppedCallback(self):
        patchcanvas.clear()
        self.killTimers()

        # just in case
        self.removeAllPlugins()

        self.ui.menu_PluginMacros.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)

        if not (self.host.isControl or self.host.isPlugin):
            self.ui.act_file_save.setEnabled(False)
            self.ui.act_engine_start.setEnabled(True)
            self.ui.act_engine_stop.setEnabled(False)
            self.ui.panelTime.setEnabled(False)

        if self.host.isPlugin or not self.fSessionManagerName:
            self.ui.act_file_open.setEnabled(False)
            self.ui.act_file_save_as.setEnabled(False)

    # --------------------------------------------------------------------------------------------------------
    # Plugins

    def removeAllPlugins(self):
        self.ui.act_plugin_remove_all.setEnabled(False)
        patchcanvas.handleAllPluginsRemoved()

        while self.ui.listWidget.takeItem(0):
            pass

        self.clearSideStuff()

        for pitem in self.fPluginList:
            if pitem is None:
                continue

            pitem.close()
            del pitem

        self.fPluginCount = 0
        self.fPluginList  = []

    # --------------------------------------------------------------------------------------------------------
    # Plugins (menu actions)

    @pyqtSlot()
    def slot_pluginAdd(self, pluginToReplace = -1):
        dialog = PluginDatabaseW(self.fParentOrSelf, self.host)

        if not dialog.exec_():
            return

        if not self.host.is_engine_running():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return

        btype    = dialog.fRetPlugin['build']
        ptype    = dialog.fRetPlugin['type']
        filename = dialog.fRetPlugin['filename']
        label    = dialog.fRetPlugin['label']
        uniqueId = dialog.fRetPlugin['uniqueId']
        extraPtr = self.getExtraPtr(dialog.fRetPlugin)

        if pluginToReplace >= 0:
            if not self.host.replace_plugin(pluginToReplace):
                CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to replace plugin"), self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
                return

        ok = self.host.add_plugin(btype, ptype, filename, None, label, uniqueId, extraPtr, 0x0)

        if pluginToReplace >= 0:
            self.host.replace_plugin(self.host.get_max_plugin_number())

        if not ok:
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot()
    def slot_pluginRemoveAll(self):
        if self.fPluginCount == 0:
            return

        # FIXME - this is not working
        # removing a plugin causes flicker, the rack-list becomes empty for some time
        # this breaks the remove-all animation, so don't bother using it
        if False and not self.host.isPlugin:
            self.projectLoadingStarted()

            app = QApplication.instance()
            app.processEvents()

            for i in reversed(range(self.fPluginCount)):
                self.host.remove_plugin(i)
                app.processEvents()

            self.projectLoadingFinished()

        self.fCurrentlyRemovingAllPlugins = True
        ok = self.host.remove_all_plugins()
        self.fCurrentlyRemovingAllPlugins = False

        if ok:
            self.removeAllPlugins()
        else:
            CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                   self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

        self.fCurrentlyRemovingAllPlugins = False

    # --------------------------------------------------------------------------------------------------------
    # Plugins (macros)

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

    @pyqtSlot()
    def slot_pluginsCompact(self):
        for pitem in self.fPluginList:
            if pitem is None:
                break
            pitem.compact()

    @pyqtSlot()
    def slot_pluginsExpand(self):
        for pitem in self.fPluginList:
            if pitem is None:
                break
            pitem.expand()

    # --------------------------------------------------------------------------------------------------------
    # Plugins (host callbacks)

    @pyqtSlot(int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName):
        pitem = self.ui.listWidget.createItem(pluginId, self.fSavedSettings[CARLA_KEY_MAIN_USE_CUSTOM_SKINS])

        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        self.ui.act_plugin_remove_all.setEnabled(self.fPluginCount > 0)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        patchcanvas.handlePluginRemoved(pluginId)

        if pluginId in self.fSelectedPlugins:
            self.clearSideStuff()

        pitem = self.getPluginItem(pluginId)

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)
        self.ui.listWidget.takeItem(pluginId)

        if pitem is not None:
            pitem.close()
            del pitem

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pitem = self.fPluginList[i]
            pitem.setPluginId(i)

        self.ui.act_plugin_remove_all.setEnabled(self.fPluginCount > 0)

    # --------------------------------------------------------------------------------------------------------
    # Canvas

    def clearSideStuff(self):
        self.scene.clearSelection()

        self.fSelectedPlugins = []

        self.ui.keyboard.allNotesOff(False)
        self.ui.scrollArea.setEnabled(False)

        self.fPeaksCleared = True
        self.ui.peak_in.displayMeter(1, 0.0, True)
        self.ui.peak_in.displayMeter(2, 0.0, True)
        self.ui.peak_out.displayMeter(1, 0.0, True)
        self.ui.peak_out.displayMeter(2, 0.0, True)

    def setupCanvas(self):
        pOptions = patchcanvas.options_t()
        pOptions.theme_name        = self.fSavedSettings[CARLA_KEY_CANVAS_THEME]
        pOptions.auto_hide_groups  = self.fSavedSettings[CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS]
        pOptions.auto_select_items = self.fSavedSettings[CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS]
        pOptions.use_bezier_lines  = self.fSavedSettings[CARLA_KEY_CANVAS_USE_BEZIER_LINES]
        pOptions.antialiasing      = self.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING]
        pOptions.eyecandy          = self.fSavedSettings[CARLA_KEY_CANVAS_EYE_CANDY]

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = False
        pFeatures.port_rename  = False
        pFeatures.handle_group_pos = True

        patchcanvas.setOptions(pOptions)
        patchcanvas.setFeatures(pFeatures)
        patchcanvas.init("Carla2", self.scene, canvasCallback, False)

        tryCanvasSize = self.fSavedSettings[CARLA_KEY_CANVAS_SIZE].split("x")

        if len(tryCanvasSize) == 2 and tryCanvasSize[0].isdigit() and tryCanvasSize[1].isdigit():
            self.fCanvasWidth  = int(tryCanvasSize[0])
            self.fCanvasHeight = int(tryCanvasSize[1])
        else:
            self.fCanvasWidth  = CARLA_DEFAULT_CANVAS_SIZE_WIDTH
            self.fCanvasHeight = CARLA_DEFAULT_CANVAS_SIZE_HEIGHT

        patchcanvas.setCanvasSize(0, 0, self.fCanvasWidth, self.fCanvasHeight)
        patchcanvas.setInitialPos(self.fCanvasWidth / 2, self.fCanvasHeight / 2)
        self.ui.graphicsView.setSceneRect(0, 0, self.fCanvasWidth, self.fCanvasHeight)

        self.ui.miniCanvasPreview.setViewTheme(patchcanvas.canvas.theme.canvas_bg, patchcanvas.canvas.theme.rubberband_brush, patchcanvas.canvas.theme.rubberband_pen.color())
        self.ui.miniCanvasPreview.init(self.scene, self.fCanvasWidth, self.fCanvasHeight, self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING])

    def updateCanvasInitialPos(self):
        x = self.ui.graphicsView.horizontalScrollBar().value() + self.width()/4
        y = self.ui.graphicsView.verticalScrollBar().value() + self.height()/4
        patchcanvas.setInitialPos(x, y)

    # --------------------------------------------------------------------------------------------------------
    # Canvas (menu actions)

    @pyqtSlot()
    def slot_canvasShowInternal(self):
        self.fExternalPatchbay = False
        self.ui.act_canvas_show_internal.blockSignals(True)
        self.ui.act_canvas_show_external.blockSignals(True)
        self.ui.act_canvas_show_internal.setChecked(True)
        self.ui.act_canvas_show_external.setChecked(False)
        self.ui.act_canvas_show_internal.blockSignals(False)
        self.ui.act_canvas_show_external.blockSignals(False)
        self.slot_canvasRefresh()

    @pyqtSlot()
    def slot_canvasShowExternal(self):
        self.fExternalPatchbay = True
        self.ui.act_canvas_show_internal.blockSignals(True)
        self.ui.act_canvas_show_external.blockSignals(True)
        self.ui.act_canvas_show_internal.setChecked(False)
        self.ui.act_canvas_show_external.setChecked(True)
        self.ui.act_canvas_show_internal.blockSignals(False)
        self.ui.act_canvas_show_external.blockSignals(False)
        self.slot_canvasRefresh()

    @pyqtSlot()
    def slot_canvasArrange(self):
        patchcanvas.arrange()

    @pyqtSlot()
    def slot_canvasRefresh(self):
        patchcanvas.clear()

        if self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK and (self.host.isControl or self.host.isPlugin):
            return

        if self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

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

        if self.fExportPrinter is None:
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

        if newPath.lower().endswith((".jpg", ".jpeg")):
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

    # --------------------------------------------------------------------------------------------------------
    # Canvas (canvas callbacks)

    @pyqtSlot(int, int, QPointF)
    def slot_canvasItemMoved(self, group_id, split_mode, pos):
        self.ui.miniCanvasPreview.update()

    @pyqtSlot(float)
    def slot_canvasScaleChanged(self, scale):
        self.ui.miniCanvasPreview.setViewScale(scale)

    @pyqtSlot(list)
    def slot_canvasPluginSelected(self, pluginList):
        self.ui.keyboard.allNotesOff(False)
        self.ui.scrollArea.setEnabled(len(pluginList) != 0) # and self.fPluginCount > 0
        self.fSelectedPlugins = pluginList

    # --------------------------------------------------------------------------------------------------------
    # Canvas (host callbacks)

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

        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

        if pluginId < 0:
            return
        if pluginId >= self.fPluginCount:
            print("sorry, can't map this plugin to canvas client", pluginId, self.fPluginCount)
            return

        patchcanvas.setGroupAsPlugin(clientId, pluginId, bool(self.host.get_plugin_info(pluginId)['hints'] & PLUGIN_HAS_CUSTOM_UI))

    @pyqtSlot(int)
    def slot_handlePatchbayClientRemovedCallback(self, clientId):
        patchcanvas.removeGroup(clientId)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

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
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

        if pluginId < 0:
            return
        if pluginId >= self.fPluginCount:
            print("sorry, can't map this plugin to canvas client", pluginId, self.fPluginCount)
            return

        patchcanvas.setGroupAsPlugin(clientId, pluginId, bool(self.host.get_plugin_info(pluginId)['hints'] & PLUGIN_HAS_CUSTOM_UI))

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortAddedCallback(self, clientId, portId, portFlags, portName):
        if portFlags & PATCHBAY_PORT_IS_INPUT:
            portMode = patchcanvas.PORT_MODE_INPUT
        else:
            portMode = patchcanvas.PORT_MODE_OUTPUT

        if portFlags & PATCHBAY_PORT_TYPE_AUDIO:
            portType    = patchcanvas.PORT_TYPE_AUDIO_JACK
            isAlternate = False
        elif portFlags & PATCHBAY_PORT_TYPE_CV:
            portType    = patchcanvas.PORT_TYPE_AUDIO_JACK
            isAlternate = True
        elif portFlags & PATCHBAY_PORT_TYPE_MIDI:
            portType    = patchcanvas.PORT_TYPE_MIDI_JACK
            isAlternate = False
        else:
            portType    = patchcanvas.PORT_TYPE_NULL
            isAlternate = False

        patchcanvas.addPort(clientId, portId, portName, portMode, portType, isAlternate)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    @pyqtSlot(int, int)
    def slot_handlePatchbayPortRemovedCallback(self, groupId, portId):
        patchcanvas.removePort(groupId, portId)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    @pyqtSlot(int, int, str)
    def slot_handlePatchbayPortRenamedCallback(self, groupId, portId, newPortName):
        patchcanvas.renamePort(groupId, portId, newPortName)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    @pyqtSlot(int, int, int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, groupOutId, portOutId, groupInId, portInId):
        patchcanvas.connectPorts(connectionId, groupOutId, portOutId, groupInId, portInId)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId, portOutId, portInId):
        patchcanvas.disconnectPorts(connectionId)
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

    # --------------------------------------------------------------------------------------------------------
    # Settings

    def saveSettings(self):
        settings = QSettings()

        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("TimePanelGeometry", self.ui.panelTime.saveGeometry())

        #settings.setValue("SplitterState", self.ui.splitter.saveState())

        if not (self.host.isControl or self.host.isPlugin):
            settings.setValue("ShowTimePanel", self.ui.panelTime.isVisible())

        settings.setValue("ShowToolbar", self.ui.toolBar.isEnabled())

        diskFolders = []

        for i in range(self.ui.cb_disk.count()):
            diskFolders.append(self.ui.cb_disk.itemData(i))

        settings.setValue("DiskFolders", diskFolders)

        settings.setValue("ShowMeters", self.ui.act_settings_show_meters.isChecked())
        settings.setValue("ShowKeyboard", self.ui.act_settings_show_keyboard.isChecked())
        settings.setValue("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().value())

    def loadSettings(self, firstTime):
        settings = QSettings()

        if firstTime:
            self.restoreGeometry(settings.value("Geometry", ""))

            if not (self.host.isControl or self.host.isPlugin):
                self.ui.panelTime.restoreGeometry(settings.value("TimePanelGeometry", ""))

                showTimePanel = settings.value("ShowTimePanel", True, type=bool)
                self.ui.act_settings_show_time_panel.setChecked(showTimePanel)

                if showTimePanel:
                    QTimer.singleShot(0, self.ui.panelTime.show)
                else:
                    self.ui.panelTime.hide()

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setEnabled(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

            #if settings.contains("SplitterState"):
                #self.ui.splitter.restoreState(settings.value("SplitterState", ""))
            #else:
                #self.ui.splitter.setSizes([210, 99999])

            diskFolders = toList(settings.value("DiskFolders", [HOME]))

            self.ui.cb_disk.setItemData(0, HOME)

            for i in range(len(diskFolders)):
                if i == 0: continue
                folder = diskFolders[i]
                self.ui.cb_disk.addItem(os.path.basename(folder), folder)

            #if MACOS and not settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, True, type=bool):
            #    self.setUnifiedTitleAndToolBarOnMac(True)

            showMeters = settings.value("ShowMeters", False, type=bool)
            self.ui.act_settings_show_meters.setChecked(showMeters)
            self.ui.peak_in.setVisible(showMeters)
            self.ui.peak_out.setVisible(showMeters)

            showKeyboard = settings.value("ShowKeyboard", not(MACOS or WINDOWS), type=bool)
            self.ui.act_settings_show_keyboard.setChecked(showKeyboard)
            self.ui.scrollArea.setVisible(showKeyboard)

            QTimer.singleShot(100, self.slot_restoreCanvasScrollbarValues)

        # TODO - complete this

        self.fSavedSettings = {
            CARLA_KEY_MAIN_PROJECT_FOLDER:      settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER,      CARLA_DEFAULT_MAIN_PROJECT_FOLDER,      type=str),
            CARLA_KEY_MAIN_REFRESH_INTERVAL:    settings.value(CARLA_KEY_MAIN_REFRESH_INTERVAL,    CARLA_DEFAULT_MAIN_REFRESH_INTERVAL,    type=int),
            CARLA_KEY_MAIN_USE_CUSTOM_SKINS:    settings.value(CARLA_KEY_MAIN_USE_CUSTOM_SKINS,    CARLA_DEFAULT_MAIN_USE_CUSTOM_SKINS,    type=bool),
            CARLA_KEY_CANVAS_THEME:             settings.value(CARLA_KEY_CANVAS_THEME,             CARLA_DEFAULT_CANVAS_THEME,             type=str),
            CARLA_KEY_CANVAS_SIZE:              settings.value(CARLA_KEY_CANVAS_SIZE,              CARLA_DEFAULT_CANVAS_SIZE,              type=str),
            CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS:  settings.value(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,  CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS,  type=bool),
            CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS: settings.value(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS, type=bool),
            CARLA_KEY_CANVAS_USE_BEZIER_LINES:  settings.value(CARLA_KEY_CANVAS_USE_BEZIER_LINES,  CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES,  type=bool),
            CARLA_KEY_CANVAS_EYE_CANDY:         settings.value(CARLA_KEY_CANVAS_EYE_CANDY,         CARLA_DEFAULT_CANVAS_EYE_CANDY,         type=int),
            CARLA_KEY_CANVAS_USE_OPENGL:        settings.value(CARLA_KEY_CANVAS_USE_OPENGL,        CARLA_DEFAULT_CANVAS_USE_OPENGL,        type=bool),
            CARLA_KEY_CANVAS_ANTIALIASING:      settings.value(CARLA_KEY_CANVAS_ANTIALIASING,      CARLA_DEFAULT_CANVAS_ANTIALIASING,      type=int),
            CARLA_KEY_CANVAS_HQ_ANTIALIASING :  settings.value(CARLA_KEY_CANVAS_HQ_ANTIALIASING,   CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING,   type=bool),
            CARLA_KEY_CUSTOM_PAINTING:         (settings.value(CARLA_KEY_MAIN_USE_PRO_THEME,    True,   type=bool) and
                                                settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR, "Black", type=str).lower() == "black")
        }

        self.fMiniCanvasUpdateTimeout = 1000 if self.fSavedSettings[CARLA_KEY_CANVAS_EYE_CANDY] == patchcanvas.EYECANDY_FULL else 0

        setEngineSettings(self.host)
        self.restartTimersIfNeeded()

    # --------------------------------------------------------------------------------------------------------
    # Settings (helpers)

    @pyqtSlot()
    def slot_restoreCanvasScrollbarValues(self):
        settings = QSettings()
        self.ui.graphicsView.horizontalScrollBar().setValue(settings.value("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().maximum()/2, type=int))
        self.ui.graphicsView.verticalScrollBar().setValue(settings.value("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().maximum()/2, type=int))

    # --------------------------------------------------------------------------------------------------------
    # Settings (menu actions)

    @pyqtSlot(bool)
    def slot_showTimePanel(self, yesNo):
        self.ui.panelTime.setVisible(yesNo)

    @pyqtSlot(bool)
    def slot_showToolbar(self, yesNo):
        self.ui.toolBar.setEnabled(yesNo)
        self.ui.toolBar.setVisible(yesNo)

    @pyqtSlot(bool)
    def slot_showCanvasMeters(self, yesNo):
        self.ui.peak_in.setVisible(yesNo)
        self.ui.peak_out.setVisible(yesNo)
        QTimer.singleShot(0, self.slot_miniCanvasCheckAll)

    @pyqtSlot(bool)
    def slot_showCanvasKeyboard(self, yesNo):
        self.ui.scrollArea.setVisible(yesNo)
        QTimer.singleShot(0, self.slot_miniCanvasCheckAll)

    @pyqtSlot()
    def slot_configureCarla(self):
        dialog = CarlaSettingsW(self.fParentOrSelf, self.host, True, hasGL)
        if not dialog.exec_():
            return

        self.loadSettings(False)

        patchcanvas.clear()

        self.setupCanvas()
        self.slot_miniCanvasCheckAll()

        if self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK and self.host.isPlugin:
            pass
        elif self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

        for pitem in self.fPluginList:
            if pitem is None:
                break
            pitem.setUsingSkins(self.fSavedSettings[CARLA_KEY_MAIN_USE_CUSTOM_SKINS])
            pitem.recreateWidget()

    # --------------------------------------------------------------------------------------------------------
    # About (menu actions)

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self.fParentOrSelf, self.host).exec_()

    @pyqtSlot()
    def slot_aboutJuce(self):
        JuceAboutW(self.fParentOrSelf).exec_()

    @pyqtSlot()
    def slot_aboutQt(self):
        QApplication.instance().aboutQt()

    # --------------------------------------------------------------------------------------------------------
    # Disk (menu actions)

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

        if not self.host.load_file(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    # --------------------------------------------------------------------------------------------------------
    # Canvas scrollbars

    @pyqtSlot(int)
    def slot_horizontalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.horizontalScrollBar().maximum()
        if maximum == 0:
            xp = 0
        else:
            xp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosX(xp)
        self.updateCanvasInitialPos()

    @pyqtSlot(int)
    def slot_verticalScrollBarChanged(self, value):
        maximum = self.ui.graphicsView.verticalScrollBar().maximum()
        if maximum == 0:
            yp = 0
        else:
            yp = float(value) / maximum
        self.ui.miniCanvasPreview.setViewPosY(yp)
        self.updateCanvasInitialPos()

    # --------------------------------------------------------------------------------------------------------
    # Canvas keyboard

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

    # --------------------------------------------------------------------------------------------------------
    # Canvas keyboard (host callbacks)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velocity):
        if pluginId in self.fSelectedPlugins:
            self.ui.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if pluginId in self.fSelectedPlugins:
            self.ui.keyboard.sendNoteOff(note, False)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        pitem = self.getPluginItem(pluginId)

        if pitem is None:
            return

        wasCompacted = pitem.isCompacted()
        isCompacted  = wasCompacted

        for i in range(self.host.get_custom_data_count(pluginId)):
            cdata = self.host.get_custom_data(pluginId, i)

            if cdata['type'] == CUSTOM_DATA_TYPE_PROPERTY and cdata['key'] == "CarlaSkinIsCompacted":
                isCompacted = bool(cdata['value'] == "true")
                break
        else:
            return

        if wasCompacted == isCompacted:
            return

        pitem.recreateWidget(True)

    # --------------------------------------------------------------------------------------------------------
    # MiniCanvas stuff

    @pyqtSlot()
    def slot_miniCanvasCheckAll(self):
        self.slot_miniCanvasCheckSize()
        self.slot_horizontalScrollBarChanged(self.ui.graphicsView.horizontalScrollBar().value())
        self.slot_verticalScrollBarChanged(self.ui.graphicsView.verticalScrollBar().value())

    @pyqtSlot()
    def slot_miniCanvasCheckSize(self):
        if self.fCanvasWidth == 0 or self.fCanvasHeight == 0:
            return

        if self.ui.tabWidget.currentIndex() == 1:
            width  = self.ui.graphicsView.width()
            height = self.ui.graphicsView.height()
        else:
            self.ui.tabWidget.blockSignals(True)
            self.ui.tabWidget.setCurrentIndex(1)
            width  = self.ui.graphicsView.width()
            height = self.ui.graphicsView.height()
            self.ui.tabWidget.setCurrentIndex(0)
            self.ui.tabWidget.blockSignals(False)

        self.ui.miniCanvasPreview.setViewSize(float(width)/self.fCanvasWidth, float(height)/self.fCanvasHeight)

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        hsb = self.ui.graphicsView.horizontalScrollBar()
        vsb = self.ui.graphicsView.verticalScrollBar()
        hsb.setValue(xp * hsb.maximum())
        vsb.setValue(yp * vsb.maximum())
        self.updateCanvasInitialPos()

    # --------------------------------------------------------------------------------------------------------
    # Timers

    def startTimers(self):
        if self.fIdleTimerFast == 0:
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])

        if self.fIdleTimerSlow == 0:
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*4)

    def restartTimersIfNeeded(self):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL])

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = self.startTimer(self.fSavedSettings[CARLA_KEY_MAIN_REFRESH_INTERVAL]*4)

    def killTimers(self):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

    # --------------------------------------------------------------------------------------------------------
    # Misc

    @pyqtSlot()
    def slot_timePanelClosed(self):
        self.ui.act_settings_show_time_panel.setChecked(False)

    @pyqtSlot(int)
    def slot_tabChanged(self, index):
        if index != 1:
            return

        self.ui.graphicsView.setFocus()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if pluginId >= self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return

        pitem.recreateWidget()

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int, int, str)
    def slot_handleNSMCallback(self, value1, value2, valueStr):
        # Error
        if value1 == 0:
            pass

        # Reply
        elif value1 == 1:
            self.fFirstEngineInit    = False
            self.fSessionManagerName = valueStr
            self.setProperWindowTitle()

        # Open
        elif value1 == 2:
            self.fClientName      = os.path.basename(valueStr)
            self.fProjectFilename = QFileInfo(valueStr+".carxp").absoluteFilePath()
            self.setProperWindowTitle()
            self.slot_engineStop(True)
            self.slot_engineStart()
            self.loadProjectNow()

        # Save
        elif value1 == 3:
            self.saveProjectNow()

        # Session is Loaded
        elif value1 == 4:
            pass

        # Show Optional Gui
        elif value1 == 5:
            self.show()

        # Hide Optional Gui
        elif value1 == 6:
            self.hide()

        self.host.nsm_ready(value1)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG:", pluginId, value1, value2, value3, valueStr)

    @pyqtSlot(str)
    def slot_handleInfoCallback(self, info):
        QMessageBox.information(self, "Information", info)

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, "Error", error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        pass # TODO

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        self.slot_fileSave()

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    # --------------------------------------------------------------------------------------------------------
    # Internal stuff

    def getExtraPtr(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']

            self.maybeLoadRDFs()

            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype in (PLUGIN_GIG, PLUGIN_SF2):
            if plugin['name'].lower().endswith(" (16 outputs)"):
                return self._true

        return None

    def maybeLoadRDFs(self):
        if not self.fLadspaRdfNeedsUpdate:
            return

        self.fLadspaRdfNeedsUpdate = False
        self.fLadspaRdfList = []

        if not haveLRDF:
            return

        settingsDir  = os.path.join(HOME, ".config", "falkTX")
        frLadspaFile = os.path.join(settingsDir, "ladspa_rdf.db")

        if os.path.exists(frLadspaFile):
            frLadspa = open(frLadspaFile, 'r')

            try:
                self.fLadspaRdfList = ladspa_rdf.get_c_ladspa_rdfs(json.load(frLadspa))
            except:
                pass

            frLadspa.close()

    # --------------------------------------------------------------------------------------------------------

    def getPluginItem(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        #if False:
            #return CarlaRackItem(self, 0, False)

        return pitem

    def getPluginEditDialog(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        if False:
            return PluginEdit(self, self.host, 0)

        return pitem.getEditDialog()

    def getPluginSlotWidget(self, pluginId):
        if pluginId >= self.fPluginCount:
            return None

        pitem = self.fPluginList[pluginId]
        if pitem is None:
            return None
        #if False:
            #return AbstractPluginSlot()

        return pitem.getWidget()

    # --------------------------------------------------------------------------------------------------------
    # show/hide event

    def showEvent(self, event):
        QMainWindow.showEvent(self, event)

        # set our gui as parent for all plugins UIs
        if not (self.host.isControl or self.host.isPlugin):
            winIdStr = "%x" % self.winId()
            self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, winIdStr)

    def hideEvent(self, event):
        # disable parent
        if not (self.host.isControl or self.host.isPlugin):
            self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0")

        QMainWindow.hideEvent(self, event)

    # --------------------------------------------------------------------------------------------------------
    # resize event

    def fixCanvasPreviewSize(self):
        self.ui.patchbay.resize(self.ui.rack.size())
        self.slot_miniCanvasCheckSize()

    def resizeEvent(self, event):
        QMainWindow.resizeEvent(self, event)

        if self.ui.tabWidget.currentIndex() != 1:
            size = self.ui.rack.size()
            self.ui.patchbay.resize(size)

        self.slot_miniCanvasCheckSize()

    # --------------------------------------------------------------------------------------------------------
    # timer event

    def idleFast(self):
        self.host.engine_idle()
        self.ui.panelTime.refreshTransport()

        if self.fPluginCount == 0 or self.fCurrentlyRemovingAllPlugins:
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().idleFast()

        for pluginId in self.fSelectedPlugins:
            self.fPeaksCleared = False
            if self.ui.peak_in.isVisible():
                self.ui.peak_in.displayMeter(1, self.host.get_input_peak_value(pluginId, True))
                self.ui.peak_in.displayMeter(2, self.host.get_input_peak_value(pluginId, False))
            if self.ui.peak_out.isVisible():
                self.ui.peak_out.displayMeter(1, self.host.get_output_peak_value(pluginId, True))
                self.ui.peak_out.displayMeter(2, self.host.get_output_peak_value(pluginId, False))
            return

        if self.fPeaksCleared:
            return

        self.fPeaksCleared = True
        self.ui.peak_in.displayMeter(1, 0.0, True)
        self.ui.peak_in.displayMeter(2, 0.0, True)
        self.ui.peak_out.displayMeter(1, 0.0, True)
        self.ui.peak_out.displayMeter(2, 0.0, True)

    def idleSlow(self):
        if self.fPluginCount == 0 or self.fCurrentlyRemovingAllPlugins:
            return

        for pitem in self.fPluginList:
            if pitem is None:
                break

            pitem.getWidget().idleSlow()

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            self.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            self.idleSlow()

        QMainWindow.timerEvent(self, event)

    # --------------------------------------------------------------------------------------------------------
    # paint event

    #def paintEvent(self, event):
        #QMainWindow.paintEvent(self, event)

        #if MACOS or not self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING]:
            #return

        #painter = QPainter(self)
        #painter.setBrush(QColor(36, 36, 36))
        #painter.setPen(QColor(62, 62, 62))
        #painter.drawRect(1, self.height()/2, self.width()-3, self.height()-self.height()/2-1)

    # --------------------------------------------------------------------------------------------------------
    # close event

    def closeEvent(self, event):
        self.killTimers()
        self.saveSettings()

        if self.host.is_engine_running() and not (self.host.isControl or self.host.isPlugin):
            self.slot_engineStop(True)

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------
# Canvas callback

def canvasCallback(action, value1, value2, valueStr):
    host = gCarla.gui.host

    if action == patchcanvas.ACTION_GROUP_INFO:
        pass

    elif action == patchcanvas.ACTION_GROUP_RENAME:
        pass

    elif action == patchcanvas.ACTION_GROUP_SPLIT:
        groupId = value1
        patchcanvas.splitGroup(groupId)
        gCarla.gui.ui.miniCanvasPreview.update()

    elif action == patchcanvas.ACTION_GROUP_JOIN:
        groupId = value1
        patchcanvas.joinGroup(groupId)
        gCarla.gui.ui.miniCanvasPreview.update()

    elif action == patchcanvas.ACTION_PORT_INFO:
        pass

    elif action == patchcanvas.ACTION_PORT_RENAME:
        pass

    elif action == patchcanvas.ACTION_PORTS_CONNECT:
        gOut, pOut, gIn, pIn = [int(i) for i in valueStr.split(":")]

        if not host.patchbay_connect(gOut, pOut, gIn, pIn):
            print("Connection failed:", host.get_last_error())

    elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
        connectionId = value1

        if not host.patchbay_disconnect(connectionId):
            print("Disconnect failed:", host.get_last_error())

    elif action == patchcanvas.ACTION_PLUGIN_CLONE:
        pluginId = value1

        if not host.clone_plugin(pluginId):
            CustomMessageBox(gCarla.gui, QMessageBox.Warning, gCarla.gui.tr("Error"), gCarla.gui.tr("Operation failed"),
                                         host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    elif action == patchcanvas.ACTION_PLUGIN_EDIT:
        pluginId = value1

        dialog = gCarla.gui.getPluginEditDialog(pluginId)

        if dialog is None:
            return

        dialog.show()

    elif action == patchcanvas.ACTION_PLUGIN_RENAME:
        pluginId = value1
        newName  = valueStr

        if not host.rename_plugin(pluginId, newName):
            CustomMessageBox(gCarla.gui, QMessageBox.Warning, gCarla.gui.tr("Error"), gCarla.gui.tr("Operation failed"),
                                         host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    elif action == patchcanvas.ACTION_PLUGIN_REMOVE:
        pluginId = value1

        if not host.remove_plugin(pluginId):
            CustomMessageBox(gCarla.gui, QMessageBox.Warning, gCarla.gui.tr("Error"), gCarla.gui.tr("Operation failed"),
                                         host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    elif action == patchcanvas.ACTION_PLUGIN_SHOW_UI:
        pluginId = value1

        host.show_custom_ui(pluginId, True)

        # FIXME
        pwidget = gCarla.gui.getPluginSlotWidget(pluginId)

        if pwidget is not None and pwidget.b_gui is not None:
            pwidget.b_gui.setChecked(True)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def engineCallback(host, action, pluginId, value1, value2, value3, valueStr):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    if action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.processMode   = value1
        host.transportMode = value2
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.processMode   = value1
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.transportMode = value1

    valueStr = charPtrToString(valueStr)

    if action == ENGINE_CALLBACK_DEBUG:
        host.DebugCallback.emit(pluginId, value1, value2, value3, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_ADDED:
        host.PluginAddedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_REMOVED:
        host.PluginRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PLUGIN_RENAMED:
        host.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
        host.PluginUnavailableCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
        host.ParameterValueChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
        host.ParameterDefaultChangedCallback.emit(pluginId, value1, value3)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        host.ParameterMidiCcChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        host.ParameterMidiChannelChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PROGRAM_CHANGED:
        host.ProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
        host.MidiProgramChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_OPTION_CHANGED:
        host.OptionChangedCallback.emit(pluginId, value1, bool(value2))
    elif action == ENGINE_CALLBACK_UI_STATE_CHANGED:
        host.UiStateChangedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_NOTE_ON:
        host.NoteOnCallback.emit(pluginId, value1, value2, round(value3))
    elif action == ENGINE_CALLBACK_NOTE_OFF:
        host.NoteOffCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_UPDATE:
        host.UpdateCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_INFO:
        host.ReloadInfoCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PARAMETERS:
        host.ReloadParametersCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_PROGRAMS:
        host.ReloadProgramsCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_RELOAD_ALL:
        host.ReloadAllCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
        host.PatchbayClientAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
        host.PatchbayClientRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
        host.PatchbayClientRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
        host.PatchbayClientDataChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
        host.PatchbayPortAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
        host.PatchbayPortRemovedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_RENAMED:
        host.PatchbayPortRenamedCallback.emit(pluginId, value1, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
        gOut, pOut, gIn, pIn = [int(i) for i in valueStr.split(":")] # FIXME
        host.PatchbayConnectionAddedCallback.emit(pluginId, gOut, pOut, gIn, pIn)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        host.PatchbayConnectionRemovedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.EngineStartedCallback.emit(value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_ENGINE_STOPPED:
        host.EngineStoppedCallback.emit()
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.ProcessModeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.TransportModeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
        host.BufferSizeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
        host.SampleRateChangedCallback.emit(value3)
    elif action == ENGINE_CALLBACK_NSM:
        host.NSMCallback.emit(value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_IDLE:
        QApplication.instance().processEvents()
    elif action == ENGINE_CALLBACK_INFO:
        host.InfoCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_ERROR:
        host.ErrorCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_QUIT:
        host.QuitCallback.emit()

# ------------------------------------------------------------------------------------------------------------
# File callback

def fileCallback(ptr, action, isDir, title, filter):
    ret = ("", "") if config_UseQt5 else ""

    if action == FILE_CALLBACK_DEBUG:
        pass
    elif action == FILE_CALLBACK_OPEN:
        ret = QFileDialog.getOpenFileName(gCarla.gui, charPtrToString(title), "", charPtrToString(filter) ) #, QFileDialog.ShowDirsOnly if isDir else 0x0)
    elif action == FILE_CALLBACK_SAVE:
        ret = QFileDialog.getSaveFileName(gCarla.gui, charPtrToString(title), "", charPtrToString(filter), QFileDialog.ShowDirsOnly if isDir else 0x0)

    if config_UseQt5:
        ret = ret[0]
    if not ret:
        return None

    # FIXME
    global fileRet
    fileRet = c_char_p(ret.encode("utf-8"))
    retval  = cast(byref(fileRet), POINTER(c_uintptr))
    return retval.contents.value

# ------------------------------------------------------------------------------------------------------------
# Init host

def initHost(initName, libPrefix, isControl, isPlugin, failError, HostClass = None):
    pathBinaries, pathResources = getPaths(libPrefix)

    # --------------------------------------------------------------------------------------------------------
    # Fail if binary dir is not found

    if not os.path.exists(pathBinaries):
        if failError:
            QMessageBox.critical(None, "Error", "Failed to find the carla binaries, cannot continue")
            sys.exit(1)
        return

    # --------------------------------------------------------------------------------------------------------
    # Set Carla library name

    libname   = "libcarla_%s2.%s"   % ("control" if isControl else "standalone", DLL_EXTENSION)
    utilsname = "libcarla_utils.%s" % (DLL_EXTENSION)

    # --------------------------------------------------------------------------------------------------------
    # Print info

    print("Carla %s started, status:" % VERSION)
    print("  Python version: %s" % sys.version.split(" ",1)[0])
    print("  Qt version:     %s" % qVersion())
    print("  PyQt version:   %s" % PYQT_VERSION_STR)
    print("  Binary dir:     %s" % pathBinaries)
    print("  Resources dir:  %s" % pathResources)

    # --------------------------------------------------------------------------------------------------------
    # Init host

    if failError:
        # no try
        host = HostClass() if HostClass is not None else CarlaHostQtDLL(os.path.join(pathBinaries, libname))
    else:
        try:
            host = HostClass() if HostClass is not None else CarlaHostQtDLL(os.path.join(pathBinaries, libname))
        except:
            host = CarlaHostQtNull()

    host.isControl = isControl
    host.isPlugin  = isPlugin

    host.set_engine_callback(lambda h,a,p,v1,v2,v3,vs: engineCallback(host,a,p,v1,v2,v3,vs))
    host.set_file_callback(fileCallback)

    # If it's a plugin the paths are already set
    if not isPlugin:
        host.pathBinaries  = pathBinaries
        host.pathResources = pathResources
        host.set_engine_option(ENGINE_OPTION_PATH_BINARIES,  0, pathBinaries)
        host.set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, pathResources)

        if not isControl:
            host.nsmOK = host.nsm_init(os.getpid(), initName)

    # --------------------------------------------------------------------------------------------------------
    # Init utils

    gCarla.utils = CarlaUtils(os.path.join(pathBinaries, utilsname))
    gCarla.utils.set_process_name(os.path.basename(initName))
    #gCarla.utils.set_locale_C()

    # --------------------------------------------------------------------------------------------------------
    # Done

    return host

# ------------------------------------------------------------------------------------------------------------
# Load host settings

def loadHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    settings = QSettings("falkTX", "Carla2")

    # bool values
    try:
        host.forceStereo = settings.value(CARLA_KEY_ENGINE_FORCE_STEREO, CARLA_DEFAULT_FORCE_STEREO, type=bool)
    except:
        host.forceStereo = CARLA_DEFAULT_FORCE_STEREO

    try:
        host.preferPluginBridges = settings.value(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, type=bool)
    except:
        host.preferPluginBridges = CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES

    try:
        host.preferUIBridges = settings.value(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES, CARLA_DEFAULT_PREFER_UI_BRIDGES, type=bool)
    except:
        host.preferUIBridges = CARLA_DEFAULT_PREFER_UI_BRIDGES

    try:
        host.uisAlwaysOnTop = settings.value(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP, CARLA_DEFAULT_UIS_ALWAYS_ON_TOP, type=bool)
    except:
        host.uisAlwaysOnTop = CARLA_DEFAULT_UIS_ALWAYS_ON_TOP

    # int values
    try:
        host.maxParameters = settings.value(CARLA_KEY_ENGINE_MAX_PARAMETERS, CARLA_DEFAULT_MAX_PARAMETERS, type=int)
    except:
        host.maxParameters = CARLA_DEFAULT_MAX_PARAMETERS

    try:
        host.uiBridgesTimeout = settings.value(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT, CARLA_DEFAULT_UI_BRIDGES_TIMEOUT, type=int)
    except:
        host.uiBridgesTimeout = CARLA_DEFAULT_UI_BRIDGES_TIMEOUT

    if host.isPlugin:
        return

    # enums
    try:
        host.transportMode = settings.value(CARLA_KEY_ENGINE_TRANSPORT_MODE, CARLA_DEFAULT_TRANSPORT_MODE, type=int)
    except:
        host.transportMode = CARLA_DEFAULT_TRANSPORT_MODE

    if not host.processModeForced:
        try:
            host.processMode = settings.value(CARLA_KEY_ENGINE_PROCESS_MODE, CARLA_DEFAULT_PROCESS_MODE, type=int)
        except:
            host.processMode = CARLA_DEFAULT_PROCESS_MODE

    host.nextProcessMode = host.processMode

    # --------------------------------------------------------------------------------------------------------
    # fix things if needed

    if host.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS and LADISH_APP_NAME:
        print("LADISH detected but using multiple clients (not allowed), forcing single client now")
        host.nextProcessMode = host.processMode = ENGINE_PROCESS_MODE_SINGLE_CLIENT

    # --------------------------------------------------------------------------------------------------------
    # run headless host now if nogui option enabled

    if gCarla.nogui:
        runHostWithoutUI(host)

# ------------------------------------------------------------------------------------------------------------
# Set host settings

def setHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    # TEST
    #host.preventBadBehaviour = True

    host.set_engine_option(ENGINE_OPTION_FORCE_STEREO,          host.forceStereo,         "")
    host.set_engine_option(ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, host.preferPluginBridges, "")
    host.set_engine_option(ENGINE_OPTION_PREFER_UI_BRIDGES,     host.preferUIBridges,     "")
    host.set_engine_option(ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     host.uisAlwaysOnTop,      "")
    host.set_engine_option(ENGINE_OPTION_MAX_PARAMETERS,        host.maxParameters,       "")
    host.set_engine_option(ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    host.uiBridgesTimeout,    "")
    host.set_engine_option(ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, host.preventBadBehaviour, "")

    if host.isPlugin:
        return

    host.set_engine_option(ENGINE_OPTION_PROCESS_MODE,          host.nextProcessMode,     "")
    host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,        host.transportMode,       "")

# ------------------------------------------------------------------------------------------------------------
# Set Engine settings according to carla preferences. Returns selected audio driver.

def setEngineSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    # --------------------------------------------------------------------------------------------------------
    # do nothing if control

    if host.isControl:
        return "Control"

    # --------------------------------------------------------------------------------------------------------

    settings = QSettings("falkTX", "Carla2")

    # --------------------------------------------------------------------------------------------------------
    # main settings

    setHostSettings(host)

    # --------------------------------------------------------------------------------------------------------
    # plugin paths

    LADSPA_PATH = toList(settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH))
    DSSI_PATH   = toList(settings.value(CARLA_KEY_PATHS_DSSI,   CARLA_DEFAULT_DSSI_PATH))
    LV2_PATH    = toList(settings.value(CARLA_KEY_PATHS_LV2,    CARLA_DEFAULT_LV2_PATH))
    VST2_PATH   = toList(settings.value(CARLA_KEY_PATHS_VST2,   CARLA_DEFAULT_VST2_PATH))
    VST3_PATH   = toList(settings.value(CARLA_KEY_PATHS_VST3,   CARLA_DEFAULT_VST3_PATH))
    GIG_PATH    = toList(settings.value(CARLA_KEY_PATHS_GIG,    CARLA_DEFAULT_GIG_PATH))
    SF2_PATH    = toList(settings.value(CARLA_KEY_PATHS_SF2,    CARLA_DEFAULT_SF2_PATH))
    SFZ_PATH    = toList(settings.value(CARLA_KEY_PATHS_SFZ,    CARLA_DEFAULT_SFZ_PATH))

    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, splitter.join(LADSPA_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI,   splitter.join(DSSI_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,    splitter.join(LV2_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST2,   splitter.join(VST2_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3,   splitter.join(VST3_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_GIG,    splitter.join(GIG_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2,    splitter.join(SF2_PATH))
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ,    splitter.join(SFZ_PATH))

    # --------------------------------------------------------------------------------------------------------
    # don't continue if plugin

    if host.isPlugin:
        return "Plugin"

    # --------------------------------------------------------------------------------------------------------
    # driver and device settings

    # driver name
    try:
        audioDriver = settings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, type=str)
    except:
        audioDriver = CARLA_DEFAULT_AUDIO_DRIVER

    # driver options
    try:
        audioDevice = settings.value("%s%s/Device" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), "", type=str)
    except:
        audioDevice = ""

    try:
        audioNumPeriods = settings.value("%s%s/NumPeriods" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_NUM_PERIODS, type=int)
    except:
        audioNumPeriods = CARLA_DEFAULT_AUDIO_NUM_PERIODS

    try:
        audioBufferSize = settings.value("%s%s/BufferSize" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_BUFFER_SIZE, type=int)
    except:
        audioBufferSize = CARLA_DEFAULT_AUDIO_BUFFER_SIZE

    try:
        audioSampleRate = settings.value("%s%s/SampleRate" % (CARLA_KEY_ENGINE_DRIVER_PREFIX, audioDriver), CARLA_DEFAULT_AUDIO_SAMPLE_RATE, type=int)
    except:
        audioSampleRate = CARLA_DEFAULT_AUDIO_SAMPLE_RATE

    host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE,      0,               audioDevice)
    host.set_engine_option(ENGINE_OPTION_AUDIO_NUM_PERIODS, audioNumPeriods, "")
    host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, audioBufferSize, "")
    host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, audioSampleRate, "")

    # --------------------------------------------------------------------------------------------------------
    # fix things if needed

    if audioDriver != "JACK" and host.transportMode == ENGINE_TRANSPORT_MODE_JACK:
        host.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL
        host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_INTERNAL, "")

    # --------------------------------------------------------------------------------------------------------
    # return selected driver name

    return audioDriver

# ------------------------------------------------------------------------------------------------------------
# Run Carla without showing UI

def runHostWithoutUI(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    # --------------------------------------------------------------------------------------------------------
    # Some initial checks

    if not gCarla.nogui:
        return

    projectFile = getInitialProjectFile(QCoreApplication.instance(), True)

    if not projectFile:
        print("Carla no-gui mode can only be used together with a project file.")
        sys.exit(1)

    # --------------------------------------------------------------------------------------------------------
    # Additional imports

    from time import sleep

    # --------------------------------------------------------------------------------------------------------
    # Init engine

    audioDriver = setEngineSettings(host)
    if not host.engine_init(audioDriver, "Carla-nogui"):
        print("Engine failed to initialize, possible reasons:\n%s" % host.get_last_error())
        sys.exit(1)

    if not host.load_project(projectFile):
        print("Failed to load selected project file, possible reasons:\n%s" % host.get_last_error())
        host.engine_close()
        sys.exit(1)

    # --------------------------------------------------------------------------------------------------------
    # Idle

    while host.is_engine_running() and not gCarla.term:
        host.engine_idle()
        sleep(0.5)

    # --------------------------------------------------------------------------------------------------------
    # Stop

    host.engine_close()
    sys.exit(0)

# ------------------------------------------------------------------------------------------------------------
