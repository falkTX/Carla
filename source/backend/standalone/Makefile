#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo)

LINK_FLAGS += $(JACKBRIDGE_LIBS)
# LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
# LINK_FLAGS += $(JUCE_CORE_LIBS)
# LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
# LINK_FLAGS += $(JUCE_EVENTS_LIBS)
# LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
# LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
LINK_FLAGS += $(RTAUDIO_LIBS)
LINK_FLAGS += $(RTMIDI_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)

ifeq ($(HAVE_OPENGL),true)
LINK_FLAGS += $(DGL_LIBS)
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LINK_FLAGS += $(LILV_LIBS)
endif

# --------------------------------------------------------------
# Plugin

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
ifeq ($(WIN32),true)
LINK_FLAGS += lrpcrt4
endif
endif

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

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a

LIBS += ../../modules/carla_native.a
LIBS += ../../modules/jackbridge.a
# LIBS += ../../modules/juce_audio_basics.a
# LIBS += ../../modules/juce_core.a
# LIBS += ../../modules/juce_data_structures.a
# LIBS += ../../modules/juce_events.a
# LIBS += ../../modules/juce_graphics.a
# LIBS += ../../modules/juce_gui_basics.a
LIBS += ../../modules/rtaudio.a
LIBS += ../../modules/rtmidi.a
LIBS += ../../modules/rtmempool.a

ifeq ($(HAVE_OPENGL),true)
LIBS += ../../modules/dgl.a
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../../modules/lilv.a
endif

# --------------------------------------------------------------

OBJS  = \
     CarlaStandalone.cpp.o

HEADERS = \
	../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp ../CarlaHost.hpp ../../modules/CarlaNative.h

ifeq ($(WIN32),true)
TARGET = ../libcarla_standalone2.dll
else
ifeq ($(MACOS),true)
TARGET = ../libcarla_standalone2.dylib
else
TARGET = ../libcarla_standalone2.so
endif
endif

# --------------------------------------------------------------

all: $(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../libcarla_standalone2.dll: $(OBJS) $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -shared -Wl,--output-def,$@.def -o $@

../libcarla_standalone2.dylib: $(OBJS) $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -dynamiclib -o $@

../libcarla_standalone2.so: $(OBJS) $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -shared -o $@

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

../libcarla_%.a: .FORCE
	$(MAKE) -C ../$*

../../modules/%.a: .FORCE
	$(MAKE) -C ../../modules $*

# --------------------------------------------------------------
