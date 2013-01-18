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

#include "pixmapdial.hpp"

#include <cmath>

#include <QtCore/QTimer>
#include <QtGui/QPainter>

PixmapDial::PixmapDial(QWidget* parent)
    : QDial(parent)
{
    m_pixmap.load(":/bitmaps/dial_01d.png");
    m_pixmap_n_str = "01";
    m_custom_paint = CUSTOM_PAINT_NULL;

    m_hovered    = false;
    m_hover_step = HOVER_MIN;

    if (m_pixmap.width() > m_pixmap.height())
        m_orientation = HORIZONTAL;
    else
        m_orientation = VERTICAL;

    m_label = "";
    m_label_pos = QPointF(0.0f, 0.0f);
    m_label_width  = 0;
    m_label_height = 0;
    m_label_gradient = QLinearGradient(0, 0, 0, 1);

    if (palette().window().color().lightness() > 100)
    {
        // Light background
        QColor c = palette().dark().color();
        m_color1 = c;
        m_color2 = QColor(c.red(), c.green(), c.blue(), 0);
        m_colorT[0] = palette().buttonText().color();
        m_colorT[1] = palette().mid().color();
    }
    else
    {
        // Dark background
        m_color1 = QColor(0, 0, 0, 255);
        m_color2 = QColor(0, 0, 0, 0);
        m_colorT[0] = Qt::white;
        m_colorT[1] = Qt::darkGray;
    }

    updateSizes();
}

int PixmapDial::getSize() const
{
    return p_size;
}

void PixmapDial::setCustomPaint(CustomPaint paint)
{
    m_custom_paint = paint;
    update();
}

void PixmapDial::setEnabled(bool enabled)
{
    if (isEnabled() != enabled)
    {
        m_pixmap.load(QString(":/dial_%1%2.png").arg(m_pixmap_n_str).arg(enabled ? "" : "d"));
        updateSizes();
        update();
    }
    QDial::setEnabled(enabled);
}

void PixmapDial::setLabel(QString label)
{
    m_label = label;

    m_label_width  = QFontMetrics(font()).width(label);
    m_label_height = QFontMetrics(font()).height();

    m_label_pos.setX(float(p_size)/2 - float(m_label_width)/2);
    m_label_pos.setY(p_size + m_label_height);

    m_label_gradient.setColorAt(0.0f, m_color1);
    m_label_gradient.setColorAt(0.6f, m_color1);
    m_label_gradient.setColorAt(1.0f, m_color2);

    m_label_gradient.setStart(0, float(p_size)/2);
    m_label_gradient.setFinalStop(0, p_size+m_label_height+5);

    m_label_gradient_rect = QRectF(float(p_size)/8, float(p_size)/2, float(p_size*6)/8, p_size+m_label_height+5);
    update();
}

void PixmapDial::setPixmap(int pixmapId)
{
    m_pixmap_n_str.sprintf("%02i", pixmapId);
    m_pixmap.load(QString(":/bitmaps/dial_%1%2.png").arg(m_pixmap_n_str).arg(isEnabled() ? "" : "d"));

    if (m_pixmap.width() > m_pixmap.height())
        m_orientation = HORIZONTAL;
    else
        m_orientation = VERTICAL;

    updateSizes();
    update();
}

QSize PixmapDial::minimumSizeHint() const
{
    return QSize(p_size, p_size);
}

QSize PixmapDial::sizeHint() const
{
    return QSize(p_size, p_size);
}

void PixmapDial::updateSizes()
{
    p_width  = m_pixmap.width();
    p_height = m_pixmap.height();

    if (p_width < 1)
        p_width = 1;

    if (p_height < 1)
        p_height = 1;

    if (m_orientation == HORIZONTAL)
    {
        p_size  = p_height;
        p_count = p_width/p_height;
    }
    else
    {
        p_size  = p_width;
        p_count = p_height/p_width;
    }

    setMinimumSize(p_size, p_size + m_label_height + 5);
    setMaximumSize(p_size, p_size + m_label_height + 5);
}

void PixmapDial::enterEvent(QEvent* event)
{
    m_hovered = true;
    if (m_hover_step == HOVER_MIN)
        m_hover_step += 1;
    QDial::enterEvent(event);
}

void PixmapDial::leaveEvent(QEvent* event)
{
    m_hovered = false;
    if (m_hover_step == HOVER_MAX)
        m_hover_step -= 1;
    QDial::leaveEvent(event);
}

