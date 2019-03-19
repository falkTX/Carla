#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla bridge for LV2 modguis
# Copyright (C) 2015-2019 Filipe Coelho <falktx@falktx.com>
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
# Imports (local)

from carla_app import CarlaApplication, gCarla, getPaths
from carla_shared import DLL_EXTENSION, setUpSignals
from carla_utils import CarlaUtils

from modgui.host import HostWindow

# ------------------------------------------------------------------------------------------------------------
# Main

if __name__ == '__main__':
    # -------------------------------------------------------------
    # Read CLI args

    if len(sys.argv) < 2:
        print("usage: %s <plugin-uri>" % sys.argv[0])
        sys.exit(1)

    libPrefix = os.getenv("CARLA_LIB_PREFIX")

    # -------------------------------------------------------------
    # App initialization

    app = CarlaApplication("Carla2-MODGUI", libPrefix)

    # -------------------------------------------------------------
    # Init utils

    pathBinaries, pathResources = getPaths(libPrefix)

    utilsname = "libcarla_utils.%s" % (DLL_EXTENSION)

    gCarla.utils = CarlaUtils(os.path.join(pathBinaries, utilsname))
    gCarla.utils.set_process_name("carla-bridge-lv2-modgui")

    # -------------------------------------------------------------
    # Set-up custom signal handling

    setUpSignals()

    # -------------------------------------------------------------
    # Create GUI

    gui = HostWindow()

    # --------------------------------------------------------------------------------------------------------
    # App-Loop

    app.exit_exec()

# ------------------------------------------------------------------------------------------------------------
