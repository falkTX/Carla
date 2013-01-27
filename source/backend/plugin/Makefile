#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I. -I.. -I../../includes -I../../libs -I../../utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore QtGui)

LINK_FLAGS      += -shared
LINK_FLAGS      += $(shell pkg-config --libs liblo QtCore QtGui)

ifeq ($(HAVE_SUIL),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags suil-0) -DWANT_SUIL
LINK_FLAGS      += $(shell pkg-config --libs suil-0)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth) -DWANT_FLUIDSYNTH
LINK_FLAGS      += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler) -DWANT_LINUXSAMPLER
LINK_FLAGS      += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------

OBJS = \
	carla_plugin.cpp.o
# 	carla_plugin_thread.cpp.o \
# 	carla_bridge.cpp.o \
# 	native.cpp.o \
# 	ladspa.cpp.o \
# 	dssi.cpp.o \
# 	lv2.cpp.o \
# 	vst.cpp.o \
# 	fluidsynth.cpp.o \
# 	linuxsampler.cpp.o

TARGET = ../carla_plugin.so

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
