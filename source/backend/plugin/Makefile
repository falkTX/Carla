#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	CarlaPlugin.cpp.o \
	CarlaPluginInternal.cpp.o \
	CarlaPluginThread.cpp.o \
	CarlaPluginUi.cpp.o \
	NativePlugin.cpp.o \
	BridgePlugin.cpp.o \
	LadspaPlugin.cpp.o \
	DssiPlugin.cpp.o \
	Lv2Plugin.cpp.o \
	VstPlugin.cpp.o \
	Vst3Plugin.cpp.o \
	AuPlugin.cpp.o \
	ReWirePlugin.cpp.o \
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

CarlaPlugin.cpp.o: CarlaPlugin.cpp $(CARLA_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) $(QTXML_FLAGS) -c -o $@

CarlaPluginInternal.cpp.o: CarlaPluginInternal.cpp $(CARLA_PLUGIN_INTERNAL_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

CarlaPluginThread.cpp.o: CarlaPluginThread.cpp $(CARLA_PLUGIN_THREAD_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

CarlaPluginUi.cpp.o: CarlaPluginUi.cpp $(CARLA_PLUGIN_UI_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

NativePlugin.cpp.o: NativePlugin.cpp $(NATIVE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

BridgePlugin.cpp.o: BridgePlugin.cpp $(BRIDGE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

LadspaPlugin.cpp.o: LadspaPlugin.cpp $(LADSPA_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

DssiPlugin.cpp.o: DssiPlugin.cpp $(DSSI_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

Lv2Plugin.cpp.o: Lv2Plugin.cpp $(LV2_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

VstPlugin.cpp.o: VstPlugin.cpp $(VST_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

Vst3Plugin.cpp.o: Vst3Plugin.cpp $(VST3_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

AuPlugin.cpp.o: AuPlugin.cpp $(AU_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

ReWirePlugin.cpp.o: ReWirePlugin.cpp $(REWIRE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CsoundPlugin.cpp.o: CsoundPlugin.cpp $(CSOUND_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(CSOUND_FLAGS) -c -o $@

JucePlugin.cpp.o: JucePlugin.cpp $(JUCE_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

FluidSynthPlugin.cpp.o: FluidSynthPlugin.cpp $(FLUIDSYNTH_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) $(QTCORE_FLAGS) -c -o $@

LinuxSamplerPlugin.cpp.o: LinuxSamplerPlugin.cpp $(LINUXSAMPLER_PLUGIN_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) $(QTCORE_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
