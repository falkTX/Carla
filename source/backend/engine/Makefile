#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore)
LINK_FLAGS      += $(shell pkg-config --libs liblo QtCore)

# --------------------------------------------------------------

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DHAVE_GETTIMEOFDAY
BUILD_CXX_FLAGS += -Irtaudio-4.0.11 -Irtmidi-2.0.1
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

# --------------------------------------------------------------

OBJS  = \
     CarlaEngine.cpp.o \
     CarlaEngineJack.cpp.o \
     CarlaEnginePlugin.cpp.o \
     CarlaEngineRtAudio.cpp.o \
     CarlaEngineOsc.cpp.o \
     CarlaEngineThread.cpp.o

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
OBJS += \
	rtaudio-4.0.11/RtAudio.cpp.o \
	rtmidi-2.0.1/RtMidi.cpp.o
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

$(SHARED): $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

$(STATIC): $(OBJS)
	$(AR) rs $@ $^
