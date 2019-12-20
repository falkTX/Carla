/*
 * Carla plugin host
 * Copyright (C) 2011-2019 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "carla_host.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <QtGui/QPainter>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFileSystemModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "ui_carla_host.hpp"

#include "CarlaHost.h"
#include "CarlaUtils.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaString.hpp"

// FIXME put in right place

static QString fixLogText(QString text)
{
    //v , Qt::CaseSensitive
    return text.replace("\x1b[30;1m", "").replace("\x1b[31m", "").replace("\x1b[0m", "");
}

//---------------------------------------------------------------------------------------------------------------------

CarlaHost::CarlaHost()
    : QObject(),
      isControl(false),
      isPlugin(false),
      isRemote(false),
      nsmOK(false),
      processMode(ENGINE_PROCESS_MODE_PATCHBAY),
      transportMode(ENGINE_TRANSPORT_MODE_INTERNAL),
      transportExtra(),
      nextProcessMode(processMode),
      processModeForced(false),
      audioDriverForced(),
      experimental(false),
      exportLV2(false),
      forceStereo(false),
      manageUIs(false),
      maxParameters(false),
      preferPluginBridges(false),
      preferUIBridges(false),
      preventBadBehaviour(false),
      showLogs(false),
      showPluginBridges(false),
      showWineBridges(false),
      uiBridgesTimeout(0),
      uisAlwaysOnTop(false),
      pathBinaries(),
      pathResources() {}

//---------------------------------------------------------------------------------------------------------------------

struct CachedSavedSettings {
    uint _CARLA_KEY_MAIN_REFRESH_INTERVAL = 0;
    bool _CARLA_KEY_MAIN_CONFIRM_EXIT = false;
    bool _CARLA_KEY_CANVAS_FANCY_EYE_CANDY = false;
};

//---------------------------------------------------------------------------------------------------------------------
// Host Private Data

struct CarlaHostWindow::PrivateData {
    Ui::CarlaHostW ui;
    const CarlaHost& host;
    CarlaHostWindow* const hostWindow;

    int fIdleTimerNull = 0;
    int fIdleTimerFast = 0;
    int fIdleTimerSlow = 0;

    bool fLadspaRdfNeedsUpdate = true;
    QStringList fLadspaRdfList;

    int fPluginCount = 0;
    QList<QWidget*> fPluginList;

    void* fPluginDatabaseDialog = nullptr;
    QStringList fFavoritePlugins;

    QCarlaString fProjectFilename;
    bool fIsProjectLoading = false;
    bool fCurrentlyRemovingAllPlugins = false;

    double fLastTransportBPM   = 0.0;
    uint64_t fLastTransportFrame = 0;
    bool fLastTransportState = false;
    uint fBufferSize         = 0;
    double fSampleRate       = 0.0;
    QCarlaString fOscAddressTCP;
    QCarlaString fOscAddressUDP;
    QCarlaString fOscReportedHost;

#ifdef CARLA_OS_MAC
    bool fMacClosingHelper = true;
#endif

    // CancelableActionCallback Box
    void* fCancelableActionBox = nullptr;

    // run a custom action after engine is properly closed
    CustomActions fCustomStopAction = CUSTOM_ACTION_NONE;

    // first attempt of auto-start engine doesn't show an error
    bool fFirstEngineInit = true;

    // to be filled with key-value pairs of current settings
    CachedSavedSettings fSavedSettings;

    QCarlaString fClientName;
    QCarlaString fSessionManagerName;

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff (patchbay)

    QImage fExportImage;

    bool fPeaksCleared = true;

    bool fExternalPatchbay = false;
    QList<uint> fSelectedPlugins;

    int fCanvasWidth  = 0;
    int fCanvasHeight = 0;
    int fMiniCanvasUpdateTimeout = 0;
    bool fWithCanvas;

    //-----------------------------------------------------------------------------------------------------------------
    // GUI stuff (disk)

    QFileSystemModel fDirModel;

    //-----------------------------------------------------------------------------------------------------------------

    // CarlaHostWindow* hostWindow,
    PrivateData(const CarlaHost& h, CarlaHostWindow* const hw, const bool withCanvas)
        : ui(),
          host(h),
          hostWindow(hw),
          fWithCanvas(withCanvas),
          fDirModel(hostWindow)
    {
        ui.setupUi(hostWindow);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Setup

#if 0
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
#endif

    void setLoadRDFsNeeded() noexcept
    {
        fLadspaRdfNeedsUpdate = true;
    }

    void setProperWindowTitle()
    {
        QString title(fClientName);

        /*
        if (fProjectFilename.isNotEmpty() && ! host.nsmOK)
            title += QString(" - %s").arg(os.path.basename(fProjectFilename));
        */
        if (fSessionManagerName.isNotEmpty())
            title += QString(" (%s)").arg(fSessionManagerName);

        hostWindow->setWindowTitle(title);
    }

    void updateBufferSize(const uint newBufferSize)
    {
        if (fBufferSize == newBufferSize)
            return;
        fBufferSize = newBufferSize;
        ui.cb_buffer_size->clear();
        ui.cb_buffer_size->addItem(QString("%1").arg(newBufferSize));
        ui.cb_buffer_size->setCurrentIndex(0);
    }

    void updateSampleRate(const double newSampleRate)
    {
        if (carla_isEqual(fSampleRate, newSampleRate))
            return;
        fSampleRate = newSampleRate;
        ui.cb_sample_rate->clear();
        ui.cb_sample_rate->addItem(QString("%1").arg(newSampleRate));
        ui.cb_sample_rate->setCurrentIndex(0);
        refreshTransport(true);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Files

#if 0
    def makeExtraFilename(self):
        return self.fProjectFilename.rsplit(".",1)[0]+".json"
#endif

    void loadProjectNow()
    {
        if (fProjectFilename.isEmpty())
            return qCritical("ERROR: loading project without filename set");
        /*
        if (host.nsmOK && ! os.path.exists(self.fProjectFilename))
            return;
        */

        projectLoadingStarted();
        fIsProjectLoading = true;

        if (! carla_load_project(fProjectFilename.toUtf8()))
        {
            fIsProjectLoading = false;
            projectLoadingFinished();

            CustomMessageBox(hostWindow,
                             QMessageBox::Critical,
                             tr("Error"),
                             tr("Failed to load project"),
                             carla_get_last_error(),
                             QMessageBox::Ok, QMessageBox::Ok);
        }
    }

#if 0
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

        if not self.fWithCanvas:
            return

        with open(self.makeExtraFilename(), 'w') as fh:
            json.dump({
                'canvas': patchcanvas.saveGroupPositions(),
            }, fh)
#endif

    void projectLoadingStarted()
    {
        ui.rack->setEnabled(false);
        ui.graphicsView->setEnabled(false);
    }

    void projectLoadingFinished()
    {
        ui.rack->setEnabled(true);
        ui.graphicsView->setEnabled(true);

        if (! fWithCanvas)
            return;

        QTimer::singleShot(1000, hostWindow, SLOT(slot_canvasRefresh()));

        /*
        const QString extrafile = makeExtraFilename();
        if not os.path.exists(extrafile):
            return;

        try:
            with open(extrafile, "r") as fh:
                canvasdata = json.load(fh)['canvas']
        except:
            return

        patchcanvas.restoreGroupPositions(canvasdata)
        */
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (menu actions)

    void engineStopFinal()
    {
        killTimers();

        if (carla_is_engine_running())
        {
            if (fCustomStopAction == CUSTOM_ACTION_PROJECT_LOAD)
            {
                removeAllPlugins();
            }
            else if (fPluginCount != 0)
            {
                fCurrentlyRemovingAllPlugins = true;
                projectLoadingStarted();
            }

            if (! carla_remove_all_plugins())
            {
                ui.text_logs->appendPlainText("Failed to remove all plugins, error was:");
                ui.text_logs->appendPlainText(carla_get_last_error());
            }

            if (! carla_engine_close())
            {
                ui.text_logs->appendPlainText("Failed to stop engine, error was:");
                ui.text_logs->appendPlainText(carla_get_last_error());
            }
        }

        if (fCustomStopAction == CUSTOM_ACTION_APP_CLOSE)
        {
            hostWindow->close();
        }
        else if (fCustomStopAction == CUSTOM_ACTION_PROJECT_LOAD)
        {
            hostWindow->slot_engineStart();
            loadProjectNow();
            carla_nsm_ready(NSM_CALLBACK_OPEN);
        }

        fCustomStopAction = CUSTOM_ACTION_NONE;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins

    void removeAllPlugins()
    {
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (menu actions)

    void showAddPluginDialog()
    {
    }

    void showAddJackAppDialog()
    {
    }

    void pluginRemoveAll()
    {
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas

    void clearSideStuff()
    {
    }

    void setupCanvas()
    {
    }

    void updateCanvasInitialPos()
    {
    }

    void updateMiniCanvasLater()
    {
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Settings

    void saveSettings()
    {
        QSafeSettings settings;

        settings.setValue("Geometry", hostWindow->saveGeometry());
        settings.setValue("ShowToolbar", ui.toolBar->isEnabled());

        if (! host.isControl)
            settings.setValue("ShowSidePanel", ui.dockWidget->isEnabled());

        QStringList diskFolders;

        /*
        for i in range(ui.cb_disk.count()):
            diskFolders.append(ui.cb_disk->itemData(i))
        */

        settings.setValue("DiskFolders", diskFolders);
        settings.setValue("LastBPM", fLastTransportBPM);

        settings.setValue("ShowMeters", ui.act_settings_show_meters->isChecked());
        settings.setValue("ShowKeyboard", ui.act_settings_show_keyboard->isChecked());
        settings.setValue("HorizontalScrollBarValue", ui.graphicsView->horizontalScrollBar()->value());
        settings.setValue("VerticalScrollBarValue", ui.graphicsView->verticalScrollBar()->value());

        settings.setValue(CARLA_KEY_ENGINE_TRANSPORT_MODE, host.transportMode);
        settings.setValue(CARLA_KEY_ENGINE_TRANSPORT_EXTRA, host.transportExtra);
    }

    void loadSettings(bool firstTime)
    {
        const QSafeSettings settings;

        if (firstTime)
        {
            const QByteArray geometry(settings.valueByteArray("Geometry"));
            if (! geometry.isNull())
                hostWindow->restoreGeometry(geometry);

            const bool showToolbar = settings.valueBool("ShowToolbar", true);
            ui.act_settings_show_toolbar->setChecked(showToolbar);
            ui.toolBar->setEnabled(showToolbar);
            ui.toolBar->setVisible(showToolbar);

            // if settings.contains("SplitterState"):
                //ui.splitter.restoreState(settings.value("SplitterState", b""))
            //else:
                //ui.splitter.setSizes([210, 99999])

            const bool showSidePanel = settings.valueBool("ShowSidePanel", true) && ! host.isControl;
            ui.act_settings_show_side_panel->setChecked(showSidePanel);
            hostWindow->slot_showSidePanel(showSidePanel);

            QStringList diskFolders;
            diskFolders.append(QDir::homePath());
            diskFolders = settings.valueStringList("DiskFolders", diskFolders);

            ui.cb_disk->setItemData(0, QDir::homePath());

            for (const auto& folder : diskFolders)
            {
                /*
                if i == 0: continue;
                folder = diskFolders[i];
                ui.cb_disk->addItem(os.path.basename(folder), folder);
                */
            }

            //if MACOS and not settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, true, bool):
            //    setUnifiedTitleAndToolBarOnMac(true)

            const bool showMeters = settings.valueBool("ShowMeters", true);
            ui.act_settings_show_meters->setChecked(showMeters);
            ui.peak_in->setVisible(showMeters);
            ui.peak_out->setVisible(showMeters);

            const bool showKeyboard = settings.valueBool("ShowKeyboard", true);
            ui.act_settings_show_keyboard->setChecked(showKeyboard);
            /*
            ui.scrollArea->setVisible(showKeyboard);
            */

            const QSafeSettings settingsDBf("falkTX", "CarlaDatabase2");
            fFavoritePlugins = settingsDBf.valueStringList("PluginDatabase/Favorites");

            QTimer::singleShot(100, hostWindow, SLOT(slot_restoreCanvasScrollbarValues()));
        }

        // TODO - complete this
        fSavedSettings._CARLA_KEY_MAIN_CONFIRM_EXIT = settings.valueBool(CARLA_KEY_MAIN_CONFIRM_EXIT,
                                                                         CARLA_DEFAULT_MAIN_CONFIRM_EXIT);

        fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL = settings.valueUInt(CARLA_KEY_MAIN_REFRESH_INTERVAL,
                                                                             CARLA_DEFAULT_MAIN_REFRESH_INTERVAL);

        fSavedSettings._CARLA_KEY_CANVAS_FANCY_EYE_CANDY = settings.valueBool(CARLA_KEY_CANVAS_FANCY_EYE_CANDY,
                                                                              CARLA_DEFAULT_CANVAS_FANCY_EYE_CANDY);

        /*
        {
            CARLA_KEY_MAIN_PROJECT_FOLDER:      settings.value(CARLA_KEY_MAIN_PROJECT_FOLDER,      CARLA_DEFAULT_MAIN_PROJECT_FOLDER,      str),
            CARLA_KEY_MAIN_EXPERIMENTAL:        settings.value(CARLA_KEY_MAIN_EXPERIMENTAL,        CARLA_DEFAULT_MAIN_EXPERIMENTAL,        bool),
            CARLA_KEY_CANVAS_THEME:             settings.value(CARLA_KEY_CANVAS_THEME,             CARLA_DEFAULT_CANVAS_THEME,             str),
            CARLA_KEY_CANVAS_SIZE:              settings.value(CARLA_KEY_CANVAS_SIZE,              CARLA_DEFAULT_CANVAS_SIZE,              str),
            CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS:  settings.value(CARLA_KEY_CANVAS_AUTO_HIDE_GROUPS,  CARLA_DEFAULT_CANVAS_AUTO_HIDE_GROUPS,  bool),
            CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS: settings.value(CARLA_KEY_CANVAS_AUTO_SELECT_ITEMS, CARLA_DEFAULT_CANVAS_AUTO_SELECT_ITEMS, bool),
            CARLA_KEY_CANVAS_USE_BEZIER_LINES:  settings.value(CARLA_KEY_CANVAS_USE_BEZIER_LINES,  CARLA_DEFAULT_CANVAS_USE_BEZIER_LINES,  bool),
            CARLA_KEY_CANVAS_EYE_CANDY:         settings.value(CARLA_KEY_CANVAS_EYE_CANDY,         CARLA_DEFAULT_CANVAS_EYE_CANDY,         bool),
            CARLA_KEY_CANVAS_USE_OPENGL:        settings.value(CARLA_KEY_CANVAS_USE_OPENGL,        CARLA_DEFAULT_CANVAS_USE_OPENGL,        bool),
            CARLA_KEY_CANVAS_ANTIALIASING:      settings.value(CARLA_KEY_CANVAS_ANTIALIASING,      CARLA_DEFAULT_CANVAS_ANTIALIASING,      int),
            CARLA_KEY_CANVAS_HQ_ANTIALIASING:   settings.value(CARLA_KEY_CANVAS_HQ_ANTIALIASING,   CARLA_DEFAULT_CANVAS_HQ_ANTIALIASING,   bool),
            CARLA_KEY_CANVAS_FULL_REPAINTS:     settings.value(CARLA_KEY_CANVAS_FULL_REPAINTS,     CARLA_DEFAULT_CANVAS_FULL_REPAINTS,     bool),
            CARLA_KEY_CANVAS_INLINE_DISPLAYS:   settings.value(CARLA_KEY_CANVAS_INLINE_DISPLAYS,   CARLA_DEFAULT_CANVAS_INLINE_DISPLAYS,   bool),
            CARLA_KEY_CUSTOM_PAINTING:         (settings.value(CARLA_KEY_MAIN_USE_PRO_THEME,    true,   bool) and
                                                settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR, "Black", str).lower() == "black"),
        }
        */

        const QSafeSettings settings2("falkTX", "Carla2");

        if (host.experimental)
        {
            const bool visible = settings2.valueBool(CARLA_KEY_EXPERIMENTAL_JACK_APPS,
                                                    CARLA_DEFAULT_EXPERIMENTAL_JACK_APPS);
            ui.act_plugin_add_jack->setVisible(visible);
        }
        else
        {
            ui.act_plugin_add_jack->setVisible(false);
        }

        fMiniCanvasUpdateTimeout = fSavedSettings._CARLA_KEY_CANVAS_FANCY_EYE_CANDY ? 1000 : 0;

        /*
        setEngineSettings(host);
        */
        restartTimersIfNeeded();
    }

    void enableTransport(const bool enabled)
    {
        ui.group_transport_controls->setEnabled(enabled);
        ui.group_transport_settings->setEnabled(enabled);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Transport

    void refreshTransport(const bool forced = false)
    {
        if (! ui.l_transport_time->isVisible())
            return;
        if (carla_isZero(fSampleRate) or ! carla_is_engine_running())
            return;

        const CarlaTransportInfo* const timeInfo = carla_get_transport_info();
        const bool     playing  = timeInfo->playing;
        const uint64_t frame    = timeInfo->frame;
        const double   bpm      = timeInfo->bpm;

        if (playing != fLastTransportState || forced)
        {
            if (playing)
            {
                const QIcon icon(":/16x16/media-playback-pause.svgz");
                ui.b_transport_play->setChecked(true);
                ui.b_transport_play->setIcon(icon);
                // ui.b_transport_play->setText(tr("&Pause"));
            }
            else
            {
                const QIcon icon(":/16x16/media-playback-start.svgz");
                ui.b_transport_play->setChecked(false);
                ui.b_transport_play->setIcon(icon);
                // ui.b_play->setText(tr("&Play"));
            }

            fLastTransportState = playing;
        }

        if (frame != fLastTransportFrame || forced)
        {
            fLastTransportFrame = frame;

            const uint32_t time = frame / fSampleRate;
            const uint32_t secs =  time % 60;
            const uint32_t mins = (time / 60) % 60;
            const uint32_t hrs  = (time / 3600) % 60;
            ui.l_transport_time->setText(QString("%1:%2:%3").arg(hrs, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));

            const uint32_t frame1 =  frame % 1000;
            const uint32_t frame2 = (frame / 1000) % 1000;
            const uint32_t frame3 = (frame / 1000000) % 1000;
            ui.l_transport_frame->setText(QString("%1'%2'%3").arg(frame3, 3, 10, QChar('0')).arg(frame2, 3, 10, QChar('0')).arg(frame1, 3, 10, QChar('0')));

            const int32_t bar  = timeInfo->bar;
            const int32_t beat = timeInfo->beat;
            const int32_t tick = timeInfo->tick;
            ui.l_transport_bbt->setText(QString("%1|%2|%3").arg(bar, 3, 10, QChar('0')).arg(beat, 2, 10, QChar('0')).arg(tick, 4, 10, QChar('0')));
        }

        if (carla_isNotEqual(bpm, fLastTransportBPM) || forced)
        {
            fLastTransportBPM = bpm;

            if (bpm > 0.0)
            {
                ui.dsb_transport_bpm->blockSignals(true);
                ui.dsb_transport_bpm->setValue(bpm);
                ui.dsb_transport_bpm->blockSignals(false);
                ui.dsb_transport_bpm->setStyleSheet("");
            }
            else
            {
                ui.dsb_transport_bpm->setStyleSheet("QDoubleSpinBox { color: palette(mid); }");
            }
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Timers

    void startTimers()
    {
        if (fIdleTimerFast == 0)
            fIdleTimerFast = hostWindow->startTimer(fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL);

        if (fIdleTimerSlow == 0)
            fIdleTimerSlow = hostWindow->startTimer(fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL*4);
    }

    void restartTimersIfNeeded()
    {
        if (fIdleTimerFast != 0)
        {
            hostWindow->killTimer(fIdleTimerFast);
            fIdleTimerFast = hostWindow->startTimer(fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL);
        }

        if (fIdleTimerSlow != 0)
        {
            hostWindow->killTimer(fIdleTimerSlow);
            fIdleTimerSlow = hostWindow->startTimer(fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL*4);;
        }
    }

    void killTimers()
    {
        if (fIdleTimerFast != 0)
        {
            hostWindow->killTimer(fIdleTimerFast);
            fIdleTimerFast = 0;
        }

        if (fIdleTimerSlow != 0)
        {
            hostWindow->killTimer(fIdleTimerSlow);
            fIdleTimerSlow = 0;
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff

    void* getExtraPtr(/*self, plugin*/)
    {
        return nullptr;
    }

    void maybeLoadRDFs()
    {
    }

    //-----------------------------------------------------------------------------------------------------------------

    /*
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
    */

    //-----------------------------------------------------------------------------------------------------------------
    // timer event

    void refreshRuntimeInfo(const float load, const int xruns)
    {
        const QString txt1(xruns >= 0 ? QString("%1").arg(xruns) : QString("--"));
        const QString txt2(xruns == 1 ? "" : "s");
        ui.b_xruns->setText(QString("%1 Xrun%2").arg(txt1).arg(txt2));
        ui.pb_dsp_load->setValue(int(load));
    }

    void getAndRefreshRuntimeInfo()
    {
        if (! ui.pb_dsp_load->isVisible())
            return;
        if (! carla_is_engine_running())
            return;
        const CarlaRuntimeEngineInfo* const info = carla_get_runtime_engine_info();
        refreshRuntimeInfo(info->load, info->xruns);
    }

    void idleFast()
    {
        carla_engine_idle();
        refreshTransport();

        if (fPluginCount == 0 || fCurrentlyRemovingAllPlugins)
            return;

        for (auto& pitem : fPluginList)
        {
            if (pitem == nullptr)
                break;

            /*
            pitem->getWidget().idleFast();
            */
        }

        for (uint pluginId : fSelectedPlugins)
        {
            fPeaksCleared = false;
            if (ui.peak_in->isVisible())
            {
                /*
                ui.peak_in->displayMeter(1, carla_get_input_peak_value(pluginId, true))
                ui.peak_in->displayMeter(2, carla_get_input_peak_value(pluginId, false))
                */
            }
            if (ui.peak_out->isVisible())
            {
                /*
                ui.peak_out->displayMeter(1, carla_get_output_peak_value(pluginId, true))
                ui.peak_out->displayMeter(2, carla_get_output_peak_value(pluginId, false))
                */
            }
            return;
        }

        if (fPeaksCleared)
            return;

        fPeaksCleared = true;
        /*
        ui.peak_in->displayMeter(1, 0.0, true);
        ui.peak_in->displayMeter(2, 0.0, true);
        ui.peak_out->displayMeter(1, 0.0, true);
        ui.peak_out->displayMeter(2, 0.0, true);
        */
    }

    void idleSlow()
    {
        getAndRefreshRuntimeInfo();

        if (fPluginCount == 0 || fCurrentlyRemovingAllPlugins)
            return;

        for (auto& pitem : fPluginList)
        {
            if (pitem == nullptr)
                break;

            /*
            pitem->getWidget().idleSlow();
            */
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // color/style change event

    void updateStyle()
    {
        // Rack padding images setup
        QImage rack_imgL(":/bitmaps/rack_padding_left.png");
        QImage rack_imgR(":/bitmaps/rack_padding_right.png");

        const qreal min_value = 0.07;
#if QT_VERSION >= 0x50600
        const qreal value_fix = 1.0/(1.0-rack_imgL.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).pixelColor(0,0).blackF());
#else
        const qreal value_fix = 1.5;
#endif

        const QColor bg_color = ui.rack->palette().window().color();
        const qreal  bg_value = 1.0 - bg_color.blackF();

        QColor pad_color;

        if (carla_isNotZero(bg_value) && bg_value < min_value)
            pad_color = bg_color.lighter(100*min_value/bg_value*value_fix);
        else
            pad_color = QColor::fromHsvF(0.0, 0.0, min_value*value_fix);

        QPainter painter;
        QRect fillRect(rack_imgL.rect().adjusted(-1,-1,1,1));

        painter.begin(&rack_imgL);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.setBrush(pad_color);
        painter.drawRect(fillRect);
        painter.end();

        const QPixmap rack_pixmapL(QPixmap::fromImage(rack_imgL));
        QPalette imgL_palette; //(ui.pad_left->palette());
        imgL_palette.setBrush(QPalette::Window, QBrush(rack_pixmapL));
        ui.pad_left->setPalette(imgL_palette);
        ui.pad_left->setAutoFillBackground(true);

        painter.begin(&rack_imgR);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.setBrush(pad_color);
        painter.drawRect(fillRect);
        painter.end();

        const QPixmap rack_pixmapR(QPixmap::fromImage(rack_imgR));
        QPalette imgR_palette; //(ui.pad_right->palette());
        imgR_palette.setBrush(QPalette::Window, QBrush(rack_pixmapR));
        ui.pad_right->setPalette(imgR_palette);
        ui.pad_right->setAutoFillBackground(true);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // close event

    bool shouldIgnoreClose()
    {
        if (host.isControl || host.isPlugin)
            return false;
        if (fCustomStopAction == CUSTOM_ACTION_APP_CLOSE)
            return false;
        if (fSavedSettings._CARLA_KEY_MAIN_CONFIRM_EXIT)
            return QMessageBox::question(hostWindow,
                                         tr("Quit"),
                                         tr("Are you sure you want to quit Carla?"),
                                         QMessageBox::Yes|QMessageBox::No) == QMessageBox::No;
        return false;
    }
};

//---------------------------------------------------------------------------------------------------------------------
// Carla Host Window

CarlaHostWindow::CarlaHostWindow(CarlaHost& h, const bool withCanvas, QWidget* const parent)
    : QMainWindow(parent),
      host(h),
      self(new PrivateData(h, this, withCanvas))
{
    gCarla.gui = this;

    static const char* const CARLA_CLIENT_NAME = getenv("CARLA_CLIENT_NAME");
    static const char* const LADISH_APP_NAME   = getenv("LADISH_APP_NAME");
    static const char* const NSM_URL           = getenv("NSM_URL");

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff

    // keep application signals alive
    self->fIdleTimerNull = startTimer(1000);

    if (host.isControl)
    {
        self->fClientName         = "Carla-Control";
        self->fSessionManagerName = "Control";
    }
    else if (host.isPlugin)
    {
        self->fClientName         = "Carla-Plugin";
        self->fSessionManagerName = "Plugin";
    }
    else if (LADISH_APP_NAME != nullptr)
    {
        self->fClientName         = LADISH_APP_NAME;
        self->fSessionManagerName = "LADISH";
    }
    else if (NSM_URL != nullptr && host.nsmOK)
    {
        self->fClientName         = "Carla.tmp";
        self->fSessionManagerName = "Non Session Manager TMP";
    }
    else
    {
        self->fClientName         = CARLA_CLIENT_NAME != nullptr ? CARLA_CLIENT_NAME : "Carla";
        self->fSessionManagerName = "";
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (engine stopped)

    if (host.isPlugin || host.isControl)
    {
        self->ui.act_file_save->setVisible(false);
        self->ui.act_engine_start->setEnabled(false);
        self->ui.act_engine_start->setVisible(false);
        self->ui.act_engine_stop->setEnabled(false);
        self->ui.act_engine_stop->setVisible(false);
        self->ui.menu_Engine->setEnabled(false);
        self->ui.menu_Engine->setVisible(false);
        if (QAction* const action = self->ui.menu_Engine->menuAction())
            action->setVisible(false);
        self->ui.tabWidget->removeTab(2);

        if (host.isControl)
        {
            self->ui.act_file_new->setVisible(false);
            self->ui.act_file_open->setVisible(false);
            self->ui.act_file_save_as->setVisible(false);
            self->ui.tabUtils->removeTab(0);
        }
        else
        {
            self->ui.act_file_save_as->setText(tr("Export as..."));

            if (! withCanvas)
                if (QTabBar* const tabBar = self->ui.tabWidget->tabBar())
                    tabBar->hide();
        }
    }
    else
    {
        self->ui.act_engine_start->setEnabled(true);
#ifdef CARLA_OS_WIN
        self->ui.tabWidget->removeTab(2);
#endif
    }

    if (host.isControl)
    {
        self->ui.act_file_refresh->setEnabled(false);
    }
    else
    {
        self->ui.act_file_connect->setEnabled(false);
        self->ui.act_file_connect->setVisible(false);
        self->ui.act_file_refresh->setEnabled(false);
        self->ui.act_file_refresh->setVisible(false);
    }

    if (self->fSessionManagerName.isNotEmpty() && ! host.isPlugin)
        self->ui.act_file_new->setEnabled(false);

    self->ui.act_file_open->setEnabled(false);
    self->ui.act_file_save->setEnabled(false);
    self->ui.act_file_save_as->setEnabled(false);
    self->ui.act_engine_stop->setEnabled(false);
    self->ui.act_plugin_remove_all->setEnabled(false);

    self->ui.act_canvas_show_internal->setChecked(false);
    self->ui.act_canvas_show_internal->setVisible(false);
    self->ui.act_canvas_show_external->setChecked(false);
    self->ui.act_canvas_show_external->setVisible(false);

    self->ui.menu_PluginMacros->setEnabled(false);
    self->ui.menu_Canvas->setEnabled(false);

    QWidget* const dockWidgetTitleBar = new QWidget(this);
    self->ui.dockWidget->setTitleBarWidget(dockWidgetTitleBar);

    if (! withCanvas)
    {
        self->ui.act_canvas_show_internal->setVisible(false);
        self->ui.act_canvas_show_external->setVisible(false);
        self->ui.act_canvas_arrange->setVisible(false);
        self->ui.act_canvas_refresh->setVisible(false);
        self->ui.act_canvas_save_image->setVisible(false);
        self->ui.act_canvas_zoom_100->setVisible(false);
        self->ui.act_canvas_zoom_fit->setVisible(false);
        self->ui.act_canvas_zoom_in->setVisible(false);
        self->ui.act_canvas_zoom_out->setVisible(false);
        self->ui.act_settings_show_meters->setVisible(false);
        self->ui.act_settings_show_keyboard->setVisible(false);
        self->ui.menu_Canvas_Zoom->setEnabled(false);
        self->ui.menu_Canvas_Zoom->setVisible(false);
        if (QAction* const action = self->ui.menu_Canvas_Zoom->menuAction())
            action->setVisible(false);
        self->ui.menu_Canvas->setEnabled(false);
        self->ui.menu_Canvas->setVisible(false);
        if (QAction* const action = self->ui.menu_Canvas->menuAction())
            action->setVisible(false);
        self->ui.tw_miniCanvas->hide();
        self->ui.tabWidget->removeTab(1);
#ifdef CARLA_OS_WIN
        self->ui.tabWidget->tabBar().hide()
#endif
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (disk)

    const QString home(QDir::homePath());
    self->fDirModel.setRootPath(home);

    if (const char* const* const exts = carla_get_supported_file_extensions())
    {
        QStringList filters;
        const QString prefix("*.");

        for (uint i=0; exts[i] != nullptr; ++i)
            filters.append(prefix + exts[i]);

        self->fDirModel.setNameFilters(filters);
    }

    self->ui.fileTreeView->setModel(&self->fDirModel);
    self->ui.fileTreeView->setRootIndex(self->fDirModel.index(home));
    self->ui.fileTreeView->setColumnHidden(1, true);
    self->ui.fileTreeView->setColumnHidden(2, true);
    self->ui.fileTreeView->setColumnHidden(3, true);
    self->ui.fileTreeView->setHeaderHidden(true);

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (transport)

    const QFontMetrics fontMetrics(self->ui.l_transport_bbt->fontMetrics());
    int minValueWidth = fontMetricsHorizontalAdvance(fontMetrics, "000|00|0000");
    int minLabelWidth = fontMetricsHorizontalAdvance(fontMetrics, self->ui.label_transport_frame->text());

    int labelTimeWidth = fontMetricsHorizontalAdvance(fontMetrics, self->ui.label_transport_time->text());
    int labelBBTWidth  = fontMetricsHorizontalAdvance(fontMetrics, self->ui.label_transport_bbt->text());

    if (minLabelWidth < labelTimeWidth)
        minLabelWidth = labelTimeWidth;
    if (minLabelWidth < labelBBTWidth)
        minLabelWidth = labelBBTWidth;

    self->ui.label_transport_frame->setMinimumWidth(minLabelWidth + 3);
    self->ui.label_transport_time->setMinimumWidth(minLabelWidth + 3);
    self->ui.label_transport_bbt->setMinimumWidth(minLabelWidth + 3);

    self->ui.l_transport_bbt->setMinimumWidth(minValueWidth + 3);
    self->ui.l_transport_frame->setMinimumWidth(minValueWidth + 3);
    self->ui.l_transport_time->setMinimumWidth(minValueWidth + 3);

    if (host.isPlugin)
    {
        self->ui.b_transport_play->setEnabled(false);
        self->ui.b_transport_stop->setEnabled(false);
        self->ui.b_transport_backwards->setEnabled(false);
        self->ui.b_transport_forwards->setEnabled(false);
        self->ui.group_transport_controls->setEnabled(false);
        self->ui.group_transport_controls->setVisible(false);
        self->ui.cb_transport_link->setEnabled(false);
        self->ui.cb_transport_link->setVisible(false);
        self->ui.cb_transport_jack->setEnabled(false);
        self->ui.cb_transport_jack->setVisible(false);
        self->ui.dsb_transport_bpm->setEnabled(false);
        self->ui.dsb_transport_bpm->setReadOnly(true);
    }

    self->ui.w_transport->setEnabled(false);

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (rack)

    // self->ui.listWidget->setHostAndParent(self->host, self);

    if (QScrollBar* const sb = self->ui.listWidget->verticalScrollBar())
    {
        self->ui.rackScrollBar->setMinimum(sb->minimum());
        self->ui.rackScrollBar->setMaximum(sb->maximum());
        self->ui.rackScrollBar->setValue(sb->value());

        /*
        sb->rangeChanged.connect(self->ui.rackScrollBar.setRange);
        sb->valueChanged.connect(self->ui.rackScrollBar.setValue);
        self->ui.rackScrollBar->rangeChanged.connect(sb.setRange);
        self->ui.rackScrollBar->valueChanged.connect(sb.setValue);
        */
    }

    self->updateStyle();

    self->ui.rack->setStyleSheet("  \
      CarlaRackList#CarlaRackList { \
        background-color: black;    \
      }                             \
    ");

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (patchbay)

    /*
    self->ui.peak_in->setChannelCount(2);
    self->ui.peak_in->setMeterColor(DigitalPeakMeter.COLOR_BLUE);
    self->ui.peak_in->setMeterOrientation(DigitalPeakMeter.VERTICAL);
    */
    self->ui.peak_in->setFixedWidth(25);

    /*
    self->ui.peak_out->setChannelCount(2);
    self->ui.peak_out->setMeterColor(DigitalPeakMeter.COLOR_GREEN);
    self->ui.peak_out->setMeterOrientation(DigitalPeakMeter.VERTICAL);
    */
    self->ui.peak_out->setFixedWidth(25);

    /*
    self->ui.scrollArea = PixmapKeyboardHArea(self->ui.patchbay);
    self->ui.keyboard   = self->ui.scrollArea.keyboard;
    self->ui.patchbay.layout().addWidget(self->ui.scrollArea, 1, 0, 1, 0);

    self->ui.scrollArea->setEnabled(false);

    self->ui.miniCanvasPreview->setRealParent(self);
    */
    if (QTabBar* const tabBar = self->ui.tw_miniCanvas->tabBar())
        tabBar->hide();

    //-----------------------------------------------------------------------------------------------------------------
    // Set up GUI (special stuff for Mac OS)

#ifdef CARLA_OS_MAC
    self->ui.act_file_quit->setMenuRole(QAction::QuitRole);
    self->ui.act_settings_configure->setMenuRole(QAction::PreferencesRole);
    self->ui.act_help_about->setMenuRole(QAction::AboutRole);
    self->ui.act_help_about_qt->setMenuRole(QAction::AboutQtRole);
    self->ui.menu_Settings->setTitle("Panels");
    if (QAction* const action = self->ui.menu_Help.menuAction())
        action->setVisible(false);
#endif

    //-----------------------------------------------------------------------------------------------------------------
    // Load Settings

    self->loadSettings(true);

    //-----------------------------------------------------------------------------------------------------------------
    // Set-up Canvas

    /*
    if (withCanvas)
    {
        self->scene = patchcanvas.PatchScene(self, self->ui.graphicsView);
        self->ui.graphicsView->setScene(self->scene);

        if (self->fSavedSettings[CARLA_KEY_CANVAS_USE_OPENGL])
        {
            self->ui.glView = QGLWidget(self);
            self->ui.graphicsView.setViewport(self->ui.glView);
        }

        setupCanvas();
    }
    */

    //-----------------------------------------------------------------------------------------------------------------
    // Connect actions to functions

    // TODO

    connect(self->ui.b_transport_play, SIGNAL(clicked(bool)), SLOT(slot_transportPlayPause(bool)));
    connect(self->ui.b_transport_stop, SIGNAL(clicked()), SLOT(slot_transportStop()));
    connect(self->ui.b_transport_backwards, SIGNAL(clicked()), SLOT(slot_transportBackwards()));
    connect(self->ui.b_transport_forwards, SIGNAL(clicked()), SLOT(slot_transportForwards()));
    connect(self->ui.dsb_transport_bpm, SIGNAL(valueChanged(qreal)), SLOT(slot_transportBpmChanged(qreal)));
    connect(self->ui.cb_transport_jack, SIGNAL(clicked(bool)), SLOT(slot_transportJackEnabled(bool)));
    connect(self->ui.cb_transport_link, SIGNAL(clicked(bool)), SLOT(slot_transportLinkEnabled(bool)));

    connect(self->ui.b_xruns, SIGNAL(clicked()), SLOT(slot_xrunClear()));

    connect(&host, SIGNAL(EngineStartedCallback(uint, int, int, int, float, QString)), SLOT(slot_handleEngineStartedCallback(uint, int, int, int, float, QString)));
    connect(&host, SIGNAL(EngineStoppedCallback()), SLOT(slot_handleEngineStoppedCallback()));

    //-----------------------------------------------------------------------------------------------------------------
    // Final setup

    self->ui.text_logs->clear();
    self->setProperWindowTitle();

    // Disable non-supported features
    const char* const* const features = carla_get_supported_features();

    if (! stringArrayContainsString(features, "link"))
    {
        self->ui.cb_transport_link->setEnabled(false);
        self->ui.cb_transport_link->setVisible(false);
    }

    if (! stringArrayContainsString(features, "juce"))
    {
        self->ui.act_help_about_juce->setEnabled(false);
        self->ui.act_help_about_juce->setVisible(false);
    }

    // Plugin needs to have timers always running so it receives messages
    if (host.isPlugin || host.isRemote)
        self->startTimers();

    // Qt needs this so it properly creates & resizes the canvas
    self->ui.tabWidget->blockSignals(true);
    self->ui.tabWidget->setCurrentIndex(1);
    self->ui.tabWidget->setCurrentIndex(0);
    self->ui.tabWidget->blockSignals(false);

    // Start in patchbay tab if using forced patchbay mode
    if (host.processModeForced && host.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        self->ui.tabWidget->setCurrentIndex(1);

    // Load initial project file if set
    if (! (host.isControl || host.isPlugin))
    {
        /*
        projectFile = getInitialProjectFile(QApplication.instance());

        if (projectFile)
            loadProjectLater(projectFile);
        */
    }

    // For NSM we wait for the open message
    if (NSM_URL != nullptr && host.nsmOK)
    {
        carla_nsm_ready(NSM_CALLBACK_INIT);
        return;
    }

    if (! host.isControl)
        QTimer::singleShot(0, this, SLOT(slot_engineStart()));
}

CarlaHostWindow::~CarlaHostWindow()
{
    delete self;
}

//---------------------------------------------------------------------------------------------------------------------
// Files (menu actions)

void CarlaHostWindow::slot_fileNew()
{
}

void CarlaHostWindow::slot_fileOpen()
{
}

void CarlaHostWindow::slot_fileSave(const bool saveAs)
{
}

void CarlaHostWindow::slot_fileSaveAs()
{
}

void CarlaHostWindow::slot_loadProjectNow()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Engine (menu actions)

void CarlaHostWindow::slot_engineStart()
{
    /*
    audioDriver = setEngineSettings(self->host)
    */
    const char* const audioDriver = "JACK";
    const bool firstInit = self->fFirstEngineInit;

    self->fFirstEngineInit = false;
    self->ui.text_logs->appendPlainText("======= Starting engine =======");

    if (carla_engine_init(audioDriver, self->fClientName.toUtf8()))
    {
        if (firstInit && ! (host.isControl or host.isPlugin))
        {
            QSafeSettings settings;
            const double lastBpm = settings.valueDouble("LastBPM", 120.0);
            if (lastBpm >= 20.0)
                carla_transport_bpm(lastBpm);
        }
        return;
    }
    else if (firstInit)
    {
        self->ui.text_logs->appendPlainText("Failed to start engine on first try, ignored");
        return;
    }

    const QCarlaString audioError(carla_get_last_error());

    if (audioError.isNotEmpty())
    {
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("Could not connect to Audio backend '%1', possible reasons:\n%2").arg(audioDriver).arg(audioError));
    }
    else
    {
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("Could not connect to Audio backend '%1'").arg(audioDriver));
    }
}

bool CarlaHostWindow::slot_engineStop(const bool forced)
{
    self->ui.text_logs->appendPlainText("======= Stopping engine =======");

    if (self->fPluginCount == 0)
    {
        self->engineStopFinal();
        return true;
    }

    if (! forced)
    {
        const QMessageBox::StandardButton ask = QMessageBox::question(this,
            tr("Warning"),
            tr("There are still some plugins loaded, you need to remove them to stop the engine.\n"
               "Do you want to do this now?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (ask != QMessageBox::Yes)
            return false;
    }

    return slot_engineStopTryAgain();
}

void CarlaHostWindow::slot_engineConfig()
{
    /*
    dialog = RuntimeDriverSettingsW(self->fParentOrSelf, self->host)

    if not dialog.exec_():
        return

    audioDevice, bufferSize, sampleRate = dialog.getValues()

    if self->host.is_engine_running():
        self->host.set_engine_buffer_size_and_sample_rate(bufferSize, sampleRate)
    else:
        self->host.set_engine_option(ENGINE_OPTION_AUDIO_DEVICE, 0, audioDevice)
        self->host.set_engine_option(ENGINE_OPTION_AUDIO_BUFFER_SIZE, bufferSize, "")
        self->host.set_engine_option(ENGINE_OPTION_AUDIO_SAMPLE_RATE, sampleRate, "")
    */
}

bool CarlaHostWindow::slot_engineStopTryAgain()
{
    if (carla_is_engine_running() && ! carla_set_engine_about_to_close())
    {
        QTimer::singleShot(0, this, SLOT(slot_engineStopTryAgain()));
        return false;
    }

    self->engineStopFinal();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
// Engine (host callbacks)

void CarlaHostWindow::slot_handleEngineStartedCallback(uint pluginCount, int processMode, int transportMode, int bufferSize, float sampleRate, QString driverName)
{
    self->ui.menu_PluginMacros->setEnabled(true);
    self->ui.menu_Canvas->setEnabled(true);
    self->ui.w_transport->setEnabled(true);

    self->ui.act_canvas_show_internal->blockSignals(true);
    self->ui.act_canvas_show_external->blockSignals(true);

    if (processMode == ENGINE_PROCESS_MODE_PATCHBAY) // and not self->host.isPlugin:
    {
        self->ui.act_canvas_show_internal->setChecked(true);
        self->ui.act_canvas_show_internal->setVisible(true);
        self->ui.act_canvas_show_external->setChecked(false);
        self->ui.act_canvas_show_external->setVisible(true);
        self->fExternalPatchbay = false;
    }
    else
    {
        self->ui.act_canvas_show_internal->setChecked(false);
        self->ui.act_canvas_show_internal->setVisible(false);
        self->ui.act_canvas_show_external->setChecked(true);
        self->ui.act_canvas_show_external->setVisible(false);
        self->fExternalPatchbay = true;
    }

    self->ui.act_canvas_show_internal->blockSignals(false);
    self->ui.act_canvas_show_external->blockSignals(false);

    if (! (host.isControl or host.isPlugin))
    {
        /*
        const bool canSave = (self->fProjectFilename.isNotEmpty() and os.path.exists(self->fProjectFilename)) or not self->fSessionManagerName;
        self->ui.act_file_save->setEnabled(canSave);
        */
        self->ui.act_engine_start->setEnabled(false);
        self->ui.act_engine_stop->setEnabled(true);
    }

    if (! host.isPlugin)
        self->enableTransport(transportMode != ENGINE_TRANSPORT_MODE_DISABLED);

    if (host.isPlugin || self->fSessionManagerName.isEmpty())
    {
        self->ui.act_file_open->setEnabled(true);
        self->ui.act_file_save_as->setEnabled(true);
    }

    self->ui.cb_transport_jack->setChecked(transportMode == ENGINE_TRANSPORT_MODE_JACK);
    self->ui.cb_transport_jack->setEnabled(driverName == "JACK" and processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);

    if (self->ui.cb_transport_link->isEnabled())
        self->ui.cb_transport_link->setChecked(host.transportExtra.contains(":link:"));

    self->updateBufferSize(bufferSize);
    self->updateSampleRate(int(sampleRate));
    self->refreshRuntimeInfo(0.0, 0);
    self->startTimers();

    self->ui.text_logs->appendPlainText("======= Engine started ========");
    self->ui.text_logs->appendPlainText("Carla engine started, details:");
    self->ui.text_logs->appendPlainText(QString("  Driver name:  %1").arg(driverName));
    self->ui.text_logs->appendPlainText(QString("  Sample rate:  %1").arg(int(sampleRate)));
    self->ui.text_logs->appendPlainText(QString("  Process mode: %1").arg(EngineProcessMode2Str((EngineProcessMode)processMode)));
}

void CarlaHostWindow::slot_handleEngineStoppedCallback()
{
    self->ui.text_logs->appendPlainText("======= Engine stopped ========");

    /*
    patchcanvas.clear();
    */
    self->killTimers();

    // just in case
    self->removeAllPlugins();
    self->refreshRuntimeInfo(0.0, 0);

    self->ui.menu_PluginMacros->setEnabled(false);
    self->ui.menu_Canvas->setEnabled(false);
    self->ui.w_transport->setEnabled(false);

    if (! (host.isControl || host.isPlugin))
    {
        self->ui.act_file_save->setEnabled(false);
        self->ui.act_engine_start->setEnabled(true);
        self->ui.act_engine_stop->setEnabled(false);
    }

    if (host.isPlugin || self->fSessionManagerName.isEmpty())
    {
        self->ui.act_file_open->setEnabled(false);
        self->ui.act_file_save_as->setEnabled(false);
    }
}

void CarlaHostWindow::slot_handleTransportModeChangedCallback(int transportMode, QString transportExtra)
{
}

void CarlaHostWindow::slot_handleBufferSizeChangedCallback(int newBufferSize)
{
}

void CarlaHostWindow::slot_handleSampleRateChangedCallback(double newSampleRate)
{
}

void CarlaHostWindow::slot_handleCancelableActionCallback(int pluginId, bool started, QString action)
{
}

void CarlaHostWindow::slot_canlableActionBoxClicked()
{
}

void CarlaHostWindow::slot_handleProjectLoadFinishedCallback()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Plugins (menu actions)

void CarlaHostWindow::slot_favoritePluginAdd()
{
}

void CarlaHostWindow::slot_showPluginActionsMenu()
{
}

void CarlaHostWindow::slot_pluginAdd()
{
}

void CarlaHostWindow::slot_confirmRemoveAll()
{
}

void CarlaHostWindow::slot_jackAppAdd()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Plugins (macros)

void CarlaHostWindow::slot_pluginsEnable()
{
}

void CarlaHostWindow::slot_pluginsDisable()
{
}

void CarlaHostWindow::slot_pluginsVolume100()
{
}

void CarlaHostWindow::slot_pluginsMute()
{
}

void CarlaHostWindow::slot_pluginsWet100()
{
}

void CarlaHostWindow::slot_pluginsBypass()
{
}

void CarlaHostWindow::slot_pluginsCenter()
{
}

void CarlaHostWindow::slot_pluginsCompact()
{
}

void CarlaHostWindow::slot_pluginsExpand()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Plugins (host callbacks)

void CarlaHostWindow::slot_handlePluginAddedCallback(int pluginId, QString pluginName)
{
}

void CarlaHostWindow::slot_handlePluginRemovedCallback(int pluginId)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Canvas (menu actions)

void CarlaHostWindow::slot_canvasShowInternal()
{
}

void CarlaHostWindow::slot_canvasShowExternal()
{
}

void CarlaHostWindow::slot_canvasArrange()
{
}

void CarlaHostWindow::slot_canvasRefresh()
{
}

void CarlaHostWindow::slot_canvasZoomFit()
{
}

void CarlaHostWindow::slot_canvasZoomIn()
{
}

void CarlaHostWindow::slot_canvasZoomOut()
{
}

void CarlaHostWindow::slot_canvasZoomReset()
{
}

void CarlaHostWindow::slot_canvasSaveImage()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Canvas (canvas callbacks)

void CarlaHostWindow::slot_canvasItemMoved(int group_id, int split_mode, QPointF pos)
{
}

void CarlaHostWindow::slot_canvasSelectionChanged()
{
}

void CarlaHostWindow::slot_canvasScaleChanged(double scale)
{
}

void CarlaHostWindow::slot_canvasPluginSelected(QList<void*> pluginList)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Canvas (host callbacks)

void CarlaHostWindow::slot_handlePatchbayClientAddedCallback(int clientId, int clientIcon, int pluginId, QString clientName)
{
}

void CarlaHostWindow::slot_handlePatchbayClientRemovedCallback(int clientId)
{
}

void CarlaHostWindow::slot_handlePatchbayClientRenamedCallback(int clientId, QString newClientName)
{
}

void CarlaHostWindow::slot_handlePatchbayClientDataChangedCallback(int clientId, int clientIcon, int pluginId)
{
}

void CarlaHostWindow::slot_handlePatchbayPortAddedCallback(int clientId, int portId, int portFlags, int portGroupId, QString portName)
{
}

void CarlaHostWindow::slot_handlePatchbayPortRemovedCallback(int groupId, int portId)
{
}

void CarlaHostWindow::slot_handlePatchbayPortChangedCallback(int groupId, int portId, int portFlags, int portGroupId, QString newPortName)
{
}

void CarlaHostWindow::slot_handlePatchbayPortGroupAddedCallback(int groupId, int portId, int portGroupId, QString newPortName)
{
}

void CarlaHostWindow::slot_handlePatchbayPortGroupRemovedCallback(int groupId, int portId)
{
}

void CarlaHostWindow::slot_handlePatchbayPortGroupChangedCallback(int groupId, int portId, int portGroupId, QString newPortName)
{
}

void CarlaHostWindow::slot_handlePatchbayConnectionAddedCallback(int connectionId, int groupOutId, int portOutId, int groupInId, int portInId)
{
}

void CarlaHostWindow::slot_handlePatchbayConnectionRemovedCallback(int connectionId, int portOutId, int portInId)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Settings (helpers)

void CarlaHostWindow::slot_restoreCanvasScrollbarValues()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Settings (menu actions)

void CarlaHostWindow::slot_showSidePanel(const bool yesNo)
{
    self->ui.dockWidget->setEnabled(yesNo);
    self->ui.dockWidget->setVisible(yesNo);
}

void CarlaHostWindow::slot_showToolbar(const bool yesNo)
{
    self->ui.toolBar->setEnabled(yesNo);
    self->ui.toolBar->setVisible(yesNo);
}

void CarlaHostWindow::slot_showCanvasMeters(const bool yesNo)
{
    self->ui.peak_in->setVisible(yesNo);
    self->ui.peak_out->setVisible(yesNo);
    /*
    QTimer::singleShot(0, slot_miniCanvasCheckAll);
    */
}

void CarlaHostWindow::slot_showCanvasKeyboard(const bool yesNo)
{
    /*
    self->ui.scrollArea->setVisible(yesNo);
    QTimer::singleShot(0, slot_miniCanvasCheckAll);
    */
}

void CarlaHostWindow::slot_configureCarla()
{
    /*
    CarlaSettingsW dialog(self->fParentOrSelf, self->host, true, hasGL);
    if not dialog.exec_():
        return
    */

    self->loadSettings(false);

    /*
    patchcanvas.clear()

    setupCanvas();
    slot_miniCanvasCheckAll();
    */

    if (host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK and host.isPlugin)
        pass();
    else if (carla_is_engine_running())
        carla_patchbay_refresh(self->fExternalPatchbay);
}

//---------------------------------------------------------------------------------------------------------------------
// About (menu actions)

void CarlaHostWindow::slot_aboutCarla()
{
}

void CarlaHostWindow::slot_aboutJuce()
{
}

void CarlaHostWindow::slot_aboutQt()
{
}

//---------------------------------------------------------------------------------------------------------------------
// Disk (menu actions)

void CarlaHostWindow::slot_diskFolderChanged(int index)
{
}

void CarlaHostWindow::slot_diskFolderAdd()
{
}

void CarlaHostWindow::slot_diskFolderRemove()
{
}

void CarlaHostWindow::slot_fileTreeDoubleClicked(QModelIndex* modelIndex)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Transport (menu actions)

void CarlaHostWindow::slot_transportPlayPause(const bool toggled)
{
    if (host.isPlugin || ! carla_is_engine_running())
        return;

    if (toggled)
        carla_transport_play();
    else
        carla_transport_pause();

    self->refreshTransport();
}

void CarlaHostWindow::slot_transportStop()
{
    if (host.isPlugin || ! carla_is_engine_running())
        return;

    carla_transport_pause();
    carla_transport_relocate(0);

    self->refreshTransport();
}

void CarlaHostWindow::slot_transportBackwards()
{
    if (host.isPlugin || ! carla_is_engine_running())
        return;

    int64_t newFrame = carla_get_current_transport_frame() - 100000;

    if (newFrame < 0)
        newFrame = 0;

    carla_transport_relocate(newFrame);
}

void CarlaHostWindow::slot_transportBpmChanged(const qreal newValue)
{
    carla_transport_bpm(newValue);
}

void CarlaHostWindow::slot_transportForwards()
{
    if (carla_isZero(self->fSampleRate) || host.isPlugin || ! carla_is_engine_running())
        return;

    const int64_t newFrame = carla_get_current_transport_frame() + int(self->fSampleRate*2.5);
    carla_transport_relocate(newFrame);
}

void CarlaHostWindow::slot_transportJackEnabled(const bool clicked)
{
    if (! carla_is_engine_running())
        return;
    host.transportMode = clicked ? ENGINE_TRANSPORT_MODE_JACK : ENGINE_TRANSPORT_MODE_INTERNAL;
    carla_set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, host.transportMode, host.transportExtra.toUtf8());
}

void CarlaHostWindow::slot_transportLinkEnabled(const bool clicked)
{
    if (! carla_is_engine_running())
        return;
    const char* const extra = clicked ? ":link:" : "";
    host.transportExtra = extra;
    carla_set_engine_option(ENGINE_OPTION_TRANSPORT_MODE, host.transportMode, host.transportExtra.toUtf8());
}

//---------------------------------------------------------------------------------------------------------------------
// Other

void CarlaHostWindow::slot_xrunClear()
{
    carla_clear_engine_xruns();
}

//---------------------------------------------------------------------------------------------------------------------
// Canvas scrollbars

void CarlaHostWindow::slot_horizontalScrollBarChanged(int value)
{
}

void CarlaHostWindow::slot_verticalScrollBarChanged(int value)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Canvas keyboard

void CarlaHostWindow::slot_noteOn(int note)
{
}

void CarlaHostWindow::slot_noteOff(int note)
{
}


//---------------------------------------------------------------------------------------------------------------------
// Canvas keyboard (host callbacks)

void CarlaHostWindow::slot_handleNoteOnCallback(int pluginId, int channel, int note, int velocity)
{
}

void CarlaHostWindow::slot_handleNoteOffCallback(int pluginId, int channel, int note)
{
}

//---------------------------------------------------------------------------------------------------------------------

void CarlaHostWindow::slot_handleUpdateCallback(int pluginId)
{
}

//---------------------------------------------------------------------------------------------------------------------
// MiniCanvas stuff

void CarlaHostWindow::slot_miniCanvasCheckAll()
{
}

void CarlaHostWindow::slot_miniCanvasCheckSize()
{
}

void CarlaHostWindow::slot_miniCanvasMoved(qreal xp, qreal yp)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Misc

void CarlaHostWindow::slot_tabChanged(int index)
{
}

void CarlaHostWindow::slot_handleReloadAllCallback(int pluginId)
{
}

//---------------------------------------------------------------------------------------------------------------------

void CarlaHostWindow::slot_handleNSMCallback(int opcode, int valueInt, QString valueStr)
{
}

//---------------------------------------------------------------------------------------------------------------------

void CarlaHostWindow::slot_handleDebugCallback(int pluginId, int value1, int value2, int value3, float valuef, QString valueStr)
{
}

void CarlaHostWindow::slot_handleInfoCallback(QString info)
{
}

void CarlaHostWindow::slot_handleErrorCallback(QString error)
{
}

void CarlaHostWindow::slot_handleQuitCallback()
{
}

void CarlaHostWindow::slot_handleInlineDisplayRedrawCallback(int pluginId)
{
}

//---------------------------------------------------------------------------------------------------------------------

void CarlaHostWindow::slot_handleSIGUSR1()
{
}

void CarlaHostWindow::slot_handleSIGTERM()
{
}

//---------------------------------------------------------------------------------------------------------------------
// show/hide event

void CarlaHostWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
}

void CarlaHostWindow::hideEvent(QHideEvent* event)
{
    QMainWindow::hideEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// resize event

void CarlaHostWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// timer event

void CarlaHostWindow::timerEvent(QTimerEvent* const event)
{
    if (event->timerId() == self->fIdleTimerFast)
        self->idleFast();

    else if (event->timerId() == self->fIdleTimerSlow)
        self->idleSlow();

    QMainWindow::timerEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// color/style change event

void CarlaHostWindow::changeEvent(QEvent* const event)
{
    switch (event->type())
    {
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
        self->updateStyle();
        break;
    default:
        break;
    }

    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// close event

void CarlaHostWindow::closeEvent(QCloseEvent* const event)
{
    if (self->shouldIgnoreClose())
    {
        event->ignore();
        return;
    }

#ifdef CARLA_OS_MAC
    if (self->fMacClosingHelper && ! (host.isControl || host.isPlugin))
    {
        self->fCustomStopAction = CUSTOM_ACTION_APP_CLOSE;
        self->fMacClosingHelper = false;
        event->ignore();

        for i in reversed(range(self->fPluginCount)):
            carla_show_custom_ui(i, false);

        QTimer::singleShot(100, SIGNAL(close()));
        return;
    }
#endif

    self->killTimers();
    self->saveSettings();

    if (carla_is_engine_running() && ! (host.isControl or host.isPlugin))
    {
        if (! slot_engineStop(true))
        {
            self->fCustomStopAction = CUSTOM_ACTION_APP_CLOSE;
            event->ignore();
            return;
        }
    }

    QMainWindow::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
// Engine callback

void _engineCallback(void* const ptr, EngineCallbackOpcode action, uint pluginId, int value1, int value2, int value3, float valuef, const char* const valueStr)
{
    /*
    carla_stdout("_engineCallback(%p, %i:%s, %u, %i, %i, %i, %f, %s)",
                 ptr, action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valuef, valueStr);
    */

    CarlaHost* const host = (CarlaHost*)(ptr);
    CARLA_SAFE_ASSERT_RETURN(host != nullptr,);

    switch (action)
    {
    case ENGINE_CALLBACK_ENGINE_STARTED:
        host->processMode   = static_cast<EngineProcessMode>(value1);
        host->transportMode = static_cast<EngineTransportMode>(value2);
        break;
    case ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
        host->processMode   = static_cast<EngineProcessMode>(value1);
        break;
    case ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
        host->transportMode  = static_cast<EngineTransportMode>(value1);
        host->transportExtra = valueStr;
        break;
    default:
        break;
    }

    // TODO
    switch (action)
    {
    case ENGINE_CALLBACK_ENGINE_STARTED:
        emit host->EngineStartedCallback(pluginId, value1, value2, value3, valuef, valueStr);
        break;
    case ENGINE_CALLBACK_ENGINE_STOPPED:
        emit host->EngineStoppedCallback();
        break;

    // FIXME
    default:
        break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// File callback

static const char* _fileCallback(void*, const FileCallbackOpcode action, const bool isDir, const char* const title, const char* const filter)
{
    QString ret;
    const QFileDialog::Options options = QFileDialog::Options(isDir ? QFileDialog::ShowDirsOnly : 0x0);

    switch (action)
    {
    case FILE_CALLBACK_DEBUG:
        break;
    case FILE_CALLBACK_OPEN:
        ret = QFileDialog::getOpenFileName(gCarla.gui, title, "", filter, nullptr, options);
        break;
    case FILE_CALLBACK_SAVE:
        ret = QFileDialog::getSaveFileName(gCarla.gui, title, "", filter, nullptr, options);
        break;
    }

    if (ret.isEmpty())
        return nullptr;

    static const char* fileRet = nullptr;

    if (fileRet != nullptr)
        delete[] fileRet;

    fileRet = carla_strdup_safe(ret.toUtf8());
    return fileRet;
}

//---------------------------------------------------------------------------------------------------------------------
// Init host

CarlaHost& initHost(const QString /*initName*/, const bool isControl, const bool isPlugin, const bool failError)
{
    CarlaString pathBinaries, pathResources;
    // = getPaths(libPrefix)

    //-----------------------------------------------------------------------------------------------------------------
    // Print info

    // ----------------------------------------------------------------------------------------------------------------
    // Init host

    if (failError)
    {
//         # no try
//         host = HostClass() if HostClass is not None else CarlaHostQtDLL(libname, loadGlobal)
    }
    else
    {
//         try:
//             host = HostClass() if HostClass is not None else CarlaHostQtDLL(libname, loadGlobal)
//         except:
//             host = CarlaHostQtNull()
    }

    static CarlaHost host;
    host.isControl = isControl;
    host.isPlugin  = isPlugin;

    carla_set_engine_callback(_engineCallback, &host);
    carla_set_file_callback(_fileCallback, nullptr);

    // If it's a plugin the paths are already set
    if (! isPlugin)
    {
        host.pathBinaries  = pathBinaries;
        host.pathResources = pathResources;
        carla_set_engine_option(ENGINE_OPTION_PATH_BINARIES,  0, pathBinaries);
        carla_set_engine_option(ENGINE_OPTION_PATH_RESOURCES, 0, pathResources);

        /*
        if (! isControl)
            host.nsmOK = carla_nsm_init(getpid(), initName);
        */
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Done

    return host;
}

//---------------------------------------------------------------------------------------------------------------------
// Load host settings

void loadHostSettings(CarlaHost& /*host*/)
{
}

//---------------------------------------------------------------------------------------------------------------------
