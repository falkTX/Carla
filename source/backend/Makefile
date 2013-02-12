#!/usr/bin/make -f
# Makefile for carla backend and standalone #
# ----------------------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------

all:
	$(MAKE) -C engine
	$(MAKE) -C plugin
	$(MAKE) -C native
	$(MAKE) -C standalone

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin
	$(MAKE) clean -C native
	$(MAKE) clean -C standalone

# --------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true
