#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core)
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_FLAGS += $(shell pkg-config --cflags --libs fluidsynth) -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_FLAGS += $(shell pkg-config --cflags --libs linuxsampler) -DWANT_LINUXSAMPLER
endif

# --------------------------------------------------------------

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS) -ldl

ifeq ($(MACOS),true)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS)
POSIX_64BIT_FLAGS = $(64BIT_FLAGS)
endif

WIN_BUILD_FLAGS   = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS   = $(32BIT_FLAGS)
WIN_64BIT_FLAGS   = $(64BIT_FLAGS)
WIN_LINK_FLAGS    = $(LINK_FLAGS) $(EXTRA_LIBS) -lole32 -luuid -lws2_32

ifeq ($(HAVE_FLUIDSYNTH),true)
WIN_LINK_FLAGS   += -ldsound -lwinmm
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
WIN_LINK_FLAGS   += -lrpcrt4
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS         = ../libs/lilv.a
LIBS_posix32 = ../libs/lilv.posix32.a
LIBS_posix64 = ../libs/lilv.posix64.a
LIBS_win32   = ../libs/lilv.win32.a
LIBS_win64   = ../libs/lilv.win64.a
endif

OBJS = carla-discovery.cpp

# --------------------------------------------------------------

ifeq ($(WIN32),true)
all: carla-discovery-native.exe
else
all: carla-discovery-native
endif

posix32: carla-discovery-posix32
posix64: carla-discovery-posix64
win32:   carla-discovery-win32.exe
win64:   carla-discovery-win64.exe

# --------------------------------------------------------------

carla-discovery-native: $(OBJS) $(LIBS)
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(NATIVE_FLAGS) $(POSIX_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

carla-discovery-native.exe: $(OBJS) $(LIBS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(NATIVE_FLAGS) $(WIN_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

carla-discovery-posix32: $(OBJS) $(LIBS_posix32)
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

carla-discovery-posix64: $(OBJS) $(LIBS_posix64)
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

carla-discovery-win32.exe: $(OBJS) $(LIBS_win32)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

carla-discovery-win64.exe: $(OBJS) $(LIBS_win64)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@ $(CMD_STRIP) $@

# --------------------------------------------------------------

clean:
	rm -f carla-discovery-*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../libs/lilv.a: .FORCE
	$(MAKE) -C ../libs lilv

../libs/lilv.%.a: .FORCE
	$(MAKE) -C ../libs lilv_$*
