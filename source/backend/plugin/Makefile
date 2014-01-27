#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

CARLA_ENGINE_OSC_HPP      = ../engine/CarlaEngineOsc.hpp $(CARLA_BACKEND_H) $(CARLA_OSC_UTILS_HPP) $(CARLA_STRING_HPP)
CARLA_PLUGIN_THREAD_HPP   = CarlaPluginThread.hpp $(CARLA_BACKEND_H) $(CARLA_THREAD_HPP)
CARLA_PLUGIN_INTERNAL_HPP = CarlaPluginInternal.hpp $(CARLA_PLUGIN_HPP) $(CARLA_PLUGIN_THREAD_HPP) $(CARLA_OSC_UTILS_HPP) $(CARLA_STATE_UTILS_HPP) $(CARLA_MUTEX_HPP) $(RT_LINKED_LIST_HPP) $(CARLA_MIDI_H)

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	CarlaPlugin.cpp.o \
	CarlaPluginInternal.cpp.o \
	CarlaPluginThread.cpp.o \
	NativePlugin.cpp.o \
	BridgePlugin.cpp.o \
	LadspaPlugin.cpp.o \
	DssiPlugin.cpp.o \
	Lv2Plugin.cpp.o \
	VstPlugin.cpp.o \
	AuPlugin.cpp.o \
	CsoundPlugin.cpp.o \
	JucePlugin.cpp.o \
	FluidSynthPlugin.cpp.o \
	LinuxSamplerPlugin.cpp.o

# ----------------------------------------------------------------------------------------------------------------------------

TARGET = ../carla_plugin.a

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

clean:
	$(RM) *.o $(TARGET)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(TARGET): $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

CarlaPlugin.cpp.o: CarlaPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) $(QTXML_FLAGS) -c -o $@

CarlaPluginInternal.cpp.o: CarlaPluginInternal.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_LIB_COUNTER_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

CarlaPluginThread.cpp.o: CarlaPluginThread.cpp $(CARLA_PLUGIN_HPP) $(CARLA_PLUGIN_THREAD_HPP) $(CARLA_ENGINE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

NativePlugin.cpp.o: NativePlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_NATIVE_H) $(CARLA_HOST_H)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

BridgePlugin.cpp.o: BridgePlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_BRIDGE_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_SHM_UTILS_HPP) $(JACK_BRIDGE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

LadspaPlugin.cpp.o: LadspaPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_LADSPA_UTILS_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

DssiPlugin.cpp.o: DssiPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_DSSI_UTILS_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@ # FIXME

Lv2Plugin.cpp.o: Lv2Plugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_LV2_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(LV2_ATOM_QUEUE_HPP) $(CARLA_ENGINE_OSC_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@ # FIXME

VstPlugin.cpp.o: VstPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_VST_UTILS_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

AuPlugin.cpp.o: AuPlugin.cpp $(CARLA_PLUGIN_HPP) $(CARLA_ENGINE_HPP) $(CARLA_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CsoundPlugin.cpp.o: CsoundPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(CSOUND_FLAGS) -c -o $@

JucePlugin.cpp.o: JucePlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

FluidSynthPlugin.cpp.o: FluidSynthPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) $(QTCORE_FLAGS) -c -o $@

LinuxSamplerPlugin.cpp.o: LinuxSamplerPlugin.cpp $(CARLA_PLUGIN_INTERNAL_HPP) $(CARLA_ENGINE_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) $(QTCORE_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
