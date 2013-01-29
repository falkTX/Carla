#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtGui)
LINK_FLAGS      += $(shell pkg-config --libs liblo QtGui)

ifeq ($(HAVE_SUIL),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags suil-0)
LINK_FLAGS      += $(shell pkg-config --libs suil-0)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth)
LINK_FLAGS      += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler)
LINK_FLAGS      += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------

OBJS = \
	carla_plugin.cpp.o \
	native.cpp.o

# 	carla_plugin_thread.cpp.o \
# 	carla_bridge.cpp.o \
# 	ladspa.cpp.o \
# 	dssi.cpp.o \
# 	lv2.cpp.o \
# 	vst.cpp.o \
# 	fluidsynth.cpp.o \
# 	linuxsampler.cpp.o

SHARED = ../libcarla_plugin.so
STATIC = ../libcarla_plugin.a

# --------------------------------------------------------------

all: $(STATIC)

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
