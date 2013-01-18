/*
 * Digital Peak Meter, a custom Qt4 widget
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

#ifndef DIGITALPEAKMETER_HPP
#define DIGITALPEAKMETER_HPP

#include <QtCore/QTimer>
#include <QtGui/QWidget>

class DigitalPeakMeter : public QWidget
{
public:
    enum Orientation {
        HORIZONTAL = 1,
        VERTICAL   = 2
    };

    enum Color {
        GREEN = 1,
        BLUE  = 2
    };

    DigitalPeakMeter(QWidget* parent);

    void displayMeter(int meter, float level);
    void setChannels(int channels);
    void setColor(Color color);
    void setOrientation(Orientation orientation);
    void setRefreshRate(int rate);
    void setSmoothRelease(int value);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

protected:
    void updateSizes();

    void paintEvent(QPaintEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    int m_channels;
    int m_smoothMultiplier;
    int m_width, m_height, m_sizeMeter;
    Orientation m_orientation;

    QColor m_colorBackground;
    QLinearGradient m_gradientMeter;

    QColor m_colorBase;
    QColor m_colorBaseT;

    QList<float> m_channelsData;
    QList<float> m_lastValueData;

    QTimer m_paintTimer;
};

#endif // DIGITALPEAKMETER_HPP
