#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)

# ifeq ($(HAVE_QT4),true)
# BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui QtXml)
# else
# BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Xml Qt5Widgets)
# endif

# --------------------------------------------------------------

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags linuxsampler)
endif

# --------------------------------------------------------------

OBJS = \
	CarlaPlugin.cpp.o \
	CarlaPluginGui.cpp.o \
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

FILES = \
	moc_CarlaPluginGui.cpp

TARGET = ../libcarla_plugin.a

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(FILES) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaPluginGui.hpp CarlaPluginInternal.hpp CarlaPluginThread.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaPluginGui.cpp.o: CarlaPluginGui.cpp moc_CarlaPluginGui.cpp CarlaPluginGui.hpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaPluginInternal.hpp CarlaPluginThread.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -DMOC_PARSING -o $@

$(TARGET): $(OBJS)
	$(AR) rs $@ $^
