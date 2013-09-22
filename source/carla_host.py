#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla host code
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
    from PyQt5.QtWidgets import QMainWindow
except:
    from PyQt4.QtGui import QMainWindow

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_database import *
from carla_settings import *
from carla_widgets import *

# ------------------------------------------------------------------------------------------------------------
# Dummy widget

class CarlaDummyW(object):
    def __init__(self, parent):
        object.__init__(self)

    def addPlugin(self, pluginId):
        pass

    def removePlugin(self, pluginId):
        pass

    def removeAllPlugins(self):
        pass

    def idleFast(self):
        pass

    def idleSlow(self):
        pass

# ------------------------------------------------------------------------------------------------------------
# Host Window

class HostWindow(QMainWindow):
    # signals
    DebugCallback = pyqtSignal(int, int, int, float, str)
    PluginAddedCallback = pyqtSignal(int)
    PluginRemovedCallback = pyqtSignal(int)
    PluginRenamedCallback = pyqtSignal(int, str)
    ParameterValueChangedCallback = pyqtSignal(int, int, float)
    ParameterDefaultChangedCallback = pyqtSignal(int, int, float)
    ParameterMidiChannelChangedCallback = pyqtSignal(int, int, int)
    ParameterMidiCcChangedCallback = pyqtSignal(int, int, int)
    ProgramChangedCallback = pyqtSignal(int, int)
    MidiProgramChangedCallback = pyqtSignal(int, int)
    NoteOnCallback = pyqtSignal(int, int, int, int)
    NoteOffCallback = pyqtSignal(int, int, int)
    ShowGuiCallback = pyqtSignal(int, int)
    UpdateCallback = pyqtSignal(int)
    ReloadInfoCallback = pyqtSignal(int)
    ReloadParametersCallback = pyqtSignal(int)
    ReloadProgramsCallback = pyqtSignal(int)
    ReloadAllCallback = pyqtSignal(int)
    PatchbayClientAddedCallback = pyqtSignal(int, int, str)
    PatchbayClientRemovedCallback = pyqtSignal(int, str)
    PatchbayClientRenamedCallback = pyqtSignal(int, str)
    PatchbayPortAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayPortRemovedCallback = pyqtSignal(int, int, str)
    PatchbayPortRenamedCallback = pyqtSignal(int, int, str)
    PatchbayConnectionAddedCallback = pyqtSignal(int, int, int)
    PatchbayConnectionRemovedCallback = pyqtSignal(int)
    PatchbayIconChangedCallback = pyqtSignal(int, int)
    BufferSizeChangedCallback = pyqtSignal(int)
    SampleRateChangedCallback = pyqtSignal(float)
    NSM_AnnounceCallback = pyqtSignal(str)
    NSM_OpenCallback = pyqtSignal(str)
    NSM_SaveCallback = pyqtSignal()
    ErrorCallback = pyqtSignal(str)
    QuitCallback = pyqtSignal()
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    def __init__(self, parent):
        QMainWindow.__init__(self, parent)
        #self.ui = ui_carla_plugin.Ui_PluginWidget()
        #self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Set bridge paths

        if carla_bridge_native:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_NATIVE, 0, carla_bridge_native)

        if carla_bridge_posix32:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_POSIX32, 0, carla_bridge_posix32)

        if carla_bridge_posix64:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_POSIX64, 0, carla_bridge_posix64)

        if carla_bridge_win32:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_WIN32, 0, carla_bridge_win32)

        if carla_bridge_win64:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_WIN64, 0, carla_bridge_win64)

        if carla_bridge_lv2_external:
            Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_EXTERNAL, 0, carla_bridge_lv2_external)

        if WINDOWS:
            if carla_bridge_lv2_windows:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, carla_bridge_lv2_windows)

            if carla_bridge_vst_hwnd:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_HWND, 0, carla_bridge_vst_hwnd)

        elif MACOS:
            if carla_bridge_lv2_cocoa:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_COCOA, 0, carla_bridge_lv2_cocoa)

            if carla_bridge_vst_mac:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_MAC, 0, carla_bridge_vst_mac)

        else:
            if carla_bridge_lv2_gtk2:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_GTK2, 0, carla_bridge_lv2_gtk2)

            if carla_bridge_lv2_gtk3:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_GTK3, 0, carla_bridge_lv2_gtk3)

            if carla_bridge_lv2_qt4:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_QT4, 0, carla_bridge_lv2_qt4)

            if carla_bridge_lv2_qt5:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_QT5, 0, carla_bridge_lv2_qt5)

            if carla_bridge_lv2_x11:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_LV2_X11, 0, carla_bridge_lv2_x11)

            if carla_bridge_vst_x11:
                Carla.host.set_engine_option(OPTION_PATH_BRIDGE_VST_X11, 0, carla_bridge_vst_x11)

        # -------------------------------------------------------------
        # Set callback

        Carla.host.set_engine_callback(EngineCallback)

        # -------------------------------------------------------------
        # Set custom signal handling

        setUpSignals()

        # -------------------------------------------------------------
        # Internal stuff

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        self.fLadspaRdfNeedsUpdate = True
        self.fLadspaRdfList = []

        self.fContainer = CarlaDummyW(self)

        # -------------------------------------------------------------

    def getExtraStuff(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']

            self.maybeLoadRDFs()

            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype in (PLUGIN_GIG, PLUGIN_SF2, PLUGIN_SFZ):
            if plugin['name'].lower().endswith(" (16 outputs)"):
                # return a dummy non-null pointer
                INTPOINTER = POINTER(c_int)
                int1 = c_int(0x1)
                addr = addressof(int1)
                return cast(addr, INTPOINTER)

        return c_nullptr

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

    def setLoadRDFsNeeded(self):
        self.fLadspaRdfNeedsUpdate = True

    def init(self):
        self.fIdleTimerFast = self.startTimer(50)
        self.fIdleTimerSlow = self.startTimer(50*2)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            Carla.host.engine_idle()
            self.fContainer.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            self.fContainer.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        if self.fIdleTimerFast != 0:
            self.killTimer(self.fIdleTimerFast)
            self.fIdleTimerFast = 0

        if self.fIdleTimerSlow != 0:
            self.killTimer(self.fIdleTimerSlow)
            self.fIdleTimerSlow = 0

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Engine callback

def EngineCallback(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId < 0 or not Carla.gui:
        return

    if action == CALLBACK_DEBUG:
        Carla.gui.DebugCallback.emit(pluginId, value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PLUGIN_ADDED:
        Carla.gui.PluginAddedCallback.emit(pluginId)
    elif action == CALLBACK_PLUGIN_REMOVED:
        Carla.gui.PluginRemovedCallback.emit(pluginId)
    elif action == CALLBACK_PLUGIN_RENAMED:
        Carla.gui.PluginRenamedCallback.emit(pluginId, valueStr)
    elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        Carla.gui.ParameterValueChangedCallback.emit(pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_DEFAULT_CHANGED:
        Carla.gui.ParameterDefaultChangedCallback.emit(pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        Carla.gui.ParameterMidiChannelChangedCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        Carla.gui.ParameterMidiCcChangedCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_PROGRAM_CHANGED:
        Carla.gui.ProgramChangedCallback.emit(pluginId, value1)
    elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        Carla.gui.MidiProgramChangedCallback.emit(pluginId, value1)
    elif action == CALLBACK_NOTE_ON:
        Carla.gui.NoteOnCallback.emit(pluginId, value1, value2, value3)
    elif action == CALLBACK_NOTE_OFF:
        Carla.gui.NoteOffCallback.emit(pluginId, value1, value2)
    elif action == CALLBACK_SHOW_GUI:
        Carla.gui.ShowGuiCallback.emit(pluginId, value1)
    elif action == CALLBACK_UPDATE:
        Carla.gui.UpdateCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_INFO:
        Carla.gui.ReloadInfoCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_PARAMETERS:
        Carla.gui.ReloadParametersCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_PROGRAMS:
        Carla.gui.ReloadProgramsCallback.emit(pluginId)
    elif action == CALLBACK_RELOAD_ALL:
        Carla.gui.ReloadAllCallback.emit(pluginId)
    elif action == CALLBACK_PATCHBAY_CLIENT_ADDED:
        Carla.gui.PatchbayClientAddedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_REMOVED:
        Carla.gui.PatchbayClientRemovedCallback.emit(value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CLIENT_RENAMED:
        Carla.gui.PatchbayClientRenamedCallback.emit(value1, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_ADDED:
        Carla.gui.PatchbayPortAddedCallback.emit(value1, value2, int(value3), cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_REMOVED:
        Carla.gui.PatchbayPortRemovedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_PORT_RENAMED:
        Carla.gui.PatchbayPortRenamedCallback.emit(value1, value2, cString(valueStr))
    elif action == CALLBACK_PATCHBAY_CONNECTION_ADDED:
        Carla.gui.PatchbayConnectionAddedCallback.emit(value1, value2, value3)
    elif action == CALLBACK_PATCHBAY_CONNECTION_REMOVED:
        Carla.gui.PatchbayConnectionRemovedCallback.emit(value1)
    elif action == CALLBACK_PATCHBAY_ICON_CHANGED:
        Carla.gui.PatchbayIconChangedCallback.emit(value1, value2)
    elif action == CALLBACK_BUFFER_SIZE_CHANGED:
        Carla.gui.BufferSizeChangedCallback.emit(value1)
    elif action == CALLBACK_SAMPLE_RATE_CHANGED:
        Carla.gui.SampleRateChangedCallback.emit(value3)
    elif action == CALLBACK_NSM_ANNOUNCE:
        Carla.gui.NSM_AnnounceCallback.emit(cString(valueStr))
    elif action == CALLBACK_NSM_OPEN:
        Carla.gui.NSM_OpenCallback.emit(cString(valueStr))
    elif action == CALLBACK_NSM_SAVE:
        Carla.gui.NSM_SaveCallback.emit()
    elif action == CALLBACK_ERROR:
        Carla.gui.ErrorCallback.emit(cString(valueStr))
    elif action == CALLBACK_QUIT:
        Carla.gui.QuitCallback.emit()
