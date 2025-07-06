#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ---------------------------------------------------------------------------------------------------------------------
# Imports (PyQt)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSlot, QT_VERSION, Qt
    from PyQt5.QtWidgets import QDialog, QDialogButtonBox, QFileDialog
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSlot, QT_VERSION, Qt
    from PyQt6.QtWidgets import QDialog, QDialogButtonBox, QFileDialog

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_settings
import ui_carla_settings_driver

from carla_backend import (
    CARLA_OS_LINUX,
    CARLA_OS_MAC,
    CARLA_OS_WIN,
    ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL,
    ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER,
    ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE,
    ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE,
    ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
    ENGINE_OPTION_FILE_PATH,
    ENGINE_OPTION_PLUGIN_PATH,
    FILE_AUDIO,
    FILE_MIDI,
    PLUGIN_LADSPA,
    PLUGIN_DSSI,
    PLUGIN_LV2,
    PLUGIN_VST2,
    PLUGIN_VST3,
    PLUGIN_SF2,
    PLUGIN_SFZ,
    PLUGIN_JSFX,
    PLUGIN_CLAP,
)

from carla_shared import (
    CARLA_KEY_MAIN_PROJECT_FOLDER,
    CARLA_KEY_MAIN_USE_PRO_THEME,
    CARLA_KEY_MAIN_PRO_THEME_COLOR,
    CARLA_KEY_MAIN_REFRESH_INTERVAL,
    CARLA_KEY_MAIN_CONFIRM_EXIT,
    CARLA_KEY_MAIN_CLASSIC_SKIN,
    CARLA_KEY_MAIN_SHOW_LOGS,
    CARLA_KEY_MAIN_SYSTEM_ICONS,
    CARLA_KEY_MAIN_EXPERIMENTAL,
    CARLA_KEY_CANVAS_THEME,
    CARLA_KEY_CANVAS_SIZE,
    CARLA_KEY_CANVAS_USE_BEZIER_LINES,
    CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,
    CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS,
    CARLA_KEY_CANVAS_EYE_CANDY,
    CARLA_KEY_CANVAS_FANCY_EYE_CANDY,
    CARLA_KEY_CANVAS_USE_OPENGL,
    CARLA_KEY_CANVAS_ANTIALIASING,
    CARLA_KEY_CANVAS_HQ_ANTIALIASING,
    CARLA_KEY_CANVAS_INLINE_DISPLAYS,
    CARLA_KEY_CANVAS_FULL_REPAINTS,
    CARLA_KEY_ENGINE_DRIVER_PREFIX,
    CARLA_KEY_ENGINE_AUDIO_DRIVER,
    CARLA_KEY_ENGINE_PROCESS_MODE,
    CARLA_KEY_ENGINE_FORCE_STEREO,
    CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES,
    CARLA_KEY_ENGINE_PREFER_UI_BRIDGES,
    CARLA_KEY_ENGINE_MANAGE_UIS,
    CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP,
    CARLA_KEY_ENGINE_MAX_PARAMETERS,
    CARLA_KEY_ENGINE_RESET_XRUNS,
    CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT,
    CARLA_KEY_OSC_ENABLED,
    CARLA_KEY_OSC_TCP_PORT_ENABLED,
    CARLA_KEY_OSC_TCP_PORT_NUMBER,
    CARLA_KEY_OSC_TCP_PORT_RANDOM,
    CARLA_KEY_OSC_UDP_PORT_ENABLED,
    CARLA_KEY_OSC_UDP_PORT_NUMBER,
    CARLA_KEY_OSC_UDP_PORT_RANDOM,
    CARLA_KEY_PATHS_AUDIO,
    CARLA_KEY_PATHS_MIDI,
    CARLA_KEY_PATHS_LADSPA,
    CARLA_KEY_PATHS_DSSI,
    CARLA_KEY_PATHS_LV2,
    CARLA_KEY_PATHS_VST2,
    CARLA_KEY_PATHS_VST3,
    CARLA_KEY_PATHS_SF2,
    CARLA_KEY_PATHS_SFZ,
    CARLA_KEY_PATHS_JSFX,
    CARLA_KEY_PATHS_CLAP,
    CARLA_KEY_WINE_EXECUTABLE,
    CARLA_KEY_WINE_AUTO_PREFIX,
    CARLA_KEY_WINE_FALLBACK_PREFIX,
    CARLA_KEY_WINE_RT_PRIO_ENABLED,
    CARLA_KEY_WINE_BASE_RT_PRIO,
    CARLA_KEY_WINE_SERVER_RT_PRIO,
    CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES,
    CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES,
    CARLA_KEY_EXPERIMENTAL_JACK_APPS,
    CARLA_KEY_EXPERIMENTAL_EXPORT_LV2,
    CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR,
    CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL,
    CARLA_DEFAULT_MAIN_PROJECT_FOLDER,
    CARLA_DEFAULT_MAIN_USE_PRO_THEME,
    CARLA_DEFAULT_MAIN_PRO_THEME_COLOR,
    CARLA_DEFAULT_MAIN_REFRESH_INTERVAL,
    CARLA_DEFAULT_MAIN_CONFIRM_EXIT,
    CARLA_DEFAULT_MAIN_CLASSIC_SKIN,
    CARLA_DEFAULT_MAIN_SHOW_LOGS,
    CARLA_DEFAULT_MAIN_SYSTEM_ICONS,
    #CARLA_DEFAULT_MAIN_EXPERIMENTAL,
    CARLA_DEFAULT_CANVAS_THEME,
    CARLA_DEFAULT_CANVAS_SIZE,
    CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES,
    CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS,
    CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS,
    CARLA_DEFAULT_CANVAS_EYE_CANDY,
    CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY,
    CARLA_DEFAULT_CANVAS_USE_OPENGL,
    CARLA_DEFAULT_CANVAS_ANTIALIASING,
    CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING,
    CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS,
    CARLA_DEFAULT_CANVAS_FULL_REPAINTS,
    CARLA_DEFAULT_FORCE_STEREO,
    CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES,
    CARLA_DEFAULT_PREFER_UI_BRIDGES,
    CARLA_DEFAULT_MANAGE_UIS,
    CARLA_DEFAULT_UIS_ALWAYS_ON_TOP,
    CARLA_DEFAULT_MAX_PARAMETERS,
    CARLA_DEFAULT_RESET_XRUNS,
    CARLA_DEFAULT_UI_BRIDGES_TIMEOUT,
    CARLA_DEFAULT_AUDIO_BUFFER_SIZE,
    CARLA_DEFAULT_AUDIO_SAMPLE_RATE,
    CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER,
    CARLA_DEFAULT_AUDIO_DRIVER,
    CARLA_DEFAULT_PROCESS_MODE,
    CARLA_DEFAULT_OSC_ENABLED,
    CARLA_DEFAULT_OSC_TCP_PORT_ENABLED,
    CARLA_DEFAULT_OSC_TCP_PORT_NUMBER,
    CARLA_DEFAULT_OSC_TCP_PORT_RANDOM,
    CARLA_DEFAULT_OSC_UDP_PORT_ENABLED,
    CARLA_DEFAULT_OSC_UDP_PORT_NUMBER,
    CARLA_DEFAULT_OSC_UDP_PORT_RANDOM,
    CARLA_DEFAULT_WINE_EXECUTABLE,
    CARLA_DEFAULT_WINE_AUTO_PREFIX,
    CARLA_DEFAULT_WINE_FALLBACK_PREFIX,
    CARLA_DEFAULT_WINE_RT_PRIO_ENABLED,
    CARLA_DEFAULT_WINE_BASE_RT_PRIO,
    CARLA_DEFAULT_WINE_SERVER_RT_PRIO,
    CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES,
    CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES,
    CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS,
    CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT,
    CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR,
    CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL,
    CARLA_DEFAULT_FILE_PATH_AUDIO,
    CARLA_DEFAULT_FILE_PATH_MIDI,
    CARLA_DEFAULT_LADSPA_PATH,
    CARLA_DEFAULT_DSSI_PATH,
    CARLA_DEFAULT_LV2_PATH,
    CARLA_DEFAULT_VST2_PATH,
    CARLA_DEFAULT_VST3_PATH,
    CARLA_DEFAULT_SF2_PATH,
    CARLA_DEFAULT_SFZ_PATH,
    CARLA_DEFAULT_JSFX_PATH,
    CARLA_DEFAULT_CLAP_PATH,
    getAndSetPath,
    getIcon,
    fontMetricsHorizontalAdvance,
    splitter,
)

from patchcanvas.theme import Theme, getThemeName

from utils import QSafeSettings

# ---------------------------------------------------------------------------------------------------------------------
# ...

BUFFER_SIZE_LIST = (16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192)
SAMPLE_RATE_LIST = (22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000)

# ---------------------------------------------------------------------------------------------------------------------
# Driver Settings

