#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla Backend code
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
from PyQt4.QtGui import QApplication, QMainWindow, QTableWidgetItem

# Imports (Custom Stuff)
import ui_carla, ui_carla_database, ui_carla_refresh
from carla_backend import *
#from shared_settings import *

# set defaults
DEFAULT_PROJECT_FOLDER = HOME
#setDefaultProjectFolder(DEFAULT_PROJECT_FOLDER)
#setDefaultPluginsPaths(LADSPA_PATH, DSSI_PATH, LV2_PATH, VST_PATH, GIG_PATH, SF2_PATH, SFZ_PATH)

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

        #self.m_project_filename = None
        #self.m_pluginCount = 0

        #self._nsmAnnounce2str = ""
        #self._nsmOpen1str = ""
        #self._nsmOpen2str = ""
        #self.nsm_server = None
        #self.nsm_url = None

        #self.m_plugin_list = []
        #for x in range(MAX_DEFAULT_PLUGINS):
            #self.m_plugin_list.append(None)

        # -------------------------------------------------------------
        # Set-up GUI stuff

        self.ui.act_engine_start.setEnabled(False)
        self.ui.act_engine_stop.setEnabled(False)
        self.ui.act_plugin_remove_all.setEnabled(False)

        #self.m_scene = CarlaScene(self, self.graphicsView)
        #self.graphicsView.setScene(self.m_scene)

        self.resize(self.width(), 0)

        #self.m_fakeEdit = PluginEdit(self, -1)
        #self.m_curEdit  = self.m_fakeEdit
        #self.w_edit.layout().addWidget(self.m_curEdit)
        #self.w_edit.layout().addStretch()

        # -------------------------------------------------------------
        # Connect actions to functions

        #self.connect(self.act_file_new, SIGNAL("triggered()"), SLOT("slot_file_new()"))
        #self.connect(self.act_file_open, SIGNAL("triggered()"), SLOT("slot_file_open()"))
        #self.connect(self.act_file_save, SIGNAL("triggered()"), SLOT("slot_file_save()"))
        #self.connect(self.act_file_save_as, SIGNAL("triggered()"), SLOT("slot_file_save_as()"))

        self.connect(self.ui.act_engine_start, SIGNAL("triggered()"), SLOT("slot_startEngine()"))
        self.connect(self.ui.act_engine_stop, SIGNAL("triggered()"), SLOT("slot_stopEngine()"))

        self.connect(self.ui.act_plugin_add, SIGNAL("triggered()"), SLOT("slot_addPlugin()"))
        #self.connect(self.act_plugin_remove_all, SIGNAL("triggered()"), SLOT("slot_remove_all()"))

        #self.connect(self.act_settings_configure, SIGNAL("triggered()"), SLOT("slot_configureCarla()"))
        self.connect(self.ui.act_help_about, SIGNAL("triggered()"), SLOT("slot_aboutCarla()"))
        self.connect(self.ui.act_help_about_qt, SIGNAL("triggered()"), app, SLOT("aboutQt()"))

        #self.connect(self, SIGNAL("SIGUSR1()"), SLOT("slot_handleSIGUSR1()"))
        #self.connect(self, SIGNAL("DebugCallback(int, int, int, double, QString)"), SLOT("slot_handleDebugCallback(int, int, int, double, QString)"))
        #self.connect(self, SIGNAL("ParameterValueCallback(int, int, double)"), SLOT("slot_handleParameterValueCallback(int, int, double)"))
        #self.connect(self, SIGNAL("ParameterMidiChannelCallback(int, int, int)"), SLOT("slot_handleParameterMidiChannelCallback(int, int, int)"))
        #self.connect(self, SIGNAL("ParameterMidiCcCallback(int, int, int)"), SLOT("slot_handleParameterMidiCcCallback(int, int, int)"))
        #self.connect(self, SIGNAL("ProgramCallback(int, int)"), SLOT("slot_handleProgramCallback(int, int)"))
        #self.connect(self, SIGNAL("MidiProgramCallback(int, int)"), SLOT("slot_handleMidiProgramCallback(int, int)"))
        #self.connect(self, SIGNAL("NoteOnCallback(int, int, int, int)"), SLOT("slot_handleNoteOnCallback(int, int, int, int)"))
        #self.connect(self, SIGNAL("NoteOffCallback(int, int, int)"), SLOT("slot_handleNoteOffCallback(int, int, int)"))
        #self.connect(self, SIGNAL("ShowGuiCallback(int, int)"), SLOT("slot_handleShowGuiCallback(int, int)"))
        #self.connect(self, SIGNAL("ResizeGuiCallback(int, int, int)"), SLOT("slot_handleResizeGuiCallback(int, int, int)"))
        #self.connect(self, SIGNAL("UpdateCallback(int)"), SLOT("slot_handleUpdateCallback(int)"))
        #self.connect(self, SIGNAL("ReloadInfoCallback(int)"), SLOT("slot_handleReloadInfoCallback(int)"))
        #self.connect(self, SIGNAL("ReloadParametersCallback(int)"), SLOT("slot_handleReloadParametersCallback(int)"))
        #self.connect(self, SIGNAL("ReloadProgramsCallback(int)"), SLOT("slot_handleReloadProgramsCallback(int)"))
        #self.connect(self, SIGNAL("ReloadAllCallback(int)"), SLOT("slot_handleReloadAllCallback(int)"))
        #self.connect(self, SIGNAL("NSM_AnnounceCallback()"), SLOT("slot_handleNSM_AnnounceCallback()"))
        #self.connect(self, SIGNAL("NSM_Open1Callback()"), SLOT("slot_handleNSM_Open1Callback()"))
        #self.connect(self, SIGNAL("NSM_Open2Callback()"), SLOT("slot_handleNSM_Open2Callback()"))
        #self.connect(self, SIGNAL("NSM_SaveCallback()"), SLOT("slot_handleNSM_SaveCallback()"))
        #self.connect(self, SIGNAL("ErrorCallback(QString)"), SLOT("slot_handleErrorCallback(QString)"))
        #self.connect(self, SIGNAL("QuitCallback()"), SLOT("slot_handleQuitCallback()"))

        #self.TIMER_GUI_STUFF  = self.startTimer(self.m_savedSettings["Main/RefreshInterval"])     # Peaks
        #self.TIMER_GUI_STUFF2 = self.startTimer(self.m_savedSettings["Main/RefreshInterval"] * 2) # LEDs and edit dialog

        #NSM_URL = os.getenv("NSM_URL")

        #if NSM_URL:
            #Carla.host.nsm_announce(NSM_URL, os.getpid())
        #else:
        QTimer.singleShot(0, self, SLOT("slot_startEngine()"))

        #QTimer.singleShot(0, self, SLOT("slot_showInitialWarning()"))

    #def loadProjectLater(self):
        #QTimer.singleShot(0, self.load_project)

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

            self.ui.act_engine_start.setEnabled(True)
            self.ui.act_engine_stop.setEnabled(False)

            audioError = cString(Carla.host.get_last_error())

            if audioError:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s', possible reasons: %s" % (audioDriver, audioError)))
            else:
                QMessageBox.critical(self, self.tr("Error"), self.tr("Could not connect to Audio backend '%s'" % audioDriver))
            return

        self.fEngineStarted   = True
        self.fFirstEngineInit = False

    def stopEngine(self):
        #if self.m_pluginCount > 0:
            #ask = QMessageBox.question(self, self.tr("Warning"), self.tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
                                                                         #"Do you want to do this now?"),
                                                                         #QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
            #if ask == QMessageBox.Yes:
                #self.slot_remove_all()
            #else:
                #return

        if Carla.host.is_engine_running() and not Carla.host.engine_close():
            print(cString(Carla.host.get_last_error()))

        self.fEngineStarted = False

    def addPlugin(self, btype, ptype, filename, name, label, extraStuff, activate):
        if not self.fEngineStarted:
            if activate:
                QMessageBox.warning(self, self.tr("Warning"), self.tr("Cannot add new plugins while engine is stopped"))
            return -1

        if not Carla.host.add_plugin(btype, ptype, filename, name, label, extraStuff):
            CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to load plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)
            return False

        else:
            print("added plugin!!")
            new_plugin_id = 0

            pwidget = PluginWidget(self, new_plugin_id)
            self.ui.w_plugins.layout().addWidget(pwidget)
            #self.m_plugin_list[new_plugin_id] = pwidget
            self.ui.act_plugin_remove_all.setEnabled(True)

            pwidget.ui.peak_in.setRefreshRate(self.fSavedSettings["Main/RefreshInterval"])
            pwidget.ui.peak_out.setRefreshRate(self.fSavedSettings["Main/RefreshInterval"])

            if activate:
                pwidget.setActive(True, True, True)

        #self.m_pluginCount += 1
        return True

    #def pluginWidgetActivated(self, widget):
        #if self.m_curEdit == widget:
            #self.keyboard.allNotesOff()

    #def pluginWidgetClicked(self, widget):
        #if self.m_curEdit == widget:
            #return

        #self.w_edit.layout().removeWidget(self.m_curEdit)
        #self.w_edit.layout().insertWidget(0, widget)

        #widget.show()
        #self.m_curEdit.hide()

        #self.m_curEdit = widget

    #@pyqtSlot()
    #def slot_showInitialWarning(self):
        #QMessageBox.warning(self, self.tr("Carla is incomplete"), self.tr(""
            #"The version of Carla you're currently running is incomplete.\n"
            #"Although most things work fine, Carla is not yet in a stable state.\n"
            #"\n"
            #"It will be fully functional for the next Cadence beta release."
            #""))

    @pyqtSlot()
    def slot_startEngine(self):
        self.startEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

    @pyqtSlot()
    def slot_stopEngine(self):
        self.stopEngine()
        check = Carla.host.is_engine_running()
        self.ui.act_file_open.setEnabled(check)
        self.ui.act_engine_start.setEnabled(not check)
        self.ui.act_engine_stop.setEnabled(check)

    #@pyqtSlot()
    #def slot_handleSIGUSR1(self):
        #print("Got SIGUSR1 -> Saving project now")
        #QTimer.singleShot(0, self, SLOT("slot_file_save()"))

    #@pyqtSlot(int, int, int, float, str)
    #def slot_handleDebugCallback(self, plugin_id, value1, value2, value3, valueStr):
        #print("DEBUG :: %i, %i, %i, %f, \"%s\")" % (plugin_id, value1, value2, value3, valueStr))

    #@pyqtSlot(int, int, float)
    #def slot_handleParameterValueCallback(self, pluginId, parameterId, value):
        #pwidget = self.m_plugin_list[pluginId]
        #if pwidget:
            #pwidget.m_parameterIconTimer = ICON_STATE_ON

            #if parameterId == PARAMETER_ACTIVE:
                #pwidget.set_active((value > 0.0), True, False)
            #elif parameterId == PARAMETER_DRYWET:
                #pwidget.set_drywet(value * 1000, True, False)
            #elif parameterId == PARAMETER_VOLUME:
                #pwidget.set_volume(value * 1000, True, False)
            #elif parameterId == PARAMETER_BALANCE_LEFT:
                #pwidget.set_balance_left(value * 1000, True, False)
            #elif parameterId == PARAMETER_BALANCE_RIGHT:
                #pwidget.set_balance_right(value * 1000, True, False)
            #elif parameterId >= 0:
                #pwidget.edit_dialog.set_parameter_to_update(parameterId)

    #@pyqtSlot(int, int, int)
    #def slot_handleParameterMidiChannelCallback(self, pluginId, parameterId, channel):
        #pwidget = self.m_plugin_list[pluginId]
        #if pwidget:
            #pwidget.edit_dialog.set_parameter_midi_channel(parameterId, channel, True)

    #@pyqtSlot(int, int, int)
    #def slot_handleParameterMidiCcCallback(self, pluginId, parameterId, cc):
        #pwidget = self.m_plugin_list[pluginId]
        #if pwidget:
            #pwidget.edit_dialog.set_parameter_midi_cc(parameterId, cc, True)

    #@pyqtSlot(int, int)
    #def slot_handleProgramCallback(self, plugin_id, program_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.set_program(program_id)
            #pwidget.m_parameterIconTimer = ICON_STATE_ON

    #@pyqtSlot(int, int)
    #def slot_handleMidiProgramCallback(self, plugin_id, midi_program_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.set_midi_program(midi_program_id)
            #pwidget.m_parameterIconTimer = ICON_STATE_ON

    #@pyqtSlot(int, int, int, int)
    #def slot_handleNoteOnCallback(self, plugin_id, channel, note, velo):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.keyboard.sendNoteOn(note, False)

    #@pyqtSlot(int, int, int)
    #def slot_handleNoteOffCallback(self, plugin_id, channel, note):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.keyboard.sendNoteOff(note, False)

    #@pyqtSlot(int, int)
    #def slot_handleShowGuiCallback(self, plugin_id, show):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #if show == 0:
                #pwidget.b_gui.setChecked(False)
                #pwidget.b_gui.setEnabled(True)
            #elif show == 1:
                #pwidget.b_gui.setChecked(True)
                #pwidget.b_gui.setEnabled(True)
            #elif show == -1:
                #pwidget.b_gui.setChecked(False)
                #pwidget.b_gui.setEnabled(False)

    #@pyqtSlot(int, int, int)
    #def slot_handleResizeGuiCallback(self, plugin_id, width, height):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #gui_dialog = pwidget.gui_dialog
            #if gui_dialog:
                #gui_dialog.setNewSize(width, height)

    #@pyqtSlot(int)
    #def slot_handleUpdateCallback(self, plugin_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.do_update()

    #@pyqtSlot(int)
    #def slot_handleReloadInfoCallback(self, plugin_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.do_reload_info()

    #@pyqtSlot(int)
    #def slot_handleReloadParametersCallback(self, plugin_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.do_reload_parameters()

    #@pyqtSlot(int)
    #def slot_handleReloadProgramsCallback(self, plugin_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.do_reload_programs()

    #@pyqtSlot(int)
    #def slot_handleReloadAllCallback(self, plugin_id):
        #pwidget = self.m_plugin_list[plugin_id]
        #if pwidget:
            #pwidget.edit_dialog.do_reload_all()

    #@pyqtSlot()
    #def slot_handleNSM_AnnounceCallback(self):
        #smName = self._nsmAnnounce2str

        #self.act_file_new.setEnabled(False)
        #self.act_file_open.setEnabled(False)
        #self.act_file_save_as.setEnabled(False)
        #self.setWindowTitle("Carla (%s)" % smName)

    #@pyqtSlot()
    #def slot_handleNSM_Open1Callback(self):
        #clientId = self._nsmOpen1str

        ## remove all previous plugins
        #self.slot_remove_all()

        ## restart engine
        #if Carla.host.is_engine_running():
            #self.stopEngine()

        #self.startEngine(clientId)

    #@pyqtSlot()
    #def slot_handleNSM_Open2Callback(self):
        #projectPath = self._nsmOpen2str

        #self.m_project_filename = projectPath

        #if os.path.exists(self.m_project_filename):
            #self.load_project()
        #else:
            #self.save_project()

        #self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))
        #Carla.host.nsm_reply_open()

    #@pyqtSlot()
    #def slot_handleNSM_SaveCallback(self):
        #self.save_project()
        #Carla.host.nsm_reply_save()

    #@pyqtSlot(str)
    #def slot_handleErrorCallback(self, error):
        #QMessageBox.critical(self, self.tr("Error"), error)

    #@pyqtSlot()
    #def slot_handleQuitCallback(self):
        #CustomMessageBox(self, QMessageBox.Warning, self.tr("Warning"),
            #self.tr("JACK has been stopped or crashed.\nPlease start JACK and restart Carla"),
            #self.tr("You may want to save your session now..."), QMessageBox.Ok, QMessageBox.Ok)

    #def remove_plugin(self, plugin_id, showError):
        #pwidget = self.m_plugin_list[plugin_id]

        ##if pwidget.edit_dialog == self.m_curEdit:
            ##self.w_edit.layout().removeWidget(self.m_curEdit)
            ##self.w_edit.layout().insertWidget(0, self.m_fakeEdit)

            ##self.m_fakeEdit.show()
            ##self.m_curEdit.hide()

            ##self.m_curEdit = self.m_fakeEdit

        #pwidget.edit_dialog.close()

        #if pwidget.gui_dialog:
            #pwidget.gui_dialog.close()

        #if Carla.host.remove_plugin(plugin_id):
            #pwidget.close()
            #pwidget.deleteLater()
            #self.w_plugins.layout().removeWidget(pwidget)
            #self.m_plugin_list[plugin_id] = None
            #self.m_pluginCount -= 1

        #elif showError:
            #CustomMessageBox(self, QMessageBox.Critical, self.tr("Error"), self.tr("Failed to remove plugin"), cString(Carla.host.get_last_error()), QMessageBox.Ok, QMessageBox.Ok)

        ## push all plugins 1 slot if rack mode
        #if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
            #for i in range(MAX_DEFAULT_PLUGINS-1):
                #if i < plugin_id: continue
                #self.m_plugin_list[i] = self.m_plugin_list[i+1]

                #if self.m_plugin_list[i]:
                    #self.m_plugin_list[i].setId(i)

            #self.m_plugin_list[MAX_DEFAULT_PLUGINS-1] = None

        ## check if there are still plugins
        #for i in range(MAX_DEFAULT_PLUGINS):
            #if self.m_plugin_list[i]: break
        #else:
            #self.act_plugin_remove_all.setEnabled(False)

    #def get_extra_stuff(self, plugin):
        #ptype = plugin['type']

        #if ptype == PLUGIN_LADSPA:
            #unique_id = plugin['unique_id']
            #for rdf_item in self.ladspa_rdf_list:
                #if rdf_item.UniqueID == unique_id:
                    #return pointer(rdf_item)

        #elif ptype == PLUGIN_DSSI:
            #if plugin['hints'] & PLUGIN_HAS_GUI:
                #gui = findDSSIGUI(plugin['binary'], plugin['name'], plugin['label'])
                #if gui:
                    #return gui.encode("utf-8")

        #return c_nullptr

    #def save_project(self):
        #content = ("<?xml version='1.0' encoding='UTF-8'?>\n"
                   #"<!DOCTYPE CARLA-PROJECT>\n"
                   #"<CARLA-PROJECT VERSION='%s'>\n") % (VERSION)

        #first_plugin = True

        #for pwidget in self.m_plugin_list:
            #if pwidget:
                #if not first_plugin:
                    #content += "\n"

                #real_plugin_name = cString(Carla.host.get_real_plugin_name(pwidget.m_pluginId))
                #if real_plugin_name:
                    #content += " <!-- %s -->\n" % xmlSafeString(real_plugin_name, True)

                #content += " <Plugin>\n"
                #content += pwidget.getSaveXMLContent()
                #content += " </Plugin>\n"

                #first_plugin = False

        #content += "</CARLA-PROJECT>\n"

        #try:
            #fd = uopen(self.m_project_filename, "w")
            #fd.write(content)
            #fd.close()
        #except:
            #QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to save project file"))

    #def load_project(self):
        #try:
            #fd = uopen(self.m_project_filename, "r")
            #projectRead = fd.read()
            #fd.close()
        #except:
            #projectRead = None

        #if not projectRead:
            #QMessageBox.critical(self, self.tr("Error"), self.tr("Failed to load project file"))
            #return

        #xml = QDomDocument()
        #xml.setContent(projectRead.encode("utf-8"))

        #xml_node = xml.documentElement()
        #if xml_node.tagName() != "CARLA-PROJECT":
            #QMessageBox.critical(self, self.tr("Error"), self.tr("Not a valid Carla project file"))
            #return

        #x_internal_plugins = None
        #x_ladspa_plugins = None
        #x_dssi_plugins = None
        #x_lv2_plugins = None
        #x_vst_plugins = None
        #x_gig_plugins = None
        #x_sf2_plugins = None
        #x_sfz_plugins = None

        #x_failedPlugins = []
        #x_saveStates = []

        #node = xml_node.firstChild()
        #while not node.isNull():
            #if node.toElement().tagName() == "Plugin":
                #x_saveState = getSaveStateDictFromXML(node)
                #x_saveStates.append(x_saveState)
            #node = node.nextSibling()

        #for x_saveState in x_saveStates:
            #ptype = x_saveState['Type']
            #label = x_saveState['Label']
            #binary = x_saveState['Binary']
            #binaryS = os.path.basename(binary)
            #unique_id = x_saveState['UniqueID']

            #if ptype == "Internal":
                #if not x_internal_plugins: x_internal_plugins = toList(self.settings_db.value("Plugins/Internal", []))
                #x_plugins = x_internal_plugins

            #elif ptype == "LADSPA":
                #if not x_ladspa_plugins:
                    #x_ladspa_plugins  = []
                    #x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_native", []))
                    #x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix32", []))
                    #x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_posix64", []))
                    #x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win32", []))
                    #x_ladspa_plugins += toList(self.settings_db.value("Plugins/LADSPA_win64", []))
                #x_plugins = x_ladspa_plugins

            #elif ptype == "DSSI":
                #if not x_dssi_plugins:
                    #x_dssi_plugins  = []
                    #x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_native", []))
                    #x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix32", []))
                    #x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_posix64", []))
                    #x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win32", []))
                    #x_dssi_plugins += toList(self.settings_db.value("Plugins/DSSI_win64", []))
                #x_plugins = x_dssi_plugins

            #elif ptype == "LV2":
                #if not x_lv2_plugins:
                    #x_lv2_plugins  = []
                    #x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_native", []))
                    #x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix32", []))
                    #x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_posix64", []))
                    #x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win32", []))
                    #x_lv2_plugins += toList(self.settings_db.value("Plugins/LV2_win64", []))
                #x_plugins = x_lv2_plugins

            #elif ptype == "VST":
                #if not x_vst_plugins:
                    #x_vst_plugins  = []
                    #x_vst_plugins += toList(self.settings_db.value("Plugins/VST_native", []))
                    #x_vst_plugins += toList(self.settings_db.value("Plugins/VST_posix32", []))
                    #x_vst_plugins += toList(self.settings_db.value("Plugins/VST_posix64", []))
                    #x_vst_plugins += toList(self.settings_db.value("Plugins/VST_win32", []))
                    #x_vst_plugins += toList(self.settings_db.value("Plugins/VST_win64", []))
                #x_plugins = x_vst_plugins

            #elif ptype == "GIG":
                #if not x_gig_plugins: x_gig_plugins = toList(self.settings_db.value("Plugins/GIG", []))
                #x_plugins = x_gig_plugins

            #elif ptype == "SF2":
                #if not x_sf2_plugins: x_sf2_plugins = toList(self.settings_db.value("Plugins/SF2", []))
                #x_plugins = x_sf2_plugins

            #elif ptype == "SFZ":
                #if not x_sfz_plugins: x_sfz_plugins = toList(self.settings_db.value("Plugins/SFZ", []))
                #x_plugins = x_sfz_plugins

            #else:
                #print("load_project() - ptype '%s' not recognized" % ptype)
                #x_failedPlugins.append(x_saveState['Name'])
                #continue

            ## Try UniqueID -> Label -> Binary (full) -> Binary (short)
            #plugin_ulB = None
            #plugin_ulb = None
            #plugin_ul = None
            #plugin_uB = None
            #plugin_ub = None
            #plugin_lB = None
            #plugin_lb = None
            #plugin_u = None
            #plugin_l = None
            #plugin_B = None

            #for _plugins in x_plugins:
                #for x_plugin in _plugins:
                    #if unique_id == x_plugin['unique_id'] and label == x_plugin['label'] and binary == x_plugin['binary']:
                        #plugin_ulB = x_plugin
                        #break
                    #elif unique_id == x_plugin['unique_id'] and label == x_plugin['label'] and binaryS == os.path.basename(x_plugin['binary']):
                        #plugin_ulb = x_plugin
                    #elif unique_id == x_plugin['unique_id'] and label == x_plugin['label']:
                        #plugin_ul = x_plugin
                    #elif unique_id == x_plugin['unique_id'] and binary == x_plugin['binary']:
                        #plugin_uB = x_plugin
                    #elif unique_id == x_plugin['unique_id'] and binaryS == os.path.basename(x_plugin['binary']):
                        #plugin_ub = x_plugin
                    #elif label == x_plugin['label'] and binary == x_plugin['binary']:
                        #plugin_lB = x_plugin
                    #elif label == x_plugin['label'] and binaryS == os.path.basename(x_plugin['binary']):
                        #plugin_lb = x_plugin
                    #elif unique_id == x_plugin['unique_id']:
                        #plugin_u = x_plugin
                    #elif label == x_plugin['label']:
                        #plugin_l = x_plugin
                    #elif binary == x_plugin['binary']:
                        #plugin_B = x_plugin

            ## LADSPA uses UniqueID or binary+label
            #if ptype == "LADSPA":
                #plugin_l = None
                #plugin_B = None

            ## DSSI uses binary+label (UniqueID ignored)
            #elif ptype == "DSSI":
                #plugin_ul = None
                #plugin_uB = None
                #plugin_ub = None
                #plugin_u = None
                #plugin_l = None
                #plugin_B = None

            ## LV2 uses URIs (label in this case)
            #elif ptype == "LV2":
                #plugin_uB = None
                #plugin_ub = None
                #plugin_u = None
                #plugin_B = None

            ## VST uses UniqueID
            #elif ptype == "VST":
                #plugin_lB = None
                #plugin_lb = None
                #plugin_l = None
                #plugin_B = None

            ## Sound Kits use binaries
            #elif ptype in ("GIG", "SF2", "SFZ"):
                #plugin_ul = None
                #plugin_u = None
                #plugin_l = None
                #plugin_B = binary

            #if plugin_ulB:
                #plugin = plugin_ulB
            #elif plugin_ulb:
                #plugin = plugin_ulb
            #elif plugin_ul:
                #plugin = plugin_ul
            #elif plugin_uB:
                #plugin = plugin_uB
            #elif plugin_ub:
                #plugin = plugin_ub
            #elif plugin_lB:
                #plugin = plugin_lB
            #elif plugin_lb:
                #plugin = plugin_lb
            #elif plugin_u:
                #plugin = plugin_u
            #elif plugin_l:
                #plugin = plugin_l
            #elif plugin_B:
                #plugin = plugin_B
            #else:
                #plugin = None

            #if plugin:
                #btype    = plugin['build']
                #ptype    = plugin['type']
                #filename = plugin['binary']
                #name     = x_saveState['Name']
                #label    = plugin['label']
                #extra_stuff   = self.get_extra_stuff(plugin)
                #new_plugin_id = self.add_plugin(btype, ptype, filename, name, label, extra_stuff, False)

                #if new_plugin_id >= 0:
                    #pwidget = self.m_plugin_list[new_plugin_id]
                    #pwidget.loadStateDict(x_saveState)

                #else:
                    #x_failedPlugins.append(x_saveState['Name'])

            #else:
                #x_failedPlugins.append(x_saveState['Name'])

        #if len(x_failedPlugins) > 0:
            #text = self.tr("The following plugins were not found or failed to initialize:\n")
            #for plugin in x_failedPlugins:
                #text += " - %s\n" % plugin

            #self.statusBar().showMessage("State file loaded with errors")
            #QMessageBox.critical(self, self.tr("Error"), text)

        #else:
            #self.statusBar().showMessage("State file loaded sucessfully!")

    #@pyqtSlot()
    #def slot_file_new(self):
        #self.slot_remove_all()
        #self.m_project_filename = None
        #self.setWindowTitle("Carla")

    #@pyqtSlot()
    #def slot_file_open(self):
        #fileFilter  = self.tr("Carla Project File (*.carxp)")
        #filenameTry = QFileDialog.getOpenFileName(self, self.tr("Open Carla Project File"), self.m_savedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

        #if filenameTry:
            #self.m_project_filename = filenameTry
            #self.slot_remove_all()
            #self.load_project()
            #self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))

    #@pyqtSlot()
    #def slot_file_save(self, saveAs=False):
        #if self.m_project_filename == None or saveAs:
            #fileFilter  = self.tr("Carla Project File (*.carxp)")
            #filenameTry = QFileDialog.getSaveFileName(self, self.tr("Save Carla Project File"), self.m_savedSettings["Main/DefaultProjectFolder"], filter=fileFilter)

            #if filenameTry:
                #if not filenameTry.endswith(".carxp"):
                    #filenameTry += ".carxp"

                #self.m_project_filename = filenameTry
                #self.save_project()
                #self.setWindowTitle("Carla - %s" % os.path.basename(self.m_project_filename))

        #else:
            #self.save_project()

    #@pyqtSlot()
    #def slot_file_save_as(self):
        #self.slot_file_save(True)

    @pyqtSlot()
    def slot_addPlugin(self):
        dialog = PluginDatabaseW(self)
        if dialog.exec_():
            btype    = dialog.fRetPlugin['build']
            ptype    = dialog.fRetPlugin['type']
            filename = dialog.fRetPlugin['binary']
            label    = dialog.fRetPlugin['label']
            extraStuff = None #self.get_extra_stuff(dialog.ret_plugin)
            self.addPlugin(btype, ptype, filename, None, label, extraStuff, True)

    #@pyqtSlot()
    #def slot_remove_all(self):
        #h = 0
        #for i in range(MAX_DEFAULT_PLUGINS):
            #pwidget = self.m_plugin_list[i]

            #if not pwidget:
                #continue

            #pwidget.setId(i-h)
            #pwidget.edit_dialog.close()

            #if pwidget.gui_dialog:
                #pwidget.gui_dialog.close()

            #if Carla.host.remove_plugin(i-h):
                #pwidget.close()
                #pwidget.deleteLater()
                #self.w_plugins.layout().removeWidget(pwidget)
                #self.m_plugin_list[i] = None

            #if Carla.processMode == PROCESS_MODE_CONTINUOUS_RACK:
                #h += 1

        #self.m_pluginCount = 0
        #self.act_plugin_remove_all.setEnabled(False)

    @pyqtSlot()
    def slot_aboutCarla(self):
        CarlaAboutW(self).exec_()

    #@pyqtSlot()
    #def slot_configureCarla(self):
        #dialog = SettingsW(self, "carla")
        #if dialog.exec_():
            #self.loadSettings(False)

            #for pwidget in self.m_plugin_list:
                #if pwidget:
                    #pwidget.peak_in.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])
                    #pwidget.peak_out.setRefreshRate(self.m_savedSettings["Main/RefreshInterval"])

    def loadRDFs(self):
        # Save RDF info for later
        self.fLadspaRdfList = []

        if haveLRDF:
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
            "Main/DefaultProjectFolder": settings.value("Main/DefaultProjectFolder", DEFAULT_PROJECT_FOLDER, type=str),
            "Main/RefreshInterval": settings.value("Main/RefreshInterval", 120, type=int)
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

    #def timerEvent(self, event):
        #if event.timerId() == self.TIMER_GUI_STUFF:
            #for pwidget in self.m_plugin_list:
                #if pwidget: pwidget.check_gui_stuff()
            #if self.m_engine_started and self.m_pluginCount > 0:
                #Carla.host.idle_guis()

        #elif event.timerId() == self.TIMER_GUI_STUFF2:
            #for pwidget in self.m_plugin_list:
                #if pwidget: pwidget.check_gui_stuff2()

        #QMainWindow.timerEvent(self, event)

    def closeEvent(self, event):
        #if self.nsm_server:
            #self.nsm_server.stop()

        self.saveSettings()

        #self.slot_remove_all()
        self.stopEngine()

        QMainWindow.closeEvent(self, event)

