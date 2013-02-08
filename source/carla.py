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

from PyQt4.QtCore import QSize
from PyQt4.QtGui import QApplication, QListWidgetItem, QMainWindow

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

import ui_carla
from carla_backend import *
from carla_shared import *
# FIXME, remove later
#from shared_settings import *

# ------------------------------------------------------------------------------------------------------------
# Main Window

class CarlaMainW(QMainWindow):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.ui =  ui_carla.Ui_CarlaMainW()
        self.ui.setupUi(self)

        # -------------------------------------------------------------
        # Load Settings

        self.loadSettings(True)
        self.loadRDFs()

        self.setStyleSheet("""
          QWidget#centralwidget {
            background-color: qlineargradient(spread:pad,
                x1:0.0, y1:0.0,
                x2:0.2, y2:1.0,
                stop:0 rgb( 7,  7,  7),
                stop:1 rgb(28, 28, 28)
            );
          }
        """)

        # -------------------------------------------------------------
        # Internal stuff

        self.fEngineStarted   = False
        self.fFirstEngineInit = False

        self.fProjectFilename = None
        self.fProjectLoading  = False

        self.fPluginCount = 0
        self.fPluginList  = []

        self.fIdleTimerFast = 0
        self.fIdleTimerSlow = 0

        #self._nsmAnnounce2str = ""
        #self._nsmOpen1str = ""
        #self._nsmOpen2str = ""
        #self.nsm_server = None
        #self.nsm_url = None

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.ui.act_engine_start.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        self.resize(self.width(), 0)

        #self.m_fakeEdit = PluginEdit(self, -1)
        #self.m_curEdit  = self.m_fakeEdit
        #self.w_edit.layout().addWidget(self.m_curEdit)
        #self.w_edit.layout().addStretch()

        # -------------------------------------------------------------
        # Connect actions to functions

        self.connect(self.ui.act_file_new, SIGNAL("triggered()"), SLOT("slot_fileNew()"))
        self.connect(self.ui.act_file_open, SIGNAL("triggered()"), SLOT("slot_fileOpen()"))
        self.connect(self.ui.act_file_save, SIGNAL("triggered()"), SLOT("slot_fileSave()"))
        self.connect(self.ui.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_fileSaveAs()"))

        self.connect(self.ui.act_engine_start, SIGNAL("triggered()"), SLOT("slot_engineStart()"))
        self.connect(self.ui.act_engine_stop, SIGNAL("triggered()"), SLOT("slot_engineStop()"))

        self.connect(self.ui.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_pluginAdd()"))
        self.connect(self.ui.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_pluginRemoveAll()"))

        self.connect(self.ui.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))
        self.connect(self.ui.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.ui.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        self.connect(self, SIGNAL("SIGTERM()"), SLOT("slot_handleSIGTERM()"))

        self.connect(self, SIGNAL("DebugCallback(int, int, int, double, QString)"), SLOT("slot_handleDebugCallback(int, int, int, double, QString)"))
        self.connect(self, SIGNAL("PluginAddedCallback(int)"), SLOT("slot_handlePluginAddedCallback(int)"))
        self.connect(self, SIGNAL("PluginRemovedCallback(int)"), SLOT("slot_handlePluginRemovedCallback(int)"))
        self.connect(self, SIGNAL("ParameterValueChangedCallback(int, int, double)"), SLOT("slot_handleParameterValueChangedCallback(int, int, double)"))
        self.connect(self, SIGNAL("ParameterDefaultChangedCallback(int, int, double)"), SLOT("slot_handleParameterDefaultChangedCallback(int, int, double)"))
        self.connect(self, SIGNAL("ParameterMidiChannelChangedCallback(int, int, int)"), SLOT("slot_handleParameterMidiChannelChangedCallback(int, int, int)"))
        self.connect(self, SIGNAL("ParameterMidiCcChangedCallback(int, int, int)"), SLOT("slot_handleParameterMidiCcChangedCallback(int, int, int)"))
        self.connect(self, SIGNAL("ProgramChangedCallback(int, int)"), SLOT("slot_handleProgramChangedCallback(int, int)"))
        self.connect(self, SIGNAL("MidiProgramChangedCallback(int, int)"), SLOT("slot_handleMidiProgramChangedCallback(int, int)"))
        self.connect(self, SIGNAL("NoteOnCallback(int, int, int, int)"), SLOT("slot_handleNoteOnCallback(int, int, int, int)"))
        self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), SLOT("slot_handleNoteOffCallback(int, int, int)"))
        self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), SLOT("slot_handleShowGuiCallback(int, int)"))
        self.connect(self, SIGNAL("UpdateCallback(int)"), SLOT("slot_handleUpdateCallback(int)"))
        self.connect(self, SIGNAL("ReloadInfoCallback(int)"), SLOT("slot_handleReloadInfoCallback(int)"))
        self.connect(self, SIGNAL("ReloadParametersCallback(int)"), SLOT("slot_handleReloadParametersCallback(int)"))
        self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), SLOT("slot_handleReloadProgramsCallback(int)"))
        self.connect(self, SIGNAL("ReloadAllCallback(int)"), SLOT("slot_handleReloadAllCallback(int)"))
        #self.connect(self, SIGNAL("NSM_AnnounceCallback()"), SLOT("slot_handleNSM_AnnounceCallback()"))
        #self.connect(self, SIGNAL("NSM_Open1Callback()"), SLOT("slot_handleNSM_Open1Callback()"))
        #self.connect(self, SIGNAL("NSM_Open2Callback()"), SLOT("slot_handleNSM_Open2Callback()"))
        #self.connect(self, SIGNAL("NSM_SaveCallback()"), SLOT("slot_handleNSM_SaveCallback()"))
        self.connect(self, SIGNAL("ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        #NSM_URL = os.getenv("NSM_URL")

        #if NSM_URL:
            #Carla.host.nsm_announce(NSM_URL, os.getpid())
        #else:
        QTimer.singleShot(0, self, SLOT("slot_engineStart()"))

    def startEngine(self, clientName = "Carla"):
        # ---------------------------------------------
        # Engine settings

        settings = QSettings()

        Carla.processMode    = settings.value("Engine/ProcessMode", PROCESS_MODE_MULTIPLE_CLIENTS, type=int)
        Carla.maxParameters  = settings.value("Engine/MaxParameters", MAX_DEFAULT_PARAMETERS, type=int)

        forceStereo          = settings.value("Engine/ForceStereo", False, type=bool)
        preferPluginBridges  = settings.value("Engine/PreferPluginBridges", False, type=bool)
        preferUiBridges      = settings.value("Engine/PreferUiBridges", True, type=bool)
        useDssiVstChunks     = settings.value("Engine/UseDssiVstChunks", False, type=bool)

        oscUiTimeout         = settings.value("Engine/OscUiTimeout", 40, type=int)
        preferredBufferSize  = settings.value("Engine/PreferredBufferSize", 512, type=int)
        preferredSampleRate  = settings.value("Engine/PreferredSampleRate", 44100, type=int)

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            forceStereo = True
        elif Carla.processMode == PROCESS_MODE_MULTIPLE_CLIENTS and os.getenv("LADISH_APP_NAME"):
            print("LADISH detected but using multiple clients (not allowed), forcing single client now")
            Carla.processMode = PROCESS_MODE_SINGLE_CLIENT

        Carla.host.set_option(OPTION_PROCESS_MODE, Carla.processMode, "")
        Carla.host.set_option(OPTION_MAX_PARAMETERS, Carla.maxParameters, "")
        Carla.host.set_option(OPTION_FORCE_STEREO, forceStereo, "")
        Carla.host.set_option(OPTION_PREFER_PLUGIN_BRIDGES, preferPluginBridges, "")
        Carla.host.set_option(OPTION_PREFER_UI_BRIDGES, preferUiBridges, "")
        Carla.host.set_option(OPTION_USE_DSSI_VST_CHUNKS, useDssiVstChunks, "")
        Carla.host.set_option(OPTION_OSC_UI_TIMEOUT, oscUiTimeout, "")
        Carla.host.set_option(OPTION_PREFERRED_BUFFER_SIZE, preferredBufferSize, "")
        Carla.host.set_option(OPTION_PREFERRED_SAMPLE_RATE, preferredSampleRate, "")

        # ---------------------------------------------
        # start

        audioDriver = settings.value("Engine/AudioDriver", "JACK", type=str)

        if not Carla.host.engine_init(audioDriver, clientName):
            if self.fFirstEngineInit:
                self.fFirstEngineInit = False
                return

            #self.ui.act_engine_start.setEnabled(True)
            #self.ui.act_engine_stop.setEnabled(False)

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons: %s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fEngineStarted   = True
        self.fFirstEngineInit = False

        self.fPluginCount = 0
        self.fPluginList  = []

        if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            maxCount = MAX_RACK_PLUGINS
        elif Carla.processMode == PROCESS_MODE_PATCHBAY:
            maxCount = MAX_PATCHBAY_PLUGINS
        else:
            maxCount = MAX_DEFAULT_PLUGINS

        for x in range(maxCount):
            self.fPluginList.append(None)

        # Peaks
        self.fIdleTimerFast = self.startTimer(self.fSavedSettings["Main/RefreshInterval"])
        # LEDs and edit dialog parameters
        self.fIdleTimerSlow = self.startTimer(self.fSavedSettings["Main/RefreshInterval"]*2)

    def stopEngine(self):
        if self.fPluginCount > 0:
            ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         "Do you want to do this now?"),
                                                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            if ask != QMessageBox.Yes:
                return

            self.removeAllPlugins()

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(cString(Carla.host.get_last_error()))

        self.fEngineStarted = False
        self.fPluginCount = 0
        self.fPluginList  = []

        self.killTimer(self.fIdleTimerFast)
        self.killTimer(self.fIdleTimerSlow)

    def loadProject(self, filename):
        self.fProjectLoading  = True
        self.fProjectFilename = filename
        Carla.host.load_project(filename)

    def loadProjectLater(self, filename):
        self.fProjectLoading  = True
        self.fProjectFilename = filename
        self.setWindowTitle("Carla - %s" % os.path.basename(filename))
        QTimer.singleShot(0, self, SLOT("slot_loadProjectLater()"))

    def saveProject(self):
        Carla.host.save_project(self.fProjectFilename)

    def addPlugin(self, btype, ptype, filename, name, label, extraStuff):
        if not self.fEngineStarted:
            QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return False

        if not Carla.host.add_plugin(btype, ptype, filename, name, label, extraStuff):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
            return False

        return True

    def removeAllPlugins(self):
        while (self.ui.w_plugins.layout().takeAt(0)):
            pass

        for i in range(self.fPluginCount):
            pwidget = self.fPluginList[i]

            if pwidget is None:
                break

            self.fPluginList[i] = None

            pwidget.ui.edit_dialog.close()
            pwidget.close()
            pwidget.deleteLater()
            del pwidget

        self.fPluginCount = 0

    @pyqtSlot()
    def slot_fileNew(self):
        self.removeAllPlugins()
        self.fProjectFilename = None
        self.fProjectLoading  = False
        self.setWindowTitle("Carla")

    @pyqtSlot()
    def slot_fileOpen(self):
        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            # FIXME - show dialog to user
            self.removeAllPlugins()
            self.loadProject(filenameTry)
            self.setWindowTitle("Carla - %s" % os.path.basename(filenameTry))

    @pyqtSlot()
    def slot_fileSave(self, saveAs=False):
        if self.fProjectFilename and not saveAs:
            return self.saveProject()

        fileFilter  = self.tr("Carla Project File (*.carxp)")
        filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.fSavedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        if filenameTry:
            if not filenameTry.endswith(".carxp"):
                filenameTry += ".carxp"

            self.fProjectFilename = filenameTry
            self.saveProject()
            self.setWindowTitle("Carla - %s" % os.path.basename(filenameTry))

    @pyqtSlot()
    def slot_fileSaveAs(self):
        self.slot_fileSave(True)

    @pyqtSlot()
    def slot_loadProjectLater(self):
        Carla.host.load_project(self.fProjectFilename)

    @pyqtSlot()
    def slot_engineStart(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

    @pyqtSlot()
    def slot_engineStop(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

    @pyqtSlot()
    def slot_pluginAdd(self):
        dialog = PluginDatabaseW(self)
        if dialog.exec_():
            btype    = dialog.fRetPlugin['build']
            ptype    = dialog.fRetPlugin['type']
            filename = dialog.fRetPlugin['binary']
            label    = dialog.fRetPlugin['label']
            extraStuff = self.getExtraStuff(dialog.fRetPlugin)
            self.addPlugin(btype, ptype, filename, None, label, extraStuff)

    @pyqtSlot()
    def slot_pluginRemoveAll(self):
        self.removeAllPlugins()

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot()
    def slot_configureCarla(self):
        CarlaAboutW(self).exec_()

    @pyqtSlot()
    def slot_handleSIGUSR1(self):
        print("Got SIGUSR1 -> Saving project now")
        QTimer.singleShot(0, self, SLOT("slot_fileSave()"))

    @pyqtSlot()
    def slot_handleSIGTERM(self):
        print("Got SIGTERM -> Closing now")
        self.close()

    @pyqtSlot(int, int, int, float, str)
    def slot_handleDebugCallback(self, pluginId, value1, value2, value3, valueStr):
        print("DEBUG :: %i, %i, %i, %f, \"%s\")" % (pluginId, value1, value2, value3, valueStr))

    @pyqtSlot(int)
    def slot_handlePluginAddedCallback(self, pluginId, pluginName="todo"):
        pwidget = PluginWidget(self, pluginId)
        pwidget.setRefreshRate(self.fSavedSettings["Main/RefreshInterval"])

        self.ui.w_plugins.layout().addWidget(pwidget)

        self.fPluginCount += 1
        self.fPluginList[pluginId] = pwidget

        if self.fPluginCount == 1:
            self.ui.act_plugin_remove_all.setEnabled(True)

        if not self.fProjectLoading:
            pwidget.setActive(True, True, True)

    @pyqtSlot(int)
    def slot_handlePluginRemovedCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        self.fPluginList[pluginId] = None
        self.fPluginCount -= 1

        self.ui.w_plugins.layout().removeWidget(pwidget)

        pwidget.ui.edit_dialog.close()
        pwidget.close()
        pwidget.deleteLater()
        del pwidget

        # push all plugins 1 slot back
        for i in range(self.fPluginCount):
            if i < pluginId:
                continue

            self.fPluginList[i] = self.fPluginList[i+1]
            self.fPluginList[i].setId(i)

        if self.fPluginCount == 0:
            self.ui.act_plugin_remove_all.setEnabled(False)

    @pyqtSlot(int, int, float)
    def slot_handleParameterValueChangedCallback(self, pluginId, parameterId, value):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterValue(value, True, False)

    @pyqtSlot(int, int, float)
    def slot_handleParameterDefaultChangedCallback(self, pluginId, parameterId, value):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterDefault(value, True, False)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiChannelChangedCallback(self, pluginId, parameterId, channel):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiChannel(parameterId, channel, True)

    @pyqtSlot(int, int, int)
    def slot_handleParameterMidiCcChangedCallback(self, pluginId, parameterId, cc):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setParameterMidiControl(parameterId, cc, True)

    @pyqtSlot(int, int)
    def slot_handleProgramChangedCallback(self, pluginId, programId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setProgram(programId)

    @pyqtSlot(int, int)
    def slot_handleMidiProgramChangedCallback(self, pluginId, midiProgramId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.setMidiProgram(midiProgramId)

    @pyqtSlot(int, int, int, int)
    def slot_handleNoteOnCallback(self, pluginId, channel, note, velo):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.keyboard.sendNoteOn(note, False)

    @pyqtSlot(int, int, int)
    def slot_handleNoteOffCallback(self, pluginId, channel, note):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.keyboard.sendNoteOff(note, False)

    @pyqtSlot(int, int)
    def slot_handleShowGuiCallback(self, pluginId, show):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        if show == 0:
            pwidget.ui.b_gui.setChecked(False)
            pwidget.ui.b_gui.setEnabled(True)
        elif show == 1:
            pwidget.ui.b_gui.setChecked(True)
            pwidget.ui.b_gui.setEnabled(True)
        elif show == -1:
            pwidget.ui.b_gui.setChecked(False)
            pwidget.ui.b_gui.setEnabled(False)

    @pyqtSlot(int)
    def slot_handleUpdateCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.do_update()

    @pyqtSlot(int)
    def slot_handleReloadInfoCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadInfo()

    @pyqtSlot(int)
    def slot_handleReloadParametersCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadParameters()

    @pyqtSlot(int)
    def slot_handleReloadProgramsCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadPrograms()

    @pyqtSlot(int)
    def slot_handleReloadAllCallback(self, pluginId):
        pwidget = self.fPluginList[pluginId]
        if pwidget is None:
            return

        pwidget.ui.edit_dialog.reloadAll()

    @pyqtSlot(str)
    def slot_handleErrorCallback(self, error):
        QMessageBox.critical(self, self.tr("Error"), error)

    @pyqtSlot()
    def slot_handleQuitCallback(self):
        CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            self.tr("Engine has been stopped or crashed.\nPlease restart Carla"),
            self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    def getExtraStuff(self, plugin):
        ptype = plugin['type']

        if ptype == PLUGIN_LADSPA:
            uniqueId = plugin['uniqueId']
            for rdfItem in self.fLadspaRdfList:
                if rdfItem.UniqueID == uniqueId:
                    return pointer(rdfItem)

        elif ptype == PLUGIN_DSSI:
            if (plugin['hints'] & PLUGIN_HAS_GUI):
                gui = findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
                if gui:
                    return gui.encode("utf-8")

        return c_nullptr

    def loadRDFs(self):
        # Save RDF info for later
        self.fLadspaRdfList = []

        if not haveLRDF:
            return

        settingsDir  = os.path.join(HOME, ".config", "Cadence")
        frLadspaFile = os.path.join(settingsDir, "ladspa_rdf.db")

        if os.path.exists(frLadspaFile):
            frLadspa = open(frLadspaFile, 'r')

            try:
                self.fLadspaRdfList = ladspa_rdf.get_c_ladspa_rdfs(json.load(frLadspa))
            except:
                pass

            frLadspa.close()

    def saveSettings(self):
        settings = QSettings()
        settings.setValue("Geometry", self.saveGeometry())
        settings.setValue("ShowToolbar", self.ui.toolBar.isVisible())

    def loadSettings(self, geometry):
        settings = QSettings()

        if geometry:
            self.restoreGeometry(settings.value("Geometry", ""))

            showToolbar = settings.value("ShowToolbar", True, type=bool)
            self.ui.act_settings_show_toolbar.setChecked(showToolbar)
            self.ui.toolBar.setVisible(showToolbar)

        self.fSavedSettings = {
            "Main/DefaultProjectFolder": settings.value("Main/DefaultProjectFolder", HOME, type=str),
            "Main/RefreshInterval": settings.value("Main/RefreshInterval", 50, type=int)
        }

        # ---------------------------------------------
        # plugin checks

        if settings.value("Engine/DisableChecks", False, type=bool):
            os.environ["CARLA_DISCOVERY_NO_PROCESSING_CHECKS"] = "true"

        elif os.getenv("CARLA_DISCOVERY_NO_PROCESSING_CHECKS"):
            os.environ.pop("CARLA_DISCOVERY_NO_PROCESSING_CHECKS")

        # ---------------------------------------------
        # plugin paths

        global LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH
        LADSPA_PATH = toList(settings.value("Paths/LADSPA", LADSPA_PATH))
        DSSI_PATH = toList(settings.value("Paths/DSSI", DSSI_PATH))
        LV2_PATH = toList(settings.value("Paths/LV2", LV2_PATH))
        VST_PATH = toList(settings.value("Paths/VST", VST_PATH))
        GIG_PATH = toList(settings.value("Paths/GIG", GIG_PATH))
        SF2_PATH = toList(settings.value("Paths/SF2", SF2_PATH))
        SFZ_PATH = toList(settings.value("Paths/SFZ", SFZ_PATH))

        os.environ["LADSPA_PATH"] = splitter.join(LADSPA_PATH)
        os.environ["DSSI_PATH"] = splitter.join(DSSI_PATH)
        os.environ["LV2_PATH"] = splitter.join(LV2_PATH)
        os.environ["VST_PATH"] = splitter.join(VST_PATH)
        os.environ["GIG_PATH"] = splitter.join(GIG_PATH)
        os.environ["SF2_PATH"] = splitter.join(SF2_PATH)
        os.environ["SFZ_PATH"] = splitter.join(SFZ_PATH)

    def timerEvent(self, event):
        if event.timerId() == self.fIdleTimerFast:
            Carla.host.engine_idle()

            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleFast()

        elif event.timerId() == self.fIdleTimerSlow:
            for pwidget in self.fPluginList:
                if pwidget is None:
                    break
                pwidget.idleSlow()

        QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        #if self.nsm_server:
            #self.nsm_server.stop()

        self.saveSettings()

        self.removeAllPlugins()
        self.stopEngine()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------

def callbackFunction(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId < 0 or not Carla.gui:
        return

    if action == CALLBACK_DEBUG:
        return Carla.gui.emit(SIGNAL("DebugCallback(int, int, int, double, QString)"), pluginId, value1, value2, value3, cString(valueStr))
    elif action == CALLBACK_PLUGIN_ADDED:
        return Carla.gui.emit(SIGNAL("PluginAddedCallback(int)"), pluginId)
    elif action == CALLBACK_PLUGIN_REMOVED:
        return Carla.gui.emit(SIGNAL("PluginRemovedCallback(int)"), pluginId)
    elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        return Carla.gui.emit(SIGNAL("ParameterValueChangedCallback(int, int, double)"), pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_DEFAULT_CHANGED:
        return Carla.gui.emit(SIGNAL("ParameterDefaultChangedCallback(int, int, double)"), pluginId, value1, value3)
    elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        return Carla.gui.emit(SIGNAL("ParameterMidiChannelChangedCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        return Carla.gui.emit(SIGNAL("ParameterMidiCcChangedCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_PROGRAM_CHANGED:
        return Carla.gui.emit(SIGNAL("ProgramChangedCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        return Carla.gui.emit(SIGNAL("MidiProgramChangedCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_NOTE_ON:
        return Carla.gui.emit(SIGNAL("NoteOnCallback(int, int, int, int)"), pluginId, value1, value2, value3)
    elif action == CALLBACK_NOTE_OFF:
        return Carla.gui.emit(SIGNAL("NoteOffCallback(int, int, int)"), pluginId, value1, value2)
    elif action == CALLBACK_SHOW_GUI:
        return Carla.gui.emit(SIGNAL("ShowGuiCallback(int, int)"), pluginId, value1)
    elif action == CALLBACK_UPDATE:
        return Carla.gui.emit(SIGNAL("UpdateCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_INFO:
        return Carla.gui.emit(SIGNAL("ReloadInfoCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_PARAMETERS:
        return Carla.gui.emit(SIGNAL("ReloadParametersCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_PROGRAMS:
        return Carla.gui.emit(SIGNAL("ReloadProgramsCallback(int)"), pluginId)
    elif action == CALLBACK_RELOAD_ALL:
        return Carla.gui.emit(SIGNAL("ReloadAllCallback(int)"), pluginId)
    #elif action == CALLBACK_NSM_ANNOUNCE:
        #Carla.gui._nsmAnnounce2str = cString(Carla.host.get_last_error())
        #Carla.gui.emit(SIGNAL("NSM_AnnounceCallback()"))
        #return
    #elif action == CALLBACK_NSM_OPEN1:
        #Carla.gui._nsmOpen1str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open1Callback()"))
        #return
    #elif action == CALLBACK_NSM_OPEN2:
        #Carla.gui._nsmOpen2str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open2Callback()"))
        #return
    #elif action == CALLBACK_NSM_SAVE:
        #return Carla.gui.emit(SIGNAL("NSM_SaveCallback()"))
    elif action == CALLBACK_ERROR:
        return Carla.gui.emit(SIGNAL("ErrorCallback(QString)"), valueStr)
    elif action == CALLBACK_QUIT:
        return Carla.gui.emit(SIGNAL("QuitCallback()"))

#--------------- main ------------------
if __name__ == '__main__':
    # App initialization
    app = QApplication(sys.argv)
    app.setApplicationName("Carla")
    app.setApplicationVersion(VERSION)
    app.setOrganizationName("Cadence")
    app.setWindowIcon(QIcon(":/scalable/carla.svg"))

    libPrefix = None
    projectFilename = None

    for i in range(len(app.arguments())):
        if i == 0: continue
        argument = app.arguments()[i]

        if argument.startswith("--with-libprefix="):
            libPrefix = argument.replace("--with-libprefix=", "")

        elif os.path.exists(argument):
            projectFilename = argument

    # Init backend
    Carla.host = Host(libPrefix)
    Carla.host.set_callback_function(callbackFunction)
    Carla.host.set_option(OPTION_PROCESS_NAME, 0, "carla")

    # Set bridge paths
    if carla_bridge_native:
        Carla.host.set_option(OPTION_PATH_BRIDGE_NATIVE, 0, carla_bridge_native)

    if carla_bridge_posix32:
        Carla.host.set_option(OPTION_PATH_BRIDGE_POSIX32, 0, carla_bridge_posix32)

    if carla_bridge_posix64:
        Carla.host.set_option(OPTION_PATH_BRIDGE_POSIX64, 0, carla_bridge_posix64)

    if carla_bridge_win32:
        Carla.host.set_option(OPTION_PATH_BRIDGE_WIN32, 0, carla_bridge_win32)

    if carla_bridge_win64:
        Carla.host.set_option(OPTION_PATH_BRIDGE_WIN64, 0, carla_bridge_win64)

    if WINDOWS:
        if carla_bridge_lv2_windows:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_WINDOWS, 0, carla_bridge_lv2_windows)

        if carla_bridge_vst_hwnd:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_HWND, 0, carla_bridge_vst_hwnd)

    elif MACOS:
        if carla_bridge_lv2_cocoa:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_COCOA, 0, carla_bridge_lv2_cocoa)

        if carla_bridge_vst_cocoa:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_COCOA, 0, carla_bridge_vst_cocoa)

    else:
        if carla_bridge_lv2_gtk2:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_GTK2, 0, carla_bridge_lv2_gtk2)

        if carla_bridge_lv2_gtk3:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_GTK3, 0, carla_bridge_lv2_gtk3)

        if carla_bridge_lv2_qt4:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_QT4, 0, carla_bridge_lv2_qt4)

        if carla_bridge_lv2_qt5:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_QT5, 0, carla_bridge_lv2_qt5)

        if carla_bridge_lv2_x11:
            Carla.host.set_option(OPTION_PATH_BRIDGE_LV2_X11, 0, carla_bridge_lv2_x11)

        if carla_bridge_vst_x11:
            Carla.host.set_option(OPTION_PATH_BRIDGE_VST_X11, 0, carla_bridge_vst_x11)

    # Create GUI and start engine
    Carla.gui = CarlaMainW()

    # Set-up custom signal handling
    setUpSignals(Carla.gui)

    # Show GUI
    Carla.gui.show()

    # Load project file if set
    if projectFilename:
        Carla.gui.loadProjectLater(projectFilename)

    # App-Loop
    ret = app.exec_()

    # Exit properly
    sys.exit(ret)
