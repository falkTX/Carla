/*
 * Carla Style, based on Qt5 fusion style
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL3.txt file
 */

#ifndef __CARLA_STYLE_HPP__
#define __CARLA_STYLE_HPP__

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
    ~CarlaStyle();

    enum ColorScheme {
        COLOR_BLACK  = 0,
        COLOR_BLUE   = 1,
        COLOR_SYSTEM = 2
    };

    void ready(QApplication* app);
    void setColorScheme(ColorScheme color);

    QPalette standardPalette() const;
    void drawPrimitive(PrimitiveElement elem,
                       const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = 0) const;
    void drawControl(ControlElement ce, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = 0) const;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const;
    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0,
                  QStyleHintReturn *returnData = 0) const;
    void drawItemText(QPainter *painter, const QRect &rect,
                      int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const;
    void polish(QWidget *widget);
    void unpolish(QWidget *widget);

private:
    QPalette fPalBlack;
    QPalette fPalBlue;
    QPalette fPalSystem;

    CarlaStylePrivate* const d;
    friend class CarlaStylePrivate;
};

#endif // __CARLA_STYLE_HPP__
