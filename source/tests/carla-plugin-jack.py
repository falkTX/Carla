#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# --------------------------------------------------------------------------------------------------------

from carla_backend import *
from signal import signal, SIGINT, SIGTERM
from time import sleep
from sys import exit

# --------------------------------------------------------------------------------------------------------

class CarlaObject(object):
    __slots__ = [
        'term'
    ]

gCarla = CarlaObject()
gCarla.term = False

def signalHandler(sig, frame):
    if sig in (SIGINT, SIGTERM):
        gCarla.term = True

# --------------------------------------------------------------------------------------------------------

binaryDir = "/home/falktx/Personal/FOSS/GIT/falkTX/Carla/bin"
host = CarlaHostDLL("/home/falktx/FOSS/GIT-mine/falkTX/Carla/bin/libcarla_standalone2.so", True)
host.set_engine_option(ENGINE_OPTION_PATH_BINARIES, 0, binaryDir)

if not host.engine_init("JACK", "Carla-Plugin-JACK"):
    print("Engine failed to initialize, possible reasons:\n%s" % host.get_last_error())
    exit(1)

if not host.add_plugin(BINARY_NATIVE, PLUGIN_JACK, "/usr/bin/lmms", "", "", 0, None, 0):
    print("Failed to load plugin, possible reasons:\n%s" % host.get_last_error())
    host.engine_close()
    exit(1)

signal(SIGINT,  signalHandler)
signal(SIGTERM, signalHandler)

while host.is_engine_running() and not gCarla.term:
    host.engine_idle()
    sleep(0.5)

if not gCarla.term:
    print("Engine closed abruptely")

if not host.engine_close():
    print("Engine failed to close, possible reasons:\n%s" % host.get_last_error())
    exit(1)

# --------------------------------------------------------------------------------------------------------
