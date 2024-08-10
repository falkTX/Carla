#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtGui import QColor
    from PyQt5.QtWidgets import QGraphicsDropShadowEffect
elif qt_config == 6:
    from PyQt6.QtGui import QColor
    from PyQt6.QtWidgets import QGraphicsDropShadowEffect

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import canvas

# ------------------------------------------------------------------------------------------------------------

class CanvasBoxShadow(QGraphicsDropShadowEffect):
    def __init__(self, parent):
        QGraphicsDropShadowEffect.__init__(self, parent)

        self.m_fakeParent = None

        self.setBlurRadius(20)
        self.setColor(canvas.theme.box_shadow)
        self.setOffset(0, 0)

    def setFakeParent(self, fakeParent):
        self.m_fakeParent = fakeParent

    def setOpacity(self, opacity):
        color = QColor(canvas.theme.box_shadow)
        color.setAlphaF(opacity)
        self.setColor(color)

    def draw(self, painter):
        if self.m_fakeParent:
            self.m_fakeParent.repaintLines()
        QGraphicsDropShadowEffect.draw(self, painter)

# ------------------------------------------------------------------------------------------------------------
