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

#ifndef CARLA_STYLE_PRIVATE_HPP_INCLUDED
#define CARLA_STYLE_PRIVATE_HPP_INCLUDED

#include "CarlaStyle.hpp"
#include "CarlaStyleAnimations.hpp"

#include <QtCore/QHash>

class QStyleAnimation;

class CarlaStylePrivate : public QObject
{
    Q_OBJECT

public:
    enum {
        menuItemHMargin    =  3, // menu item hor text margin
        menuArrowHMargin   =  6, // menu arrow horizontal margin
        menuRightBorder    = 15, // right border on menus
        menuCheckMarkWidth = 12  // checkmarks width on menus
    };

    CarlaStylePrivate(CarlaStyle* const style)
        : fStyle(style),
          fAnimationFps(60)
    {
    }

    ~CarlaStylePrivate() override
    {
        qDeleteAll(fAnimations);
    }

    int animationFps() const
    {
        return fAnimationFps;
    }

    // Used for grip handles
    QColor lightShade() const
    {
        return QColor(102, 102, 102, 90);
    }

    QColor darkShade() const
    {
        return QColor(0, 0, 0, 60);
    }

    QColor topShadow() const
    {
        return QColor(0, 0, 0, 18);
    }

    QColor innerContrastLine() const
    {
        return QColor(255, 255, 255, 30);
    }

    QColor highlight(const QPalette& pal) const
    {
        return pal.color(QPalette::Active, QPalette::Highlight);
    }

    QColor highlightedText(const QPalette& pal) const
    {
        return pal.color(QPalette::Active, QPalette::HighlightedText);
    }

    QColor outline(const QPalette& pal) const
    {
        if (! pal.window().texture().isNull())
            return QColor(0, 0, 0, 160);
        return pal.background().color().darker(140);
    }

    QColor highlightedOutline(const QPalette &pal) const
    {
        QColor highlightedOutline = highlight(pal).darker(125);
        if (highlightedOutline.value() > 160)
            highlightedOutline.setHsl(highlightedOutline.hue(), highlightedOutline.saturation(), 160);
        return highlightedOutline;
    }

    QColor tabFrameColor(const QPalette& pal) const
    {
        if (! pal.button().texture().isNull())
            return QColor(255, 255, 255, 8);
        return buttonColor(pal).lighter(104);
    }

    QColor buttonColor(const QPalette& pal) const
    {
        QColor buttonColor = pal.button().color();
        const int val = qGray(buttonColor.rgb());
        buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
        buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
        return buttonColor;
    }

    QIcon tabBarcloseButtonIcon;

    QList<const QObject*> animationTargets() const
    {
        return fAnimations.keys();
    }

    CarlaStyleAnimation* animation(const QObject* target) const
    {
        return fAnimations.value(target);
    }

    void startAnimation(CarlaStyleAnimation* animation) const
    {
        stopAnimation(animation->target());
        fStyle->connect(animation, SIGNAL(destroyed()), SLOT(_removeAnimation()), Qt::UniqueConnection);
        fAnimations.insert(animation->target(), animation);
        animation->start();
    }

    void stopAnimation(const QObject* target) const
    {
        CarlaStyleAnimation* const animation = fAnimations.take(target);
        if (animation != nullptr && animation->state() != QAbstractAnimation::Stopped)
            animation->stop();
    }

private:
    CarlaStyle* const fStyle;
    int fAnimationFps;
    mutable QHash<const QObject*, CarlaStyleAnimation*> fAnimations;

private slots:
    void _removeAnimation()
    {
        if (QObject* const animation = fStyle->sender())
           fAnimations.remove(animation->parent());
    }
};

#endif // CARLA_STYLE_PRIVATE_HPP_INCLUDED
