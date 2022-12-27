/*
 * Carla plugin host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtWidgets/QDialog>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "CarlaDefines.h"

static constexpr const uint PLUGIN_QUERY_API_VERSION = 12;

struct HostSettings {
    bool showPluginBridges;
    bool showWineBridges;
    bool useSystemIcons;
};

struct PluginInfo {
    uint API;
    uint build;
    uint type;
    uint hints;
    QString category;
    QString filename;
    QString name;
    QString label;
    QString maker;
    uint64_t uniqueId;
    uint audioIns;
    uint audioOuts;
    uint cvIns;
    uint cvOuts;
    uint midiIns;
    uint midiOuts;
    uint parametersIns;
    uint parametersOuts;
};

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

class PluginListDialog : public QDialog
{
    struct Self;
    Self& self;

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit PluginListDialog(QWidget* parent, const HostSettings& hostSettings);
    ~PluginListDialog() override;

    // ----------------------------------------------------------------------------------------------------------------
    // public methods

    // ----------------------------------------------------------------------------------------------------------------
    // protected methods

protected:
    void showEvent(QShowEvent*) override;

    // ----------------------------------------------------------------------------------------------------------------
    // private methods

    void loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // private slots

private slots:
    void slot_cellClicked(int row, int column);
    void slot_cellDoubleClicked(int row, int column);
    void slot_focusSearchFieldAndSelectAll();
    void slot_addPlugin();
    void slot_checkPlugin(int row);
    void slot_checkFilters();
    void slot_checkFiltersCategoryAll(bool clicked);
    void slot_checkFiltersCategorySpecific(bool clicked);
    void slot_refreshPlugins();
    void slot_clearFilters();
    void slot_saveSettings();
};

// --------------------------------------------------------------------------------------------------------------------

extern "C" {

struct PluginListDialogResults {
    uint API;
    uint build;
    uint type;
    uint hints;
    const char* category;
    const char* filename;
    const char* name;
    const char* label;
    const char* maker;
    uint64_t uniqueId;
    uint audioIns;
    uint audioOuts;
    uint cvIns;
    uint cvOuts;
    uint midiIns;
    uint midiOuts;
    uint parametersIns;
    uint parametersOuts;
};

CARLA_API
PluginListDialogResults* carla_frontend_createAndExecPluginListDialog(void* parent/*, const HostSettings& hostSettings*/);

}

// --------------------------------------------------------------------------------------------------------------------
