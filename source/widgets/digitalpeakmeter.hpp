/*
 * Digital Peak Meter, a custom Qt widget
 * Copyright (C) 2011-2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef DIGITALPEAKMETER_HPP_INCLUDED
#define DIGITALPEAKMETER_HPP_INCLUDED

#include "CarlaJuceUtils.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QWidget>
#else
# include <QtGui/QWidget>
#endif

// ------------------------------------------------------------------------------------------------------------

class DigitalPeakMeter : public QWidget
{
public:
    enum Color {
        COLOR_GREEN = 1,
        COLOR_BLUE  = 2
    };

    enum Orientation {
        HORIZONTAL = 1,
        VERTICAL   = 2
    };

    enum Style {
        STYLE_DEFAULT = 1,
        STYLE_OPENAV  = 2,
        STYLE_RNCBC   = 3
    };

    // --------------------------------------------------------------------------------------------------------

    DigitalPeakMeter(QWidget* const p)
        : QWidget(p),
          fChannelCount(0),
          fChannelData(nullptr),
          fLastChannelData(nullptr),
          fMeterColor(COLOR_GREEN),
          fMeterColorBase(93, 231, 61),
          fMeterColorBaseAlt(15, 110, 15, 100),
          fMeterLinesEnabled(true),
          fMeterOrientation(VERTICAL),
          fMeterStyle(STYLE_DEFAULT),
          fMeterBackground("#111111"),
          fMeterGradient(0, 0, 0, 0),
          fSmoothMultiplier(1)
    {
        updateGrandient();
    }

    ~DigitalPeakMeter() override
    {
        if (fChannelData != nullptr)
        {
            delete[] fChannelData;
            fChannelData = nullptr;
        }

        if (fLastChannelData != nullptr)
        {
            delete[] fLastChannelData;
            fLastChannelData = nullptr;
        }
    }

    // --------------------------------------------------------------------------------------------------------

    int channelCount() const noexcept
    {
        return fChannelCount;
    }

    void setChannelCount(const int count)
    {
        if (fChannelCount == count)
            return;

        if (count < 0)
            return qCritical("DigitalPeakMeter::setChannelCount(%i) - channel count must be a positive integer or zero", count);

        fChannelCount    = count;
        fChannelData     = new float[count];
        fLastChannelData = new float[count];

        for (int i=count; --i >= 0;)
        {
            /**/fChannelData[i] = 0.0f;
            fLastChannelData[i] = 0.0f;
        }
    }

    // --------------------------------------------------------------------------------------------------------

    Color meterColor() const noexcept
    {
        return fMeterColor;
    }

    void setMeterColor(const Color color)
    {
        if (fMeterColor == color)
            return;

        if (! QList<Color>({COLOR_GREEN, COLOR_BLUE}).contains(color))
            return qCritical("DigitalPeakMeter::setMeterColor(%i) - invalid color", color);

        switch (color)
        {
        case COLOR_GREEN:
            fMeterColorBase    = QColor(93, 231, 61);
            fMeterColorBaseAlt = QColor(15, 110, 15, 100);
            break;
        case COLOR_BLUE:
            fMeterColorBase    = QColor(82, 238, 248);
            fMeterColorBaseAlt = QColor(15, 15, 110, 100);
            break;
        }

        fMeterColor = color;

        updateGrandient();
    }

    // --------------------------------------------------------------------------------------------------------

    bool meterLinesEnabled() const noexcept
    {
        return fMeterLinesEnabled;
    }

    void setMeterLinesEnabled(const bool yesNo)
    {
        if (fMeterLinesEnabled == yesNo)
            return;

        fMeterLinesEnabled = yesNo;
    }

    // --------------------------------------------------------------------------------------------------------

    Orientation meterOrientation() const noexcept
    {
        return fMeterOrientation;
    }

    void setMeterOrientation(const Orientation orientation)
    {
        if (fMeterOrientation == orientation)
            return;

        if (! QList<Orientation>({HORIZONTAL, VERTICAL}).contains(orientation))
            return qCritical("DigitalPeakMeter::setMeterOrientation(%i) - invalid orientation", orientation);

        fMeterOrientation = orientation;

        updateGrandient();
    }

    // --------------------------------------------------------------------------------------------------------

    Style meterStyle() const noexcept
    {
        return fMeterStyle;
    }

    void setMeterStyle(const Style style)
    {
        if (fMeterStyle == style)
            return;

        if (! QList<Style>({STYLE_DEFAULT, STYLE_OPENAV, STYLE_RNCBC}).contains(style))
            return qCritical("DigitalPeakMeter::setMeterStyle(%i) - invalid style", style);

        switch (style)
        {
        case STYLE_DEFAULT:
            fMeterBackground = QColor("#111111");
            break;
        case STYLE_OPENAV:
            fMeterBackground = QColor("#1A1A1A");
            break;
        case STYLE_RNCBC:
            fMeterBackground = QColor("#111111");
            break;
        }

        fMeterStyle = style;

        updateGrandient();
    }

    // --------------------------------------------------------------------------------------------------------

    int smoothMultiplier() const noexcept
    {
        return fSmoothMultiplier;
    }

    void setSmoothMultiplier(const int value)
    {
        if (fSmoothMultiplier == value)
            return;

        if (value < 0)
            return qCritical("DigitalPeakMeter::setSmoothMultiplier(%i) - value must be >= 0", value);
        if (value > 5)
            return qCritical("DigitalPeakMeter::setSmoothMultiplier(%i) - value must be < 5", value);

        fSmoothMultiplier = value;
    }

    // --------------------------------------------------------------------------------------------------------

    void displayMeter(const int meter, float level, bool forced = false)
    {
        if (meter <= 0 or meter > fChannelCount)
            return qCritical("DigitalPeakMeter::displayMeter(%i, %f) - invalid meter number", meter, level);

        const int i = meter - 1;

        if (fSmoothMultiplier > 0 && ! forced)
            level = (fLastChannelData[i] * float(fSmoothMultiplier) + level) / float(fSmoothMultiplier + 1);

        if (level < 0.001f)
            level = 0.0f;
        else if (level > 0.999f)
            level = 1.0f;

        if (fChannelData[i] != level)
        {
            fChannelData[i] = level;
            update();
        }

        fLastChannelData[i] = level;
    }

    // --------------------------------------------------------------------------------------------------------

