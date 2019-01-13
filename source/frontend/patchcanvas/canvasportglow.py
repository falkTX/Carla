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

from PyQt5.QtWidgets import QGraphicsDropShadowEffect

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom)

from . import (
    canvas,
    PORT_TYPE_AUDIO_JACK,
    PORT_TYPE_MIDI_ALSA,
    PORT_TYPE_MIDI_JACK,
    PORT_TYPE_PARAMETER,
)

# ------------------------------------------------------------------------------------------------------------

class CanvasPortGlow(QGraphicsDropShadowEffect):
    def __init__(self, port_type, parent):
        QGraphicsDropShadowEffect.__init__(self, parent)

        self.setBlurRadius(12)
        self.setOffset(0, 0)

        if port_type == PORT_TYPE_AUDIO_JACK:
            self.setColor(canvas.theme.line_audio_jack_glow)
        elif port_type == PORT_TYPE_MIDI_JACK:
            self.setColor(canvas.theme.line_midi_jack_glow)
        elif port_type == PORT_TYPE_MIDI_ALSA:
            self.setColor(canvas.theme.line_midi_alsa_glow)
        elif port_type == PORT_TYPE_PARAMETER:
            self.setColor(canvas.theme.line_parameter_glow)

# ------------------------------------------------------------------------------------------------------------
