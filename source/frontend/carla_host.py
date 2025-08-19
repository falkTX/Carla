#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import json

# ------------------------------------------------------------------------------------------------------------
# Imports (ctypes)

from ctypes import (
    byref, pointer
)

# ------------------------------------------------------------------------------------------------------------
# Imports (PyQt)

from qt_compat import qt_config

if qt_config == 5:
    # This fails in some configurations, assume >= 5.6.0 in that case
    try:
        from PyQt5.Qt import PYQT_VERSION
    except ImportError:
        PYQT_VERSION = 0x50600

    from PyQt5.QtCore import (
        QT_VERSION,
        qCritical,
        QBuffer,
        QEvent,
        QEventLoop,
        QFileInfo,
        QIODevice,
        QMimeData,
        QModelIndex,
        QPointF,
        QTimer,
    )
    from PyQt5.QtGui import (
        QBrush,
        QImage,
        QImageWriter,
        QPainter,
        QPalette,
    )
    from PyQt5.QtWidgets import (
        QAction,
        QApplication,
        QInputDialog,
        QFileSystemModel,
        QListWidgetItem,
        QGraphicsView,
        QMainWindow,
    )

elif qt_config == 6:
    from PyQt6.QtCore import (
        PYQT_VERSION,
        QT_VERSION,
        qCritical,
        QBuffer,
        QEvent,
        QEventLoop,
        QFileInfo,
        QIODevice,
        QMimeData,
        QModelIndex,
        QPointF,
        QTimer,
    )
    from PyQt6.QtGui import (
        QAction,
        QBrush,
        QFileSystemModel,
        QImage,
        QImageWriter,
        QPainter,
        QPalette,
    )
    from PyQt6.QtWidgets import (
        QApplication,
        QInputDialog,
        QListWidgetItem,
        QGraphicsView,
        QMainWindow,
    )

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_host

from carla_app import *
from carla_backend import *
from carla_backend_qt import CarlaHostQtDLL, CarlaHostQtNull
from carla_frontend import CarlaFrontendLib
from carla_shared import *
from carla_settings import *
from carla_utils import *
from carla_widgets import *

from patchcanvas import patchcanvas
from widgets.digitalpeakmeter import DigitalPeakMeter
from widgets.pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------------------
# Try Import OpenGL

try:
    if qt_config == 5:
        from PyQt5.QtOpenGL import QGLWidget
    elif qt_config == 6:
        from PyQt6.QtOpenGL import QGLWidget
    hasGL = True
except:
    hasGL = False

# ------------------------------------------------------------------------------------------------------------
# Safe exception hook, needed for PyQt5

def sys_excepthook(typ, value, tback):
    return sys.__excepthook__(typ, value, tback)

# ------------------------------------------------------------------------------------------------------------
# Session Management support

CARLA_CLIENT_NAME = os.getenv("CARLA_CLIENT_NAME")
LADISH_APP_NAME   = os.getenv("LADISH_APP_NAME")
NSM_URL           = os.getenv("NSM_URL")

# ------------------------------------------------------------------------------------------------------------
# Small print helper

def processMode2Str(processMode):
    if processMode == ENGINE_PROCESS_MODE_SINGLE_CLIENT:
        return "Single client"
    if processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        return "Multi client"
    if processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
        return "Continuous Rack"
    if processMode == ENGINE_PROCESS_MODE_PATCHBAY:
        return "Patchbay"
    if processMode == ENGINE_PROCESS_MODE_BRIDGE:
        return "Bridge"
    return "Unknown"

# ------------------------------------------------------------------------------------------------------------
# Carla Print class

