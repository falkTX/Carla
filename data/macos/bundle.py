#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION

# ------------------------------------------------------------------------------------------------------------

options  = {
  "packages": ["re", "sip", "subprocess", "inspect"]
}

boptions  = {
  "iconfile": "./resources/ico/carla.icns"
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options, "bdist_mac": boptions},
      executables = [Executable("./source/Carla.pyw")])

# ------------------------------------------------------------------------------------------------------------
