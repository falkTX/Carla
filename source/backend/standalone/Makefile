#!/usr/bin/make -f
# Makefile for carla-standalone #
# --------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------
# Shared

LINK_FLAGS      += -shared
LINK_FLAGS      += $(shell pkg-config --libs liblo QtGui)

# --------------------------------------------------------------
# Engine

ifeq ($(HAVE_ALSA),true)
LINK_FLAGS      += $(shell pkg-config --libs alsa)
endif
ifeq ($(HAVE_JACK),true)
LINK_FLAGS      += $(shell pkg-config --libs jack)
endif
ifeq ($(HAVE_PULSEAUDIO),true)
LINK_FLAGS      += $(shell pkg-config --libs libpulse-simple)
endif

# --------------------------------------------------------------
# Native

LINK_FLAGS      +=  -lGL

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml) -lpthread
endif

# --------------------------------------------------------------
# Plugin


ifeq ($(HAVE_SUIL),true)
LINK_FLAGS      += $(shell pkg-config --libs suil-0)
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS      += $(shell pkg-config --libs fluidsynth)
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS      += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------


LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_native.a
LIBS += ../libcarla_plugin.a

OBJS  = \
     carla_standalone.cpp.o

STATIC = ../libcarla_standalone.a
TARGET = ../libcarla_standalone.so

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(STATIC) $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(STATIC): $(OBJS) $(LIBS)
	$(AR) rs $@ $^

$(TARGET): $(OBJS) $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@

test: $(OBJS) $(LIBS)
	$(CXX) $^ $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

../libcarla_engine.a:
	$(MAKE) -C ../engine $@

../libcarla_native.a:
	$(MAKE) -C ../native $@

../libcarla_plugin.a:
	$(MAKE) -C ../plugin $@
