#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# --------------------------------------------------------------------------------------------------------

from carla_backend import *
from signal import signal, SIGINT, SIGTERM
from time import sleep

# --------------------------------------------------------------------------------------------------------

global term
term = False
def signalHandler(sig, frame):
    if sig in (SIGINT, SIGTERM):
        global term
        term = True

# --------------------------------------------------------------------------------------------------------

host = CarlaHostDLL("/home/falktx/FOSS/GIT-mine/falkTX/Carla/bin/libcarla_standalone2.so")

if not host.engine_init("JACK", "Carla-uhe-test"):
    print("Engine failed to initialize, possible reasons:\n%s" % host.get_last_error())
    sys.exit(1)

if not host.add_plugin(BINARY_NATIVE, PLUGIN_VST2, "/home/falktx/.vst/u-he/ACE.64.so", "", "", 0, None, 0):
    print("Failed to load plugin, possible reasons:\n%s" % host.get_last_error())
    host.engine_close()
    sys.exit(1)

signal(SIGINT,  signalHandler)
signal(SIGTERM, signalHandler)

while host.is_engine_running() and not term:
    host.engine_idle()
    sleep(0.5)

host.engine_close()

# --------------------------------------------------------------------------------------------------------
