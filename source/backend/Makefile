#!/usr/bin/make -f
# Makefile for carla backend #
# ----------------------------------------- #
# Created by falkTX
#

all:

# --------------------------------------------------------------

# carla_backend.so: carla_engine.so carla_native.so carla_plugin.so carla_standalone.so
# 	$(CXX) $^

carla_engine.so:
	$(MAKE) -C engine

carla_native.so:
	$(MAKE) -C native

carla_plugin.so:
	$(MAKE) -C plugin

carla_standalone.so:
	$(MAKE) -C standalone

# --------------------------------------------------------------

clean:
	rm -f *.so
	$(MAKE) clean -C engine
	$(MAKE) clean -C native
	$(MAKE) clean -C plugin
	$(MAKE) clean -C standalone
