#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ------------------------------------------------------------------------------------------------------------
# Imports (cx_Freeze)

from cx_Freeze import setup, Executable

# ------------------------------------------------------------------------------------------------------------
# Imports (Custom Stuff)

# from carla_host import *

# ------------------------------------------------------------------------------------------------------------

options = {
  "packages": ["re", "sip", "subprocess", "inspect"]
}

setup(name = "Carla",
      version = "1.9.3",
      description = "Carla Plugin Host",
      options = {"bdist_exe": options},
      executables = [Executable("./source/carla.pyw")])

# ------------------------------------------------------------------------------------------------------------
