#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.deps
include $(CWD)/Makefile.mk

# --------------------------------------------------------------

ifeq ($(WIN64),true)
winv=64
else
winv=32
endif

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += -I../includes/vst
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
NATIVE_BUILD_FLAGS += $(FLUIDSYNTH_FLAGS)
NATIVE_LINK_FLAGS  += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_BUILD_FLAGS += $(LINUXSAMPLER_FLAGS)
NATIVE_LINK_FLAGS  += $(LINUXSAMPLER_LIBS)
endif

# --------------------------------------------------------------

LIBS          = ../modules/lilv.a
LIBS_posix32  = ../modules/lilv.posix32.a
LIBS_posix64  = ../modules/lilv.posix64.a
LIBS_win32    = ../modules/lilv.win32.a
LIBS_win64    = ../modules/lilv.win64.a
LINK_FLAGS   += $(LILV_LIBS)

LIBS         += ../modules/juce_core.a
LIBS_posix32 += ../modules/juce_core.posix32.a
LIBS_posix64 += ../modules/juce_core.posix64.a
LIBS_win32   += ../modules/juce_core.win32.a
LIBS_win64   += ../modules/juce_core.win64.a
LINK_FLAGS   += $(JUCE_CORE_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LIBS         += ../modules/juce_audio_basics.a
LIBS_posix32 += ../modules/juce_audio_basics.posix32.a
LIBS_posix64 += ../modules/juce_audio_basics.posix64.a
LIBS_win32   += ../modules/juce_audio_basics.win32.a
LIBS_win64   += ../modules/juce_audio_basics.win64.a
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
ifeq ($(MACOS),true)
LIBS         += ../modules/juce_gui_extra.a
LIBS_posix32 += ../modules/juce_gui_extra.posix32.a
LIBS_posix64 += ../modules/juce_gui_extra.posix64.a
endif
LINK_FLAGS   += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS   += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS   += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS   += $(JUCE_EVENTS_LIBS)
LINK_FLAGS   += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS   += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(MACOS),true)
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

ifeq ($(UNIX),true)
LINK_FLAGS += -ldl
endif

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

carla-discovery.cpp.o: carla-discovery.cpp $(DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

carla-discovery.cpp.posix32.o: carla-discovery.cpp $(DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

carla-discovery.cpp.posix64.o: carla-discovery.cpp $(DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

carla-discovery.cpp.win32.o: carla-discovery.cpp $(DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

carla-discovery.cpp.win64.o: carla-discovery.cpp $(DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

../../bin/carla-discovery-native: carla-discovery.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@

../../bin/carla-discovery-native.exe: carla-discovery.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@ && \
	cd ../../bin/ && ln -s carla-discovery-native.exe carla-discovery-win$(winv).exe

../../bin/carla-discovery-posix32: carla-discovery.cpp.posix32.o $(LIBS_posix32)
	$(CXX) $< $(LIBS_START) $(LIBS_posix32) $(LIBS_END) $(32BIT_FLAGS) $(LINK_FLAGS) -o $@

../../bin/carla-discovery-posix64: carla-discovery.cpp.posix64.o $(DEPS) $(LIBS_posix64)
	$(CXX) $< $(LIBS_START) $(LIBS_posix64) $(LIBS_END) $(64BIT_FLAGS) $(LINK_FLAGS) -o $@

../../bin/carla-discovery-win32.exe: carla-discovery.cpp.win32.o $(DEPS) $(LIBS_win32)
	$(CXX) $< $(LIBS_START) $(LIBS_win32) $(LIBS_END) $(32BIT_FLAGS) $(LINK_FLAGS) -o $@

../../bin/carla-discovery-win64.exe: carla-discovery.cpp.win64.o $(DEPS) $(LIBS_win64)
	$(CXX) $< $(LIBS_START) $(LIBS_win64) $(LIBS_END) $(64BIT_FLAGS) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../../bin/carla-discovery-*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
