#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from abc import abstractmethod

# ------------------------------------------------------------------------------------------------------------
# Imports (PyQt)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QByteArray
    from PyQt5.QtGui import QCursor, QIcon, QPalette, QPixmap
    from PyQt5.QtWidgets import (
        QDialog,
        QFileDialog,
        QInputDialog,
        QMenu,
        QMessageBox,
        QScrollArea,
        QVBoxLayout,
        QWidget,
    )
elif qt_config == 6:
    from PyQt6.QtCore import pyqtSignal, pyqtSlot, Qt, QByteArray
    from PyQt6.QtGui import QCursor, QIcon, QPalette, QPixmap
    from PyQt6.QtWidgets import (
        QDialog,
        QFileDialog,
        QInputDialog,
        QMenu,
        QMessageBox,
        QScrollArea,
        QVBoxLayout,
        QWidget,
    )

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

import ui_carla_edit
import ui_carla_parameter

from carla_backend import (
    BINARY_NATIVE,
    CARLA_VERSION_STRING,
    CARLA_OS_MAC,
    CARLA_OS_WIN,
    PLUGIN_INTERNAL,
    PLUGIN_DSSI,
    PLUGIN_LV2,
    PLUGIN_VST2,
    PLUGIN_SF2,
    PLUGIN_SFZ,
    PLUGIN_CAN_DRYWET,
    PLUGIN_CAN_VOLUME,
    PLUGIN_CAN_BALANCE,
    PLUGIN_CAN_PANNING,
    PLUGIN_CATEGORY_SYNTH,
    PLUGIN_OPTION_FIXED_BUFFERS,
    PLUGIN_OPTION_FORCE_STEREO,
    PLUGIN_OPTION_MAP_PROGRAM_CHANGES,
    PLUGIN_OPTION_USE_CHUNKS,
    PLUGIN_OPTION_SEND_CONTROL_CHANGES,
    PLUGIN_OPTION_SEND_CHANNEL_PRESSURE,
    PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH,
    PLUGIN_OPTION_SEND_PITCHBEND,
    PLUGIN_OPTION_SEND_ALL_SOUND_OFF,
    PLUGIN_OPTION_SEND_PROGRAM_CHANGES,
    PLUGIN_OPTION_SKIP_SENDING_NOTES,
    PARAMETER_DRYWET,
    PARAMETER_VOLUME,
    PARAMETER_BALANCE_LEFT,
    PARAMETER_BALANCE_RIGHT,
    PARAMETER_PANNING,
    PARAMETER_CTRL_CHANNEL,
    PARAMETER_IS_ENABLED,
    PARAMETER_IS_AUTOMATABLE,
    PARAMETER_IS_READ_ONLY,
    PARAMETER_USES_SCALEPOINTS,
    PARAMETER_USES_CUSTOM_TEXT,
    PARAMETER_CAN_BE_CV_CONTROLLED,
    PARAMETER_INPUT, PARAMETER_OUTPUT,
    CONTROL_INDEX_NONE,
    CONTROL_INDEX_MIDI_PITCHBEND,
    CONTROL_INDEX_MIDI_LEARN,
    CONTROL_INDEX_CV
)

from carla_shared import (
    MIDI_CC_LIST, MAX_MIDI_CC_LIST_ITEM,
    countDecimalPoints,
    fontMetricsHorizontalAdvance,
    setUpSignals,
    gCarla
)

from carla_utils import getPluginTypeAsString

from widgets.collapsablewidget import CollapsibleBox
from widgets.pixmapkeyboard import PixmapKeyboardHArea

# ------------------------------------------------------------------------------------------------------------
# Carla GUI defines

ICON_STATE_ON   = 3 # turns on, sets as wait
ICON_STATE_WAIT = 2 # nothing, sets as off
ICON_STATE_OFF  = 1 # turns off, sets as null
ICON_STATE_NULL = 0 # nothing

# ------------------------------------------------------------------------------------------------------------
# Plugin Parameter

