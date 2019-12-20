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

#ifndef CARLA_HOST_HPP_INCLUDED
#define CARLA_HOST_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtWidgets/QMainWindow>

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_shared.hpp"

#include "CarlaBackend.h"
#include "CarlaJuceUtils.hpp"

CARLA_BACKEND_USE_NAMESPACE;

//---------------------------------------------------------------------------------------------------------------------

class CarlaHost : public QObject
{
    Q_OBJECT

public:
    // info about this host object
    bool isControl;
    bool isPlugin;
    bool isRemote;
    bool nsmOK;

    // settings
    EngineProcessMode processMode;
    EngineTransportMode transportMode;
    QCarlaString transportExtra;
    EngineProcessMode nextProcessMode;
    bool processModeForced;
    QCarlaString audioDriverForced;

    // settings
    bool experimental;
    bool exportLV2;
    bool forceStereo;
    bool manageUIs;
    uint maxParameters;
    bool preferPluginBridges;
    bool preferUIBridges;
    bool preventBadBehaviour;
    bool showLogs;
    bool showPluginBridges;
    bool showWineBridges;
    bool uiBridgesTimeout;
    bool uisAlwaysOnTop;

    // settings
    QString pathBinaries;
    QString pathResources;

    CarlaHost();

    // Qt Stuff
    Q_SIGNAL void EngineStartedCallback(uint, int, int, int, float, QString);
    Q_SIGNAL void EngineStoppedCallback();
};

//---------------------------------------------------------------------------------------------------------------------
// Host Window

class CarlaHostWindow : public QMainWindow
{
    Q_OBJECT

    // signals
    Q_SIGNAL void SIGTERM();
    Q_SIGNAL void SIGUSR1();

    enum CustomActions {
        CUSTOM_ACTION_NONE,
        CUSTOM_ACTION_APP_CLOSE,
        CUSTOM_ACTION_PROJECT_LOAD
    };

public:
    //-----------------------------------------------------------------------------------------------------------------

    CarlaHostWindow(CarlaHost& host, const bool withCanvas, QWidget* const parent = nullptr);
    ~CarlaHostWindow() override;

private:
    CarlaHost& host;
    struct PrivateData;
    PrivateData* const self;

    //-----------------------------------------------------------------------------------------------------------------
    // Files (menu actions)

    Q_SLOT void slot_fileNew();
    Q_SLOT void slot_fileOpen();
    Q_SLOT void slot_fileSave(bool saveAs = false);
    Q_SLOT void slot_fileSaveAs();
    Q_SLOT void slot_loadProjectNow();

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (menu actions)

    Q_SLOT void slot_engineStart();
    Q_SLOT bool slot_engineStop(bool forced = false);
    Q_SLOT void slot_engineConfig();
    Q_SLOT bool slot_engineStopTryAgain();

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (host callbacks)

    Q_SLOT void slot_handleEngineStartedCallback(uint pluginCount, int processMode, int transportMode, int bufferSize, float sampleRate, QString driverName);
    Q_SLOT void slot_handleEngineStoppedCallback();
    Q_SLOT void slot_handleTransportModeChangedCallback(int transportMode, QString transportExtra);
    Q_SLOT void slot_handleBufferSizeChangedCallback(int newBufferSize);
    Q_SLOT void slot_handleSampleRateChangedCallback(double newSampleRate);
    Q_SLOT void slot_handleCancelableActionCallback(int pluginId, bool started, QString action);
    Q_SLOT void slot_canlableActionBoxClicked();
    Q_SLOT void slot_handleProjectLoadFinishedCallback();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (menu actions)

    Q_SLOT void slot_favoritePluginAdd();
    Q_SLOT void slot_showPluginActionsMenu();
    Q_SLOT void slot_pluginAdd();
    Q_SLOT void slot_confirmRemoveAll();
    Q_SLOT void slot_jackAppAdd();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (macros)

    Q_SLOT void slot_pluginsEnable();
    Q_SLOT void slot_pluginsDisable();
    Q_SLOT void slot_pluginsVolume100();
    Q_SLOT void slot_pluginsMute();
    Q_SLOT void slot_pluginsWet100();
    Q_SLOT void slot_pluginsBypass();
    Q_SLOT void slot_pluginsCenter();
    Q_SLOT void slot_pluginsCompact();
    Q_SLOT void slot_pluginsExpand();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (host callbacks)

    Q_SLOT void slot_handlePluginAddedCallback(int pluginId, QString pluginName);
    Q_SLOT void slot_handlePluginRemovedCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (menu actions)

    Q_SLOT void slot_canvasShowInternal();
    Q_SLOT void slot_canvasShowExternal();
    Q_SLOT void slot_canvasArrange();
    Q_SLOT void slot_canvasRefresh();
    Q_SLOT void slot_canvasZoomFit();
    Q_SLOT void slot_canvasZoomIn();
    Q_SLOT void slot_canvasZoomOut();
    Q_SLOT void slot_canvasZoomReset();
    Q_SLOT void slot_canvasSaveImage();

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (canvas callbacks)

    Q_SLOT void slot_canvasItemMoved(int group_id, int split_mode, QPointF pos);
    Q_SLOT void slot_canvasSelectionChanged();
    Q_SLOT void slot_canvasScaleChanged(double scale);
    Q_SLOT void slot_canvasPluginSelected(QList<void*> pluginList);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (host callbacks)

