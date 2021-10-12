/*
 * Carla plugin host
 * Copyright (C) 2011-2020 Filipe Coelho <falktx@falktx.com>
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


#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

//---------------------------------------------------------------------------------------------------------------------

#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <QtGui/QPainter>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFileSystemModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>

//---------------------------------------------------------------------------------------------------------------------

#include "ui_carla_host.hpp"

//---------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_database.hpp"
#include "carla_settings.hpp"
#include "carla_skin.hpp"

#include "CarlaHost.h"
#include "CarlaUtils.h"

#include "CarlaBackendUtils.hpp"
#include "CarlaMathUtils.hpp"
#include "CarlaString.hpp"

// FIXME put in right place
/*
static QString fixLogText(QString text)
{
    //v , Qt::CaseSensitive
    return text.replace("\x1b[30;1m", "").replace("\x1b[31m", "").replace("\x1b[0m", "");
}
*/

//---------------------------------------------------------------------------------------------------------------------
// Session Management support

static const char* const CARLA_CLIENT_NAME = getenv("CARLA_CLIENT_NAME");
static const char* const LADISH_APP_NAME   = getenv("LADISH_APP_NAME");
static const char* const NSM_URL           = getenv("NSM_URL");

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
      maxParameters(0),
      resetXruns(false),
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
// Carla Host Window

enum CustomActions {
    CUSTOM_ACTION_NONE,
    CUSTOM_ACTION_APP_CLOSE,
    CUSTOM_ACTION_PROJECT_LOAD
};

struct CachedSavedSettings {
    int _CARLA_KEY_MAIN_REFRESH_INTERVAL = 0;
    bool _CARLA_KEY_MAIN_CONFIRM_EXIT = false;
    bool _CARLA_KEY_CANVAS_FANCY_EYE_CANDY = false;
};

//---------------------------------------------------------------------------------------------------------------------

struct CarlaHostWindow::PrivateData {
    Ui::CarlaHostW ui;
    CarlaHost& host;
    CarlaHostWindow* const hostWindow;

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff

    QWidget* const fParentOrSelf;

    int fIdleTimerNull; // to keep application signals alive
    int fIdleTimerFast;
    int fIdleTimerSlow;

    bool fLadspaRdfNeedsUpdate;
    QList<void*> fLadspaRdfList;

    int fPluginCount;
    QList<QWidget*> fPluginList;

    void* fPluginDatabaseDialog;
    QStringList fFavoritePlugins;

    QCarlaString fProjectFilename;
    bool fIsProjectLoading;
    bool fCurrentlyRemovingAllPlugins;

    double fLastTransportBPM;
    uint64_t fLastTransportFrame;
    bool fLastTransportState;
    uint fBufferSize;
    double fSampleRate;
    QCarlaString fOscAddressTCP;
    QCarlaString fOscAddressUDP;
    QCarlaString fOscReportedHost;

#ifdef CARLA_OS_MAC
    bool fMacClosingHelper;
#endif

    // CancelableActionCallback Box
    void* fCancelableActionBox;

    // run a custom action after engine is properly closed
    CustomActions fCustomStopAction;

    // first attempt of auto-start engine doesn't show an error
    bool fFirstEngineInit;

    // to be filled with key-value pairs of current settings
    CachedSavedSettings fSavedSettings;

    QCarlaString fClientName;
    QCarlaString fSessionManagerName;

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff (patchbay)

    QImage fExportImage;

    bool fPeaksCleared;

    bool fExternalPatchbay;
    QList<uint> fSelectedPlugins;

    int fCanvasWidth;
    int fCanvasHeight;
    int fMiniCanvasUpdateTimeout;
    const bool fWithCanvas;

    //-----------------------------------------------------------------------------------------------------------------
    // GUI stuff (disk)

    QFileSystemModel fDirModel;

    //-----------------------------------------------------------------------------------------------------------------

