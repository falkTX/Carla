#!/usr/bin/make -f
# Makefile for carla libs #
# ----------------------- #
# Created by falkTX
#

all:

# --------------------------------------------------------------

dgl.a:
	$(MAKE) -C distrho/dgl

dgl.%.a:
	$(MAKE) -C distrho/dgl $*

# --------------------------------------------------------------

lilv.a:
	$(MAKE) -C lilv

lilv.%.a:
	$(MAKE) -C lilv $*

# --------------------------------------------------------------

rtmempool.a:
	$(MAKE) -C rtmempool

rtmempool.%.a:
	$(MAKE) -C rtmempool $*

# --------------------------------------------------------------

widgets.a:
	$(MAKE) -C ../widgets

# --------------------------------------------------------------

jackbridge-win32.dll:
	$(MAKE) -C jackbridge win32

jackbridge-win64.dll:
	$(MAKE) -C jackbridge win64

jackbridge-win32.dll.so:
	$(MAKE) -C jackbridge wine32

jackbridge-win64.dll.so:
	$(MAKE) -C jackbridge wine64

# --------------------------------------------------------------

clean:
	rm -f *~ *.a *.def *.dll *.dylib *.so
	$(MAKE) clean -C distrho/dgl
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C ../widgets
