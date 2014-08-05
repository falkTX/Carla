#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.deps
include $(CWD)/Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += -I../includes/ladspa -I../includes/vst
ifeq ($(MACOS_OR_WIN32),true)
BUILD_CXX_FLAGS += -I../includes/vst3
endif
ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
endif

32BIT_FLAGS += -DBUILD_BRIDGE
64BIT_FLAGS += -DBUILD_BRIDGE

# --------------------------------------------------------------

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif

# --------------------------------------------------------------

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_FLAGS += $(FLUIDSYNTH_FLAGS) $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_FLAGS += $(LINUXSAMPLER_FLAGS) $(LINUXSAMPLER_LIBS)
endif

# --------------------------------------------------------------

LIBS          = ../modules/lilv.a
LIBS_posix32  = ../modules/lilv.posix32.a
LIBS_posix64  = ../modules/lilv.posix64.a
LIBS_win32    = ../modules/lilv.win32.a
LIBS_win64    = ../modules/lilv.win64.a
LINK_FLAGS    = $(LILV_LIBS)

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
LINK_FLAGS   += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS   += $(JUCE_CORE_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LIBS         += ../modules/juce_audio_processors.a
LIBS_posix32 += ../modules/juce_audio_processors.posix32.a
LIBS_posix64 += ../modules/juce_audio_processors.posix64.a
LIBS_win32   += ../modules/juce_audio_processors.win32.a
LIBS_win64   += ../modules/juce_audio_processors.win64.a
LIBS         += ../modules/juce_data_structures.a
LIBS_posix32 += ../modules/juce_data_structures.posix32.a
LIBS_posix64 += ../modules/juce_data_structures.posix64.a
LIBS_win32   += ../modules/juce_data_structures.win32.a
LIBS_win64   += ../modules/juce_data_structures.win64.a
LIBS         += ../modules/juce_events.a
LIBS_posix32 += ../modules/juce_events.posix32.a
LIBS_posix64 += ../modules/juce_events.posix64.a
LIBS_win32   += ../modules/juce_events.win32.a
LIBS_win64   += ../modules/juce_events.win64.a
LIBS         += ../modules/juce_graphics.a
LIBS_posix32 += ../modules/juce_graphics.posix32.a
LIBS_posix64 += ../modules/juce_graphics.posix64.a
LIBS_win32   += ../modules/juce_graphics.win32.a
LIBS_win64   += ../modules/juce_graphics.win64.a
LIBS         += ../modules/juce_gui_basics.a
LIBS_posix32 += ../modules/juce_gui_basics.posix32.a
LIBS_posix64 += ../modules/juce_gui_basics.posix64.a
LIBS_win32   += ../modules/juce_gui_basics.win32.a
LIBS_win64   += ../modules/juce_gui_basics.win64.a
LINK_FLAGS   += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS   += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS   += $(JUCE_EVENTS_LIBS)
LINK_FLAGS   += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS   += $(JUCE_GUI_BASICS_LIBS)
endif

ifeq ($(MACOS),true)
LIBS         += ../modules/juce_gui_extra.a
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS)

ifneq ($(HAIKU),true)
POSIX_LINK_FLAGS += -ldl
endif

WIN_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS = $(32BIT_FLAGS)
WIN_64BIT_FLAGS = $(64BIT_FLAGS)
WIN_LINK_FLAGS  = $(LINK_FLAGS) $(EXTRA_LIBS)

# --------------------------------------------------------------

DEPS  = $(CARLA_BACKEND_UTILS_HPP)
DEPS += $(CARLA_LIB_UTILS_HPP)
DEPS += $(CARLA_MATH_UTILS_HPP)
DEPS += $(CARLA_MIDI_H)
# juce_audio_processors.h
DEPS += $(CARLA_LADSPA_UTILS_HPP)
DEPS += $(CARLA_DSSI_UTILS_CPP)
DEPS += $(CARLA_LV2_UTILS_HPP)
DEPS += $(CARLA_VST_UTILS_HPP)

OBJS = carla-discovery.cpp

# --------------------------------------------------------------

all:     ../../bin/carla-discovery-native$(APP_EXT)
posix32: ../../bin/carla-discovery-posix32
posix64: ../../bin/carla-discovery-posix64
win32:   ../../bin/carla-discovery-win32.exe
win64:   ../../bin/carla-discovery-win64.exe

# --------------------------------------------------------------

../../bin/carla-discovery-native: $(OBJS) $(DEPS) $(LIBS)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS) $(LIBS_END) $(POSIX_BUILD_FLAGS) $(NATIVE_FLAGS) $(POSIX_LINK_FLAGS) -o $@

../../bin/carla-discovery-native.exe: $(OBJS) $(DEPS) $(LIBS)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS) $(LIBS_END) $(WIN_BUILD_FLAGS) $(NATIVE_FLAGS) $(WIN_LINK_FLAGS) -o $@

../../bin/carla-discovery-posix32: $(OBJS) $(DEPS) $(LIBS_posix32)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS_posix32) $(LIBS_END) $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

../../bin/carla-discovery-posix64: $(OBJS) $(DEPS) $(LIBS_posix64)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS_posix64) $(LIBS_END) $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

../../bin/carla-discovery-win32.exe: $(OBJS) $(DEPS) $(LIBS_win32)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS_win32) $(LIBS_END) $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

../../bin/carla-discovery-win64.exe: $(OBJS) $(DEPS) $(LIBS_win64)
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS_win64) $(LIBS_END) $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	$(RM) ../../bin/carla-discovery-*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
