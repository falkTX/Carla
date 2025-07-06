/*
 * Carla application
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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

#include "carla_app.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Imports (Global)

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Weffc++"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include <QtGui/QIcon>
#include <QtGui/QPalette>

#include <QtWidgets/QApplication>

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

//---------------------------------------------------------------------------------------------------------------------
// Imports (Custom)

#include "carla_shared.hpp"

//---------------------------------------------------------------------------------------------------------------------

CarlaApplication::CarlaApplication(const QString appName, int& argc, char* argv[])
    : fApp(nullptr)
{
    QCarlaString pathBinaries, pathResources;
    /*
    pathBinaries, pathResources = getPaths(libPrefix)

    // Needed for MacOS and Windows
    if (os.path.exists(CWD))
        QApplication.addLibraryPath(CWD)

    // Needed for local wine build
    if CARLA_OS_WIN and CWD.endswith(("frontend", "resources")) and os.getenv("CXFREEZE") is None:
        if CARLA_OS_64BIT:
            path = "H:\\builds\\msys2-x86_64\\mingw64\\share\\qt5\\plugins"
        else:
            path = "H:\\builds\\msys2-i686\\mingw32\\share\\qt5\\plugins"
        QApplication.addLibraryPath(path)
    */

    QCarlaString stylesDir;

    // Use binary dir as library path
    if (QDir(pathBinaries).exists())
    {
        QApplication::addLibraryPath(pathBinaries);
        stylesDir = pathBinaries;
    }

    // base settings
    const QSafeSettings settings("falkTX", appName);
#ifdef CARLA_OS_MAC
    const bool useProTheme = true;
#else
    const bool useProTheme = settings.valueBool(CARLA_KEY_MAIN_USE_PRO_THEME, CARLA_DEFAULT_MAIN_USE_PRO_THEME);
#endif

    if (! useProTheme)
    {
        createApp(appName, argc, argv);
        return;
    }

    // set style
    QApplication::setStyle(stylesDir.isNotEmpty() ? "carla" : "fusion");

    // create app
    QApplication* const guiApp = createApp(appName, argc, argv);

    if (guiApp == nullptr)
        return;

    guiApp->setStyle(stylesDir.isNotEmpty() ? "carla" : "fusion");

#ifdef MACOS
    if (true)
#else
    // set palette
    const QString proThemeColor(settings.valueString(CARLA_KEY_MAIN_PRO_THEME_COLOR, CARLA_DEFAULT_MAIN_PRO_THEME_COLOR).toLower());

    if (proThemeColor == "black")