class DriverSettingsW(QDialog):
    AUTOMATIC_OPTION = "(Auto)"

    def __init__(self, parent, host, driverIndex, driverName):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_settings_driver.Ui_DriverSettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------------------------------------------------------
        # Internal stuff

        self.fDriverIndex = driverIndex
        self.fDriverName  = driverName
        self.fDeviceNames = host.get_engine_driver_device_names(driverIndex)

        self.fBufferSizes = BUFFER_SIZE_LIST
        self.fSampleRates = SAMPLE_RATE_LIST

        # -------------------------------------------------------------------------------------------------------------
        # Set-up GUI

        for name in self.fDeviceNames:
            self.ui.cb_device.addItem(name)

        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        if CARLA_OS_MAC:
            self.setWindowModality(Qt.WindowModal)

        # -------------------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Set-up connections

        self.accepted.connect(self.slot_saveSettings)
        self.ui.b_panel.clicked.connect(self.slot_showDevicePanel)
        self.ui.cb_device.currentIndexChanged.connect(self.slot_updateDeviceInfo)

    # -----------------------------------------------------------------------------------------------------------------

    def getValues(self):
        audioDevice = self.ui.cb_device.currentText()
        bufferSize  = self.ui.cb_buffersize.currentText()
        sampleRate  = self.ui.cb_samplerate.currentText()

        if bufferSize == self.AUTOMATIC_OPTION:
            bufferSize = "0"
        if sampleRate == self.AUTOMATIC_OPTION:
            sampleRate = "0"

        return (audioDevice, int(bufferSize), int(sampleRate))

    # -----------------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSafeSettings("falkTX", "Carla2")

        args = (CARLA_KEY_ENGINE_DRIVER_PREFIX, self.fDriverName)
        audioDevice       = settings.value("%s%s/Device"       % args, "",                                str)
        audioBufferSize   = settings.value("%s%s/BufferSize"   % args, CARLA_DEFAULT_AUDIO_BUFFER_SIZE,   int)
        audioSampleRate   = settings.value("%s%s/SampleRate"   % args, CARLA_DEFAULT_AUDIO_SAMPLE_RATE,   int)
        audioTripleBuffer = settings.value("%s%s/TripleBuffer" % args, CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER, bool)

        if audioDevice and audioDevice in self.fDeviceNames:
            self.ui.cb_device.setCurrentIndex(self.fDeviceNames.index(audioDevice))
        else:
            self.ui.cb_device.setCurrentIndex(-1)

        # fill combo-boxes first
        self.slot_updateDeviceInfo()

        if audioBufferSize and audioBufferSize in self.fBufferSizes:
            self.ui.cb_buffersize.setCurrentIndex(self.fBufferSizes.index(audioBufferSize))
        elif self.fBufferSizes == BUFFER_SIZE_LIST:
            self.ui.cb_buffersize.setCurrentIndex(BUFFER_SIZE_LIST.index(CARLA_DEFAULT_AUDIO_BUFFER_SIZE))
        else:
            self.ui.cb_buffersize.setCurrentIndex(int(len(self.fBufferSizes)/2))

        if audioSampleRate and audioSampleRate in self.fSampleRates:
            self.ui.cb_samplerate.setCurrentIndex(self.fSampleRates.index(audioSampleRate))
        elif self.fSampleRates == SAMPLE_RATE_LIST:
            self.ui.cb_samplerate.setCurrentIndex(SAMPLE_RATE_LIST.index(CARLA_DEFAULT_AUDIO_SAMPLE_RATE))
        else:
            self.ui.cb_samplerate.setCurrentIndex(int(len(self.fSampleRates)/2))

        self.ui.cb_triple_buffer.setChecked(audioTripleBuffer and self.ui.cb_triple_buffer.isEnabled())

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSafeSettings("falkTX", "Carla2")

        bufferSize = self.ui.cb_buffersize.currentText()
        sampleRate = self.ui.cb_samplerate.currentText()

        if bufferSize == self.AUTOMATIC_OPTION:
            bufferSize = "0"
        if sampleRate == self.AUTOMATIC_OPTION:
            sampleRate = "0"

        args = (CARLA_KEY_ENGINE_DRIVER_PREFIX, self.fDriverName)
        settings.setValue("%s%s/Device"       % args, self.ui.cb_device.currentText())
        settings.setValue("%s%s/BufferSize"   % args, bufferSize)
        settings.setValue("%s%s/SampleRate"   % args, sampleRate)
        settings.setValue("%s%s/TripleBuffer" % args, self.ui.cb_triple_buffer.isChecked())

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_showDevicePanel(self):
        self.host.show_engine_driver_device_control_panel(self.fDriverIndex, self.ui.cb_device.currentText())

    @pyqtSlot()
    def slot_updateDeviceInfo(self):
        deviceName = self.ui.cb_device.currentText()

        oldBufferSize = self.ui.cb_buffersize.currentText()
        oldSampleRate = self.ui.cb_samplerate.currentText()

        self.ui.cb_buffersize.clear()
        self.ui.cb_samplerate.clear()

        driverDeviceInfo  = self.host.get_engine_driver_device_info(self.fDriverIndex, deviceName)
        driverDeviceHints = driverDeviceInfo['hints']
        self.fBufferSizes = driverDeviceInfo['bufferSizes']
        self.fSampleRates = driverDeviceInfo['sampleRates']

        if driverDeviceHints & ENGINE_DRIVER_DEVICE_CAN_TRIPLE_BUFFER:
            self.ui.cb_triple_buffer.setEnabled(True)
        else:
            self.ui.cb_triple_buffer.setEnabled(False)

        if driverDeviceHints & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL:
            self.ui.b_panel.setEnabled(True)
        else:
            self.ui.b_panel.setEnabled(False)

        if self.fBufferSizes:
            match = False
            for bsize in self.fBufferSizes:
                sbsize = str(bsize)
                self.ui.cb_buffersize.addItem(sbsize)

                if oldBufferSize == sbsize:
                    match = True
                    self.ui.cb_buffersize.setCurrentIndex(self.ui.cb_buffersize.count()-1)

            if not match:
                if CARLA_DEFAULT_AUDIO_BUFFER_SIZE in self.fBufferSizes:
                    self.ui.cb_buffersize.setCurrentIndex(self.fBufferSizes.index(CARLA_DEFAULT_AUDIO_BUFFER_SIZE))
                else:
                    self.ui.cb_buffersize.setCurrentIndex(int(len(self.fBufferSizes)/2))

        else:
            self.ui.cb_buffersize.addItem(self.AUTOMATIC_OPTION)
            self.ui.cb_buffersize.setCurrentIndex(0)

        if self.fSampleRates:
            match = False
            for srate in self.fSampleRates:
                ssrate = str(int(srate))
                self.ui.cb_samplerate.addItem(ssrate)

                if oldSampleRate == ssrate:
                    match = True
                    self.ui.cb_samplerate.setCurrentIndex(self.ui.cb_samplerate.count()-1)

            if not match:
                if CARLA_DEFAULT_AUDIO_SAMPLE_RATE in self.fSampleRates:
                    self.ui.cb_samplerate.setCurrentIndex(self.fSampleRates.index(CARLA_DEFAULT_AUDIO_SAMPLE_RATE))
                else:
                    self.ui.cb_samplerate.setCurrentIndex(int(len(self.fSampleRates)/2))

        else:
            self.ui.cb_samplerate.addItem(self.AUTOMATIC_OPTION)
            self.ui.cb_samplerate.setCurrentIndex(0)

# ---------------------------------------------------------------------------------------------------------------------
# Runtime Driver Settings

class RuntimeDriverSettingsW(QDialog):
    def __init__(self, parent, host):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_settings_driver.Ui_DriverSettingsW()
        self.ui.setupUi(self)

        driverDeviceInfo = host.get_runtime_engine_driver_device_info()

        # -------------------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.ui.cb_device.clear()
        self.ui.cb_buffersize.clear()
        self.ui.cb_samplerate.clear()
        self.ui.cb_triple_buffer.hide()
        self.ui.ico_restart.hide()
        self.ui.label_restart.hide()

        self.ui.layout_triple_buffer.takeAt(2)
        self.ui.layout_triple_buffer.takeAt(1)
        self.ui.layout_triple_buffer.takeAt(0)
        self.ui.verticalLayout.removeItem(self.ui.layout_triple_buffer)

        self.ui.layout_restart.takeAt(3)
        self.ui.layout_restart.takeAt(2)
        self.ui.layout_restart.takeAt(1)
        self.ui.layout_restart.takeAt(0)
        self.ui.verticalLayout.removeItem(self.ui.layout_restart)

        self.adjustSize()
        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        if CARLA_OS_MAC:
            self.setWindowModality(Qt.WindowModal)

        # -------------------------------------------------------------------------------------------------------------
        # Load runtime settings

        if host.is_engine_running():
            self.ui.cb_device.addItem(driverDeviceInfo['name'])
            self.ui.cb_device.setCurrentIndex(0)
            self.ui.cb_device.setEnabled(False)
        else:
            self.ui.cb_device.addItem(driverDeviceInfo['name'])
            self.ui.cb_device.setCurrentIndex(0)

        if driverDeviceInfo['bufferSizes']:
            for bsize in driverDeviceInfo['bufferSizes']:
                sbsize = str(bsize)
                self.ui.cb_buffersize.addItem(sbsize)

                if driverDeviceInfo['bufferSize'] == bsize:
                    self.ui.cb_buffersize.setCurrentIndex(self.ui.cb_buffersize.count()-1)

        else:
            self.ui.cb_buffersize.addItem(DriverSettingsW.AUTOMATIC_OPTION)
            self.ui.cb_buffersize.setCurrentIndex(0)

        if (driverDeviceInfo['hints'] & ENGINE_DRIVER_DEVICE_VARIABLE_BUFFER_SIZE) == 0x0:
            self.ui.cb_buffersize.setEnabled(False)

        if driverDeviceInfo['sampleRates']:
            for srate in driverDeviceInfo['sampleRates']:
                ssrate = str(int(srate))
                self.ui.cb_samplerate.addItem(ssrate)

                if driverDeviceInfo['sampleRate'] == srate:
                    self.ui.cb_samplerate.setCurrentIndex(self.ui.cb_samplerate.count()-1)

        else:
            self.ui.cb_samplerate.addItem(DriverSettingsW.AUTOMATIC_OPTION)
            self.ui.cb_samplerate.setCurrentIndex(0)

        if (driverDeviceInfo['hints'] & ENGINE_DRIVER_DEVICE_VARIABLE_SAMPLE_RATE) == 0x0:
            self.ui.cb_samplerate.setEnabled(False)

        if (driverDeviceInfo['hints'] & ENGINE_DRIVER_DEVICE_HAS_CONTROL_PANEL) == 0x0:
            self.ui.b_panel.setEnabled(False)

        # -------------------------------------------------------------------------------------------------------------
        # Set-up connections

        self.ui.b_panel.clicked.connect(self.slot_showDevicePanel)

    # -----------------------------------------------------------------------------------------------------------------

    def getValues(self):
        audioDevice = self.ui.cb_device.currentText()
        bufferSize  = self.ui.cb_buffersize.currentText()
        sampleRate  = self.ui.cb_samplerate.currentText()

        if bufferSize == DriverSettingsW.AUTOMATIC_OPTION:
            bufferSize = "0"
        if sampleRate == DriverSettingsW.AUTOMATIC_OPTION:
            sampleRate = "0"

        return (audioDevice, int(bufferSize), int(sampleRate))

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_showDevicePanel(self):
        self.host.show_engine_device_control_panel()

