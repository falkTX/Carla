#!/usr/bin/make -f
# Makefile for carla-bridges #
# -------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DBUILD_BRIDGE
BUILD_CXX_FLAGS += -I. -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)
LINK_FLAGS      += $(LIBLO_LIBS)

# --------------------------------------------------------------
# Plugin bridges

BUILD_PLUGIN_FLAGS  = $(BUILD_CXX_FLAGS)
BUILD_PLUGIN_FLAGS += -I../backend/engine -I../backend/plugin

LINK_PLUGIN_FLAGS   = $(LINK_FLAGS)
LINK_PLUGIN_FLAGS  += $(JUCE_AUDIO_BASICS_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_AUDIO_FORMATS_LIBS)
LINK_PLUGIN_FLAGS  += $(JUCE_CORE_LIBS)

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_PLUGIN_FLAGS += -DVESTIGE_HEADER
endif

ifeq ($(MACOS_OR_WIN32),true)
LINK_PLUGIN_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_PLUGIN_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_PLUGIN_FLAGS += $(JUCE_EVENTS_LIBS)
LINK_PLUGIN_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_PLUGIN_FLAGS += $(JUCE_GUI_BASICS_LIBS)
endif

ifeq ($(HAVE_X11),true)
LINK_PLUGIN_FLAGS += $(X11_LIBS)
endif

# --------------------------------------------------------------
# Plugin bridges (POSIX)

POSIX_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS)
POSIX_64BIT_FLAGS = $(64BIT_FLAGS)
POSIX_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS)

ifneq ($(HAIKU),true)
POSIX_LINK_FLAGS += -ldl -lpthread
endif

ifeq ($(LINUX),true)
POSIX_32BIT_FLAGS += -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS += -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
endif

# --------------------------------------------------------------
# Plugin bridges (Windows)

WIN_BUILD_FLAGS = $(BUILD_PLUGIN_FLAGS) -DJACKBRIDGE_EXPORT
WIN_32BIT_FLAGS = $(32BIT_FLAGS)
WIN_64BIT_FLAGS = $(64BIT_FLAGS)
WIN_LINK_FLAGS  = $(LINK_PLUGIN_FLAGS) -L../modules

# --------------------------------------------------------------
# Plugin bridges (Native)

NATIVE_BUILD_FLAGS  = $(POSIX_BUILD_FLAGS)
NATIVE_LINK_FLAGS   = $(POSIX_LINK_FLAGS)

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_BUILD_FLAGS += $(FLUIDSYNTH_FLAGS)
NATIVE_LINK_FLAGS  += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_BUILD_FLAGS += $(LINUXSAMPLER_FLAGS)
NATIVE_LINK_FLAGS  += $(LINUXSAMPLER_LIBS)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
NATIVE_BUILD_FLAGS += -DWANT_ZYNADDSUBFX
ifeq ($(HAVE_ZYN_UI_DEPS),true)
NATIVE_LINK_FLAGS  += -DWANT_ZYNADDSUBFX_UI
endif
endif

NATIVE_LINK_FLAGS  += $(NATIVE_PLUGINS_LIBS)

# --------------------------------------------------------------

all: native

# --------------------------------------------------------------

native:  ../../bin/carla-bridge-native$(APP_EXT)
posix32: ../../bin/carla-bridge-posix32
posix64: ../../bin/carla-bridge-posix64
win32:   ../../bin/carla-bridge-win32.exe
win64:   ../../bin/carla-bridge-win64.exe

# --------------------------------------------------------------
# native

OBJS_NATIVE  = CarlaBridgePlugin__native.o

# carla-engine
OBJS_NATIVE += \
	../backend/engine/CarlaEngine__native.o \
	../backend/engine/CarlaEngineClient__native.o \
	../backend/engine/CarlaEngineData__native.o \
	../backend/engine/CarlaEngineInternal__native.o \
	../backend/engine/CarlaEngineOsc__native.o \
	../backend/engine/CarlaEngineOscSend__native.o \
	../backend/engine/CarlaEnginePorts__native.o \
	../backend/engine/CarlaEngineThread__native.o \
	../backend/engine/CarlaEngineJack__native.o \
	../backend/engine/CarlaEngineBridge__native.o