#endif
    {
        QPalette palBlack;
        palBlack.setColor(QPalette::Disabled, QPalette::Window, QColor(14, 14, 14));
        palBlack.setColor(QPalette::Active,   QPalette::Window, QColor(17, 17, 17));
        palBlack.setColor(QPalette::Inactive, QPalette::Window, QColor(17, 17, 17));
        palBlack.setColor(QPalette::Disabled, QPalette::WindowText, QColor(83, 83, 83));
        palBlack.setColor(QPalette::Active,   QPalette::WindowText, QColor(240, 240, 240));
        palBlack.setColor(QPalette::Inactive, QPalette::WindowText, QColor(240, 240, 240));
        palBlack.setColor(QPalette::Disabled, QPalette::Base, QColor(6, 6, 6));
        palBlack.setColor(QPalette::Active,   QPalette::Base, QColor(7, 7, 7));
        palBlack.setColor(QPalette::Inactive, QPalette::Base, QColor(7, 7, 7));
        palBlack.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(12, 12, 12));
        palBlack.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(14, 14, 14));
        palBlack.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(14, 14, 14));
        palBlack.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(4, 4, 4));
        palBlack.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(4, 4, 4));
        palBlack.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(4, 4, 4));
        palBlack.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(230, 230, 230));
        palBlack.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(230, 230, 230));
        palBlack.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(230, 230, 230));
        palBlack.setColor(QPalette::Disabled, QPalette::Text, QColor(74, 74, 74));
        palBlack.setColor(QPalette::Active,   QPalette::Text, QColor(230, 230, 230));
        palBlack.setColor(QPalette::Inactive, QPalette::Text, QColor(230, 230, 230));
        palBlack.setColor(QPalette::Disabled, QPalette::Button, QColor(24, 24, 24));
        palBlack.setColor(QPalette::Active,   QPalette::Button, QColor(28, 28, 28));
        palBlack.setColor(QPalette::Inactive, QPalette::Button, QColor(28, 28, 28));
        palBlack.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(90, 90, 90));
        palBlack.setColor(QPalette::Active,   QPalette::ButtonText, QColor(240, 240, 240));
        palBlack.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(240, 240, 240));
        palBlack.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
        palBlack.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
        palBlack.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
        palBlack.setColor(QPalette::Disabled, QPalette::Light, QColor(191, 191, 191));
        palBlack.setColor(QPalette::Active,   QPalette::Light, QColor(191, 191, 191));
        palBlack.setColor(QPalette::Inactive, QPalette::Light, QColor(191, 191, 191));
        palBlack.setColor(QPalette::Disabled, QPalette::Midlight, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Active,   QPalette::Midlight, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Inactive, QPalette::Midlight, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Disabled, QPalette::Dark, QColor(129, 129, 129));
        palBlack.setColor(QPalette::Active,   QPalette::Dark, QColor(129, 129, 129));
        palBlack.setColor(QPalette::Inactive, QPalette::Dark, QColor(129, 129, 129));
        palBlack.setColor(QPalette::Disabled, QPalette::Mid, QColor(94, 94, 94));
        palBlack.setColor(QPalette::Active,   QPalette::Mid, QColor(94, 94, 94));
        palBlack.setColor(QPalette::Inactive, QPalette::Mid, QColor(94, 94, 94));
        palBlack.setColor(QPalette::Disabled, QPalette::Shadow, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Active,   QPalette::Shadow, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Inactive, QPalette::Shadow, QColor(155, 155, 155));
        palBlack.setColor(QPalette::Disabled, QPalette::Highlight, QColor(14, 14, 14));
        palBlack.setColor(QPalette::Active,   QPalette::Highlight, QColor(60, 60, 60));
        palBlack.setColor(QPalette::Inactive, QPalette::Highlight, QColor(34, 34, 34));
        palBlack.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(83, 83, 83));
        palBlack.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(255, 255, 255));
        palBlack.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(240, 240, 240));
        palBlack.setColor(QPalette::Disabled, QPalette::Link, QColor(34, 34, 74));
        palBlack.setColor(QPalette::Active,   QPalette::Link, QColor(100, 100, 230));
        palBlack.setColor(QPalette::Inactive, QPalette::Link, QColor(100, 100, 230));
        palBlack.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(74, 34, 74));
        palBlack.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(230, 100, 230));
        palBlack.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(230, 100, 230));
        guiApp->setPalette(palBlack);
    }
    else if (proThemeColor == "blue")
    {
        QPalette palBlue;
        palBlue.setColor(QPalette::Disabled, QPalette::Window, QColor(32, 35, 39));
        palBlue.setColor(QPalette::Active,   QPalette::Window, QColor(37, 40, 45));
        palBlue.setColor(QPalette::Inactive, QPalette::Window, QColor(37, 40, 45));
        palBlue.setColor(QPalette::Disabled, QPalette::WindowText, QColor(89, 95, 104));
        palBlue.setColor(QPalette::Active,   QPalette::WindowText, QColor(223, 237, 255));
        palBlue.setColor(QPalette::Inactive, QPalette::WindowText, QColor(223, 237, 255));
        palBlue.setColor(QPalette::Disabled, QPalette::Base, QColor(48, 53, 60));
        palBlue.setColor(QPalette::Active,   QPalette::Base, QColor(55, 61, 69));
        palBlue.setColor(QPalette::Inactive, QPalette::Base, QColor(55, 61, 69));
        palBlue.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(60, 64, 67));
        palBlue.setColor(QPalette::Active,   QPalette::AlternateBase, QColor(69, 73, 77));
        palBlue.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(69, 73, 77));
        palBlue.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(182, 193, 208));
        palBlue.setColor(QPalette::Active,   QPalette::ToolTipBase, QColor(182, 193, 208));
        palBlue.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(182, 193, 208));
        palBlue.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(42, 44, 48));
        palBlue.setColor(QPalette::Active,   QPalette::ToolTipText, QColor(42, 44, 48));
        palBlue.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(42, 44, 48));
        palBlue.setColor(QPalette::Disabled, QPalette::Text, QColor(96, 103, 113));
        palBlue.setColor(QPalette::Active,   QPalette::Text, QColor(210, 222, 240));
        palBlue.setColor(QPalette::Inactive, QPalette::Text, QColor(210, 222, 240));
        palBlue.setColor(QPalette::Disabled, QPalette::Button, QColor(51, 55, 62));
        palBlue.setColor(QPalette::Active,   QPalette::Button, QColor(59, 63, 71));
        palBlue.setColor(QPalette::Inactive, QPalette::Button, QColor(59, 63, 71));
        palBlue.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(98, 104, 114));
        palBlue.setColor(QPalette::Active,   QPalette::ButtonText, QColor(210, 222, 240));
        palBlue.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(210, 222, 240));
        palBlue.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
        palBlue.setColor(QPalette::Active,   QPalette::BrightText, QColor(255, 255, 255));
        palBlue.setColor(QPalette::Inactive, QPalette::BrightText, QColor(255, 255, 255));
        palBlue.setColor(QPalette::Disabled, QPalette::Light, QColor(59, 64, 72));
        palBlue.setColor(QPalette::Active,   QPalette::Light, QColor(63, 68, 76));
        palBlue.setColor(QPalette::Inactive, QPalette::Light, QColor(63, 68, 76));
        palBlue.setColor(QPalette::Disabled, QPalette::Midlight, QColor(48, 52, 59));
        palBlue.setColor(QPalette::Active,   QPalette::Midlight, QColor(51, 56, 63));
        palBlue.setColor(QPalette::Inactive, QPalette::Midlight, QColor(51, 56, 63));
        palBlue.setColor(QPalette::Disabled, QPalette::Dark, QColor(18, 19, 22));
        palBlue.setColor(QPalette::Active,   QPalette::Dark, QColor(20, 22, 25));
        palBlue.setColor(QPalette::Inactive, QPalette::Dark, QColor(20, 22, 25));
        palBlue.setColor(QPalette::Disabled, QPalette::Mid, QColor(28, 30, 34));
        palBlue.setColor(QPalette::Active,   QPalette::Mid, QColor(32, 35, 39));
        palBlue.setColor(QPalette::Inactive, QPalette::Mid, QColor(32, 35, 39));
        palBlue.setColor(QPalette::Disabled, QPalette::Shadow, QColor(13, 14, 16));
        palBlue.setColor(QPalette::Active,   QPalette::Shadow, QColor(15, 16, 18));
        palBlue.setColor(QPalette::Inactive, QPalette::Shadow, QColor(15, 16, 18));
        palBlue.setColor(QPalette::Disabled, QPalette::Highlight, QColor(32, 35, 39));
        palBlue.setColor(QPalette::Active,   QPalette::Highlight, QColor(14, 14, 17));
        palBlue.setColor(QPalette::Inactive, QPalette::Highlight, QColor(27, 28, 33));
        palBlue.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(89, 95, 104));
        palBlue.setColor(QPalette::Active,   QPalette::HighlightedText, QColor(217, 234, 253));
        palBlue.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor(223, 237, 255));
        palBlue.setColor(QPalette::Disabled, QPalette::Link, QColor(79, 100, 118));
        palBlue.setColor(QPalette::Active,   QPalette::Link, QColor(156, 212, 255));
        palBlue.setColor(QPalette::Inactive, QPalette::Link, QColor(156, 212, 255));
        palBlue.setColor(QPalette::Disabled, QPalette::LinkVisited, QColor(51, 74, 118));
        palBlue.setColor(QPalette::Active,   QPalette::LinkVisited, QColor(64, 128, 255));
        palBlue.setColor(QPalette::Inactive, QPalette::LinkVisited, QColor(64, 128, 255));
        guiApp->setPalette(palBlue);
    }
}

