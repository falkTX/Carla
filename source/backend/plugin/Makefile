#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

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
	Vst3Plugin.cpp.o \
	AuPlugin.cpp.o \
	JucePlugin.cpp.o \
	FluidSynthPlugin.cpp.o \
	LinuxSamplerPlugin.cpp.o

TARGETS = \
	../carla_plugin.a

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

../carla_plugin.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

# --------------------------------------------------------------

CarlaPlugin.cpp.o: CarlaPlugin.cpp $(CARLA_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaPluginInternal.cpp.o: CarlaPluginInternal.cpp $(CARLA_PLUGIN_INTERNAL_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaPluginThread.cpp.o: CarlaPluginThread.cpp $(CARLA_PLUGIN_THREAD_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

NativePlugin.cpp.o: NativePlugin.cpp $(NATIVE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

BridgePlugin.cpp.o: BridgePlugin.cpp $(BRIDGE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

LadspaPlugin.cpp.o: LadspaPlugin.cpp $(LADSPA_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

DssiPlugin.cpp.o: DssiPlugin.cpp $(DSSI_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

Lv2Plugin.cpp.o: Lv2Plugin.cpp $(LV2_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

VstPlugin.cpp.o: VstPlugin.cpp $(VST_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

Vst3Plugin.cpp.o: Vst3Plugin.cpp $(VST3_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

AuPlugin.cpp.o: AuPlugin.cpp $(AU_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

JucePlugin.cpp.o: JucePlugin.cpp $(JUCE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

FluidSynthPlugin.cpp.o: FluidSynthPlugin.cpp $(FLUIDSYNTH_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) -c -o $@

LinuxSamplerPlugin.cpp.o: LinuxSamplerPlugin.cpp $(LINUXSAMPLER_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) -c -o $@

# --------------------------------------------------------------
