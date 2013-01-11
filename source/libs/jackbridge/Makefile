#!/usr/bin/make -f
# Makefile for jackbridge #
# ------------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

WINECC ?= winegcc

BUILD_C_FLAGS += -DJACKBRIDGE_EXPORT -I.
LINK_FLAGS    += -shared

WIN_BUILD_FLAGS  = $(BUILD_C_FLAGS) -DJACKBRIDGE_DUMMY -w
WIN_32BIT_FLAGS  = $(32BIT_FLAGS)
WIN_64BIT_FLAGS  = $(64BIT_FLAGS)
WIN_LINK_FLAGS   = $(LINK_FLAGS)

WINE_BUILD_FLAGS = $(BUILD_C_FLAGS) -fPIC
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine
WINE_LINK_FLAGS  = $(LINK_FLAGS) $(shell pkg-config --libs jack) -ldl

OBJS = jackbridge.c

# --------------------------------------------------------------

all:

win32:  ../jackbridge-win32.dll
win64:  ../jackbridge-win64.dll
wine32: ../jackbridge-win32.dll.so
wine64: ../jackbridge-win64.dll.so

# --------------------------------------------------------------

../jackbridge-win32.dll: $(OBJS)
	$(CC) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ && $(STRIP) $@

../jackbridge-win64.dll: $(OBJS)
	$(CC) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -Wl,--output-def,$@.def,--out-implib,$@.a -o $@ && $(STRIP) $@

../jackbridge-win32.dll.so: $(OBJS) ../jackbridge-win32.dll.def
	$(WINECC) $^ $(WINE_BUILD_FLAGS) $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) -mno-cygwin -o $@ && $(STRIP) $@

../jackbridge-win64.dll.so: $(OBJS) ../jackbridge-win64.dll.def
	$(WINECC) $^ $(WINE_BUILD_FLAGS) $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) -mno-cygwin -o $@ && $(STRIP) $@

# --------------------------------------------------------------
