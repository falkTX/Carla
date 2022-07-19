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

#include "qsafesettings.hpp"

//---------------------------------------------------------------------------------------------------------------------

bool QSafeSettings::valueBool(const QString& key, const bool defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::Bool) && var.isValid())
        return var.toBool();
    
    return defaultValue;    
}

Qt::CheckState QSafeSettings::valueCheckState(const QString& key, const Qt::CheckState defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::UInt) && var.isValid())
    {
        const uint value = var.toUInt();

        switch (value)
        {
        case Qt::Unchecked:
        case Qt::PartiallyChecked:
        case Qt::Checked:
            return static_cast<Qt::CheckState>(value);
        }
    }

    return defaultValue;
}

int QSafeSettings::valueIntPositive(const QString& key, const int defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::Int) && var.isValid())
        return defaultValue;

    const int value = var.toInt();
    return value >= 0 ? value : defaultValue;
}

uint QSafeSettings::valueUInt(const QString& key, const uint defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::UInt) && var.isValid())
        return var.toUInt();

    return defaultValue;
}

double QSafeSettings::valueDouble(const QString& key, const double defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::Double) && var.isValid())
        return var.toDouble();

    return defaultValue;
}

QString QSafeSettings::valueString(const QString& key, const QString& defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::String) && var.isValid())
        return var.toString();

    return defaultValue;
}

QByteArray QSafeSettings::valueByteArray(const QString& key, const QByteArray defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::ByteArray) && var.isValid())
        return var.toByteArray();

    return defaultValue;
}

QStringList QSafeSettings::valueStringList(const QString& key, const QStringList defaultValue) const
{
    QVariant var(value(key, defaultValue));

    if (!var.isNull() && var.convert(QVariant::StringList) && var.isValid())
        return var.toStringList();

    return defaultValue;
}

//---------------------------------------------------------------------------------------------------------------------
