/*
 * Carla settings code
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

#include "carla_settings.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtWidgets/QDialog>

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_app.hpp"
#include "carla_shared.hpp"
#include "resources_rc.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Driver Settings

class DriverSettingsW : public QDialog
{
    // static const char* const AUTOMATIC_OPTION = "(Auto)";

public:
    DriverSettingsW(QWidget* const parent = nullptr)
        : QDialog(parent)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Main

int main(int argc, char* argv[])
{
    // ----------------------------------------------------------------------------------------------------------------
    // Read CLI args

    const QString initName(handleInitialCommandLineArguments(argc, argv));

    // ----------------------------------------------------------------------------------------------------------------
    // App initialization

    const CarlaApplication app("Carla2-Settings", argc, argv);

    // ----------------------------------------------------------------------------------------------------------------
    // Init host backend

    /*
    Host& host = initHost(initName, false, false, true);
    loadHostSettings(host);
    */

    // ----------------------------------------------------------------------------------------------------------------
    // Create GUI

    DriverSettingsW gui;

    // ----------------------------------------------------------------------------------------------------------------
    // Show GUI

    gui.show();

    // ----------------------------------------------------------------------------------------------------------------
    // App-Loop

    return app.exec();
}
