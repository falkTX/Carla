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

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtWidgets/QMainWindow>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_shared.hpp"
#include "carla_widgets.hpp"

#include "CarlaHost.h"
#include "CarlaJuceUtils.hpp"

CARLA_BACKEND_USE_NAMESPACE;

//---------------------------------------------------------------------------------------------------------------------

class CarlaHost : public QObject
{
    Q_OBJECT

public:
    // host handle
    CarlaHostHandle handle;

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
    bool resetXruns;
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

signals:
    void SignalTerminate();
    void SignalSave();

    // Engine stuff
    void EngineStartedCallback(uint, int, int, uint, float, QString);
    void EngineStoppedCallback();

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaHost)
};

//---------------------------------------------------------------------------------------------------------------------
// Host Window

class CarlaHostWindow : public QMainWindow,
                        public PluginEditParentMeta
{
    Q_OBJECT

public:
    //-----------------------------------------------------------------------------------------------------------------

    CarlaHostWindow(CarlaHost& host, const bool withCanvas, QWidget* const parent = nullptr);
    ~CarlaHostWindow() override;

private:
    struct PrivateData;
    PrivateData* const self;

protected:
    //-----------------------------------------------------------------------------------------------------------------
    // Plugin Editor Parent

    void editDialogVisibilityChanged(int pluginId, bool visible) override;
    void editDialogPluginHintsChanged(int pluginId, int hints) override;
    void editDialogParameterValueChanged(int pluginId, int parameterId, float value) override;
    void editDialogProgramChanged(int pluginId, int index) override;
    void editDialogMidiProgramChanged(int pluginId, int index) override;
    void editDialogNotePressed(int pluginId, int note) override;
    void editDialogNoteReleased(int pluginId, int note) override;
    void editDialogMidiActivityChanged(int pluginId, bool onOff) override;

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

private slots:
    //-----------------------------------------------------------------------------------------------------------------
    // Files (menu actions)

    void slot_fileNew();
    void slot_fileOpen();
    void slot_fileSave(bool saveAs = false);
    void slot_fileSaveAs();
    void slot_loadProjectNow();

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (menu actions)

    void slot_engineStart();
    bool slot_engineStop(bool forced = false);
    void slot_engineConfig();
    bool slot_engineStopTryAgain();

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (host callbacks)

    void slot_handleEngineStartedCallback(uint pluginCount, int processMode, int transportMode, uint bufferSize, float sampleRate, QString driverName);
    void slot_handleEngineStoppedCallback();
    void slot_handleTransportModeChangedCallback(int transportMode, QString transportExtra);
    void slot_handleBufferSizeChangedCallback(int newBufferSize);
    void slot_handleSampleRateChangedCallback(double newSampleRate);
    void slot_handleCancelableActionCallback(int pluginId, bool started, QString action);
    void slot_canlableActionBoxClicked();
    void slot_handleProjectLoadFinishedCallback();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (menu actions)

    void slot_favoritePluginAdd();
    void slot_showPluginActionsMenu();
    void slot_pluginAdd();
    void slot_confirmRemoveAll();
    void slot_jackAppAdd();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (macros)

    void slot_pluginsEnable();
    void slot_pluginsDisable();
    void slot_pluginsVolume100();
    void slot_pluginsMute();
    void slot_pluginsWet100();
    void slot_pluginsBypass();
    void slot_pluginsCenter();
    void slot_pluginsCompact();
    void slot_pluginsExpand();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (host callbacks)

    void slot_handlePluginAddedCallback(int pluginId, QString pluginName);
    void slot_handlePluginRemovedCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (menu actions)

    void slot_canvasShowInternal();
    void slot_canvasShowExternal();
    void slot_canvasArrange();
    void slot_canvasRefresh();
    void slot_canvasZoomFit();
    void slot_canvasZoomIn();
    void slot_canvasZoomOut();
    void slot_canvasZoomReset();
    void slot_canvasSaveImage();

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (canvas callbacks)

    void slot_canvasItemMoved(int group_id, int split_mode, QPointF pos);
    void slot_canvasSelectionChanged();
    void slot_canvasScaleChanged(double scale);
    void slot_canvasPluginSelected(QList<void*> pluginList);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (host callbacks)

    void slot_handlePatchbayClientAddedCallback(int clientId, int clientIcon, int pluginId, QString clientName);
    void slot_handlePatchbayClientRemovedCallback(int clientId);
    void slot_handlePatchbayClientRenamedCallback(int clientId, QString newClientName);
    void slot_handlePatchbayClientDataChangedCallback(int clientId, int clientIcon, int pluginId);
    void slot_handlePatchbayPortAddedCallback(int clientId, int portId, int portFlags, int portGroupId, QString portName);
    void slot_handlePatchbayPortRemovedCallback(int groupId, int portId);
    void slot_handlePatchbayPortChangedCallback(int groupId, int portId, int portFlags, int portGroupId, QString newPortName);
    void slot_handlePatchbayPortGroupAddedCallback(int groupId, int portId, int portGroupId, QString newPortName);
    void slot_handlePatchbayPortGroupRemovedCallback(int groupId, int portId);
    void slot_handlePatchbayPortGroupChangedCallback(int groupId, int portId, int portGroupId, QString newPortName);
    void slot_handlePatchbayConnectionAddedCallback(int connectionId, int groupOutId, int portOutId, int groupInId, int portInId);
    void slot_handlePatchbayConnectionRemovedCallback(int connectionId, int portOutId, int portInId);

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (helpers)

    void slot_restoreCanvasScrollbarValues();

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (menu actions)

    void slot_showSidePanel(bool yesNo);
    void slot_showToolbar(bool yesNo);
    void slot_showCanvasMeters(bool yesNo);
    void slot_showCanvasKeyboard(bool yesNo);
    void slot_configureCarla();

    //-----------------------------------------------------------------------------------------------------------------
    // About (menu actions)

    void slot_aboutCarla();
    void slot_aboutJuce();
    void slot_aboutQt();

    //-----------------------------------------------------------------------------------------------------------------
    // Disk (menu actions)

    void slot_diskFolderChanged(int index);
    void slot_diskFolderAdd();
    void slot_diskFolderRemove();
    void slot_fileTreeDoubleClicked(QModelIndex* modelIndex);

    //-----------------------------------------------------------------------------------------------------------------
    // Transport (menu actions)

    void slot_transportPlayPause(bool toggled);
    void slot_transportStop();
    void slot_transportBackwards();
    void slot_transportBpmChanged(qreal newValue);
    void slot_transportForwards();
    void slot_transportJackEnabled(bool clicked);
    void slot_transportLinkEnabled(bool clicked);

    //-----------------------------------------------------------------------------------------------------------------
    // Other

    void slot_xrunClear();

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas scrollbars

    void slot_horizontalScrollBarChanged(int value);
    void slot_verticalScrollBarChanged(int value);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard

    void slot_noteOn(int note);
    void slot_noteOff(int note);

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard (host callbacks)

    void slot_handleNoteOnCallback(int pluginId, int channel, int note, int velocity);
    void slot_handleNoteOffCallback(int pluginId, int channel, int note);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_handleUpdateCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------
    // MiniCanvas stuff

    void slot_miniCanvasCheckAll();
    void slot_miniCanvasCheckSize();
    void slot_miniCanvasMoved(qreal xp, qreal yp);

    //-----------------------------------------------------------------------------------------------------------------
    // Misc

    void slot_tabChanged(int index);
    void slot_handleReloadAllCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_handleNSMCallback(int opcode, int valueInt, QString valueStr);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_handleDebugCallback(int pluginId, int value1, int value2, int value3, float valuef, QString valueStr);
    void slot_handleInfoCallback(QString info);
    void slot_handleErrorCallback(QString error);
    void slot_handleQuitCallback();
    void slot_handleInlineDisplayRedrawCallback(int pluginId);

    //-----------------------------------------------------------------------------------------------------------------

    void slot_handleSIGUSR1();
    void slot_handleSIGTERM();

    //-----------------------------------------------------------------------------------------------------------------

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaHostWindow)
};

//---------------------------------------------------------------------------------------------------------------------
// Init host

CarlaHost& initHost(const QString initName, const bool isControl, const bool isPlugin, const bool failError);

//---------------------------------------------------------------------------------------------------------------------
// Load host settings

void loadHostSettings(CarlaHost& host);

//---------------------------------------------------------------------------------------------------------------------
// Set host settings

void setHostSettings(const CarlaHost& host);

//---------------------------------------------------------------------------------------------------------------------
// Set Engine settings according to carla preferences. Returns selected audio driver.

QString setEngineSettings(CarlaHost& host);

//---------------------------------------------------------------------------------------------------------------------
// Run Carla without showing UI

void runHostWithoutUI(CarlaHost& host);

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_HOST_HPP_INCLUDED
