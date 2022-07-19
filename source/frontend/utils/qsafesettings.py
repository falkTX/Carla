#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla plugin host
# Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

# ---------------------------------------------------------------------------------------------------------------------
# Imports (Global)

from PyQt5.QtCore import QSettings

# ---------------------------------------------------------------------------------------------------------------------
# Safer QSettings class, which does not throw if type mismatches

class QSafeSettings(QSettings):
    def value(self, key, defaultValue, valueType):
        if not isinstance(defaultValue, valueType):
            print("QSafeSettings.value() - defaultValue type mismatch for key", key)

        try:
            return QSettings.value(self, key, defaultValue, valueType)
        except:
            return defaultValue

# ---------------------------------------------------------------------------------------------------------------------
