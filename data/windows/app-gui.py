#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

from carla_host import VERSION
from os import getenv

# ------------------------------------------------------------------------------------------------------------

name = getenv("SCRIPT_NAME")

if name == "Carla":
    description = "Carla Plugin Host"
    build_exe = ".\\build\\Carla\\"
elif name == "Carla-Control":
    description = "Carla Remote Control"
    build_exe = ".\\build\\Carla-Control\\"
else:
    description = name
    build_exe = ".\\build\\{}-resources\\".format(name)

options = {
  "zip_include_packages": ["*"],
  "zip_exclude_packages": ["PyQt5"],
  "replace_paths": [["*",".\\lib\\"]],
  "build_exe": build_exe,
  "optimize": True,
}

exe_options = {
  "script": ".\\source\\frontend\\{}".format(name),
  "icon": ".\\resources\\ico\\carla.ico",
  "copyright": "Copyright (C) 2011-2021 Filipe Coelho",
  "base": "Win32GUI",
  "targetName": "{}.exe".format(name),
}

setup(name = name,
      version = VERSION,
      description = description,
      options = {"build_exe": options},
      executables = [Executable(**exe_options)])

# ------------------------------------------------------------------------------------------------------------