# ---------------------------------------------------------------------------------------------------------------------
# Settings Dialog

class CarlaSettingsW(QDialog):
    # Tab indexes
    TAB_INDEX_MAIN         = 0
    TAB_INDEX_CANVAS       = 1
    TAB_INDEX_ENGINE       = 2
    TAB_INDEX_OSC          = 3
    TAB_INDEX_FILEPATHS    = 4
    TAB_INDEX_PLUGINPATHS  = 5
    TAB_INDEX_WINE         = 6
    TAB_INDEX_EXPERIMENTAL = 7
    TAB_INDEX_NONE         = 8

    # File Path indexes
    FILEPATH_INDEX_AUDIO = 0
    FILEPATH_INDEX_MIDI  = 1

    # Plugin Path indexes
    PLUGINPATH_INDEX_LADSPA = 0
    PLUGINPATH_INDEX_DSSI   = 1
    PLUGINPATH_INDEX_LV2    = 2
    PLUGINPATH_INDEX_VST2   = 3
    PLUGINPATH_INDEX_VST3   = 4
    PLUGINPATH_INDEX_SF2    = 5
    PLUGINPATH_INDEX_SFZ    = 6
    PLUGINPATH_INDEX_JSFX   = 7
    PLUGINPATH_INDEX_CLAP   = 8

    # Single and Multiple client mode is only for JACK,
    # but we still want to match QComboBox index to backend defines,
    # so add +2 pos padding if driverName != "JACK".
    PROCESS_MODE_NON_JACK_PADDING = 2

    # -----------------------------------------------------------------------------------------------------------------

    def __init__(self, parent, host, hasCanvas, hasCanvasGL):
        QDialog.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_settings.Ui_CarlaSettingsW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------------------------------------------------------
        # Set-up GUI

        self.ui.lw_page.setFixedWidth(48 + 6*3 +
                                      fontMetricsHorizontalAdvance(self.ui.lw_page.fontMetrics(), "  Experimental  "))

        for i in range(host.get_engine_driver_count()):
            self.ui.cb_engine_audio_driver.addItem(host.get_engine_driver_name(i))

        for i in range(Theme.THEME_MAX):
            self.ui.cb_canvas_theme.addItem(getThemeName(i))

        if CARLA_OS_MAC:
            self.ui.group_main_theme.setEnabled(False)
            self.ui.group_main_theme.setVisible(False)

        if CARLA_OS_WIN or host.isControl:
            self.ui.ch_main_show_logs.setEnabled(False)
            self.ui.ch_main_show_logs.setVisible(False)

        if host.isControl:
            self.ui.lw_page.hideRow(self.TAB_INDEX_ENGINE)
            self.ui.lw_page.hideRow(self.TAB_INDEX_FILEPATHS)
            self.ui.lw_page.hideRow(self.TAB_INDEX_PLUGINPATHS)
            self.ui.ch_exp_export_lv2.hide()
            self.ui.group_experimental_engine.hide()
            self.ui.cb_canvas_inline_displays.hide()

        elif not hasCanvas:
            self.ui.lw_page.hideRow(self.TAB_INDEX_CANVAS)

        elif not hasCanvasGL:
            self.ui.cb_canvas_use_opengl.setEnabled(False)
            self.ui.cb_canvas_render_hq_aa.setEnabled(False)

        if host.isPlugin:
            self.ui.cb_engine_audio_driver.setEnabled(False)

        if host.audioDriverForced is not None:
            self.ui.cb_engine_audio_driver.setEnabled(False)
            self.ui.tb_engine_driver_config.setEnabled(False)

        if host.processModeForced:
            self.ui.cb_engine_process_mode_jack.setEnabled(False)
            self.ui.cb_engine_process_mode_other.setEnabled(False)

            if self.host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK:
                self.ui.ch_engine_force_stereo.setEnabled(False)

        if host.isControl or host.isPlugin:
            self.ui.ch_main_confirm_exit.hide()
            self.ui.ch_exp_load_lib_global.hide()
            self.ui.lw_page.hideRow(self.TAB_INDEX_OSC)
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

        if not CARLA_OS_LINUX:
            self.ui.ch_exp_wine_bridges.setVisible(False)
            self.ui.ch_exp_prevent_bad_behaviour.setVisible(False)
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

        if not CARLA_OS_MAC:
            self.ui.label_engine_ui_bridges_mac_note.setVisible(False)

        if not (CARLA_OS_LINUX or CARLA_OS_MAC):
            self.ui.ch_exp_jack_apps.setVisible(False)

        # FIXME, not implemented yet
        self.ui.ch_engine_uis_always_on_top.hide()

        # FIXME broken in some Qt5 versions
        self.ui.cb_canvas_eyecandy.setEnabled(QT_VERSION >= 0x50c00)

        # removed in Qt6
        if QT_VERSION >= 0x60000:
            self.ui.cb_canvas_render_hq_aa.hide()

        self.setWindowFlags(self.windowFlags() & ~Qt.WindowContextHelpButtonHint)

        if CARLA_OS_MAC:
            self.setWindowModality(Qt.WindowModal)

        # -------------------------------------------------------------------------------------------------------------
        # Load settings

        self.loadSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Set-up Icons

        if self.ui.ch_main_system_icons.isChecked():
            self.ui.b_main_proj_folder_open.setIcon(getIcon('document-open', 16, 'svgz'))
            self.ui.b_filepaths_add.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.b_filepaths_change.setIcon(getIcon('edit-rename', 16, 'svgz'))
            self.ui.b_filepaths_remove.setIcon(getIcon('list-remove', 16, 'svgz'))
            self.ui.b_paths_add.setIcon(getIcon('list-add', 16, 'svgz'))
            self.ui.b_paths_change.setIcon(getIcon('edit-rename', 16, 'svgz'))
            self.ui.b_paths_remove.setIcon(getIcon('list-remove', 16, 'svgz'))

        # -------------------------------------------------------------------------------------------------------------
        # Set-up connections

        self.accepted.connect(self.slot_saveSettings)
        self.ui.buttonBox.button(QDialogButtonBox.Reset).clicked.connect(self.slot_resetSettings)

        self.ui.b_main_proj_folder_open.clicked.connect(self.slot_getAndSetProjectPath)

        self.ui.cb_engine_audio_driver.currentIndexChanged.connect(self.slot_engineAudioDriverChanged)
        self.ui.tb_engine_driver_config.clicked.connect(self.slot_showAudioDriverSettings)

        self.ui.b_paths_add.clicked.connect(self.slot_addPluginPath)
        self.ui.b_paths_remove.clicked.connect(self.slot_removePluginPath)
        self.ui.b_paths_change.clicked.connect(self.slot_changePluginPath)
        self.ui.cb_paths.currentIndexChanged.connect(self.slot_pluginPathTabChanged)
        self.ui.lw_ladspa.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_dssi.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_lv2.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_vst.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_vst3.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_sf2.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_sfz.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_jsfx.currentRowChanged.connect(self.slot_pluginPathRowChanged)
        self.ui.lw_clap.currentRowChanged.connect(self.slot_pluginPathRowChanged)

        self.ui.b_filepaths_add.clicked.connect(self.slot_addFilePath)
        self.ui.b_filepaths_remove.clicked.connect(self.slot_removeFilePath)
        self.ui.b_filepaths_change.clicked.connect(self.slot_changeFilePath)
        self.ui.cb_filepaths.currentIndexChanged.connect(self.slot_filePathTabChanged)
        self.ui.lw_files_audio.currentRowChanged.connect(self.slot_filePathRowChanged)
        self.ui.lw_files_midi.currentRowChanged.connect(self.slot_filePathRowChanged)

        self.ui.ch_main_experimental.toggled.connect(self.slot_enableExperimental)
        self.ui.ch_exp_wine_bridges.toggled.connect(self.slot_enableWineBridges)
        self.ui.cb_exp_plugin_bridges.toggled.connect(self.slot_pluginBridgesToggled)
        self.ui.cb_canvas_eyecandy.toggled.connect(self.slot_canvasEyeCandyToggled)
        #self.ui.cb_canvas_fancy_eyecandy.toggled.connect(self.slot_canvasFancyEyeCandyToggled)
        self.ui.cb_canvas_use_opengl.toggled.connect(self.slot_canvasOpenGLToggled)

        # -------------------------------------------------------------------------------------------------------------
        # Post-connect setup

        self.ui.lw_ladspa.setCurrentRow(0)
        self.ui.lw_dssi.setCurrentRow(0)
        self.ui.lw_lv2.setCurrentRow(0)
        self.ui.lw_vst.setCurrentRow(0)
        self.ui.lw_vst3.setCurrentRow(0)
        self.ui.lw_sf2.setCurrentRow(0)
        self.ui.lw_sfz.setCurrentRow(0)
        self.ui.lw_jsfx.setCurrentRow(0)
        self.ui.lw_clap.setCurrentRow(0)

        self.ui.lw_files_audio.setCurrentRow(0)
        self.ui.lw_files_midi.setCurrentRow(0)

        self.ui.lw_page.setCurrentCell(0, 0)

        self.slot_filePathTabChanged(0)
        self.slot_pluginPathTabChanged(0)

        self.adjustSize()

    # -----------------------------------------------------------------------------------------------------------------

    def loadSettings(self):
        settings = QSafeSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Main

        self.ui.ch_main_show_logs.setChecked(self.host.showLogs)
        self.ui.ch_engine_uis_always_on_top.setChecked(self.host.uisAlwaysOnTop)

        self.ui.le_main_proj_folder.setText(
            settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER, CARLA_DEFAULT_MAIN_PROJECT_FOLDER, str))

        self.ui.ch_main_theme_pro.setChecked(self.ui.group_main_theme.isEnabled() and
                                             settings.value(CARLA_KEY_MAIN_USE_PRO_THEME,
                                                            CARLA_DEFAULT_MAIN_USE_PRO_THEME, bool))

        self.ui.cb_main_theme_color.setCurrentIndex(
            self.ui.cb_main_theme_color.findText(settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR,
                                                                CARLA_DEFAULT_MAIN_PRO_THEME_COLOR, str)))

        self.ui.sb_main_refresh_interval.setValue(
            settings.value(CARLA_KEY_MAIN_REFRESH_INTERVAL, CARLA_DEFAULT_MAIN_REFRESH_INTERVAL, int))

        self.ui.ch_main_confirm_exit.setChecked(
            settings.value(CARLA_KEY_MAIN_CONFIRM_EXIT, CARLA_DEFAULT_MAIN_CONFIRM_EXIT, bool))

        self.ui.cb_main_classic_skin_default.setChecked(
            settings.value(CARLA_KEY_MAIN_CLASSIC_SKIN, CARLA_DEFAULT_MAIN_CLASSIC_SKIN, bool))

        self.ui.ch_main_system_icons.setChecked(
            settings.value(CARLA_KEY_MAIN_SYSTEM_ICONS, CARLA_DEFAULT_MAIN_SYSTEM_ICONS, bool))

        # -------------------------------------------------------------------------------------------------------------
        # Canvas

        self.ui.cb_canvas_theme.setCurrentIndex(
            self.ui.cb_canvas_theme.findText(settings.value(CARLA_KEY_CANVAS_THEME, CARLA_DEFAULT_CANVAS_THEME, str)))

        self.ui.cb_canvas_size.setCurrentIndex(
            self.ui.cb_canvas_size.findText(settings.value(CARLA_KEY_CANVAS_SIZE, CARLA_DEFAULT_CANVAS_SIZE, str)))

        self.ui.cb_canvas_bezier_lines.setChecked(
            settings.value(CARLA_KEY_CANVAS_USE_BEZIER_LINES, CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES, bool))

        self.ui.cb_canvas_hide_groups.setChecked(
            settings.value(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS, CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS, bool))

        self.ui.cb_canvas_auto_select.setChecked(
            settings.value(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS, bool))

        if self.ui.cb_canvas_eyecandy.isEnabled():
            self.ui.cb_canvas_eyecandy.setChecked(
                settings.value(CARLA_KEY_CANVAS_EYE_CANDY, CARLA_DEFAULT_CANVAS_EYE_CANDY, bool))

        self.ui.cb_canvas_fancy_eyecandy.setChecked(
            settings.value(CARLA_KEY_CANVAS_FANCY_EYE_CANDY, CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY, bool))

        self.ui.cb_canvas_use_opengl.setChecked(self.ui.cb_canvas_use_opengl.isEnabled() and
                                                settings.value(CARLA_KEY_CANVAS_USE_OPENGL,
                                                               CARLA_DEFAULT_CANVAS_USE_OPENGL, bool))

        self.ui.cb_canvas_render_aa.setCheckState(
            Qt.CheckState(settings.value(CARLA_KEY_CANVAS_ANTIALIASING, CARLA_DEFAULT_CANVAS_ANTIALIASING, int)))

        self.ui.cb_canvas_render_hq_aa.setChecked(self.ui.cb_canvas_render_hq_aa.isEnabled() and
                                                  settings.value(CARLA_KEY_CANVAS_HQ_ANTIALIASING,
                                                                 CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING, bool))

        self.ui.cb_canvas_full_repaints.setChecked(
            settings.value(CARLA_KEY_CANVAS_FULL_REPAINTS, CARLA_DEFAULT_CANVAS_FULL_REPAINTS, bool))

        self.ui.cb_canvas_inline_displays.setChecked(
            settings.value(CARLA_KEY_CANVAS_INLINE_DISPLAYS, CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS, bool))

        # -------------------------------------------------------------------------------------------------------------

        settings = QSafeSettings("falkTX", "Carla2")

        # -------------------------------------------------------------------------------------------------------------
        # Main

        self.ui.ch_main_experimental.setChecked(self.host.experimental)

        if not self.host.experimental:
            self.ui.lw_page.hideRow(self.TAB_INDEX_EXPERIMENTAL)
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

        elif not self.host.showWineBridges:
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

        # -------------------------------------------------------------------------------------------------------------
        # Engine

        if self.host.isPlugin:
            audioDriver = "Plugin"
            self.ui.cb_engine_audio_driver.setCurrentIndex(0)
        elif self.host.audioDriverForced:
            audioDriver = self.host.audioDriverForced
            self.ui.cb_engine_audio_driver.setCurrentIndex(0)
        else:
            audioDriver = settings.value(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER, str)

            for i in range(self.ui.cb_engine_audio_driver.count()):
                if self.ui.cb_engine_audio_driver.itemText(i) == audioDriver:
                    self.ui.cb_engine_audio_driver.setCurrentIndex(i)
                    break
            else:
                self.ui.cb_engine_audio_driver.setCurrentIndex(-1)

        if audioDriver == "JACK":
            self.ui.sw_engine_process_mode.setCurrentIndex(0)
        else:
            self.ui.sw_engine_process_mode.setCurrentIndex(1)

        self.ui.tb_engine_driver_config.setEnabled(self.host.audioDriverForced is None and not self.host.isPlugin)

        self.ui.cb_engine_process_mode_jack.setCurrentIndex(self.host.nextProcessMode)

        if self.host.nextProcessMode >= self.PROCESS_MODE_NON_JACK_PADDING:
            self.ui.cb_engine_process_mode_other.setCurrentIndex(self.host.nextProcessMode -
                                                                 self.PROCESS_MODE_NON_JACK_PADDING)
        else:
            self.ui.cb_engine_process_mode_other.setCurrentIndex(0)

        self.ui.sb_engine_max_params.setValue(self.host.maxParameters)
        self.ui.cb_engine_reset_xruns.setChecked(self.host.resetXruns)
        self.ui.ch_engine_manage_uis.setChecked(self.host.manageUIs)
        self.ui.ch_engine_prefer_ui_bridges.setChecked(self.host.preferUIBridges)
        self.ui.sb_engine_ui_bridges_timeout.setValue(self.host.uiBridgesTimeout)
        self.ui.ch_engine_force_stereo.setChecked(self.host.forceStereo or
                                                  not self.ui.ch_engine_force_stereo.isEnabled())
        self.ui.ch_engine_prefer_plugin_bridges.setChecked(self.host.preferPluginBridges)
        self.ui.ch_exp_export_lv2.setChecked(self.host.exportLV2)
        self.ui.cb_exp_plugin_bridges.setChecked(self.host.showPluginBridges)
        self.ui.ch_exp_wine_bridges.setChecked(self.host.showWineBridges)

        # -------------------------------------------------------------------------------------------------------------
        # OSC

        self.ui.ch_osc_enable.setChecked(
            settings.value(CARLA_KEY_OSC_ENABLED, CARLA_DEFAULT_OSC_ENABLED, bool))

        self.ui.group_osc_tcp_port.setChecked(
            settings.value(CARLA_KEY_OSC_TCP_PORT_ENABLED, CARLA_DEFAULT_OSC_TCP_PORT_ENABLED, bool))

        self.ui.group_osc_udp_port.setChecked(
            settings.value(CARLA_KEY_OSC_UDP_PORT_ENABLED, CARLA_DEFAULT_OSC_UDP_PORT_ENABLED, bool))

        self.ui.sb_osc_tcp_port_number.setValue(
            settings.value(CARLA_KEY_OSC_TCP_PORT_NUMBER, CARLA_DEFAULT_OSC_TCP_PORT_NUMBER, int))

        self.ui.sb_osc_udp_port_number.setValue(
            settings.value(CARLA_KEY_OSC_UDP_PORT_NUMBER, CARLA_DEFAULT_OSC_UDP_PORT_NUMBER, int))

        if settings.value(CARLA_KEY_OSC_TCP_PORT_RANDOM, CARLA_DEFAULT_OSC_TCP_PORT_RANDOM, bool):
            self.ui.rb_osc_tcp_port_specific.setChecked(False)
            self.ui.rb_osc_tcp_port_random.setChecked(True)
        else:
            self.ui.rb_osc_tcp_port_random.setChecked(False)
            self.ui.rb_osc_tcp_port_specific.setChecked(True)

        if settings.value(CARLA_KEY_OSC_UDP_PORT_RANDOM, CARLA_DEFAULT_OSC_UDP_PORT_RANDOM, bool):
            self.ui.rb_osc_udp_port_specific.setChecked(False)
            self.ui.rb_osc_udp_port_random.setChecked(True)
        else:
            self.ui.rb_osc_udp_port_random.setChecked(False)
            self.ui.rb_osc_udp_port_specific.setChecked(True)

        # -------------------------------------------------------------------------------------------------------------
        # File Paths

        audioPaths = settings.value(CARLA_KEY_PATHS_AUDIO, CARLA_DEFAULT_FILE_PATH_AUDIO, list)
        midiPaths = settings.value(CARLA_KEY_PATHS_MIDI, CARLA_DEFAULT_FILE_PATH_MIDI, list)

        audioPaths.sort()
        midiPaths.sort()

        for audioPath in audioPaths:
            if not audioPath:
                continue
            self.ui.lw_files_audio.addItem(audioPath)

        for midiPath in midiPaths:
            if not midiPath:
                continue
            self.ui.lw_files_midi.addItem(midiPath)

        # -------------------------------------------------------------------------------------------------------------
        # Plugin Paths

        ladspas = settings.value(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH, list)
        dssis   = settings.value(CARLA_KEY_PATHS_DSSI,   CARLA_DEFAULT_DSSI_PATH, list)
        lv2s    = settings.value(CARLA_KEY_PATHS_LV2,    CARLA_DEFAULT_LV2_PATH, list)
        vst2s   = settings.value(CARLA_KEY_PATHS_VST2,   CARLA_DEFAULT_VST2_PATH, list)
        vst3s   = settings.value(CARLA_KEY_PATHS_VST3,   CARLA_DEFAULT_VST3_PATH, list)
        sf2s    = settings.value(CARLA_KEY_PATHS_SF2,    CARLA_DEFAULT_SF2_PATH, list)
        sfzs    = settings.value(CARLA_KEY_PATHS_SFZ,    CARLA_DEFAULT_SFZ_PATH, list)
        jsfxs   = settings.value(CARLA_KEY_PATHS_JSFX,   CARLA_DEFAULT_JSFX_PATH, list)
        claps   = settings.value(CARLA_KEY_PATHS_CLAP,   CARLA_DEFAULT_CLAP_PATH, list)

        ladspas.sort()
        dssis.sort()
        lv2s.sort()
        vst2s.sort()
        vst3s.sort()
        sf2s.sort()
        sfzs.sort()
        jsfxs.sort()
        claps.sort()

        for ladspa in ladspas:
            if not ladspa:
                continue
            self.ui.lw_ladspa.addItem(ladspa)

        for dssi in dssis:
            if not dssi:
                continue
            self.ui.lw_dssi.addItem(dssi)

        for lv2 in lv2s:
            if not lv2:
                continue
            self.ui.lw_lv2.addItem(lv2)

        for vst2 in vst2s:
            if not vst2:
                continue
            self.ui.lw_vst.addItem(vst2)

        for vst3 in vst3s:
            if not vst3:
                continue
            self.ui.lw_vst3.addItem(vst3)

        for sf2 in sf2s:
            if not sf2:
                continue
            self.ui.lw_sf2.addItem(sf2)

        for sfz in sfzs:
            if not sfz:
                continue
            self.ui.lw_sfz.addItem(sfz)

        for jsfx in jsfxs:
            if not jsfx:
                continue
            self.ui.lw_jsfx.addItem(jsfx)

        for clap in claps:
            if not clap:
                continue
            self.ui.lw_clap.addItem(clap)

        # -------------------------------------------------------------------------------------------------------------
        # Wine

        self.ui.le_wine_exec.setText(
            settings.value(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE, str))

        self.ui.cb_wine_prefix_detect.setChecked(
            settings.value(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX, bool))

        self.ui.le_wine_prefix_fallback.setText(
            settings.value(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX, str))

        self.ui.group_wine_realtime.setChecked(
            settings.value(CARLA_KEY_WINE_RT_PRIO_ENABLED, CARLA_DEFAULT_WINE_RT_PRIO_ENABLED, bool))

        self.ui.sb_wine_base_prio.setValue(
            settings.value(CARLA_KEY_WINE_BASE_RT_PRIO, CARLA_DEFAULT_WINE_BASE_RT_PRIO, int))

        self.ui.sb_wine_server_prio.setValue(
            settings.value(CARLA_KEY_WINE_SERVER_RT_PRIO, CARLA_DEFAULT_WINE_SERVER_RT_PRIO, int))

        # -------------------------------------------------------------------------------------------------------------
        # Experimental

        self.ui.ch_exp_jack_apps.setChecked(
            settings.value(CARLA_KEY_EXPERIMENTAL_JACK_APPS, CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS, bool))

        self.ui.ch_exp_export_lv2.setChecked(
            settings.value(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2, CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT, bool))

        self.ui.ch_exp_load_lib_global.setChecked(
            settings.value(CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL, CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL, bool))

        self.ui.ch_exp_prevent_bad_behaviour.setChecked(
            settings.value(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR,
                           CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR, bool))

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_saveSettings(self):
        settings = QSafeSettings()

        self.host.experimental = self.ui.ch_main_experimental.isChecked()

        if not self.host.experimental:
            self.resetExperimentalSettings()

        # -------------------------------------------------------------------------------------------------------------
        # Main

        settings.setValue(CARLA_KEY_MAIN_PROJECT_FOLDER,   self.ui.le_main_proj_folder.text())
        settings.setValue(CARLA_KEY_MAIN_CONFIRM_EXIT,     self.ui.ch_main_confirm_exit.isChecked())
        settings.setValue(CARLA_KEY_MAIN_CLASSIC_SKIN,     self.ui.cb_main_classic_skin_default.isChecked())
        settings.setValue(CARLA_KEY_MAIN_USE_PRO_THEME,    self.ui.ch_main_theme_pro.isChecked())
        settings.setValue(CARLA_KEY_MAIN_PRO_THEME_COLOR,  self.ui.cb_main_theme_color.currentText())
        settings.setValue(CARLA_KEY_MAIN_REFRESH_INTERVAL, self.ui.sb_main_refresh_interval.value())
        settings.setValue(CARLA_KEY_MAIN_SYSTEM_ICONS,     self.ui.ch_main_system_icons.isChecked())

        # -------------------------------------------------------------------------------------------------------------
        # Canvas

        settings.setValue(CARLA_KEY_CANVAS_THEME,             self.ui.cb_canvas_theme.currentText())
        settings.setValue(CARLA_KEY_CANVAS_SIZE,              self.ui.cb_canvas_size.currentText())
        settings.setValue(CARLA_KEY_CANVAS_USE_BEZIER_LINES,  self.ui.cb_canvas_bezier_lines.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,  self.ui.cb_canvas_hide_groups.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, self.ui.cb_canvas_auto_select.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_EYE_CANDY,         self.ui.cb_canvas_eyecandy.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_FANCY_EYE_CANDY,   self.ui.cb_canvas_fancy_eyecandy.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_USE_OPENGL,        self.ui.cb_canvas_use_opengl.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_HQ_ANTIALIASING,   self.ui.cb_canvas_render_hq_aa.isChecked())
        # NOTE: checkState() 0, 1, 2 match their enum variants
        settings.setValue(CARLA_KEY_CANVAS_ANTIALIASING,      self.ui.cb_canvas_render_aa.checkState())
        settings.setValue(CARLA_KEY_CANVAS_FULL_REPAINTS,     self.ui.cb_canvas_full_repaints.isChecked())
        settings.setValue(CARLA_KEY_CANVAS_INLINE_DISPLAYS,   self.ui.cb_canvas_inline_displays.isChecked())

        # -------------------------------------------------------------------------------------------------------------

        settings = QSafeSettings("falkTX", "Carla2")

        # -------------------------------------------------------------------------------------------------------------
        # Main

        settings.setValue(CARLA_KEY_MAIN_EXPERIMENTAL, self.host.experimental)

        # -------------------------------------------------------------------------------------------------------------
        # Engine

        audioDriver = self.ui.cb_engine_audio_driver.currentText()

        if audioDriver and self.host.audioDriverForced is None and not self.host.isPlugin:
            settings.setValue(CARLA_KEY_ENGINE_AUDIO_DRIVER, audioDriver)

        if not self.host.processModeForced:
            # engine sends callback if processMode really changes
            if audioDriver == "JACK":
                self.host.nextProcessMode = self.ui.cb_engine_process_mode_jack.currentIndex()
            else:
                self.host.nextProcessMode = (self.ui.cb_engine_process_mode_other.currentIndex() +
                                             self.PROCESS_MODE_NON_JACK_PADDING)

            settings.setValue(CARLA_KEY_ENGINE_PROCESS_MODE, self.host.nextProcessMode)

        self.host.exportLV2           = self.ui.ch_exp_export_lv2.isChecked()
        self.host.forceStereo         = self.ui.ch_engine_force_stereo.isChecked()
        self.host.resetXruns          = self.ui.cb_engine_reset_xruns.isChecked()
        self.host.maxParameters       = self.ui.sb_engine_max_params.value()
        self.host.manageUIs           = self.ui.ch_engine_manage_uis.isChecked()
        self.host.preferPluginBridges = self.ui.ch_engine_prefer_plugin_bridges.isChecked()
        self.host.preferUIBridges     = self.ui.ch_engine_prefer_ui_bridges.isChecked()
        self.host.showLogs            = self.ui.ch_main_show_logs.isChecked()
        self.host.showPluginBridges   = self.ui.cb_exp_plugin_bridges.isChecked()
        self.host.showWineBridges     = self.ui.ch_exp_wine_bridges.isChecked()
        self.host.uiBridgesTimeout    = self.ui.sb_engine_ui_bridges_timeout.value()
        self.host.uisAlwaysOnTop      = self.ui.ch_engine_uis_always_on_top.isChecked()

        if self.ui.ch_engine_force_stereo.isEnabled():
            settings.setValue(CARLA_KEY_ENGINE_FORCE_STEREO,      self.host.forceStereo)

        settings.setValue(CARLA_KEY_MAIN_SHOW_LOGS,               self.host.showLogs)
        settings.setValue(CARLA_KEY_ENGINE_MAX_PARAMETERS,        self.host.maxParameters)
        settings.setValue(CARLA_KEY_ENGINE_RESET_XRUNS,           self.host.resetXruns)
        settings.setValue(CARLA_KEY_ENGINE_MANAGE_UIS,            self.host.manageUIs)
        settings.setValue(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, self.host.preferPluginBridges)
        settings.setValue(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES,     self.host.preferUIBridges)
        settings.setValue(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT,    self.host.uiBridgesTimeout)
        settings.setValue(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP,     self.host.uisAlwaysOnTop)
        settings.setValue(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2,      self.host.exportLV2)
        settings.setValue(CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES,  self.host.showPluginBridges)
        settings.setValue(CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES,    self.host.showWineBridges)

        # -------------------------------------------------------------------------------------------------------------
        # OSC

        settings.setValue(CARLA_KEY_OSC_ENABLED,          self.ui.ch_osc_enable.isChecked())
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_ENABLED, self.ui.group_osc_tcp_port.isChecked())
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_ENABLED, self.ui.group_osc_udp_port.isChecked())
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_RANDOM,  self.ui.rb_osc_tcp_port_random.isChecked())
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_RANDOM,  self.ui.rb_osc_udp_port_random.isChecked())
        settings.setValue(CARLA_KEY_OSC_TCP_PORT_NUMBER,  self.ui.sb_osc_tcp_port_number.value())
        settings.setValue(CARLA_KEY_OSC_UDP_PORT_NUMBER,  self.ui.sb_osc_udp_port_number.value())

        # -------------------------------------------------------------------------------------------------------------
        # File Paths

        audioPaths = []
        midiPaths  = []

        for i in range(self.ui.lw_files_audio.count()):
            audioPaths.append(self.ui.lw_files_audio.item(i).text())

        for i in range(self.ui.lw_files_midi.count()):
            midiPaths.append(self.ui.lw_files_midi.item(i).text())

        self.host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, splitter.join(audioPaths))
        self.host.set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_MIDI,  splitter.join(midiPaths))

        settings.setValue(CARLA_KEY_PATHS_AUDIO, audioPaths)
        settings.setValue(CARLA_KEY_PATHS_MIDI,  midiPaths)

        # -------------------------------------------------------------------------------------------------------------
        # Plugin Paths

        ladspas = []
        dssis   = []
        lv2s    = []
        vst2s   = []
        vst3s   = []
        sf2s    = []
        sfzs    = []
        jsfxs   = []
        claps   = []

        for i in range(self.ui.lw_ladspa.count()):
            ladspas.append(self.ui.lw_ladspa.item(i).text())

        for i in range(self.ui.lw_dssi.count()):
            dssis.append(self.ui.lw_dssi.item(i).text())

        for i in range(self.ui.lw_lv2.count()):
            lv2s.append(self.ui.lw_lv2.item(i).text())

        for i in range(self.ui.lw_vst.count()):
            vst2s.append(self.ui.lw_vst.item(i).text())

        for i in range(self.ui.lw_vst3.count()):
            vst3s.append(self.ui.lw_vst3.item(i).text())

        for i in range(self.ui.lw_sf2.count()):
            sf2s.append(self.ui.lw_sf2.item(i).text())

        for i in range(self.ui.lw_sfz.count()):
            sfzs.append(self.ui.lw_sfz.item(i).text())

        for i in range(self.ui.lw_jsfx.count()):
            jsfxs.append(self.ui.lw_jsfx.item(i).text())

        for i in range(self.ui.lw_clap.count()):
            claps.append(self.ui.lw_clap.item(i).text())

        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, splitter.join(ladspas))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI,   splitter.join(dssis))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,    splitter.join(lv2s))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST2,   splitter.join(vst2s))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3,   splitter.join(vst3s))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2,    splitter.join(sf2s))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ,    splitter.join(sfzs))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_JSFX,   splitter.join(jsfxs))
        self.host.set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_CLAP,   splitter.join(claps))

        settings.setValue(CARLA_KEY_PATHS_LADSPA, ladspas)
        settings.setValue(CARLA_KEY_PATHS_DSSI,   dssis)
        settings.setValue(CARLA_KEY_PATHS_LV2,    lv2s)
        settings.setValue(CARLA_KEY_PATHS_VST2,   vst2s)
        settings.setValue(CARLA_KEY_PATHS_VST3,   vst3s)
        settings.setValue(CARLA_KEY_PATHS_SF2,    sf2s)
        settings.setValue(CARLA_KEY_PATHS_SFZ,    sfzs)
        settings.setValue(CARLA_KEY_PATHS_JSFX,   jsfxs)
        settings.setValue(CARLA_KEY_PATHS_CLAP,   claps)

        # -------------------------------------------------------------------------------------------------------------
        # Wine

        settings.setValue(CARLA_KEY_WINE_EXECUTABLE, self.ui.le_wine_exec.text())
        settings.setValue(CARLA_KEY_WINE_AUTO_PREFIX, self.ui.cb_wine_prefix_detect.isChecked())
        settings.setValue(CARLA_KEY_WINE_FALLBACK_PREFIX, self.ui.le_wine_prefix_fallback.text())
        settings.setValue(CARLA_KEY_WINE_RT_PRIO_ENABLED, self.ui.group_wine_realtime.isChecked())
        settings.setValue(CARLA_KEY_WINE_BASE_RT_PRIO, self.ui.sb_wine_base_prio.value())
        settings.setValue(CARLA_KEY_WINE_SERVER_RT_PRIO, self.ui.sb_wine_server_prio.value())

        # -------------------------------------------------------------------------------------------------------------
        # Experimental

        settings.setValue(CARLA_KEY_EXPERIMENTAL_JACK_APPS, self.ui.ch_exp_jack_apps.isChecked())
        settings.setValue(CARLA_KEY_EXPERIMENTAL_LOAD_LIB_GLOBAL, self.ui.ch_exp_load_lib_global.isChecked())
        settings.setValue(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR,
                          self.ui.ch_exp_prevent_bad_behaviour.isChecked())

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_resetSettings(self):
        currentRow = self.ui.lw_page.currentRow()

        # -------------------------------------------------------------------------------------------------------------
        # Main

        if currentRow == self.TAB_INDEX_MAIN:
            self.ui.le_main_proj_folder.setText(CARLA_DEFAULT_MAIN_PROJECT_FOLDER)
            self.ui.ch_main_theme_pro.setChecked(CARLA_DEFAULT_MAIN_USE_PRO_THEME and
                                                 self.ui.group_main_theme.isEnabled())
            self.ui.cb_main_theme_color.setCurrentIndex(
                self.ui.cb_main_theme_color.findText(CARLA_DEFAULT_MAIN_PRO_THEME_COLOR))
            self.ui.sb_main_refresh_interval.setValue(CARLA_DEFAULT_MAIN_REFRESH_INTERVAL)
            self.ui.ch_main_confirm_exit.setChecked(CARLA_DEFAULT_MAIN_CONFIRM_EXIT)
            self.ui.cb_main_classic_skin_default(CARLA_DEFAULT_MAIN_CLASSIC_SKIN)
            self.ui.ch_main_show_logs.setChecked(CARLA_DEFAULT_MAIN_SHOW_LOGS)

        # -------------------------------------------------------------------------------------------------------------
        # Canvas

        elif currentRow == self.TAB_INDEX_CANVAS:
            self.ui.cb_canvas_theme.setCurrentIndex(self.ui.cb_canvas_theme.findText(CARLA_DEFAULT_CANVAS_THEME))
            self.ui.cb_canvas_size.setCurrentIndex(self.ui.cb_canvas_size.findText(CARLA_DEFAULT_CANVAS_SIZE))
            self.ui.cb_canvas_bezier_lines.setChecked(CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES)
            self.ui.cb_canvas_hide_groups.setChecked(CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS)
            self.ui.cb_canvas_auto_select.setChecked(CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS)
            self.ui.cb_canvas_eyecandy.setChecked(CARLA_DEFAULT_CANVAS_EYE_CANDY and self.ui.cb_canvas_eyecandy.isEnabled())
            self.ui.cb_canvas_render_aa.setCheckState(Qt.PartiallyChecked) # CARLA_DEFAULT_CANVAS_ANTIALIASING
            self.ui.cb_canvas_full_repaints.setChecked(CARLA_DEFAULT_CANVAS_FULL_REPAINTS)

        # -------------------------------------------------------------------------------------------------------------
        # Engine

        elif currentRow == self.TAB_INDEX_ENGINE:
            if not self.host.isPlugin:
                self.ui.cb_engine_audio_driver.setCurrentIndex(0)

            if not self.host.processModeForced:
                if self.ui.cb_engine_audio_driver.currentText() == "JACK":
                    self.ui.cb_engine_process_mode_jack.setCurrentIndex(CARLA_DEFAULT_PROCESS_MODE)
                    self.ui.sw_engine_process_mode.setCurrentIndex(0) # show all modes
                else:
                    self.ui.cb_engine_process_mode_other.setCurrentIndex(CARLA_DEFAULT_PROCESS_MODE -
                                                                         self.PROCESS_MODE_NON_JACK_PADDING)
                    self.ui.sw_engine_process_mode.setCurrentIndex(1) # hide single+multi client modes

            self.ui.sb_engine_max_params.setValue(CARLA_DEFAULT_MAX_PARAMETERS)
            self.ui.cb_engine_reset_xruns.setChecked(CARLA_DEFAULT_RESET_XRUNS)
            self.ui.ch_engine_uis_always_on_top.setChecked(CARLA_DEFAULT_UIS_ALWAYS_ON_TOP)
            self.ui.ch_engine_prefer_ui_bridges.setChecked(CARLA_DEFAULT_PREFER_UI_BRIDGES)
            self.ui.sb_engine_ui_bridges_timeout.setValue(CARLA_DEFAULT_UI_BRIDGES_TIMEOUT)
            self.ui.ch_engine_manage_uis.setChecked(CARLA_DEFAULT_MANAGE_UIS)

        # -------------------------------------------------------------------------------------------------------------
        # OSC

        elif currentRow == self.TAB_INDEX_OSC:
            self.ui.ch_osc_enable.setChecked(CARLA_DEFAULT_OSC_ENABLED)
            self.ui.group_osc_tcp_port.setChecked(CARLA_DEFAULT_OSC_TCP_PORT_ENABLED)
            self.ui.group_osc_udp_port.setChecked(CARLA_DEFAULT_OSC_UDP_PORT_ENABLED)
            self.ui.sb_osc_tcp_port_number.setValue(CARLA_DEFAULT_OSC_TCP_PORT_NUMBER)
            self.ui.sb_osc_udp_port_number.setValue(CARLA_DEFAULT_OSC_UDP_PORT_NUMBER)

            if CARLA_DEFAULT_OSC_TCP_PORT_RANDOM:
                self.ui.rb_osc_tcp_port_specific.setChecked(False)
                self.ui.rb_osc_tcp_port_random.setChecked(True)
            else:
                self.ui.rb_osc_tcp_port_random.setChecked(False)
                self.ui.rb_osc_tcp_port_specific.setChecked(True)

            if CARLA_DEFAULT_OSC_UDP_PORT_RANDOM:
                self.ui.rb_osc_udp_port_specific.setChecked(False)
                self.ui.rb_osc_udp_port_random.setChecked(True)
            else:
                self.ui.rb_osc_udp_port_random.setChecked(False)
                self.ui.rb_osc_udp_port_specific.setChecked(True)

        # -------------------------------------------------------------------------------------------------------------
        # Plugin Paths

        elif currentRow == self.TAB_INDEX_FILEPATHS:
            curIndex = self.ui.tw_filepaths.currentIndex()

            if curIndex == self.FILEPATH_INDEX_AUDIO:
                self.ui.lw_files_audio.clear()

            elif curIndex == self.FILEPATH_INDEX_MIDI:
                self.ui.lw_files_midi.clear()

        # -------------------------------------------------------------------------------------------------------------
        # Plugin Paths

        elif currentRow == self.TAB_INDEX_PLUGINPATHS:
            curIndex = self.ui.tw_paths.currentIndex()

            if curIndex == self.PLUGINPATH_INDEX_LADSPA:
                paths = CARLA_DEFAULT_LADSPA_PATH
                paths.sort()
                self.ui.lw_ladspa.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_ladspa.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_DSSI:
                paths = CARLA_DEFAULT_DSSI_PATH
                paths.sort()
                self.ui.lw_dssi.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_dssi.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_LV2:
                paths = CARLA_DEFAULT_LV2_PATH
                paths.sort()
                self.ui.lw_lv2.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_lv2.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_VST2:
                paths = CARLA_DEFAULT_VST2_PATH
                paths.sort()
                self.ui.lw_vst.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_vst.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_VST3:
                paths = CARLA_DEFAULT_VST3_PATH
                paths.sort()
                self.ui.lw_vst3.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_vst3.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_SF2:
                paths = CARLA_DEFAULT_SF2_PATH
                paths.sort()
                self.ui.lw_sf2.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_sf2.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_SFZ:
                paths = CARLA_DEFAULT_SFZ_PATH
                paths.sort()
                self.ui.lw_sfz.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_sfz.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_JSFX:
                paths = CARLA_DEFAULT_JSFX_PATH
                paths.sort()
                self.ui.lw_jsfx.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_jsfx.addItem(path)

            elif curIndex == self.PLUGINPATH_INDEX_CLAP:
                paths = CARLA_DEFAULT_CLAP_PATH
                paths.sort()
                self.ui.lw_clap.clear()

                for path in paths:
                    if not path:
                        continue
                    self.ui.lw_clap.addItem(path)

        # -------------------------------------------------------------------------------------------------------------
        # Wine

        elif currentRow == self.TAB_INDEX_WINE:
            self.ui.le_wine_exec.setText(CARLA_DEFAULT_WINE_EXECUTABLE)
            self.ui.cb_wine_prefix_detect.setChecked(CARLA_DEFAULT_WINE_AUTO_PREFIX)
            self.ui.le_wine_prefix_fallback.setText(CARLA_DEFAULT_WINE_FALLBACK_PREFIX)
            self.ui.group_wine_realtime.setChecked(CARLA_DEFAULT_WINE_RT_PRIO_ENABLED)
            self.ui.sb_wine_base_prio.setValue(CARLA_DEFAULT_WINE_BASE_RT_PRIO)
            self.ui.sb_wine_server_prio.setValue(CARLA_DEFAULT_WINE_SERVER_RT_PRIO)

        # -------------------------------------------------------------------------------------------------------------
        # Experimental

        elif currentRow == self.TAB_INDEX_EXPERIMENTAL:
            self.resetExperimentalSettings()

    def resetExperimentalSettings(self):
        # Forever experimental
        self.ui.cb_exp_plugin_bridges.setChecked(CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES)
        self.ui.ch_exp_wine_bridges.setChecked(CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES)
        self.ui.ch_exp_jack_apps.setChecked(CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS)
        self.ui.ch_exp_export_lv2.setChecked(CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT)
        self.ui.ch_exp_load_lib_global.setChecked(CARLA_DEFAULT_EXPERIMENTAL_LOAD_LIB_GLOBAL)
        self.ui.ch_exp_prevent_bad_behaviour.setChecked(CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR)

        # Temporary, until stable
        self.ui.ch_main_system_icons.setChecked(CARLA_DEFAULT_MAIN_SYSTEM_ICONS)
        self.ui.cb_canvas_fancy_eyecandy.setChecked(CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY)
        self.ui.cb_canvas_use_opengl.setChecked(CARLA_DEFAULT_CANVAS_USE_OPENGL and
                                                self.ui.cb_canvas_use_opengl.isEnabled())
        self.ui.cb_canvas_render_hq_aa.setChecked(CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING and
                                                  self.ui.cb_canvas_render_hq_aa.isEnabled())
        self.ui.cb_canvas_inline_displays.setChecked(CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS)
        self.ui.ch_engine_force_stereo.setChecked(CARLA_DEFAULT_FORCE_STEREO)
        self.ui.ch_engine_prefer_plugin_bridges.setChecked(CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_enableExperimental(self, toggled):
        if toggled:
            self.ui.lw_page.showRow(self.TAB_INDEX_EXPERIMENTAL)
            if self.ui.ch_exp_wine_bridges.isChecked() and not self.host.isControl:
                self.ui.lw_page.showRow(self.TAB_INDEX_WINE)
        else:
            self.ui.lw_page.hideRow(self.TAB_INDEX_EXPERIMENTAL)
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

    @pyqtSlot(bool)
    def slot_enableWineBridges(self, toggled):
        if toggled and not self.host.isControl:
            self.ui.lw_page.showRow(self.TAB_INDEX_WINE)
        else:
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

    @pyqtSlot(bool)
    def slot_pluginBridgesToggled(self, toggled):
        if not toggled:
            self.ui.ch_exp_wine_bridges.setChecked(False)
            self.ui.ch_engine_prefer_plugin_bridges.setChecked(False)
            self.ui.lw_page.hideRow(self.TAB_INDEX_WINE)

    @pyqtSlot(bool)
    def slot_canvasEyeCandyToggled(self, toggled):
        if not toggled:
            # disable fancy eyecandy too
            self.ui.cb_canvas_fancy_eyecandy.setChecked(False)

    @pyqtSlot(bool)
    def slot_canvasFancyEyeCandyToggled(self, toggled):
        if toggled:
            # make sure normal eyecandy is enabled too
            self.ui.cb_canvas_eyecandy.setChecked(True)

    @pyqtSlot(bool)
    def slot_canvasOpenGLToggled(self, toggled):
        if not toggled:
            # uncheck GL specific option
            self.ui.cb_canvas_render_hq_aa.setChecked(False)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_getAndSetProjectPath(self):
        # FIXME
        getAndSetPath(self, self.ui.le_main_proj_folder)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_engineAudioDriverChanged(self):
        if self.ui.cb_engine_audio_driver.currentText() == "JACK":
            self.ui.sw_engine_process_mode.setCurrentIndex(0)
        else:
            self.ui.sw_engine_process_mode.setCurrentIndex(1)

    @pyqtSlot()
    def slot_showAudioDriverSettings(self):
        driverIndex = self.ui.cb_engine_audio_driver.currentIndex()
        driverName  = self.ui.cb_engine_audio_driver.currentText()
        DriverSettingsW(self, self.host, driverIndex, driverName).exec_()

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_addPluginPath(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        curIndex = self.ui.tw_paths.currentIndex()

        if curIndex == self.PLUGINPATH_INDEX_LADSPA:
            self.ui.lw_ladspa.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_DSSI:
            self.ui.lw_dssi.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_LV2:
            self.ui.lw_lv2.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_VST2:
            self.ui.lw_vst.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_VST3:
            self.ui.lw_vst3.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_SF2:
            self.ui.lw_sf2.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_SFZ:
            self.ui.lw_sfz.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_JSFX:
            self.ui.lw_jsfx.addItem(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_CLAP:
            self.ui.lw_clap.addItem(newPath)

    @pyqtSlot()
    def slot_removePluginPath(self):
        curIndex = self.ui.tw_paths.currentIndex()

        if curIndex == self.PLUGINPATH_INDEX_LADSPA:
            self.ui.lw_ladspa.takeItem(self.ui.lw_ladspa.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_DSSI:
            self.ui.lw_dssi.takeItem(self.ui.lw_dssi.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_LV2:
            self.ui.lw_lv2.takeItem(self.ui.lw_lv2.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_VST2:
            self.ui.lw_vst.takeItem(self.ui.lw_vst.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_VST3:
            self.ui.lw_vst3.takeItem(self.ui.lw_vst3.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_SF2:
            self.ui.lw_sf2.takeItem(self.ui.lw_sf2.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_SFZ:
            self.ui.lw_sfz.takeItem(self.ui.lw_sfz.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_JSFX:
            self.ui.lw_jsfx.takeItem(self.ui.lw_jsfx.currentRow())
        elif curIndex == self.PLUGINPATH_INDEX_CLAP:
            self.ui.lw_clap.takeItem(self.ui.lw_clap.currentRow())

    @pyqtSlot()
    def slot_changePluginPath(self):
        curIndex = self.ui.tw_paths.currentIndex()

        if curIndex == self.PLUGINPATH_INDEX_LADSPA:
            currentPath = self.ui.lw_ladspa.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_DSSI:
            currentPath = self.ui.lw_dssi.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_LV2:
            currentPath = self.ui.lw_lv2.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_VST2:
            currentPath = self.ui.lw_vst.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_VST3:
            currentPath = self.ui.lw_vst3.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_SF2:
            currentPath = self.ui.lw_sf2.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_SFZ:
            currentPath = self.ui.lw_sfz.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_JSFX:
            currentPath = self.ui.lw_jsfx.currentItem().text()
        elif curIndex == self.PLUGINPATH_INDEX_CLAP:
            currentPath = self.ui.lw_clap.currentItem().text()
        else:
            currentPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), currentPath, QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        if curIndex == self.PLUGINPATH_INDEX_LADSPA:
            self.ui.lw_ladspa.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_DSSI:
            self.ui.lw_dssi.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_LV2:
            self.ui.lw_lv2.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_VST2:
            self.ui.lw_vst.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_VST3:
            self.ui.lw_vst3.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_SF2:
            self.ui.lw_sf2.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_SFZ:
            self.ui.lw_sfz.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_JSFX:
            self.ui.lw_jsfx.currentItem().setText(newPath)
        elif curIndex == self.PLUGINPATH_INDEX_CLAP:
            self.ui.lw_clap.currentItem().setText(newPath)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_pluginPathTabChanged(self, index):
        if index == self.PLUGINPATH_INDEX_LADSPA:
            row = self.ui.lw_ladspa.currentRow()
        elif index == self.PLUGINPATH_INDEX_DSSI:
            row = self.ui.lw_dssi.currentRow()
        elif index == self.PLUGINPATH_INDEX_LV2:
            row = self.ui.lw_lv2.currentRow()
        elif index == self.PLUGINPATH_INDEX_VST2:
            row = self.ui.lw_vst.currentRow()
        elif index == self.PLUGINPATH_INDEX_VST3:
            row = self.ui.lw_vst3.currentRow()
        elif index == self.PLUGINPATH_INDEX_SF2:
            row = self.ui.lw_sf2.currentRow()
        elif index == self.PLUGINPATH_INDEX_SFZ:
            row = self.ui.lw_sfz.currentRow()
        elif index == self.PLUGINPATH_INDEX_JSFX:
            row = self.ui.lw_jsfx.currentRow()
        elif index == self.PLUGINPATH_INDEX_CLAP:
            row = self.ui.lw_clap.currentRow()
        else:
            row = -1

        self.slot_pluginPathRowChanged(row)

    @pyqtSlot(int)
    def slot_pluginPathRowChanged(self, row):
        check = bool(row >= 0)
        self.ui.b_paths_remove.setEnabled(check)
        self.ui.b_paths_change.setEnabled(check)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot()
    def slot_addFilePath(self):
        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), "", QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        curIndex = self.ui.tw_filepaths.currentIndex()

        if curIndex == self.FILEPATH_INDEX_AUDIO:
            self.ui.lw_files_audio.addItem(newPath)
        elif curIndex == self.FILEPATH_INDEX_MIDI:
            self.ui.lw_files_midi.addItem(newPath)

    @pyqtSlot()
    def slot_removeFilePath(self):
        curIndex = self.ui.tw_filepaths.currentIndex()

        if curIndex == self.FILEPATH_INDEX_AUDIO:
            self.ui.lw_files_audio.takeItem(self.ui.lw_files_audio.currentRow())
        elif curIndex == self.FILEPATH_INDEX_MIDI:
            self.ui.lw_files_midi.takeItem(self.ui.lw_files_midi.currentRow())

    @pyqtSlot()
    def slot_changeFilePath(self):
        curIndex = self.ui.tw_filepaths.currentIndex()

        if curIndex == self.FILEPATH_INDEX_AUDIO:
            currentPath = self.ui.lw_files_audio.currentItem().text()
        elif curIndex == self.FILEPATH_INDEX_MIDI:
            currentPath = self.ui.lw_files_midi.currentItem().text()
        else:
            currentPath = ""

        newPath = QFileDialog.getExistingDirectory(self, self.tr("Add Path"), currentPath, QFileDialog.ShowDirsOnly)

        if not newPath:
            return

        if curIndex == self.FILEPATH_INDEX_AUDIO:
            self.ui.lw_files_audio.currentItem().setText(newPath)
        elif curIndex == self.FILEPATH_INDEX_MIDI:
            self.ui.lw_files_midi.currentItem().setText(newPath)

    # -----------------------------------------------------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_filePathTabChanged(self, index):
        if index == self.FILEPATH_INDEX_AUDIO:
            row = self.ui.lw_files_audio.currentRow()
        elif index == self.FILEPATH_INDEX_MIDI:
            row = self.ui.lw_files_midi.currentRow()
        else:
            row = -1

        self.slot_filePathRowChanged(row)

    @pyqtSlot(int)
    def slot_filePathRowChanged(self, row):
        check = bool(row >= 0)
        self.ui.b_filepaths_remove.setEnabled(check)
        self.ui.b_filepaths_change.setEnabled(check)

# ---------------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    from carla_app import CarlaApplication
    from carla_host import initHost as _initHost, loadHostSettings as _loadHostSettings
    # pylint: disable=ungrouped-imports
    from carla_shared import handleInitialCommandLineArguments
    # pylint: enable=ungrouped-imports

    initName, libPrefix = handleInitialCommandLineArguments(__file__ if "__file__" in dir() else None)

    _app  = CarlaApplication("Carla2-Settings", libPrefix)
    _host = _initHost("Carla2-Settings", libPrefix, False, False, False)
    _loadHostSettings(_host)

    _gui = CarlaSettingsW(None, _host, True, True)
    _gui.show()

    _app.exit_exec()

# ---------------------------------------------------------------------------------------------------------------------
