/*
 * Pixmap Button, a custom Qt4 widget
 * Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#include "ledbutton.hpp"

#include <QtGui/QPainter>

LEDButton::LEDButton(QWidget* parent):
    QPushButton(parent)
{
    m_pixmap_rect = QRectF(0, 0, 0, 0);

    setCheckable(true);
    setText("");

    setColor(BLUE);
}

LEDButton::~LEDButton()
{
}

void LEDButton::setColor(Color color)
{
    m_color = color;

    int size;
    if (1) //color in (self.BLUE, self.GREEN, self.RED, self.YELLOW):
        size = 14;
    else if (color == BIG_RED)
        size = 64;
    else
        return qCritical("LEDButton::setColor(%i) - Invalid color", color);

    setPixmapSize(size);
}

void LEDButton::setPixmapSize(int size)
{
    m_pixmap_rect = QRectF(0, 0, size, size);

    setMinimumWidth(size);
    setMaximumWidth(size);
    setMinimumHeight(size);
    setMaximumHeight(size);
}

void LEDButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    if (isChecked())
    {
        if (m_color == BLUE)
            m_pixmap.load(":/bitmaps/led_blue.png");
        else if (m_color == GREEN)
            m_pixmap.load(":/bitmaps/led_green.png");
        else if (m_color == RED)
            m_pixmap.load(":/bitmaps/led_red.png");
        else if (m_color == YELLOW)
            m_pixmap.load(":/bitmaps/led_yellow.png");
        else if (m_color == BIG_RED)
            m_pixmap.load(":/bitmaps/led-big_on.png");
        else
            return;
    }
    else
    {
        if (1) //self.m_color in (self.BLUE, self.GREEN, self.RED, self.YELLOW):
            m_pixmap.load(":/bitmaps/led_off.png");
        else if (m_color == BIG_RED)
            m_pixmap.load(":/bitmaps/led-big_off.png");
        else
            return;
    }

    painter.drawPixmap(m_pixmap_rect, m_pixmap, m_pixmap_rect);
}
