#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(RTAUDIO_FLAGS)
BUILD_CXX_FLAGS += $(RTMIDI_FLAGS)

# --------------------------------------------------------------

OBJSp = \
	CarlaEngine.cpp.o \
	CarlaEngineOsc.cpp.o \
	CarlaEngineThread.cpp.o \
	CarlaEngineNative.cpp.o

OBJS  = $(OBJSp) \
	CarlaEngineJack.cpp.o \
	CarlaEngineRtAudio.cpp.o

# ifeq ($(HAVE_JUCE),true)
# OBJS += \
# 	CarlaEngineJuce.cpp.o
# endif

HEADERS = \
	../CarlaBackend.h ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp

TARGET  = ../libcarla_engine.a
TARGETp = ../libcarla_engine_plugin.a

# --------------------------------------------------------------

all: $(TARGET) $(TARGETp)

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(TARGET): $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

$(TARGETp): $(OBJSp)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS) $(OBJSp) $(TARGET) $(TARGETp)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