class CarlaPrint:
    def __init__(self, err):
        self.err = err

    def flush(self):
        gCarla.utils.fflush(self.err)

    def write(self, string):
        gCarla.utils.fputs(self.err, string)

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
#class HostWindow(QMainWindow, PluginEditParentMeta, metaclass=PyQtMetaClass):
    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    # CustomActions
    CUSTOM_ACTION_NONE         = 0
    CUSTOM_ACTION_APP_CLOSE    = 1
    CUSTOM_ACTION_PROJECT_LOAD = 2

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

        self.fIdleTimerNull = self.startTimer(1000) # keep application signals alive
        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fPluginCount = 0
        self.fPluginList  = []

        self.fPluginListDialog = None
        self.fFavoritePlugins = []

        self.fProjectFilename  = ""
        self.fIsProjectLoading = False
        self.fCurrentlyRemovingAllPlugins = False
        self.fHasLoadedLv2Plugins = False

        self.fRecentFileList = []

        self.fLastTransportBPM   = 0.0
        self.fLastTransportFrame = 0
        self.fLastTransportState = False
        self.fBufferSize         = 0
        self.fSampleRate         = 0.0
        self.fOscAddressTCP      = ""
        self.fOscAddressUDP      = ""

        if CARLA_OS_MAC:
            self.fMacClosingHelper = True

        # CancelableActionCallback Box
        self.fCancelableActionBox = None

        # run a custom action after engine is properly closed
        self.fCustomStopAction = self.CUSTOM_ACTION_NONE

        # first attempt of auto-start engine doesn't show an error
        self.fFirstEngineInit = True

        # to be filled with key-value pairs of current settings
        self.fSavedSettings = {}

        # true if NSM server handles our window management
        self.fWindowCloseHideGui = False

        if host.isControl:
            self.fClientName         = "Carla-Control"
            self.fSessionManagerName = "Control"
        elif host.isPlugin:
            self.fClientName         = "Carla-Plugin"
            self.fSessionManagerName = "Plugin"
        elif LADISH_APP_NAME:
            self.fClientName         = LADISH_APP_NAME
            self.fSessionManagerName = ""
        elif NSM_URL and host.nsmOK:
            self.fClientName         = "Carla.tmp"
            self.fSessionManagerName = "Non Session Manager TMP"
            self.fWindowCloseHideGui = True
        else:
            self.fClientName         = CARLA_CLIENT_NAME or "Carla"
            self.fSessionManagerName = ""

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff (patchbay)

        self.fPeaksCleared = True

        self.fExternalPatchbay = False
        self.fSelectedPlugins  = []

        self.fCanvasWidth  = 0
        self.fCanvasHeight = 0
        self.fMiniCanvasUpdateTimeout = 0

        self.fWithCanvas = withCanvas

        # ----------------------------------------------------------------------------------------------------
        # Internal stuff (logs)

        self.autoscrollOnNewLog = True
        self.lastLogSliderPos = 0

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (engine stopped)

        if self.host.isPlugin or self.host.isControl:
            self.ui.act_file_save.setVisible(False)
            self.ui.act_engine_start.setEnabled(False)
            self.ui.act_engine_start.setVisible(False)
            self.ui.act_engine_stop.setEnabled(False)
            self.ui.act_engine_stop.setVisible(False)
            self.ui.menu_Engine.setEnabled(False)
            self.ui.menu_Engine.setVisible(False)
            self.ui.menu_Engine.menuAction().setVisible(False)
            self.ui.tabWidget.removeTab(2)

            if self.host.isControl:
                self.ui.act_file_new.setVisible(False)
                self.ui.act_file_open.setVisible(False)
                self.ui.menu_Open_Recent.setVisible(False)
                self.ui.act_file_save_as.setVisible(False)
                self.ui.tabUtils.removeTab(0)
            else:
                self.ui.act_file_save_as.setText(self.tr("Export as..."))

                if not withCanvas:
                    self.ui.tabWidget.tabBar().hide()

        else:
            self.ui.act_engine_start.setEnabled(True)

            if CARLA_OS_WIN:
                self.ui.tabWidget.removeTab(2)

        if self.host.isControl:
            self.ui.act_file_refresh.setEnabled(False)
        else:
            self.ui.act_file_connect.setEnabled(False)
            self.ui.act_file_connect.setVisible(False)
            self.ui.act_file_refresh.setEnabled(False)
            self.ui.act_file_refresh.setVisible(False)

        if self.fSessionManagerName and not self.host.isPlugin:
            self.ui.act_file_new.setEnabled(False)

        self.ui.act_file_open.setEnabled(False)
        self.ui.menu_Open_Recent.setEnabled(False)
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
            self.ui.tw_miniCanvas.hide()
            self.ui.tabWidget.removeTab(1)
            if CARLA_OS_WIN:
                self.ui.tabWidget.tabBar().hide()

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (disk)

        exts = gCarla.utils.get_supported_file_extensions()

        self.fDirModel = QFileSystemModel(self)
        self.fDirModel.setRootPath(HOME)
        self.fDirModel.setNameFilters(tuple(("*." + i) for i in exts))

        self.ui.fileTreeView.setModel(self.fDirModel)
        self.ui.fileTreeView.setRootIndex(self.fDirModel.index(HOME))
        self.ui.fileTreeView.setColumnHidden(1, True)
        self.ui.fileTreeView.setColumnHidden(2, True)
        self.ui.fileTreeView.setColumnHidden(3, True)
        self.ui.fileTreeView.setHeaderHidden(True)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (transport)

        fontMetrics   = self.ui.l_transport_bbt.fontMetrics()
        minValueWidth = fontMetricsHorizontalAdvance(fontMetrics, "000|00|0000")
        minLabelWidth = fontMetricsHorizontalAdvance(fontMetrics, self.ui.label_transport_frame.text())

        labelTimeWidth = fontMetricsHorizontalAdvance(fontMetrics, self.ui.label_transport_time.text())
        labelBBTWidth  = fontMetricsHorizontalAdvance(fontMetrics, self.ui.label_transport_bbt.text())

        if minLabelWidth < labelTimeWidth:
            minLabelWidth = labelTimeWidth
        if minLabelWidth < labelBBTWidth:
            minLabelWidth = labelBBTWidth

        self.ui.label_transport_frame.setMinimumWidth(minLabelWidth + 3)
        self.ui.label_transport_time.setMinimumWidth(minLabelWidth + 3)
        self.ui.label_transport_bbt.setMinimumWidth(minLabelWidth + 3)

        self.ui.l_transport_bbt.setMinimumWidth(minValueWidth + 3)
        self.ui.l_transport_frame.setMinimumWidth(minValueWidth + 3)
        self.ui.l_transport_time.setMinimumWidth(minValueWidth + 3)

        if host.isPlugin:
            self.ui.b_transport_play.setEnabled(False)
            self.ui.b_transport_stop.setEnabled(False)
            self.ui.b_transport_backwards.setEnabled(False)
            self.ui.b_transport_forwards.setEnabled(False)
            self.ui.group_transport_controls.setEnabled(False)
            self.ui.group_transport_controls.setVisible(False)
            self.ui.cb_transport_link.setEnabled(False)
            self.ui.cb_transport_link.setVisible(False)
            self.ui.cb_transport_jack.setEnabled(False)
            self.ui.cb_transport_jack.setVisible(False)
            self.ui.dsb_transport_bpm.setEnabled(False)
            self.ui.dsb_transport_bpm.setReadOnly(True)

        self.ui.w_transport.setEnabled(False)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (rack)

        self.ui.listWidget.setHostAndParent(self.host, self)

        sb = self.ui.listWidget.verticalScrollBar()
        self.ui.rackScrollBar.setMinimum(sb.minimum())
        self.ui.rackScrollBar.setMaximum(sb.maximum())
        self.ui.rackScrollBar.setValue(sb.value())

        sb.rangeChanged.connect(self.ui.rackScrollBar.setRange)
        sb.valueChanged.connect(self.ui.rackScrollBar.setValue)
        self.ui.rackScrollBar.rangeChanged.connect(sb.setRange)
        self.ui.rackScrollBar.valueChanged.connect(sb.setValue)

        self.updateStyle()

        self.ui.rack.setStyleSheet("""
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
        self.ui.tw_miniCanvas.tabBar().hide()

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (logs)

        self.ui.text_logs.textChanged.connect(self.slot_logButtonsState)
        self.ui.logs_clear.clicked.connect(self.slot_logClear)
        self.ui.logs_save.clicked.connect(self.slot_logSave)
        self.ui.logs_autoscroll.stateChanged.connect(self.slot_toggleLogAutoscroll)
        self.ui.text_logs.verticalScrollBar().valueChanged.connect(self.slot_logSliderMoved)

        # ----------------------------------------------------------------------------------------------------
        # Set up GUI (special stuff for Mac OS)

        if CARLA_OS_MAC:
            self.ui.act_file_quit.setMenuRole(QAction.QuitRole)
            self.ui.act_settings_configure.setMenuRole(QAction.PreferencesRole)
            self.ui.act_help_about.setMenuRole(QAction.AboutRole)
            self.ui.act_help_about_qt.setMenuRole(QAction.AboutQtRole)
            self.ui.menu_Settings.setTitle("Panels")

        # ----------------------------------------------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)
        self.updateRecentFiles()

        # ----------------------------------------------------------------------------------------------------
        # Set-up Canvas

        if withCanvas:
            self.scene = patchcanvas.PatchScene(self, self.ui.graphicsView)
            self.ui.graphicsView.setScene(self.scene)

            if self.fSavedSettings[CARLA_KEY_CANVAS_USE_OPENGL] and hasGL:
                self.ui.glView = QGLWidget(self)
                self.ui.graphicsView.setViewport(self.ui.glView)

            self.setupCanvas()

        # ----------------------------------------------------------------------------------------------------
        # Set-up Icons

        if self.fSavedSettings[CARLA_KEY_MAIN_SYSTEM_ICONS]:
            self.ui.act_file_connect.setIcon(getIcon('network-connect', 16, 'svgz'))
            self.ui.act_file_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
            self.ui.act_file_new.setIcon(getIcon('document-new', 16, 'svgz'))
            self.ui.act_file_open.setIcon(getIcon('document-open', 16, 'svgz'))
            self.ui.menu_Open_Recent.setIcon(getIcon('document-open', 16, 'svgz'))
            self.ui.act_file_save.setIcon(getIcon('document-save', 16, 'svgz'))
            self.ui.act_file_save_as.setIcon(getIcon('document-save-as', 16, 'svgz'))
            self.ui.act_file_quit.setIcon(getIcon('application-exit', 16, 'svgz'))
            self.ui.act_engine_start.setIcon(getIcon('media-playback-start', 16, 'svgz'))
            self.ui.act_engine_stop.setIcon(getIcon('media-playback-stop', 16, 'svgz'))
            self.ui.act_engine_panic.setIcon(getIcon('dialog-warning', 16, 'svgz'))
            self.ui.act_engine_config.setIcon(getIcon('configure', 16, 'svgz'))
            self.ui.act_plugin_add.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.act_plugin_add_jack.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.act_plugin_remove_all.setIcon(getIcon('edit-delete', 16, 'svgz'))
            self.ui.act_canvas_arrange.setIcon(getIcon('view-sort-ascending', 16, 'svgz'))
            self.ui.act_canvas_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
            self.ui.act_canvas_zoom_fit.setIcon(getIcon('zoom-fit-best', 16, 'svgz'))
            self.ui.act_canvas_zoom_in.setIcon(getIcon('zoom-in', 16, 'svgz'))
            self.ui.act_canvas_zoom_out.setIcon(getIcon('zoom-out', 16, 'svgz'))
            self.ui.act_canvas_zoom_100.setIcon(getIcon('zoom-original', 16, 'svgz'))
            self.ui.act_settings_configure.setIcon(getIcon('configure', 16, 'svgz'))
            self.ui.b_disk_add.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.b_disk_remove.setIcon(getIcon('list-remove', 16, 'svgz'))
            self.ui.b_transport_play.setIcon(getIcon('media-playback-start', 16, 'svgz'))
            self.ui.b_transport_stop.setIcon(getIcon('media-playback-stop', 16, 'svgz'))
            self.ui.b_transport_backwards.setIcon(getIcon('media-seek-backward', 16, 'svgz'))
            self.ui.b_transport_forwards.setIcon(getIcon('media-seek-forward', 16, 'svgz'))
            self.ui.logs_clear.setIcon(getIcon('edit-clear', 16, 'svgz'))
            self.ui.logs_save.setIcon(getIcon('document-save', 16, 'svgz'))

        # ----------------------------------------------------------------------------------------------------
        # Connect actions to functions

        self.ui.act_file_new.triggered.connect(self.slot_fileNew)
        self.ui.act_file_open.triggered.connect(self.slot_fileOpen)
        self.ui.act_file_save.triggered.connect(self.slot_fileSave)
        self.ui.act_file_save_as.triggered.connect(self.slot_fileSaveAs)

        self.ui.act_engine_start.triggered.connect(self.slot_engineStart)
        self.ui.act_engine_stop.triggered.connect(self.slot_engineStop)
        self.ui.act_engine_panic.triggered.connect(self.slot_pluginsDisable)
        self.ui.act_engine_config.triggered.connect(self.slot_engineConfig)

        self.ui.act_plugin_add.triggered.connect(self.slot_pluginAdd)
        self.ui.act_plugin_add_jack.triggered.connect(self.slot_jackAppAdd)
        self.ui.act_plugin_remove_all.triggered.connect(self.slot_confirmRemoveAll)

        self.ui.act_plugins_enable.triggered.connect(self.slot_pluginsEnable)
        self.ui.act_plugins_disable.triggered.connect(self.slot_pluginsDisable)
        self.ui.act_plugins_volume100.triggered.connect(self.slot_pluginsVolume100)
        self.ui.act_plugins_mute.triggered.connect(self.slot_pluginsMute)
        self.ui.act_plugins_wet100.triggered.connect(self.slot_pluginsWet100)
        self.ui.act_plugins_bypass.triggered.connect(self.slot_pluginsBypass)
        self.ui.act_plugins_center.triggered.connect(self.slot_pluginsCenter)
        self.ui.act_plugins_compact.triggered.connect(self.slot_pluginsCompact)
        self.ui.act_plugins_expand.triggered.connect(self.slot_pluginsExpand)

        self.ui.act_settings_show_toolbar.toggled.connect(self.slot_showToolbar)
        self.ui.act_settings_show_meters.toggled.connect(self.slot_showCanvasMeters)
        self.ui.act_settings_show_keyboard.toggled.connect(self.slot_showCanvasKeyboard)
        self.ui.act_settings_show_side_panel.toggled.connect(self.slot_showSidePanel)
        self.ui.act_settings_configure.triggered.connect(self.slot_configureCarla)

        self.ui.act_help_about.triggered.connect(self.slot_aboutCarla)
        self.ui.act_help_about_qt.triggered.connect(self.slot_aboutQt)

        self.ui.cb_disk.currentIndexChanged.connect(self.slot_diskFolderChanged)
        self.ui.b_disk_add.clicked.connect(self.slot_diskFolderAdd)
        self.ui.b_disk_remove.clicked.connect(self.slot_diskFolderRemove)
        self.ui.fileTreeView.doubleClicked.connect(self.slot_fileTreeDoubleClicked)

        self.ui.b_transport_play.clicked.connect(self.slot_transportPlayPause)
        self.ui.b_transport_stop.clicked.connect(self.slot_transportStop)
        self.ui.b_transport_backwards.clicked.connect(self.slot_transportBackwards)
        self.ui.b_transport_forwards.clicked.connect(self.slot_transportForwards)
        self.ui.dsb_transport_bpm.valueChanged.connect(self.slot_transportBpmChanged)
        self.ui.cb_transport_jack.clicked.connect(self.slot_transportJackEnabled)
        self.ui.cb_transport_link.clicked.connect(self.slot_transportLinkEnabled)

        self.ui.b_xruns.clicked.connect(self.slot_xrunClear)

        self.ui.listWidget.customContextMenuRequested.connect(self.slot_showPluginActionsMenu)

        self.ui.keyboard.noteOn.connect(self.slot_noteOn)
        self.ui.keyboard.noteOff.connect(self.slot_noteOff)

        self.ui.tabWidget.currentChanged.connect(self.slot_tabChanged)
        self.ui.toolBar.visibilityChanged.connect(self.slot_toolbarVisibilityChanged)

        if withCanvas:
            self.ui.act_canvas_show_internal.triggered.connect(self.slot_canvasShowInternal)
            self.ui.act_canvas_show_external.triggered.connect(self.slot_canvasShowExternal)
            self.ui.act_canvas_arrange.triggered.connect(self.slot_canvasArrange)
            self.ui.act_canvas_refresh.triggered.connect(self.slot_canvasRefresh)
            self.ui.act_canvas_zoom_fit.triggered.connect(self.slot_canvasZoomFit)
            self.ui.act_canvas_zoom_in.triggered.connect(self.slot_canvasZoomIn)
            self.ui.act_canvas_zoom_out.triggered.connect(self.slot_canvasZoomOut)
            self.ui.act_canvas_zoom_100.triggered.connect(self.slot_canvasZoomReset)
            self.ui.act_canvas_save_image.triggered.connect(self.slot_canvasSaveImage)
            self.ui.act_canvas_save_image_2x.triggered.connect(self.slot_canvasSaveImage)
            self.ui.act_canvas_save_image_4x.triggered.connect(self.slot_canvasSaveImage)
            self.ui.act_canvas_copy_clipboard.triggered.connect(self.slot_canvasCopyToClipboard)
            self.ui.act_canvas_arrange.setEnabled(False) # TODO, later
            self.ui.graphicsView.horizontalScrollBar().valueChanged.connect(self.slot_horizontalScrollBarChanged)
            self.ui.graphicsView.verticalScrollBar().valueChanged.connect(self.slot_verticalScrollBarChanged)
            self.ui.miniCanvasPreview.miniCanvasMoved.connect(self.slot_miniCanvasMoved)
            self.scene.scaleChanged.connect(self.slot_canvasScaleChanged)
            self.scene.pluginSelected.connect(self.slot_canvasPluginSelected)
            self.scene.selectionChanged.connect(self.slot_canvasSelectionChanged)

        self.SIGUSR1.connect(self.slot_handleSIGUSR1)
        self.SIGTERM.connect(self.slot_handleSIGTERM)

        host.EngineStartedCallback.connect(self.slot_handleEngineStartedCallback)
        host.EngineStoppedCallback.connect(self.slot_handleEngineStoppedCallback)
        host.TransportModeChangedCallback.connect(self.slot_handleTransportModeChangedCallback)
        host.BufferSizeChangedCallback.connect(self.slot_handleBufferSizeChangedCallback)
        host.SampleRateChangedCallback.connect(self.slot_handleSampleRateChangedCallback)
        host.CancelableActionCallback.connect(self.slot_handleCancelableActionCallback)
        host.ProjectLoadFinishedCallback.connect(self.slot_handleProjectLoadFinishedCallback)

        host.PluginAddedCallback.connect(self.slot_handlePluginAddedCallback)
        host.PluginRemovedCallback.connect(self.slot_handlePluginRemovedCallback)
        host.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

        host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)

        host.UpdateCallback.connect(self.slot_handleUpdateCallback)

        if withCanvas:
            host.PatchbayClientAddedCallback.connect(self.slot_handlePatchbayClientAddedCallback)
            host.PatchbayClientRemovedCallback.connect(self.slot_handlePatchbayClientRemovedCallback)
            host.PatchbayClientRenamedCallback.connect(self.slot_handlePatchbayClientRenamedCallback)
            host.PatchbayClientDataChangedCallback.connect(self.slot_handlePatchbayClientDataChangedCallback)
            host.PatchbayClientPositionChangedCallback.connect(self.slot_handlePatchbayClientPositionChangedCallback)
            host.PatchbayPortAddedCallback.connect(self.slot_handlePatchbayPortAddedCallback)
            host.PatchbayPortRemovedCallback.connect(self.slot_handlePatchbayPortRemovedCallback)
            host.PatchbayPortChangedCallback.connect(self.slot_handlePatchbayPortChangedCallback)
            host.PatchbayPortGroupAddedCallback.connect(self.slot_handlePatchbayPortGroupAddedCallback)
            host.PatchbayPortGroupRemovedCallback.connect(self.slot_handlePatchbayPortGroupRemovedCallback)
            host.PatchbayPortGroupChangedCallback.connect(self.slot_handlePatchbayPortGroupChangedCallback)
            host.PatchbayConnectionAddedCallback.connect(self.slot_handlePatchbayConnectionAddedCallback)
            host.PatchbayConnectionRemovedCallback.connect(self.slot_handlePatchbayConnectionRemovedCallback)

        host.NSMCallback.connect(self.slot_handleNSMCallback)

        host.DebugCallback.connect(self.slot_handleDebugCallback)
        host.InfoCallback.connect(self.slot_handleInfoCallback)
        host.ErrorCallback.connect(self.slot_handleErrorCallback)
        host.QuitCallback.connect(self.slot_handleQuitCallback)
        host.InlineDisplayRedrawCallback.connect(self.slot_handleInlineDisplayRedrawCallback)

        # ----------------------------------------------------------------------------------------------------
        # Final setup

        self.ui.text_logs.clear()
        self.slot_logButtonsState(False)
        self.setProperWindowTitle()

        # Disable non-supported features
        features = gCarla.utils.get_supported_features()

        if "link" not in features:
            self.ui.cb_transport_link.setEnabled(False)
            self.ui.cb_transport_link.setVisible(False)

        # Plugin needs to have timers always running so it receives messages
        if self.host.isPlugin or self.host.isRemote:
            self.startTimers()

        # Qt needs this so it properly creates & resizes the canvas
        self.ui.tabWidget.blockSignals(True)
        self.ui.tabWidget.setCurrentIndex(1)
        self.ui.tabWidget.setCurrentIndex(0)
        self.ui.tabWidget.blockSignals(False)

        # Start in patchbay tab if using forced patchbay mode
        if host.processModeForced and host.processMode == ENGINE_PROCESS_MODE_PATCHBAY:
            self.ui.tabWidget.setCurrentIndex(1)

        # Load initial project file if set
        if not (self.host.isControl or self.host.isPlugin):
            projectFile = getInitialProjectFile(QApplication.instance())

            if projectFile:
                self.loadProjectLater(projectFile)

        # For NSM we wait for the open message
        if NSM_URL and host.nsmOK:
            host.nsm_ready(NSM_CALLBACK_INIT)
            return

        if not host.isControl:
            QTimer.singleShot(0, self.slot_engineStart)

    # --------------------------------------------------------------------------------------------------------
    # Manage visibility state, needed for NSM

    def hideForNSM(self):
        for pitem in reversed(self.fPluginList):
            if pitem is None:
                continue
            pitem.getWidget().hideCustomUI()
        self.hide()

    def showIfNeeded(self):
        if self.host.nsmOK:
            self.ui.act_file_quit.setText(self.tr("Hide"))
            QApplication.instance().setQuitOnLastWindowClosed(False)
        else:
            self.show()

    # --------------------------------------------------------------------------------------------------------
    # Setup

    def compactPlugin(self, pluginId):
        if pluginId > self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]

        if pitem is None:
            return

        pitem.recreateWidget(True)

    def changePluginColor(self, pluginId, color, colorStr):
        if pluginId > self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]

        if pitem is None:
            return

        self.host.set_custom_data(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaColor", colorStr)
        pitem.recreateWidget(newColor = color)

    def changePluginSkin(self, pluginId, skin):
        if pluginId > self.fPluginCount:
            return

        pitem = self.fPluginList[pluginId]

        if pitem is None:
            return

        self.host.set_custom_data(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaSkin", skin)
        if skin not in ("default","rncbc","presets","mpresets"):
            pitem.recreateWidget(newSkin = skin, newColor = (255,255,255))
        else:
            pitem.recreateWidget(newSkin = skin)

    def findPluginInPatchbay(self, pluginId):
        if pluginId > self.fPluginCount:
            return

        if self.fExternalPatchbay:
            self.slot_canvasShowInternal()

        if not patchcanvas.focusGroupUsingPluginId(pluginId):
            name = self.host.get_plugin_info(pluginId)['name']
            if not patchcanvas.focusGroupUsingGroupName(name):
                return

        self.ui.tabWidget.setCurrentIndex(1)

    def switchPlugins(self, pluginIdA, pluginIdB):
        if pluginIdA == pluginIdB:
            return
        if pluginIdA < 0 or pluginIdB < 0:
            return
        if pluginIdA >= self.fPluginCount or pluginIdB >= self.fPluginCount:
            return

        self.host.switch_plugins(pluginIdA, pluginIdB)

        itemA = self.fPluginList[pluginIdA]
        compactA = itemA.isCompacted()
        guiShownA = itemA.isGuiShown()

        itemB = self.fPluginList[pluginIdB]
        compactB = itemB.isCompacted()
        guiShownB = itemB.isGuiShown()

        itemA.setPluginId(pluginIdA)
        itemA.recreateWidget2(compactB, guiShownB)

        itemB.setPluginId(pluginIdB)
        itemB.recreateWidget2(compactA, guiShownA)

        if self.fWithCanvas:
            self.slot_canvasRefresh()

    def setLoadRDFsNeeded(self):
        self.fLadspaRdfNeedsUpdate = True

    def setProperWindowTitle(self):
        title = self.fClientName

        if self.fProjectFilename and not self.host.nsmOK:
            title += " - %s" % os.path.basename(self.fProjectFilename)
        if self.fSessionManagerName:
            title += " (%s)" % self.fSessionManagerName

        self.setWindowTitle(title)

    def updateBufferSize(self, newBufferSize):
        if self.fBufferSize == newBufferSize:
            return
        self.fBufferSize = newBufferSize
        self.ui.cb_buffer_size.clear()
        self.ui.cb_buffer_size.addItem(str(newBufferSize))
        self.ui.cb_buffer_size.setCurrentIndex(0)

    def updateSampleRate(self, newSampleRate):
        if self.fSampleRate == newSampleRate:
            return
        self.fSampleRate = newSampleRate
        self.ui.cb_sample_rate.clear()
        self.ui.cb_sample_rate.addItem(str(newSampleRate))
        self.ui.cb_sample_rate.setCurrentIndex(0)
        self.refreshTransport(True)

    # --------------------------------------------------------------------------------------------------------
    # Files

    def loadProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: loading project without filename set")
        if self.host.nsmOK and not os.path.exists(self.fProjectFilename):
            return

        self.projectLoadingStarted()
        self.fIsProjectLoading = True

        if not self.host.load_project(self.fProjectFilename):
            self.fIsProjectLoading = False
            self.projectLoadingFinished(True)

            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load project"),
                             self.host.get_last_error(),
                             QMessageBox.Ok, QMessageBox.Ok)

    def loadProjectLater(self, filename):
        self.fProjectFilename = QFileInfo(filename).absoluteFilePath()
        self.setProperWindowTitle()
        QTimer.singleShot(1, self.slot_loadProjectNow)

    def saveProjectNow(self):
        if not self.fProjectFilename:
            return qCritical("ERROR: saving project without filename set")

        if not self.host.save_project(self.fProjectFilename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to save project"),
                             self.host.get_last_error(),
                             QMessageBox.Ok, QMessageBox.Ok)
            return

    def projectLoadingStarted(self):
        self.ui.rack.setEnabled(False)
        self.ui.graphicsView.setEnabled(False)

    def projectLoadingFinished(self, refreshCanvas):
        self.ui.rack.setEnabled(True)
        self.ui.graphicsView.setEnabled(True)

        if self.fCustomStopAction == self.CUSTOM_ACTION_APP_CLOSE or not self.fWithCanvas:
            return

        if refreshCanvas and not self.loadExternalCanvasGroupPositionsIfNeeded(self.fProjectFilename):
            QTimer.singleShot(1, self.slot_canvasRefresh)

    def loadExternalCanvasGroupPositionsIfNeeded(self, filename):
        extrafile = filename.rsplit(".",1)[0]+".json"
        if not os.path.exists(extrafile):
            return False

        with open(filename, "r") as fh:
            if "".join(fh.readlines(90)).find("<CARLA-PROJECT VERSION='2.0'>") < 0:
                return False

        with open(extrafile, "r") as fh:
            try:
                canvasdata = json.load(fh)['canvas']
            except:
                return False

        print("NOTICE: loading old-style canvas group positions via legacy json file")
        patchcanvas.restoreGroupPositions(canvasdata)
        QTimer.singleShot(1, self.slot_canvasRefresh)
        return True

    # --------------------------------------------------------------------------------------------------------
    # Files (menu actions)

    @pyqtSlot()
    def slot_fileNew(self):
        if self.fPluginCount > 0 and QMessageBox.question(self, self.tr("New File"),
                                                                self.tr("Plugins that are currently loaded will be removed. Are you sure?"),
                                                                QMessageBox.Yes|QMessageBox.No) == QMessageBox.No:
            return

        self.pluginRemoveAll()
        self.fProjectFilename = ""
        self.setProperWindowTitle()
        self.host.clear_project_filename()

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter = self.tr("Carla Project File (*.carxp);;Carla Preset File (*.carxs)")
        filename, ok = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        # FIXME use ok value, test if it works as expected
        if not filename:
            return

        newFile = True

        if self.fPluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Question"), self.tr("There are some plugins loaded, do you want to remove them now?"),
                                                                          QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            newFile = (ask == QMessageBox.Yes)

        if newFile:
            self.pluginRemoveAll()
            self.fProjectFilename = filename
            self.setProperWindowTitle()
            self.loadProjectNow()
        else:
            filenameOld = self.fProjectFilename
            self.fProjectFilename = filename
            self.loadProjectNow()
            self.fProjectFilename = filenameOld

        self.addToRecentFilesList(self.fProjectFilename)
    
    def addToRecentFilesList(self, filePath):
        # FIXME: move this constant elsewhere? or add it to global settings?
        MAX_RECENT_FILES = 4
        settings = QSafeSettings()
        recentFileList = settings.value("RecentFileList", [], list)

        # TODO: reorder the list so last used are listed first?
        if filePath not in recentFileList:
            recentFileList.insert(0, filePath)
            if len(recentFileList) > MAX_RECENT_FILES:
                recentFileList.pop()

        settings.setValue("RecentFileList", recentFileList)
        del settings
        self.updateRecentFiles()

    def updateRecentFiles(self):
        settings = QSafeSettings()
        recentFileList = settings.value("RecentFileList", [], list)
        del settings

        menu = self.ui.menu_Open_Recent
        menu.clear()

        for f in recentFileList:
            act = menu.addAction(f)
            act.triggered.connect(self.slot_fileMenuOpenRecent)
        act = menu.addSeparator()
        act = menu.addAction("Clear")
        act.triggered.connect(self.slot_clearRecentFiles)

    @pyqtSlot()
    def slot_clearRecentFiles(self):
        menu = self.ui.menu_Open_Recent
        menu.clear()
        settings = QSafeSettings()
        settings.setValue("RecentFileList", [])
        del settings

    @pyqtSlot()
    def slot_fileMenuOpenRecent(self):
        sender = self.sender()
        filename = sender.text()

        newFile = True

        if self.fPluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Question"), self.tr("There are some plugins loaded, do you want to remove them now?"),
                                                                          QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            newFile = (ask == QMessageBox.Yes)

        if newFile:
            self.pluginRemoveAll()
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
        filename, ok = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], filter=fileFilter)

        # FIXME use ok value, test if it works as expected
        if not filename:
            return

        if not filename.lower().endswith(".carxp"):
            filename += ".carxp"

        if self.fProjectFilename != filename:
            self.fProjectFilename = filename
            self.setProperWindowTitle()

        self.addToRecentFilesList(filename)
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
        audioDriver = setEngineSettings(self.host, self.fSavedSettings)

        firstInit = self.fFirstEngineInit
        self.fFirstEngineInit = False

        self.ui.text_logs.appendPlainText("======= Starting engine =======")

        if self.host.engine_init(audioDriver, self.fClientName):
            if firstInit and not (self.host.isControl or self.host.isPlugin):
                settings = QSafeSettings()
                lastBpm  = settings.value("LastBPM", 120.0, float)
                del settings
                if lastBpm >= 20.0:
                    self.host.transport_bpm(lastBpm)
            return

        elif firstInit:
            self.ui.text_logs.appendPlainText("Failed to start engine on first try, ignored")
            return

        audioError = self.host.get_last_error()

        if audioError:
            QMessageBox.critical(self, self.tr("Error"),
                                 self.tr("Could not connect to Audio backend '%s', possible reasons:\n%s" % (audioDriver, audioError)))
        else:
            QMessageBox.critical(self, self.tr("Error"),
                                 self.tr("Could not connect to Audio backend '%s'" % audioDriver))

    @pyqtSlot()
    def slot_engineStop(self, forced = False):
        self.ui.text_logs.appendPlainText("======= Stopping engine =======")

        if self.fPluginCount == 0 or not self.host.is_engine_running():
            self.engineStopFinal()
            return True

        if not forced:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask != QMessageBox.Yes:
                return False

        return self.slot_engineStopTryAgain()

    @pyqtSlot()
    def slot_engineConfig(self):
        engineRunning = self.host.is_engine_running()

        if engineRunning:
            dialog = RuntimeDriverSettingsW(self.fParentOrSelf, self.host)

        else:
            if self.host.isPlugin:
                driverName = "Plugin"
                driverIndex = 0
            elif self.host.audioDriverForced:
                driverName = self.host.audioDriverForced
                driverIndex = 0
            else:
                settings = QSafeSettings()
                driverName = settings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, str)
                for i in range(self.host.get_engine_driver_count()):
                    if self.host.get_engine_driver_name(i) == driverName:
                        driverIndex = i
                        break
                else:
                    driverIndex = -1
                del settings
            dialog = DriverSettingsW(self.fParentOrSelf, self.host, driverIndex, driverName)
            dialog.ui.ico_restart.hide()
            dialog.ui.label_restart.hide()
            dialog.adjustSize()

        if not dialog.exec_():
            return

        audioDevice, bufferSize, sampleRate = dialog.getValues()

        if engineRunning:
            self.host.set_engine_buffer_size_and_sample_rate(bufferSize, sampleRate)
        else:
            self.host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE, 0, audioDevice)
            self.host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, bufferSize, "")
            self.host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, sampleRate, "")

    @pyqtSlot()
    def slot_engineStopTryAgain(self):
        if self.host.is_engine_running() and not self.host.set_engine_about_to_close():
            QTimer.singleShot(0, self.slot_engineStopTryAgain)
            return False

        self.engineStopFinal()
        return True

    def engineStopFinal(self):
        patchcanvas.handleAllPluginsRemoved()
        self.killTimers()

        if self.fCustomStopAction == self.CUSTOM_ACTION_PROJECT_LOAD:
            self.removeAllPlugins()
        else:
            self.fProjectFilename = ""
            self.setProperWindowTitle()
            if self.fPluginCount != 0:
                self.fCurrentlyRemovingAllPlugins = True
                self.projectLoadingStarted()

        if self.host.is_engine_running() and not self.host.remove_all_plugins():
            self.ui.text_logs.appendPlainText("Failed to remove all plugins, error was:")
            self.ui.text_logs.appendPlainText(self.host.get_last_error())

        if not self.host.engine_close():
            self.ui.text_logs.appendPlainText("Failed to stop engine, error was:")
            self.ui.text_logs.appendPlainText(self.host.get_last_error())

        if self.fCustomStopAction == self.CUSTOM_ACTION_APP_CLOSE:
            self.close()
        elif self.fCustomStopAction == self.CUSTOM_ACTION_PROJECT_LOAD:
            self.slot_engineStart()
            self.loadProjectNow()
            self.host.nsm_ready(NSM_CALLBACK_OPEN)

        self.fCustomStopAction = self.CUSTOM_ACTION_NONE

    # --------------------------------------------------------------------------------------------------------
    # Engine (host callbacks)

    @pyqtSlot(int, int, int, int, float, str)
    def slot_handleEngineStartedCallback(self, pluginCount, processMode, transportMode, bufferSize, sampleRate, driverName):
        self.ui.menu_PluginMacros.setEnabled(True)
        self.ui.menu_Canvas.setEnabled(True)
        self.ui.w_transport.setEnabled(True)

        self.ui.act_canvas_show_internal.blockSignals(True)
        self.ui.act_canvas_show_external.blockSignals(True)

        if processMode == ENGINE_PROCESS_MODE_PATCHBAY and not self.host.isPlugin:
            self.ui.act_canvas_show_internal.setChecked(True)
            self.ui.act_canvas_show_internal.setVisible(True)
            self.ui.act_canvas_show_external.setChecked(False)
            self.ui.act_canvas_show_external.setVisible(True)
            self.fExternalPatchbay = False
        else:
            self.ui.act_canvas_show_internal.setChecked(False)
            self.ui.act_canvas_show_internal.setVisible(False)
            self.ui.act_canvas_show_external.setChecked(True)
            self.ui.act_canvas_show_external.setVisible(False)
            self.fExternalPatchbay = not self.host.isPlugin

        self.ui.act_canvas_show_internal.blockSignals(False)
        self.ui.act_canvas_show_external.blockSignals(False)

        if not (self.host.isControl or self.host.isPlugin):
            canSave = (self.fProjectFilename and os.path.exists(self.fProjectFilename)) or not self.fSessionManagerName
            self.ui.act_file_save.setEnabled(canSave)
            self.ui.act_engine_start.setEnabled(False)
            self.ui.act_engine_stop.setEnabled(True)

        if not self.host.isPlugin:
            self.enableTransport(transportMode != ENGINE_TRANSPORT_MODE_DISABLED)

        if self.host.isPlugin or not self.fSessionManagerName:
            self.ui.act_file_open.setEnabled(True)
            self.ui.menu_Open_Recent.setEnabled(True)
            self.ui.act_file_save_as.setEnabled(True)

        self.ui.cb_transport_jack.setChecked(transportMode == ENGINE_TRANSPORT_MODE_JACK)
        self.ui.cb_transport_jack.setEnabled(driverName == "JACK" and processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)

        if self.ui.cb_transport_link.isEnabled():
            self.ui.cb_transport_link.setChecked(":link:" in self.host.transportExtra)

        self.updateBufferSize(bufferSize)
        self.updateSampleRate(int(sampleRate))
        self.refreshRuntimeInfo(0.0, 0)
        self.startTimers()

        self.ui.text_logs.appendPlainText("======= Engine started ========")
        self.ui.text_logs.appendPlainText("Carla engine started, details:")
        self.ui.text_logs.appendPlainText("  Driver name:  %s" % driverName)
        self.ui.text_logs.appendPlainText("  Sample rate:  %i" % int(sampleRate))
        self.ui.text_logs.appendPlainText("  Process mode: %s" % processMode2Str(processMode))

    @pyqtSlot()
    def slot_handleEngineStoppedCallback(self):
        self.ui.text_logs.appendPlainText("======= Engine stopped ========")

        if self.fWithCanvas:
            patchcanvas.clear()

        self.killTimers()

        # just in case
        self.removeAllPlugins()
        self.refreshRuntimeInfo(0.0, 0)

        self.ui.menu_PluginMacros.setEnabled(False)
        self.ui.menu_Canvas.setEnabled(False)
        self.ui.w_transport.setEnabled(False)

        if not (self.host.isControl or self.host.isPlugin):
            self.ui.act_file_save.setEnabled(False)
            self.ui.act_engine_start.setEnabled(True)
            self.ui.act_engine_stop.setEnabled(False)

        if self.host.isPlugin or not self.fSessionManagerName:
            self.ui.act_file_open.setEnabled(False)
            self.ui.act_file_open_recent.setEnabled(False)
            self.ui.act_file_save_as.setEnabled(False)

    @pyqtSlot(int, str)
    def slot_handleTransportModeChangedCallback(self, transportMode, transportExtra):
        self.enableTransport(transportMode != ENGINE_TRANSPORT_MODE_DISABLED)

        self.ui.cb_transport_jack.setChecked(transportMode == ENGINE_TRANSPORT_MODE_JACK)
        self.ui.cb_transport_link.setChecked(":link:" in transportExtra)

    @pyqtSlot(int)
    def slot_handleBufferSizeChangedCallback(self, newBufferSize):
        self.updateBufferSize(newBufferSize)

    @pyqtSlot(float)
    def slot_handleSampleRateChangedCallback(self, newSampleRate):
        self.updateSampleRate(int(newSampleRate))

    @pyqtSlot(int, bool, str)
    def slot_handleCancelableActionCallback(self, pluginId, started, action):
        if self.fCancelableActionBox is not None:
            self.fCancelableActionBox.close()

        if started:
            self.fCancelableActionBox = QMessageBox(self)
            self.fCancelableActionBox.setIcon(QMessageBox.Information)
            self.fCancelableActionBox.setWindowTitle(self.tr("Action in progress"))
            self.fCancelableActionBox.setText(action)
            self.fCancelableActionBox.setInformativeText(self.tr("An action is in progress, please wait..."))
            self.fCancelableActionBox.setStandardButtons(QMessageBox.Cancel)
            self.fCancelableActionBox.setDefaultButton(QMessageBox.Cancel)
            self.fCancelableActionBox.buttonClicked.connect(self.slot_canlableActionBoxClicked)
            self.fCancelableActionBox.show()

        else:
            self.fCancelableActionBox = None

    @pyqtSlot()
    def slot_canlableActionBoxClicked(self):
        self.host.cancel_engine_action()

    @pyqtSlot()
    def slot_handleProjectLoadFinishedCallback(self):
        self.fIsProjectLoading = False
        self.projectLoadingFinished(False)

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

    def showAddPluginDialog(self):
        # TODO self.fHasLoadedLv2Plugins
        if self.fPluginListDialog is None:
            hostSettings = {
                'showPluginBridges': self.fSavedSettings[CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES],
                'showWineBridges': self.fSavedSettings[CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES],
                'useSystemIcons': self.fSavedSettings[CARLA_KEY_MAIN_SYSTEM_ICONS],
                'wineAutoPrefix': self.fSavedSettings[CARLA_KEY_WINE_AUTO_PREFIX],
                'wineExecutable': self.fSavedSettings[CARLA_KEY_WINE_EXECUTABLE],
                'wineFallbackPrefix': self.fSavedSettings[CARLA_KEY_WINE_FALLBACK_PREFIX],
            }
            self.fPluginListDialog = d = gCarla.felib.createPluginListDialog(self.fParentOrSelf, hostSettings)

            gCarla.felib.setPluginListDialogPath(d, PLUGIN_LADSPA, self.fSavedSettings[CARLA_KEY_PATHS_LADSPA])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_DSSI, self.fSavedSettings[CARLA_KEY_PATHS_DSSI])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_LV2, self.fSavedSettings[CARLA_KEY_PATHS_LV2])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_VST2, self.fSavedSettings[CARLA_KEY_PATHS_VST2])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_VST3, self.fSavedSettings[CARLA_KEY_PATHS_VST3])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_SF2, self.fSavedSettings[CARLA_KEY_PATHS_SF2])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_SFZ, self.fSavedSettings[CARLA_KEY_PATHS_SFZ])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_JSFX, self.fSavedSettings[CARLA_KEY_PATHS_JSFX])
            gCarla.felib.setPluginListDialogPath(d, PLUGIN_CLAP, self.fSavedSettings[CARLA_KEY_PATHS_CLAP])

        ret = gCarla.felib.execPluginListDialog(self.fPluginListDialog)

        # TODO
        #if dialog.fFavoritePluginsChanged:
            #self.fFavoritePlugins = dialog.fFavoritePlugins

        if not ret:
            return

        if not self.host.is_engine_running():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return

        btype    = ret['build']
        ptype    = ret['type']
        filename = ret['filename']
        label    = ret['label']
        uniqueId = ret['uniqueId']
        extraPtr = None

        return (btype, ptype, filename, label, uniqueId, extraPtr)

    def showAddJackAppDialog(self):
        ret = gCarla.felib.createAndExecJackAppDialog(self.fParentOrSelf, self.fProjectFilename)

        if not ret:
            return

        if not self.host.is_engine_running():
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return

        return ret

    @pyqtSlot()
    def slot_favoritePluginAdd(self):
        plugin = self.sender().data()

        if plugin is None:
            return

        if not self.host.add_plugin(plugin['build'], plugin['type'], plugin['filename'], None,
                                    plugin['label'], plugin['uniqueId'], None, PLUGIN_OPTIONS_NULL):
            # remove plugin from favorites
            try:
                self.fFavoritePlugins.remove(plugin)
            except ValueError:
                pass
            else:
                settingsDBf = QSafeSettings("falkTX", "CarlaDatabase2")
                settingsDBf.setValue("PluginDatabase/Favorites", self.fFavoritePlugins)
                settingsDBf.sync()
                del settingsDBf

            CustomMessageBox(self,
                             QMessageBox.Critical,
                             self.tr("Error"),
                             self.tr("Failed to load plugin"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot()
    def slot_showPluginActionsMenu(self):
        menu = QMenu(self)

        menu.addSection("Plugins")
        menu.addAction(self.ui.act_plugin_add)

        if len(self.fFavoritePlugins) != 0:
            fmenu = QMenu("Add from favorites", self)
            for p in self.fFavoritePlugins:
                act = fmenu.addAction(p['name'])
                act.setData(p)
                act.triggered.connect(self.slot_favoritePluginAdd)
            menu.addMenu(fmenu)

        menu.addAction(self.ui.act_plugin_remove_all)

        menu.addSection("All plugins (macros)")
        menu.addAction(self.ui.act_plugins_enable)
        menu.addAction(self.ui.act_plugins_disable)
        menu.addSeparator()
        menu.addAction(self.ui.act_plugins_volume100)
        menu.addAction(self.ui.act_plugins_mute)
        menu.addSeparator()
        menu.addAction(self.ui.act_plugins_wet100)
        menu.addAction(self.ui.act_plugins_bypass)
        menu.addSeparator()
        menu.addAction(self.ui.act_plugins_center)
        menu.addSeparator()
        menu.addAction(self.ui.act_plugins_compact)
        menu.addAction(self.ui.act_plugins_expand)

        menu.exec_(QCursor.pos())

    @pyqtSlot()
    def slot_pluginAdd(self):
        data = self.showAddPluginDialog()

        if data is None:
            return

        btype, ptype, filename, label, uniqueId, extraPtr = data

        if not self.host.add_plugin(btype, ptype, filename, None, label, uniqueId, extraPtr, PLUGIN_OPTIONS_NULL):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot()
    def slot_confirmRemoveAll(self):
        if self.fPluginCount == 0:
            return

        if QMessageBox.question(self, self.tr("Remove All"),
                                      self.tr("Are you sure you want to remove all plugins?"),
                                      QMessageBox.Yes|QMessageBox.No) == QMessageBox.No:
            return

        self.pluginRemoveAll()

    def pluginRemoveAll(self):
        if self.fPluginCount == 0:
            return

        self.fCurrentlyRemovingAllPlugins = True
        self.projectLoadingStarted()

        if not self.host.remove_all_plugins():
            self.projectLoadingFinished(True)
            self.fCurrentlyRemovingAllPlugins = False
            CustomMessageBox(self, QMessageBox.Warning, self.tr("Error"), self.tr("Operation failed"),
                                   self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    @pyqtSlot()
    def slot_jackAppAdd(self):
        data = self.showAddJackAppDialog()

        if data is None:
            return

        if not data['command']:
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Cannot add jack application"),
                             self.tr("command is empty"), QMessageBox.Ok, QMessageBox.Ok)
            return

        if not self.host.add_plugin(BINARY_NATIVE, PLUGIN_JACK,
                                    data['command'], data['name'], data['labelSetup'],
                                    0, None, PLUGIN_OPTIONS_NULL):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

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

    @pyqtSlot(int, int, str)
    def slot_handlePluginAddedCallback(self, pluginId, pluginType, pluginName):
        if pluginId != self.fPluginCount:
            print("ERROR: pluginAdded mismatch Id:", pluginId, self.fPluginCount)
            pitem = self.getPluginItem(pluginId)
            pitem.recreateWidget()
            return

        pitem = self.ui.listWidget.createItem(pluginId, self.fSavedSettings[CARLA_KEY_MAIN_CLASSIC_SKIN])
        self.fPluginList.append(pitem)
        self.fPluginCount += 1

        self.ui.act_plugin_remove_all.setEnabled(self.fPluginCount > 0)

        if pluginType == PLUGIN_LV2:
            self.fHasLoadedLv2Plugins = True

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        if self.fWithCanvas:
            patchcanvas.handlePluginRemoved(pluginId)

        if pluginId in self.fSelectedPlugins:
            self.clearSideStuff()

        if self.fPluginCount == 0:
            return

        pitem = self.getPluginItem(pluginId)

        self.fPluginCount -= 1
        self.fPluginList.pop(pluginId)
        self.ui.listWidget.takeItem(pluginId)

        if pitem is not None:
            pitem.close()
            del pitem

        if self.fPluginCount == 0:
            self.ui.act_plugin_remove_all.setEnabled(False)
            if self.fCurrentlyRemovingAllPlugins:
                self.fCurrentlyRemovingAllPlugins = False
                self.projectLoadingFinished(False)
            return

        # push all plugins 1 slot back
        for i in range(pluginId, self.fPluginCount):
            pitem = self.fPluginList[i]
            pitem.setPluginId(i)

        self.ui.act_plugin_remove_all.setEnabled(True)

    # --------------------------------------------------------------------------------------------------------
    # Canvas

    def clearSideStuff(self):
        if self.fWithCanvas:
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
        pOptions.inline_displays   = self.fSavedSettings[CARLA_KEY_CANVAS_INLINE_DISPLAYS]

        if self.fSavedSettings[CARLA_KEY_CANVAS_FANCY_EYE_CANDY]:
            pOptions.eyecandy = patchcanvas.EYECANDY_FULL
        elif self.fSavedSettings[CARLA_KEY_CANVAS_EYE_CANDY]:
            pOptions.eyecandy = patchcanvas.EYECANDY_SMALL
        else:
            pOptions.eyecandy = patchcanvas.EYECANDY_NONE

        pFeatures = patchcanvas.features_t()
        pFeatures.group_info   = False
        pFeatures.group_rename = False
        pFeatures.port_info    = False
        pFeatures.port_rename  = False
        pFeatures.handle_group_pos = False

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

        if self.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING] != patchcanvas.ANTIALIASING_NONE:
            self.ui.graphicsView.setRenderHint(QPainter.Antialiasing, True)

            fullAA = self.fSavedSettings[CARLA_KEY_CANVAS_ANTIALIASING] == patchcanvas.ANTIALIASING_FULL
            self.ui.graphicsView.setRenderHint(QPainter.SmoothPixmapTransform, fullAA)
            self.ui.graphicsView.setRenderHint(QPainter.TextAntialiasing, fullAA)

            if self.fSavedSettings[CARLA_KEY_CANVAS_USE_OPENGL] and hasGL and QPainter.HighQualityAntialiasing is not None:
                self.ui.graphicsView.setRenderHint(QPainter.HighQualityAntialiasing, self.fSavedSettings[CARLA_KEY_CANVAS_HQ_ANTIALIASING])

        else:
            self.ui.graphicsView.setRenderHint(QPainter.Antialiasing, False)

        if self.fSavedSettings[CARLA_KEY_CANVAS_FULL_REPAINTS]:
            self.ui.graphicsView.setViewportUpdateMode(QGraphicsView.FullViewportUpdate)
        else:
            self.ui.graphicsView.setViewportUpdateMode(QGraphicsView.MinimalViewportUpdate)

    def updateCanvasInitialPos(self):
        x = self.ui.graphicsView.horizontalScrollBar().value() + self.width()/4
        y = self.ui.graphicsView.verticalScrollBar().value() + self.height()/4
        patchcanvas.setInitialPos(x, y)

    def updateMiniCanvasLater(self):
        QTimer.singleShot(self.fMiniCanvasUpdateTimeout, self.ui.miniCanvasPreview.update)

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

        if self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK and self.host.isPlugin:
            return

        if self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

        self.updateMiniCanvasLater()

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

    def _canvasImageRender(self, zoom = 1.0):
        image   = QImage(self.scene.width()*zoom, self.scene.height()*zoom, QImage.Format_RGB32)
        painter = QPainter(image)
        painter.save()
        painter.setRenderHints(painter.renderHints() | QPainter.Antialiasing | QPainter.TextAntialiasing)
        self.scene.clearSelection()
        self.scene.render(painter)
        painter.restore()
        del painter
        return image

    def _canvasImageWrite(self, iw: QImageWriter, imgFormat: bytes, image: QImage):
        iw.setFormat(imgFormat)
        iw.setCompression(-1)
        if QT_VERSION >= 0x50500:
            iw.setOptimizedWrite(True)
        iw.write(image)

    @pyqtSlot()
    def slot_canvasSaveImage(self):
        if self.fProjectFilename:
            dir = QFileInfo(self.fProjectFilename).absoluteDir().absolutePath()
        else:
            dir = self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER]

        fileDialog = QFileDialog(self)
        fileDialog.setAcceptMode(QFileDialog.AcceptSave)
        fileDialog.setDirectory(dir)
        fileDialog.setFileMode(QFileDialog.AnyFile)
        fileDialog.setMimeTypeFilters(("image/png", "image/jpeg"))
        fileDialog.setNameFilter(self.tr("Images (*.png *.jpg)"))
        fileDialog.setOptions(QFileDialog.DontUseCustomDirectoryIcons)
        fileDialog.setWindowTitle(self.tr("Save Image"))

        ok = fileDialog.exec_()

        if not ok:
            return

        newPath = fileDialog.selectedFiles()

        if len(newPath) != 1:
            return

        newPath = newPath[0]

        if QT_VERSION >= 0x50900:
            if fileDialog.selectedMimeTypeFilter() == "image/jpeg":
                imgFormat = b"JPG"
            else:
                imgFormat = b"PNG"
        else:
            if newPath.lower().endswith((".jpg", ".jpeg")):
                imgFormat = b"JPG"
            else:
                imgFormat = b"PNG"

        sender = self.sender()
        if sender == self.ui.act_canvas_save_image_2x:
            zoom = 2.0
        elif sender == self.ui.act_canvas_save_image_4x:
            zoom = 4.0
        else:
            zoom = 1.0

        image = self._canvasImageRender(zoom)
        iw = QImageWriter(newPath)
        self._canvasImageWrite(iw, imgFormat, image)

    @pyqtSlot()
    def slot_canvasCopyToClipboard(self):
        buffer = QBuffer()
        buffer.open(QIODevice.WriteOnly)

        image = self._canvasImageRender()
        iw = QImageWriter(buffer, b"PNG")
        self._canvasImageWrite(iw, b"PNG", image)

        buffer.close()

        mimeData = QMimeData()
        mimeData.setData("image/png", buffer.buffer());

        QApplication.clipboard().setMimeData(mimeData)

    # --------------------------------------------------------------------------------------------------------
    # Canvas (canvas callbacks)

    @pyqtSlot()
    def slot_canvasSelectionChanged(self):
        self.updateMiniCanvasLater()

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

        self.updateMiniCanvasLater()

        if pluginId < 0:
            return
        if pluginId >= self.fPluginCount and pluginId != MAIN_CARLA_PLUGIN_ID:
            print("Error mapping plugin to canvas client:", clientName)
            return

        if pluginId == MAIN_CARLA_PLUGIN_ID:
            hasCustomUI = False
            hasInlineDisplay = False
        else:
            hints = self.host.get_plugin_info(pluginId)['hints']
            hasCustomUI = bool(hints & PLUGIN_HAS_CUSTOM_UI)
            hasInlineDisplay = bool(hints & PLUGIN_HAS_INLINE_DISPLAY)

        patchcanvas.setGroupAsPlugin(clientId, pluginId, hasCustomUI, hasInlineDisplay)

    @pyqtSlot(int)
    def slot_handlePatchbayClientRemovedCallback(self, clientId):
        patchcanvas.removeGroup(clientId)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, str)
    def slot_handlePatchbayClientRenamedCallback(self, clientId, newClientName):
        patchcanvas.renameGroup(clientId, newClientName)
        self.updateMiniCanvasLater()

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
        self.updateMiniCanvasLater()

        if pluginId < 0:
            return
        if pluginId >= self.fPluginCount and pluginId != MAIN_CARLA_PLUGIN_ID:
            print("sorry, can't map this plugin to canvas client", pluginId, self.fPluginCount)
            return

        if pluginId == MAIN_CARLA_PLUGIN_ID:
            hasCustomUI = False
            hasInlineDisplay = False
        else:
            hints = self.host.get_plugin_info(pluginId)['hints']
            hasCustomUI = bool(hints & PLUGIN_HAS_CUSTOM_UI)
            hasInlineDisplay = bool(hints & PLUGIN_HAS_INLINE_DISPLAY)

        patchcanvas.setGroupAsPlugin(clientId, pluginId, hasCustomUI, hasInlineDisplay)

    @pyqtSlot(int, int, int, int, int)
    def slot_handlePatchbayClientPositionChangedCallback(self, clientId, x1, y1, x2, y2):
        if (x1 != 0 and x2 != 0) or (y1 != 0 and y2 != 0):
            patchcanvas.splitGroup(clientId)
        else:
            patchcanvas.joinGroup(clientId)
        patchcanvas.setGroupPosFull(clientId, x1, y1, x2, y2)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, int, int, int, str)
    def slot_handlePatchbayPortAddedCallback(self, clientId, portId, portFlags, portGroupId, portName):
        if portFlags & PATCHBAY_PORT_IS_INPUT:
            portMode = patchcanvas.PORT_MODE_INPUT
        else:
            portMode = patchcanvas.PORT_MODE_OUTPUT

        if portFlags & PATCHBAY_PORT_TYPE_AUDIO:
            portType    = patchcanvas.PORT_TYPE_AUDIO_JACK
            isAlternate = False
        elif portFlags & PATCHBAY_PORT_TYPE_CV:
            portType    = patchcanvas.PORT_TYPE_PARAMETER
            isAlternate = False
        elif portFlags & PATCHBAY_PORT_TYPE_MIDI:
            portType    = patchcanvas.PORT_TYPE_MIDI_JACK
            isAlternate = False
        else:
            portType    = patchcanvas.PORT_TYPE_NULL
            isAlternate = False

        patchcanvas.addPort(clientId, portId, portName, portMode, portType, isAlternate)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, int)
    def slot_handlePatchbayPortRemovedCallback(self, groupId, portId):
        patchcanvas.removePort(groupId, portId)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, int, int, int, str)
    def slot_handlePatchbayPortChangedCallback(self, groupId, portId, portFlags, portGroupId, newPortName):
        patchcanvas.renamePort(groupId, portId, newPortName)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortGroupAddedCallback(self, groupId, portId, portGroupId, newPortName):
        # TODO
        pass

    @pyqtSlot(int, int)
    def slot_handlePatchbayPortGroupRemovedCallback(self, groupId, portId):
        # TODO
        pass

    @pyqtSlot(int, int, int, str)
    def slot_handlePatchbayPortGroupChangedCallback(self, groupId, portId, portGroupId, newPortName):
        # TODO
        pass

    @pyqtSlot(int, int, int, int, int)
    def slot_handlePatchbayConnectionAddedCallback(self, connectionId, groupOutId, portOutId, groupInId, portInId):
        patchcanvas.connectPorts(connectionId, groupOutId, portOutId, groupInId, portInId)
        self.updateMiniCanvasLater()

    @pyqtSlot(int, int, int)
    def slot_handlePatchbayConnectionRemovedCallback(self, connectionId, portOutId, portInId):
        patchcanvas.disconnectPorts(connectionId)
        self.updateMiniCanvasLater()

    # --------------------------------------------------------------------------------------------------------
    # Settings

    def saveSettings(self):
        settings = QSafeSettings()

        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("ShowToolbar", self.ui.toolBar.isEnabled())
        settings.setValue("ShowSidePanel", self.ui.dockWidget.isEnabled())

        diskFolders = []

        for i in range(self.ui.cb_disk.count()):
            diskFolders.append(self.ui.cb_disk.itemData(i))

        settings.setValue("DiskFolders", diskFolders)
        settings.setValue("LastBPM", self.fLastTransportBPM)

        settings.setValue("ShowMeters", self.ui.act_settings_show_meters.isChecked())
        settings.setValue("ShowKeyboard", self.ui.act_settings_show_keyboard.isChecked())
        settings.setValue("HorizontalScrollBarValue", self.ui.graphicsView.horizontalScrollBar().value())
        settings.setValue("VerticalScrollBarValue", self.ui.graphicsView.verticalScrollBar().value())

        settings.setValue(CARLA_KEY_ENGINE_TRANSPORT_MODE,  self.host.transportMode)
        settings.setValue(CARLA_KEY_ENGINE_TRANSPORT_EXTRA, self.host.transportExtra)

        return settings

    def loadSettings(self, firstTime):
        settings = QSafeSettings()

        if self.fPluginListDialog is not None:
            gCarla.felib.destroyPluginListDialog(self.fPluginListDialog)
            self.fPluginListDialog = None

        if firstTime:
            geometry = settings.value("Geometry", QByteArray(), QByteArray)
            if not geometry.isNull():
                self.restoreGeometry(geometry)

            showToolbar = settings.value("ShowToolbar", True, bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.blockSignals(True)
            self.ui.toolBar.setEnabled(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)
            self.ui.toolBar.blockSignals(False)

            #if settings.contains("SplitterState"):
                #self.ui.splitter.restoreState(settings.value("SplitterState", b""))
            #else:
                #self.ui.splitter.setSizes([210, 99999])

            showSidePanel = settings.value("ShowSidePanel", True, bool)
            self.ui.act_settings_show_side_panel.setChecked(showSidePanel)
            self.slot_showSidePanel(showSidePanel)

            diskFolders = settings.value("DiskFolders", [HOME], list)

            self.ui.cb_disk.setItemData(0, HOME)

            for i in range(len(diskFolders)):
                if i == 0: continue
                folder = diskFolders[i]
                self.ui.cb_disk.addItem(os.path.basename(folder), folder)

            #if CARLA_OS_MAC and not settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, True, bool):
            #    self.setUnifiedTitleAndToolBarOnMac(True)

            showMeters = settings.value("ShowMeters", True, bool)
            self.ui.act_settings_show_meters.setChecked(showMeters)
            self.ui.peak_in.setVisible(showMeters)
            self.ui.peak_out.setVisible(showMeters)

            showKeyboard = settings.value("ShowKeyboard", True, bool)
            self.ui.act_settings_show_keyboard.setChecked(showKeyboard)
            self.ui.scrollArea.setVisible(showKeyboard)

            settingsDBf = QSafeSettings("falkTX", "CarlaDatabase2")
            self.fFavoritePlugins = settingsDBf.value("PluginDatabase/Favorites", [], list)

            QTimer.singleShot(100, self.slot_restoreCanvasScrollbarValues)

        # TODO - complete this
        oldSettings = self.fSavedSettings

        if self.host.audioDriverForced is not None:
            audioDriver = self.host.audioDriverForced
        else:
            audioDriver = settings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, str)

        audioDriverPrefix = CARLA_KEY_ENGINE_DRIVER_PREFIX + audioDriver

        self.fSavedSettings = {
            CARLA_KEY_MAIN_PROJECT_FOLDER:      settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER,      CARLA_DEFAULT_MAIN_PROJECT_FOLDER,      str),
            CARLA_KEY_MAIN_CONFIRM_EXIT:        settings.value(CARLA_KEY_MAIN_CONFIRM_EXIT,        CARLA_DEFAULT_MAIN_CONFIRM_EXIT,        bool),
            CARLA_KEY_MAIN_CLASSIC_SKIN:        settings.value(CARLA_KEY_MAIN_CLASSIC_SKIN,        CARLA_DEFAULT_MAIN_CLASSIC_SKIN,        bool),
            CARLA_KEY_MAIN_REFRESH_INTERVAL:    settings.value(CARLA_KEY_MAIN_REFRESH_INTERVAL,    CARLA_DEFAULT_MAIN_REFRESH_INTERVAL,    int),
            CARLA_KEY_MAIN_SYSTEM_ICONS:        settings.value(CARLA_KEY_MAIN_SYSTEM_ICONS,        CARLA_DEFAULT_MAIN_SYSTEM_ICONS,        bool),
            CARLA_KEY_MAIN_EXPERIMENTAL:        settings.value(CARLA_KEY_MAIN_EXPERIMENTAL,        CARLA_DEFAULT_MAIN_EXPERIMENTAL,        bool),
            CARLA_KEY_CANVAS_THEME:             settings.value(CARLA_KEY_CANVAS_THEME,             CARLA_DEFAULT_CANVAS_THEME,             str),
            CARLA_KEY_CANVAS_SIZE:              settings.value(CARLA_KEY_CANVAS_SIZE,              CARLA_DEFAULT_CANVAS_SIZE,              str),
            CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS:  settings.value(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,  CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS,  bool),
            CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS: settings.value(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS, bool),
            CARLA_KEY_CANVAS_USE_BEZIER_LINES:  settings.value(CARLA_KEY_CANVAS_USE_BEZIER_LINES,  CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES,  bool),
            CARLA_KEY_CANVAS_EYE_CANDY:         settings.value(CARLA_KEY_CANVAS_EYE_CANDY,         CARLA_DEFAULT_CANVAS_EYE_CANDY,         bool),
            CARLA_KEY_CANVAS_FANCY_EYE_CANDY:   settings.value(CARLA_KEY_CANVAS_FANCY_EYE_CANDY,   CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY,   bool),
            CARLA_KEY_CANVAS_USE_OPENGL:        settings.value(CARLA_KEY_CANVAS_USE_OPENGL,        CARLA_DEFAULT_CANVAS_USE_OPENGL,        bool),
            CARLA_KEY_CANVAS_ANTIALIASING:      settings.value(CARLA_KEY_CANVAS_ANTIALIASING,      CARLA_DEFAULT_CANVAS_ANTIALIASING,      int),
            CARLA_KEY_CANVAS_HQ_ANTIALIASING:   settings.value(CARLA_KEY_CANVAS_HQ_ANTIALIASING,   CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING,   bool),
            CARLA_KEY_CANVAS_FULL_REPAINTS:     settings.value(CARLA_KEY_CANVAS_FULL_REPAINTS,     CARLA_DEFAULT_CANVAS_FULL_REPAINTS,     bool),
            CARLA_KEY_CUSTOM_PAINTING:         (settings.value(CARLA_KEY_MAIN_USE_PRO_THEME,    True,   bool) and
                                                settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR, "Black", str).lower() == "black"),

            # engine
            CARLA_KEY_ENGINE_AUDIO_DRIVER: audioDriver,
            CARLA_KEY_ENGINE_AUDIO_DEVICE: settings.value(audioDriverPrefix+"/Device", "", str),
            CARLA_KEY_ENGINE_BUFFER_SIZE: settings.value(audioDriverPrefix+"/BufferSize", CARLA_DEFAULT_AUDIO_BUFFER_SIZE, int),
            CARLA_KEY_ENGINE_SAMPLE_RATE: settings.value(audioDriverPrefix+"/SampleRate", CARLA_DEFAULT_AUDIO_SAMPLE_RATE, int),
            CARLA_KEY_ENGINE_TRIPLE_BUFFER: settings.value(audioDriverPrefix+"/TripleBuffer", CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER, bool),

            # file paths
            CARLA_KEY_PATHS_AUDIO: splitter.join(settings.value(CARLA_KEY_PATHS_AUDIO, CARLA_DEFAULT_FILE_PATH_AUDIO, list)),
            CARLA_KEY_PATHS_MIDI: splitter.join(settings.value(CARLA_KEY_PATHS_MIDI,  CARLA_DEFAULT_FILE_PATH_MIDI, list)),

            # plugin paths
            CARLA_KEY_PATHS_LADSPA: splitter.join(settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH, list)),
            CARLA_KEY_PATHS_DSSI: splitter.join(settings.value(CARLA_KEY_PATHS_DSSI, CARLA_DEFAULT_DSSI_PATH, list)),
            CARLA_KEY_PATHS_LV2: splitter.join(settings.value(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH, list)),
            CARLA_KEY_PATHS_VST2: splitter.join(settings.value(CARLA_KEY_PATHS_VST2, CARLA_DEFAULT_VST2_PATH, list)),
            CARLA_KEY_PATHS_VST3: splitter.join(settings.value(CARLA_KEY_PATHS_VST3, CARLA_DEFAULT_VST3_PATH, list)),
            CARLA_KEY_PATHS_SF2: splitter.join(settings.value(CARLA_KEY_PATHS_SF2, CARLA_DEFAULT_SF2_PATH, list)),
            CARLA_KEY_PATHS_SFZ: splitter.join(settings.value(CARLA_KEY_PATHS_SFZ, CARLA_DEFAULT_SFZ_PATH, list)),
            CARLA_KEY_PATHS_JSFX: splitter.join(settings.value(CARLA_KEY_PATHS_JSFX, CARLA_DEFAULT_JSFX_PATH, list)),
            CARLA_KEY_PATHS_CLAP: splitter.join(settings.value(CARLA_KEY_PATHS_CLAP, CARLA_DEFAULT_CLAP_PATH, list)),

            # osc
            CARLA_KEY_OSC_ENABLED: settings.value(CARLA_KEY_OSC_ENABLED, CARLA_DEFAULT_OSC_ENABLED, bool),
            CARLA_KEY_OSC_TCP_PORT_ENABLED: settings.value(CARLA_KEY_OSC_TCP_PORT_ENABLED, CARLA_DEFAULT_OSC_TCP_PORT_ENABLED, bool),
            CARLA_KEY_OSC_TCP_PORT_RANDOM: settings.value(CARLA_KEY_OSC_TCP_PORT_RANDOM, CARLA_DEFAULT_OSC_TCP_PORT_RANDOM, bool),
            CARLA_KEY_OSC_TCP_PORT_NUMBER: settings.value(CARLA_KEY_OSC_TCP_PORT_NUMBER, CARLA_DEFAULT_OSC_TCP_PORT_NUMBER, int),
            CARLA_KEY_OSC_UDP_PORT_ENABLED: settings.value(CARLA_KEY_OSC_UDP_PORT_ENABLED, CARLA_DEFAULT_OSC_UDP_PORT_ENABLED, bool),
            CARLA_KEY_OSC_UDP_PORT_RANDOM: settings.value(CARLA_KEY_OSC_UDP_PORT_RANDOM, CARLA_DEFAULT_OSC_UDP_PORT_RANDOM, bool),
            CARLA_KEY_OSC_UDP_PORT_NUMBER: settings.value(CARLA_KEY_OSC_UDP_PORT_NUMBER, CARLA_DEFAULT_OSC_UDP_PORT_NUMBER, int),

            # wine
            CARLA_KEY_WINE_EXECUTABLE: settings.value(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE, str),
            CARLA_KEY_WINE_AUTO_PREFIX: settings.value(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX, bool),
            CARLA_KEY_WINE_FALLBACK_PREFIX: settings.value(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX, str),
            CARLA_KEY_WINE_RT_PRIO_ENABLED: settings.value(CARLA_KEY_WINE_RT_PRIO_ENABLED, CARLA_DEFAULT_WINE_RT_PRIO_ENABLED, bool),
            CARLA_KEY_WINE_BASE_RT_PRIO: settings.value(CARLA_KEY_WINE_BASE_RT_PRIO, CARLA_DEFAULT_WINE_BASE_RT_PRIO, int),
            CARLA_KEY_WINE_SERVER_RT_PRIO: settings.value(CARLA_KEY_WINE_SERVER_RT_PRIO, CARLA_DEFAULT_WINE_SERVER_RT_PRIO, int),

            # experimental switches
            CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES:
                settings.value(CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES, bool),
            CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES:
                settings.value(CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES, bool),
        }

        if not self.host.isControl:
            self.fSavedSettings[CARLA_KEY_CANVAS_INLINE_DISPLAYS] = settings.value(CARLA_KEY_CANVAS_INLINE_DISPLAYS, CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS, bool)
        else:
            self.fSavedSettings[CARLA_KEY_CANVAS_INLINE_DISPLAYS] = False

        settings2 = QSafeSettings("falkTX", "Carla2")

        if self.host.experimental:
            visible = settings2.value(CARLA_KEY_EXPERIMENTAL_JACK_APPS, CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS, bool)
            self.ui.act_plugin_add_jack.setVisible(visible)
        else:
            self.ui.act_plugin_add_jack.setVisible(False)

        self.fMiniCanvasUpdateTimeout = 1000 if self.fSavedSettings[CARLA_KEY_CANVAS_FANCY_EYE_CANDY] else 0

        setEngineSettings(self.host, self.fSavedSettings)
        self.restartTimersIfNeeded()

        if oldSettings.get(CARLA_KEY_MAIN_CLASSIC_SKIN, None) not in (self.fSavedSettings[CARLA_KEY_MAIN_CLASSIC_SKIN], None):
            newSkin = "classic" if self.fSavedSettings[CARLA_KEY_MAIN_CLASSIC_SKIN] else None

            for pitem in self.fPluginList:
                if pitem is None:
                    continue
                pitem.recreateWidget(newSkin = newSkin)

        return settings

    # --------------------------------------------------------------------------------------------------------
    # Settings (helpers)

    def enableTransport(self, enabled):
        self.ui.group_transport_controls.setEnabled(enabled)
        self.ui.group_transport_settings.setEnabled(enabled)

    @pyqtSlot()
    def slot_restoreCanvasScrollbarValues(self):
        settings = QSafeSettings()
        horiz = settings.value("HorizontalScrollBarValue", int(self.ui.graphicsView.horizontalScrollBar().maximum()/2), int)
        vertc = settings.value("VerticalScrollBarValue", int(self.ui.graphicsView.verticalScrollBar().maximum()/2), int)
        self.ui.graphicsView.horizontalScrollBar().setValue(horiz)
        self.ui.graphicsView.verticalScrollBar().setValue(vertc)

    # --------------------------------------------------------------------------------------------------------
    # Settings (menu actions)

    @pyqtSlot(bool)
    def slot_showSidePanel(self, yesNo):
        self.ui.dockWidget.setEnabled(yesNo)
        self.ui.dockWidget.setVisible(yesNo)

    @pyqtSlot(bool)
    def slot_showToolbar(self, yesNo):
        self.ui.toolBar.blockSignals(True)
        self.ui.toolBar.setEnabled(yesNo)
        self.ui.toolBar.setVisible(yesNo)
        self.ui.toolBar.blockSignals(False)

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

        if self.fWithCanvas:
            patchcanvas.clear()
            self.setupCanvas()
            self.slot_miniCanvasCheckAll()

        if self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK and self.host.isPlugin:
            pass
        elif self.host.is_engine_running():
            self.host.patchbay_refresh(self.fExternalPatchbay)

    # --------------------------------------------------------------------------------------------------------
    # About (menu actions)

    @pyqtSlot()
    def slot_aboutCarla(self):
        gCarla.felib.createAndExecAboutDialog(self.fParentOrSelf,
                                              self.host.handle,
                                              self.host.isControl,
                                              self.host.isPlugin)

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

        if not self.ui.listWidget.isDragUrlValid(filename):
            return

        if not self.host.load_file(filename):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"),
                             self.tr("Failed to load file"),
                             self.host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)
            return

        if filename.endswith(".carxp"):
            self.loadExternalCanvasGroupPositionsIfNeeded(filename)

    # --------------------------------------------------------------------------------------------------------
    # Transport

    def refreshTransport(self, forced = False):
        if not self.ui.l_transport_time.isVisible():
            return
        if self.fSampleRate == 0.0 or not self.host.is_engine_running():
            return

        timeInfo = self.host.get_transport_info()
        playing  = timeInfo['playing']
        frame    = timeInfo['frame']
        bpm      = timeInfo['bpm']

        if playing != self.fLastTransportState or forced:
            if playing:
                if self.fSavedSettings[CARLA_KEY_MAIN_SYSTEM_ICONS]:
                    icon = getIcon('media-playback-pause', 16, 'svgz')
                else:
                    icon = QIcon(":/16x16/media-playback-pause.svgz")
                self.ui.b_transport_play.setChecked(True)
                self.ui.b_transport_play.setIcon(icon)
                #self.ui.b_transport_play.setText(self.tr("&Pause"))
            else:
                if self.fSavedSettings[CARLA_KEY_MAIN_SYSTEM_ICONS]:
                    icon = getIcon('media-playback-start', 16, 'svgz')
                else:
                    icon = QIcon(":/16x16/media-playback-start.svgz")
                self.ui.b_transport_play.setChecked(False)
                self.ui.b_transport_play.setIcon(icon)
                #self.ui.b_play.setText(self.tr("&Play"))

            self.fLastTransportState = playing

        if frame != self.fLastTransportFrame or forced:
            self.fLastTransportFrame = frame

            time = frame / self.fSampleRate
            secs =  time % 60
            mins = (time / 60) % 60
            hrs  = (time / 3600) % 60
            self.ui.l_transport_time.setText("%02i:%02i:%02i" % (hrs, mins, secs))

            frame1 =  frame % 1000
            frame2 = (frame / 1000) % 1000
            frame3 = (frame / 1000000) % 1000
            self.ui.l_transport_frame.setText("%03i'%03i'%03i" % (frame3, frame2, frame1))

            bar  = timeInfo['bar']
            beat = timeInfo['beat']
            tick = timeInfo['tick']
            self.ui.l_transport_bbt.setText("%03i|%02i|%04i" % (bar, beat, tick))

        if bpm != self.fLastTransportBPM or forced:
            self.fLastTransportBPM = bpm

            if bpm > 0.0:
                self.ui.dsb_transport_bpm.blockSignals(True)
                self.ui.dsb_transport_bpm.setValue(bpm)
                self.ui.dsb_transport_bpm.blockSignals(False)
                self.ui.dsb_transport_bpm.setStyleSheet("")
            else:
                self.ui.dsb_transport_bpm.setStyleSheet("QDoubleSpinBox { color: palette(mid); }")

    # --------------------------------------------------------------------------------------------------------
    # Transport (menu actions)

    @pyqtSlot(bool)
    def slot_transportPlayPause(self, toggled):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        if toggled:
            self.host.transport_play()
        else:
            self.host.transport_pause()

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportStop(self):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        self.host.transport_pause()
        self.host.transport_relocate(0)

        self.refreshTransport()

    @pyqtSlot()
    def slot_transportBackwards(self):
        if self.host.isPlugin or not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() - 100000

        if newFrame < 0:
            newFrame = 0

        self.host.transport_relocate(newFrame)

    @pyqtSlot(float)
    def slot_transportBpmChanged(self, newValue):
        self.host.transport_bpm(newValue)

    @pyqtSlot()
    def slot_transportForwards(self):
        if self.fSampleRate == 0.0 or self.host.isPlugin or not self.host.is_engine_running():
            return

        newFrame = self.host.get_current_transport_frame() + int(self.fSampleRate*2.5)
        self.host.transport_relocate(newFrame)

    @pyqtSlot(bool)
    def slot_transportJackEnabled(self, clicked):
        if not self.host.is_engine_running():
            return
        self.host.transportMode = ENGINE_TRANSPORT_MODE_JACK if clicked else ENGINE_TRANSPORT_MODE_INTERNAL
        self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,
                                    self.host.transportMode,
                                    self.host.transportExtra)

    @pyqtSlot(bool)
    def slot_transportLinkEnabled(self, clicked):
        if not self.host.is_engine_running():
            return
        extra = ":link:" if clicked else ""
        self.host.transportExtra = extra
        self.host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,
                                    self.host.transportMode,
                                    self.host.transportExtra)

    # --------------------------------------------------------------------------------------------------------
    # Other

    @pyqtSlot(bool)
    def slot_xrunClear(self):
        self.host.clear_engine_xruns()

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
        if self.fPluginCount == 0:
            return

        for pluginId in self.fSelectedPlugins:
            self.host.send_midi_note(pluginId, 0, note, 100)

            pedit = self.getPluginEditDialog(pluginId)
            pedit.noteOn(0, note, 100)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        if self.fPluginCount == 0:
            return

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

        check = self.host.get_custom_data_value(pluginId, CUSTOM_DATA_TYPE_PROPERTY, "CarlaSkinIsCompacted")
        if not check:
            return
        isCompacted = bool(check == "true")

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

        currentIndex = self.ui.tabWidget.currentIndex()

        if currentIndex == 1:
            width  = self.ui.graphicsView.width()
            height = self.ui.graphicsView.height()
        else:
            self.ui.tabWidget.blockSignals(True)
            self.ui.tabWidget.setCurrentIndex(1)
            width  = self.ui.graphicsView.width()
            height = self.ui.graphicsView.height()
            self.ui.tabWidget.setCurrentIndex(currentIndex)
            self.ui.tabWidget.blockSignals(False)

        self.scene.updateLimits()

        self.ui.miniCanvasPreview.setViewSize(float(width)/self.fCanvasWidth, float(height)/self.fCanvasHeight)

    @pyqtSlot(float, float)
    def slot_miniCanvasMoved(self, xp, yp):
        hsb = self.ui.graphicsView.horizontalScrollBar()
        vsb = self.ui.graphicsView.verticalScrollBar()
        hsb.setValue(xp * hsb.maximum())
        vsb.setValue(yp * vsb.maximum())
        self.updateCanvasInitialPos()

    # --------------------------------------------------------------------------------------------------------
    # Logs autoscroll, save and clear

    @pyqtSlot(int)
    def slot_toggleLogAutoscroll(self, checkState):
        self.autoscrollOnNewLog = checkState == Qt.Checked
        if self.autoscrollOnNewLog:
            self.ui.text_logs.verticalScrollBar().setValue(self.ui.text_logs.verticalScrollBar().maximum())

    @pyqtSlot(int)
    def slot_logSliderMoved(self, slider_pos):
        if self.ui.text_logs.verticalScrollBar().hasTracking() or self.autoscrollOnNewLog:
            self.lastLogSliderPos = slider_pos
        else:
            self.ui.text_logs.verticalScrollBar().setValue(self.lastLogSliderPos)

    @pyqtSlot()
    def slot_logButtonsState(self, enabled=True):
        self.ui.logs_clear.setEnabled(enabled)
        self.ui.logs_save.setEnabled(enabled)

    @pyqtSlot()
    def slot_logSave(self):
        filename = os.path.join(self.fSavedSettings[CARLA_KEY_MAIN_PROJECT_FOLDER], 'carla_log.txt')
        filename, _ = QFileDialog.getSaveFileName(self, self.tr("Save Logs"), filename)

        if not filename:
            return

        try:
            with open(filename, "w") as logfile:
                logfile.write(self.ui.text_logs.toPlainText())
                logfile.close()
        except:
            return

    @pyqtSlot()
    def slot_logClear(self):
        self.ui.text_logs.clear()
        self.ui.text_logs.appendPlainText("======= Logs cleared ========")
        self.slot_logButtonsState(False)

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

    @pyqtSlot(bool)
    def slot_toolbarVisibilityChanged(self, visible):
        self.ui.toolBar.blockSignals(True)
        self.ui.toolBar.setEnabled(visible)
        self.ui.toolBar.blockSignals(False)
        self.ui.act_settings_show_toolbar.setChecked(visible)

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
    def slot_handleNSMCallback(self, opcode, valueInt, valueStr):
        if opcode == NSM_CALLBACK_INIT:
            return

        # Error
        elif opcode == NSM_CALLBACK_ERROR:
            pass

        # Reply
        elif opcode == NSM_CALLBACK_ANNOUNCE:
            self.fFirstEngineInit    = False
            self.fSessionManagerName = valueStr
            self.setProperWindowTitle()

            # If NSM server does not support optional-gui, revert our initial assumptions that it does
            if (valueInt & (1 << 1)) == 0x0:
                self.fWindowCloseHideGui = False
                self.ui.act_file_quit.setText(self.tr("&Quit"))
                QApplication.instance().setQuitOnLastWindowClosed(True)
                self.show()

        # Open
        elif opcode == NSM_CALLBACK_OPEN:
            self.fProjectFilename = QFileInfo(valueStr+".carxp").absoluteFilePath()
            self.setProperWindowTitle()

            self.fCustomStopAction = self.CUSTOM_ACTION_PROJECT_LOAD
            self.slot_engineStop(True)
            return

        # Save
        elif opcode == NSM_CALLBACK_SAVE:
            self.saveProjectNow()

        # Session is Loaded
        elif opcode == NSM_CALLBACK_SESSION_IS_LOADED:
            pass

        # Show Optional Gui
        elif opcode == NSM_CALLBACK_SHOW_OPTIONAL_GUI:
            self.show()

        # Hide Optional Gui
        elif opcode == NSM_CALLBACK_HIDE_OPTIONAL_GUI:
            self.hideForNSM()

        # Set client name
        elif opcode == NSM_CALLBACK_SET_CLIENT_NAME_ID:
            self.fClientName = valueStr
            return

        self.host.nsm_ready(opcode)

    # --------------------------------------------------------------------------------------------------------

    def fixLogText(self, text):
        return text.replace("\x1b[30;1m", "").replace("\x1b[31m", "").replace("\x1b[0m", "")

    @pyqtSlot(int, int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valuef, valueStr):
        self.ui.text_logs.appendPlainText(self.fixLogText(valueStr))

    @pyqtSlot(str)
    def slot_handleInfoCallback(self, info):
        QMessageBox.information(self, "Information", info)

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, "Error", error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        self.fIsProjectLoading = False
        self.killTimers()
        self.removeAllPlugins()
        self.projectLoadingFinished(False)

    @pyqtSlot(int)
    def slot_handleInlineDisplayRedrawCallback(self, pluginId):
        # FIXME
        if self.fIdleTimerSlow != 0 and self.fIdleTimerFast != 0 and pluginId < self.fPluginCount and not self.fIsProjectLoading:
            patchcanvas.redrawPluginGroup(pluginId)

    # --------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        self.slot_fileSave()

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.fCustomStopAction = self.CUSTOM_ACTION_APP_CLOSE
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

        elif ptype == PLUGIN_SF2:
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

    def getPluginCount(self):
        return self.fPluginCount

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

    def waitForPendingEvents(self):
        pass

    # --------------------------------------------------------------------------------------------------------
    # show/hide event

    def showEvent(self, event):
        self.getAndRefreshRuntimeInfo()
        self.refreshTransport(True)
        QMainWindow.showEvent(self, event)

        if QT_VERSION >= 0x50600:
            self.host.set_engine_option(ENGINE_OPTION_FRONTEND_UI_SCALE, int(self.devicePixelRatioF() * 1000), "")
            print("Frontend pixel ratio is", self.devicePixelRatioF())

        # set our gui as parent for all plugins UIs
        if self.host.manageUIs and not self.host.isControl:
            if CARLA_OS_MAC:
                nsViewPtr = int(self.winId())
                winIdStr  = "%x" % gCarla.utils.cocoa_get_window(nsViewPtr)
            elif CARLA_OS_WIN or QApplication.platformName() == "xcb":
                winIdStr = "%x" % int(self.winId())
            else:
                winIdStr = "0"
            self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, winIdStr)

    def hideEvent(self, event):
        # disable parent
        if not self.host.isControl:
            self.host.set_engine_option(ENGINE_OPTION_FRONTEND_WIN_ID, 0, "0")

        QMainWindow.hideEvent(self, event)

    # --------------------------------------------------------------------------------------------------------
    # resize event

    def resizeEvent(self, event):
        QMainWindow.resizeEvent(self, event)

        if self.fWithCanvas:
            self.slot_miniCanvasCheckSize()

    # --------------------------------------------------------------------------------------------------------
    # timer event

    def refreshRuntimeInfo(self, load, xruns):
        txt1 = str(xruns) if (xruns >= 0) else "--"
        txt2 = "" if (xruns == 1) else "s"
        self.ui.b_xruns.setText("%s Xrun%s" % (txt1, txt2))
        self.ui.pb_dsp_load.setValue(int(load))

    def getAndRefreshRuntimeInfo(self):
        if not self.ui.pb_dsp_load.isVisible():
            return
        if not self.host.is_engine_running():
            return
        info = self.host.get_runtime_engine_info()
        self.refreshRuntimeInfo(info['load'], info['xruns'])

    def idleFast(self):
        self.host.engine_idle()
        self.refreshTransport()

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
        self.getAndRefreshRuntimeInfo()

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
    # color/style change event

    def changeEvent(self, event):
        if event.type() in (QEvent.PaletteChange, QEvent.StyleChange):
            self.updateStyle()
        QMainWindow.changeEvent(self, event)

    def updateStyle(self):
        # Rack padding images setup
        rack_imgL = QImage(":/bitmaps/rack_padding_left.png")
        rack_imgR = QImage(":/bitmaps/rack_padding_right.png")

        min_value = 0.07

        if PYQT_VERSION >= 0x50600:
            value_fix = 1.0/(1.0-rack_imgL.scaled(1, 1, Qt.IgnoreAspectRatio, Qt.SmoothTransformation).pixelColor(0,0).blackF())
        else:
            value_fix = 1.5

        rack_pal = self.ui.rack.palette()
        bg_color = rack_pal.window().color()
        fg_color = rack_pal.text().color()
        bg_value = 1.0 - bg_color.blackF()
        if bg_value != 0.0 and bg_value < min_value:
            pad_color = bg_color.lighter(int(100*min_value/bg_value*value_fix))
        else:
            pad_color = QColor.fromHsvF(0.0, 0.0, min_value*value_fix)

        painter = QPainter()
        fillRect = rack_imgL.rect().adjusted(-1,-1,1,1)

        painter.begin(rack_imgL)
        painter.setCompositionMode(QPainter.CompositionMode_Multiply)
        painter.setBrush(pad_color)
        painter.drawRect(fillRect)
        painter.end()
        rack_pixmapL = QPixmap(rack_imgL)
        self.imgL_palette = QPalette()
        self.imgL_palette.setBrush(QPalette.Window, QBrush(rack_pixmapL))
        self.ui.pad_left.setPalette(self.imgL_palette)
        self.ui.pad_left.setAutoFillBackground(True)

        painter.begin(rack_imgR)
        painter.setCompositionMode(QPainter.CompositionMode_Multiply)
        painter.setBrush(pad_color)
        painter.drawRect(fillRect)
        painter.end()
        rack_pixmapR = QPixmap(rack_imgR)
        self.imgR_palette = QPalette()
        self.imgR_palette.setBrush(QPalette.Window, QBrush(rack_pixmapR))
        self.ui.pad_right.setPalette(self.imgR_palette)
        self.ui.pad_right.setAutoFillBackground(True)

        # qt's rgba is actually argb, so convert that
        bg_color_value = bg_color.rgba()
        bg_color_value = ((bg_color_value & 0xffffff) << 8) | (bg_color_value >> 24)

        fg_color_value = fg_color.rgba()
        fg_color_value = ((fg_color_value & 0xffffff) << 8) | (fg_color_value >> 24)

        self.host.set_engine_option(ENGINE_OPTION_FRONTEND_BACKGROUND_COLOR, bg_color_value, "")
        self.host.set_engine_option(ENGINE_OPTION_FRONTEND_FOREGROUND_COLOR, fg_color_value, "")

    # --------------------------------------------------------------------------------------------------------
    # paint event

    #def paintEvent(self, event):
        #QMainWindow.paintEvent(self, event)

        #if CARLA_OS_MAC or not self.fSavedSettings[CARLA_KEY_CUSTOM_PAINTING]:
            #return

        #painter = QPainter(self)
        #painter.setBrush(QColor(36, 36, 36))
        #painter.setPen(QColor(62, 62, 62))
        #painter.drawRect(1, self.height()/2, self.width()-3, self.height()-self.height()/2-1)

    # --------------------------------------------------------------------------------------------------------
    # close event

    def shouldIgnoreClose(self):
        if self.host.isControl or self.host.isPlugin:
            return False
        if self.fCustomStopAction == self.CUSTOM_ACTION_APP_CLOSE:
            return False
        if self.fWindowCloseHideGui:
            return False
        if self.fSavedSettings[CARLA_KEY_MAIN_CONFIRM_EXIT]:
            return QMessageBox.question(self, self.tr("Quit"),
                                              self.tr("Are you sure you want to quit Carla?"),
                                              QMessageBox.Yes|QMessageBox.No) == QMessageBox.No
        return False

    def closeEvent(self, event):
        if self.shouldIgnoreClose():
            event.ignore()
            return

        if self.fWindowCloseHideGui and self.fCustomStopAction != self.CUSTOM_ACTION_APP_CLOSE:
            self.hideForNSM()
            self.host.nsm_ready(NSM_CALLBACK_HIDE_OPTIONAL_GUI)
            return

        patchcanvas.handleAllPluginsRemoved()

        if CARLA_OS_MAC and self.fMacClosingHelper and not (self.host.isControl or self.host.isPlugin):
            self.fCustomStopAction = self.CUSTOM_ACTION_APP_CLOSE
            self.fMacClosingHelper = False
            event.ignore()

            for i in reversed(range(self.fPluginCount)):
                self.host.show_custom_ui(i, False)

            QTimer.singleShot(100, self.close)
            return

        self.killTimers()
        self.saveSettings()

        if self.host.is_engine_running() and not (self.host.isControl or self.host.isPlugin):
            if not self.slot_engineStop(True):
                self.fCustomStopAction = self.CUSTOM_ACTION_APP_CLOSE
                event.ignore()
                return

        QMainWindow.closeEvent(self, event)

        # if we reach this point, fully close ourselves
        gCarla.gui = None
        QApplication.instance().quit()

# ------------------------------------------------------------------------------------------------
# Canvas callback

def canvasCallback(action, value1, value2, valueStr):
    host = gCarla.gui.host

    if gCarla.gui.fCustomStopAction == HostWindow.CUSTOM_ACTION_APP_CLOSE:
        return

    if action == patchcanvas.ACTION_GROUP_INFO:
        pass

    elif action == patchcanvas.ACTION_GROUP_RENAME:
        pass

    elif action == patchcanvas.ACTION_GROUP_SPLIT:
        groupId = value1
        patchcanvas.splitGroup(groupId)
        gCarla.gui.updateMiniCanvasLater()

    elif action == patchcanvas.ACTION_GROUP_JOIN:
        groupId = value1
        patchcanvas.joinGroup(groupId)
        gCarla.gui.updateMiniCanvasLater()

    elif action == patchcanvas.ACTION_GROUP_POSITION:
        if gCarla.gui.fIsProjectLoading:
            return
        if not host.is_engine_running():
            return
        groupId = value1
        x1, y1, x2, y2 = tuple(int(i) for i in valueStr.split(":"))
        host.patchbay_set_group_pos(gCarla.gui.fExternalPatchbay, groupId, x1, y1, x2, y2)
        gCarla.gui.updateMiniCanvasLater()

    elif action == patchcanvas.ACTION_PORT_INFO:
        pass

    elif action == patchcanvas.ACTION_PORT_RENAME:
        pass

    elif action == patchcanvas.ACTION_PORTS_CONNECT:
        gOut, pOut, gIn, pIn = tuple(int(i) for i in valueStr.split(":"))

        if not host.patchbay_connect(gCarla.gui.fExternalPatchbay, gOut, pOut, gIn, pIn):
            print("Connection failed:", host.get_last_error())

    elif action == patchcanvas.ACTION_PORTS_DISCONNECT:
        connectionId = value1

        if not host.patchbay_disconnect(gCarla.gui.fExternalPatchbay, connectionId):
            print("Disconnect failed:", host.get_last_error())

    elif action == patchcanvas.ACTION_PLUGIN_CLONE:
        pluginId = value1

        if not host.clone_plugin(pluginId):
            CustomMessageBox(gCarla.gui, QMessageBox.Warning, gCarla.gui.tr("Error"), gCarla.gui.tr("Operation failed"),
                                         host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    elif action == patchcanvas.ACTION_PLUGIN_EDIT:
        pluginId = value1
        pwidget  = gCarla.gui.getPluginSlotWidget(pluginId)

        if pwidget is not None:
            pwidget.showEditDialog()

    elif action == patchcanvas.ACTION_PLUGIN_RENAME:
        pluginId = value1
        pwidget  = gCarla.gui.getPluginSlotWidget(pluginId)

        if pwidget is not None:
            pwidget.showRenameDialog()

    elif action == patchcanvas.ACTION_PLUGIN_REPLACE:
        pluginId = value1
        pwidget  = gCarla.gui.getPluginSlotWidget(pluginId)

        if pwidget is not None:
            pwidget.showReplaceDialog()

    elif action == patchcanvas.ACTION_PLUGIN_REMOVE:
        pluginId = value1

        if not host.remove_plugin(pluginId):
            CustomMessageBox(gCarla.gui, QMessageBox.Warning, gCarla.gui.tr("Error"), gCarla.gui.tr("Operation failed"),
                                         host.get_last_error(), QMessageBox.Ok, QMessageBox.Ok)

    elif action == patchcanvas.ACTION_PLUGIN_SHOW_UI:
        pluginId = value1
        pwidget  = gCarla.gui.getPluginSlotWidget(pluginId)

        if pwidget is not None:
            pwidget.showCustomUI()

    elif action == patchcanvas.ACTION_BG_RIGHT_CLICK:
        gCarla.gui.slot_showPluginActionsMenu()

    elif action == patchcanvas.ACTION_INLINE_DISPLAY:
        if gCarla.gui.fIsProjectLoading:
            return
        if not host.is_engine_running():
            return
        pluginId = value1
        width, height = [int(v) for v in valueStr.split(":")]
        return host.render_inline_display(pluginId, width, height)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def engineCallback(host, action, pluginId, value1, value2, value3, valuef, valueStr):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    valueStr = charPtrToString(valueStr)

    if action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.processMode   = value1
        host.transportMode = value2
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.processMode   = value1
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.transportMode  = value1
        host.transportExtra = valueStr

    if action == ENGINE_CALLBACK_DEBUG:
        host.DebugCallback.emit(pluginId, value1, value2, value3, valuef, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_ADDED:
        host.PluginAddedCallback.emit(pluginId, value1, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_REMOVED:
        host.PluginRemovedCallback.emit(pluginId)
    elif action == ENGINE_CALLBACK_PLUGIN_RENAMED:
        host.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
        host.PluginUnavailableCallback.emit(pluginId, valueStr)
    elif action == ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
        host.ParameterValueChangedCallback.emit(pluginId, value1, valuef)
    elif action == ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
        host.ParameterDefaultChangedCallback.emit(pluginId, value1, valuef)
    elif action == ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED:
        host.ParameterMappedControlIndexChangedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED:
        minimum, maximum = (float(v) for v in valueStr.split(":", 2))
        host.ParameterMappedRangeChangedCallback.emit(pluginId, value1, minimum, maximum)
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
        host.NoteOnCallback.emit(pluginId, value1, value2, value3)
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
    elif action == ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED:
        host.PatchbayClientPositionChangedCallback.emit(pluginId, value1, value2, value3, int(round(valuef)))
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
        host.PatchbayPortAddedCallback.emit(pluginId, value1, value2, value3, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
        host.PatchbayPortRemovedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED:
        host.PatchbayPortChangedCallback.emit(pluginId, value1, value2, value3, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED:
        host.PatchbayPortGroupAddedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED:
        host.PatchbayPortGroupRemovedCallback.emit(pluginId, value1)
    elif action == ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED:
        host.PatchbayPortGroupChangedCallback.emit(pluginId, value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
        gOut, pOut, gIn, pIn = [int(i) for i in valueStr.split(":")] # FIXME
        host.PatchbayConnectionAddedCallback.emit(pluginId, gOut, pOut, gIn, pIn)
    elif action == ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        host.PatchbayConnectionRemovedCallback.emit(pluginId, value1, value2)
    elif action == ENGINE_CALLBACK_ENGINE_STARTED:
        host.EngineStartedCallback.emit(pluginId, value1, value2, value3, valuef, valueStr)
    elif action == ENGINE_CALLBACK_ENGINE_STOPPED:
        host.EngineStoppedCallback.emit()
    elif action == ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host.ProcessModeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host.TransportModeChangedCallback.emit(value1, valueStr)
    elif action == ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
        host.BufferSizeChangedCallback.emit(value1)
    elif action == ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
        host.SampleRateChangedCallback.emit(valuef)
    elif action == ENGINE_CALLBACK_CANCELABLE_ACTION:
        host.CancelableActionCallback.emit(pluginId, bool(value1 != 0), valueStr)
    elif action == ENGINE_CALLBACK_PROJECT_LOAD_FINISHED:
        host.ProjectLoadFinishedCallback.emit()
    elif action == ENGINE_CALLBACK_NSM:
        host.NSMCallback.emit(value1, value2, valueStr)
    elif action == ENGINE_CALLBACK_IDLE:
        QApplication.processEvents()
    elif action == ENGINE_CALLBACK_INFO:
        host.InfoCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_ERROR:
        host.ErrorCallback.emit(valueStr)
    elif action == ENGINE_CALLBACK_QUIT:
        host.QuitCallback.emit()
    elif action == ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW:
        host.InlineDisplayRedrawCallback.emit(pluginId)
    else:
        print("unhandled action", action)

# ------------------------------------------------------------------------------------------------------------
# File callback

def fileCallback(ptr, action, isDir, title, filter):
    title  = charPtrToString(title)
    filter = charPtrToString(filter)

    if action == FILE_CALLBACK_OPEN:
        ret, ok = QFileDialog.getOpenFileName(gCarla.gui, title, "", filter) #, QFileDialog.ShowDirsOnly if isDir else 0x0)
    elif action == FILE_CALLBACK_SAVE:
        ret, ok = QFileDialog.getSaveFileName(gCarla.gui, title, "", filter, QFileDialog.ShowDirsOnly if isDir else 0x0)
    else:
        ret, ok = ("", "")

    # FIXME use ok value, test if it works as expected
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
    # Check if we should open main lib as local or global

    settings = QSafeSettings("falkTX", "Carla2")

    loadGlobal = settings.value(CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL, CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL, bool)

    # --------------------------------------------------------------------------------------------------------
    # Set Carla library name

    libname   = "libcarla_%s2.%s" % ("control" if isControl else "standalone", DLL_EXTENSION)
    libname   = os.path.join(pathBinaries, libname)
    felibname = os.path.join(pathBinaries, "libcarla_frontend.%s" % (DLL_EXTENSION))
    utilsname = os.path.join(pathBinaries, "libcarla_utils.%s" % (DLL_EXTENSION))

    # --------------------------------------------------------------------------------------------------------
    # Print info

    if not (gCarla.nogui and isinstance(gCarla.nogui, int)):
        print("Carla %s started, status:" % CARLA_VERSION_STRING)
        print("  Python version: %s" % sys.version.split(" ",1)[0])
        print("  Qt version:     %s" % QT_VERSION_STR)
        print("  PyQt version:   %s" % PYQT_VERSION_STR)
        print("  Binary dir:     %s" % pathBinaries)
        print("  Resources dir:  %s" % pathResources)

    # --------------------------------------------------------------------------------------------------------
    # Init host

    if failError:
        # no try
        host = HostClass() if HostClass is not None else CarlaHostQtDLL(libname, loadGlobal)
    else:
        try:
            host = HostClass() if HostClass is not None else CarlaHostQtDLL(libname, loadGlobal)
        except:
            host = CarlaHostQtNull()

    host.isControl = isControl
    host.isPlugin  = isPlugin

    host.set_engine_callback(lambda h,a,p,v1,v2,v3,vf,vs: engineCallback(host,a,p,v1,v2,v3,vf,vs))
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
    # Init frontend lib

    if not gCarla.nogui:
        gCarla.felib = CarlaFrontendLib(felibname)

    # --------------------------------------------------------------------------------------------------------
    # Init utils

    gCarla.utils = CarlaUtils(utilsname)
    gCarla.utils.set_process_name(os.path.basename(initName))

    try:
        sys.stdout.flush()
    except:
        pass

    sys.stdout = CarlaPrint(False)
    sys.stderr = CarlaPrint(True)
    sys.excepthook = sys_excepthook

    # --------------------------------------------------------------------------------------------------------
    # Done

    return host

# ------------------------------------------------------------------------------------------------------------
# Load host settings

def loadHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    settings = QSafeSettings("falkTX", "Carla2")

    host.experimental = settings.value(CARLA_KEY_MAIN_EXPERIMENTAL, CARLA_DEFAULT_MAIN_EXPERIMENTAL, bool)
    host.exportLV2 = settings.value(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2, CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT, bool)
    host.manageUIs = settings.value(CARLA_KEY_ENGINE_MANAGE_UIS, CARLA_DEFAULT_MANAGE_UIS, bool)
    host.maxParameters = settings.value(CARLA_KEY_ENGINE_MAX_PARAMETERS, CARLA_DEFAULT_MAX_PARAMETERS, int)
    host.resetXruns = settings.value(CARLA_KEY_ENGINE_RESET_XRUNS, CARLA_DEFAULT_RESET_XRUNS, bool)
    host.forceStereo = settings.value(CARLA_KEY_ENGINE_FORCE_STEREO, CARLA_DEFAULT_FORCE_STEREO, bool)
    host.preferPluginBridges = settings.value(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES, bool)
    host.preferUIBridges = settings.value(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES, CARLA_DEFAULT_PREFER_UI_BRIDGES, bool)
    host.preventBadBehaviour = settings.value(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR, CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR, bool)
    host.showLogs = settings.value(CARLA_KEY_MAIN_SHOW_LOGS, CARLA_DEFAULT_MAIN_SHOW_LOGS, bool) and not CARLA_OS_WIN
    host.showPluginBridges = settings.value(CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES, bool)
    host.showWineBridges = settings.value(CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES, bool)
    host.uiBridgesTimeout = settings.value(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT, CARLA_DEFAULT_UI_BRIDGES_TIMEOUT, int)
    host.uisAlwaysOnTop = settings.value(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP, CARLA_DEFAULT_UIS_ALWAYS_ON_TOP, bool)

    if host.isPlugin:
        return

    host.transportExtra = settings.value(CARLA_KEY_ENGINE_TRANSPORT_EXTRA, "", str)

    # enums
    if host.audioDriverForced is None:
        host.transportMode = settings.value(CARLA_KEY_ENGINE_TRANSPORT_MODE, CARLA_DEFAULT_TRANSPORT_MODE, int)

    if not host.processModeForced:
        host.processMode = settings.value(CARLA_KEY_ENGINE_PROCESS_MODE, CARLA_DEFAULT_PROCESS_MODE, int)

    host.nextProcessMode = host.processMode

    # --------------------------------------------------------------------------------------------------------
    # fix things if needed

    if host.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS:
        if LADISH_APP_NAME:
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            host.nextProcessMode = host.processMode = ENGINE_PROCESS_MODE_SINGLE_CLIENT

        else:
            host.transportMode = ENGINE_TRANSPORT_MODE_JACK

    if gCarla.nogui:
        host.showLogs = False

    # --------------------------------------------------------------------------------------------------------
    # run headless host now if nogui option enabled

    if gCarla.nogui:
        runHostWithoutUI(host)

# ------------------------------------------------------------------------------------------------------------
# Set host settings

def setHostSettings(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    host.set_engine_option(ENGINE_OPTION_FORCE_STEREO,          host.forceStereo,         "")
    host.set_engine_option(ENGINE_OPTION_MAX_PARAMETERS,        host.maxParameters,       "")
    host.set_engine_option(ENGINE_OPTION_RESET_XRUNS,           host.resetXruns,          "")
    host.set_engine_option(ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, host.preferPluginBridges, "")
    host.set_engine_option(ENGINE_OPTION_PREFER_UI_BRIDGES,     host.preferUIBridges,     "")
    host.set_engine_option(ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, host.preventBadBehaviour, "")
    host.set_engine_option(ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    host.uiBridgesTimeout,    "")
    host.set_engine_option(ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     host.uisAlwaysOnTop,      "")

    if host.isPlugin or host.isRemote or host.is_engine_running():
        return

    host.set_engine_option(ENGINE_OPTION_PROCESS_MODE,          host.nextProcessMode,     "")
    host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE,        host.transportMode,       host.transportExtra)
    host.set_engine_option(ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT,  host.showLogs,            "")

    if not (NSM_URL and host.nsmOK):
        host.set_engine_option(ENGINE_OPTION_CLIENT_NAME_PREFIX, 0, gCarla.cnprefix)

# ---------------------------------------------------------------------------------------------------------------------
# Set Engine settings according to carla preferences. Returns selected audio driver.

def setEngineSettings(host, settings, oscPort = None):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    # -----------------------------------------------------------------------------------------------------------------
    # do nothing if control

    if host.isControl:
        return "Control"

    # -----------------------------------------------------------------------------------------------------------------
    # fetch settings as needed

    if settings is None:
        qsettings = QSafeSettings("falkTX", "Carla2")

        if host.audioDriverForced is not None:
            audioDriver = host.audioDriverForced
        else:
            audioDriver = qsettings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, str)

        audioDriverPrefix = CARLA_KEY_ENGINE_DRIVER_PREFIX + audioDriver

        settings = {
            # engine
            CARLA_KEY_ENGINE_AUDIO_DRIVER: audioDriver,
            CARLA_KEY_ENGINE_AUDIO_DEVICE: qsettings.value(audioDriverPrefix+"/Device", "", str),
            CARLA_KEY_ENGINE_BUFFER_SIZE: qsettings.value(audioDriverPrefix+"/BufferSize", CARLA_DEFAULT_AUDIO_BUFFER_SIZE, int),
            CARLA_KEY_ENGINE_SAMPLE_RATE: qsettings.value(audioDriverPrefix+"/SampleRate", CARLA_DEFAULT_AUDIO_SAMPLE_RATE, int),
            CARLA_KEY_ENGINE_TRIPLE_BUFFER: qsettings.value(audioDriverPrefix+"/TripleBuffer", CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER, bool),

            # file paths
            CARLA_KEY_PATHS_AUDIO: splitter.join(qsettings.value(CARLA_KEY_PATHS_AUDIO, CARLA_DEFAULT_FILE_PATH_AUDIO, list)),
            CARLA_KEY_PATHS_MIDI: splitter.join(qsettings.value(CARLA_KEY_PATHS_MIDI,  CARLA_DEFAULT_FILE_PATH_MIDI, list)),

            # plugin paths
            CARLA_KEY_PATHS_LADSPA: splitter.join(qsettings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH, list)),
            CARLA_KEY_PATHS_DSSI: splitter.join(qsettings.value(CARLA_KEY_PATHS_DSSI, CARLA_DEFAULT_DSSI_PATH, list)),
            CARLA_KEY_PATHS_LV2: splitter.join(qsettings.value(CARLA_KEY_PATHS_LV2, CARLA_DEFAULT_LV2_PATH, list)),
            CARLA_KEY_PATHS_VST2: splitter.join(qsettings.value(CARLA_KEY_PATHS_VST2, CARLA_DEFAULT_VST2_PATH, list)),
            CARLA_KEY_PATHS_VST3: splitter.join(qsettings.value(CARLA_KEY_PATHS_VST3, CARLA_DEFAULT_VST3_PATH, list)),
            CARLA_KEY_PATHS_SF2: splitter.join(qsettings.value(CARLA_KEY_PATHS_SF2, CARLA_DEFAULT_SF2_PATH, list)),
            CARLA_KEY_PATHS_SFZ: splitter.join(qsettings.value(CARLA_KEY_PATHS_SFZ, CARLA_DEFAULT_SFZ_PATH, list)),
            CARLA_KEY_PATHS_JSFX: splitter.join(qsettings.value(CARLA_KEY_PATHS_JSFX, CARLA_DEFAULT_JSFX_PATH, list)),
            CARLA_KEY_PATHS_CLAP: splitter.join(qsettings.value(CARLA_KEY_PATHS_CLAP, CARLA_DEFAULT_CLAP_PATH, list)),

            # osc
            CARLA_KEY_OSC_ENABLED: qsettings.value(CARLA_KEY_OSC_ENABLED, CARLA_DEFAULT_OSC_ENABLED, bool),
            CARLA_KEY_OSC_TCP_PORT_ENABLED: qsettings.value(CARLA_KEY_OSC_TCP_PORT_ENABLED, CARLA_DEFAULT_OSC_TCP_PORT_ENABLED, bool),
            CARLA_KEY_OSC_TCP_PORT_RANDOM: qsettings.value(CARLA_KEY_OSC_TCP_PORT_RANDOM, CARLA_DEFAULT_OSC_TCP_PORT_RANDOM, bool),
            CARLA_KEY_OSC_TCP_PORT_NUMBER: qsettings.value(CARLA_KEY_OSC_TCP_PORT_NUMBER, CARLA_DEFAULT_OSC_TCP_PORT_NUMBER, int),
            CARLA_KEY_OSC_UDP_PORT_ENABLED: qsettings.value(CARLA_KEY_OSC_UDP_PORT_ENABLED, CARLA_DEFAULT_OSC_UDP_PORT_ENABLED, bool),
            CARLA_KEY_OSC_UDP_PORT_RANDOM: qsettings.value(CARLA_KEY_OSC_UDP_PORT_RANDOM, CARLA_DEFAULT_OSC_UDP_PORT_RANDOM, bool),
            CARLA_KEY_OSC_UDP_PORT_NUMBER: qsettings.value(CARLA_KEY_OSC_UDP_PORT_NUMBER, CARLA_DEFAULT_OSC_UDP_PORT_NUMBER, int),

            # wine
            CARLA_KEY_WINE_EXECUTABLE: qsettings.value(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE, str),
            CARLA_KEY_WINE_AUTO_PREFIX: qsettings.value(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX, bool),
            CARLA_KEY_WINE_FALLBACK_PREFIX: qsettings.value(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX, str),
            CARLA_KEY_WINE_RT_PRIO_ENABLED: qsettings.value(CARLA_KEY_WINE_RT_PRIO_ENABLED, CARLA_DEFAULT_WINE_RT_PRIO_ENABLED, bool),
            CARLA_KEY_WINE_BASE_RT_PRIO: qsettings.value(CARLA_KEY_WINE_BASE_RT_PRIO, CARLA_DEFAULT_WINE_BASE_RT_PRIO, int),
            CARLA_KEY_WINE_SERVER_RT_PRIO: qsettings.value(CARLA_KEY_WINE_SERVER_RT_PRIO, CARLA_DEFAULT_WINE_SERVER_RT_PRIO, int),
        }

    # -----------------------------------------------------------------------------------------------------------------
    # main settings

    setHostSettings(host)

    # -----------------------------------------------------------------------------------------------------------------
    # file paths

    host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, settings[CARLA_KEY_PATHS_AUDIO])
    host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_MIDI, settings[CARLA_KEY_PATHS_MIDI])

    # -----------------------------------------------------------------------------------------------------------------
    # plugin paths

    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, settings[CARLA_KEY_PATHS_LADSPA])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI, settings[CARLA_KEY_PATHS_DSSI])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2, settings[CARLA_KEY_PATHS_LV2])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST2, settings[CARLA_KEY_PATHS_VST2])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3, settings[CARLA_KEY_PATHS_VST3])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2, settings[CARLA_KEY_PATHS_SF2])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ, settings[CARLA_KEY_PATHS_SFZ])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_JSFX, settings[CARLA_KEY_PATHS_JSFX])
    host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_CLAP, settings[CARLA_KEY_PATHS_CLAP])

    # -----------------------------------------------------------------------------------------------------------------
    # don't continue if plugin

    if host.isPlugin:
        return "Plugin"

    # -----------------------------------------------------------------------------------------------------------------
    # osc settings

    if oscPort is not None and isinstance(oscPort, int):
        oscEnabled = True
        portNumTCP = portNumUDP = oscPort

    else:
        oscEnabled = settings[CARLA_KEY_OSC_ENABLED]

        if not settings[CARLA_KEY_OSC_TCP_PORT_ENABLED]:
            portNumTCP = -1
        elif settings[CARLA_KEY_OSC_TCP_PORT_RANDOM]:
            portNumTCP = 0
        else:
            portNumTCP = settings[CARLA_KEY_OSC_TCP_PORT_NUMBER]

        if not settings[CARLA_KEY_OSC_UDP_PORT_ENABLED]:
            portNumUDP = -1
        elif settings[CARLA_KEY_OSC_UDP_PORT_RANDOM]:
            portNumUDP = 0
        else:
            portNumUDP = settings[CARLA_KEY_OSC_UDP_PORT_NUMBER]

    host.set_engine_option(ENGINE_OPTION_OSC_ENABLED, 1 if oscEnabled else 0, "")
    host.set_engine_option(ENGINE_OPTION_OSC_PORT_TCP, portNumTCP, "")
    host.set_engine_option(ENGINE_OPTION_OSC_PORT_UDP, portNumUDP, "")

    # -----------------------------------------------------------------------------------------------------------------
    # wine settings

    host.set_engine_option(ENGINE_OPTION_WINE_EXECUTABLE, 0, settings[CARLA_KEY_WINE_EXECUTABLE])
    host.set_engine_option(ENGINE_OPTION_WINE_AUTO_PREFIX, 1 if settings[CARLA_KEY_WINE_AUTO_PREFIX] else 0, "")
    host.set_engine_option(ENGINE_OPTION_WINE_FALLBACK_PREFIX, 0, os.path.expanduser(settings[CARLA_KEY_WINE_FALLBACK_PREFIX]))
    host.set_engine_option(ENGINE_OPTION_WINE_RT_PRIO_ENABLED, 1 if settings[CARLA_KEY_WINE_RT_PRIO_ENABLED] else 0, "")
    host.set_engine_option(ENGINE_OPTION_WINE_BASE_RT_PRIO, settings[CARLA_KEY_WINE_BASE_RT_PRIO], "")
    host.set_engine_option(ENGINE_OPTION_WINE_SERVER_RT_PRIO, settings[CARLA_KEY_WINE_SERVER_RT_PRIO], "")

    # -----------------------------------------------------------------------------------------------------------------
    # driver and device settings

    # driver name
    audioDriver = settings[CARLA_KEY_ENGINE_AUDIO_DRIVER]

    # Only setup audio things if engine is not running
    if not host.is_engine_running():
        host.set_engine_option(ENGINE_OPTION_AUDIO_DRIVER, 0, audioDriver)
        host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE, 0, settings[CARLA_KEY_ENGINE_AUDIO_DEVICE])

        if not audioDriver.startswith("JACK"):
            host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, settings[CARLA_KEY_ENGINE_BUFFER_SIZE], "")
            host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, settings[CARLA_KEY_ENGINE_SAMPLE_RATE], "")
            host.set_engine_option(ENGINE_OPTION_AUDIO_TRIPLE_BUFFER, 1 if settings[CARLA_KEY_ENGINE_TRIPLE_BUFFER] else 0, "")

    # -----------------------------------------------------------------------------------------------------------------
    # fix things if needed

    if audioDriver != "JACK" and host.transportMode == ENGINE_TRANSPORT_MODE_JACK:
        host.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL
        host.set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, ENGINE_TRANSPORT_MODE_INTERNAL, host.transportExtra)

    # -----------------------------------------------------------------------------------------------------------------
    # return selected driver name

    return audioDriver

# ---------------------------------------------------------------------------------------------------------------------
# Run Carla without showing UI

def runHostWithoutUI(host):
    # kdevelop likes this :)
    if False: host = CarlaHostNull()

    # --------------------------------------------------------------------------------------------------------
    # Some initial checks

    if not gCarla.nogui:
        return

    projectFile = getInitialProjectFile(True)

    if gCarla.nogui is True:
        oscPort = None

        if not projectFile:
            print("Carla no-gui mode can only be used together with a project file.")
            sys.exit(1)

    else:
        oscPort = gCarla.nogui

    # --------------------------------------------------------------------------------------------------------
    # Additional imports

    from time import sleep

    # --------------------------------------------------------------------------------------------------------
    # Init engine

    audioDriver = setEngineSettings(host, None, oscPort)
    if not host.engine_init(audioDriver, CARLA_CLIENT_NAME or "Carla"):
        print("Engine failed to initialize, possible reasons:\n%s" % host.get_last_error())
        sys.exit(1)

    if projectFile and not host.load_project(projectFile):
        print("Failed to load selected project file, possible reasons:\n%s" % host.get_last_error())
        host.engine_close()
        sys.exit(1)

    # --------------------------------------------------------------------------------------------------------
    # Idle

    print("Carla ready!")

    while host.is_engine_running() and not gCarla.term:
        host.engine_idle()
        sleep(0.0333) # 30 Hz

    # --------------------------------------------------------------------------------------------------------
    # Stop

    host.engine_close()
    sys.exit(0)

# ------------------------------------------------------------------------------------------------------------
