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

struct HostSettings {
    bool showPluginBridges;
    bool showWineBridges;
    bool useSystemIcons;
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
    // private methods

    void loadSettings();
    void reAddPlugins();

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
    int btype;
    int ptype;
    const char* binary;
    const char* label;
    // TODO
};

CARLA_API
PluginListDialogResults* carla_frontend_createAndExecPluginListDialog(void* parent/*, const HostSettings& hostSettings*/);

}

// --------------------------------------------------------------------------------------------------------------------
