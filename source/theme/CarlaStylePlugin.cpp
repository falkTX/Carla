/*
 * Carla Style, based on Qt5 fusion style
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the doc/LGPL.txt file
 */

#include "CarlaStylePlugin.hpp"
#include "CarlaStyle.hpp"

CarlaStylePlugin::CarlaStylePlugin(QObject* parentObj)
    : QStylePlugin(parentObj) {}

QStyle* CarlaStylePlugin::create(const QString& key)
{
    return (key.toLower() == "carla") ? new CarlaStyle() : nullptr;
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
QStringList CarlaStylePlugin::keys() const
{
    return QStringList() << "Carla";
}

Q_EXPORT_PLUGIN2(Carla, CarlaStylePlugin)
#endif