class PluginParameter(QWidget):
    mappedControlChanged = pyqtSignal(int, int)
    mappedRangeChanged   = pyqtSignal(int, float, float)
    midiChannelChanged   = pyqtSignal(int, int)
    valueChanged         = pyqtSignal(int, float)

    def __init__(self, parent, host, pInfo, pluginId, tabIndex):
        QWidget.__init__(self, parent)
        self.host = host
        self.ui = ui_carla_parameter.Ui_PluginParameter()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fDecimalPoints = max(2, countDecimalPoints(pInfo['step'], pInfo['stepSmall']))
        self.fCanBeInCV     = pInfo['hints'] & PARAMETER_CAN_BE_CV_CONTROLLED
        self.fMappedCtrl    = pInfo['mappedControlIndex']
        self.fMappedMinimum = pInfo['mappedMinimum']
        self.fMappedMaximum = pInfo['mappedMaximum']
        self.fMinimum       = pInfo['minimum']
        self.fMaximum       = pInfo['maximum']
        self.fMidiChannel   = pInfo['midiChannel']
        self.fParameterId   = pInfo['index']
        self.fPluginId      = pluginId
        self.fTabIndex      = tabIndex

        # -------------------------------------------------------------
        # Set-up GUI

        pType  = pInfo['type']
        pHints = pInfo['hints']

        self.ui.l_name.setText(pInfo['name'])
        self.ui.widget.setName(pInfo['name'])
        self.ui.widget.setMinimum(pInfo['minimum'])
        self.ui.widget.setMaximum(pInfo['maximum'])
        self.ui.widget.setDefault(pInfo['default'])
        self.ui.widget.setLabel(pInfo['unit'])
        self.ui.widget.setStep(pInfo['step'])
        self.ui.widget.setStepSmall(pInfo['stepSmall'])
        self.ui.widget.setStepLarge(pInfo['stepLarge'])
        self.ui.widget.setScalePoints(pInfo['scalePoints'], bool(pHints & PARAMETER_USES_SCALEPOINTS))

        if pInfo['comment']:
            self.ui.l_name.setToolTip(pInfo['comment'])
            self.ui.widget.setToolTip(pInfo['comment'])

        if pType == PARAMETER_INPUT:
            if not pHints & PARAMETER_IS_ENABLED:
                self.ui.l_name.setEnabled(False)
                self.ui.l_status.setEnabled(False)
                self.ui.widget.setEnabled(False)
                self.ui.widget.setReadOnly(True)
                self.ui.tb_options.setEnabled(False)

            elif not pHints & PARAMETER_IS_AUTOMATABLE:
                self.ui.l_status.setEnabled(False)
                self.ui.tb_options.setEnabled(False)

            if pHints & PARAMETER_IS_READ_ONLY:
                self.ui.l_status.setEnabled(False)
                self.ui.widget.setReadOnly(True)
                self.ui.tb_options.setEnabled(False)

        elif pType == PARAMETER_OUTPUT:
            self.ui.widget.setReadOnly(True)

        else:
            self.ui.l_status.setVisible(False)
            self.ui.widget.setVisible(False)
            self.ui.tb_options.setVisible(False)

        # Only set value after all hints are handled
        self.ui.widget.setValue(pInfo['current'])

        if pHints & PARAMETER_USES_CUSTOM_TEXT and not host.isPlugin:
            self.ui.widget.setTextCallback(self._textCallBack)

        self.ui.l_status.setFixedWidth(fontMetricsHorizontalAdvance(self.ui.l_status.fontMetrics(),
                                                                    self.tr("CC%i Ch%i" % (119,16))))

        self.ui.widget.setValueCallback(self._valueCallBack)
        self.ui.widget.updateAll()

        self.setMappedControlIndex(pInfo['mappedControlIndex'])
        self.setMidiChannel(pInfo['midiChannel'])
        self.updateStatusLabel()

        # -------------------------------------------------------------
        # Set-up connections

        self.ui.tb_options.clicked.connect(self.slot_optionsCustomMenu)
        self.ui.widget.dragStateChanged.connect(self.slot_parameterDragStateChanged)

        # -------------------------------------------------------------

    def getPluginId(self):
        return self.fPluginId

    def getTabIndex(self):
        return self.fTabIndex

    def setPluginId(self, pluginId):
        self.fPluginId = pluginId

    def setDefault(self, value):
        self.ui.widget.setDefault(value)

    def setValue(self, value):
        self.ui.widget.blockSignals(True)
        self.ui.widget.setValue(value)
        self.ui.widget.blockSignals(False)

    def setMappedControlIndex(self, control):
        self.fMappedCtrl = control
        self.updateStatusLabel()

    def setMappedRange(self, minimum, maximum):
        self.fMappedMinimum = minimum
        self.fMappedMaximum = maximum

    def setMidiChannel(self, channel):
        self.fMidiChannel = channel
        self.updateStatusLabel()

    def setLabelWidth(self, width):
        self.ui.l_name.setFixedWidth(width)

    def updateStatusLabel(self):
        if self.fMappedCtrl == CONTROL_INDEX_NONE:
            text = self.tr("Unmapped")
        elif self.fMappedCtrl == CONTROL_INDEX_CV:
            text = self.tr("CV export")
        elif self.fMappedCtrl == CONTROL_INDEX_MIDI_PITCHBEND:
            text = self.tr("PBend Ch%i" % (self.fMidiChannel,))
        elif self.fMappedCtrl == CONTROL_INDEX_MIDI_LEARN:
            text = self.tr("MIDI Learn")
        else:
            text = self.tr("CC%i Ch%i" % (self.fMappedCtrl, self.fMidiChannel))

        self.ui.l_status.setText(text)

    @pyqtSlot()
    def slot_optionsCustomMenu(self):
        menu = QMenu(self)

        if self.fMappedCtrl == CONTROL_INDEX_NONE:
            title = self.tr("Unmapped")
        elif self.fMappedCtrl == CONTROL_INDEX_CV:
            title = self.tr("Exposed as CV port")
        elif self.fMappedCtrl == CONTROL_INDEX_MIDI_PITCHBEND:
            title = self.tr("Mapped to MIDI Pitchbend, channel %i" % (self.fMidiChannel,))
        elif self.fMappedCtrl == CONTROL_INDEX_MIDI_LEARN:
            title = self.tr("MIDI Learn active")
        else:
            title = self.tr("Mapped to MIDI control %i, channel %i" % (self.fMappedCtrl, self.fMidiChannel))

        if self.fMappedCtrl != CONTROL_INDEX_NONE:
            title += " (range: %g-%g)" % (self.fMappedMinimum, self.fMappedMaximum)

        actTitle = menu.addAction(title)
        actTitle.setEnabled(False)

        menu.addSeparator()

        actUnmap = menu.addAction(self.tr("Unmap"))

        if self.fMappedCtrl == CONTROL_INDEX_NONE:
            actUnmap.setCheckable(True)
            actUnmap.setChecked(True)

        if self.fCanBeInCV:
            menu.addSection("CV")
            actCV = menu.addAction(self.tr("Expose as CV port"))
            if self.fMappedCtrl == CONTROL_INDEX_CV:
                actCV.setCheckable(True)
                actCV.setChecked(True)
        else:
            actCV = None

        menu.addSection("MIDI")

        if not self.ui.widget.isReadOnly():
            actLearn = menu.addAction(self.tr("MIDI Learn"))

            if self.fMappedCtrl == CONTROL_INDEX_MIDI_LEARN:
                actLearn.setCheckable(True)
                actLearn.setChecked(True)
        else:
            actLearn = None

        menuMIDI = menu.addMenu(self.tr("MIDI Control"))

        if self.fMappedCtrl not in (CONTROL_INDEX_NONE,
                                    CONTROL_INDEX_CV,
                                    CONTROL_INDEX_MIDI_PITCHBEND,
                                    CONTROL_INDEX_MIDI_LEARN):
            action = menuMIDI.menuAction()
            action.setCheckable(True)
            action.setChecked(True)

        inlist = False
        actCCs = []
        for cc in MIDI_CC_LIST:
            action = menuMIDI.addAction(cc)
            actCCs.append(action)

            if self.fMappedCtrl >= 0 and self.fMappedCtrl <= MAX_MIDI_CC_LIST_ITEM:
                ccx = int(cc.split(" [", 1)[0], 10)

                if ccx > self.fMappedCtrl and not inlist:
                    inlist = True
                    action = menuMIDI.addAction(self.tr("%02i [0x%02X] (Custom)" % (self.fMappedCtrl,
                                                                                    self.fMappedCtrl)))
                    action.setCheckable(True)
                    action.setChecked(True)
                    actCCs.append(action)

                elif ccx == self.fMappedCtrl:
                    inlist = True
                    action.setCheckable(True)
                    action.setChecked(True)

        if self.fMappedCtrl > MAX_MIDI_CC_LIST_ITEM and self.fMappedCtrl <= 0x77:
            action = menuMIDI.addAction(self.tr("%02i [0x%02X] (Custom)" % (self.fMappedCtrl, self.fMappedCtrl)))
            action.setCheckable(True)
            action.setChecked(True)
            actCCs.append(action)

        actCustomCC = menuMIDI.addAction(self.tr("Custom..."))

        # TODO
        #actPitchbend = menu.addAction(self.tr("MIDI Pitchbend"))

        #if self.fMappedCtrl == CONTROL_INDEX_MIDI_PITCHBEND:
            #actPitchbend.setCheckable(True)
            #actPitchbend.setChecked(True)

        menuChannel = menu.addMenu(self.tr("MIDI Channel"))

        actChannels = []
        for i in range(1, 16+1):
            action = menuChannel.addAction("%i" % i)
            actChannels.append(action)

            if self.fMidiChannel == i:
                action.setCheckable(True)
                action.setChecked(True)

        if self.fMappedCtrl != CONTROL_INDEX_NONE:
            if self.fMappedCtrl == CONTROL_INDEX_CV:
                menu.addSection("Range (Scaled CV input)")
            else:
                menu.addSection("Range (MIDI bounds)")
            actRangeMinimum = menu.addAction(self.tr("Set minimum... (%g)" % self.fMappedMinimum))
            actRangeMaximum = menu.addAction(self.tr("Set maximum... (%g)" % self.fMappedMaximum))
        else:
            actRangeMinimum = actRangeMaximum = None

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            return

        if actSel in actChannels:
            channel = int(actSel.text())
            self.fMidiChannel = channel
            self.updateStatusLabel()
            self.midiChannelChanged.emit(self.fParameterId, channel)
            return

        if actSel == actRangeMinimum:
            value, ok = QInputDialog.getDouble(self,
                                               self.tr("Custom Minimum"),
                                               "Custom minimum value to use:",
                                               self.fMappedMinimum,
                                               self.fMinimum if self.fMappedCtrl != CONTROL_INDEX_CV else -9e6,
                                               self.fMaximum if self.fMappedCtrl != CONTROL_INDEX_CV else 9e6,
                                               self.fDecimalPoints)
            if not ok:
                return

            self.fMappedMinimum = value
            self.mappedRangeChanged.emit(self.fParameterId, self.fMappedMinimum, self.fMappedMaximum)
            return

        if actSel == actRangeMaximum:
            value, ok = QInputDialog.getDouble(self,
                                               self.tr("Custom Maximum"),
                                               "Custom maximum value to use:",
                                               self.fMappedMaximum,
                                               self.fMinimum if self.fMappedCtrl != CONTROL_INDEX_CV else -9e6,
                                               self.fMaximum if self.fMappedCtrl != CONTROL_INDEX_CV else 9e6,
                                               self.fDecimalPoints)
            if not ok:
                return

            self.fMappedMaximum = value
            self.mappedRangeChanged.emit(self.fParameterId, self.fMappedMinimum, self.fMappedMaximum)
            return

        if actSel == actUnmap:
            ctrl = CONTROL_INDEX_NONE
        elif actSel == actCV:
            ctrl = CONTROL_INDEX_CV
        elif actSel == actCustomCC:
            value = self.fMappedCtrl if self.fMappedCtrl >= 0x01 and self.fMappedCtrl <= 0x77 else 1
            ctrl, ok = QInputDialog.getInt(self,
                                           self.tr("Custom CC"),
                                           "Custom MIDI CC to use:",
                                           value,
                                           0x01, 0x77, 1)
            if not ok:
                return
        #elif actSel == actPitchbend:
            #ctrl = CONTROL_INDEX_MIDI_PITCHBEND
        elif actSel == actLearn:
            ctrl = CONTROL_INDEX_MIDI_LEARN
        elif actSel in actCCs:
            ctrl = int(actSel.text().split(" ", 1)[0].replace("&",""), 10)
        else:
            return

        self.fMappedCtrl = ctrl
        self.updateStatusLabel()
        self.mappedControlChanged.emit(self.fParameterId, ctrl)

    @pyqtSlot(bool)
    def slot_parameterDragStateChanged(self, touch):
        self.host.set_parameter_touch(self.fPluginId, self.fParameterId, touch)

    def _textCallBack(self):
        return self.host.get_parameter_text(self.fPluginId, self.fParameterId)

    def _valueCallBack(self, value):
        self.valueChanged.emit(self.fParameterId, value)

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor Parent (Meta class)