QApplication* CarlaApplication::createApp(const QString& appName, int& argc, char* argv[])
{
#ifdef CARLA_OS_LINUX
    QApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

#ifdef CARLA_OS_MAC
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

#if QT_VERSION >= 0x50600
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#ifdef CARLA_OS_WIN
    static int r_argc = argc + 2;
    static char** const r_argv = static_cast<char**>(calloc(sizeof(char*), r_argc));
    static char argvExtra1[] = "-platform\0";
    static char argvExtra2[] = "windows:fontengine=freetype\0";
    for (int i=0; i<argc; ++i)
        r_argv[i] = argv[i];
    r_argv[argc] = argvExtra1;
    r_argv[argc] = argvExtra2;
#else
# define r_argc argc
# define r_argv argv
#endif

    fApp = gCarla.nogui ? new QCoreApplication(r_argc, r_argv) : new QApplication(r_argc, r_argv);
    fApp->setApplicationName(appName);
    fApp->setApplicationVersion(CARLA_VERSION_STRING);
    fApp->setOrganizationName("falkTX");

#ifndef CARLA_OS_WIN
# undef r_argc
# undef r_argv
#endif

    if (gCarla.nogui)
        return nullptr;

    QApplication* const guiApp = dynamic_cast<QApplication*>(fApp);
    CARLA_SAFE_ASSERT_RETURN(guiApp != nullptr, nullptr);

    if (appName.toLower() == "carla-control")
    {
#if QT_VERSION >= 0x50700
        guiApp->setDesktopFileName("carla-control");
#endif
        guiApp->setWindowIcon(QIcon(":/scalable/carla-control.svg"));
    }
    else
    {
#if QT_VERSION >= 0x50700
        guiApp->setDesktopFileName("carla");
#endif
        guiApp->setWindowIcon(QIcon(":/scalable/carla.svg"));
    }

    return guiApp;
}

//---------------------------------------------------------------------------------------------------------------------
