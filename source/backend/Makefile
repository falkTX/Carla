#!/usr/bin/make -f
# Makefile for carla backend and standalone #
# -------------------------------------------------------------- #
# Created by falkTX
#

all: libcarla_standalone.so

# --------------------------------------------------------------

libcarla_engine.%:
	$(MAKE) -C engine ../$@

libcarla_native.%:
	$(MAKE) -C native ../$@

libcarla_plugin.%:
	$(MAKE) -C plugin ../$@

libcarla_standalone.%:
	$(MAKE) -C standalone ../$@

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C engine
	$(MAKE) clean -C native
	$(MAKE) clean -C plugin
	$(MAKE) clean -C standalone
