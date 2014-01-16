#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

OBJS = \
	CarlaEngine.cpp.o \
	CarlaEngineInternal.cpp.o \
	CarlaEngineOsc.cpp.o \
	CarlaEngineThread.cpp.o

OBJSa = $(OBJS) \
	CarlaEngineJack.cpp.o \
	CarlaEngineRtAudio.cpp.o

ifeq ($(HAVE_JUCE),true)
OBJSa += \
	CarlaEngineJuce.cpp.o
endif

ifneq ($(WIN32),true)
OBJSa += \
	CarlaEngineNative.cpp.o
OBJSp  = $(OBJS) \
	CarlaEngineNative.cpp.exp.o
endif

TARGETa = ../carla_engine.a

ifneq ($(WIN32),true)
TARGETp = ../carla_engine_plugin.a
endif

# --------------------------------------------------------------

all: $(TARGETa) $(TARGETp)

# --------------------------------------------------------------

CarlaEngine.cpp.o: CarlaEngine.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_ENGINE_UTILS_HPP) $(CARLA_STATE_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaEngineRtAudio.cpp.o: CarlaEngineRtAudio.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(RTAUDIO_FLAGS) $(RTMIDI_FLAGS) -c -o $@

$(TARGETa): $(OBJSa)
	$(RM) $@
	$(AR) crs $@ $^

$(TARGETp): $(OBJSp)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

clean:
	$(RM) *.o $(OBJSp) $(TARGET) $(TARGETp)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

CARLA_ENGINE_INTERNAL_HPP = CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp $(CARLA_ENGINE_HPP) $(CARLA_MUTEX_HPP) $(LINKED_LIST_HPP)

# %.cpp.o: %.cpp $(HEADERS)
# 	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@
#
# %.cpp.exp.o: %.cpp $(HEADERS)
# 	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -c -o $@