# carla-plugin
OBJS_NATIVE += \
	../backend/plugin/CarlaPlugin__native.o \
	../backend/plugin/CarlaPluginInternal__native.o \
	../backend/plugin/CarlaPluginThread__native.o \
	../backend/plugin/NativePlugin__native.o \
	../backend/plugin/LadspaPlugin__native.o \
	../backend/plugin/DssiPlugin__native.o \
	../backend/plugin/Lv2Plugin__native.o \
	../backend/plugin/VstPlugin__native.o \
	../backend/plugin/Vst3Plugin__native.o \
	../backend/plugin/AuPlugin__native.o \
	../backend/plugin/JucePlugin__native.o \
	../backend/plugin/FluidSynthPlugin__native.o \
	../backend/plugin/LinuxSamplerPlugin__native.o

# carla-standalone
OBJS_NATIVE += \
	../backend/CarlaStandalone__native.o

# libs
LIBS_NATIVE = \
	../modules/native-plugins.a \
	../modules/jackbridge.a \
	../modules/juce_audio_basics.a \
	../modules/juce_audio_formats.a \
	../modules/juce_core.a \
	../modules/lilv.a \
	../modules/rtmempool.a

ifeq ($(MACOS_OR_WIN32),true)
LIBS_NATIVE += \
	../modules/juce_audio_processors.a \
	../modules/juce_data_structures.a \
	../modules/juce_events.a \
	../modules/juce_graphics.a \
	../modules/juce_gui_basics.a \
	../modules/juce_gui_extra.a
endif

../../bin/carla-bridge-native: $(OBJS_NATIVE) $(LIBS_NATIVE)
	$(CXX) $(OBJS_NATIVE) $(LIBS_START) $(LIBS_NATIVE) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

