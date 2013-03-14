#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs gl liblo QtCore QtXml)
LINK_FLAGS += -lX11

# --------------------------------------------------------------
# Engine

ifeq ($(HAVE_ALSA),true)
LINK_FLAGS += $(shell pkg-config --libs alsa)
endif
ifeq ($(HAVE_JACK),true)
LINK_FLAGS += $(shell pkg-config --libs jack)
endif
ifeq ($(HAVE_PULSEAUDIO),true)
LINK_FLAGS += $(shell pkg-config --libs libpulse-simple)
endif

# --------------------------------------------------------------
# Plugin

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------
# Native

LINK_FLAGS += $(shell pkg-config --libs QtGui)

ifeq ($(HAVE_AF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs libavcodec libavformat sndfile)
endif
ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs fftw3 mxml zlib)
endif

# --------------------------------------------------------------

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a
LIBS += ../libcarla_native.a
LIBS += ../../libs/dgl.a
LIBS += ../../libs/lilv.a
LIBS += ../../libs/rtmempool.a
LIBS += ../../libs/widgets.a

OBJS  = \
     CarlaStandalone.cpp.o

SHARED = ../libcarla_standalone.so
STATIC = ../libcarla_standalone.a

# --------------------------------------------------------------

all: $(SHARED)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../CarlaEngine.hpp ../CarlaPlugin.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(SHARED): $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@ && $(STRIP) $@

$(STATIC): $(OBJS) $(LIBS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

../libcarla_%.a:
	$(MAKE) -C ../$* $@

../../libs/%.a:
	$(MAKE) -C ../../libs/ $*.a
