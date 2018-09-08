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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "ledbutton.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

LEDButton::LEDButton(QWidget* parent):
    QPushButton(parent),
    fColor(OFF),
    fLastColor(OFF),
    fPixmapRect(0, 0, 0, 0)
{
    setCheckable(true);
    setText("");

    setColor(BLUE);

    // matching fLastColor
    fPixmap.load(":/bitmaps/led_off.png");
}

void LEDButton::setColor(Color color)
{
    fColor = color;
    int size = 14;

    fPixmapRect = QRectF(0, 0, size, size);

    setMinimumSize(size, size);
    setMaximumSize(size, size);
}

void LEDButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    event->accept();

    if (isChecked())
    {
        if (fLastColor != fColor)
        {
            switch (fColor)
            {
            case OFF:
                fPixmap.load(":/bitmaps/led_off.png");
                break;
            case BLUE:
                fPixmap.load(":/bitmaps/led_blue.png");
                break;
            case GREEN:
                fPixmap.load(":/bitmaps/led_green.png");
                break;
            case RED:
                fPixmap.load(":/bitmaps/led_red.png");
                break;
            case YELLOW:
                fPixmap.load(":/bitmaps/led_yellow.png");
                break;
            default:
                return;
            }

            fLastColor = fColor;
        }
    }
    else if (fLastColor != OFF)
    {
        fPixmap.load(":/bitmaps/led_off.png");
        fLastColor = OFF;
    }

    painter.drawPixmap(fPixmapRect, fPixmap, fPixmapRect);
}
