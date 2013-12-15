/*
 * Carla Style, based on Qt5 fusion style
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_STYLE_PLUGIN_HPP_INCLUDED
#define CARLA_STYLE_PLUGIN_HPP_INCLUDED

#include "CarlaDefines.h"

#include <QtCore/Qt>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QStylePlugin>
#else
# include <QtGui/QStylePlugin>
#endif

class CarlaStylePlugin : public QStylePlugin
{
    Q_OBJECT
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "carlastyle.json")
#endif

public:
    CarlaStylePlugin(QObject* parent = nullptr);
    QStyle* create(const QString& key) override;
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    QStringList keys() const override;
#endif
};

#endif // CARLA_STYLE_PLUGIN_HPP_INCLUDED
