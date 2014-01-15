#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

OBJS = \
	CarlaEngine.cpp.o \
	CarlaEngine_data.cpp.o \
	CarlaEngine_internal.cpp.o \
	CarlaEngine_osc.cpp.o \
	CarlaEngine_patchbay.cpp.o \
	CarlaEngine_ports.cpp.o \
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

HEADERS = \
	../CarlaBackend.h ../CarlaEngine.hpp ../CarlaPlugin.hpp \
	CarlaEngineInternal.hpp CarlaEngineOsc.hpp CarlaEngineThread.hpp

TARGETa = ../carla_engine.a

ifneq ($(WIN32),true)
TARGETp = ../carla_engine_plugin.a
endif

# --------------------------------------------------------------

all: $(TARGETa) $(TARGETp)

# --------------------------------------------------------------

%.cpp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.exp.o: %.cpp $(HEADERS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -c -o $@

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