    Q_SLOT void slot_handlePatchbayClientAddedCallback(int clientId, int clientIcon, int pluginId, QString clientName);
    Q_SLOT void slot_handlePatchbayClientRemovedCallback(int clientId);
    Q_SLOT void slot_handlePatchbayClientRenamedCallback(int clientId, QString newClientName);
    Q_SLOT void slot_handlePatchbayClientDataChangedCallback(int clientId, int clientIcon, int pluginId);
    Q_SLOT void slot_handlePatchbayPortAddedCallback(int clientId, int portId, int portFlags, int portGroupId, QString portName);
    Q_SLOT void slot_handlePatchbayPortRemovedCallback(int groupId, int portId);
    Q_SLOT void slot_handlePatchbayPortChangedCallback(int groupId, int portId, int portFlags, int portGroupId, QString newPortName);
    Q_SLOT void slot_handlePatchbayPortGroupAddedCallback(int groupId, int portId, int portGroupId, QString newPortName);
    Q_SLOT void slot_handlePatchbayPortGroupRemovedCallback(int groupId, int portId);
    Q_SLOT void slot_handlePatchbayPortGroupChangedCallback(int groupId, int portId, int portGroupId, QString newPortName);
    Q_SLOT void slot_handlePatchbayConnectionAddedCallback(int connectionId, int groupOutId, int portOutId, int groupInId, int portInId);
    Q_SLOT void slot_handlePatchbayConnectionRemovedCallback(int connectionId, int portOutId, int portInId);

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (helpers)

    Q_SLOT void slot_restoreCanvasScrollbarValues();

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (menu actions)

    Q_SLOT void slot_showSidePanel(bool yesNo);
    Q_SLOT void slot_showToolbar(bool yesNo);
    Q_SLOT void slot_showCanvasMeters(bool yesNo);
    Q_SLOT void slot_showCanvasKeyboard(bool yesNo);
    Q_SLOT void slot_configureCarla();

    //-----------------------------------------------------------------------------------------------------------------
    // About (menu actions)

    Q_SLOT void slot_aboutCarla();
    Q_SLOT void slot_aboutJuce();
    Q_SLOT void slot_aboutQt();

    //-----------------------------------------------------------------------------------------------------------------
    // Disk (menu actions)

    Q_SLOT void slot_diskFolderChanged(int index);
    Q_SLOT void slot_diskFolderAdd();
    Q_SLOT void slot_diskFolderRemove();
    Q_SLOT void slot_fileTreeDoubleClicked(QModelIndex* modelIndex);

    //-----------------------------------------------------------------------------------------------------------------
    // Transport (menu actions)

    Q_SLOT void slot_transportPlayPause(bool toggled);
    Q_SLOT void slot_transportStop();
    Q_SLOT void slot_transportBackwards();
    Q_SLOT void slot_transportBpmChanged(qreal newValue);
    Q_SLOT void slot_transportForwards();
    Q_SLOT void slot_transportJackEnabled(bool clicked);
    Q_SLOT void slot_transportLinkEnabled(bool clicked);

    //-----------------------------------------------------------------------------------------------------------------
    // Other

    Q_SLOT void slot_xrunClear();

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas scrollbars

    Q_SLOT void slot_horizontalScrollBarChanged(int value);
    Q_SLOT void slot_verticalScrollBarChanged(int value);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard

    Q_SLOT void slot_noteOn(int note);
    Q_SLOT void slot_noteOff(int note);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard (host callbacks)

    Q_SLOT void slot_handleNoteOnCallback(int pluginId, int channel, int note, int velocity);
    Q_SLOT void slot_handleNoteOffCallback(int pluginId, int channel, int note);

    //-----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_handleUpdateCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------
    // MiniCanvas stuff

    Q_SLOT void slot_miniCanvasCheckAll();
    Q_SLOT void slot_miniCanvasCheckSize();
    Q_SLOT void slot_miniCanvasMoved(qreal xp, qreal yp);

    //-----------------------------------------------------------------------------------------------------------------
    // Misc

    Q_SLOT void slot_tabChanged(int index);
    Q_SLOT void slot_handleReloadAllCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_handleNSMCallback(int opcode, int valueInt, QString valueStr);

    //-----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_handleDebugCallback(int pluginId, int value1, int value2, int value3, float valuef, QString valueStr);
    Q_SLOT void slot_handleInfoCallback(QString info);
    Q_SLOT void slot_handleErrorCallback(QString error);
    Q_SLOT void slot_handleQuitCallback();
    Q_SLOT void slot_handleInlineDisplayRedrawCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------

    Q_SLOT void slot_handleSIGUSR1();
    Q_SLOT void slot_handleSIGTERM();

    //-----------------------------------------------------------------------------------------------------------------
    // show/hide event

    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------
    // resize event

    void resizeEvent(QResizeEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------
    // timer event

    void timerEvent(QTimerEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------
    // color/style change event

    void changeEvent(QEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------
    // close event

    void closeEvent(QCloseEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaHostWindow)
};

//---------------------------------------------------------------------------------------------------------------------
// Init host

CarlaHost& initHost(const QString initName, const bool isControl, const bool isPlugin, const bool failError);

//---------------------------------------------------------------------------------------------------------------------
// Load host settings

void loadHostSettings(CarlaHost& host);

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_HOST_HPP_INCLUDED