    // CarlaHostWindow* hostWindow,
    PrivateData(CarlaHostWindow* const hw, CarlaHost& h, const bool withCanvas)
        : ui(),
          host(h),
          hostWindow(hw),
          // Internal stuff
          fParentOrSelf(hw->parentWidget() != nullptr ? hw->parentWidget() : hw),
          fIdleTimerNull(0),
          fIdleTimerFast(0),
          fIdleTimerSlow(0),
          fLadspaRdfNeedsUpdate(true),
          fLadspaRdfList(),
          fPluginCount(0),
          fPluginList(),
          fPluginDatabaseDialog(nullptr),
          fFavoritePlugins(),
          fProjectFilename(),
          fIsProjectLoading(false),
          fCurrentlyRemovingAllPlugins(false),
          fLastTransportBPM(0.0),
          fLastTransportFrame(0),
          fLastTransportState(false),
          fBufferSize(0),
          fSampleRate(0.0),
          fOscAddressTCP(),
          fOscAddressUDP(),
          fOscReportedHost(),
#ifdef CARLA_OS_MAC
          fMacClosingHelper(true),
#endif
          fCancelableActionBox(nullptr),
          fCustomStopAction(CUSTOM_ACTION_NONE),
          fFirstEngineInit(true),
          fSavedSettings(),
          fClientName(),
          fSessionManagerName(),
          // Internal stuff (patchbay)
          fExportImage(),
          fPeaksCleared(true),
          fExternalPatchbay(false),
          fSelectedPlugins(),
          fCanvasWidth(0),
          fCanvasHeight(0),
          fMiniCanvasUpdateTimeout(0),
          fWithCanvas(withCanvas),
          // disk
          fDirModel(hostWindow)
    {
        ui.setupUi(hostWindow);

        //-------------------------------------------------------------------------------------------------------------
        // Internal stuff

        if (host.isControl)
        {
            fClientName         = "Carla-Control";
            fSessionManagerName = "Control";
        }
        else if (host.isPlugin)
        {
            fClientName         = "Carla-Plugin";
            fSessionManagerName = "Plugin";
        }
        else if (LADISH_APP_NAME != nullptr)
        {
            fClientName         = LADISH_APP_NAME;
            fSessionManagerName = "LADISH";
        }
        else if (NSM_URL != nullptr && host.nsmOK)
        {
            fClientName         = "Carla.tmp";
            fSessionManagerName = "Non Session Manager TMP";
        }
        else
        {
            fClientName         = CARLA_CLIENT_NAME != nullptr ? CARLA_CLIENT_NAME : "Carla";
            fSessionManagerName = "";
        }

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (engine stopped)

        if (host.isPlugin || host.isControl)
        {
            ui.act_file_save->setVisible(false);
            ui.act_engine_start->setEnabled(false);
            ui.act_engine_start->setVisible(false);
            ui.act_engine_stop->setEnabled(false);
            ui.act_engine_stop->setVisible(false);
            ui.menu_Engine->setEnabled(false);
            ui.menu_Engine->setVisible(false);
            if (QAction* const action = ui.menu_Engine->menuAction())
                action->setVisible(false);
            ui.tabWidget->removeTab(2);

            if (host.isControl)
            {
                ui.act_file_new->setVisible(false);
                ui.act_file_open->setVisible(false);
                ui.act_file_save_as->setVisible(false);
                ui.tabUtils->removeTab(0);
            }
            else
            {
                ui.act_file_save_as->setText(tr("Export as..."));

                if (! withCanvas)
                    if (QTabBar* const tabBar = ui.tabWidget->tabBar())
                        tabBar->hide();
            }
        }
        else
        {
            ui.act_engine_start->setEnabled(true);
#ifdef CARLA_OS_WIN
            ui.tabWidget->removeTab(2);
#endif
        }

        if (host.isControl)
        {
            ui.act_file_refresh->setEnabled(false);
        }
        else
        {
            ui.act_file_connect->setEnabled(false);
            ui.act_file_connect->setVisible(false);
            ui.act_file_refresh->setEnabled(false);
            ui.act_file_refresh->setVisible(false);
        }

        if (fSessionManagerName.isNotEmpty() && ! host.isPlugin)
            ui.act_file_new->setEnabled(false);

        ui.act_file_open->setEnabled(false);
        ui.act_file_save->setEnabled(false);
        ui.act_file_save_as->setEnabled(false);
        ui.act_engine_stop->setEnabled(false);
        ui.act_plugin_remove_all->setEnabled(false);

        ui.act_canvas_show_internal->setChecked(false);
        ui.act_canvas_show_internal->setVisible(false);
        ui.act_canvas_show_external->setChecked(false);
        ui.act_canvas_show_external->setVisible(false);

        ui.menu_PluginMacros->setEnabled(false);
        ui.menu_Canvas->setEnabled(false);

        QWidget* const dockWidgetTitleBar = new QWidget(hostWindow);
        ui.dockWidget->setTitleBarWidget(dockWidgetTitleBar);

        if (! withCanvas)
        {
            ui.act_canvas_show_internal->setVisible(false);
            ui.act_canvas_show_external->setVisible(false);
            ui.act_canvas_arrange->setVisible(false);
            ui.act_canvas_refresh->setVisible(false);
            ui.act_canvas_save_image->setVisible(false);
            ui.act_canvas_zoom_100->setVisible(false);
            ui.act_canvas_zoom_fit->setVisible(false);
            ui.act_canvas_zoom_in->setVisible(false);
            ui.act_canvas_zoom_out->setVisible(false);
            ui.act_settings_show_meters->setVisible(false);
            ui.act_settings_show_keyboard->setVisible(false);
            ui.menu_Canvas_Zoom->setEnabled(false);
            ui.menu_Canvas_Zoom->setVisible(false);
            if (QAction* const action = ui.menu_Canvas_Zoom->menuAction())
                action->setVisible(false);
            ui.menu_Canvas->setEnabled(false);
            ui.menu_Canvas->setVisible(false);
            if (QAction* const action = ui.menu_Canvas->menuAction())
                action->setVisible(false);
            ui.tw_miniCanvas->hide();
            ui.tabWidget->removeTab(1);
#ifdef CARLA_OS_WIN
            ui.tabWidget->tabBar().hide()
#endif
        }

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (disk)

        const QString home(QDir::homePath());
        fDirModel.setRootPath(home);

        if (const char* const* const exts = carla_get_supported_file_extensions())
        {
            QStringList filters;
            const QString prefix("*.");

            for (uint i=0; exts[i] != nullptr; ++i)
                filters.append(prefix + exts[i]);

            fDirModel.setNameFilters(filters);
        }

        ui.fileTreeView->setModel(&fDirModel);
        ui.fileTreeView->setRootIndex(fDirModel.index(home));
        ui.fileTreeView->setColumnHidden(1, true);
        ui.fileTreeView->setColumnHidden(2, true);
        ui.fileTreeView->setColumnHidden(3, true);
        ui.fileTreeView->setHeaderHidden(true);

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (transport)

        const QFontMetrics fontMetrics(ui.l_transport_bbt->fontMetrics());
        int minValueWidth = fontMetricsHorizontalAdvance(fontMetrics, "000|00|0000");
        int minLabelWidth = fontMetricsHorizontalAdvance(fontMetrics, ui.label_transport_frame->text());

        int labelTimeWidth = fontMetricsHorizontalAdvance(fontMetrics, ui.label_transport_time->text());
        int labelBBTWidth  = fontMetricsHorizontalAdvance(fontMetrics, ui.label_transport_bbt->text());

        if (minLabelWidth < labelTimeWidth)
            minLabelWidth = labelTimeWidth;
        if (minLabelWidth < labelBBTWidth)
            minLabelWidth = labelBBTWidth;

        ui.label_transport_frame->setMinimumWidth(minLabelWidth + 3);
        ui.label_transport_time->setMinimumWidth(minLabelWidth + 3);
        ui.label_transport_bbt->setMinimumWidth(minLabelWidth + 3);

        ui.l_transport_bbt->setMinimumWidth(minValueWidth + 3);
        ui.l_transport_frame->setMinimumWidth(minValueWidth + 3);
        ui.l_transport_time->setMinimumWidth(minValueWidth + 3);

        if (host.isPlugin)
        {
            ui.b_transport_play->setEnabled(false);
            ui.b_transport_stop->setEnabled(false);
            ui.b_transport_backwards->setEnabled(false);
            ui.b_transport_forwards->setEnabled(false);
            ui.group_transport_controls->setEnabled(false);
            ui.group_transport_controls->setVisible(false);
            ui.cb_transport_link->setEnabled(false);
            ui.cb_transport_link->setVisible(false);
            ui.cb_transport_jack->setEnabled(false);
            ui.cb_transport_jack->setVisible(false);
            ui.dsb_transport_bpm->setEnabled(false);
            ui.dsb_transport_bpm->setReadOnly(true);
        }

        ui.w_transport->setEnabled(false);

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (rack)

        // ui.listWidget->setHostAndParent(host, self);

        if (QScrollBar* const sb = ui.listWidget->verticalScrollBar())
        {
            ui.rackScrollBar->setMinimum(sb->minimum());
            ui.rackScrollBar->setMaximum(sb->maximum());
            ui.rackScrollBar->setValue(sb->value());

            /*
            sb->rangeChanged.connect(ui.rackScrollBar.setRange);
            sb->valueChanged.connect(ui.rackScrollBar.setValue);
            ui.rackScrollBar->rangeChanged.connect(sb.setRange);
            ui.rackScrollBar->valueChanged.connect(sb.setValue);
            */
        }

        updateStyle();

        ui.rack->setStyleSheet("      \
        CarlaRackList#CarlaRackList { \
            background-color: black;  \
        }                             \
        ");

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (patchbay)

        /*
        ui.peak_in->setChannelCount(2);
        ui.peak_in->setMeterColor(DigitalPeakMeter.COLOR_BLUE);
        ui.peak_in->setMeterOrientation(DigitalPeakMeter.VERTICAL);
        */
        ui.peak_in->setFixedWidth(25);

        /*
        ui.peak_out->setChannelCount(2);
        ui.peak_out->setMeterColor(DigitalPeakMeter.COLOR_GREEN);
        ui.peak_out->setMeterOrientation(DigitalPeakMeter.VERTICAL);
        */
        ui.peak_out->setFixedWidth(25);

        /*
        ui.scrollArea = PixmapKeyboardHArea(ui.patchbay);
        ui.keyboard   = ui.scrollArea.keyboard;
        ui.patchbay.layout().addWidget(ui.scrollArea, 1, 0, 1, 0);

        ui.scrollArea->setEnabled(false);

        ui.miniCanvasPreview->setRealParent(self);
        */
        if (QTabBar* const tabBar = ui.tw_miniCanvas->tabBar())
            tabBar->hide();

        //-------------------------------------------------------------------------------------------------------------
        // Set up GUI (special stuff for Mac OS)

#ifdef CARLA_OS_MAC
        ui.act_file_quit->setMenuRole(QAction::QuitRole);
        ui.act_settings_configure->setMenuRole(QAction::PreferencesRole);
        ui.act_help_about->setMenuRole(QAction::AboutRole);
        ui.act_help_about_qt->setMenuRole(QAction::AboutQtRole);
        ui.menu_Settings->setTitle("Panels");
        if (QAction* const action = ui.menu_Help.menuAction())
            action->setVisible(false);
#endif

        //-------------------------------------------------------------------------------------------------------------
        // Load Settings

        loadSettings(true);

        //-------------------------------------------------------------------------------------------------------------
        // Final setup

        ui.text_logs->clear();
        setProperWindowTitle();

        // Disable non-supported features
        const char* const* const features = carla_get_supported_features();

        if (! stringArrayContainsString(features, "link"))
        {
            ui.cb_transport_link->setEnabled(false);
            ui.cb_transport_link->setVisible(false);
        }

        if (! stringArrayContainsString(features, "juce"))
        {
            ui.act_help_about_juce->setEnabled(false);
            ui.act_help_about_juce->setVisible(false);
        }

        // Plugin needs to have timers always running so it receives messages
        if (host.isPlugin || host.isRemote)
        {
            startTimers();
        }
        // Load initial project file if set
        else
        {
            /*
            projectFile = getInitialProjectFile(QApplication.instance());

            if (projectFile)
                loadProjectLater(projectFile);
            */
        }

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

        if (! carla_load_project(host.handle, fProjectFilename.toUtf8()))
        {
            fIsProjectLoading = false;
            projectLoadingFinished();

            CustomMessageBox(hostWindow,
                             QMessageBox::Critical,
                             tr("Error"),
                             tr("Failed to load project"),
                             carla_get_last_error(host.handle),
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

        if (carla_is_engine_running(host.handle))
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

            if (! carla_remove_all_plugins(host.handle))
            {
                ui.text_logs->appendPlainText("Failed to remove all plugins, error was:");
                ui.text_logs->appendPlainText(carla_get_last_error(host.handle));
            }

            if (! carla_engine_close(host.handle))
            {
                ui.text_logs->appendPlainText("Failed to stop engine, error was:");
                ui.text_logs->appendPlainText(carla_get_last_error(host.handle));
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
            carla_nsm_ready(host.handle, NSM_CALLBACK_OPEN);
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

            // if settings.contains("SplitterState"):
                //ui.splitter.restoreState(settings.value("SplitterState", b""))
            //else:
                //ui.splitter.setSizes([210, 99999])

            const bool toolbarVisible = settings.valueBool("ShowToolbar", true);
            ui.act_settings_show_toolbar->setChecked(toolbarVisible);
            showToolbar(toolbarVisible);

            const bool sidePanelVisible = settings.valueBool("ShowSidePanel", true) && ! host.isControl;
            ui.act_settings_show_side_panel->setChecked(sidePanelVisible);
            showSidePanel(sidePanelVisible);

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

        fSavedSettings._CARLA_KEY_MAIN_REFRESH_INTERVAL = settings.valueIntPositive(CARLA_KEY_MAIN_REFRESH_INTERVAL,
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

        setEngineSettings(host);
        restartTimersIfNeeded();
    }

    void enableTransport(const bool enabled)
    {
        ui.group_transport_controls->setEnabled(enabled);
        ui.group_transport_settings->setEnabled(enabled);
    }

    void showSidePanel(const bool yesNo)
    {
        ui.dockWidget->setEnabled(yesNo);
        ui.dockWidget->setVisible(yesNo);
    }

    void showToolbar(const bool yesNo)
    {
        ui.toolBar->setEnabled(yesNo);
        ui.toolBar->setVisible(yesNo);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Transport

    void refreshTransport(const bool forced = false)
    {
        if (! ui.l_transport_time->isVisible())
            return;
        if (carla_isZero(fSampleRate) or ! carla_is_engine_running(host.handle))
            return;

        const CarlaTransportInfo* const timeInfo = carla_get_transport_info(host.handle);
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

            const uint64_t time = frame / static_cast<uint32_t>(fSampleRate);
            const uint64_t secs =  time % 60;
            const uint64_t mins = (time / 60) % 60;
            const uint64_t hrs  = (time / 3600) % 60;
            ui.l_transport_time->setText(QString("%1:%2:%3").arg(hrs, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));

            const uint64_t frame1 =  frame % 1000;
            const uint64_t frame2 = (frame / 1000) % 1000;
            const uint64_t frame3 = (frame / 1000000) % 1000;
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

    void refreshRuntimeInfo(const float load, const uint xruns)
    {
        const QString txt1(xruns == 0 ? QString("%1").arg(xruns) : QString("--"));
        const QString txt2(xruns == 1 ? "" : "s");
        ui.b_xruns->setText(QString("%1 Xrun%2").arg(txt1).arg(txt2));
        ui.pb_dsp_load->setValue(int(load));
    }

    void getAndRefreshRuntimeInfo()
    {
        if (! ui.pb_dsp_load->isVisible())
            return;
        if (! carla_is_engine_running(host.handle))
            return;
        const CarlaRuntimeEngineInfo* const info = carla_get_runtime_engine_info(host.handle);
        refreshRuntimeInfo(info->load, info->xruns);
    }

    void idleFast()
    {
        carla_engine_idle(host.handle);
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
            pad_color = bg_color.lighter(static_cast<int>(100*min_value/bg_value*value_fix));
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

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

//---------------------------------------------------------------------------------------------------------------------

CarlaHostWindow::CarlaHostWindow(CarlaHost& host, const bool withCanvas, QWidget* const parent)
    : QMainWindow(parent),
      self(new PrivateData(this, host, withCanvas))
{
    gCarla.gui = this;

    self->fIdleTimerNull = startTimer(1000);

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

    connect(self->ui.act_file_new, SIGNAL(triggered()), SLOT(slot_fileNew()));
    connect(self->ui.act_file_open, SIGNAL(triggered()), SLOT(slot_fileOpen()));
    connect(self->ui.act_file_save, SIGNAL(triggered()), SLOT(slot_fileSave()));
    connect(self->ui.act_file_save_as, SIGNAL(triggered()), SLOT(slot_fileSaveAs()));

    connect(self->ui.act_engine_start, SIGNAL(triggered()), SLOT(slot_engineStart()));
    connect(self->ui.act_engine_stop, SIGNAL(triggered()), SLOT(slot_engineStop()));
    connect(self->ui.act_engine_panic, SIGNAL(triggered()), SLOT(slot_pluginsDisable()));
    connect(self->ui.act_engine_config, SIGNAL(triggered()), SLOT(slot_engineConfig()));

    connect(self->ui.act_plugin_add, SIGNAL(triggered()), SLOT(slot_pluginAdd()));
    connect(self->ui.act_plugin_add_jack, SIGNAL(triggered()), SLOT(slot_jackAppAdd()));
    connect(self->ui.act_plugin_remove_all, SIGNAL(triggered()), SLOT(slot_confirmRemoveAll()));

    connect(self->ui.act_plugins_enable, SIGNAL(triggered()), SLOT(slot_pluginsEnable()));
    connect(self->ui.act_plugins_disable, SIGNAL(triggered()), SLOT(slot_pluginsDisable()));
    connect(self->ui.act_plugins_volume100, SIGNAL(triggered()), SLOT(slot_pluginsVolume100()));
    connect(self->ui.act_plugins_mute, SIGNAL(triggered()), SLOT(slot_pluginsMute()));
    connect(self->ui.act_plugins_wet100, SIGNAL(triggered()), SLOT(slot_pluginsWet100()));
    connect(self->ui.act_plugins_bypass, SIGNAL(triggered()), SLOT(slot_pluginsBypass()));
    connect(self->ui.act_plugins_center, SIGNAL(triggered()), SLOT(slot_pluginsCenter()));
    connect(self->ui.act_plugins_compact, SIGNAL(triggered()), SLOT(slot_pluginsCompact()));
    connect(self->ui.act_plugins_expand, SIGNAL(triggered()), SLOT(slot_pluginsExpand()));

    connect(self->ui.act_settings_show_toolbar, SIGNAL(toggled(bool)), SLOT(slot_showToolbar(bool)));
    connect(self->ui.act_settings_show_meters, SIGNAL(toggled(bool)), SLOT(slot_showCanvasMeters(bool)));
    connect(self->ui.act_settings_show_keyboard, SIGNAL(toggled(bool)), SLOT(slot_showCanvasKeyboard(bool)));
    connect(self->ui.act_settings_show_side_panel, SIGNAL(toggled(bool)), SLOT(slot_showSidePanel(bool)));
    connect(self->ui.act_settings_configure, SIGNAL(triggered()), SLOT(slot_configureCarla()));

    connect(self->ui.act_help_about, SIGNAL(triggered()), SLOT(slot_aboutCarla()));
    connect(self->ui.act_help_about_juce, SIGNAL(triggered()), SLOT(slot_aboutJuce()));
    connect(self->ui.act_help_about_qt, SIGNAL(triggered()), SLOT(slot_aboutQt()));

    connect(self->ui.cb_disk, SIGNAL(currentIndexChanged(int)), SLOT(slot_diskFolderChanged(int)));
    connect(self->ui.b_disk_add, SIGNAL(clicked()), SLOT(slot_diskFolderAdd()));
    connect(self->ui.b_disk_remove, SIGNAL(clicked()), SLOT(slot_diskFolderRemove()));
    connect(self->ui.fileTreeView, SIGNAL(doubleClicked(QModelIndex*)), SLOT(slot_fileTreeDoubleClicked(QModelIndex*)));

    connect(self->ui.b_transport_play, SIGNAL(clicked(bool)), SLOT(slot_transportPlayPause(bool)));
    connect(self->ui.b_transport_stop, SIGNAL(clicked()), SLOT(slot_transportStop()));
    connect(self->ui.b_transport_backwards, SIGNAL(clicked()), SLOT(slot_transportBackwards()));
    connect(self->ui.b_transport_forwards, SIGNAL(clicked()), SLOT(slot_transportForwards()));
    connect(self->ui.dsb_transport_bpm, SIGNAL(valueChanged(qreal)), SLOT(slot_transportBpmChanged(qreal)));
    connect(self->ui.cb_transport_jack, SIGNAL(clicked(bool)), SLOT(slot_transportJackEnabled(bool)));
    connect(self->ui.cb_transport_link, SIGNAL(clicked(bool)), SLOT(slot_transportLinkEnabled(bool)));

    connect(self->ui.b_xruns, SIGNAL(clicked()), SLOT(slot_xrunClear()));

    connect(self->ui.listWidget, SIGNAL(customContextMenuRequested()), SLOT(slot_showPluginActionsMenu()));

    /*
    connect(self->ui.keyboard, SIGNAL(noteOn(int)), SLOT(slot_noteOn(int)));
    connect(self->ui.keyboard, SIGNAL(noteOff(int)), SLOT(slot_noteOff(int)));
    */

    connect(self->ui.tabWidget, SIGNAL(currentChanged(int)), SLOT(slot_tabChanged(int)));

    if (withCanvas)
    {
        connect(self->ui.act_canvas_show_internal, SIGNAL(triggered()), SLOT(slot_canvasShowInternal()));
        connect(self->ui.act_canvas_show_external, SIGNAL(triggered()), SLOT(slot_canvasShowExternal()));
        connect(self->ui.act_canvas_arrange, SIGNAL(triggered()), SLOT(slot_canvasArrange()));
        connect(self->ui.act_canvas_refresh, SIGNAL(triggered()), SLOT(slot_canvasRefresh()));
        connect(self->ui.act_canvas_zoom_fit, SIGNAL(triggered()), SLOT(slot_canvasZoomFit()));
        connect(self->ui.act_canvas_zoom_in, SIGNAL(triggered()), SLOT(slot_canvasZoomIn()));
        connect(self->ui.act_canvas_zoom_out, SIGNAL(triggered()), SLOT(slot_canvasZoomOut()));
        connect(self->ui.act_canvas_zoom_100, SIGNAL(triggered()), SLOT(slot_canvasZoomReset()));
        connect(self->ui.act_canvas_save_image, SIGNAL(triggered()), SLOT(slot_canvasSaveImage()));
        self->ui.act_canvas_arrange->setEnabled(false); // TODO, later
        connect(self->ui.graphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(slot_horizontalScrollBarChanged(int)));
        connect(self->ui.graphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(slot_verticalScrollBarChanged(int)));
        /*
        connect(self->ui.miniCanvasPreview, SIGNAL(miniCanvasMoved(qreal, qreal)), SLOT(slot_miniCanvasMoved(qreal, qreal)));
        connect(self.scene, SIGNAL(scaleChanged()), SLOT(slot_canvasScaleChanged()));
        connect(self.scene, SIGNAL(sceneGroupMoved()), SLOT(slot_canvasItemMoved()));
        connect(self.scene, SIGNAL(pluginSelected()), SLOT(slot_canvasPluginSelected()));
        connect(self.scene, SIGNAL(selectionChanged()), SLOT(slot_canvasSelectionChanged()));
        */
    }

    connect(&host, SIGNAL(SIGUSR1()), SLOT(slot_handleSIGUSR1()));
    connect(&host, SIGNAL(SIGTERM()), SLOT(slot_handleSIGTERM()));

    connect(&host, SIGNAL(EngineStartedCallback(uint, int, int, uint, float, QString)), SLOT(slot_handleEngineStartedCallback(uint, int, int, uint, float, QString)));
    connect(&host, SIGNAL(EngineStoppedCallback()), SLOT(slot_handleEngineStoppedCallback()));
    connect(&host, SIGNAL(TransportModeChangedCallback()), SLOT(slot_handleTransportModeChangedCallback()));
    connect(&host, SIGNAL(BufferSizeChangedCallback()), SLOT(slot_handleBufferSizeChangedCallback()));
    connect(&host, SIGNAL(SampleRateChangedCallback()), SLOT(slot_handleSampleRateChangedCallback()));
    connect(&host, SIGNAL(CancelableActionCallback()), SLOT(slot_handleCancelableActionCallback()));
    connect(&host, SIGNAL(ProjectLoadFinishedCallback()), SLOT(slot_handleProjectLoadFinishedCallback()));

    connect(&host, SIGNAL(PluginAddedCallback()), SLOT(slot_handlePluginAddedCallback()));
    connect(&host, SIGNAL(PluginRemovedCallback()), SLOT(slot_handlePluginRemovedCallback()));
    connect(&host, SIGNAL(ReloadAllCallback()), SLOT(slot_handleReloadAllCallback()));

    connect(&host, SIGNAL(NoteOnCallback()), SLOT(slot_handleNoteOnCallback()));
    connect(&host, SIGNAL(NoteOffCallback()), SLOT(slot_handleNoteOffCallback()));

    connect(&host, SIGNAL(UpdateCallback()), SLOT(slot_handleUpdateCallback()));

    connect(&host, SIGNAL(PatchbayClientAddedCallback()), SLOT(slot_handlePatchbayClientAddedCallback()));
    connect(&host, SIGNAL(PatchbayClientRemovedCallback()), SLOT(slot_handlePatchbayClientRemovedCallback()));
    connect(&host, SIGNAL(PatchbayClientRenamedCallback()), SLOT(slot_handlePatchbayClientRenamedCallback()));
    connect(&host, SIGNAL(PatchbayClientDataChangedCallback()), SLOT(slot_handlePatchbayClientDataChangedCallback()));
    connect(&host, SIGNAL(PatchbayPortAddedCallback()), SLOT(slot_handlePatchbayPortAddedCallback()));
    connect(&host, SIGNAL(PatchbayPortRemovedCallback()), SLOT(slot_handlePatchbayPortRemovedCallback()));
    connect(&host, SIGNAL(PatchbayPortChangedCallback()), SLOT(slot_handlePatchbayPortChangedCallback()));
    connect(&host, SIGNAL(PatchbayPortGroupAddedCallback()), SLOT(slot_handlePatchbayPortGroupAddedCallback()));
    connect(&host, SIGNAL(PatchbayPortGroupRemovedCallback()), SLOT(slot_handlePatchbayPortGroupRemovedCallback()));
    connect(&host, SIGNAL(PatchbayPortGroupChangedCallback()), SLOT(slot_handlePatchbayPortGroupChangedCallback()));

    connect(&host, SIGNAL(PatchbayConnectionAddedCallback()), SLOT(slot_handlePatchbayConnectionAddedCallback()));
    connect(&host, SIGNAL(PatchbayConnectionRemovedCallback()), SLOT(slot_handlePatchbayConnectionRemovedCallback()));

    connect(&host, SIGNAL(NSMCallback()), SLOT(slot_handleNSMCallback()));

    connect(&host, SIGNAL(DebugCallback()), SLOT(slot_handleDebugCallback()));
    connect(&host, SIGNAL(InfoCallback()), SLOT(slot_handleInfoCallback()));
    connect(&host, SIGNAL(ErrorCallback()), SLOT(slot_handleErrorCallback()));
    connect(&host, SIGNAL(QuitCallback()), SLOT(slot_handleQuitCallback()));
    connect(&host, SIGNAL(InlineDisplayRedrawCallback()), SLOT(slot_handleInlineDisplayRedrawCallback()));

    //-----------------------------------------------------------------------------------------------------------------
    // Final setup

    // Qt needs this so it properly creates & resizes the canvas
    self->ui.tabWidget->blockSignals(true);
    self->ui.tabWidget->setCurrentIndex(1);
    self->ui.tabWidget->setCurrentIndex(0);
    self->ui.tabWidget->blockSignals(false);

    // Start in patchbay tab if using forced patchbay mode
    if (host.processModeForced && host.processMode == ENGINE_PROCESS_MODE_PATCHBAY)
        self->ui.tabWidget->setCurrentIndex(1);

    // For NSM we wait for the open message
    if (NSM_URL != nullptr && host.nsmOK)
    {
        carla_nsm_ready(host.handle, NSM_CALLBACK_INIT);
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
// Plugin Editor Parent

void CarlaHostWindow::editDialogVisibilityChanged(int pluginId, bool visible)
{
}

void CarlaHostWindow::editDialogPluginHintsChanged(int pluginId, int hints)
{
}

void CarlaHostWindow::editDialogParameterValueChanged(int pluginId, int parameterId, float value)
{
}

void CarlaHostWindow::editDialogProgramChanged(int pluginId, int index)
{
}

void CarlaHostWindow::editDialogMidiProgramChanged(int pluginId, int index)
{
}

void CarlaHostWindow::editDialogNotePressed(int pluginId, int note)
{
}

void CarlaHostWindow::editDialogNoteReleased(int pluginId, int note)
{
}

void CarlaHostWindow::editDialogMidiActivityChanged(int pluginId, bool onOff)
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

    QMainWindow::changeEvent(event);
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
    if (self->fMacClosingHelper && ! (self->host.isControl || self->host.isPlugin))
    {
        self->fCustomStopAction = CUSTOM_ACTION_APP_CLOSE;
        self->fMacClosingHelper = false;
        event->ignore();

        for i in reversed(range(self->fPluginCount)):
            carla_show_custom_ui(self->host.handle, i, false);

        QTimer::singleShot(100, SIGNAL(close()));
        return;
    }
#endif

    self->killTimers();
    self->saveSettings();

    if (carla_is_engine_running(self->host.handle) && ! (self->host.isControl or self->host.isPlugin))
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
    const QString audioDriver = setEngineSettings(self->host);
    const bool firstInit = self->fFirstEngineInit;

    self->fFirstEngineInit = false;
    self->ui.text_logs->appendPlainText("======= Starting engine =======");

    if (carla_engine_init(self->host.handle, audioDriver.toUtf8(), self->fClientName.toUtf8()))
    {
        if (firstInit && ! (self->host.isControl or self->host.isPlugin))
        {
            QSafeSettings settings;
            const double lastBpm = settings.valueDouble("LastBPM", 120.0);
            if (lastBpm >= 20.0)
                carla_transport_bpm(self->host.handle, lastBpm);
        }
        return;
    }
    else if (firstInit)
    {
        self->ui.text_logs->appendPlainText("Failed to start engine on first try, ignored");
        return;
    }

    const QCarlaString audioError(carla_get_last_error(self->host.handle));

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
    RuntimeDriverSettingsW dialog(self->host.handle, self->fParentOrSelf);

    if (dialog.exec())
        return;

    QString audioDevice;
    uint bufferSize;
    double sampleRate;
    dialog.getValues(audioDevice, bufferSize, sampleRate);

    if (carla_is_engine_running(self->host.handle))
    {
        carla_set_engine_buffer_size_and_sample_rate(self->host.handle, bufferSize, sampleRate);
    }
    else
    {
        carla_set_engine_option(self->host.handle, ENGINE_OPTION_AUDIO_DEVICE, 0, audioDevice.toUtf8());
        carla_set_engine_option(self->host.handle, ENGINE_OPTION_AUDIO_BUFFER_SIZE, static_cast<int>(bufferSize), "");
        carla_set_engine_option(self->host.handle, ENGINE_OPTION_AUDIO_SAMPLE_RATE, static_cast<int>(sampleRate), "");
    }
}

bool CarlaHostWindow::slot_engineStopTryAgain()
{
    if (carla_is_engine_running(self->host.handle) && ! carla_set_engine_about_to_close(self->host.handle))
    {
        QTimer::singleShot(0, this, SLOT(slot_engineStopTryAgain()));
        return false;
    }

    self->engineStopFinal();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
// Engine (host callbacks)

void CarlaHostWindow::slot_handleEngineStartedCallback(uint pluginCount, int processMode, int transportMode, uint bufferSize, float sampleRate, QString driverName)
{
    self->ui.menu_PluginMacros->setEnabled(true);
    self->ui.menu_Canvas->setEnabled(true);
    self->ui.w_transport->setEnabled(true);

    self->ui.act_canvas_show_internal->blockSignals(true);
    self->ui.act_canvas_show_external->blockSignals(true);

    if (processMode == ENGINE_PROCESS_MODE_PATCHBAY) // && ! self->host.isPlugin:
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

    if (! (self->host.isControl or self->host.isPlugin))
    {
        const bool canSave = (self->fProjectFilename.isNotEmpty() && QFile(self->fProjectFilename).exists()) || self->fSessionManagerName.isEmpty();
        self->ui.act_file_save->setEnabled(canSave);
        self->ui.act_engine_start->setEnabled(false);
        self->ui.act_engine_stop->setEnabled(true);
    }

    if (! self->host.isPlugin)
        self->enableTransport(transportMode != ENGINE_TRANSPORT_MODE_DISABLED);

    if (self->host.isPlugin || self->fSessionManagerName.isEmpty())
    {
        self->ui.act_file_open->setEnabled(true);
        self->ui.act_file_save_as->setEnabled(true);
    }

    self->ui.cb_transport_jack->setChecked(transportMode == ENGINE_TRANSPORT_MODE_JACK);
    self->ui.cb_transport_jack->setEnabled(driverName == "JACK" and processMode != ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS);

    if (self->ui.cb_transport_link->isEnabled())
        self->ui.cb_transport_link->setChecked(self->host.transportExtra.contains(":link:"));

    self->updateBufferSize(bufferSize);
    self->updateSampleRate(sampleRate);
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
    // TODO
    patchcanvas.clear();
    */
    self->killTimers();

    // just in case
    self->removeAllPlugins();
    self->refreshRuntimeInfo(0.0, 0);

    self->ui.menu_PluginMacros->setEnabled(false);
    self->ui.menu_Canvas->setEnabled(false);
    self->ui.w_transport->setEnabled(false);

    if (! (self->host.isControl || self->host.isPlugin))
    {
        self->ui.act_file_save->setEnabled(false);
        self->ui.act_engine_start->setEnabled(true);
        self->ui.act_engine_stop->setEnabled(false);
    }

    if (self->host.isPlugin || self->fSessionManagerName.isEmpty())
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
    self->showSidePanel(yesNo);
}

void CarlaHostWindow::slot_showToolbar(const bool yesNo)
{
    self->showToolbar(yesNo);
}

void CarlaHostWindow::slot_showCanvasMeters(const bool yesNo)
{
    self->ui.peak_in->setVisible(yesNo);
    self->ui.peak_out->setVisible(yesNo);
    QTimer::singleShot(0, this, SLOT(slot_miniCanvasCheckAll()));
}

void CarlaHostWindow::slot_showCanvasKeyboard(const bool yesNo)
{
    /*
    // TODO
    self->ui.scrollArea->setVisible(yesNo);
    */
    QTimer::singleShot(0, this, SLOT(slot_miniCanvasCheckAll()));
}

void CarlaHostWindow::slot_configureCarla()
{
    const bool hasGL = true;
    CarlaSettingsW dialog(self->fParentOrSelf, self->host, true, hasGL);
    if (! dialog.exec())
        return;

    self->loadSettings(false);

    /*
    // TODO
    patchcanvas.clear()

    setupCanvas();
    */
    slot_miniCanvasCheckAll();

    if (self->host.processMode == ENGINE_PROCESS_MODE_CONTINUOUS_RACK && self->host.isPlugin)
        pass();
    else if (carla_is_engine_running(self->host.handle))
        carla_patchbay_refresh(self->host.handle, self->fExternalPatchbay);
}

//---------------------------------------------------------------------------------------------------------------------
// About (menu actions)

void CarlaHostWindow::slot_aboutCarla()
{
    CarlaAboutW(self->fParentOrSelf, self->host).exec();
}

void CarlaHostWindow::slot_aboutJuce()
{
    JuceAboutW(self->fParentOrSelf).exec();
}

void CarlaHostWindow::slot_aboutQt()
{
    qApp->aboutQt();
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
    if (self->host.isPlugin || ! carla_is_engine_running(self->host.handle))
        return;

    if (toggled)
        carla_transport_play(self->host.handle);
    else
        carla_transport_pause(self->host.handle);

    self->refreshTransport();
}

void CarlaHostWindow::slot_transportStop()
{
    if (self->host.isPlugin || ! carla_is_engine_running(self->host.handle))
        return;

    carla_transport_pause(self->host.handle);
    carla_transport_relocate(self->host.handle, 0);

    self->refreshTransport();
}

void CarlaHostWindow::slot_transportBackwards()
{
    if (self->host.isPlugin || ! carla_is_engine_running(self->host.handle))
        return;

    uint64_t newFrame = carla_get_current_transport_frame(self->host.handle);

    if (newFrame > 100000)
        newFrame -= 100000;
    else
        newFrame = 0;

    carla_transport_relocate(self->host.handle, newFrame);
}

void CarlaHostWindow::slot_transportBpmChanged(const qreal newValue)
{
    carla_transport_bpm(self->host.handle, newValue);
}

void CarlaHostWindow::slot_transportForwards()
{
    if (carla_isZero(self->fSampleRate) || self->host.isPlugin || ! carla_is_engine_running(self->host.handle))
        return;

    const uint64_t newFrame = carla_get_current_transport_frame(self->host.handle) + uint64_t(self->fSampleRate*2.5);
    carla_transport_relocate(self->host.handle, newFrame);
}

void CarlaHostWindow::slot_transportJackEnabled(const bool clicked)
{
    if (! carla_is_engine_running(self->host.handle))
        return;
    self->host.transportMode = clicked ? ENGINE_TRANSPORT_MODE_JACK : ENGINE_TRANSPORT_MODE_INTERNAL;
    carla_set_engine_option(self->host.handle, ENGINE_OPTION_TRANSPORT_MODE, self->host.transportMode, self->host.transportExtra.toUtf8());
}

void CarlaHostWindow::slot_transportLinkEnabled(const bool clicked)
{
    if (! carla_is_engine_running(self->host.handle))
        return;
    const char* const extra = clicked ? ":link:" : "";
    self->host.transportExtra = extra;
    carla_set_engine_option(self->host.handle, ENGINE_OPTION_TRANSPORT_MODE, self->host.transportMode, self->host.transportExtra.toUtf8());
}

//---------------------------------------------------------------------------------------------------------------------
// Other

void CarlaHostWindow::slot_xrunClear()
{
    carla_clear_engine_xruns(self->host.handle);
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
// Canvas callback

/*
static void _canvasCallback(void* const ptr, const int action, int value1, int value2, QString valueStr)
{
    CarlaHost* const host = (CarlaHost*)(ptr);
    CARLA_SAFE_ASSERT_RETURN(host != nullptr,);

    switch (action)
    {
    }
}
*/

//---------------------------------------------------------------------------------------------------------------------
// Engine callback

static void _engineCallback(void* const ptr, const EngineCallbackOpcode action, uint pluginId, int value1, int value2, int value3, float valuef, const char* const valueStr)
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
        CARLA_SAFE_ASSERT_INT_RETURN(value3 >= 0, value3,);
        emit host->EngineStartedCallback(pluginId, value1, value2, static_cast<uint>(value3), valuef, valueStr);
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

    static QByteArray byteRet;
    byteRet = ret.toUtf8();

    return byteRet.constData();
}

//---------------------------------------------------------------------------------------------------------------------
// Init host

CarlaHost& initHost(const QString initName, const bool isControl, const bool isPlugin, const bool failError)
{
    QString pathBinaries, pathResources;
    getPaths(pathBinaries, pathResources);

    //-----------------------------------------------------------------------------------------------------------------
    // Fail if binary dir is not found

    if (! QDir(pathBinaries).exists())
    {
        if (failError)
        {
            QMessageBox::critical(nullptr, "Error", "Failed to find the carla binaries, cannot continue");
            std::exit(1);
        }
        // FIXME?
        // return;
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Print info

    carla_stdout(QString("Carla %1 started, status:").arg(CARLA_VERSION_STRING).toUtf8());
    carla_stdout(QString("  Qt version:     %1").arg(QT_VERSION_STR).toUtf8());
    carla_stdout(QString("  Binary dir:     %1").arg(pathBinaries).toUtf8());
    carla_stdout(QString("  Resources dir:  %1").arg(pathResources).toUtf8());

    // ----------------------------------------------------------------------------------------------------------------
    // Init host

    // TODO
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

    // TODO
    if (isPlugin)
        pass();
    else if (isControl)
        pass();
    else
        host.handle = carla_standalone_host_init();

    carla_set_engine_callback(host.handle, _engineCallback, &host);
    carla_set_file_callback(host.handle, _fileCallback, nullptr);

    // If it's a plugin the paths are already set
    if (! isPlugin)
    {
        host.pathBinaries  = pathBinaries;
        host.pathResources = pathResources;
        carla_set_engine_option(host.handle, ENGINE_OPTION_PATH_BINARIES, 0, pathBinaries.toUtf8());
        carla_set_engine_option(host.handle, ENGINE_OPTION_PATH_RESOURCES, 0, pathResources.toUtf8());

        if (! isControl)
        {
            const pid_t pid = getpid();
            if (pid > 0)
                host.nsmOK = carla_nsm_init(host.handle, static_cast<uint64_t>(pid), initName.toUtf8());
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Done

    gCarla.host = &host;
    return host;
}

//---------------------------------------------------------------------------------------------------------------------
// Load host settings

void loadHostSettings(CarlaHost& host)
{
    const QSafeSettings settings("falkTX", "Carla2");

    host.experimental = settings.valueBool(CARLA_KEY_MAIN_EXPERIMENTAL, CARLA_DEFAULT_MAIN_EXPERIMENTAL);
    host.exportLV2 = settings.valueBool(CARLA_KEY_EXPERIMENTAL_EXPORT_LV2, CARLA_DEFAULT_EXPERIMENTAL_LV2_EXPORT);
    host.manageUIs = settings.valueBool(CARLA_KEY_ENGINE_MANAGE_UIS, CARLA_DEFAULT_MANAGE_UIS);
    host.maxParameters = settings.valueUInt(CARLA_KEY_ENGINE_MAX_PARAMETERS, CARLA_DEFAULT_MAX_PARAMETERS);
    host.resetXruns = settings.valueBool(CARLA_KEY_ENGINE_RESET_XRUNS, CARLA_DEFAULT_RESET_XRUNS);
    host.forceStereo = settings.valueBool(CARLA_KEY_ENGINE_FORCE_STEREO, CARLA_DEFAULT_FORCE_STEREO);
    host.preferPluginBridges = settings.valueBool(CARLA_KEY_ENGINE_PREFER_PLUGIN_BRIDGES, CARLA_DEFAULT_PREFER_PLUGIN_BRIDGES);
    host.preferUIBridges = settings.valueBool(CARLA_KEY_ENGINE_PREFER_UI_BRIDGES, CARLA_DEFAULT_PREFER_UI_BRIDGES);
    host.preventBadBehaviour = settings.valueBool(CARLA_KEY_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR, CARLA_DEFAULT_EXPERIMENTAL_PREVENT_BAD_BEHAVIOUR);
    host.showLogs = settings.valueBool(CARLA_KEY_MAIN_SHOW_LOGS, CARLA_DEFAULT_MAIN_SHOW_LOGS);
    host.showPluginBridges = settings.valueBool(CARLA_KEY_EXPERIMENTAL_PLUGIN_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_PLUGIN_BRIDGES);
    host.showWineBridges = settings.valueBool(CARLA_KEY_EXPERIMENTAL_WINE_BRIDGES, CARLA_DEFAULT_EXPERIMENTAL_WINE_BRIDGES);
    host.uiBridgesTimeout = settings.valueUInt(CARLA_KEY_ENGINE_UI_BRIDGES_TIMEOUT, CARLA_DEFAULT_UI_BRIDGES_TIMEOUT);
    host.uisAlwaysOnTop = settings.valueBool(CARLA_KEY_ENGINE_UIS_ALWAYS_ON_TOP, CARLA_DEFAULT_UIS_ALWAYS_ON_TOP);

    if (host.isPlugin)
        return;

    host.transportExtra = settings.valueString(CARLA_KEY_ENGINE_TRANSPORT_EXTRA, "");

    // enums
    /*
    // TODO
    if (host.audioDriverForced.isNotEmpty())
        host.transportMode = settings.valueUInt(CARLA_KEY_ENGINE_TRANSPORT_MODE, CARLA_DEFAULT_TRANSPORT_MODE);

    if (! host.processModeForced)
        host.processMode = settings.valueUInt(CARLA_KEY_ENGINE_PROCESS_MODE, CARLA_DEFAULT_PROCESS_MODE);
    */

    host.nextProcessMode = host.processMode;

    // ----------------------------------------------------------------------------------------------------------------
    // fix things if needed

    if (host.processMode == ENGINE_PROCESS_MODE_MULTIPLE_CLIENTS)
    {
        if (LADISH_APP_NAME)
        {
            carla_stdout("LADISH detected but using multiple clients (not allowed), forcing single client now");
            host.nextProcessMode = host.processMode = ENGINE_PROCESS_MODE_SINGLE_CLIENT;
        }
        else
        {
            host.transportMode = ENGINE_TRANSPORT_MODE_JACK;
        }
    }

    if (gCarla.nogui)
        host.showLogs = false;

    // ----------------------------------------------------------------------------------------------------------------
    // run headless host now if nogui option enabled

    if (gCarla.nogui)
        runHostWithoutUI(host);
}

//---------------------------------------------------------------------------------------------------------------------
// Set host settings

void setHostSettings(const CarlaHost& host)
{
    carla_set_engine_option(host.handle, ENGINE_OPTION_FORCE_STEREO,          host.forceStereo,         "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_MAX_PARAMETERS,        static_cast<int>(host.maxParameters), "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, host.preferPluginBridges, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_PREFER_UI_BRIDGES,     host.preferUIBridges,     "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_PREVENT_BAD_BEHAVIOUR, host.preventBadBehaviour, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_UI_BRIDGES_TIMEOUT,    host.uiBridgesTimeout,    "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_UIS_ALWAYS_ON_TOP,     host.uisAlwaysOnTop,      "");

    if (host.isPlugin || host.isRemote || carla_is_engine_running(host.handle))
        return;

    carla_set_engine_option(host.handle, ENGINE_OPTION_PROCESS_MODE,          host.nextProcessMode,     "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_TRANSPORT_MODE,        host.transportMode,       host.transportExtra.toUtf8());
    carla_set_engine_option(host.handle, ENGINE_OPTION_DEBUG_CONSOLE_OUTPUT,  host.showLogs,            "");
}

//---------------------------------------------------------------------------------------------------------------------
// Set Engine settings according to carla preferences. Returns selected audio driver.

QString setEngineSettings(CarlaHost& host)
{
    //-----------------------------------------------------------------------------------------------------------------
    // do nothing if control

    if (host.isControl)
        return "Control";

    //-----------------------------------------------------------------------------------------------------------------

    const QSafeSettings settings("falkTX", "Carla2");

    //-----------------------------------------------------------------------------------------------------------------
    // main settings

    setHostSettings(host);

    //-----------------------------------------------------------------------------------------------------------------
    // file paths

    QStringList FILE_PATH_AUDIO = settings.valueStringList(CARLA_KEY_PATHS_AUDIO, CARLA_DEFAULT_FILE_PATH_AUDIO);
    QStringList FILE_PATH_MIDI  = settings.valueStringList(CARLA_KEY_PATHS_MIDI,  CARLA_DEFAULT_FILE_PATH_MIDI);

    /*
    // TODO
    carla_set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_AUDIO, splitter.join(FILE_PATH_AUDIO));
    carla_set_engine_option(ENGINE_OPTION_FILE_PATH, FILE_MIDI,  splitter.join(FILE_PATH_MIDI));
    */
    // CARLA_PATH_SPLITTER

    //-----------------------------------------------------------------------------------------------------------------
    // plugin paths

    QStringList LADSPA_PATH = settings.valueStringList(CARLA_KEY_PATHS_LADSPA, CARLA_DEFAULT_LADSPA_PATH);
    QStringList DSSI_PATH   = settings.valueStringList(CARLA_KEY_PATHS_DSSI,   CARLA_DEFAULT_DSSI_PATH);
    QStringList LV2_PATH    = settings.valueStringList(CARLA_KEY_PATHS_LV2,    CARLA_DEFAULT_LV2_PATH);
    QStringList VST2_PATH   = settings.valueStringList(CARLA_KEY_PATHS_VST2,   CARLA_DEFAULT_VST2_PATH);
    QStringList VST3_PATH   = settings.valueStringList(CARLA_KEY_PATHS_VST3,   CARLA_DEFAULT_VST3_PATH);
    QStringList SF2_PATH    = settings.valueStringList(CARLA_KEY_PATHS_SF2,    CARLA_DEFAULT_SF2_PATH);
    QStringList SFZ_PATH    = settings.valueStringList(CARLA_KEY_PATHS_SFZ,    CARLA_DEFAULT_SFZ_PATH);
    QStringList JSFX_PATH   = settings.valueStringList(CARLA_KEY_PATHS_JSFX,   CARLA_DEFAULT_JSFX_PATH);

    /*
    // TODO
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LADSPA, splitter.join(LADSPA_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_DSSI,   splitter.join(DSSI_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_LV2,    splitter.join(LV2_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST2,   splitter.join(VST2_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_VST3,   splitter.join(VST3_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SF2,    splitter.join(SF2_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_SFZ,    splitter.join(SFZ_PATH))
    carla_set_engine_option(ENGINE_OPTION_PLUGIN_PATH, PLUGIN_JSFX,   splitter.join(JSFX_PATH))
    */

    //-----------------------------------------------------------------------------------------------------------------
    // don't continue if plugin

    if (host.isPlugin)
        return "Plugin";

    //-----------------------------------------------------------------------------------------------------------------
    // osc settings

    const bool oscEnabled = settings.valueBool(CARLA_KEY_OSC_ENABLED, CARLA_DEFAULT_OSC_ENABLED);

    int portNumTCP, portNumUDP;

    if (! settings.valueBool(CARLA_KEY_OSC_TCP_PORT_ENABLED, CARLA_DEFAULT_OSC_TCP_PORT_ENABLED))
        portNumTCP = -1;
    else if (settings.valueBool(CARLA_KEY_OSC_TCP_PORT_RANDOM, CARLA_DEFAULT_OSC_TCP_PORT_RANDOM))
        portNumTCP = 0;
    else
        portNumTCP = settings.valueIntPositive(CARLA_KEY_OSC_TCP_PORT_NUMBER, CARLA_DEFAULT_OSC_TCP_PORT_NUMBER);

    if (! settings.valueBool(CARLA_KEY_OSC_UDP_PORT_ENABLED, CARLA_DEFAULT_OSC_UDP_PORT_ENABLED))
        portNumUDP = -1;
    else if (settings.valueBool(CARLA_KEY_OSC_UDP_PORT_RANDOM, CARLA_DEFAULT_OSC_UDP_PORT_RANDOM))
        portNumUDP = 0;
    else
        portNumUDP = settings.valueIntPositive(CARLA_KEY_OSC_UDP_PORT_NUMBER, CARLA_DEFAULT_OSC_UDP_PORT_NUMBER);

    carla_set_engine_option(host.handle, ENGINE_OPTION_OSC_ENABLED, oscEnabled ? 1 : 0, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_OSC_PORT_TCP, portNumTCP, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_OSC_PORT_UDP, portNumUDP, "");

    //-----------------------------------------------------------------------------------------------------------------
    // wine settings

    const QString optWineExecutable = settings.valueString(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE);
    const bool optWineAutoPrefix = settings.valueBool(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX);
    const QString optWineFallbackPrefix = settings.valueString(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX);
    const bool optWineRtPrioEnabled = settings.valueBool(CARLA_KEY_WINE_RT_PRIO_ENABLED, CARLA_DEFAULT_WINE_RT_PRIO_ENABLED);
    const int optWineBaseRtPrio = settings.valueIntPositive(CARLA_KEY_WINE_BASE_RT_PRIO,   CARLA_DEFAULT_WINE_BASE_RT_PRIO);
    const int optWineServerRtPrio = settings.valueIntPositive(CARLA_KEY_WINE_SERVER_RT_PRIO, CARLA_DEFAULT_WINE_SERVER_RT_PRIO);

    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_EXECUTABLE, 0, optWineExecutable.toUtf8());
    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_AUTO_PREFIX, optWineAutoPrefix ? 1 : 0, "");
    /*
    // TODO
    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_FALLBACK_PREFIX, 0, os.path.expanduser(optWineFallbackPrefix));
    */
    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_RT_PRIO_ENABLED, optWineRtPrioEnabled ? 1 : 0, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_BASE_RT_PRIO, optWineBaseRtPrio, "");
    carla_set_engine_option(host.handle, ENGINE_OPTION_WINE_SERVER_RT_PRIO, optWineServerRtPrio, "");

    //-----------------------------------------------------------------------------------------------------------------
    // driver and device settings

    QString audioDriver;

    // driver name
    if (host.audioDriverForced.isNotEmpty())
        audioDriver = host.audioDriverForced;
    else
        audioDriver = settings.valueString(CARLA_KEY_ENGINE_AUDIO_DRIVER, CARLA_DEFAULT_AUDIO_DRIVER);

    // driver options
    const QString prefix(QString("%1%2").arg(CARLA_KEY_ENGINE_DRIVER_PREFIX).arg(audioDriver));
    const QString audioDevice = settings.valueString(QString("%1/Device").arg(prefix), "");
    const int audioBufferSize = settings.valueIntPositive(QString("%1/BufferSize").arg(prefix), CARLA_DEFAULT_AUDIO_BUFFER_SIZE);
    const int audioSampleRate = settings.valueIntPositive(QString("%1/SampleRate").arg(prefix), CARLA_DEFAULT_AUDIO_SAMPLE_RATE);
    const bool audioTripleBuffer = settings.valueBool(QString("%1/TripleBuffer").arg(prefix), CARLA_DEFAULT_AUDIO_TRIPLE_BUFFER);

    // Only setup audio things if engine is not running
    if (! carla_is_engine_running(host.handle))
    {
        carla_set_engine_option(host.handle, ENGINE_OPTION_AUDIO_DRIVER, 0, audioDriver.toUtf8());
        carla_set_engine_option(host.handle, ENGINE_OPTION_AUDIO_DEVICE, 0, audioDevice.toUtf8());

        if (! audioDriver.startsWith("JACK"))
        {
            carla_set_engine_option(host.handle, ENGINE_OPTION_AUDIO_BUFFER_SIZE, audioBufferSize, "");
            carla_set_engine_option(host.handle, ENGINE_OPTION_AUDIO_SAMPLE_RATE, audioSampleRate, "");
            carla_set_engine_option(host.handle, ENGINE_OPTION_AUDIO_TRIPLE_BUFFER, audioTripleBuffer ? 1 : 0, "");
        }
    }

    //-----------------------------------------------------------------------------------------------------------------
    // fix things if needed

    if (audioDriver != "JACK" && host.transportMode == ENGINE_TRANSPORT_MODE_JACK)
    {
        host.transportMode = ENGINE_TRANSPORT_MODE_INTERNAL;
        carla_set_engine_option(host.handle,
                                ENGINE_OPTION_TRANSPORT_MODE,
                                ENGINE_TRANSPORT_MODE_INTERNAL,
                                host.transportExtra.toUtf8());
    }

    //-----------------------------------------------------------------------------------------------------------------
    // return selected driver name

    return audioDriver;
}

//---------------------------------------------------------------------------------------------------------------------
// Run Carla without showing UI

void runHostWithoutUI(CarlaHost& host)
{
    //-----------------------------------------------------------------------------------------------------------------
    // Some initial checks

    if (! gCarla.nogui)
        return;

    const QString projectFile = getInitialProjectFile(true);

    if (projectFile.isEmpty())
    {
        carla_stdout("Carla no-gui mode can only be used together with a project file.");
        std::exit(1);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Init engine

    const QString audioDriver = setEngineSettings(host);

    if (! carla_engine_init(host.handle, audioDriver.toUtf8(), "Carla"))
    {
        carla_stdout("Engine failed to initialize, possible reasons:\n%s", carla_get_last_error(host.handle));
        std::exit(1);
    }

    if (! carla_load_project(host.handle, projectFile.toUtf8()))
    {
        carla_stdout("Failed to load selected project file, possible reasons:\n%s", carla_get_last_error(host.handle));
        carla_engine_close(host.handle);
        std::exit(1);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Idle

    carla_stdout("Carla ready!");

    while (carla_is_engine_running(host.handle) && ! gCarla.term)
    {
        carla_engine_idle(host.handle);
        carla_msleep(33); // 30 Hz
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Stop

    carla_engine_close(host.handle);
    std::exit(0);
}

//---------------------------------------------------------------------------------------------------------------------
