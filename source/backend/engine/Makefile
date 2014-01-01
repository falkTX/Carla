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

OBJS = \
	CarlaEngine.cpp.o \
	CarlaEngineOsc.cpp.o \
	CarlaEngineThread.cpp.o

OBJSa = $(OBJS) \
	CarlaEngineNative.cpp.o \
	CarlaEngineJack.cpp.o \
	CarlaEngineRtAudio.cpp.o

ifeq ($(HAVE_JUCE),true)
OBJSa += \
	CarlaEngineJuce.cpp.o
endif

OBJSp = $(OBJS) \
	CarlaEngineNative.cpp.exp.o

HEADERS = \
	../CarlaBackend.h ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp

TARGETa = ../libcarla_engine.a
TARGETp = ../libcarla_engine_plugin.a

# --------------------------------------------------------------

all: $(TARGETa) $(TARGETp)

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.exp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -c -o $@

$(TARGETa): $(OBJSa)
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
