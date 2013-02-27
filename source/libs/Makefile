#!/usr/bin/make -f
# Makefile for carla libs #
# ----------------------- #
# Created by falkTX
#

all:

# --------------------------------------------------------------

dgl.a:
	$(MAKE) -C distrho/dgl

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

lilv.a:
	$(MAKE) -C lilv

lilv_%.a:
	$(MAKE) -C lilv $*

# --------------------------------------------------------------

rtmempool.a:
	$(MAKE) -C rtmempool

rtmempool_%.a:
	$(MAKE) -C rtmempool $*

# --------------------------------------------------------------

clean:
	rm -f *.a *.def *.dll *.so
	$(MAKE) clean -C distrho/dgl
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtmempool
