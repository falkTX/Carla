/*
 * Carla Style, based on Qt5 fusion style
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies)
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the doc/LGPL.txt file
 */

#ifndef CARLA_STYLE_ANIMATIONS_HPP_INCLUDED
#define CARLA_STYLE_ANIMATIONS_HPP_INCLUDED

#include "CarlaStyle.hpp"

#include <QtCore/QAbstractAnimation>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtGui/QImage>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QWidget>
#else
# include <QtGui/QWidget>
#endif

class CarlaStyleAnimation : public QAbstractAnimation
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
    Q_OBJECT
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__clang__)
# pragma clang diagnostic pop
#endif

public:
    CarlaStyleAnimation(QObject* target)
      : QAbstractAnimation(),
        _delay(0),
        _duration(-1),
        _startTime(QTime::currentTime())
    {
        if (target != nullptr)
        {
            moveToThread(target->thread());
            setParent(target);
        }

        connect(this, SIGNAL(finished()), SLOT(deleteLater()));
    }

    QObject* target() const
    {
        return parent();
    }

    int duration() const override
    {
        return _duration;
    }

    void setDuration(int duration)
    {
        _duration = duration;
    }

    int delay() const
    {
        return _delay;
    }

    void setDelay(int delay)
    {
        _delay = delay;
    }

    QTime startTime() const
    {
        return _startTime;
    }

    void setStartTime(const QTime& time)
    {
        _startTime = time;
    }

    void updateTarget()
    {
        QEvent event(QEvent::HoverEnter);
        QCoreApplication::sendEvent(target(), &event);
    }

protected:
    virtual bool isUpdateNeeded() const
    {
        return currentTime() > _delay;
    }

    virtual void updateCurrentTime(int /*time*/) override
    {
        if (QObject* tgt = target())
        {
            if (tgt->isWidgetType())
            {
                QWidget* widget = static_cast<QWidget*>(tgt);
                if (widget->window()->isMinimized() || ! widget->isVisible())
                    stop();
            }

            if (isUpdateNeeded())
                updateTarget();
        }
    }

private:
    int _delay;
    int _duration;
    QTime _startTime;
};

class CarlaProgressStyleAnimation : public CarlaStyleAnimation
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
    Q_OBJECT
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__clang__)
# pragma clang diagnostic pop
#endif

public:
    CarlaProgressStyleAnimation(int speed, QObject* target)
        : CarlaStyleAnimation(target),
          _speed(speed),
          _step(-1)
    {
    }

    int animationStep() const
    {
        return currentTime() / (1000.0 / _speed);
    }

    int progressStep(int width) const
    {
        int step = animationStep();
        int progress = (step * width / _speed) % width;
        if (((step * width / _speed) % (2 * width)) >= width)
            progress = width - progress;
        return progress;
    }

    int speed() const
    {
        return _speed;
    }

    void setSpeed(int speed)
    {
        _speed = speed;
    }

protected:
    bool isUpdateNeeded() const override
    {
        if (CarlaStyleAnimation::isUpdateNeeded())
        {
            int current = animationStep();
            if (_step == -1 || _step != current)
            {
                _step = current;
                return true;
            }
        }
        return false;
    }

private:
    int _speed;
    mutable int _step;
};

#endif // CARLA_STYLE_ANIMATIONS_HPP_INCLUDED
