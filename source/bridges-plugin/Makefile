#!/usr/bin/make -f
# Makefile for carla-bridges #
# -------------------------- #
# Created by falkTX
#

CWD=..
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/bridges-plugin/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/bridges-plugin/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin bridges (Common)

BUILD_CXX_FLAGS += -DBUILD_BRIDGE -I. -I../backend -I../backend/engine -I../backend/plugin -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif

LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_FORMATS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(MACOS),true)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

LINK_FLAGS += $(LIBLO_LIBS)

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin bridges (POSIX)

POSIX_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
POSIX_32BIT_FLAGS = $(32BIT_FLAGS)
POSIX_64BIT_FLAGS = $(64BIT_FLAGS)
POSIX_LINK_FLAGS  = $(LINK_FLAGS)

ifneq ($(HAIKU),true)
POSIX_LINK_FLAGS += -ldl -lpthread
endif

ifeq ($(LINUX),true)
POSIX_32BIT_FLAGS += -L/usr/lib32 -L/usr/lib/i386-linux-gnu
POSIX_64BIT_FLAGS += -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin bridges (Windows)

WIN_BUILD_FLAGS = $(BUILD_CXX_FLAGS)
WIN_32BIT_FLAGS = $(32BIT_FLAGS)
WIN_64BIT_FLAGS = $(64BIT_FLAGS)
WIN_LINK_FLAGS  = $(LINK_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin bridges (Native)

ifeq ($(WIN32),true)
NATIVE_BUILD_FLAGS  = $(WIN_BUILD_FLAGS)
NATIVE_LINK_FLAGS   = $(WIN_LINK_FLAGS)
else
NATIVE_BUILD_FLAGS  = $(POSIX_BUILD_FLAGS)
NATIVE_LINK_FLAGS   = $(POSIX_LINK_FLAGS)
endif

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
NATIVE_BUILD_FLAGS += -DWANT_ZYNADDSUBFX_UI
endif
endif

NATIVE_LINK_FLAGS  += $(NATIVE_PLUGINS_LIBS)

# ----------------------------------------------------------------------------------------------------------------------------

all: native

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/CarlaBridgePlugin*.o $(BINDIR)/carla-bridge-*
	rm -f $(OBJS_NATIVE)
	rm -f $(OBJS_POSIX32)
	rm -f $(OBJS_POSIX64)
	rm -f $(OBJS_WIN32)
	rm -f $(OBJS_WIN64)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

native:  $(BINDIR)/carla-bridge-native$(APP_EXT)
posix32: $(BINDIR)/carla-bridge-posix32
posix64: $(BINDIR)/carla-bridge-posix64
win32:   $(BINDIR)/carla-bridge-win32.exe
win64:   $(BINDIR)/carla-bridge-win64.exe

# ----------------------------------------------------------------------------------------------------------------------------
# native

OBJS_NATIVE  = $(OBJDIR)/CarlaBridgePlugin.cpp.o

# carla-engine
OBJS_NATIVE += \
	$(OBJDIR)/CarlaEngine.cpp.o \
	$(OBJDIR)/CarlaEngineClient.cpp.o \
	$(OBJDIR)/CarlaEngineData.cpp.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.o \
	$(OBJDIR)/CarlaEngineThread.cpp.o \
	$(OBJDIR)/CarlaEngineJack.cpp.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.o

# carla-plugin
OBJS_NATIVE += \
	$(OBJDIR)/CarlaPlugin.cpp.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.o \
	$(OBJDIR)/CarlaPluginThread.cpp.o \
	$(OBJDIR)/NativePlugin.cpp.o \
	$(OBJDIR)/LadspaPlugin.cpp.o \
	$(OBJDIR)/DssiPlugin.cpp.o \
	$(OBJDIR)/Lv2Plugin.cpp.o \
	$(OBJDIR)/VstPlugin.cpp.o \
	$(OBJDIR)/Vst3Plugin.cpp.o \
	$(OBJDIR)/AuPlugin.cpp.o \
	$(OBJDIR)/JucePlugin.cpp.o \
	$(OBJDIR)/FluidSynthPlugin.cpp.o \
	$(OBJDIR)/LinuxSamplerPlugin.cpp.o

# carla-standalone
OBJS_NATIVE += \
	$(OBJDIR)/CarlaStandalone.cpp.o

# libs
LIBS_NATIVE = \
	$(MODULEDIR)/jackbridge.a \
	$(MODULEDIR)/juce_audio_basics.a \
	$(MODULEDIR)/juce_audio_formats.a \
	$(MODULEDIR)/juce_core.a \
	$(MODULEDIR)/lilv.a \
	$(MODULEDIR)/native-plugins.a \
	$(MODULEDIR)/rtmempool.a

ifeq ($(MACOS_OR_WIN32),true)
LIBS_NATIVE += \
	$(MODULEDIR)/juce_audio_processors.a \
	$(MODULEDIR)/juce_data_structures.a \
	$(MODULEDIR)/juce_events.a \
	$(MODULEDIR)/juce_graphics.a \
	$(MODULEDIR)/juce_gui_basics.a

ifeq ($(MACOS),true)
LIBS_NATIVE += \
	$(MODULEDIR)/juce_gui_extra.a
endif
endif

$(BINDIR)/carla-bridge-native$(APP_EXT): $(OBJS_NATIVE) $(LIBS_NATIVE)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-bridge-native$(APP_EXT)"
	@$(CXX) $(OBJS_NATIVE) $(LIBS_START) $(LIBS_NATIVE) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(OBJDIR)/CarlaBridgePlugin.cpp.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (bridge)"
	@$(CXX) $< $(NATIVE_BUILD_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaEng%.cpp.o: ../backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (bridge)"
	@$(CXX) $< $(NATIVE_BUILD_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.o: ../backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (bridge)"
	@$(CXX) $< $(NATIVE_BUILD_FLAGS) -MMD -c -o $@

$(OBJDIR)/%Plugin.cpp.o: ../backend/plugin/%Plugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*Plugin.cpp (bridge)"
	@$(CXX) $< $(NATIVE_BUILD_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.o: ../backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (bridge)"
	@$(CXX) $< $(NATIVE_BUILD_FLAGS) -MMD -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# posix32

OBJS_POSIX32  = CarlaBridgePlugin.cpp.posix32.o

# carla-engine
OBJS_POSIX32 += \
	$(OBJDIR)/CarlaEngine.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineClient.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineData.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.posix32.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineThread.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineJack.cpp.posix32.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.posix32.o

# carla-plugin
OBJS_POSIX32 += \
	$(OBJDIR)/CarlaPlugin.cpp.posix32.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.posix32.o \
	$(OBJDIR)/CarlaPluginThread.cpp.posix32.o \
	$(OBJDIR)/LadspaPlugin.cpp.posix32.o \
	$(OBJDIR)/DssiPlugin.cpp.posix32.o \
	$(OBJDIR)/Lv2Plugin.cpp.posix32.o \
	$(OBJDIR)/VstPlugin.cpp.posix32.o \
	$(OBJDIR)/Vst3Plugin.cpp.posix32.o \
	$(OBJDIR)/AuPlugin.cpp.posix32.o \
	$(OBJDIR)/JucePlugin.cpp.posix32.o

# carla-standalone
OBJS_POSIX32 += \
	$(OBJDIR)/CarlaStandalone.cpp.posix32.o

# libs
LIBS_POSIX32 = \
	$(MODULEDIR)/jackbridge.posix32.a \
	$(MODULEDIR)/juce_audio_basics.posix32.a \
	$(MODULEDIR)/juce_core.posix32.a \
	$(MODULEDIR)/lilv.posix32.a \
	$(MODULEDIR)/rtmempool.posix32.a

ifeq ($(MACOS),true)
LIBS_POSIX32 += \
	$(MODULEDIR)/juce_audio_processors.posix32.a \
	$(MODULEDIR)/juce_data_structures.posix32.a \
	$(MODULEDIR)/juce_events.posix32.a \
	$(MODULEDIR)/juce_graphics.posix32.a \
	$(MODULEDIR)/juce_gui_basics.posix32.a \
	$(MODULEDIR)/juce_gui_extra.posix32.a
endif

$(BINDIR)/carla-bridge-posix32: $(OBJS_POSIX32) $(LIBS_POSIX32)
	$(CXX) $(OBJS_POSIX32) $(LIBS_START) $(LIBS_POSIX32) $(LIBS_END) $(POSIX_LINK_FLAGS) $(POSIX_32BIT_FLAGS) -o $@

$(OBJDIR)/%.cpp.posix32.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_32BIT_FLAGS) -DBRIDGE_PLUGIN -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# posix64

OBJS_POSIX64  = CarlaBridgePlugin.cpp.posix64.o

# carla-engine
OBJS_POSIX64 += \
	$(OBJDIR)/CarlaEngine.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineClient.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineData.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.posix64.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineThread.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineJack.cpp.posix64.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.posix64.o

# carla-plugin
OBJS_POSIX64 += \
	$(OBJDIR)/CarlaPlugin.cpp.posix64.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.posix64.o \
	$(OBJDIR)/CarlaPluginThread.cpp.posix64.o \
	$(OBJDIR)/LadspaPlugin.cpp.posix64.o \
	$(OBJDIR)/DssiPlugin.cpp.posix64.o \
	$(OBJDIR)/Lv2Plugin.cpp.posix64.o \
	$(OBJDIR)/VstPlugin.cpp.posix64.o \
	$(OBJDIR)/Vst3Plugin.cpp.posix64.o \
	$(OBJDIR)/AuPlugin.cpp.posix64.o \
	$(OBJDIR)/JucePlugin.cpp.posix64.o

# carla-standalone
OBJS_POSIX64 += \
	$(OBJDIR)/CarlaStandalone.cpp.posix64.o

# libs
LIBS_POSIX64 = \
	$(MODULEDIR)/jackbridge.posix64.a \
	$(MODULEDIR)/juce_audio_basics.posix64.a \
	$(MODULEDIR)/juce_core.posix64.a \
	$(MODULEDIR)/lilv.posix64.a \
	$(MODULEDIR)/rtmempool.posix64.a

ifeq ($(MACOS),true)
LIBS_POSIX64 += \
	$(MODULEDIR)/juce_audio_processors.posix64.a \
	$(MODULEDIR)/juce_data_structures.posix64.a \
	$(MODULEDIR)/juce_events.posix64.a \
	$(MODULEDIR)/juce_graphics.posix64.a \
	$(MODULEDIR)/juce_gui_basics.posix64.a \
	$(MODULEDIR)/juce_gui_extra.posix64.a
endif

$(BINDIR)/carla-bridge-posix64: $(OBJS_POSIX64) $(LIBS_POSIX64)
	$(CXX) $(OBJS_POSIX64) $(LIBS_START) $(LIBS_POSIX64) $(LIBS_END) $(POSIX_LINK_FLAGS) $(POSIX_64BIT_FLAGS) -o $@

$(OBJDIR)/%.cpp.posix64.o: %.cpp
	$(CXX) $< $(POSIX_BUILD_FLAGS) $(POSIX_64BIT_FLAGS) -DBRIDGE_PLUGIN -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# win32

OBJS_WIN32  = CarlaBridgePlugin.cpp.win32.o

# carla-engine
OBJS_WIN32 += \
	$(OBJDIR)/CarlaEngine.cpp.win32.o \
	$(OBJDIR)/CarlaEngineClient.cpp.win32.o \
	$(OBJDIR)/CarlaEngineData.cpp.win32.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.win32.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.win32.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.win32.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.win32.o \
	$(OBJDIR)/CarlaEngineThread.cpp.win32.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.win32.o \
	$(OBJDIR)/CarlaEngineJack.cpp.win32.o

# carla-plugin
OBJS_WIN32 += \
	$(OBJDIR)/CarlaPlugin.cpp.win32.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.win32.o \
	$(OBJDIR)/CarlaPluginThread.cpp.win32.o \
	$(OBJDIR)/LadspaPlugin.cpp.win32.o \
	$(OBJDIR)/DssiPlugin.cpp.win32.o \
	$(OBJDIR)/Lv2Plugin.cpp.win32.o \
	$(OBJDIR)/VstPlugin.cpp.win32.o \
	$(OBJDIR)/Vst3Plugin.cpp.win32.o \
	$(OBJDIR)/AuPlugin.cpp.win32.o \
	$(OBJDIR)/JucePlugin.cpp.win32.o

# carla-standalone
OBJS_WIN32 += \
	$(OBJDIR)/CarlaStandalone.cpp.win32.o

# libs
LIBS_WIN32 = \
	$(MODULEDIR)/jackbridge.win32e.a \
	$(MODULEDIR)/lilv.win32.a \
	$(MODULEDIR)/rtmempool.win32.a

LIBS_WIN32 += \
	$(MODULEDIR)/juce_audio_basics.win32.a \
	$(MODULEDIR)/juce_audio_processors.win32.a \
	$(MODULEDIR)/juce_core.win32.a \
	$(MODULEDIR)/juce_data_structures.win32.a \
	$(MODULEDIR)/juce_events.win32.a \
	$(MODULEDIR)/juce_graphics.win32.a \
	$(MODULEDIR)/juce_gui_basics.win32.a

$(BINDIR)/carla-bridge-win32.exe: $(OBJS_WIN32) $(LIBS_WIN32)
	$(CXX) $(OBJS_WIN32) $(LIBS_START) $(LIBS_WIN32) $(LIBS_END) $(WIN_LINK_FLAGS) $(WIN_32BIT_FLAGS) -o $@

$(OBJDIR)/%.cpp.win32.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_32BIT_FLAGS) -DBRIDGE_PLUGIN -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# win64

OBJS_WIN64  = CarlaBridgePlugin.cpp.win64.o

# carla-engine
OBJS_WIN64 += \
	$(OBJDIR)/CarlaEngine.cpp.win64.o \
	$(OBJDIR)/CarlaEngineClient.cpp.win64.o \
	$(OBJDIR)/CarlaEngineData.cpp.win64.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.win64.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.win64.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.win64.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.win64.o \
	$(OBJDIR)/CarlaEngineThread.cpp.win64.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.win64.o \
	$(OBJDIR)/CarlaEngineJack.cpp.win64.o

# carla-plugin
OBJS_WIN64 += \
	$(OBJDIR)/CarlaPlugin.cpp.win64.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.win64.o \
	$(OBJDIR)/CarlaPluginThread.cpp.win64.o \
	$(OBJDIR)/LadspaPlugin.cpp.win64.o \
	$(OBJDIR)/DssiPlugin.cpp.win64.o \
	$(OBJDIR)/Lv2Plugin.cpp.win64.o \
	$(OBJDIR)/VstPlugin.cpp.win64.o \
	$(OBJDIR)/Vst3Plugin.cpp.win64.o \
	$(OBJDIR)/AuPlugin.cpp.win64.o \
	$(OBJDIR)/JucePlugin.cpp.win64.o

# carla-standalone
OBJS_WIN64 += \
	$(OBJDIR)/CarlaStandalone.cpp.win64.o

# libs
LIBS_WIN64 = \
	$(MODULEDIR)/jackbridge.win64e.a \
	$(MODULEDIR)/lilv.win64.a \
	$(MODULEDIR)/rtmempool.win64.a

LIBS_WIN64 += \
	$(MODULEDIR)/juce_audio_basics.win64.a \
	$(MODULEDIR)/juce_audio_processors.win64.a \
	$(MODULEDIR)/juce_core.win64.a \
	$(MODULEDIR)/juce_data_structures.win64.a \
	$(MODULEDIR)/juce_events.win64.a \
	$(MODULEDIR)/juce_graphics.win64.a \
	$(MODULEDIR)/juce_gui_basics.win64.a

$(BINDIR)/carla-bridge-win64.exe: $(OBJS_WIN64) $(LIBS_WIN64)
	$(CXX) $(OBJS_WIN64) $(LIBS_START) $(LIBS_WIN64) $(LIBS_END) $(WIN_LINK_FLAGS) $(WIN_64BIT_FLAGS) -o $@

$(OBJDIR)/%.cpp.win64.o: %.cpp
	$(CXX) $< $(WIN_BUILD_FLAGS) $(WIN_64BIT_FLAGS) -DBRIDGE_PLUGIN -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
