#!/usr/bin/make -f
# Makefile for carla-standalone #
# --------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------
# Shared

LINK_FLAGS += $(shell pkg-config --libs liblo QtGui)

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

ifeq ($(HAVE_SUIL),true)
LINK_FLAGS += $(shell pkg-config --libs suil-0)
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------
# Native

LINK_FLAGS += -lGL

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs fftw3 mxml) -lpthread
endif

# --------------------------------------------------------------

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a
LIBS += ../libcarla_native.a
LIBS += ../../libs/rtmempool.a

OBJS  = \
     carla_standalone.cpp.o

SHARED = ../libcarla_standalone.so
STATIC = ../libcarla_standalone.a

# --------------------------------------------------------------

all: $(SHARED)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(SHARED): $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

$(STATIC): $(OBJS) $(LIBS)
	$(AR) rs $@ $^

# --------------------------------------------------------------

../libcarla_%.a:
	$(MAKE) -C ../$* $@

../../libs/%.a:
	$(MAKE) -C ../../libs/ $*.a
