#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)
LINK_FLAGS      += $(shell pkg-config --libs liblo)

ifeq ($(HAVE_QT5),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Xml)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Xml)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtXml)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtXml)
endif

# --------------------------------------------------------------

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DHAVE_GETTIMEOFDAY
BUILD_CXX_FLAGS += -Irtaudio-4.0.11 -Irtaudio-4.0.11/include -Irtmidi-2.0.1
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -D__RTAUDIO_DEBUG__ -D__RTMIDI_DEBUG__
endif
endif

# --------------------------------------------------------------

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__ -D__LINUX_ALSASEQ__
LINK_FLAGS      += $(shell pkg-config --libs alsa)
endif

ifeq ($(HAVE_JACK),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack) -D__UNIX_JACK__
LINK_FLAGS      += $(shell pkg-config --libs jack)
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
LINK_FLAGS      += $(shell pkg-config --libs libpulse-simple)
endif

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -D__MACOSX_CORE__
endif

ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__ -D__WINDOWS_MM__
endif

PLUGIN_CXX_FLAGS  = $(BUILD_CXX_FLAGS)
PLUGIN_CXX_FLAGS += -Idistrho -I../../libs/distrho
PLUGIN_CXX_FLAGS += -DWANT_PLUGIN

# --------------------------------------------------------------

OBJS  = \
	CarlaEngine.cpp.o \
	CarlaEngineOsc.cpp.o \
	CarlaEngineThread.cpp.o \
	CarlaEngineBridge.cpp.o \
	CarlaEngineJack.cpp.o \
	CarlaEnginePlugin.cpp.o \
	CarlaEngineRtAudio.cpp.o

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
OBJS += \
	rtaudio-4.0.11/RtAudio.cpp.o \
	rtmidi-2.0.1/RtMidi.cpp.o

ifeq ($(WIN32),true)
OBJS += \
	rtaudio-4.0.11/include/asio.cpp.o \
	rtaudio-4.0.11/include/asiodrivers.cpp.o \
	rtaudio-4.0.11/include/asiolist.cpp.o \
	rtaudio-4.0.11/include/iasiothiscallresolver.cpp.o
endif
endif

SHARED = ../libcarla_engine.so
STATIC = ../libcarla_engine.a

# --------------------------------------------------------------

all: $(STATIC)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.dssi.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_DSSI -c -o $@

%.cpp.lv2.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_LV2 -c -o $@

%.cpp.vst.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_VST -c -o $@

$(SHARED): $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@ && $(STRIP) $@

$(STATIC): $(OBJS)
	$(AR) rs $@ $^
