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

#include <QtCore/QSettings>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "CarlaDefines.h"

//---------------------------------------------------------------------------------------------------------------------
// Safer QSettings class, which does not throw if type mismatches

class QSafeSettings : public QSettings
{
public:
    inline QSafeSettings()
        : QSettings() {}

    inline QSafeSettings(const QString& organization, const QString& application)
        : QSettings(organization, application) {}

    bool valueBool(const QString& key, bool defaultValue) const;
    Qt::CheckState valueCheckState(const QString& key, Qt::CheckState defaultValue) const;
    int valueIntPositive(const QString& key, int defaultValue) const;
    uint valueUInt(const QString& key, uint defaultValue) const;
    double valueDouble(const QString& key, double defaultValue) const;
    QString valueString(const QString& key, const QString& defaultValue) const;
    QByteArray valueByteArray(const QString& key, QByteArray defaultValue = {}) const;
    QStringList valueStringList(const QString& key, QStringList defaultValue = {}) const;
};

//---------------------------------------------------------------------------------------------------------------------
