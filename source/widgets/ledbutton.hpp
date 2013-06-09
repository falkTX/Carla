/*
 * LED Button, a custom Qt4 widget
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __LEDBUTTON_HPP__
#define __LEDBUTTON_HPP__

#include "CarlaJuceUtils.hpp"

#include <QtGui/QPixmap>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QPushButton>
#else
# include <QtGui/QPushButton>
#endif

class LEDButton : public QPushButton
{
    Q_OBJECT

public:
    enum Color {
        OFF    = 0,
        BLUE   = 1,
        GREEN  = 2,
        RED    = 3,
        YELLOW = 4
    };

    LEDButton(QWidget* parent);

    void setColor(Color color);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Color   fColor;
    Color   fLastColor;
    QPixmap fPixmap;
    QRectF  fPixmapRect;

    CARLA_LEAK_DETECTOR(LEDButton)
};

#endif // __LEDBUTTON_HPP__
