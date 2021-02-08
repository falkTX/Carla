#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla application
# Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os
import sys

# ------------------------------------------------------------------------------------------------------------
# Imports (ctypes)

from ctypes import CDLL, RTLD_GLOBAL

# ------------------------------------------------------------------------------------------------------------
# Imports (PyQt5)

from PyQt5.QtCore import QT_VERSION, Qt, QCoreApplication, QLibraryInfo, QLocale, QTranslator
from PyQt5.QtGui import QColor, QIcon, QPalette
from PyQt5.QtWidgets import QApplication

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from carla_backend import kIs64bit, LINUX, MACOS, WINDOWS

from carla_shared import (
    CARLA_KEY_MAIN_USE_PRO_THEME,
    CARLA_KEY_MAIN_PRO_THEME_COLOR,
    CARLA_DEFAULT_MAIN_USE_PRO_THEME,
    CARLA_DEFAULT_MAIN_PRO_THEME_COLOR,
    CWD, VERSION,
    getPaths,
    QSafeSettings,
    gCarla
)

# ------------------------------------------------------------------------------------------------------------

class CarlaApplication():
    def __init__(self, appName = "Carla2", libPrefix = None):
        pathBinaries, pathResources = getPaths(libPrefix)

        # Needed for MacOS and Windows
        if os.path.exists(CWD) and (MACOS or WINDOWS):
            QApplication.addLibraryPath(CWD)

        # Needed for local wine build
        if WINDOWS and CWD.endswith(("frontend", "resources")) and os.getenv("CXFREEZE") is None:
            if kIs64bit:
                path = "H:\\PawPawBuilds\\targets\\win64\\lib\\qt5\\plugins"
            else:
                path = "H:\\PawPawBuilds\\targets\\win32\\lib\\qt5\\plugins"
            QApplication.addLibraryPath(path)

        # Use binary dir as library path
        if os.path.exists(pathBinaries):
            QApplication.addLibraryPath(pathBinaries)
            stylesDir = pathBinaries

        # If style is not available we can still fake it
        else:
            stylesDir = ""

        # Set up translations
        currentLocale = QLocale()
        appTranslator = QTranslator()
        sysTranslator = None
        pathTranslations = os.path.join(pathResources, "translations")
        if appTranslator.load(currentLocale, "carla", "_", pathTranslations):
            sysTranslator = QTranslator()
            pathSysTranslations = pathTranslations
            if not sysTranslator.load(currentLocale, "qt", "_", pathSysTranslations):
                pathSysTranslations = QLibraryInfo.location(QLibraryInfo.TranslationsPath)
                sysTranslator.load(currentLocale, "qt", "_", pathSysTranslations)
        else:
            appTranslator = None
        self.fAppTranslator = appTranslator
        self.fSysTranslator = sysTranslator

        # base settings
        settings    = QSafeSettings("falkTX", appName)
        useProTheme = MACOS or settings.value(CARLA_KEY_MAIN_USE_PRO_THEME, CARLA_DEFAULT_MAIN_USE_PRO_THEME, bool)

        if not useProTheme:
            self.createApp(appName)
            return

        # set style
        QApplication.setStyle("carla" if stylesDir else "fusion")

        # create app
        self.createApp(appName)

        if gCarla.nogui:
            return

        self.fApp.setStyle("carla" if stylesDir else "fusion")

        if WINDOWS:
            carlastyle1 = os.path.join(pathBinaries, "styles", "carlastyle.dll")
            carlastyle2 = os.path.join(pathResources, "styles", "carlastyle.dll")
            carlastyle = carlastyle2 if os.path.exists(carlastyle2) else carlastyle1
            self._stylelib = CDLL(carlastyle, RTLD_GLOBAL)
            self._stylelib.set_qt_app_style.argtypes = None
            self._stylelib.set_qt_app_style.restype = None
            self._stylelib.set_qt_app_style()

        # set palette
        proThemeColor = settings.value(CARLA_KEY_MAIN_PRO_THEME_COLOR, CARLA_DEFAULT_MAIN_PRO_THEME_COLOR, str).lower()

        if MACOS or proThemeColor == "black":
            self.createPaletteBlack()

        elif proThemeColor == "blue":
            self.createPaletteBlue()

    def createApp(self, appName):
        if LINUX:
            # AA_X11InitThreads is not available on old PyQt versions
            try:
                attr = Qt.AA_X11InitThreads
            except:
                attr = 10
            QApplication.setAttribute(attr)

        if MACOS:
            QApplication.setAttribute(Qt.AA_DontShowIconsInMenus)

        if QT_VERSION >= 0x50600:
            QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
            QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)

        args = sys.argv[:]

        if WINDOWS:
            args += ["-platform", "windows:fontengine=freetype"]

        self.fApp = QCoreApplication(args) if gCarla.nogui else QApplication(args)
        self.fApp.setApplicationName(appName)
        self.fApp.setApplicationVersion(VERSION)
        self.fApp.setOrganizationName("falkTX")

        if self.fAppTranslator is not None:
            self.fApp.installTranslator(self.fAppTranslator)
            self.fApp.installTranslator(self.fSysTranslator)

        if gCarla.nogui:
            return

        if appName == "Carla2-Control":
            if QT_VERSION >= 0x50700:
                self.fApp.setDesktopFileName("carla-control")
            self.fApp.setWindowIcon(QIcon(":/scalable/carla-control.svg"))
        else:
            if QT_VERSION >= 0x50700:
                self.fApp.setDesktopFileName("carla")
            self.fApp.setWindowIcon(QIcon(":/scalable/carla.svg"))

    def createPaletteBlack(self):
        self.fPalBlack = QPalette()
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Window, QColor(14, 14, 14))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Window, QColor(17, 17, 17))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Window, QColor(17, 17, 17))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.WindowText, QColor(83, 83, 83))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.WindowText, QColor(240, 240, 240))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.WindowText, QColor(240, 240, 240))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Base, QColor(6, 6, 6))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Base, QColor(7, 7, 7))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Base, QColor(7, 7, 7))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.AlternateBase, QColor(12, 12, 12))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.AlternateBase, QColor(14, 14, 14))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.AlternateBase, QColor(14, 14, 14))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.ToolTipBase, QColor(4, 4, 4))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.ToolTipBase, QColor(4, 4, 4))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.ToolTipBase, QColor(4, 4, 4))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.ToolTipText, QColor(230, 230, 230))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.ToolTipText, QColor(230, 230, 230))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.ToolTipText, QColor(230, 230, 230))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Text, QColor(74, 74, 74))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Text, QColor(230, 230, 230))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Text, QColor(230, 230, 230))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Button, QColor(24, 24, 24))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Button, QColor(28, 28, 28))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Button, QColor(28, 28, 28))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(90, 90, 90))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.ButtonText, QColor(240, 240, 240))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.ButtonText, QColor(240, 240, 240))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Light, QColor(191, 191, 191))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Light, QColor(191, 191, 191))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Light, QColor(191, 191, 191))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Midlight, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Midlight, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Midlight, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Dark, QColor(129, 129, 129))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Dark, QColor(129, 129, 129))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Dark, QColor(129, 129, 129))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Mid, QColor(94, 94, 94))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Mid, QColor(94, 94, 94))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Mid, QColor(94, 94, 94))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Shadow, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Shadow, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Shadow, QColor(155, 155, 155))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Highlight, QColor(14, 14, 14))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Highlight, QColor(60, 60, 60))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Highlight, QColor(34, 34, 34))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.HighlightedText, QColor(83, 83, 83))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.HighlightedText, QColor(255, 255, 255))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.HighlightedText, QColor(240, 240, 240))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.Link, QColor(34, 34, 74))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.Link, QColor(100, 100, 230))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.Link, QColor(100, 100, 230))
        self.fPalBlack.setColor(QPalette.Disabled, QPalette.LinkVisited, QColor(74, 34, 74))
        self.fPalBlack.setColor(QPalette.Active,   QPalette.LinkVisited, QColor(230, 100, 230))
        self.fPalBlack.setColor(QPalette.Inactive, QPalette.LinkVisited, QColor(230, 100, 230))
        self.fApp.setPalette(self.fPalBlack)

    def createPaletteBlue(self):
        self.fPalBlue = QPalette()
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Window, QColor(32, 35, 39))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Window, QColor(37, 40, 45))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Window, QColor(37, 40, 45))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.WindowText, QColor(89, 95, 104))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.WindowText, QColor(223, 237, 255))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.WindowText, QColor(223, 237, 255))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Base, QColor(48, 53, 60))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Base, QColor(55, 61, 69))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Base, QColor(55, 61, 69))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.AlternateBase, QColor(60, 64, 67))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.AlternateBase, QColor(69, 73, 77))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.AlternateBase, QColor(69, 73, 77))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.ToolTipBase, QColor(182, 193, 208))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.ToolTipBase, QColor(182, 193, 208))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.ToolTipBase, QColor(182, 193, 208))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.ToolTipText, QColor(42, 44, 48))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.ToolTipText, QColor(42, 44, 48))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.ToolTipText, QColor(42, 44, 48))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Text, QColor(96, 103, 113))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Text, QColor(210, 222, 240))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Text, QColor(210, 222, 240))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Button, QColor(51, 55, 62))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Button, QColor(59, 63, 71))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Button, QColor(59, 63, 71))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(98, 104, 114))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.ButtonText, QColor(210, 222, 240))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.ButtonText, QColor(210, 222, 240))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.BrightText, QColor(255, 255, 255))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Light, QColor(59, 64, 72))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Light, QColor(63, 68, 76))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Light, QColor(63, 68, 76))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Midlight, QColor(48, 52, 59))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Midlight, QColor(51, 56, 63))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Midlight, QColor(51, 56, 63))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Dark, QColor(18, 19, 22))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Dark, QColor(20, 22, 25))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Dark, QColor(20, 22, 25))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Mid, QColor(28, 30, 34))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Mid, QColor(32, 35, 39))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Mid, QColor(32, 35, 39))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Shadow, QColor(13, 14, 16))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Shadow, QColor(15, 16, 18))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Shadow, QColor(15, 16, 18))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Highlight, QColor(32, 35, 39))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Highlight, QColor(14, 14, 17))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Highlight, QColor(27, 28, 33))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.HighlightedText, QColor(89, 95, 104))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.HighlightedText, QColor(217, 234, 253))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.HighlightedText, QColor(223, 237, 255))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.Link, QColor(79, 100, 118))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.Link, QColor(156, 212, 255))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.Link, QColor(156, 212, 255))
        self.fPalBlue.setColor(QPalette.Disabled, QPalette.LinkVisited, QColor(51, 74, 118))
        self.fPalBlue.setColor(QPalette.Active,   QPalette.LinkVisited, QColor(64, 128, 255))
        self.fPalBlue.setColor(QPalette.Inactive, QPalette.LinkVisited, QColor(64, 128, 255))
        self.fApp.setPalette(self.fPalBlue)

    def exec_(self):
        return self.fApp.exec_()

    def exit_exec(self):
        return sys.exit(self.fApp.exec_())

    def getApp(self):
        return self.fApp

    def quit(self):
        self.fApp.quit()

# ------------------------------------------------------------------------------------------------------------
