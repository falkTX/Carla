#!/usr/bin/make -f
# Makefile for carla plugin exports #
# --------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

BUILD_CXX_FLAGS += -I../includes -I../utils -isystem ../modules

# --------------------------------------------------------------

ifeq ($(HAVE_OPENGL),true)
BUILD_CXX_FLAGS += -DWANT_OPENGL
endif

ifeq ($(HAVE_AF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_AUDIOFILE
endif

ifeq ($(HAVE_MF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_MIDIFILE
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
endif

# --------------------------------------------------------------
# Common

# LINK_FLAGS += $(shell pkg-config --libs liblo)

# --------------------------------------------------------------
# Plugin

# ifeq ($(HAVE_FLUIDSYNTH),true)
# LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
# endif
#
# ifeq ($(HAVE_LINUXSAMPLER),true)
# LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
# endif

# --------------------------------------------------------------
# Native

ifeq ($(HAVE_AF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs sndfile)
ifeq ($(HAVE_FFMPEG),true)
LINK_FLAGS += $(shell pkg-config --libs libavcodec libavformat libavutil)
endif
endif

ifeq ($(HAVE_MF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs smf)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs ntk_images ntk)
endif
endif

# --------------------------------------------------------------
# Libs

LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)
# LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
# LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
# LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)

ifeq ($(HAVE_OPENGL),true)
LINK_FLAGS += $(DGL_LIBS)
endif

# ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# LINK_FLAGS += $(LILV_LIBS)
# endif

# --------------------------------------------------------------

LIBS  = ../modules/carla_native.a
LIBS += ../modules/juce_audio_basics.a
LIBS += ../modules/juce_core.a
# LIBS += ../modules/juce_data_structures.a
LIBS += ../modules/juce_events.a
# LIBS += ../modules/juce_graphics.a
# LIBS += ../modules/juce_gui_basics.a
LIBS += ../modules/rtmempool.a

ifeq ($(HAVE_OPENGL),true)
LIBS += ../modules/dgl.a
endif

# ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# LIBS += ../modules/lilv.a
# endif

# --------------------------------------------------------------

ifeq ($(WIN32),true)
TARGETS = carla-native-lv2-export.exe carla-native.lv2/carla-native.dll
else
ifeq ($(MACOS),true)
TARGETS = carla-native-lv2-export carla-native.lv2/carla-native.dylib
else
TARGETS = carla-native-lv2-export carla-native.lv2/carla-native.so
endif
endif

# --------------------------------------------------------------

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o
	$(RM) carla-native.lv2/*.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

carla-native-base.cpp.o: carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-lv2.cpp.o: carla-native-lv2.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-lv2-export.cpp.o: carla-native-lv2-export.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

carla-native-lv2-export: carla-native-lv2-export.cpp.o $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
	./carla-native-lv2-export

carla-native-lv2-export.exe: carla-native-lv2-export.cpp.o $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
	./carla-native-lv2-export.exe

# --------------------------------------------------------------

carla-native.lv2/carla-native.dll: carla-native-lv2.cpp.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

carla-native.lv2/carla-native.dylib: carla-native-lv2.cpp.o $(LIBS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

carla-native.lv2/carla-native.so: carla-native-lv2.cpp.o $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../modules/%.a: .FORCE
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------
