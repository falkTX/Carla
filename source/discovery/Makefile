#!/usr/bin/make -f
# Makefile for carla-discovery #
# ----------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_FLAGS    += $(shell pkg-config --cflags --libs fluidsynth) -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_FLAGS    += $(shell pkg-config --cflags --libs linuxsampler) -DWANT_LINUXSAMPLER
endif

# --------------------------------------------------------------

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS) -ldl
#-lpthread

WIN_BUILD_FLAGS   = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS   = $(32BIT_FLAGS)
WIN_64BIT_FLAGS   = $(64BIT_FLAGS)
WIN_LINK_FLAGS    = $(LINK_FLAGS) -static -mwindows -lole32 -luuid -lws2_32

# --------------------------------------------------------------

all: carla-discovery-native

posix32: carla-discovery-posix32
posix64: carla-discovery-posix64
win32:   carla-discovery-win32.exe
win64:   carla-discovery-win64.exe

# --------------------------------------------------------------

OBJS = carla-discovery.cpp

carla-discovery-native: $(OBJS) ../libs/lilv.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_LINK_FLAGS) $(NATIVE_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-posix32: $(OBJS) ../libs/lilv_posix32.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-posix64: $(OBJS) ../libs/lilv_posix64.a
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-win32.exe: $(OBJS) ../libs/lilv_win32.a
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ && $(STRIP) $@

carla-discovery-win64.exe: $(OBJS) ../libs/lilv_win64.a
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

../libs/lilv.a:
	$(MAKE) -C ../libs/lilv

../libs/lilv_%.a:
	$(MAKE) -C ../libs/lilv $*

# --------------------------------------------------------------

clean:
	rm -f carla-discovery-*

debug:
	$(MAKE) DEBUG=true
