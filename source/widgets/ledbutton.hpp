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

#ifndef LEDBUTTON_HPP
#define LEDBUTTON_HPP

#include <QtGui/QPixmap>
#include <QtGui/QPushButton>

class LEDButton : public QPushButton
{
    Q_OBJECT

public:
    enum Color {
        BLUE    = 1,
        GREEN   = 2,
        RED     = 3,
        YELLOW  = 4,
        BIG_RED = 5
    };

    LEDButton(QWidget* parent);
    ~LEDButton();

    void setColor(Color color);

protected:
    void setPixmapSize(int size);

    void paintEvent(QPaintEvent* event);

private:
    QPixmap m_pixmap;
    QRectF  m_pixmap_rect;

    Color m_color;
};

#endif // LEDBUTTON_HPP
