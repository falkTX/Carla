#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------
# Base

LINK_FLAGS  += $(LIBLO_LIBS)
LINK_FLAGS  += $(QTCORE_LIBS)
LINK_FLAGS  += $(QTXML_LIBS)

# ----------------------------------------------------------------------------------------------------------------------------
# Common

STATIC_LIBS  = ../carla_engine.a
STATIC_LIBS += ../carla_plugin.a
STATIC_LIBS += ../../modules/jackbridge.a
STATIC_LIBS += ../../modules/rtmempool.a

LINK_FLAGS  += $(JACKBRIDGE_LIBS)
LINK_FLAGS  += $(RTMEMPOOL_LIBS)

ifeq ($(HAVE_DGL),true)
STATIC_LIBS += ../../modules/dgl.a
LINK_FLAGS  += $(DGL_LIBS)
endif

ifeq ($(HAVE_JUCE),true)
STATIC_LIBS += ../../modules/juce_audio_basics.a
STATIC_LIBS += ../../modules/juce_audio_devices.a
STATIC_LIBS += ../../modules/juce_audio_processors.a
STATIC_LIBS += ../../modules/juce_core.a
STATIC_LIBS += ../../modules/juce_data_structures.a
STATIC_LIBS += ../../modules/juce_events.a
STATIC_LIBS += ../../modules/juce_graphics.a
STATIC_LIBS += ../../modules/juce_gui_basics.a
STATIC_LIBS += ../../modules/juce_gui_extra.a
LINK_FLAGS  += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS  += $(JUCE_AUDIO_DEVICES_LIBS)
LINK_FLAGS  += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS  += $(JUCE_CORE_LIBS)
LINK_FLAGS  += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS  += $(JUCE_EVENTS_LIBS)
LINK_FLAGS  += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS  += $(JUCE_GUI_BASICS_LIBS)
LINK_FLAGS  += $(JUCE_GUI_EXTRA_LIBS)
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Engine

STATIC_LIBS += ../../modules/rtaudio.a
STATIC_LIBS += ../../modules/rtmidi.a

LINK_FLAGS  += $(RTAUDIO_LIBS)
LINK_FLAGS  += $(RTMIDI_LIBS)

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
STATIC_LIBS += ../../modules/lilv.a
LINK_FLAGS  += $(LILV_LIBS)
endif

ifeq ($(HAVE_CSOUND),true)
LINK_FLAGS += $(CSOUND_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(LINUXSAMPLER_LIBS)
endif

# ifeq ($(HAVE_WAYLAND),true)
# LINK_FLAGS += $(WAYLAND_LIBS)
# endif

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Native

STATIC_LIBS += ../../modules/native-plugins.a

ifeq ($(HAVE_AF_DEPS),true)
STATIC_LIBS += ../../modules/audio_decoder.a
endif

LINK_FLAGS  += $(NATIVE_PLUGINS_LIBS)

# ----------------------------------------------------------------------------------------------------------------------------

BIN_PATH = ../../../bin
TARGET   = $(BIN_PATH)/libcarla_standalone2.$(LIB_EXT)

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

clean:
	$(RM) *.o $(TARGET)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(TARGET): CarlaStandalone.cpp.o $(STATIC_LIBS)
	$(CXX) $< $(LIBS_START) $(STATIC_LIBS) $(LIBS_END) $(LINK_FLAGS) $(EXTRA_LIBS) $(SHARED) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

CarlaStandalone.cpp.o: CarlaStandalone.cpp $(CARLA_STANDALONE_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

../carla_%.a:
	$(MAKE) -C ../$*

../../modules/%.a:
	$(MAKE) -C ../../modules $*

# ----------------------------------------------------------------------------------------------------------------------------
