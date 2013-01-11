#!/usr/bin/make -f
# Makefile for carla libs #
# ----------------------------------------- #
# Created by falkTX
#

all:

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

lilv_posix32.a:
	$(MAKE) -C lilv posix32

lilv_posix64.a:
	$(MAKE) -C lilv posix64

lilv_win32.a:
	$(MAKE) -C lilv win32

lilv_win64.a:
	$(MAKE) -C lilv win64

# --------------------------------------------------------------

rtmempool.a:
	$(MAKE) -C rtmempool

rtmempool_posix32.a:
	$(MAKE) -C rtmempool posix32

rtmempool_posix64.a:
	$(MAKE) -C rtmempool posix64

rtmempool_win32.a:
	$(MAKE) -C rtmempool win32

rtmempool_win64.a:
	$(MAKE) -C rtmempool win64

# --------------------------------------------------------------

clean:
	rm -f *.a *.def *.dll *.so
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtmempool
