#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, QObject
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, QObject

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import CarlaHostNull, CarlaHostDLL, CarlaHostPlugin

# ------------------------------------------------------------------------------------------------------------
# Carla Host PyQt signals

class CarlaHostSignals(QObject):
    # signals
    DebugCallback = pyqtSignal(int, int, int, int, float, str)
    PluginAddedCallback = pyqtSignal(int, int, str)
    PluginRemovedCallback = pyqtSignal(int)
    PluginRenamedCallback = pyqtSignal(int, str)
    PluginUnavailableCallback = pyqtSignal(int, str)
    ParameterValueChangedCallback = pyqtSignal(int, int, float)
    ParameterDefaultChangedCallback = pyqtSignal(int, int, float)
    ParameterMappedControlIndexChangedCallback = pyqtSignal(int, int, int)
    ParameterMappedRangeChangedCallback = pyqtSignal(int, int, float, float)
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
    PatchbayClientPositionChangedCallback = pyqtSignal(int, int, int, int, int)
    PatchbayPortAddedCallback = pyqtSignal(int, int, int, int, str)
    PatchbayPortRemovedCallback = pyqtSignal(int, int)
    PatchbayPortChangedCallback = pyqtSignal(int, int, int, int, str)
    PatchbayPortGroupAddedCallback = pyqtSignal(int, int, int, str)
    PatchbayPortGroupRemovedCallback = pyqtSignal(int, int)
    PatchbayPortGroupChangedCallback = pyqtSignal(int, int, int, str)
    PatchbayConnectionAddedCallback = pyqtSignal(int, int, int, int, int)
    PatchbayConnectionRemovedCallback = pyqtSignal(int, int, int)
    EngineStartedCallback = pyqtSignal(int, int, int, int, float, str)
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
    InlineDisplayRedrawCallback = pyqtSignal(int)

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

# pylint: disable=abstract-method
class CarlaHostQtPlugin(CarlaHostPlugin, CarlaHostSignals):
    def __init__(self):
        CarlaHostSignals.__init__(self)
        CarlaHostPlugin.__init__(self)

# pylint: enable=abstract-method
# ------------------------------------------------------------------------------------------------------------
