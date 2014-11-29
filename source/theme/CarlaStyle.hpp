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

#ifndef CARLA_STYLE_HPP_INCLUDED
#define CARLA_STYLE_HPP_INCLUDED

#include "CarlaDefines.h"

#include <QtCore/Qt>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QCommonStyle>
#else
# include <QtGui/QCommonStyle>
#endif

class CarlaStylePrivate;

class CarlaStyle : public QCommonStyle
{
    Q_OBJECT

public:
    CarlaStyle();
    ~CarlaStyle() override;

    QPalette standardPalette() const override;
    void drawPrimitive(PrimitiveElement elem, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget = nullptr) const override;
    void drawControl(ControlElement ce, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
                            const QWidget* widget) const override;
    QRect subElementRect(SubElement r, const QStyleOption* opt, const QWidget* widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const override;
    int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const override;
    void drawItemText(QPainter *painter, const QRect& rect, int flags, const QPalette& pal, bool enabled, const QString& text,
                      QPalette::ColorRole textRole = QPalette::NoRole) const override;
    void polish(QApplication* app) override;
    void polish(QPalette& pal) override;
    void polish(QWidget* widget) override;
    void unpolish(QApplication* app) override;
    void unpolish(QWidget* widget) override;

private:
    CarlaStylePrivate* const d;
    friend class CarlaStylePrivate;

    CARLA_DECLARE_NON_COPY_CLASS(CarlaStyle);
};

#endif // CARLA_STYLE_HPP_INCLUDED