# ------------------------------------------------------------------------------------------------

def callback_function(ptr, action, pluginId, value1, value2, value3, valueStr):
    if pluginId < 0 or not Carla.gui:
        return

    #if action == CALLBACK_DEBUG:
        #Carla.gui.emit(SIGNAL("DebugCallback(int, int, int, double, QString)"), pluginId, value1, value2, value3, cString(valueStr))
    #elif action == CALLBACK_PARAMETER_VALUE_CHANGED:
        #Carla.gui.emit(SIGNAL("ParameterValueCallback(int, int, double)"), pluginId, value1, value3)
    #elif action == CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
        #Carla.gui.emit(SIGNAL("ParameterMidiChannelCallback(int, int, int)"), pluginId, value1, value2)
    #elif action == CALLBACK_PARAMETER_MIDI_CC_CHANGED:
        #Carla.gui.emit(SIGNAL("ParameterMidiCcCallback(int, int, int)"), pluginId, value1, value2)
    #elif action == CALLBACK_PROGRAM_CHANGED:
        #Carla.gui.emit(SIGNAL("ProgramCallback(int, int)"), pluginId, value1)
    #elif action == CALLBACK_MIDI_PROGRAM_CHANGED:
        #Carla.gui.emit(SIGNAL("MidiProgramCallback(int, int)"), pluginId, value1)
    #elif action == CALLBACK_NOTE_ON:
        #Carla.gui.emit(SIGNAL("NoteOnCallback(int, int, int, int)"), pluginId, value1, value2, value3)
    #elif action == CALLBACK_NOTE_OFF:
        #Carla.gui.emit(SIGNAL("NoteOffCallback(int, int, int)"), pluginId, value1, value2)
    #elif action == CALLBACK_SHOW_GUI:
        #Carla.gui.emit(SIGNAL("ShowGuiCallback(int, int)"), pluginId, value1)
    #elif action == CALLBACK_UPDATE:
        #Carla.gui.emit(SIGNAL("UpdateCallback(int)"), pluginId)
    #elif action == CALLBACK_RELOAD_INFO:
        #Carla.gui.emit(SIGNAL("ReloadInfoCallback(int)"), pluginId)
    #elif action == CALLBACK_RELOAD_PARAMETERS:
        #Carla.gui.emit(SIGNAL("ReloadParametersCallback(int)"), pluginId)
    #elif action == CALLBACK_RELOAD_PROGRAMS:
        #Carla.gui.emit(SIGNAL("ReloadProgramsCallback(int)"), pluginId)
    #elif action == CALLBACK_RELOAD_ALL:
        #Carla.gui.emit(SIGNAL("ReloadAllCallback(int)"), pluginId)
    #elif action == CALLBACK_NSM_ANNOUNCE:
        #Carla.gui._nsmAnnounce2str = cString(Carla.host.get_last_error())
        #Carla.gui.emit(SIGNAL("NSM_AnnounceCallback()"))
    #elif action == CALLBACK_NSM_OPEN1:
        #Carla.gui._nsmOpen1str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open1Callback()"))
    #elif action == CALLBACK_NSM_OPEN2:
        #Carla.gui._nsmOpen2str = cString(valueStr)
        #Carla.gui.emit(SIGNAL("NSM_Open2Callback()"))
    #elif action == CALLBACK_NSM_SAVE:
        #Carla.gui.emit(SIGNAL("NSM_SaveCallback()"))
    #elif action == CALLBACK_ERROR:
        #Carla.gui.emit(SIGNAL("ErrorCallback(QString)"), cString(Carla.host.get_last_error()))
    #elif action == CALLBACK_QUIT:
        #Carla.gui.emit(SIGNAL("QuitCallback()"))

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
    Carla.host.set_callback_function(callback_function)
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
    #setUpSignals(Carla.gui)

    # Show GUI
    Carla.gui.show()

    # Load project file if set
    #if projectFilename:
        #Carla.gui.m_project_filename = projectFilename
        #Carla.gui.loadProjectLater()
        #Carla.gui.setWindowTitle("Carla - %s" % os.path.basename(projectFilename)) # FIXME - put in loadProject

    # App-Loop
    ret = app.exec_()

    # Exit properly
    sys.exit(ret)
