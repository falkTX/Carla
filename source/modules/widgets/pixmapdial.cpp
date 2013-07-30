/*
 * Pixmap Dial, a custom Qt4 widget
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

#include "pixmapdial.hpp"

#include <cmath>

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

PixmapDial::PixmapDial(QWidget* parent)
    : QDial(parent),
      fPixmap(":/bitmaps/dial_01d.png"),
      fPixmapNum("01"),
      fCustomPaint(CUSTOM_PAINT_NULL),
      fOrientation(fPixmap.width() > fPixmap.height() ? HORIZONTAL : VERTICAL),
      fHovered(false),
      fHoverStep(HOVER_MIN),
      fLabel(""),
      fLabelPos(0.0f, 0.0f),
      fLabelWidth(0),
      fLabelHeight(0),
      fLabelGradient(0, 0, 0, 1)
{
    fLabelFont.setPointSize(6);

    if (palette().window().color().lightness() > 100)
    {
        // Light background
        const QColor c(palette().dark().color());
        fColor1    = c;
        fColor2    = QColor(c.red(), c.green(), c.blue(), 0);
        fColorT[0] = palette().buttonText().color();
        fColorT[1] = palette().mid().color();
    }
    else
    {
        // Dark background
        fColor1    = QColor(0, 0, 0, 255);
        fColor2    = QColor(0, 0, 0, 0);
        fColorT[0] = Qt::white;
        fColorT[1] = Qt::darkGray;
    }

    updateSizes();
}

int PixmapDial::getSize() const
{
    return fSize;
}

void PixmapDial::setCustomPaint(CustomPaint paint)
{
    fCustomPaint = paint;
    fLabelPos.setY(fSize + fLabelHeight/2);
    update();
}

void PixmapDial::setEnabled(bool enabled)
{
    if (isEnabled() != enabled)
    {
        fPixmap.load(QString(":/bitmaps/dial_%1%2.png").arg(fPixmapNum).arg(enabled ? "" : "d"));
        updateSizes();
        update();
    }
    QDial::setEnabled(enabled);
}

void PixmapDial::setLabel(QString label)
{
    fLabel = label;

    fLabelWidth  = QFontMetrics(font()).width(label);
    fLabelHeight = QFontMetrics(font()).height();

    fLabelPos.setX(float(fSize)/2.0f - float(fLabelWidth)/2.0f);
    fLabelPos.setY(fSize + fLabelHeight);

    fLabelGradient.setColorAt(0.0f, fColor1);
    fLabelGradient.setColorAt(0.6f, fColor1);
    fLabelGradient.setColorAt(1.0f, fColor2);

    fLabelGradient.setStart(0, float(fSize)/2.0f);
    fLabelGradient.setFinalStop(0, fSize + fLabelHeight + 5);

    fLabelGradientRect = QRectF(float(fSize)/8.0f, float(fSize)/2.0f, float(fSize*6)/8.0f, fSize+fLabelHeight+5);
    update();
}

void PixmapDial::setPixmap(int pixmapId)
{
    fPixmapNum.sprintf("%02i", pixmapId);
    fPixmap.load(QString(":/bitmaps/dial_%1%2.png").arg(fPixmapNum).arg(isEnabled() ? "" : "d"));

    if (fPixmap.width() > fPixmap.height())
        fOrientation = HORIZONTAL;
    else
        fOrientation = VERTICAL;

    updateSizes();
    update();
}

QSize PixmapDial::minimumSizeHint() const
{
    return QSize(fSize, fSize);
}

QSize PixmapDial::sizeHint() const
{
    return QSize(fSize, fSize);
}

void PixmapDial::updateSizes()
{
    fWidth  = fPixmap.width();
    fHeight = fPixmap.height();

    if (fWidth < 1)
        fWidth = 1;

    if (fHeight < 1)
        fHeight = 1;

    if (fOrientation == HORIZONTAL)
    {
        fSize  = fHeight;
        fCount = fWidth/fHeight;
    }
    else
    {
        fSize  = fWidth;
        fCount = fHeight/fWidth;
    }

    setMinimumSize(fSize, fSize + fLabelHeight + 5);
    setMaximumSize(fSize, fSize + fLabelHeight + 5);
}

void PixmapDial::enterEvent(QEvent* event)
{
    fHovered = true;
    if (fHoverStep == HOVER_MIN)
        fHoverStep = HOVER_MIN + 1;
    QDial::enterEvent(event);
}

void PixmapDial::leaveEvent(QEvent* event)
{
    fHovered = false;
    if (fHoverStep == HOVER_MAX)
        fHoverStep = HOVER_MAX - 1;
    QDial::leaveEvent(event);
}

void PixmapDial::paintEvent(QPaintEvent* event)
{
    event->accept();

    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (! fLabel.isEmpty())
    {
        if (fCustomPaint == CUSTOM_PAINT_NULL)
        {
            painter.setPen(fColor2);
            painter.setBrush(fLabelGradient);
            painter.drawRect(fLabelGradientRect);
        }

        painter.setFont(fLabelFont);
        painter.setPen(fColorT[isEnabled() ? 0 : 1]);
        painter.drawText(fLabelPos, fLabel);
    }

    if (isEnabled())
    {
        float current = value()-minimum();
        float divider = maximum()-minimum();

        if (divider == 0.0f)
            return;

        float value = current/divider;
        QRectF source, target(0.0f, 0.0f, fSize, fSize);

        int xpos, ypos, per = (fCount-1)*value;

        if (fOrientation == HORIZONTAL)
        {
            xpos = fSize*per;
            ypos = 0.0f;
        }
        else
        {
            xpos = 0.0f;
            ypos = fSize*per;
        }

        source = QRectF(xpos, ypos, fSize, fSize);
        painter.drawPixmap(target, fPixmap, source);

        // Custom knobs (Dry/Wet and Volume)
        if (fCustomPaint == CUSTOM_PAINT_CARLA_WET || fCustomPaint == CUSTOM_PAINT_CARLA_VOL)
        {
            // knob color
            QColor colorGreen(0x5D, 0xE7, 0x3D, 191 + fHoverStep*7);
            QColor colorBlue(0x3E, 0xB8, 0xBE, 191 + fHoverStep*7);

            // draw small circle
            QRectF ballRect(8.0f, 8.0f, 15.0f, 15.0f);
            QPainterPath ballPath;
            ballPath.addEllipse(ballRect);
            //painter.drawRect(ballRect);
            float tmpValue  = (0.375f + 0.75f*value);
            float ballValue = tmpValue - std::floor(tmpValue);
            QPointF ballPoint(ballPath.pointAtPercent(ballValue));

            // draw arc
            int startAngle = 216*16;
            int spanAngle  = -252*16*value;

            if (fCustomPaint == CUSTOM_PAINT_CARLA_WET)
            {
                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 0));
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2f, 2.2f));

                QConicalGradient gradient(15.5f, 15.5f, -45);
                gradient.setColorAt(0.0f,   colorBlue);
                gradient.setColorAt(0.125f, colorBlue);
                gradient.setColorAt(0.625f, colorGreen);
                gradient.setColorAt(0.75f,  colorGreen);
                gradient.setColorAt(0.76f,  colorGreen);
                gradient.setColorAt(1.0f,   colorGreen);
                painter.setBrush(gradient);
                painter.setPen(QPen(gradient, 3));
            }
            else
            {
                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 0));
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2f, 2.2f));

                painter.setBrush(colorBlue);
                painter.setPen(QPen(colorBlue, 3));
            }

            painter.drawArc(4.0f, 4.0f, 26.0f, 26.0f, startAngle, spanAngle);
        }
        // Custom knobs (L and R)
        else if (fCustomPaint == CUSTOM_PAINT_CARLA_L || fCustomPaint == CUSTOM_PAINT_CARLA_R)
        {
            // knob color
            QColor color(0xAD + fHoverStep*5, 0xD5 + fHoverStep*4, 0x4B + fHoverStep*5);

            // draw small circle
            QRectF ballRect(7.0f, 8.0f, 11.0f, 12.0f);
            QPainterPath ballPath;
            ballPath.addEllipse(ballRect);
            //painter.drawRect(ballRect);
            float tmpValue  = (0.375f + 0.75f*value);
            float ballValue = tmpValue - std::floor(tmpValue);
            QPointF ballPoint(ballPath.pointAtPercent(ballValue));

            painter.setBrush(color);
            painter.setPen(QPen(color, 0));
            painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.0f, 2.0f));

            int startAngle, spanAngle;

            // draw arc
            if (fCustomPaint == CUSTOM_PAINT_CARLA_L)
            {
                startAngle = 216*16;
                spanAngle  = -252.0*16*value;
            }
            else if (fCustomPaint == CUSTOM_PAINT_CARLA_R)
            {
                startAngle = 324.0*16;
                spanAngle  = 252.0*16*(1.0-value);
            }
            else
                return;

            painter.setPen(QPen(color, 2));
            painter.drawArc(3.5f, 4.5f, 22.0f, 22.0f, startAngle, spanAngle);

            if (HOVER_MIN < fHoverStep && fHoverStep < HOVER_MAX)
            {
                fHoverStep += fHovered ? 1 : -1;
                QTimer::singleShot(20, this, SLOT(update()));
            }
        }

        if (HOVER_MIN < fHoverStep && fHoverStep < HOVER_MAX)
        {
            fHoverStep += fHovered ? 1 : -1;
            QTimer::singleShot(20, this, SLOT(update()));
        }
    }
    else
    {
        QRectF target(0.0f, 0.0f, fSize, fSize);
        painter.drawPixmap(target, fPixmap, target);
    }

    painter.restore();
}

void PixmapDial::resizeEvent(QResizeEvent* event)
{
    updateSizes();
    QDial::resizeEvent(event);
}
