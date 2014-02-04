#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

CARLA_ENGINE_OSC_HPP      = CarlaEngineOsc.hpp $(CARLA_BACKEND_H) $(CARLA_OSC_UTILS_HPP) $(CARLA_MUTEX_HPP) $(CARLA_STRING_HPP)
CARLA_ENGINE_THREAD_HPP   = CarlaEngineThread.hpp $(CARLA_BACKEND_H) $(CARLA_THREAD_HPP)
CARLA_ENGINE_INTERNAL_HPP = CarlaEngineInternal.hpp $(CARLA_ENGINE_OSC_HPP) $(CARLA_ENGINE_THREAD_HPP) $(CARLA_ENGINE_HPP) $(CARLA_MUTEX_HPP) $(LINKED_LIST_HPP)

# ----------------------------------------------------------------------------------------------------------------------------

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

# ----------------------------------------------------------------------------------------------------------------------------

TARGETa = ../carla_engine.a

ifneq ($(WIN32),true)
TARGETp = ../carla_engine_plugin.a
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETa) $(TARGETp)

clean:
	$(RM) *.o $(TARGET) $(TARGETp)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(TARGETa): $(OBJSa)
	$(RM) $@
	$(AR) crs $@ $^

$(TARGETp): $(OBJSp)
	$(RM) $@
	$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

CarlaEngine.cpp.o: CarlaEngine.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_ENGINE_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_STATE_UTILS_HPP) $(CARLA_MIDI_H)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) $(QTXML_FLAGS) -c -o $@

CarlaEngineInternal.cpp.o: CarlaEngineInternal.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_MIDI_H) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaEngineOsc.cpp.o: CarlaEngineOsc.cpp $(CARLA_ENGINE_HPP) $(CARLA_ENGINE_OSC_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BRIDGE_UTILS_HPP) $(CARLA_MIDI_H)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaEngineThread.cpp.o: CarlaEngineThread.cpp $(CARLA_ENGINE_HPP) $(CARLA_ENGINE_THREAD_HPP) $(CARLA_PLUGIN_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaEngineJack.cpp.o: CarlaEngineJack.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_ENGINE_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_MIDI_H) $(JACK_BRIDGE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

CarlaEngineRtAudio.cpp.o: CarlaEngineRtAudio.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(RT_LINKED_LIST_HPP) $(RTAUDIO_HPP) $(RTMIDI_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(RTAUDIO_FLAGS) $(RTMIDI_FLAGS) -c -o $@

CarlaEngineJuce.cpp.o: CarlaEngineJuce.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_BACKEND_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaEngineNative.cpp.o: CarlaEngineNative.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_PIPE_UTILS_HPP) $(CARLA_STATE_UTILS_HPP) $(CARLA_NATIVE_HPP) $(CARLA_STATE_UTILS_CPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) $(QTXML_FLAGS) -c -o $@

CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp $(CARLA_ENGINE_INTERNAL_HPP) $(CARLA_PLUGIN_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_PIPE_UTILS_HPP) $(CARLA_STATE_UTILS_HPP) $(CARLA_NATIVE_HPP) $(CARLA_STATE_UTILS_CPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) $(QTXML_FLAGS) -DCARLA_PLUGIN_EXPORT -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
