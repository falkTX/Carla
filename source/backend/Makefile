#!/usr/bin/make -f
# Makefile for carla backend and standalone #
# -------------------------------------------------------------- #
# Created by falkTX
#

all: carla_standalone

# --------------------------------------------------------------

carla_standalone:
	$(MAKE) -C standalone

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C engine
	$(MAKE) clean -C native
	$(MAKE) clean -C plugin
	$(MAKE) clean -C standalone