%__native.o: %.cpp
	$(CXX) $< $(NATIVE_BUILD_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix32

OBJS_POSIX32  = CarlaBridgePlugin__posix32.o

# carla-engine
OBJS_POSIX32 += \
	../backend/engine/CarlaEngine__posix32.o \
	../backend/engine/CarlaEngineClient__posix32.o \
	../backend/engine/CarlaEngineData__posix32.o \
	../backend/engine/CarlaEngineInternal__posix32.o \
	../backend/engine/CarlaEngineOsc__posix32.o \
	../backend/engine/CarlaEngineOscSend__posix32.o \
	../backend/engine/CarlaEnginePorts__posix32.o \
	../backend/engine/CarlaEngineThread__posix32.o \
	../backend/engine/CarlaEngineJack__posix32.o \
	../backend/engine/CarlaEngineBridge__posix32.o

# carla-plugin
OBJS_POSIX32 += \
	../backend/plugin/CarlaPlugin__posix32.o \
	../backend/plugin/CarlaPluginInternal__posix32.o \
	../backend/plugin/CarlaPluginThread__posix32.o \
	../backend/plugin/LadspaPlugin__posix32.o \
	../backend/plugin/DssiPlugin__posix32.o \
	../backend/plugin/Lv2Plugin__posix32.o \
	../backend/plugin/VstPlugin__posix32.o \
	../backend/plugin/Vst3Plugin__posix32.o \
	../backend/plugin/AuPlugin__posix32.o \
	../backend/plugin/JucePlugin__posix32.o

# carla-standalone
OBJS_POSIX32 += \
	../backend/CarlaStandalone__posix32.o

# libs
LIBS_POSIX32 = \
	../modules/jackbridge.posix32.a \
	../modules/juce_audio_basics.posix32.a \
	../modules/juce_core.posix32.a \
	../modules/lilv.posix32.a \
	../modules/rtmempool.posix32.a

ifeq ($(MACOS),true)
LIBS_POSIX32 += \
	../modules/juce_audio_processors.posix32.a \
	../modules/juce_data_structures.posix32.a \
	../modules/juce_events.posix32.a \
	../modules/juce_graphics.posix32.a \
	../modules/juce_gui_basics.posix32.a
endif

../../bin/carla-bridge-posix32: $(OBJS_POSIX32) $(LIBS_POSIX32)
	$(CXX) $(OBJS_POSIX32) $(LIBS_START) $(LIBS_POSIX32) $(LIBS_END) $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@

%__posix32.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# posix64

OBJS_POSIX64  = CarlaBridgePlugin__posix64.o

# carla-engine
OBJS_POSIX64 += \
	../backend/engine/CarlaEngine__posix64.o \
	../backend/engine/CarlaEngineClient__posix64.o \
	../backend/engine/CarlaEngineData__posix64.o \
	../backend/engine/CarlaEngineInternal__posix64.o \
	../backend/engine/CarlaEngineOsc__posix64.o \
	../backend/engine/CarlaEngineOscSend__posix64.o \
	../backend/engine/CarlaEnginePorts__posix64.o \
	../backend/engine/CarlaEngineThread__posix64.o \
	../backend/engine/CarlaEngineJack__posix64.o \
	../backend/engine/CarlaEngineBridge__posix64.o

# carla-plugin
OBJS_POSIX64 += \
	../backend/plugin/CarlaPlugin__posix64.o \
	../backend/plugin/CarlaPluginInternal__posix64.o \
	../backend/plugin/CarlaPluginThread__posix64.o \
	../backend/plugin/LadspaPlugin__posix64.o \
	../backend/plugin/DssiPlugin__posix64.o \
	../backend/plugin/Lv2Plugin__posix64.o \
	../backend/plugin/VstPlugin__posix64.o \
	../backend/plugin/Vst3Plugin__posix64.o \
	../backend/plugin/AuPlugin__posix32.o \
	../backend/plugin/JucePlugin__posix64.o

# carla-standalone
OBJS_POSIX64 += \
	../backend/CarlaStandalone__posix64.o

# libs
LIBS_POSIX64 = \
	../modules/jackbridge.posix64.a \
	../modules/juce_audio_basics.posix64.a \
	../modules/juce_core.posix64.a \
	../modules/lilv.posix64.a \
	../modules/rtmempool.posix64.a

ifeq ($(MACOS),true)
LIBS_POSIX64 += \
	../modules/juce_audio_processors.posix64.a \
	../modules/juce_data_structures.posix64.a \
	../modules/juce_events.posix64.a \
	../modules/juce_graphics.posix64.a \
	../modules/juce_gui_basics.posix64.a
endif

../../bin/carla-bridge-posix64: $(OBJS_POSIX64) $(LIBS_POSIX64)
	$(CXX) $(OBJS_POSIX64) $(LIBS_START) $(LIBS_POSIX64) $(LIBS_END) $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@

%__posix64.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win32

OBJS_WIN32  = CarlaBridgePlugin__win32.o

# carla-engine
OBJS_WIN32 += \
	../backend/engine/CarlaEngine__win32.o \
	../backend/engine/CarlaEngineClient__win32.o \
	../backend/engine/CarlaEngineData__win32.o \
	../backend/engine/CarlaEngineInternal__win32.o \
	../backend/engine/CarlaEngineOsc__win32.o \
	../backend/engine/CarlaEngineOscSend__win32.o \
	../backend/engine/CarlaEnginePorts__win32.o \
	../backend/engine/CarlaEngineThread__win32.o \
	../backend/engine/CarlaEngineBridge__win32.o \
	../backend/engine/CarlaEngineJack__win32.o

# carla-plugin
OBJS_WIN32 += \
	../backend/plugin/CarlaPlugin__win32.o \
	../backend/plugin/CarlaPluginInternal__win32.o \
	../backend/plugin/CarlaPluginThread__win32.o \
	../backend/plugin/LadspaPlugin__win32.o \
	../backend/plugin/DssiPlugin__win32.o \
	../backend/plugin/Lv2Plugin__win32.o \
	../backend/plugin/VstPlugin__win32.o \
	../backend/plugin/Vst3Plugin__win32.o \
	../backend/plugin/AuPlugin__win32.o \
	../backend/plugin/JucePlugin__win32.o

# carla-standalone
OBJS_WIN32 += \
	../backend/CarlaStandalone__win32.o

# libs
LIBS_WIN32 = \
	../modules/lilv.win32.a \
	../modules/rtmempool.win32.a

LIBS_WIN32 += \
	../modules/juce_audio_basics.win32.a \
	../modules/juce_audio_processors.win32.a \
	../modules/juce_core.win32.a \
	../modules/juce_data_structures.win32.a \
	../modules/juce_events.win32.a \
	../modules/juce_graphics.win32.a \
	../modules/juce_gui_basics.win32.a

../../bin/carla-bridge-win32.exe: $(OBJS_WIN32) $(LIBS_WIN32) ../modules/jackbridge-win32.dll
	$(CXX) $(OBJS_WIN32) $(LIBS_START) $(LIBS_WIN32) $(LIBS_END) $(WIN_LINK_FLAGS) $(WIN_32BIT_FLAGS) -ljackbridge-win32 -o $@

%__win32.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) -c -o $@

