#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += -I../includes/ladspa -I../includes/vst -I../includes/vst3
#-Wno-multichar

BUILD_CXX_FLAGS += $(QTCORE_FLAGS)
LINK_FLAGS      += $(QTCORE_LIBS)

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
endif

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -DWANT_AU
endif
ifeq ($(MACOS_OR_WIN32),true)
BUILD_CXX_FLAGS += -DWANT_VST3
endif
ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif
endif

# --------------------------------------------------------------

ifeq ($(HAVE_CSOUND),true)
NATIVE_FLAGS += -DWANT_CSOUND $(CSOUND_FLAGS) $(CSOUND_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_FLAGS += -DWANT_FLUIDSYNTH $(FLUIDSYNTH_FLAGS) $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_FLAGS += -DWANT_LINUXSAMPLER $(LINUXSAMPLER_FLAGS) $(LINUXSAMPLER_LIBS)
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

ifeq ($(MACOS),true)
LIBS         += ../modules/juce_gui_extra.a
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif

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

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
POSIX_LINK_FLAGS  = $(LINK_FLAGS)

ifneq ($(HAIKU),true)
POSIX_LINK_FLAGS += -ldl
endif

WIN_BUILD_FLAGS   = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS   = $(32BIT_FLAGS)
WIN_64BIT_FLAGS   = $(64BIT_FLAGS)
WIN_LINK_FLAGS    = $(LINK_FLAGS) $(EXTRA_LIBS)

# --------------------------------------------------------------

DEPS  = ../includes/CarlaDefines.h
DEPS += ../includes/CarlaMIDI.h
DEPS += ../backend/CarlaBackend.h
DEPS += ../utils/CarlaString.hpp
DEPS += ../utils/CarlaUtils.hpp
DEPS += ../utils/CarlaBackendUtils.hpp
DEPS += ../utils/CarlaJuceUtils.hpp
DEPS += ../utils/CarlaLibUtils.hpp
DEPS += ../utils/CarlaMathUtils.hpp
DEPS += ../utils/CarlaLadspaUtils.hpp
DEPS += ../utils/CarlaDssiUtils.hpp
DEPS += ../utils/CarlaLv2Utils.hpp
DEPS += ../utils/CarlaVstUtils.hpp

OBJS = carla-discovery.cpp

# --------------------------------------------------------------

ifeq ($(WIN32),true)
all: ../../../bin/carla-discovery-native.exe
else
all: ../../../bin/carla-discovery-native
endif

posix32: carla-discovery-posix32
posix64: carla-discovery-posix64
win32:   carla-discovery-win32.exe
win64:   carla-discovery-win64.exe

# --------------------------------------------------------------

../../../bin/carla-discovery-native: $(OBJS) $(DEPS) $(LIBS)
ifeq ($(MACOS),true)
	$(CXX) $< $(LIBS) $(POSIX_BUILD_FLAGS) $(NATIVE_FLAGS) $(POSIX_LINK_FLAGS) -o $@
else
	$(CXX) $< -Wl,--start-group $(LIBS) -Wl,--end-group $(POSIX_BUILD_FLAGS) $(NATIVE_FLAGS) $(POSIX_LINK_FLAGS) -o $@
endif

carla-discovery-native.exe: $(OBJS) $(DEPS) $(LIBS)
	$(CXX) $< -Wl,--start-group $(LIBS) -Wl,--end-group $(WIN_BUILD_FLAGS) $(NATIVE_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-discovery-posix32: $(OBJS) $(DEPS) $(LIBS_posix32)
	$(CXX) $< -Wl,--start-group $(LIBS_posix32) -Wl,--end-group $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

carla-discovery-posix64: $(OBJS) $(DEPS) $(LIBS_posix64)
	$(CXX) $< -Wl,--start-group $(LIBS_posix64) -Wl,--end-group $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) $(POSIX_LINK_FLAGS) -o $@

carla-discovery-win32.exe: $(OBJS) $(DEPS) $(LIBS_win32)
	$(CXX) $< -Wl,--start-group $(LIBS_win32) -Wl,--end-group $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

carla-discovery-win64.exe: $(OBJS) $(DEPS) $(LIBS_win64)
	$(CXX) $< -Wl,--start-group $(LIBS_win64) -Wl,--end-group $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) $(WIN_LINK_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	rm -f carla-discovery-*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../modules/juce_audio_basics.%.a: .FORCE
	$(MAKE) -C ../modules juce_audio_basics_$*

../modules/juce_audio_processors.%.a: .FORCE
	$(MAKE) -C ../modules juce_audio_processors_$*

../modules/juce_core.%.a: .FORCE
	$(MAKE) -C ../modules juce_core_$*

../modules/juce_data_structures.%.a: .FORCE
	$(MAKE) -C ../modules juce_data_structures_$*

../modules/juce_events.%.a: .FORCE
	$(MAKE) -C ../modules juce_events_$*

../modules/juce_graphics.%.a: .FORCE
	$(MAKE) -C ../modules juce_graphics_$*

../modules/juce_gui_basics.%.a: .FORCE
	$(MAKE) -C ../modules juce_gui_basics_$*

../modules/lilv.%.a: .FORCE
	$(MAKE) -C ../modules lilv_$*

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------
