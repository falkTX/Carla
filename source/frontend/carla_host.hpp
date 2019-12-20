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
    // Setup

    void compactPlugin(uint pluginId);
    void changePluginColor(uint pluginId, QString color, QString colorStr);
    void changePluginSkin(uint pluginId, QString skin);
    void switchPlugins(uint pluginIdA, uint pluginIdB);
    void setLoadRDFsNeeded() noexcept;
    void setProperWindowTitle();
    void updateBufferSize(uint newBufferSize);
    void updateSampleRate(double newSampleRate);

    //-----------------------------------------------------------------------------------------------------------------
    // Files

    //-----------------------------------------------------------------------------------------------------------------
    // Files (menu actions)

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (menu actions)

    Q_SLOT void slot_engineStart();
    Q_SLOT bool slot_engineStop(bool forced = false);
    Q_SLOT void slot_engineConfig();
    Q_SLOT bool slot_engineStopTryAgain();
    void engineStopFinal();

    //-----------------------------------------------------------------------------------------------------------------
    // Engine (host callbacks)

    Q_SLOT void slot_handleEngineStartedCallback(uint pluginCount, int processMode, int transportMode, int bufferSize, float sampleRate, QString driverName);
    Q_SLOT void slot_handleEngineStoppedCallback();

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (menu actions)

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (macros)

    //-----------------------------------------------------------------------------------------------------------------
    // Plugins (host callbacks)

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (menu actions)

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (canvas callbacks)

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas (host callbacks)

    //-----------------------------------------------------------------------------------------------------------------
    // Settings

    void saveSettings();
    void loadSettings(bool firstTime);

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (helpers)

    //-----------------------------------------------------------------------------------------------------------------
    // Settings (menu actions)

    Q_SLOT void slot_showSidePanel(bool yesNo);
    Q_SLOT void slot_showToolbar(bool yesNo);
    Q_SLOT void slot_showCanvasMeters(bool yesNo);
    Q_SLOT void slot_showCanvasKeyboard(bool yesNo);
    Q_SLOT void slot_configureCarla();

    //-----------------------------------------------------------------------------------------------------------------
    // About (menu actions)

    //-----------------------------------------------------------------------------------------------------------------
    // Disk (menu actions)

    //-----------------------------------------------------------------------------------------------------------------
    // Transport

    void refreshTransport(bool forced = false);

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

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard

    //-----------------------------------------------------------------------------------------------------------------
    // Canvas keyboard (host callbacks)

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------
    // MiniCanvas stuff

    //-----------------------------------------------------------------------------------------------------------------
    // Timers

    void startTimers();
    void restartTimersIfNeeded();
    void killTimers();

    //-----------------------------------------------------------------------------------------------------------------
    // Misc

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------
    // Internal stuff

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------
    // show/hide event

    //-----------------------------------------------------------------------------------------------------------------
    // resize event

    //-----------------------------------------------------------------------------------------------------------------
    // timer event

    void refreshRuntimeInfo(float load, int xruns);
    void getAndRefreshRuntimeInfo();
    void idleFast();
    void idleSlow();
    void timerEvent(QTimerEvent* event) override;

    //-----------------------------------------------------------------------------------------------------------------
    // color/style change event

    void changeEvent(QEvent* event) override;
    void updateStyle();

    //-----------------------------------------------------------------------------------------------------------------
    // close event

    bool shouldIgnoreClose();
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
