#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla rack widget code
# Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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
# Imports (Config)

from carla_config import *

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

if config_UseQt5:
    from PyQt5.QtCore import Qt, QSize, QTimer
    from PyQt5.QtGui import QPixmap
    from PyQt5.QtWidgets import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem
else:
    from PyQt4.QtCore import Qt, QSize, QTimer
    from PyQt4.QtGui import QAbstractItemView, QApplication, QHBoxLayout, QLabel, QListWidget, QListWidgetItem, QPixmap

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import *
from carla_skin import *

# ------------------------------------------------------------------------------------------------------------
# Rack widget list

class CarlaRackList(QListWidget):
    def __init__(self, parent, host):
        QListWidget.__init__(self, parent)
        self.host = host

# ------------------------------------------------------------------------------------------------------------
# Rack widget

class CarlaRackW(QFrame):
#class CarlaRackW(QFrame, HostWidgetMeta, metaclass=PyQtMetaClass):
    def __init__(self, parent, host, doSetup = True):
        QFrame.__init__(self, parent)
        self.host = host
