#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -D__UNIX_JACK__
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtXml)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Xml)
endif

# --------------------------------------------------------------

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DHAVE_GETTIMEOFDAY
BUILD_CXX_FLAGS += -Irtaudio-4.0.12 -Irtaudio-4.0.12/include -Irtmidi-2.0.1
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -D__RTAUDIO_DEBUG__ -D__RTMIDI_DEBUG__
endif
endif

# --------------------------------------------------------------

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__ -D__LINUX_ALSASEQ__
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
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

OBJSp = \
	CarlaEngine.cpp.o \
	CarlaEngineOsc.cpp.o \
	CarlaEngineThread.cpp.o \
	CarlaEngineNative.cpp.o

OBJS  = $(OBJSp) \
	CarlaEngineBridge.cpp.o \
	CarlaEngineJack.cpp.o \
	CarlaEngineRtAudio.cpp.o

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
OBJS += \
	rtaudio-4.0.12/RtAudio.cpp.o \
	rtmidi-2.0.1/RtMidi.cpp.o

ifeq ($(WIN32),true)
OBJS += \
	rtaudio-4.0.12/include/asio.cpp.o \
	rtaudio-4.0.12/include/asiodrivers.cpp.o \
	rtaudio-4.0.12/include/asiolist.cpp.o \
	rtaudio-4.0.12/include/iasiothiscallresolver.cpp.o
endif
endif

TARGET  = ../libcarla_engine.a
TARGETp = ../libcarla_engine_plugin.a

# --------------------------------------------------------------

all: $(TARGET) $(TARGETp)

clean:
	rm -f $(OBJS) $(OBJSp) $(TARGET) $(TARGETp)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

HEADERS = \
	../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.dssi.o: %.cpp $(HEADERS)
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_DSSI -c -o $@

%.cpp.lv2.o: %.cpp $(HEADERS)
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_LV2 -c -o $@

%.cpp.vst.o: %.cpp $(HEADERS)
	$(CXX) $< $(PLUGIN_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_VST -c -o $@

$(TARGET): $(OBJS)
	$(AR) rs $@ $^

$(TARGETp): $(OBJSp)
	$(AR) rs $@ $^