class PluginEditParentMeta():
#class PluginEditParentMeta(metaclass=ABCMeta):
    @abstractmethod
    def editDialogVisibilityChanged(self, pluginId, visible):
        raise NotImplementedError

    @abstractmethod
    def editDialogPluginHintsChanged(self, pluginId, hints):
        raise NotImplementedError

    @abstractmethod
    def editDialogParameterValueChanged(self, pluginId, parameterId, value):
        raise NotImplementedError

    @abstractmethod
    def editDialogProgramChanged(self, pluginId, index):
        raise NotImplementedError

    @abstractmethod
    def editDialogMidiProgramChanged(self, pluginId, index):
        raise NotImplementedError

    @abstractmethod
    def editDialogNotePressed(self, pluginId, note):
        raise NotImplementedError

    @abstractmethod
    def editDialogNoteReleased(self, pluginId, note):
        raise NotImplementedError

    @abstractmethod
    def editDialogMidiActivityChanged(self, pluginId, onOff):
        raise NotImplementedError

# ------------------------------------------------------------------------------------------------------------
# Plugin Editor (Built-in)

class PluginEdit(QDialog):
    # signals
    SIGTERM = pyqtSignal()
    SIGUSR1 = pyqtSignal()

    def __init__(self, parent, host, pluginId):
        QDialog.__init__(self, parent.window() if parent is not None else None)
        self.host = host
        self.ui = ui_carla_edit.Ui_PluginEdit()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Internal stuff

        self.fGeometry   = QByteArray()
        self.fParent     = parent
        self.fPluginId   = pluginId
        self.fPluginInfo = None

        self.fCurrentStateFilename = None
        self.fControlChannel = round(host.get_internal_parameter_value(pluginId, PARAMETER_CTRL_CHANNEL))
        self.fFirstInit      = True

        self.fParameterList      = [] # (type, id, widget)
        self.fParametersToUpdate = [] # (id, value)

        self.fPlayingNotes = [] # (channel, note)

        self.fTabIconOff    = QIcon(":/scalable/led_off.svg")
        self.fTabIconOn     = QIcon(":/scalable/led_yellow.svg")
        self.fTabIconTimers = []

        # used during testing
        self.fIdleTimerId = 0

        # -------------------------------------------------------------
        # Set-up GUI

        labelPluginFont = self.ui.label_plugin.font()
        labelPluginFont.setPixelSize(15)
        labelPluginFont.setWeight(75)
        self.ui.label_plugin.setFont(labelPluginFont)

        self.ui.dial_drywet.setCustomPaintMode(self.ui.dial_drywet.CUSTOM_PAINT_MODE_CARLA_WET)
        self.ui.dial_drywet.setImage(3)
        self.ui.dial_drywet.setLabel("Dry/Wet")
        self.ui.dial_drywet.setMinimum(0.0)
        self.ui.dial_drywet.setMaximum(1.0)
        self.ui.dial_drywet.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_DRYWET))

        self.ui.dial_vol.setCustomPaintMode(self.ui.dial_vol.CUSTOM_PAINT_MODE_CARLA_VOL)
        self.ui.dial_vol.setImage(3)
        self.ui.dial_vol.setLabel("Volume")
        self.ui.dial_vol.setMinimum(0.0)
        self.ui.dial_vol.setMaximum(1.27)
        self.ui.dial_vol.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_VOLUME))

        self.ui.dial_b_left.setCustomPaintMode(self.ui.dial_b_left.CUSTOM_PAINT_MODE_CARLA_L)
        self.ui.dial_b_left.setImage(4)
        self.ui.dial_b_left.setLabel("L")
        self.ui.dial_b_left.setMinimum(-1.0)
        self.ui.dial_b_left.setMaximum(1.0)
        self.ui.dial_b_left.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_BALANCE_LEFT))

        self.ui.dial_b_right.setCustomPaintMode(self.ui.dial_b_right.CUSTOM_PAINT_MODE_CARLA_R)
        self.ui.dial_b_right.setImage(4)
        self.ui.dial_b_right.setLabel("R")
        self.ui.dial_b_right.setMinimum(-1.0)
        self.ui.dial_b_right.setMaximum(1.0)
        self.ui.dial_b_right.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_BALANCE_RIGHT))

        self.ui.dial_pan.setCustomPaintMode(self.ui.dial_b_right.CUSTOM_PAINT_MODE_CARLA_PAN)
        self.ui.dial_pan.setImage(4)
        self.ui.dial_pan.setLabel("Pan")
        self.ui.dial_pan.setMinimum(-1.0)
        self.ui.dial_pan.setMaximum(1.0)
        self.ui.dial_pan.setValue(host.get_internal_parameter_value(pluginId, PARAMETER_PANNING))

        self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)

        self.ui.scrollArea = PixmapKeyboardHArea(self)
        self.ui.keyboard   = self.ui.scrollArea.keyboard
        self.ui.keyboard.setEnabled(self.fControlChannel >= 0)
        self.layout().addWidget(self.ui.scrollArea)

        self.ui.scrollArea.setEnabled(False)
        self.ui.scrollArea.setVisible(False)

        # todo
        self.ui.rb_balance.setEnabled(False)
        self.ui.rb_balance.setVisible(False)
        self.ui.rb_pan.setEnabled(False)
        self.ui.rb_pan.setVisible(False)

        flags  = self.windowFlags()
        flags &= ~Qt.WindowContextHelpButtonHint
        self.setWindowFlags(flags)

        self.reloadAll()

        self.fFirstInit = False

        # -------------------------------------------------------------
        # Set-up connections

        self.finished.connect(self.slot_finished)

        self.ui.ch_fixed_buffer.clicked.connect(self.slot_optionChanged)
        self.ui.ch_force_stereo.clicked.connect(self.slot_optionChanged)
        self.ui.ch_map_program_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_use_chunks.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_notes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_program_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_control_changes.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_channel_pressure.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_note_aftertouch.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_pitchbend.clicked.connect(self.slot_optionChanged)
        self.ui.ch_send_all_sound_off.clicked.connect(self.slot_optionChanged)

        self.ui.dial_drywet.realValueChanged.connect(self.slot_dryWetChanged)
        self.ui.dial_vol.realValueChanged.connect(self.slot_volumeChanged)
        self.ui.dial_b_left.realValueChanged.connect(self.slot_balanceLeftChanged)
        self.ui.dial_b_right.realValueChanged.connect(self.slot_balanceRightChanged)
        self.ui.dial_pan.realValueChanged.connect(self.slot_panChanged)
        self.ui.sb_ctrl_channel.valueChanged.connect(self.slot_ctrlChannelChanged)

        self.ui.dial_drywet.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_vol.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_left.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_b_right.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.dial_pan.customContextMenuRequested.connect(self.slot_knobCustomMenu)
        self.ui.sb_ctrl_channel.customContextMenuRequested.connect(self.slot_channelCustomMenu)

        self.ui.keyboard.noteOn.connect(self.slot_noteOn)
        self.ui.keyboard.noteOff.connect(self.slot_noteOff)

        self.ui.cb_programs.currentIndexChanged.connect(self.slot_programIndexChanged)
        self.ui.cb_midi_programs.currentIndexChanged.connect(self.slot_midiProgramIndexChanged)

        self.ui.b_save_state.clicked.connect(self.slot_stateSave)
        self.ui.b_load_state.clicked.connect(self.slot_stateLoad)

        host.NoteOnCallback.connect(self.slot_handleNoteOnCallback)
        host.NoteOffCallback.connect(self.slot_handleNoteOffCallback)
        host.UpdateCallback.connect(self.slot_handleUpdateCallback)
        host.ReloadInfoCallback.connect(self.slot_handleReloadInfoCallback)
        host.ReloadParametersCallback.connect(self.slot_handleReloadParametersCallback)
        host.ReloadProgramsCallback.connect(self.slot_handleReloadProgramsCallback)
        host.ReloadAllCallback.connect(self.slot_handleReloadAllCallback)

    #------------------------------------------------------------------

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velocity):
        if self.fPluginId != pluginId:
            return

        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOn(note, False)

        playItem = (channel, note)

        if playItem not in self.fPlayingNotes:
            self.fPlayingNotes.append(playItem)

        if len(self.fPlayingNotes) == 1 and self.fParent is not None:
            self.fParent.editDialogMidiActivityChanged(self.fPluginId, True)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        if self.fPluginId != pluginId:
            return

        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOff(note, False)

        playItem = (channel, note)

        if playItem in self.fPlayingNotes:
            self.fPlayingNotes.remove(playItem)

        if self.fPlayingNotes and self.fParent is not None:
            self.fParent.editDialogMidiActivityChanged(self.fPluginId, False)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.updateInfo()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        if self.fPluginId == pluginId:
            self.reloadAll()

    #------------------------------------------------------------------

    def updateInfo(self):
        # Update current program text
        if self.ui.cb_programs.count() > 0:
            pIndex = self.ui.cb_programs.currentIndex()
            if pIndex >= 0:
                pName  = self.host.get_program_name(self.fPluginId, pIndex)
                #pName  = pName[:40] + (pName[40:] and "...")
                self.ui.cb_programs.setItemText(pIndex, pName)

        # Update current midi program text
        if self.ui.cb_midi_programs.count() > 0:
            mpIndex = self.ui.cb_midi_programs.currentIndex()
            if mpIndex >= 0:
                mpData  = self.host.get_midi_program_data(self.fPluginId, mpIndex)
                mpBank  = mpData['bank']
                mpProg  = mpData['program']
                mpName  = mpData['name']
                #mpName  = mpName[:40] + (mpName[40:] and "...")
                self.ui.cb_midi_programs.setItemText(mpIndex, "%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

        # Update all parameter values
        for _, paramId, paramWidget in self.fParameterList:
            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

        # and the internal ones too
        self.ui.dial_drywet.blockSignals(True)
        self.ui.dial_drywet.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_DRYWET))
        self.ui.dial_drywet.blockSignals(False)

        self.ui.dial_vol.blockSignals(True)
        self.ui.dial_vol.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_VOLUME))
        self.ui.dial_vol.blockSignals(False)

        self.ui.dial_b_left.blockSignals(True)
        self.ui.dial_b_left.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_BALANCE_LEFT))
        self.ui.dial_b_left.blockSignals(False)

        self.ui.dial_b_right.blockSignals(True)
        self.ui.dial_b_right.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_BALANCE_RIGHT))
        self.ui.dial_b_right.blockSignals(False)

        self.ui.dial_pan.blockSignals(True)
        self.ui.dial_pan.setValue(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_PANNING))
        self.ui.dial_pan.blockSignals(False)

        self.fControlChannel = round(self.host.get_internal_parameter_value(self.fPluginId, PARAMETER_CTRL_CHANNEL))
        self.ui.sb_ctrl_channel.blockSignals(True)
        self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)
        self.ui.sb_ctrl_channel.blockSignals(False)
        self.ui.keyboard.allNotesOff()
        self._updateCtrlPrograms()

        self.fParametersToUpdate = []

    #------------------------------------------------------------------

    def reloadAll(self):
        self.fPluginInfo = self.host.get_plugin_info(self.fPluginId)

        self.reloadInfo()
        self.reloadParameters()
        self.reloadPrograms()

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_save_state.setEnabled(False)

        if not self.ui.scrollArea.isEnabled():
            self.resize(self.width(), self.height()-self.ui.scrollArea.height())

    #------------------------------------------------------------------

    def reloadInfo(self):
        realPluginName = self.host.get_real_plugin_name(self.fPluginId)
        #audioCountInfo = self.host.get_audio_port_count_info(self.fPluginId)
        midiCountInfo  = self.host.get_midi_port_count_info(self.fPluginId)
        #paramCountInfo = self.host.get_parameter_count_info(self.fPluginId)

        pluginHints = self.fPluginInfo['hints']

        self.ui.le_type.setText(getPluginTypeAsString(self.fPluginInfo['type']))

        self.ui.label_name.setEnabled(bool(realPluginName))
        self.ui.le_name.setEnabled(bool(realPluginName))
        self.ui.le_name.setText(realPluginName)
        self.ui.le_name.setToolTip(realPluginName)

        self.ui.label_label.setEnabled(bool(self.fPluginInfo['label']))
        self.ui.le_label.setEnabled(bool(self.fPluginInfo['label']))
        self.ui.le_label.setText(self.fPluginInfo['label'])
        self.ui.le_label.setToolTip(self.fPluginInfo['label'])

        self.ui.label_maker.setEnabled(bool(self.fPluginInfo['maker']))
        self.ui.le_maker.setEnabled(bool(self.fPluginInfo['maker']))
        self.ui.le_maker.setText(self.fPluginInfo['maker'])
        self.ui.le_maker.setToolTip(self.fPluginInfo['maker'])

        self.ui.label_copyright.setEnabled(bool(self.fPluginInfo['copyright']))
        self.ui.le_copyright.setEnabled(bool(self.fPluginInfo['copyright']))
        self.ui.le_copyright.setText(self.fPluginInfo['copyright'])
        self.ui.le_copyright.setToolTip(self.fPluginInfo['copyright'])

        self.ui.label_unique_id.setEnabled(bool(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setEnabled(bool(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setText(str(self.fPluginInfo['uniqueId']))
        self.ui.le_unique_id.setToolTip(str(self.fPluginInfo['uniqueId']))

        self.ui.label_plugin.setText("\n%s\n" % (self.fPluginInfo['name'] or "(none)"))
        self.setWindowTitle(self.fPluginInfo['name'] or "(none)")

        self.ui.dial_drywet.setEnabled(pluginHints & PLUGIN_CAN_DRYWET)
        self.ui.dial_vol.setEnabled(pluginHints & PLUGIN_CAN_VOLUME)
        self.ui.dial_b_left.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_b_right.setEnabled(pluginHints & PLUGIN_CAN_BALANCE)
        self.ui.dial_pan.setEnabled(pluginHints & PLUGIN_CAN_PANNING)

        optsAvailable = self.fPluginInfo['optionsAvailable']
        optsEnabled = self.fPluginInfo['optionsEnabled']
        self.ui.ch_use_chunks.setEnabled(optsAvailable & PLUGIN_OPTION_USE_CHUNKS)
        self.ui.ch_use_chunks.setChecked(optsEnabled & PLUGIN_OPTION_USE_CHUNKS)
        self.ui.ch_fixed_buffer.setEnabled(optsAvailable & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_fixed_buffer.setChecked(optsEnabled & PLUGIN_OPTION_FIXED_BUFFERS)
        self.ui.ch_force_stereo.setEnabled(optsAvailable & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_force_stereo.setChecked(optsEnabled & PLUGIN_OPTION_FORCE_STEREO)
        self.ui.ch_map_program_changes.setEnabled(optsAvailable & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_map_program_changes.setChecked(optsEnabled & PLUGIN_OPTION_MAP_PROGRAM_CHANGES)
        self.ui.ch_send_notes.setEnabled(optsAvailable & PLUGIN_OPTION_SKIP_SENDING_NOTES)
        self.ui.ch_send_notes.setChecked((self.ui.ch_send_notes.isEnabled() and
                                          (optsEnabled & PLUGIN_OPTION_SKIP_SENDING_NOTES) == 0x0))
        self.ui.ch_send_control_changes.setEnabled(optsAvailable & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
        self.ui.ch_send_control_changes.setChecked(optsEnabled & PLUGIN_OPTION_SEND_CONTROL_CHANGES)
        self.ui.ch_send_channel_pressure.setEnabled(optsAvailable & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
        self.ui.ch_send_channel_pressure.setChecked(optsEnabled & PLUGIN_OPTION_SEND_CHANNEL_PRESSURE)
        self.ui.ch_send_note_aftertouch.setEnabled(optsAvailable & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
        self.ui.ch_send_note_aftertouch.setChecked(optsEnabled & PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH)
        self.ui.ch_send_pitchbend.setEnabled(optsAvailable & PLUGIN_OPTION_SEND_PITCHBEND)
        self.ui.ch_send_pitchbend.setChecked(optsEnabled & PLUGIN_OPTION_SEND_PITCHBEND)
        self.ui.ch_send_all_sound_off.setEnabled(optsAvailable & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)
        self.ui.ch_send_all_sound_off.setChecked(optsEnabled & PLUGIN_OPTION_SEND_ALL_SOUND_OFF)

        canSendPrograms = bool((optsAvailable & PLUGIN_OPTION_SEND_PROGRAM_CHANGES) != 0 and
                               (optsEnabled & PLUGIN_OPTION_MAP_PROGRAM_CHANGES) == 0)
        self.ui.ch_send_program_changes.setEnabled(canSendPrograms)
        self.ui.ch_send_program_changes.setChecked(optsEnabled & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)

        self.ui.sw_programs.setCurrentIndex(0 if self.fPluginInfo['type'] in (PLUGIN_VST2, PLUGIN_SFZ) else 1)

        # Show/hide keyboard
        showKeyboard = (self.fPluginInfo['category'] == PLUGIN_CATEGORY_SYNTH or midiCountInfo['ins'] > 0)
        self.ui.scrollArea.setEnabled(showKeyboard)
        self.ui.scrollArea.setVisible(showKeyboard)

        # Force-update parent for new hints
        if self.fParent is not None and not self.fFirstInit:
            self.fParent.editDialogPluginHintsChanged(self.fPluginId, pluginHints)

    def reloadParameters(self):
        # Reset
        self.fParameterList      = []
        self.fParametersToUpdate = []
        self.fTabIconTimers      = []

        # Save current tab state
        tabIndex  = self.ui.tabWidget.currentIndex()
        tabWidget = self.ui.tabWidget.currentWidget()
        scrollVal = tabWidget.verticalScrollBar().value() if isinstance(tabWidget, QScrollArea) else None
        del tabWidget

        # Remove all previous parameters
        for _ in range(self.ui.tabWidget.count()-1):
            self.ui.tabWidget.widget(1).deleteLater()
            self.ui.tabWidget.removeTab(1)

        parameterCount = self.host.get_parameter_count(self.fPluginId)

        # -----------------------------------------------------------------

        if parameterCount <= 0:
            return

        # -----------------------------------------------------------------

        paramInputList   = []
        paramOutputList  = []
        paramInputWidth  = 0
        paramOutputWidth = 0
        unusedParameters = 0

        paramInputListFull  = [] # ([params], width)
        paramOutputListFull = [] # ([params], width)

        for i in range(parameterCount):
            if i - unusedParameters == self.host.maxParameters:
                break

            paramData = self.host.get_parameter_data(self.fPluginId, i)

            if paramData['type'] not in (PARAMETER_INPUT, PARAMETER_OUTPUT):
                unusedParameters += 1
                continue
            if (paramData['hints'] & PARAMETER_IS_ENABLED) == 0:
                unusedParameters += 1
                continue

            paramInfo   = self.host.get_parameter_info(self.fPluginId, i)
            paramRanges = self.host.get_parameter_ranges(self.fPluginId, i)
            paramValue  = self.host.get_current_parameter_value(self.fPluginId, i)

            parameter = {
                'type':  paramData['type'],
                'hints': paramData['hints'],
                'name':  paramInfo['name'],
                'unit':  paramInfo['unit'],
                'scalePoints': [],

                'index':   paramData['index'],
                'default': paramRanges['def'],
                'minimum': paramRanges['min'],
                'maximum': paramRanges['max'],
                'step':    paramRanges['step'],
                'stepSmall': paramRanges['stepSmall'],
                'stepLarge': paramRanges['stepLarge'],
                'mappedControlIndex': paramData['mappedControlIndex'],
                'mappedMinimum': paramData['mappedMinimum'],
                'mappedMaximum': paramData['mappedMaximum'],
                'midiChannel': paramData['midiChannel']+1,

                'comment':   paramInfo['comment'],
                'groupName': paramInfo['groupName'],

                'current': paramValue
            }

            for j in range(paramInfo['scalePointCount']):
                scalePointInfo = self.host.get_parameter_scalepoint_info(self.fPluginId, i, j)

                parameter['scalePoints'].append({
                    'value': scalePointInfo['value'],
                    'label': scalePointInfo['label']
                })

            #parameter['name'] = parameter['name'][:30] + (parameter['name'][30:] and "...")

            # -----------------------------------------------------------------
            # Get width values, in packs of 20

            if parameter['type'] == PARAMETER_INPUT:
                paramInputWidthTMP = fontMetricsHorizontalAdvance(self.fontMetrics(), parameter['name'])

                if paramInputWidthTMP > paramInputWidth:
                    paramInputWidth = paramInputWidthTMP

                paramInputList.append(parameter)

            else:
                paramOutputWidthTMP = fontMetricsHorizontalAdvance(self.fontMetrics(), parameter['name'])

                if paramOutputWidthTMP > paramOutputWidth:
                    paramOutputWidth = paramOutputWidthTMP

                paramOutputList.append(parameter)

        paramInputListFull.append((paramInputList, paramInputWidth))
        paramOutputListFull.append((paramOutputList, paramOutputWidth))

        # Create parameter tabs + widgets
        self._createParameterWidgets(PARAMETER_INPUT,  paramInputListFull,  self.tr("Parameters"))
        self._createParameterWidgets(PARAMETER_OUTPUT, paramOutputListFull, self.tr("Outputs"))

        # Restore tab state
        if tabIndex < self.ui.tabWidget.count():
            self.ui.tabWidget.setCurrentIndex(tabIndex)
            if scrollVal is not None:
                self.ui.tabWidget.currentWidget().verticalScrollBar().setValue(scrollVal)

    def reloadPrograms(self):
        # Programs
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.clear()

        programCount = self.host.get_program_count(self.fPluginId)

        if programCount > 0:
            self.ui.cb_programs.setEnabled(True)
            self.ui.label_programs.setEnabled(True)

            for i in range(programCount):
                pName = self.host.get_program_name(self.fPluginId, i)
                #pName = pName[:40] + (pName[40:] and "...")
                self.ui.cb_programs.addItem(pName)

            self.ui.cb_programs.setCurrentIndex(self.host.get_current_program_index(self.fPluginId))

        else:
            self.ui.cb_programs.setEnabled(False)
            self.ui.label_programs.setEnabled(False)

        self.ui.cb_programs.blockSignals(False)

        # MIDI Programs
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.clear()

        midiProgramCount = self.host.get_midi_program_count(self.fPluginId)

        if midiProgramCount > 0:
            self.ui.cb_midi_programs.setEnabled(True)
            self.ui.label_midi_programs.setEnabled(True)

            for i in range(midiProgramCount):
                mpData = self.host.get_midi_program_data(self.fPluginId, i)
                mpBank = mpData['bank']
                mpProg = mpData['program']
                mpName = mpData['name']
                #mpName = mpName[:40] + (mpName[40:] and "...")

                self.ui.cb_midi_programs.addItem("%03i:%03i - %s" % (mpBank+1, mpProg+1, mpName))

            self.ui.cb_midi_programs.setCurrentIndex(self.host.get_current_midi_program_index(self.fPluginId))

        else:
            self.ui.cb_midi_programs.setEnabled(False)
            self.ui.label_midi_programs.setEnabled(False)

        self.ui.cb_midi_programs.blockSignals(False)

        self.ui.sw_programs.setEnabled(programCount > 0 or midiProgramCount > 0)

        if self.fPluginInfo['type'] == PLUGIN_LV2:
            self.ui.b_load_state.setEnabled(programCount > 0)

    #------------------------------------------------------------------

    def clearNotes(self):
        self.fPlayingNotes = []
        self.ui.keyboard.allNotesOff()

    def noteOn(self, channel, note, velocity):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOn(note, False)

    def noteOff(self, channel, note):
        if self.fControlChannel == channel:
            self.ui.keyboard.sendNoteOff(note, False)

    #------------------------------------------------------------------

    def getHints(self):
        return self.fPluginInfo['hints']

    def setPluginId(self, idx):
        self.fPluginId = idx

    def setName(self, name):
        self.fPluginInfo['name'] = name
        self.ui.label_plugin.setText("\n%s\n" % name)
        self.setWindowTitle(name)

    #------------------------------------------------------------------

    def setParameterValue(self, parameterId, value):
        for paramItem in self.fParametersToUpdate:
            if paramItem[0] == parameterId:
                paramItem[1] = value
                break
        else:
            self.fParametersToUpdate.append([parameterId, value])

    def setParameterDefault(self, parameterId, value):
        for _, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setDefault(value)
                break

    def setParameterMappedControlIndex(self, parameterId, control):
        for _, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMappedControlIndex(control)
                break

    def setParameterMappedRange(self, parameterId, minimum, maximum):
        for _, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMappedRange(minimum, maximum)
                break

    def setParameterMidiChannel(self, parameterId, channel):
        for _, paramId, paramWidget in self.fParameterList:
            if paramId == parameterId:
                paramWidget.setMidiChannel(channel+1)
                break

    def setProgram(self, index):
        self.ui.cb_programs.blockSignals(True)
        self.ui.cb_programs.setCurrentIndex(index)
        self.ui.cb_programs.blockSignals(False)
        self._updateParameterValues()

    def setMidiProgram(self, index):
        self.ui.cb_midi_programs.blockSignals(True)
        self.ui.cb_midi_programs.setCurrentIndex(index)
        self.ui.cb_midi_programs.blockSignals(False)
        self._updateParameterValues()

    def setOption(self, option, yesNo):
        if option == PLUGIN_OPTION_USE_CHUNKS:
            widget = self.ui.ch_use_chunks
        elif option == PLUGIN_OPTION_FIXED_BUFFERS:
            widget = self.ui.ch_fixed_buffer
        elif option == PLUGIN_OPTION_FORCE_STEREO:
            widget = self.ui.ch_force_stereo
        elif option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES:
            widget = self.ui.ch_map_program_changes
        elif option == PLUGIN_OPTION_SKIP_SENDING_NOTES:
            widget = self.ui.ch_send_notes
            yesNo = not yesNo
        elif option == PLUGIN_OPTION_SEND_PROGRAM_CHANGES:
            widget = self.ui.ch_send_program_changes
        elif option == PLUGIN_OPTION_SEND_CONTROL_CHANGES:
            widget = self.ui.ch_send_control_changes
        elif option == PLUGIN_OPTION_SEND_CHANNEL_PRESSURE:
            widget = self.ui.ch_send_channel_pressure
        elif option == PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH:
            widget = self.ui.ch_send_note_aftertouch
        elif option == PLUGIN_OPTION_SEND_PITCHBEND:
            widget = self.ui.ch_send_pitchbend
        elif option == PLUGIN_OPTION_SEND_ALL_SOUND_OFF:
            widget = self.ui.ch_send_all_sound_off
        else:
            return

        widget.blockSignals(True)
        widget.setChecked(yesNo)
        widget.blockSignals(False)

    #------------------------------------------------------------------

    def setVisible(self, yesNo):
        if yesNo:
            if not self.fGeometry.isNull():
                self.restoreGeometry(self.fGeometry)
        else:
            self.fGeometry = self.saveGeometry()

        QDialog.setVisible(self, yesNo)

        if CARLA_OS_MAC and yesNo:
            parent = self.parent()
            if parent is None:
                return
            gCarla.utils.cocoa_set_transient_window_for(self.windowHandle().winId(), parent.windowHandle().winId())

    #------------------------------------------------------------------

    def idleSlow(self):
        # Check Tab icons
        for i in range(len(self.fTabIconTimers)):
            if self.fTabIconTimers[i] == ICON_STATE_ON:
                self.fTabIconTimers[i] = ICON_STATE_WAIT
            elif self.fTabIconTimers[i] == ICON_STATE_WAIT:
                self.fTabIconTimers[i] = ICON_STATE_OFF
            elif self.fTabIconTimers[i] == ICON_STATE_OFF:
                self.fTabIconTimers[i] = ICON_STATE_NULL
                self.ui.tabWidget.setTabIcon(i+1, self.fTabIconOff)

        # Check parameters needing update
        for index, value in self.fParametersToUpdate:
            if index == PARAMETER_DRYWET:
                self.ui.dial_drywet.blockSignals(True)
                self.ui.dial_drywet.setValue(value)
                self.ui.dial_drywet.blockSignals(False)

            elif index == PARAMETER_VOLUME:
                self.ui.dial_vol.blockSignals(True)
                self.ui.dial_vol.setValue(value)
                self.ui.dial_vol.blockSignals(False)

            elif index == PARAMETER_BALANCE_LEFT:
                self.ui.dial_b_left.blockSignals(True)
                self.ui.dial_b_left.setValue(value)
                self.ui.dial_b_left.blockSignals(False)

            elif index == PARAMETER_BALANCE_RIGHT:
                self.ui.dial_b_right.blockSignals(True)
                self.ui.dial_b_right.setValue(value)
                self.ui.dial_b_right.blockSignals(False)

            elif index == PARAMETER_PANNING:
                self.ui.dial_pan.blockSignals(True)
                self.ui.dial_pan.setValue(value)
                self.ui.dial_pan.blockSignals(False)

            elif index == PARAMETER_CTRL_CHANNEL:
                self.fControlChannel = round(value)
                self.ui.sb_ctrl_channel.blockSignals(True)
                self.ui.sb_ctrl_channel.setValue(self.fControlChannel+1)
                self.ui.sb_ctrl_channel.blockSignals(False)
                self.ui.keyboard.allNotesOff()
                self._updateCtrlPrograms()

            elif index >= 0:
                for paramType, paramId, paramWidget in self.fParameterList:
                    if paramId != index:
                        continue
                    # FIXME see below
                    if paramType != PARAMETER_INPUT:
                        continue

                    paramWidget.blockSignals(True)
                    paramWidget.setValue(value)
                    paramWidget.blockSignals(False)

                    #if paramType == PARAMETER_INPUT:
                    tabIndex = paramWidget.getTabIndex()

                    if self.fTabIconTimers[tabIndex-1] == ICON_STATE_NULL:
                        self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOn)

                    self.fTabIconTimers[tabIndex-1] = ICON_STATE_ON
                    break

        # Clear all parameters
        self.fParametersToUpdate = []

        # Update parameter outputs | FIXME needed?
        for paramType, paramId, paramWidget in self.fParameterList:
            if paramType != PARAMETER_OUTPUT:
                continue

            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_stateSave(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            # TODO
            return

        if self.fCurrentStateFilename:
            askTry = QMessageBox.question(self,
                                          self.tr("Overwrite?"),
                                          self.tr("Overwrite previously created file?"),
                                          QMessageBox.Ok|QMessageBox.Cancel)

            if askTry == QMessageBox.Ok:
                self.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)
                return

            self.fCurrentStateFilename = None

        fileFilter = self.tr("Carla State File (*.carxs)")
        filename, _ = QFileDialog.getSaveFileName(self, self.tr("Save Plugin State File"), filter=fileFilter)

        # FIXME use ok value, test if it works as expected
        if not filename:
            return

        if not filename.lower().endswith(".carxs"):
            filename += ".carxs"

        self.fCurrentStateFilename = filename
        self.host.save_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    @pyqtSlot()
    def slot_stateLoad(self):
        if self.fPluginInfo['type'] == PLUGIN_LV2:
            presetList = []

            for i in range(self.host.get_program_count(self.fPluginId)):
                presetList.append("%03i - %s" % (i+1, self.host.get_program_name(self.fPluginId, i)))

            ret = QInputDialog.getItem(self,
                                       self.tr("Open LV2 Preset"),
                                       self.tr("Select an LV2 Preset:"),
                                       presetList, 0, False)

            if ret[1]:
                index = int(ret[0].split(" - ", 1)[0])-1
                self.host.set_program(self.fPluginId, index)
                self.setMidiProgram(-1)

            return

        fileFilter = self.tr("Carla State File (*.carxs)")
        filename, _ = QFileDialog.getOpenFileName(self, self.tr("Open Plugin State File"), filter=fileFilter)

        # FIXME use ok value, test if it works as expected
        if not filename:
            return

        self.fCurrentStateFilename = filename
        self.host.load_plugin_state(self.fPluginId, self.fCurrentStateFilename)

    #------------------------------------------------------------------

    @pyqtSlot(bool)
    def slot_optionChanged(self, clicked):
        sender = self.sender()

        if sender == self.ui.ch_use_chunks:
            option = PLUGIN_OPTION_USE_CHUNKS
        elif sender == self.ui.ch_fixed_buffer:
            option = PLUGIN_OPTION_FIXED_BUFFERS
        elif sender == self.ui.ch_force_stereo:
            option = PLUGIN_OPTION_FORCE_STEREO
        elif sender == self.ui.ch_map_program_changes:
            option = PLUGIN_OPTION_MAP_PROGRAM_CHANGES
        elif sender == self.ui.ch_send_notes:
            option = PLUGIN_OPTION_SKIP_SENDING_NOTES
            clicked = not clicked
        elif sender == self.ui.ch_send_program_changes:
            option = PLUGIN_OPTION_SEND_PROGRAM_CHANGES
        elif sender == self.ui.ch_send_control_changes:
            option = PLUGIN_OPTION_SEND_CONTROL_CHANGES
        elif sender == self.ui.ch_send_channel_pressure:
            option = PLUGIN_OPTION_SEND_CHANNEL_PRESSURE
        elif sender == self.ui.ch_send_note_aftertouch:
            option = PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH
        elif sender == self.ui.ch_send_pitchbend:
            option = PLUGIN_OPTION_SEND_PITCHBEND
        elif sender == self.ui.ch_send_all_sound_off:
            option = PLUGIN_OPTION_SEND_ALL_SOUND_OFF
        else:
            return

        #--------------------------------------------------------------
        # handle map-program-changes and send-program-changes conflict

        if option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES and clicked:
            self.ui.ch_send_program_changes.setEnabled(False)

            # disable send-program-changes if needed
            if self.ui.ch_send_program_changes.isChecked():
                self.host.set_option(self.fPluginId, PLUGIN_OPTION_SEND_PROGRAM_CHANGES, False)

        #--------------------------------------------------------------
        # set option

        self.host.set_option(self.fPluginId, option, clicked)

        #--------------------------------------------------------------
        # handle map-program-changes and send-program-changes conflict

        if option == PLUGIN_OPTION_MAP_PROGRAM_CHANGES and not clicked:
            self.ui.ch_send_program_changes.setEnabled(
                self.fPluginInfo['optionsAvailable'] & PLUGIN_OPTION_SEND_PROGRAM_CHANGES)

            # restore send-program-changes if needed
            if self.ui.ch_send_program_changes.isChecked():
                self.host.set_option(self.fPluginId, PLUGIN_OPTION_SEND_PROGRAM_CHANGES, True)

    #------------------------------------------------------------------

    @pyqtSlot(float)
    def slot_dryWetChanged(self, value):
        self.host.set_drywet(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_DRYWET, value)

    @pyqtSlot(float)
    def slot_volumeChanged(self, value):
        self.host.set_volume(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_VOLUME, value)

    @pyqtSlot(float)
    def slot_balanceLeftChanged(self, value):
        self.host.set_balance_left(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_BALANCE_LEFT, value)

    @pyqtSlot(float)
    def slot_balanceRightChanged(self, value):
        self.host.set_balance_right(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_BALANCE_RIGHT, value)

    @pyqtSlot(float)
    def slot_panChanged(self, value):
        self.host.set_panning(self.fPluginId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, PARAMETER_PANNING, value)

    @pyqtSlot(int)
    def slot_ctrlChannelChanged(self, value):
        self.fControlChannel = value-1
        self.host.set_ctrl_channel(self.fPluginId, self.fControlChannel)

        self.ui.keyboard.allNotesOff()
        self._updateCtrlPrograms()

    #------------------------------------------------------------------

    @pyqtSlot(int, float)
    def slot_parameterValueChanged(self, parameterId, value):
        self.host.set_parameter_value(self.fPluginId, parameterId, value)

        if self.fParent is not None:
            self.fParent.editDialogParameterValueChanged(self.fPluginId, parameterId, value)

    @pyqtSlot(int, int)
    def slot_parameterMappedControlChanged(self, parameterId, control):
        self.host.set_parameter_mapped_control_index(self.fPluginId, parameterId, control)

    @pyqtSlot(int, float, float)
    def slot_parameterMappedRangeChanged(self, parameterId, minimum, maximum):
        self.host.set_parameter_mapped_range(self.fPluginId, parameterId, minimum, maximum)

    @pyqtSlot(int, int)
    def slot_parameterMidiChannelChanged(self, parameterId, channel):
        self.host.set_parameter_midi_channel(self.fPluginId, parameterId, channel-1)

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_programIndexChanged(self, index):
        self.host.set_program(self.fPluginId, index)

        if self.fParent is not None:
            self.fParent.editDialogProgramChanged(self.fPluginId, index)

        self._updateParameterValues()

    @pyqtSlot(int)
    def slot_midiProgramIndexChanged(self, index):
        self.host.set_midi_program(self.fPluginId, index)

        if self.fParent is not None:
            self.fParent.editDialogMidiProgramChanged(self.fPluginId, index)

        self._updateParameterValues()

    #------------------------------------------------------------------

    @pyqtSlot(int)
    def slot_noteOn(self, note):
        if self.fControlChannel >= 0:
            self.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 100)

        if self.fParent is not None:
            self.fParent.editDialogNotePressed(self.fPluginId, note)

    @pyqtSlot(int)
    def slot_noteOff(self, note):
        if self.fControlChannel >= 0:
            self.host.send_midi_note(self.fPluginId, self.fControlChannel, note, 0)

        if self.fParent is not None:
            self.fParent.editDialogNoteReleased(self.fPluginId, note)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_finished(self):
        if self.fParent is not None:
            self.fParent.editDialogVisibilityChanged(self.fPluginId, False)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_knobCustomMenu(self):
        sender   = self.sender()
        knobName = sender.objectName()

        if knobName == "dial_drywet":
            minimum = 0.0
            maximum = 1.0
            default = 1.0
            label   = "Dry/Wet"
        elif knobName == "dial_vol":
            minimum = 0.0
            maximum = 1.27
            default = 1.0
            label   = "Volume"
        elif knobName == "dial_b_left":
            minimum = -1.0
            maximum = 1.0
            default = -1.0
            label   = "Balance-Left"
        elif knobName == "dial_b_right":
            minimum = -1.0
            maximum = 1.0
            default = 1.0
            label   = "Balance-Right"
        elif knobName == "dial_pan":
            minimum = -1.0
            maximum = 1.0
            default = 0.0
            label   = "Panning"
        else:
            minimum = 0.0
            maximum = 1.0
            default = 0.5
            label   = "Unknown"

        menu = QMenu(self)
        actReset = menu.addAction(self.tr("Reset (%i%%)" % (default*100)))
        menu.addSeparator()
        actMinimum = menu.addAction(self.tr("Set to Minimum (%i%%)" % (minimum*100)))
        actCenter  = menu.addAction(self.tr("Set to Center"))
        actMaximum = menu.addAction(self.tr("Set to Maximum (%i%%)" % (maximum*100)))
        menu.addSeparator()
        actSet = menu.addAction(self.tr("Set value..."))

        if label not in ("Balance-Left", "Balance-Right", "Panning"):
            menu.removeAction(actCenter)

        actSelected = menu.exec_(QCursor.pos())

        if actSelected == actSet:
            current   = minimum + (maximum-minimum)*(float(sender.value())/10000)
            value, ok = QInputDialog.getInt(self,
                                            self.tr("Set value"),
                                            label,
                                            round(current*100.0),
                                            round(minimum*100.0),
                                            round(maximum*100.0),
                                            1)
            if ok:
                value = float(value)/100.0

            if not ok:
                return

        elif actSelected == actMinimum:
            value = minimum
        elif actSelected == actMaximum:
            value = maximum
        elif actSelected == actReset:
            value = default
        elif actSelected == actCenter:
            value = 0.0
        else:
            return

        sender.setValue(value, True)

    #------------------------------------------------------------------

    @pyqtSlot()
    def slot_channelCustomMenu(self):
        menu = QMenu(self)

        actNone = menu.addAction(self.tr("None"))

        if self.fControlChannel+1 == 0:
            actNone.setCheckable(True)
            actNone.setChecked(True)

        for i in range(1, 16+1):
            action = menu.addAction("%i" % i)

            if self.fControlChannel+1 == i:
                action.setCheckable(True)
                action.setChecked(True)

        actSel = menu.exec_(QCursor.pos())

        if not actSel:
            pass
        elif actSel == actNone:
            self.ui.sb_ctrl_channel.setValue(0)
        elif actSel:
            selChannel = int(actSel.text())
            self.ui.sb_ctrl_channel.setValue(selChannel)

    #------------------------------------------------------------------

    def _createParameterWidgets(self, paramType, paramListFull, tabPageName):
        groupWidgets = {}

        for paramList, width in paramListFull:
            if not paramList:
                break

            tabIndex = self.ui.tabWidget.count()

            scrollArea = QScrollArea(self.ui.tabWidget)
            scrollArea.setWidgetResizable(True)
            scrollArea.setFrameStyle(0)

            palette1 = scrollArea.palette()
            palette1.setColor(QPalette.Background, Qt.transparent)
            scrollArea.setPalette(palette1)

            palette2 = scrollArea.palette()
            palette2.setColor(QPalette.Background, palette2.color(QPalette.Button))

            scrollAreaWidget = QWidget(scrollArea)
            scrollAreaLayout = QVBoxLayout(scrollAreaWidget)
            scrollAreaLayout.setSpacing(3)

            for paramInfo in paramList:
                groupName = paramInfo['groupName']
                if groupName:
                    groupSymbol, groupName = groupName.split(":",1)
                    groupLayout, groupWidget = groupWidgets.get(groupSymbol, (None, None))
                    if groupLayout is None:
                        groupWidget = CollapsibleBox(groupName, scrollAreaWidget)
                        groupLayout = groupWidget.getContentLayout()
                        groupWidget.setPalette(palette2)
                        scrollAreaLayout.addWidget(groupWidget)
                        groupWidgets[groupSymbol] = (groupLayout, groupWidget)
                else:
                    groupLayout = scrollAreaLayout
                    groupWidget = scrollAreaWidget

                paramWidget = PluginParameter(groupWidget, self.host, paramInfo, self.fPluginId, tabIndex)
                paramWidget.setLabelWidth(width)
                groupLayout.addWidget(paramWidget)

                self.fParameterList.append((paramType, paramInfo['index'], paramWidget))

                if paramType == PARAMETER_INPUT:
                    paramWidget.valueChanged.connect(self.slot_parameterValueChanged)

                paramWidget.mappedControlChanged.connect(self.slot_parameterMappedControlChanged)
                paramWidget.mappedRangeChanged.connect(self.slot_parameterMappedRangeChanged)
                paramWidget.midiChannelChanged.connect(self.slot_parameterMidiChannelChanged)

            scrollAreaLayout.addStretch()

            scrollArea.setWidget(scrollAreaWidget)

            self.ui.tabWidget.addTab(scrollArea, tabPageName)

            if paramType == PARAMETER_INPUT:
                self.ui.tabWidget.setTabIcon(tabIndex, self.fTabIconOff)

            self.fTabIconTimers.append(ICON_STATE_NULL)

    def _updateCtrlPrograms(self):
        self.ui.keyboard.setEnabled(self.fControlChannel >= 0)

        if self.fPluginInfo['category'] != PLUGIN_CATEGORY_SYNTH:
            return
        if self.fPluginInfo['type'] not in (PLUGIN_INTERNAL, PLUGIN_SF2):
            return

        if self.fControlChannel < 0:
            self.ui.cb_programs.setEnabled(False)
            self.ui.cb_midi_programs.setEnabled(False)
            return

        self.ui.cb_programs.setEnabled(True)
        self.ui.cb_midi_programs.setEnabled(True)

        pIndex = self.host.get_current_program_index(self.fPluginId)

        if self.ui.cb_programs.currentIndex() != pIndex:
            self.setProgram(pIndex)

        mpIndex = self.host.get_current_midi_program_index(self.fPluginId)

        if self.ui.cb_midi_programs.currentIndex() != mpIndex:
            self.setMidiProgram(mpIndex)

    def _updateParameterValues(self):
        for _, paramId, paramWidget in self.fParameterList:
            paramWidget.blockSignals(True)
            paramWidget.setValue(self.host.get_current_parameter_value(self.fPluginId, paramId))
            paramWidget.blockSignals(False)

    #------------------------------------------------------------------

    def testTimer(self):
        self.fIdleTimerId = self.startTimer(50)

        self.SIGTERM.connect(self.testTimerClose)
        gCarla.gui = self
        setUpSignals()

    def testTimerClose(self):
        self.close()
        _app.quit()

    #------------------------------------------------------------------

    def closeEvent(self, event):
        if self.fIdleTimerId != 0:
            self.killTimer(self.fIdleTimerId)
            self.fIdleTimerId = 0

            self.host.engine_close()

        QDialog.closeEvent(self, event)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerId:
            self.host.engine_idle()
            self.idleSlow()

        QDialog.timerEvent(self, event)

# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    from carla_app import CarlaApplication
    from carla_host import initHost as _initHost, loadHostSettings as _loadHostSettings

    _app  = CarlaApplication()
    _host = _initHost("Widgets", None, False, False, False)
    _loadHostSettings(_host)

    _host.engine_init("JACK", "Carla-Widgets")
    _host.add_plugin(BINARY_NATIVE, PLUGIN_DSSI, "/usr/lib/dssi/karplong.so", "karplong", "karplong", 0, None, 0x0)
    _host.set_active(0, True)

    gui2 = PluginEdit(None, _host, 0)
    gui2.testTimer()
    gui2.show()

    _app.exit_exec()
