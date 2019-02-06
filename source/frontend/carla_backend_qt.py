#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code (Qt stuff)
# Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import pyqtSignal, QObject

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import *

# ------------------------------------------------------------------------------------------------------------
# Carla Host PyQt signals

class CarlaHostSignals(QObject):
    # signals
    DebugCallback = pyqtSignal(int, int, int, float, str)
    PluginAddedCallback = pyqtSignal(int, str)
    PluginRemovedCallback = pyqtSignal(int)
    PluginRenamedCallback = pyqtSignal(int, str)
    PluginUnavailableCallback = pyqtSignal(int, str)
    ParameterValueChangedCallback = pyqtSignal(int, int, float)
    ParameterDefaultChangedCallback = pyqtSignal(int, int, float)
    ParameterMidiCcChangedCallback = pyqtSignal(int, int, int)
    ParameterMidiChannelChangedCallback = pyqtSignal(int, int, int)
    ProgramChangedCallback = pyqtSignal(int, int)
    MidiProgramChangedCallback = pyqtSignal(int, int)
    OptionChangedCallback = pyqtSignal(int, int, bool)
    UiStateChangedCallback = pyqtSignal(int, int)
    NoteOnCallback = pyqtSignal(int, int, int, int)
    NoteOffCallback = pyqtSignal(int, int, int)
    UpdateCallback = pyqtSignal(int)
    ReloadInfoCallback = pyqtSignal(int)
    ReloadParametersCallback = pyqtSignal(int)
    ReloadProgramsCallback = pyqtSignal(int)
    ReloadAllCallback = pyqtSignal(int)
    PatchbayClientAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayClientRemovedCallback = pyqtSignal(int)
    PatchbayClientRenamedCallback = pyqtSignal(int, str)
    PatchbayClientDataChangedCallback = pyqtSignal(int, int, int)
    PatchbayPortAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayPortRemovedCallback = pyqtSignal(int, int)
    PatchbayPortRenamedCallback = pyqtSignal(int, int, str)
    PatchbayConnectionAddedCallback = pyqtSignal(int, int, int, int, int)
    PatchbayConnectionRemovedCallback = pyqtSignal(int, int, int)
    EngineStartedCallback = pyqtSignal(int, int, float, str)
    EngineStoppedCallback = pyqtSignal()
    ProcessModeChangedCallback = pyqtSignal(int)
    TransportModeChangedCallback = pyqtSignal(int, str)
    BufferSizeChangedCallback = pyqtSignal(int)
    SampleRateChangedCallback = pyqtSignal(float)
    CancelableActionCallback = pyqtSignal(int, bool, str)
    ProjectLoadFinishedCallback = pyqtSignal()
    NSMCallback = pyqtSignal(int, int, str)
    InfoCallback = pyqtSignal(str)
    ErrorCallback = pyqtSignal(str)
    QuitCallback = pyqtSignal()

# ------------------------------------------------------------------------------------------------------------
# Carla Host object (dummy/null, does nothing)

class CarlaHostQtNull(CarlaHostNull, CarlaHostSignals):
    def __init__(self):
        CarlaHostSignals.__init__(self)
        CarlaHostNull.__init__(self)

# ------------------------------------------------------------------------------------------------------------
# Carla Host object using a DLL

class CarlaHostQtDLL(CarlaHostDLL, CarlaHostSignals):
    def __init__(self, libName, loadGlobal):
        CarlaHostSignals.__init__(self)
        CarlaHostDLL.__init__(self, libName, loadGlobal)

# ------------------------------------------------------------------------------------------------------------
# Carla Host object for plugins (using pipes)

class CarlaHostQtPlugin(CarlaHostPlugin, CarlaHostSignals):
    def __init__(self):
        CarlaHostSignals.__init__(self)
        CarlaHostPlugin.__init__(self)

# ------------------------------------------------------------------------------------------------------------
