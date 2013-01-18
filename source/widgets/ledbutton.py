#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Button, a custom Qt4 widget
# Copyright (C) 2011-2012 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the COPYING file

# Imports (Global)
from PyQt4.QtCore import qCritical, QRectF
from PyQt4.QtGui import QPainter, QPixmap, QPushButton

# Widget Class
class LEDButton(QPushButton):
    BLUE    = 1
    GREEN   = 2
    RED     = 3
    YELLOW  = 4
    BIG_RED = 5

    def __init__(self, parent):
        QPushButton.__init__(self, parent)

        self.m_pixmap = QPixmap()
        self.m_pixmap_rect = QRectF(0, 0, 0, 0)

        self.setCheckable(True)
        self.setText("")

        self.setColor(self.BLUE)

    def setColor(self, color):
        self.m_color = color

        if color in (self.BLUE, self.GREEN, self.RED, self.YELLOW):
            size = 14
        elif color == self.BIG_RED:
            size = 32
        else:
            return qCritical("LEDButton::setColor(%i) - Invalid color" % color)

        self.setPixmapSize(size)

    def setPixmapSize(self, size):
        self.m_pixmap_rect = QRectF(0, 0, size, size)

        self.setMinimumWidth(size)
        self.setMaximumWidth(size)
        self.setMinimumHeight(size)
        self.setMaximumHeight(size)

    def paintEvent(self, event):
        painter = QPainter(self)

        if self.isChecked():
            if self.m_color == self.BLUE:
                self.m_pixmap.load(":/bitmaps/led_blue.png")
            elif self.m_color == self.GREEN:
                self.m_pixmap.load(":/bitmaps/led_green.png")
            elif self.m_color == self.RED:
                self.m_pixmap.load(":/bitmaps/led_red.png")
            elif self.m_color == self.YELLOW:
                self.m_pixmap.load(":/bitmaps/led_yellow.png")
            elif self.m_color == self.BIG_RED:
                self.m_pixmap.load(":/bitmaps/led-big_on.png")
            else:
                return
        else:
            if self.m_color in (self.BLUE, self.GREEN, self.RED, self.YELLOW):
                self.m_pixmap.load(":/bitmaps/led_off.png")
            elif self.m_color == self.BIG_RED:
                self.m_pixmap.load(":/bitmaps/led-big_off.png")
            else:
                return

        painter.drawPixmap(self.m_pixmap_rect, self.m_pixmap, self.m_pixmap_rect)