# --------------------------------------------------------------
# win64

OBJS_WIN64  = CarlaBridgePlugin__win64.o

# carla-engine
OBJS_WIN64 += \
	../backend/engine/CarlaEngine__win64.o \
	../backend/engine/CarlaEngineClient__win64.o \
	../backend/engine/CarlaEngineData__win64.o \
	../backend/engine/CarlaEngineInternal__win64.o \
	../backend/engine/CarlaEngineOsc__win64.o \
	../backend/engine/CarlaEngineOscSend__win64.o \
	../backend/engine/CarlaEnginePorts__win64.o \
	../backend/engine/CarlaEngineThread__win64.o \
	../backend/engine/CarlaEngineBridge__win64.o \
	../backend/engine/CarlaEngineJack__win64.o

# carla-plugin
OBJS_WIN64 += \
	../backend/plugin/CarlaPlugin__win64.o \
	../backend/plugin/CarlaPluginInternal__win64.o \
	../backend/plugin/CarlaPluginThread__win64.o \
	../backend/plugin/LadspaPlugin__win64.o \
	../backend/plugin/DssiPlugin__win64.o \
	../backend/plugin/Lv2Plugin__win64.o \
	../backend/plugin/VstPlugin__win64.o \
	../backend/plugin/Vst3Plugin__win64.o \
	../backend/plugin/AuPlugin__win64.o \
	../backend/plugin/JucePlugin__win64.o

# carla-standalone
OBJS_WIN64 += \
	../backend/CarlaStandalone__win64.o

# libs
LIBS_WIN64 = \
	../modules/lilv.win64.a \
	../modules/rtmempool.win64.a

LIBS_WIN64 += \
	../modules/juce_audio_basics.win64.a \
	../modules/juce_audio_processors.win64.a \
	../modules/juce_core.win64.a \
	../modules/juce_data_structures.win64.a \
	../modules/juce_events.win64.a \
	../modules/juce_graphics.win64.a \
	../modules/juce_gui_basics.win64.a

../../bin/carla-bridge-win64.exe: $(OBJS_WIN64) $(LIBS_WIN64) ../modules/jackbridge-win64.dll
	$(CXX) $(OBJS_WIN64) $(LIBS_START) $(LIBS_WIN64) $(LIBS_END) $(WIN_LINK_FLAGS) $(WIN_64BIT_FLAGS) -ljackbridge-win64 -o $@

%__win64.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o
	$(RM) ../../bin/carla-bridge-*
	$(RM) $(OBJS_NATIVE)
	$(RM) $(OBJS_POSIX32)
	$(RM) $(OBJS_POSIX64)
	$(RM) $(OBJS_WIN32)
	$(RM) $(OBJS_WIN64)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
