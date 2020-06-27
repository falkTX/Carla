#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas engine using QGraphicsView/Scene
# Copyright (C) 2010-2019 Filipe Coelho <falktx@falktx.com>
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

from PyQt5.QtCore import QAbstractAnimation
from PyQt5.QtWidgets import QGraphicsObject

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import canvas, CanvasBoxType

# ------------------------------------------------------------------------------------------------------------

class CanvasFadeAnimation(QAbstractAnimation):
    def __init__(self, item, show):
        QAbstractAnimation.__init__(self)

        self.m_show = show
        self.m_duration = 0
        self.m_item = item
        self.m_item_is_object = isinstance(item, QGraphicsObject)

    def item(self):
        return self.m_item

    def forceStop(self):
        self.blockSignals(True)
        self.stop()
        self.blockSignals(False)

    def setDuration(self, time):
        if self.m_item.opacity() == 0 and not self.m_show:
            self.m_duration = 0
        else:
            if self.m_item_is_object:
                self.m_item.blockSignals(True)
                self.m_item.show()
                self.m_item.blockSignals(False)
            else:
                self.m_item.show()
            self.m_duration = time

    def duration(self):
        return self.m_duration

    def updateCurrentTime(self, time):
        if self.m_duration == 0:
            return

        if self.m_show:
            value = float(time) / self.m_duration
        else:
            value = 1.0 - (float(time) / self.m_duration)

        try:
            self.m_item.setOpacity(value)
        except RuntimeError:
            print("CanvasFadeAnimation::updateCurrentTime() - failed to animate canvas item, already destroyed?")
            self.forceStop()
            canvas.animation_list.remove(self)
            return

        if self.m_item.type() == CanvasBoxType:
            self.m_item.setShadowOpacity(value)

    def updateDirection(self, direction):
        pass

    def updateState(self, oldState, newState):
        pass

# ------------------------------------------------------------------------------------------------------------
