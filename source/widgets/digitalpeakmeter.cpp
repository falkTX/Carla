/*
 * Digital Peak Meter, a custom Qt4 widget
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

#include "digitalpeakmeter.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

DigitalPeakMeter::DigitalPeakMeter(QWidget* parent)
    : QWidget(parent),
      fChannels(0),
      fSmoothMultiplier(1),
      fWidth(0),
      fHeight(0),
      fSizeMeter(0),
      fOrientation(VERTICAL),
      fColorBackground("#111111"),
      fGradientMeter(0, 0, 1, 1),
      fColorBase(93, 231, 61),
      fColorBaseAlt(15, 110, 15, 100),
      fChannelsData(nullptr),
      fLastValueData(nullptr)
{
    setChannels(0);
    setColor(GREEN);
}

DigitalPeakMeter::~DigitalPeakMeter()
{
    if (fChannelsData != nullptr)
        delete[] fChannelsData;
    if (fLastValueData != nullptr)
        delete[] fLastValueData;
}

void DigitalPeakMeter::displayMeter(int meter, float level)
{
    Q_ASSERT(fChannelsData != nullptr);
    Q_ASSERT(meter > 0 && meter <= fChannels);

    if (meter <= 0 || meter > fChannels || fChannelsData == nullptr)
        return qCritical("DigitalPeakMeter::displayMeter(%i, %f) - invalid meter number", meter, level);

    int i = meter - 1;

    if (fSmoothMultiplier > 0)
        level = (fLastValueData[i] * fSmoothMultiplier + level) / float(fSmoothMultiplier + 1);

    if (level < 0.001f)
        level = 0.0f;
    else if (level > 0.999f)
        level = 1.0f;

    if (fChannelsData[i] != level)
    {
        fChannelsData[i] = level;
        update();
    }

    fLastValueData[i] = level;
}

void DigitalPeakMeter::setChannels(int channels)
{
    Q_ASSERT(channels >= 0);

    if (channels < 0)
        return qCritical("DigitalPeakMeter::setChannels(%i) - 'channels' must be a positive integer", channels);

    fChannels = channels;

    if (fChannelsData != nullptr)
        delete[] fChannelsData;
    if (fLastValueData != nullptr)
        delete[] fLastValueData;

    if (channels > 0)
    {
        fChannelsData  = new float[channels];
        fLastValueData = new float[channels];

        for (int i=0; i < channels; ++i)
        {
            fChannelsData[i]  = 0.0f;
            fLastValueData[i] = 0.0f;
        }
    }
    else
    {
        fChannelsData  = nullptr;
        fLastValueData = nullptr;
    }
}

void DigitalPeakMeter::setColor(Color color)
{
    if (color == GREEN)
    {
        fColorBase    = QColor(93, 231, 61);
        fColorBaseAlt = QColor(15, 110, 15, 100);
    }
    else if (color == BLUE)
    {
        fColorBase    = QColor(82, 238, 248);
        fColorBaseAlt = QColor(15, 15, 110, 100);
    }
    else
        return qCritical("DigitalPeakMeter::setColor(%i) - invalid color", color);

    setOrientation(fOrientation);
}

void DigitalPeakMeter::setOrientation(Orientation orientation)
{
    fOrientation = orientation;

    if (fOrientation == HORIZONTAL)
    {
        fGradientMeter.setColorAt(0.0f, fColorBase);
        fGradientMeter.setColorAt(0.2f, fColorBase);
        fGradientMeter.setColorAt(0.4f, fColorBase);
        fGradientMeter.setColorAt(0.6f, fColorBase);
        fGradientMeter.setColorAt(0.8f, Qt::yellow);
        fGradientMeter.setColorAt(1.0f, Qt::red);
    }
    else if (fOrientation == VERTICAL)
    {
        fGradientMeter.setColorAt(0.0f, Qt::red);
        fGradientMeter.setColorAt(0.2f, Qt::yellow);
        fGradientMeter.setColorAt(0.4f, fColorBase);
        fGradientMeter.setColorAt(0.6f, fColorBase);
        fGradientMeter.setColorAt(0.8f, fColorBase);
        fGradientMeter.setColorAt(1.0f, fColorBase);
    }
    else
        return qCritical("DigitalPeakMeter::setOrientation(%i) - invalid orientation", orientation);

    updateSizes();
}

void DigitalPeakMeter::setSmoothRelease(int value)
{
    Q_ASSERT(value >= 0 && value <= 5);

    if (value < 0)
        value = 0;
    else if (value > 5)
        value = 5;

    fSmoothMultiplier = value;
}

QSize DigitalPeakMeter::minimumSizeHint() const
{
    return QSize(10, 10);
}

QSize DigitalPeakMeter::sizeHint() const
{
    return QSize(fWidth, fHeight);
}

void DigitalPeakMeter::updateSizes()
{
    fWidth  = width();
    fHeight = height();
    fSizeMeter = 0;

    if (fOrientation == HORIZONTAL)
    {
        fGradientMeter.setFinalStop(fWidth, 0);

        if (fChannels > 0)
            fSizeMeter = fHeight/fChannels;
    }
    else if (fOrientation == VERTICAL)
    {
        fGradientMeter.setFinalStop(0, fHeight);

        if (fChannels > 0)
            fSizeMeter = fWidth/fChannels;
    }
}

void DigitalPeakMeter::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    event->accept();

    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, fWidth, fHeight);

    int meterX = 0;
    painter.setPen(fColorBackground);
    painter.setBrush(fGradientMeter);

    for (int i=0; i < fChannels; ++i)
    {
        float value, level = fChannelsData[i];

        if (fOrientation == HORIZONTAL)
            value = level * float(fWidth);
        else if (fOrientation == VERTICAL)
            value = float(fHeight) - (level * float(fHeight));
        else
            value = 0.0f;

        if (fOrientation == HORIZONTAL)
            painter.drawRect(0, meterX, int(value), fSizeMeter);
        else if (fOrientation == VERTICAL)
            painter.drawRect(meterX, int(value), fSizeMeter, fHeight);

        meterX += fSizeMeter;
    }

    painter.setBrush(Qt::black);

    if (fOrientation == HORIZONTAL)
    {
        // Variables
        float lsmall = fWidth;
        float lfull  = fHeight - 1;

        // Base
        painter.setPen(fColorBaseAlt);
        painter.drawLine(lsmall * 0.25f, 2, lsmall * 0.25f, lfull-2.0f);
        painter.drawLine(lsmall * 0.50f, 2, lsmall * 0.50f, lfull-2.0f);

        // Yellow
        painter.setPen(QColor(110, 110, 15, 100));
        painter.drawLine(lsmall * 0.70f, 2, lsmall * 0.70f, lfull-2.0f);
        painter.drawLine(lsmall * 0.83f, 2, lsmall * 0.83f, lfull-2.0f);

        // Orange
        painter.setPen(QColor(180, 110, 15, 100));
        painter.drawLine(lsmall * 0.90f, 2, lsmall * 0.90f, lfull-2.0f);

        // Red
        painter.setPen(QColor(110, 15, 15, 100));
        painter.drawLine(lsmall * 0.96f, 2, lsmall * 0.96f, lfull-2.0f);

    }
    else if (fOrientation == VERTICAL)
    {
        // Variables
        float lsmall = fHeight;
        float lfull  = fWidth - 1;

        // Base
        painter.setPen(fColorBaseAlt);
        painter.drawLine(2, lsmall - (lsmall * 0.25f), lfull-2.0f, lsmall - (lsmall * 0.25f));
        painter.drawLine(2, lsmall - (lsmall * 0.50f), lfull-2.0f, lsmall - (lsmall * 0.50f));

        // Yellow
        painter.setPen(QColor(110, 110, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.70f), lfull-2.0f, lsmall - (lsmall * 0.70f));
        painter.drawLine(2, lsmall - (lsmall * 0.83f), lfull-2.0f, lsmall - (lsmall * 0.83f));

        // Orange
        painter.setPen(QColor(180, 110, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.90f), lfull-2.0f, lsmall - (lsmall * 0.90f));

        // Red
        painter.setPen(QColor(110, 15, 15, 100));
        painter.drawLine(2, lsmall - (lsmall * 0.96f), lfull-2.0f, lsmall - (lsmall * 0.96f));
    }
}

void DigitalPeakMeter::resizeEvent(QResizeEvent* event)
{
    updateSizes();
    QWidget::resizeEvent(event);
}