protected:
    void updateGrandient()
    {
        fMeterGradient = QLinearGradient(0, 0, 1, 1);

        if (fMeterStyle == STYLE_OPENAV)
        {
            fMeterGradient.setColorAt(0.0, fMeterColorBase);
            fMeterGradient.setColorAt(1.0, fMeterColorBase);
        }
        else
        {
            switch (fMeterOrientation)
            {
            case HORIZONTAL:
                fMeterGradient.setColorAt(0.0, fMeterColorBase);
                fMeterGradient.setColorAt(0.2, fMeterColorBase);
                fMeterGradient.setColorAt(0.4, fMeterColorBase);
                fMeterGradient.setColorAt(0.6, fMeterColorBase);
                fMeterGradient.setColorAt(0.8, Qt::yellow);
                fMeterGradient.setColorAt(1.0, Qt::red);
                break;
            case VERTICAL:
                fMeterGradient.setColorAt(0.0, Qt::red);
                fMeterGradient.setColorAt(0.2, Qt::yellow);
                fMeterGradient.setColorAt(0.4, fMeterColorBase);
                fMeterGradient.setColorAt(0.6, fMeterColorBase);
                fMeterGradient.setColorAt(0.8, fMeterColorBase);
                fMeterGradient.setColorAt(1.0, fMeterColorBase);
                break;
            }
        }

        updateGrandientFinalStop();
    }

    void updateGrandientFinalStop()
    {
        switch (fMeterOrientation)
        {
        case HORIZONTAL:
            fMeterGradient.setFinalStop(width(), 0);
            break;
        case VERTICAL:
            fMeterGradient.setFinalStop(0, height());
            break;
        }
    }

    // --------------------------------------------------------------------------------------------------------

    QSize minimumSizeHint() const override
    {
        return QSize(10, 10);
    }

    QSize sizeHint() const override
    {
        return QSize(width(), height());
    }

    // --------------------------------------------------------------------------------------------------------

    void paintEvent(QPaintEvent* const ev) override
    {
        QPainter painter(this);
        ev->accept();

        const int width_  = width();
        const int height_ = height();

        // draw background
        painter.setPen(QPen(fMeterBackground, 2));
        painter.setBrush(fMeterBackground);
        painter.drawRect(0, 0, width_, height_);
    }

    // --------------------------------------------------------------------------------------------------------

    void resizeEvent(QResizeEvent* const ev) override
    {
        QWidget::resizeEvent(ev);
        updateGrandientFinalStop();
    }

    // --------------------------------------------------------------------------------------------------------

private:
    int    fChannelCount;
    float* fChannelData;
    float* fLastChannelData;

    Color  fMeterColor;
    QColor fMeterColorBase;
    QColor fMeterColorBaseAlt;

    bool        fMeterLinesEnabled;
    Orientation fMeterOrientation;
    Style       fMeterStyle;

    QColor          fMeterBackground;
    QLinearGradient fMeterGradient;

    int fSmoothMultiplier;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DigitalPeakMeter)
};

// ------------------------------------------------------------------------------------------------------------

#endif // DIGITALPEAKMETER_HPP_INCLUDED
