/*
 * Carla application
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_APP_HPP_INCLUDED
#define CARLA_APP_HPP_INCLUDED

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------

class QApplication;

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "CarlaJuceUtils.hpp"

//---------------------------------------------------------------------------------------------------------------------

class CarlaApplication
{
public:
    CarlaApplication(const QString appName, int& argc, char* argv[]);

    int exec() const
    {
        return fApp->exec();
    }

    void quit() const
    {
        fApp->quit();
    }

    QCoreApplication* getApp() const noexcept
    {
        return fApp;
    }

private:
    QCoreApplication* fApp;

    QApplication* createApp(const QString& appName, int& argc, char* argv[]);

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaApplication)
};

//---------------------------------------------------------------------------------------------------------------------

#endif // CARLA_APP_HPP_INCLUDED