void PixmapDial::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    if (! m_label.isEmpty())
    {
        painter.setPen(m_color2);
        painter.setBrush(m_label_gradient);
        painter.drawRect(m_label_gradient_rect);

        painter.setPen(m_colorT[isEnabled() ? 0 : 1]);
        painter.drawText(m_label_pos, m_label);
    }

    if (isEnabled())
    {
        float current = value()-minimum();
        float divider = maximum()-minimum();

        if (divider == 0.0f)
            return;

        float value = current/divider;
        QRectF source, target(0.0f, 0.0f, p_size, p_size);

        int xpos, ypos, per = (p_count-1)*value;

        if (m_orientation == HORIZONTAL)
        {
            xpos = p_size*per;
            ypos = 0.0f;
        }
        else
        {
            xpos = 0.0f;
            ypos = p_size*per;
        }

        source = QRectF(xpos, ypos, p_size, p_size);
        painter.drawPixmap(target, m_pixmap, source);

        // Custom knobs (Dry/Wet and Volume)
        if (m_custom_paint == CUSTOM_PAINT_CARLA_WET || m_custom_paint == CUSTOM_PAINT_CARLA_VOL)
        {
            // knob color
            QColor colorGreen(0x5D, 0xE7, 0x3D, 191 + m_hover_step*7);
            QColor colorBlue(0x3E, 0xB8, 0xBE, 191 + m_hover_step*7);

            // draw small circle
            QRectF ballRect(8.0, 8.0, 15.0, 15.0);
            QPainterPath ballPath;
            ballPath.addEllipse(ballRect);
            //painter.drawRect(ballRect);
            float tmpValue  = (0.375f + 0.75f*value);
            float ballValue = tmpValue - floorf(tmpValue);
            QPointF ballPoint(ballPath.pointAtPercent(ballValue));

            // draw arc
            int startAngle = 216*16;
            int spanAngle  = -252*16*value;

            if (m_custom_paint == CUSTOM_PAINT_CARLA_WET)
            {
                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 0));
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2));

                QConicalGradient gradient(15.5, 15.5, -45);
                gradient.setColorAt(0.0,   colorBlue);
                gradient.setColorAt(0.125, colorBlue);
                gradient.setColorAt(0.625, colorGreen);
                gradient.setColorAt(0.75,  colorGreen);
                gradient.setColorAt(0.76,  colorGreen);
                gradient.setColorAt(1.0,   colorGreen);
                painter.setBrush(gradient);
                painter.setPen(QPen(gradient, 3));
            }
            else
            {
                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 0));
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2));

                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 3));
            }

            painter.drawArc(4.0, 4.0, 26.0, 26.0, startAngle, spanAngle);
        }
        // Custom knobs (L and R)
        else if (m_custom_paint == CUSTOM_PAINT_CARLA_L || m_custom_paint == CUSTOM_PAINT_CARLA_R)
        {
            // knob color
            QColor color(0xAD + m_hover_step*5, 0xD5 + m_hover_step*4, 0x4B + m_hover_step*5);

            // draw small circle
            QRectF ballRect(7.0, 8.0, 11.0, 12.0);
            QPainterPath ballPath;
            ballPath.addEllipse(ballRect);
            //painter.drawRect(ballRect);
            float tmpValue  = (0.375f + 0.75f*value);
            float ballValue = tmpValue - floorf(tmpValue);
            QPointF ballPoint(ballPath.pointAtPercent(ballValue));

            painter.setBrush(color);
            painter.setPen(QPen(color, 0));
            painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.0f, 2.0f));

            int startAngle, spanAngle;

            // draw arc
            if (m_custom_paint == CUSTOM_PAINT_CARLA_L)
            {
                startAngle = 216*16;
                spanAngle  = -252.0*16*value;
            }
            else if (m_custom_paint == CUSTOM_PAINT_CARLA_R)
            {
                startAngle = 324.0*16;
                spanAngle  = 252.0*16*(1.0-value);
            }
            else
                return;

            painter.setPen(QPen(color, 2));
            painter.drawArc(3.5, 4.5, 22.0, 22.0, startAngle, spanAngle);

            if (HOVER_MIN < m_hover_step && m_hover_step < HOVER_MAX)
            {
                m_hover_step += m_hovered ? 1 : -1;
                QTimer::singleShot(20, this, SLOT(update()));
            }
        }

        if (HOVER_MIN < m_hover_step && m_hover_step < HOVER_MAX)
        {
            m_hover_step += m_hovered ? 1 : -1;
            QTimer::singleShot(20, this, SLOT(update()));
        }
    }
    else
    {
        QRectF target(0.0, 0.0, p_size, p_size);
        painter.drawPixmap(target, m_pixmap, target);
    }
}

void PixmapDial::resizeEvent(QResizeEvent* event)
{
    updateSizes();
    QDial::resizeEvent(event);
}
