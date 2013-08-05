#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../modules -I../utils

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

LIBS         = ../modules/juce_core.a
LIBS_posix32 = ../modules/juce_core.posix32.a
LIBS_posix64 = ../modules/juce_core.posix64.a
LIBS_win32   = ../modules/juce_core.win32.a
LIBS_win64   = ../modules/juce_core.win64.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS         += ../modules/lilv.a
LIBS_posix32 += ../modules/lilv.posix32.a
LIBS_posix64 += ../modules/lilv.posix64.a
LIBS_win32   += ../modules/lilv.win32.a
LIBS_win64   += ../modules/lilv.win64.a
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
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(NATIVE_FLAGS) $(POSIX_LINK_FLAGS) -o $@

carla-discovery-native.exe: $(OBJS) $(LIBS)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(NATIVE_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-discovery-posix32: $(OBJS) $(LIBS_posix32)
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

carla-discovery-posix64: $(OBJS) $(LIBS_posix64)
	$(CXX) $^ $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

carla-discovery-win32.exe: $(OBJS) $(LIBS_win32)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-discovery-win64.exe: $(OBJS) $(LIBS_win64)
	$(CXX) $^ $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	rm -f carla-discovery-*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*

../modules/juce_core.%.a: .FORCE
	$(MAKE) -C ../modules juce_core_$*

../modules/lilv.%.a: .FORCE
	$(MAKE) -C ../modules lilv_$*
