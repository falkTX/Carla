#!/usr/bin/make -f
# Makefile for carla plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
LINK_FLAGS += $(LIBLO_LIBS)
LINK_FLAGS += $(QTCORE_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)

# --------------------------------------------------------------
# Engine

LINK_FLAGS += $(JACKBRIDGE_LIBS)

# --------------------------------------------------------------
# Plugin

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LINK_FLAGS += $(LILV_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(LINUXSAMPLER_LIBS)
endif

# --------------------------------------------------------------
# Coomon

ifeq ($(HAVE_DGL),true)
LINK_FLAGS += $(DGL_LIBS)
endif

ifeq ($(HAVE_JUCE_UI),true)
LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif

# --------------------------------------------------------------
# Native

LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)

# --------------------------------------------------------------
# Libs

LIBS += ../backend/carla_engine_plugin.a
LIBS += ../backend/carla_plugin.a

LIBS += ../modules/native-plugins.a
LIBS += ../modules/jackbridge.a
LIBS += ../modules/juce_audio_basics.a
LIBS += ../modules/juce_audio_processors.a
LIBS += ../modules/juce_core.a
LIBS += ../modules/juce_events.a
LIBS += ../modules/rtmempool.a

ifeq ($(HAVE_AF_DEPS),true)
LIBS += ../modules/audio_decoder.a
endif

ifeq ($(HAVE_DGL),true)
LIBS += ../modules/dgl.a
endif

ifeq ($(HAVE_JUCE_UI),true)
LIBS += ../modules/juce_data_structures.a
LIBS += ../modules/juce_graphics.a
LIBS += ../modules/juce_gui_basics.a
LIBS += ../modules/juce_gui_extra.a
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../modules/lilv.a
endif

# --------------------------------------------------------------

TARGETS = \
	../../bin/carla-native.lv2/carla-native$(LIB_EXT) \
	../../bin/carla-native-lv2-export$(APP_EXT)

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

../../bin/carla-native.lv2/carla-native$(LIB_EXT): carla-native-lv2.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/carla-native-lv2-export$(APP_EXT): carla-native-lv2-export.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------
