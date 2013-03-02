#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# PatchBay Canvas Themes
# Copyright (C) 2010-2012 Filipe Coelho <falktx@falktx.com>
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

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QFont, QPen, QPixmap

# ------------------------------------------------------------------------------------------------------------
# patchcanvas-theme.cpp

class Theme(object):
    # enum PortType
    THEME_PORT_SQUARE  = 0
    THEME_PORT_POLYGON = 1

    # enum List
    THEME_MODERN_DARK      = 0
    THEME_MODERN_DARK_TINY = 1
    THEME_MODERN_LIGHT     = 2
    THEME_CLASSIC_DARK     = 3
    THEME_OOSTUDIO         = 4
    THEME_MAX              = 5

    # enum BackgroundType
    THEME_BG_SOLID    = 0
    THEME_BG_GRADIENT = 1

    def __init__(self, idx):
        object.__init__(self)

        self.idx = idx

        if idx == self.THEME_MODERN_DARK:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt.DashLine)
            self.box_bg_1 = QColor(32, 34, 35)
            self.box_bg_2 = QColor(43, 47, 48)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_header_pixmap  = None
            self.box_header_height  = 24
            self.box_header_spacing = 0

            self.box_text = QPen(QColor(240, 240, 240), 0)
            self.box_text_sel  = self.box_text
            self.box_text_ypos = 16
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 8
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = True

            # Ports
            self.port_text = QPen(QColor(250, 250, 250), 0)
            self.port_text_ypos = 12
            self.port_bg_pixmap = None
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_POLYGON

            self.port_audio_jack_pen = QPen(QColor(63, 90, 126), 1)
            self.port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1)
            self.port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
            self.port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1)
            self.port_midi_a2j_pen = QPen(QColor(137, 76, 43), 1)
            self.port_midi_a2j_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)
            self.port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
            self.port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50)
            self.port_midi_a2j_bg = QColor(101, 47, 16)
            self.port_midi_a2j_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)
            self.port_midi_alsa_bg = QColor(64, 112, 18)
            self.port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_height   = 15
            self.port_offset   = 0
            self.port_spacing  = 3
            self.port_spacingT = 2

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 76, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_MODERN_DARK_TINY:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(206, 207, 208), 1, Qt.DashLine)
            self.box_bg_1 = QColor(32, 34, 35)
            self.box_bg_2 = QColor(43, 47, 48)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_header_pixmap  = None
            self.box_header_height  = 14
            self.box_header_spacing = 0

            self.box_text = QPen(QColor(240, 240, 240), 0)
            self.box_text_sel  = self.box_text
            self.box_text_ypos = 10
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 7
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = False

            # Ports
            self.port_text = QPen(QColor(250, 250, 250), 0)
            self.port_text_ypos = 9
            self.port_bg_pixmap = None
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 6
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_POLYGON

            self.port_audio_jack_pen = QPen(QColor(63, 90, 126), 1)
            self.port_audio_jack_pen_sel = QPen(QColor(63 + 30, 90 + 30, 126 + 30), 1)
            self.port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
            self.port_midi_jack_pen_sel = QPen(QColor(159 + 30, 44 + 30, 42 + 30), 1)
            self.port_midi_a2j_pen = QPen(QColor(137, 76, 43), 1)
            self.port_midi_a2j_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)
            self.port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
            self.port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_audio_jack_bg_sel = QColor(35 + 50, 61 + 50, 99 + 50)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_jack_bg_sel = QColor(120 + 50, 15 + 50, 16 + 50)
            self.port_midi_a2j_bg = QColor(101, 47, 16)
            self.port_midi_a2j_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)
            self.port_midi_alsa_bg = QColor(64, 112, 18)
            self.port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_height   = 11
            self.port_offset   = 0
            self.port_spacing  = 2
            self.port_spacingT = 1

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 90, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 90, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 76, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_MODERN_LIGHT:
            # Canvas
            self.canvas_bg = QColor(248, 249, 250)

            # Boxes
            self.box_pen = QPen(QColor(176, 177, 178), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(1, 2, 3), 2, Qt.DashLine)
            self.box_bg_1 = QColor(250, 250, 250)
            self.box_bg_2 = QColor(200, 200, 200)
            self.box_shadow = QColor(1, 1, 1, 100)
            self.box_header_pixmap  = None
            self.box_header_height  = 24
            self.box_header_spacing = 0

            self.box_text = QPen(QColor(1, 1, 1), 0)
            self.box_text_sel  = self.box_text
            self.box_text_ypos = 16
            self.box_font_name = "Ubuntu"
            self.box_font_size = 11
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = True

            # Ports
            self.port_text = QPen(QColor(255, 255, 255), 1)
            self.port_text_ypos = 12
            self.port_bg_pixmap = None
            self.port_font_name = "Ubuntu"
            self.port_font_size = 10
            self.port_font_state = QFont.Bold
            self.port_mode = self.THEME_PORT_POLYGON

            self.port_audio_jack_pen = QPen(QColor(103, 130, 166), 2)
            self.port_audio_jack_pen_sel = QPen(QColor(103 + 136, 190 + 130, 226 + 130), 1)
            self.port_midi_jack_pen = QPen(QColor(159, 44, 42), 1)
            self.port_midi_jack_pen_sel = QPen(QColor(90 + 30, 44 + 30, 42 + 30), 1)
            self.port_midi_a2j_pen = QPen(QColor(137, 76, 43), 1)
            self.port_midi_a2j_pen_sel = QPen(QColor(137 + 30, 76 + 30, 43 + 30), 1)
            self.port_midi_alsa_pen = QPen(QColor(93, 141, 46), 1)
            self.port_midi_alsa_pen_sel = QPen(QColor(93 + 30, 141 + 30, 46 + 30), 1)

            self.port_audio_jack_bg = QColor(0, 0, 180)
            self.port_audio_jack_bg_sel = QColor(135 + 150, 161 + 150, 199 + 150)
            self.port_midi_jack_bg = QColor(130, 15, 16)
            self.port_midi_jack_bg_sel = QColor(90 + 30, 15 + 50, 16 + 50)
            self.port_midi_a2j_bg = QColor(101, 47, 16)
            self.port_midi_a2j_bg_sel = QColor(101 + 50, 47 + 50, 16 + 50)
            self.port_midi_alsa_bg = QColor(64, 112, 18)
            self.port_midi_alsa_bg_sel = QColor(64 + 50, 112 + 50, 18 + 50)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_height   = 15
            self.port_offset   = 0
            self.port_spacing  = 3
            self.port_spacingT = 2

            # Lines
            self.line_audio_jack = QColor(63, 90, 126)
            self.line_audio_jack_sel = QColor(63 + 63, 90 + 90, 126 + 90)
            self.line_audio_jack_glow = QColor(100, 100, 200)
            self.line_midi_jack = QColor(159, 44, 42)
            self.line_midi_jack_sel = QColor(159 + 44, 44 + 90, 42 + 90)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(137, 43, 43)
            self.line_midi_a2j_sel = QColor(137 + 90, 76 + 90, 43 + 90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(93, 141, 46)
            self.line_midi_alsa_sel = QColor(93 + 90, 141 + 90, 46 + 90)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(206, 207, 208), 1, Qt.SolidLine)
            self.rubberband_brush = QColor(76, 77, 78, 100)

        elif idx == self.THEME_CLASSIC_DARK:
            # Canvas
            self.canvas_bg = QColor(0, 0, 0)

            # Boxes
            self.box_pen = QPen(QColor(147 - 70, 151 - 70, 143 - 70), 2, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(147, 151, 143), 2, Qt.DashLine)
            self.box_bg_1 = QColor(30, 34, 36)
            self.box_bg_2 = QColor(30, 34, 36)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_header_pixmap  = None
            self.box_header_height  = 19
            self.box_header_spacing = 0

            self.box_text = QPen(QColor(255, 255, 255), 0)
            self.box_text_sel  = self.box_text
            self.box_text_ypos = 12
            self.box_font_name = "Sans"
            self.box_font_size = 9
            self.box_font_state = QFont.Normal

            self.box_bg_type  = self.THEME_BG_GRADIENT
            self.box_use_icon = False

            # Ports
            self.port_text = QPen(QColor(250, 250, 250), 0)
            self.port_text_ypos = 11
            self.port_bg_pixmap = None
            self.port_font_name = "Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_SQUARE

            self.port_audio_jack_pen = QPen(QColor(35, 61, 99), 0)
            self.port_audio_jack_pen_sel = QPen(QColor(255, 0, 0), 0)
            self.port_midi_jack_pen = QPen(QColor(120, 15, 16), 0)
            self.port_midi_jack_pen_sel = QPen(QColor(255, 0, 0), 0)
            self.port_midi_a2j_pen = QPen(QColor(101, 47, 17), 0)
            self.port_midi_a2j_pen_sel = QPen(QColor(255, 0, 0), 0)
            self.port_midi_alsa_pen = QPen(QColor(63, 112, 19), 0)
            self.port_midi_alsa_pen_sel = QPen(QColor(255, 0, 0), 0)

            self.port_audio_jack_bg = QColor(35, 61, 99)
            self.port_audio_jack_bg_sel = QColor(255, 0, 0)
            self.port_midi_jack_bg = QColor(120, 15, 16)
            self.port_midi_jack_bg_sel = QColor(255, 0, 0)
            self.port_midi_a2j_bg = QColor(101, 47, 17)
            self.port_midi_a2j_bg_sel = QColor(255, 0, 0)
            self.port_midi_alsa_bg = QColor(63, 112, 19)
            self.port_midi_alsa_bg_sel = QColor(255, 0, 0)

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_text
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_text
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_text
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_text

            self.port_height   = 14
            self.port_offset   = -1
            self.port_spacing  = 1
            self.port_spacingT = 0

            # Lines
            self.line_audio_jack = QColor(53, 78, 116)
            self.line_audio_jack_sel = QColor(255, 0, 0)
            self.line_audio_jack_glow = QColor(255, 0, 0)
            self.line_midi_jack = QColor(139, 32, 32)
            self.line_midi_jack_sel = QColor(255, 0, 0)
            self.line_midi_jack_glow = QColor(255, 0, 0)
            self.line_midi_a2j = QColor(120, 65, 33)
            self.line_midi_a2j_sel = QColor(255, 0, 0)
            self.line_midi_a2j_glow = QColor(255, 0, 0)
            self.line_midi_alsa = QColor(81, 130, 36)
            self.line_midi_alsa_sel = QColor(255, 0, 0)
            self.line_midi_alsa_glow = QColor(255, 0, 0)

            self.rubberband_pen = QPen(QColor(147, 151, 143), 2, Qt.SolidLine)
            self.rubberband_brush = QColor(35, 61, 99, 100)

        elif idx == self.THEME_OOSTUDIO:
            # Canvas
            self.canvas_bg = QColor(11, 11, 11)

            # Boxes
            self.box_pen = QPen(QColor(76, 77, 78), 1, Qt.SolidLine)
            self.box_pen_sel = QPen(QColor(189, 122, 214), 1, Qt.DashLine)
            self.box_bg_1 = QColor(46, 46, 46)
            self.box_bg_2 = QColor(23, 23, 23)
            self.box_shadow = QColor(89, 89, 89, 180)
            self.box_header_pixmap  = QPixmap(":/bitmaps/canvas/frame_node_header.png")
            self.box_header_height  = 22
            self.box_header_spacing = 6

            self.box_text = QPen(QColor(144, 144, 144), 0)
            self.box_text_sel  = QPen(QColor(189, 122, 214), 0)
            self.box_text_ypos = 16
            self.box_font_name = "Deja Vu Sans"
            self.box_font_size = 8
            self.box_font_state = QFont.Bold

            self.box_bg_type  = self.THEME_BG_SOLID
            self.box_use_icon = False

            # Ports
            normalPortBG = QColor(46, 46, 46)
            selPortBG = QColor(23, 23, 23)

            self.port_text = QPen(QColor(155, 155, 155), 0)
            self.port_text_ypos = 14
            self.port_bg_pixmap = QPixmap(":/bitmaps/canvas/frame_port_bg.png")
            self.port_font_name = "Deja Vu Sans"
            self.port_font_size = 8
            self.port_font_state = QFont.Normal
            self.port_mode = self.THEME_PORT_SQUARE

            self.port_audio_jack_pen = QPen(selPortBG, 2)
            self.port_audio_jack_pen_sel = QPen(QColor(1, 230, 238), 1)
            self.port_midi_jack_pen = QPen(selPortBG, 2)
            self.port_midi_jack_pen_sel = QPen(QColor(252, 118, 118), 1)
            self.port_midi_a2j_pen = QPen(selPortBG, 2)
            self.port_midi_a2j_pen_sel = QPen(QColor(137, 76, 43), 1)
            self.port_midi_alsa_pen = QPen(selPortBG, 2)
            self.port_midi_alsa_pen_sel = QPen(QColor(129, 244, 118), 0)

            self.port_audio_jack_bg = normalPortBG
            self.port_audio_jack_bg_sel = selPortBG
            self.port_midi_jack_bg = normalPortBG
            self.port_midi_jack_bg_sel = selPortBG
            self.port_midi_a2j_bg = normalPortBG
            self.port_midi_a2j_bg_sel = selPortBG
            self.port_midi_alsa_bg = normalPortBG
            self.port_midi_alsa_bg_sel = selPortBG

            self.port_audio_jack_text = self.port_text
            self.port_audio_jack_text_sel = self.port_audio_jack_pen_sel
            self.port_midi_jack_text = self.port_text
            self.port_midi_jack_text_sel = self.port_midi_jack_pen_sel
            self.port_midi_a2j_text = self.port_text
            self.port_midi_a2j_text_sel = self.port_midi_a2j_pen_sel
            self.port_midi_alsa_text = self.port_text
            self.port_midi_alsa_text_sel = self.port_midi_alsa_pen_sel

            # missing, ports 2
            self.port_height   = 19
            self.port_offset   = 1
            self.port_spacing  = 5
            self.port_spacingT = 0

            # Lines
            self.line_audio_jack = QColor(64, 64, 64)
            self.line_audio_jack_sel = QColor(1, 230, 238)
            self.line_audio_jack_glow = QColor(100, 200, 100)
            self.line_midi_jack = QColor(64, 64, 64)
            self.line_midi_jack_sel = QColor(252, 118, 118)
            self.line_midi_jack_glow = QColor(200, 100, 100)
            self.line_midi_a2j = QColor(64, 64, 64)
            self.line_midi_a2j_sel = QColor(137+90, 76+90, 43+90)
            self.line_midi_a2j_glow = QColor(166, 133, 133)
            self.line_midi_alsa = QColor(64, 64, 64)
            self.line_midi_alsa_sel = QColor(129, 244, 118)
            self.line_midi_alsa_glow = QColor(100, 200, 100)

            self.rubberband_pen = QPen(QColor(1, 230, 238), 2, Qt.SolidLine)
            self.rubberband_brush = QColor(90, 90, 90, 100)

def getDefaultTheme():
    return Theme.THEME_MODERN_DARK

def getThemeName(idx):
    if idx == Theme.THEME_MODERN_DARK:
        return "Modern Dark"
    elif idx == Theme.THEME_MODERN_DARK_TINY:
        return "Modern Dark (Tiny)"
    elif idx == Theme.THEME_MODERN_LIGHT:
        return "Modern Light"
    elif idx == Theme.THEME_CLASSIC_DARK:
        return "Classic Dark"
    elif idx == Theme.THEME_OOSTUDIO:
        return "OpenOctave Studio"
    else:
        return ""

def getDefaultThemeName():
    return "Modern Dark"
