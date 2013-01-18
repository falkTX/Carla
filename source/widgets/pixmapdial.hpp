/*
 * Pixmap Dial, a custom Qt4 widget
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

#ifndef PIXMAPDIAL_HPP
#define PIXMAPDIAL_HPP

#include <QtGui/QDial>
#include <QtGui/QPixmap>

class PixmapDial : public QDial
{
public:
    enum CustomPaint {
        CUSTOM_PAINT_NULL      = 0,
        CUSTOM_PAINT_CARLA_WET = 1,
        CUSTOM_PAINT_CARLA_VOL = 2,
        CUSTOM_PAINT_CARLA_L   = 3,
        CUSTOM_PAINT_CARLA_R   = 4
    };

    PixmapDial(QWidget* parent);

    int  getSize() const;
    void setCustomPaint(CustomPaint paint);
    void setEnabled(bool enabled);
    void setLabel(QString label);
    void setPixmap(int pixmapId);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    void updateSizes();

    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    enum Orientation {
        HORIZONTAL = 0,
        VERTICAL   = 1
    };

    static const unsigned short HOVER_MIN = 0;
    static const unsigned short HOVER_MAX = 9;

    // -------------------------------------

    QPixmap m_pixmap;
    QString m_pixmap_n_str;

    CustomPaint m_custom_paint;
    Orientation m_orientation;

    bool m_hovered;
    unsigned short m_hover_step;

    QString m_label;
    QPointF m_label_pos;
    int     m_label_width;
    int     m_label_height;

    QLinearGradient m_label_gradient;
    QRectF m_label_gradient_rect;

    QColor m_color1;
    QColor m_color2;
    QColor m_colorT[2];

    int p_width, p_height, p_size, p_count;
};

#endif // PIXMAPDIAL_HPP
