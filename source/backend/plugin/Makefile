#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler)
endif

# --------------------------------------------------------------

OBJS = \
	CarlaPlugin.cpp.o \
	CarlaPluginThread.cpp.o \
	BridgePlugin.cpp.o \
	NativePlugin.cpp.o \
	LadspaPlugin.cpp.o \
	DssiPlugin.cpp.o \
	Lv2Plugin.cpp.o \
	VstPlugin.cpp.o \
	Vst3Plugin.cpp.o \
	FluidSynthPlugin.cpp.o \
	LinuxSamplerPlugin.cpp.o

HEADERS = \
	../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaPluginInternal.hpp CarlaPluginThread.hpp

TARGET = ../libcarla_plugin.a

# --------------------------------------------------------------

all: $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
