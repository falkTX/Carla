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

#include "qcarlastring.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

class PluginListRefreshDialog : public QDialog
{
    struct Self;
    Self& self;

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit PluginListRefreshDialog(QWidget* parent, bool useSystemIcons);
    ~PluginListRefreshDialog() override;

    // ----------------------------------------------------------------------------------------------------------------
    // public methods

    // ----------------------------------------------------------------------------------------------------------------
    // protected methods

protected:
    void closeEvent(QCloseEvent*) override;

    // ----------------------------------------------------------------------------------------------------------------
    // private methods

private:
    void loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // private slots

private slots:
    void slot_saveSettings();
    void slot_start();
    void slot_skip();
    void slot_checkTools();
    void slot_handlePluginLook(int percent, QString plugin);
    void slot_handlePluginThreadFinished();
};

// --------------------------------------------------------------------------------------------------------------------

extern "C" {

struct PluginListRefreshDialogResults {
    char todo;
};

CARLA_API
PluginListRefreshDialogResults* carla_frontend_createAndExecPluginListRefreshDialog(void* parent, bool useSystemIcons);

}

// --------------------------------------------------------------------------------------------------------------------
