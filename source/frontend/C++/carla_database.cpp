/*
 * Carla plugin database code
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

#include "carla_database.hpp"

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
#include <QtCore/QFileInfo>

#include <QtWidgets/QPushButton>

//---------------------------------------------------------------------------------------------------------------------

#include "ui_carla_add_jack.hpp"
#include "ui_carla_database.hpp"
#include "ui_carla_refresh.hpp"

//---------------------------------------------------------------------------------------------------------------------

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_host.hpp"

#include "CarlaHost.h"
#include "CarlaLibJackHints.h"

// --------------------------------------------------------------------------------------------------------------------
// Separate Thread for Plugin Search

struct WineSettings {
    QString executable;
    bool autoPrefix;
    QString fallbackPrefix;

    WineSettings()
        : executable(),
          autoPrefix(false),
          fallbackPrefix()
    {
        const QSafeSettings settings("falkTX", "Carla2");

        executable = settings.valueString(CARLA_KEY_WINE_EXECUTABLE, CARLA_DEFAULT_WINE_EXECUTABLE);
        autoPrefix = settings.valueBool(CARLA_KEY_WINE_AUTO_PREFIX, CARLA_DEFAULT_WINE_AUTO_PREFIX);
        fallbackPrefix = settings.valueString(CARLA_KEY_WINE_FALLBACK_PREFIX, CARLA_DEFAULT_WINE_FALLBACK_PREFIX);
    }
};

struct SearchPluginsThread::PrivateData {
    bool fContinueChecking;
    QString fPathBinaries;

    bool fCheckNative;
    bool fCheckPosix32;
    bool fCheckPosix64;
    bool fCheckWin32;
    bool fCheckWin64;

    bool fCheckLADSPA;
    bool fCheckDSSI;
    bool fCheckLV2;
    bool fCheckVST2;
    bool fCheckVST3;
    bool fCheckAU;
    bool fCheckSF2;
    bool fCheckSFZ;

    WineSettings fWineSettings;

    QString fToolNative;

    uint fCurCount;
    uint fCurPercentValue;
    uint fLastCheckValue;
    bool fSomethingChanged;

    PrivateData(void*, const QString pathBinaries)
        : fContinueChecking(false),
          fPathBinaries(pathBinaries),
          fCheckNative(false),
          fCheckPosix32(false),
          fCheckPosix64(false),
          fCheckWin32(false),
          fCheckWin64(false),
          fCheckLADSPA(false),
          fCheckDSSI(false),
          fCheckLV2(false),
          fCheckVST2(false),
          fCheckVST3(false),
          fCheckAU(false),
          fCheckSF2(false),
          fCheckSFZ(false),
          fWineSettings(),
          fToolNative(),
          fCurCount(0),
          fCurPercentValue(0),
          fLastCheckValue(0),
          fSomethingChanged(false)
    {
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

SearchPluginsThread::SearchPluginsThread(QObject* parent, const QString pathBinaries)
    : QThread(parent),
      self(new PrivateData(this, pathBinaries))
{
}

SearchPluginsThread::~SearchPluginsThread()
{
    delete self;
}

void SearchPluginsThread::run()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin Refresh Dialog

struct PluginRefreshW::PrivateData {
    Ui::PluginRefreshW ui;

    SearchPluginsThread fThread;
    QPixmap fIconYes;
    QPixmap fIconNo;

    PrivateData(PluginRefreshW* const refreshDialog, const CarlaHost& host)
        : ui(),
          fThread(refreshDialog, host.pathBinaries),
          fIconYes(":/16x16/dialog-ok-apply.svgz"),
          fIconNo(":/16x16/dialog-error.svgz")
    {
        ui.setupUi(refreshDialog);

        // ------------------------------------------------------------------------------------------------------------
        // Internal stuff

        const bool hasNative  = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-native");
        const bool hasPosix32 = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-posix32");
        const bool hasPosix64 = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-posix64");
        const bool hasWin32   = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-win32.exe");
        const bool hasWin64   = QFileInfo::exists(host.pathBinaries + CARLA_OS_SEP_STR "carla-discovery-win64.exe");
    }

    void loadSettings()
    {
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

PluginRefreshW::PluginRefreshW(QWidget* const parent, const CarlaHost& host)
    : QDialog(parent),
      self(new PrivateData(this, host))
{
    // ----------------------------------------------------------------------------------------------------------------
    // Resize to minimum size, as it's very likely UI stuff was hidden

    resize(minimumSize());

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, SIGNAL(finished(int)), SLOT(slot_saveSettings()));
    connect(self->ui.b_start, SIGNAL(clicked()), SLOT(slot_start()));
    connect(self->ui.b_skip, SIGNAL(clicked()), SLOT(slot_skip()));
    connect(self->ui.ch_native, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_posix32, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_posix64, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_win32, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_win64, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_ladspa, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_dssi, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_lv2, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_vst, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_vst3, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_au, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_sf2, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_sfz, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(self->ui.ch_jsfx, SIGNAL(clicked()), SLOT(slot_checkTools()));
    connect(&self->fThread, SIGNAL(pluginLook(float, QString)), SLOT(slot_handlePluginLook(float, QString)));
    connect(&self->fThread, SIGNAL(finished(int)), SLOT(slot_handlePluginThreadFinished()));

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    slot_checkTools();
}

PluginRefreshW::~PluginRefreshW()
{
    delete self;
}

void PluginRefreshW::getValues(QString& audioDevice, uint& bufferSize, double& sampleRate)
{
}

void PluginRefreshW::closeEvent(QCloseEvent* event)
{
}

void PluginRefreshW::slot_saveSettings()
{
}

void PluginRefreshW::slot_start()
{
}

void PluginRefreshW::slot_skip()
{
}

void PluginRefreshW::slot_checkTools()
{
}

void PluginRefreshW::slot_handlePluginLook(float percent, QString plugin)
{
}

void PluginRefreshW::slot_handlePluginThreadFinished()
{
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin Database Dialog

struct PluginDatabaseW::PrivateData {
    PrivateData(void*)
    {
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

PluginDatabaseW::PluginDatabaseW(QWidget* parent, const CarlaHost& host, bool hasCanvas, bool hasCanvasGL)
    : QDialog(parent),
      self(new PrivateData(this))
{
}

PluginDatabaseW::~PluginDatabaseW()
{
    delete self;
}

void PluginDatabaseW::showEvent(QShowEvent* event)
{
}

void PluginDatabaseW::slot_cellClicked(int row, int column)
{
}

void PluginDatabaseW::slot_cellDoubleClicked(int row, int column)
{
}

void PluginDatabaseW::slot_addPlugin()
{
}

void PluginDatabaseW::slot_checkPlugin(int row)
{
}

void PluginDatabaseW::slot_checkFilters()
{
}

void PluginDatabaseW::slot_refreshPlugins()
{
}

void PluginDatabaseW::slot_clearFilters()
{
}

void PluginDatabaseW::slot_saveSettings()
{
}

// --------------------------------------------------------------------------------------------------------------------
