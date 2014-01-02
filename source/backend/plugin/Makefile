#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

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
	CsoundPlugin.cpp.o \
	FluidSynthPlugin.cpp.o \
	LinuxSamplerPlugin.cpp.o \
	JucePlugin.cpp.o

HEADERS = \
	../CarlaBackend.h ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaPluginInternal.hpp CarlaPluginThread.hpp

TARGET = ../carla_plugin.a

# --------------------------------------------------------------

all: $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

FluidSynthPlugin.cpp.o: FluidSynthPlugin.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) -c -o $@

LinuxSamplerPlugin.cpp.o: LinuxSamplerPlugin.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
