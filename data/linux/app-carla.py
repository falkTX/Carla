#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION

# ------------------------------------------------------------------------------------------------------------

options = {
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["PyQt5"],
  "replace_paths": [["*","./lib/"]],
  "build_exe": "./build-carla",
  "optimize": True,
}

exe_options = {
  "script": "./source/frontend/carla",
  "targetName": "carla",
}

setup(name = "Carla",
      version = VERSION,
      description = "Carla Plugin Host",
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------

