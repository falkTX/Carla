#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC
BUILD_CXX_FLAGS += -I. -I.. -I../../includes -I../../libs -I../../utils
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo QtCore)

LINK_FLAGS      += -shared
LINK_FLAGS      += $(shell pkg-config --libs liblo QtCore)

ifeq ($(HAVE_JACK),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags jack) -D__UNIX_JACK__
LINK_FLAGS      += $(shell pkg-config --libs jack)
WANT_JACK        = true
endif

ifeq ($(HAVE_ALSA),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__ -D__LINUX_ALSASEQ__
LINK_FLAGS      += $(shell pkg-config --libs alsa)
WANT_RTAUDIO     = true
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
LINK_FLAGS      += $(shell pkg-config --libs libpulse-simple)
WANT_RTAUDIO     = true
endif

OBJS = \
     carla_engine.cpp.o \
     carla_engine_osc.cpp.o \
     carla_engine_thread.cpp.o \
     jack.cpp.o \
     rtaudio.cpp.o

TARGET = ../carla_engine.so

# --------------------------------------------------------------

ifeq ($(WANT_JACK),true)
BUILD_CXX_FLAGS += -DCARLA_ENGINE_JACK
endif

ifeq ($(WANT_RTAUDIO),true)
BUILD_CXX_FLAGS += -DCARLA_ENGINE_RTAUDIO -DHAVE_GETTIMEOFDAY
BUILD_CXX_FLAGS += -Irtaudio-4.0.11 -Irtmidi-2.0.1
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -D__RTAUDIO_DEBUG__ -D__RTMIDI_DEBUG__
else
BUILD_CXX_FLAGS += -D_FORTIFY_SOURCE=2
endif
OBJS += rtaudio-4.0.11/RtAudio.cpp.o
OBJS += rtmidi-2.0.1/RtMidi.cpp.o
endif

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ $(LINK_FLAGS) -o $@
