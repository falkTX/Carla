#!/usr/bin/make -f
# Makefile for carla libs #
# ----------------------- #
# Created by falkTX
#

all:

# --------------------------------------------------------------

dgl:
	$(MAKE) -C distrho/dgl

dgl_%:
	$(MAKE) -C distrho/dgl $*

# --------------------------------------------------------------

lilv:
	$(MAKE) -C lilv

lilv_%:
	$(MAKE) -C lilv $*

# --------------------------------------------------------------

rtmempool:
	$(MAKE) -C rtmempool

rtmempool_%:
	$(MAKE) -C rtmempool $*

# --------------------------------------------------------------

theme:
	$(MAKE) -C ../theme

theme_%:
	$(MAKE) -C ../theme $*

# --------------------------------------------------------------

widgets:
	$(MAKE) -C ../widgets

# --------------------------------------------------------------

jackbridge-win32:
	$(MAKE) -C jackbridge win32

jackbridge-win64:
	$(MAKE) -C jackbridge win64

jackbridge-wine32:
	$(MAKE) -C jackbridge wine32

jackbridge-wine64:
	$(MAKE) -C jackbridge wine64

# --------------------------------------------------------------

clean:
	rm -f *.a *.def *.dll *.dylib *.so
	$(MAKE) clean -C distrho/dgl
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C ../theme
	$(MAKE) clean -C ../widgets

# --------------------------------------------------------------

.PHONY: lilv rtmempool
