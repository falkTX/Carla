#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules -Wno-multichar

BUILD_CXX_FLAGS += $(QTCORE_FLAGS)
LINK_FLAGS      += $(QTCORE_LIBS)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2
# -DWANT_VST
# ifeq ($(CARLA_VESTIGE_HEADER),true)
# BUILD_CXX_FLAGS += -DVESTIGE_HEADER
# endif
endif

# --------------------------------------------------------------

# ifeq ($(HAVE_CSOUND),true)
# NATIVE_FLAGS += $(shell pkg-config --cflags --libs sndfile) -lcsound64 -DWANT_CSOUND
# endif

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_FLAGS += $(FLUIDSYNTH_FLAGS) $(FLUIDSYNTH_LIBS) -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_FLAGS += $(LINUXSAMPLER_FLAGS) $(LINUXSAMPLER_LIBS) -DWANT_LINUXSAMPLER
endif

# --------------------------------------------------------------

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS         += ../modules/lilv.a
LIBS_posix32 += ../modules/lilv.posix32.a
LIBS_posix64 += ../modules/lilv.posix64.a
LIBS_win32   += ../modules/lilv.win32.a
LIBS_win64   += ../modules/lilv.win64.a
LINK_FLAGS   += $(LILV_LIBS)
endif

ifeq ($(HAVE_JUCE),true)
LIBS         += ../modules/juce_audio_basics.a
LIBS_posix32 += ../modules/juce_audio_basics.posix32.a
LIBS_posix64 += ../modules/juce_audio_basics.posix64.a
LIBS_win32   += ../modules/juce_audio_basics.win32.a
LIBS_win64   += ../modules/juce_audio_basics.win64.a
LIBS         += ../modules/juce_core.a
LIBS_posix32 += ../modules/juce_core.posix32.a
LIBS_posix64 += ../modules/juce_core.posix64.a
LIBS_win32   += ../modules/juce_core.win32.a
LIBS_win64   += ../modules/juce_core.win64.a
LINK_FLAGS   += $(JUCE_CORE_LIBS) $(JUCE_AUDIO_BASICS_LIBS)
endif

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS) -ldl

WIN_BUILD_FLAGS   = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS   = $(32BIT_FLAGS)
WIN_64BIT_FLAGS   = $(64BIT_FLAGS)
WIN_LINK_FLAGS    = $(LINK_FLAGS)

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

../modules/lilv.%.a: .FORCE
	$(MAKE) -C ../modules lilv_$*

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------
