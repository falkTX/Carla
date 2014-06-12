/*
 * Carla style
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef FRONTEND_CARLA_STYLE_HPP_INCLUDED
#define FRONTEND_CARLA_STYLE_HPP_INCLUDED

// ------------------------------------------------------------------------------------------------------------
// Imports (Global)

#include <QtGui/QPalette>

class QApplication;
class CarlaStyle;

// ------------------------------------------------------------------------------------------------------------

class CarlaApplication
{
public:
    CarlaApplication(int& argc, char* argv[], const QString& appName = "Carla2");
    ~CarlaApplication();

private:
    QApplication* fApp;
    CarlaStyle* fStyle;

    QPalette fPalSystem;
    QPalette fPalBlack;
    QPalette fPalBlue;

    void loadSettings();
};

// ------------------------------------------------------------------------------------------------------------

#endif // FRONTEND_CARLA_STYLE_HPP_INCLUDED
