/*
 * Carla style
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "carla_shared.hpp"
#include "carla_style.hpp"

#include "theme/CarlaStyle.hpp"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtWidgets/QApplication>

// ------------------------------------------------------------------------------------------------------------

CarlaApplication::CarlaApplication(int& argc, char* argv[], const QString& appName)
    : fApp(nullptr),
      fStyle(nullptr)
{
    fApp = new QApplication(argc, argv, true);
    fApp->setApplicationName(appName);
    fApp->setApplicationVersion(VERSION);
    fApp->setOrganizationName("falkTX");

    if (appName.toLower() == "carla-control")
        fApp->setWindowIcon(QIcon(":/scalable/carla-control.svg"));
    else
        fApp->setWindowIcon(QIcon(":/scalable/carla.svg"));

    fPalSystem = fApp->palette();

    fPalBlack.setColor(QPalette::Disabled, QPalette::Window, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Active,   QPalette::Window, QColor(17, 17, 17));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Window, QColor(17, 17, 17));
    fPalBlack.setColor(QPalette::Disabled, QPalette::WindowText, QColor(83, 83, 83));
    fPalBlack.setColor(QPalette::Active,   QPalette::WindowText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Inactive, QPalette::WindowText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Base, QColor(6, 6, 6));
    fPalBlack.setColor(QPalette::Active,   QPalette::Base, QColor(7, 7, 7));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Base, QColor(7, 7, 7));
    fPalBlack.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(12, 12, 12));
    fPalBlack.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(4, 4, 4));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Text, QColor(74, 74, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::Text, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Text, QColor(230, 230, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Button, QColor(24, 24, 24));
    fPalBlack.setColor(QPalette::Active,   QPalette::Button, QColor(28, 28, 28));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Button, QColor(28, 28, 28));
    fPalBlack.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
    fPalBlack.setColor(QPalette::Active,   QPalette::ButtonText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Active,   QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Light, QColor(191, 191, 191));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Active,   QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Midlight, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Active,   QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Dark, QColor(129, 129, 129));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Active,   QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Mid, QColor(94, 94, 94));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Active,   QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Shadow, QColor(155, 155, 155));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Highlight, QColor(14, 14, 14));
    fPalBlack.setColor(QPalette::Active,   QPalette::Highlight, QColor(60, 60, 60));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Highlight, QColor(34, 34, 34));
    fPalBlack.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(83, 83, 83));
    fPalBlack.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(255, 255, 255));
    fPalBlack.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(240, 240, 240));
    fPalBlack.setColor(QPalette::Disabled, QPalette::Link, QColor(34, 34, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::Link, QColor(100, 100, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::Link, QColor(100, 100, 230));
    fPalBlack.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(74, 34, 74));
    fPalBlack.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(230, 100, 230));
    fPalBlack.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(230, 100, 230));

    fPalBlue.setColor(QPalette::Disabled, QPalette::Window, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Active,   QPalette::Window, QColor(37, 40, 45));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Window, QColor(37, 40, 45));
    fPalBlue.setColor(QPalette::Disabled, QPalette::WindowText, QColor(89, 95, 104));
    fPalBlue.setColor(QPalette::Active,   QPalette::WindowText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::WindowText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Base, QColor(48, 53, 60));
    fPalBlue.setColor(QPalette::Active,   QPalette::Base, QColor(55, 61, 69));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Base, QColor(55, 61, 69));
    fPalBlue.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(60, 64, 67));
    fPalBlue.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(69, 73, 77));
    fPalBlue.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(69, 73, 77));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(182, 193, 208));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(42, 44, 48));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Text, QColor(96, 103, 113));
    fPalBlue.setColor(QPalette::Active,   QPalette::Text, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Text, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Button, QColor(51, 55, 62));
    fPalBlue.setColor(QPalette::Active,   QPalette::Button, QColor(59, 63, 71));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Button, QColor(59, 63, 71));
    fPalBlue.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(98, 104, 114));
    fPalBlue.setColor(QPalette::Active,   QPalette::ButtonText, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(210, 222, 240));
    fPalBlue.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Light, QColor(59, 64, 72));
    fPalBlue.setColor(QPalette::Active,   QPalette::Light, QColor(63, 68, 76));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Light, QColor(63, 68, 76));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Midlight, QColor(48, 52, 59));
    fPalBlue.setColor(QPalette::Active,   QPalette::Midlight, QColor(51, 56, 63));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Midlight, QColor(51, 56, 63));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Dark, QColor(18, 19, 22));
    fPalBlue.setColor(QPalette::Active,   QPalette::Dark, QColor(20, 22, 25));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Dark, QColor(20, 22, 25));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Mid, QColor(28, 30, 34));
    fPalBlue.setColor(QPalette::Active,   QPalette::Mid, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Mid, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Shadow, QColor(13, 14, 16));
    fPalBlue.setColor(QPalette::Active,   QPalette::Shadow, QColor(15, 16, 18));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Shadow, QColor(15, 16, 18));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Highlight, QColor(32, 35, 39));
    fPalBlue.setColor(QPalette::Active,   QPalette::Highlight, QColor(14, 14, 17));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Highlight, QColor(27, 28, 33));
    fPalBlue.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(89, 95, 104));
    fPalBlue.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(217, 234, 253));
    fPalBlue.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(223, 237, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::Link, QColor(79, 100, 118));
    fPalBlue.setColor(QPalette::Active,   QPalette::Link, QColor(156, 212, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::Link, QColor(156, 212, 255));
    fPalBlue.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(51, 74, 118));
    fPalBlue.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(64, 128, 255));
    fPalBlue.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(64, 128, 255));

    loadSettings();
}

CarlaApplication::~CarlaApplication()
{
    CARLA_SAFE_ASSERT_RETURN(fApp != nullptr,);

    if (fStyle != nullptr)
    {
        //delete fStyle;
        fStyle = nullptr;
    }

    delete fApp;
    fApp = nullptr;
}

void CarlaApplication::loadSettings()
{
    CARLA_SAFE_ASSERT_RETURN(fApp != nullptr,);

    QSettings settings;

    const bool useProTheme(settings.value("Main/UseProTheme", true).toBool());

    if (useProTheme)
    {
        //QFont font("DejaVu Sans [Book]", 8, QFont::Normal);
        //fApp->setFont(font);
        //QApplication::setFont(font);

        // TODO

        if (fStyle == nullptr)
            fStyle = new CarlaStyle();

        fApp->setStyle(fStyle);
        QApplication::setStyle(fStyle);

        const QString proThemeColor(settings.value("Main/ProThemeColor", "Black").toString().toLower());

        if (proThemeColor == "black")
            fApp->setPalette(fPalBlack);

        else if (proThemeColor == "blue")
            fApp->setPalette(fPalBlue);
    }

    carla_stdout("Using \"%s\" theme", fApp->style()->objectName().toUtf8().constData());
}

// ------------------------------------------------------------------------------------------------------------
