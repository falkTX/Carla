#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from qt_compat import qt_config

if qt_config == 5:
    from PyQt5.QtCore import qCritical, QRectF
    from PyQt5.QtGui import QPainter
    from PyQt5.QtSvg import QGraphicsSvgItem, QSvgRenderer
    from PyQt5.QtWidgets import QGraphicsColorizeEffect
elif qt_config == 6:
    from PyQt6.QtCore import qCritical, QRectF
    from PyQt6.QtGui import QPainter
    from PyQt6.QtSvg import QSvgRenderer
    from PyQt6.QtSvgWidgets import QGraphicsSvgItem
    from PyQt6.QtWidgets import QGraphicsColorizeEffect

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    icon2str,
    CanvasIconType,
    ICON_APPLICATION,
    ICON_HARDWARE,
    ICON_DISTRHO,
    ICON_FILE,
    ICON_PLUGIN,
    ICON_LADISH_ROOM,
)

# ------------------------------------------------------------------------------------------------------------

class CanvasIcon(QGraphicsSvgItem):
    def __init__(self, icon, name, parent):
        QGraphicsSvgItem.__init__(self)
        self.setParentItem(parent)

        self.m_renderer = None
        self.p_size = QRectF(0, 0, 0, 0)

        self.m_colorFX = QGraphicsColorizeEffect(self)
        self.m_colorFX.setColor(canvas.theme.box_text.color())

        self.setGraphicsEffect(self.m_colorFX)
        self.setIcon(icon, name)

    def setIcon(self, icon, name):
        name = name.lower()
        icon_path = ""

        if icon == ICON_APPLICATION:
            self.p_size = QRectF(3, 2, 19, 18)

            if "audacious" in name:
                icon_path = ":/scalable/pb_audacious.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "clementine" in name:
                icon_path = ":/scalable/pb_clementine.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "distrho" in name:
                icon_path = ":/scalable/pb_distrho.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "jamin" in name:
                icon_path = ":/scalable/pb_jamin.svg"
                self.p_size = QRectF(5, 3, 16, 16)
            elif "mplayer" in name:
                icon_path = ":/scalable/pb_mplayer.svg"
                self.p_size = QRectF(5, 4, 16, 16)
            elif "vlc" in name:
                icon_path = ":/scalable/pb_vlc.svg"
                self.p_size = QRectF(5, 3, 16, 16)

            else:
                icon_path = ":/scalable/pb_generic.svg"
                self.p_size = QRectF(4, 3, 16, 16)

        elif icon == ICON_HARDWARE:
            icon_path = ":/scalable/pb_hardware.svg"
            self.p_size = QRectF(5, 2, 16, 16)

        elif icon == ICON_DISTRHO:
            icon_path = ":/scalable/pb_distrho.svg"
            self.p_size = QRectF(5, 4, 16, 16)

        elif icon == ICON_FILE:
            icon_path = ":/scalable/pb_file.svg"
            self.p_size = QRectF(5, 4, 16, 16)

        elif icon == ICON_PLUGIN:
            icon_path = ":/scalable/pb_plugin.svg"
            self.p_size = QRectF(5, 4, 16, 16)

        elif icon == ICON_LADISH_ROOM:
            # TODO - make a unique ladish-room icon
            icon_path = ":/scalable/pb_hardware.svg"
            self.p_size = QRectF(5, 2, 16, 16)

        else:
            self.p_size = QRectF(0, 0, 0, 0)
            qCritical("PatchCanvas::CanvasIcon.setIcon(%s, %s) - unsupported icon requested" % (
                      icon2str(icon), name.encode()))
            return

        self.m_renderer = QSvgRenderer(icon_path, canvas.scene)
        self.setSharedRenderer(self.m_renderer)
        self.update()

    def type(self):
        return CanvasIconType

    def boundingRect(self):
        return self.p_size

    def paint(self, painter, option, widget):
        if not self.m_renderer:
            QGraphicsSvgItem.paint(self, painter, option, widget)
            return

        painter.save()
        painter.setRenderHint(QPainter.Antialiasing, False)
        painter.setRenderHint(QPainter.TextAntialiasing, False)
        self.m_renderer.render(painter, self.p_size)
        painter.restore()

# ------------------------------------------------------------------------------------------------------------
