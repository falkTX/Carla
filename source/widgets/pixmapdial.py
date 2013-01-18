#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Pixmap Dial, a custom Qt4 widget
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
from math import floor
from PyQt4.QtCore import Qt, QPointF, QRectF, QTimer, QSize, SLOT
from PyQt4.QtGui import QColor, QConicalGradient, QDial, QFontMetrics, QLinearGradient, QPainter, QPainterPath, QPen, QPixmap

# Widget Class
class PixmapDial(QDial):
    # enum Orientation
    HORIZONTAL = 0
    VERTICAL   = 1

    # enum CustomPaint
    CUSTOM_PAINT_NULL      = 0
    CUSTOM_PAINT_CARLA_WET = 1
    CUSTOM_PAINT_CARLA_VOL = 2
    CUSTOM_PAINT_CARLA_L   = 3
    CUSTOM_PAINT_CARLA_R   = 4

    HOVER_MIN = 0
    HOVER_MAX = 9

    def __init__(self, parent):
        QDial.__init__(self, parent)

        self.m_pixmap = QPixmap(":/bitmaps/dial_01d.png")
        self.m_pixmap_n_str = "01"
        self.m_custom_paint = self.CUSTOM_PAINT_NULL

        self.m_hovered    = False
        self.m_hover_step = self.HOVER_MIN

        if self.m_pixmap.width() > self.m_pixmap.height():
            self.m_orientation = self.HORIZONTAL
        else:
            self.m_orientation = self.VERTICAL

        self.m_label = ""
        self.m_label_pos = QPointF(0.0, 0.0)
        self.m_label_width = 0
        self.m_label_height = 0
        self.m_label_gradient = QLinearGradient(0, 0, 0, 1)

        if self.palette().window().color().lightness() > 100:
            # Light background
            c = self.palette().dark().color()
            self.m_color1 = c
            self.m_color2 = QColor(c.red(), c.green(), c.blue(), 0)
            self.m_colorT = [self.palette().buttonText().color(), self.palette().mid().color()]
        else:
            # Dark background
            self.m_color1 = QColor(0, 0, 0, 255)
            self.m_color2 = QColor(0, 0, 0, 0)
            self.m_colorT = [Qt.white, Qt.darkGray]

        self.updateSizes()

    def getSize(self):
        return self.p_size

    def setCustomPaint(self, paint):
        self.m_custom_paint = paint
        self.update()

    def setEnabled(self, enabled):
        if self.isEnabled() != enabled:
            self.m_pixmap.load(":/bitmaps/dial_%s%s.png" % (self.m_pixmap_n_str, "" if enabled else "d"))
            self.updateSizes()
            self.update()
        QDial.setEnabled(self, enabled)

    def setLabel(self, label):
        self.m_label = label

        self.m_label_width  = QFontMetrics(self.font()).width(label)
        self.m_label_height = QFontMetrics(self.font()).height()

        self.m_label_pos.setX(float(self.p_size)/2 - float(self.m_label_width)/2)
        self.m_label_pos.setY(self.p_size + self.m_label_height)

        self.m_label_gradient.setColorAt(0.0, self.m_color1)
        self.m_label_gradient.setColorAt(0.6, self.m_color1)
        self.m_label_gradient.setColorAt(1.0, self.m_color2)

        self.m_label_gradient.setStart(0, float(self.p_size)/2)
        self.m_label_gradient.setFinalStop(0, self.p_size + self.m_label_height + 5)

        self.m_label_gradient_rect = QRectF(float(self.p_size)/8, float(self.p_size)/2, float(self.p_size)*6/8, self.p_size+self.m_label_height+5)
        self.update()

    def setPixmap(self, pixmapId):
        self.m_pixmap_n_str = "%02i" % pixmapId
        self.m_pixmap.load(":/bitmaps/dial_%s%s.png" % (self.m_pixmap_n_str, "" if self.isEnabled() else "d"))

        if self.m_pixmap.width() > self.m_pixmap.height():
            self.m_orientation = self.HORIZONTAL
        else:
            self.m_orientation = self.VERTICAL

        self.updateSizes()
        self.update()

    def minimumSizeHint(self):
        return QSize(self.p_size, self.p_size)

    def sizeHint(self):
        return QSize(self.p_size, self.p_size)

    def updateSizes(self):
        self.p_width  = self.m_pixmap.width()
        self.p_height = self.m_pixmap.height()

        if self.p_width < 1:
            self.p_width = 1

        if self.p_height < 1:
            self.p_height = 1

        if self.m_orientation == self.HORIZONTAL:
            self.p_size  = self.p_height
            self.p_count = self.p_width / self.p_height
        else:
            self.p_size  = self.p_width
            self.p_count = self.p_height / self.p_width

        self.setMinimumSize(self.p_size, self.p_size + self.m_label_height + 5)
        self.setMaximumSize(self.p_size, self.p_size + self.m_label_height + 5)

    def enterEvent(self, event):
        self.m_hovered = True
        if self.m_hover_step  == self.HOVER_MIN:
            self.m_hover_step += 1
        QDial.enterEvent(self, event)

    def leaveEvent(self, event):
        self.m_hovered = False
        if self.m_hover_step  == self.HOVER_MAX:
            self.m_hover_step -= 1
        QDial.leaveEvent(self, event)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, True)

        if self.m_label:
            painter.setPen(self.m_color2)
            painter.setBrush(self.m_label_gradient)
            painter.drawRect(self.m_label_gradient_rect)

            painter.setPen(self.m_colorT[0 if self.isEnabled() else 1])
            painter.drawText(self.m_label_pos, self.m_label)

        if self.isEnabled():
            current = float(self.value() - self.minimum())
            divider = float(self.maximum() - self.minimum())

            if divider == 0.0:
                return

            value  = current / divider
            target = QRectF(0.0, 0.0, self.p_size, self.p_size)

            per = int((self.p_count - 1) * value)

            if self.m_orientation == self.HORIZONTAL:
                xpos = self.p_size * per
                ypos = 0.0
            else:
                xpos = 0.0
                ypos = self.p_size * per

            source = QRectF(xpos, ypos, self.p_size, self.p_size)
            painter.drawPixmap(target, self.m_pixmap, source)

            # Custom knobs (Dry/Wet and Volume)
            if self.m_custom_paint in (self.CUSTOM_PAINT_CARLA_WET, self.CUSTOM_PAINT_CARLA_VOL):
                # knob color
                colorGreen = QColor(0x5D, 0xE7, 0x3D, 191 + self.m_hover_step*7)
                colorBlue  = QColor(0x3E, 0xB8, 0xBE, 191 + self.m_hover_step*7)

                # draw small circle
                ballRect = QRectF(8.0, 8.0, 15.0, 15.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*value)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                # draw arc
                startAngle = 216*16
                spanAngle  = -252*16*value

                if self.m_custom_paint == self.CUSTOM_PAINT_CARLA_WET:
                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 0))
                    painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2))

                    gradient = QConicalGradient(15.5, 15.5, -45)
                    gradient.setColorAt(0.0,   colorBlue)
                    gradient.setColorAt(0.125, colorBlue)
                    gradient.setColorAt(0.625, colorGreen)
                    gradient.setColorAt(0.75,  colorGreen)
                    gradient.setColorAt(0.76,  colorGreen)
                    gradient.setColorAt(1.0,   colorGreen)
                    painter.setBrush(gradient)
                    painter.setPen(QPen(gradient, 3))

                else:
                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 0))
                    painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.2, 2.2))

                    painter.setBrush(colorBlue)
                    painter.setPen(QPen(colorBlue, 3))

                painter.drawArc(4.0, 4.0, 26.0, 26.0, startAngle, spanAngle)

            # Custom knobs (L and R)
            elif self.m_custom_paint in (self.CUSTOM_PAINT_CARLA_L, self.CUSTOM_PAINT_CARLA_R):
                # knob color
                color = QColor(0xAD + self.m_hover_step*5, 0xD5 + self.m_hover_step*4, 0x4B + self.m_hover_step*5)

                # draw small circle
                ballRect = QRectF(7.0, 8.0, 11.0, 12.0)
                ballPath = QPainterPath()
                ballPath.addEllipse(ballRect)
                #painter.drawRect(ballRect)
                tmpValue  = (0.375 + 0.75*value)
                ballValue = tmpValue - floor(tmpValue)
                ballPoint = ballPath.pointAtPercent(ballValue)

                painter.setBrush(color)
                painter.setPen(QPen(color, 0))
                painter.drawEllipse(QRectF(ballPoint.x(), ballPoint.y(), 2.0, 2.0))

                # draw arc
                if self.m_custom_paint == self.CUSTOM_PAINT_CARLA_L:
                    startAngle = 216*16
                    spanAngle  = -252.0*16*value
                elif self.m_custom_paint == self.CUSTOM_PAINT_CARLA_R:
                    startAngle = 324.0*16
                    spanAngle  = 252.0*16*(1.0-value)
                else:
                    return

                painter.setPen(QPen(color, 2))
                painter.drawArc(3.5, 4.5, 22.0, 22.0, startAngle, spanAngle)

            if self.HOVER_MIN < self.m_hover_step < self.HOVER_MAX:
                self.m_hover_step += 1 if self.m_hovered else -1
                QTimer.singleShot(20, self, SLOT("update()"))

        else:
            target = QRectF(0.0, 0.0, self.p_size, self.p_size)
            painter.drawPixmap(target, self.m_pixmap, target)

    def resizeEvent(self, event):
        self.updateSizes()
        QDial.resizeEvent(self, event)
