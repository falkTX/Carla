# -*- coding: utf-8 -*-

# Carla plugin host (main entry point)
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
# Imports
import sys
from carla_host import (
    CarlaApplication, setUpSignals,
    initHost, loadHostSettings)


# ------------------------------------------------------------------------------------------------------------
# Constants

MODE_HOST     = "carla"
MODE_RACK     = "carla-rack"
MODE_PATCHBAY = "carla-patchbay"
MODE_CONTROL  = "carla-control"


# ------------------------------------------------------------------------------------------------------------
# Print usage and/or version

def version(libPrefix):
    """ Print version information and exit. """
    from carla_shared import VERSION, PYQT_VERSION_STR, qVersion, getPaths
    pathBinaries, pathResources = getPaths(libPrefix)
    print()
    print("Carla %s" % VERSION)
    print("  Python version: %s" % sys.version.split(" ",1)[0])
    print("  Qt version:     %s" % qVersion())
    print("  PyQt version:   %s" % PYQT_VERSION_STR)
    print("  Binary dir:     %s" % pathBinaries)
    print("  Resources dir:  %s" % pathResources)


def usage(mode):
    print()
    if mode != MODE_CONTROL:
        print("%s: JACK %s%s%s." % (
            mode,
            "patchbay"    if mode == MODE_PATCHBAY or mode == MODE_HOST else "",
            " and "       if mode == MODE_HOST                          else "",
            "plugin host" if mode == MODE_RACK     or mode == MODE_HOST else ""))
        print()
        print("Usage: %s [OPTION]... [FILE]" % mode)
        print()
        print("  where FILE can point to a Carla session file to be loaded")
    else:
        print("%s: Control a remote Carla instance via OSC." % mode)
        print()
        print("Usage: %s [OPTION]... [OSC_URL]" % mode)
        print()
        print("  where OSC_URL needs to be in format osc.udp://host:port")
    print("  and OPTION can be one or more of the following:")
    print()
    print("  --version  Print version info and exit.")
    print("  --usage    Print command line usage info and exit.")
    print("  --help     Print both of the above, and exit.")
    print()
    print("  --with-appname=<name>    Set name of process.")
    print("  --with-libprefix=<path>  Look for libraries in specified directory.")


# ------------------------------------------------------------------------------------------------------------
# Main

def start(mode = MODE_HOST):

    # -------------------------------------------------------------
    # Import relevant modules depending on mode
    if mode != MODE_CONTROL:
        from carla_host import (
            HostWindow,
            ENGINE_PROCESS_MODE_CONTINUOUS_RACK,
            ENGINE_PROCESS_MODE_PATCHBAY)
        if mode == MODE_RACK:
            from carla_host import LADISH_APP_NAME, NSM_URL
    else:
        from carla_control import CarlaHostOSC, HostWindowOSC

    # -------------------------------------------------------------
    # Read CLI args

    initName  = os.path.basename(__file__) if ("__file__" in dir() and os.path.dirname(__file__) in PATH) else sys.argv[0]
    libPrefix = None
    oscAddr   = None

    for arg in sys.argv:
        if arg == "--version":
            version(libPrefix)
            sys.exit(0)

        elif arg == "--usage":
            usage(mode)
            sys.exit(0)

        elif arg == "--help":
            version(libPrefix)
            usage(mode)
            sys.exit(0)

        elif arg.startswith("--with-appname="):
            initName = os.path.basename(arg.replace("--with-appname=", ""))

        elif arg.startswith("--with-libprefix="):
            libPrefix = arg.replace("--with-libprefix=", "")

        elif mode == MODE_CONTROL and arg.startswith("osc."):
            oscAddr = arg

    # -------------------------------------------------------------
    # App initialization

    appName = "Carla2-Rack"     if mode == MODE_RACK     else \
              "Carla2-Patchbay" if mode == MODE_PATCHBAY else \
              "Carla2-Control"  if mode == MODE_CONTROL  else "Carla2"
    app = CarlaApplication(appName, libPrefix)

    # -------------------------------------------------------------
    # Set-up custom signal handling

    setUpSignals()

    # -------------------------------------------------------------
    # Init host backend

    if mode != MODE_CONTROL:
        host = initHost(initName, libPrefix, False, False, True)
        if mode == MODE_RACK:
            host.processMode       = ENGINE_PROCESS_MODE_CONTINUOUS_RACK
            host.processModeForced = True
        if mode == MODE_PATCHBAY:
            host.processMode       = ENGINE_PROCESS_MODE_PATCHBAY
            host.processModeForced = True
    else:
        host = initHost(initName, libPrefix, True, False, True, CarlaHostOSC)
    loadHostSettings(host)

    # -------------------------------------------------------------
    # Create GUI

    if mode != MODE_CONTROL:
        withCanvas = True
        if mode == MODE_RACK:
           withCanvas = not (LADISH_APP_NAME or NSM_URL) 
        gui = HostWindow(host, withCanvas)
    else:
        gui = HostWindowOSC(host, oscAddr)

    # -------------------------------------------------------------
    # Load project file if set

    if mode != MODE_CONTROL:
        args = app.arguments()

        if len(args) > 1:
            arg = args[-1]

            if arg.startswith("--with-appname=") or arg.startswith("--with-libprefix="):
                pass
            elif arg == "--gdb":
                pass
            elif os.path.exists(arg):
                gui.loadProjectLater(arg)

    # -------------------------------------------------------------
    # Show GUI

    gui.show()

    # -------------------------------------------------------------
    # App-Loop

    app.exit_exec()
