#!/usr/bin/make -f
# Makefile for carla-bridges-plugin #
# --------------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=carla-bridge
include $(CWD)/Makefile.mk

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

BUILD_CXX_FLAGS += -DBUILD_BRIDGE -I. -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/utils -isystem $(CWD)/modules
BUILD_CXX_FLAGS += -I$(CWD)/backend/engine -I$(CWD)/backend/plugin
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif

32BIT_FLAGS += -DBRIDGE_PLUGIN
64BIT_FLAGS += -DBRIDGE_PLUGIN

# ----------------------------------------------------------------------------------------------------------------------------

LIBS_native   = $(MODULEDIR)/jackbridge.a
LIBS_posix32  = $(MODULEDIR)/jackbridge.posix32.a
LIBS_posix64  = $(MODULEDIR)/jackbridge.posix64.a
LIBS_win32    = $(MODULEDIR)/jackbridge.win32e.a
LIBS_win64    = $(MODULEDIR)/jackbridge.win64e.a
LINK_FLAGS   += $(JACKBRIDGE_LIBS)

LIBS_native  += $(MODULEDIR)/juce_audio_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_basics.win64.a
LINK_FLAGS   += $(JUCE_AUDIO_BASICS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_core.a
LIBS_posix32 += $(MODULEDIR)/juce_core.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_core.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_core.win32.a
LIBS_win64   += $(MODULEDIR)/juce_core.win64.a
LINK_FLAGS   += $(JUCE_CORE_LIBS)

LIBS_native  += $(MODULEDIR)/lilv.a
LIBS_posix32 += $(MODULEDIR)/lilv.posix32.a
LIBS_posix64 += $(MODULEDIR)/lilv.posix64.a
LIBS_win32   += $(MODULEDIR)/lilv.win32.a
LIBS_win64   += $(MODULEDIR)/lilv.win64.a
LINK_FLAGS   += $(LILV_LIBS)

LIBS_native  += $(MODULEDIR)/rtmempool.a
LIBS_posix32 += $(MODULEDIR)/rtmempool.posix32.a
LIBS_posix64 += $(MODULEDIR)/rtmempool.posix64.a
LIBS_win32   += $(MODULEDIR)/rtmempool.win32.a
LIBS_win64   += $(MODULEDIR)/rtmempool.win64.a
LINK_FLAGS   += $(RTMEMPOOL_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LIBS_native  += $(MODULEDIR)/juce_audio_processors.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_processors.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_processors.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_processors.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_processors.win64.a
LINK_FLAGS   += $(JUCE_AUDIO_PROCESSORS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_data_structures.a
LIBS_posix32 += $(MODULEDIR)/juce_data_structures.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_data_structures.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_data_structures.win32.a
LIBS_win64   += $(MODULEDIR)/juce_data_structures.win64.a
LINK_FLAGS   += $(JUCE_DATA_STRUCTURES_LIBS)

LIBS_native  += $(MODULEDIR)/juce_events.a
LIBS_posix32 += $(MODULEDIR)/juce_events.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_events.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_events.win32.a
LIBS_win64   += $(MODULEDIR)/juce_events.win64.a
LINK_FLAGS   += $(JUCE_EVENTS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_graphics.a
LIBS_posix32 += $(MODULEDIR)/juce_graphics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_graphics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_graphics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_graphics.win64.a
LINK_FLAGS   += $(JUCE_GRAPHICS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_gui_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_gui_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_gui_basics.win64.a
LINK_FLAGS   += $(JUCE_GUI_BASICS_LIBS)

ifeq ($(MACOS),true)
LIBS_native  += $(MODULEDIR)/juce_gui_extra.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_extra.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_extra.posix64.a
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

LINK_FLAGS += $(LIBLO_LIBS)

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

ifeq ($(UNIX),true)
LINK_FLAGS += -ldl -lpthread
endif

# ----------------------------------------------------------------------------------------------------------------------------

NATIVE_BUILD_FLAGS  = $(NATIVE_PLUGINS_FLAGS)
NATIVE_LINK_FLAGS   =

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_BUILD_FLAGS += $(FLUIDSYNTH_FLAGS)
NATIVE_LINK_FLAGS  += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_BUILD_FLAGS += $(LINUXSAMPLER_FLAGS)
NATIVE_LINK_FLAGS  += $(LINUXSAMPLER_LIBS)
endif

LIBS_native        += $(MODULEDIR)/juce_audio_formats.a
NATIVE_LINK_FLAGS  += $(JUCE_AUDIO_FORMATS_LIBS)

LIBS_native        += $(MODULEDIR)/native-plugins.a
NATIVE_LINK_FLAGS  += $(NATIVE_PLUGINS_LIBS)

ifeq ($(HAVE_DGL),true)
LIBS_native        += $(MODULEDIR)/dgl.a
endif

# ----------------------------------------------------------------------------------------------------------------------------

OBJS_native = \
	$(OBJDIR)/CarlaBridgePlugin.cpp.o \
	$(OBJDIR)/CarlaEngine.cpp.o \
	$(OBJDIR)/CarlaEngineClient.cpp.o \
	$(OBJDIR)/CarlaEngineData.cpp.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.o \
	$(OBJDIR)/CarlaEngineThread.cpp.o \
	$(OBJDIR)/CarlaEngineJack.cpp.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.o \
	$(OBJDIR)/CarlaPlugin.cpp.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.o \
	$(OBJDIR)/CarlaPluginBridge.cpp.o \
	$(OBJDIR)/CarlaPluginNative.cpp.o \
	$(OBJDIR)/CarlaPluginLADSPA.cpp.o \
	$(OBJDIR)/CarlaPluginDSSI.cpp.o \
	$(OBJDIR)/CarlaPluginLV2.cpp.o \
	$(OBJDIR)/CarlaPluginVST2.cpp.o \
	$(OBJDIR)/CarlaPluginVST3.cpp.o \
	$(OBJDIR)/CarlaPluginAU.cpp.o \
	$(OBJDIR)/CarlaPluginJuce.cpp.o \
	$(OBJDIR)/CarlaPluginFluidSynth.cpp.o \
	$(OBJDIR)/CarlaPluginLinuxSampler.cpp.o \
	$(OBJDIR)/CarlaStandalone.cpp.o

OBJS_arch = \
	$(OBJDIR)/CarlaBridgePlugin.cpp.arch.o \
	$(OBJDIR)/CarlaEngine.cpp.arch.o \
	$(OBJDIR)/CarlaEngineClient.cpp.arch.o \
	$(OBJDIR)/CarlaEngineData.cpp.arch.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.arch.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.arch.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.arch.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.arch.o \
	$(OBJDIR)/CarlaEngineThread.cpp.arch.o \
	$(OBJDIR)/CarlaEngineJack.cpp.arch.o \
	$(OBJDIR)/CarlaEngineBridge.cpp.arch.o \
	$(OBJDIR)/CarlaPlugin.cpp.arch.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.arch.o \
	$(OBJDIR)/CarlaPluginLADSPA.cpp.arch.o \
	$(OBJDIR)/CarlaPluginDSSI.cpp.arch.o \
	$(OBJDIR)/CarlaPluginLV2.cpp.arch.o \
	$(OBJDIR)/CarlaPluginVST2.cpp.arch.o \
	$(OBJDIR)/CarlaPluginVST3.cpp.arch.o \
	$(OBJDIR)/CarlaPluginAU.cpp.arch.o \
	$(OBJDIR)/CarlaPluginJuce.cpp.arch.o \
	$(OBJDIR)/CarlaStandalone.cpp.arch.o

OBJS_posix32 = $(OBJS_arch:%.arch.o=%.posix32.o)
OBJS_posix64 = $(OBJS_arch:%.arch.o=%.posix64.o)
OBJS_win32   = $(OBJS_arch:%.arch.o=%.win32.o)
OBJS_win64   = $(OBJS_arch:%.arch.o=%.win64.o)

# ----------------------------------------------------------------------------------------------------------------------------

all: native

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/$(MODULENAME)-native
	rm -f $(BINDIR)/$(MODULENAME)-posix32
	rm -f $(BINDIR)/$(MODULENAME)-posix64
	rm -f $(BINDIR)/$(MODULENAME)-win*.exe

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

native:  $(BINDIR)/$(MODULENAME)-native$(APP_EXT)
posix32: $(BINDIR)/$(MODULENAME)-posix32
posix64: $(BINDIR)/$(MODULENAME)-posix64
win32:   $(BINDIR)/$(MODULENAME)-win32.exe
win64:   $(BINDIR)/$(MODULENAME)-win64.exe

# ----------------------------------------------------------------------------------------------------------------------------
# linking

$(BINDIR)/$(MODULENAME)-native$(APP_EXT): $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native$(APP_EXT)"
	@$(CXX) $(OBJS_native) $(LIBS_START) $(LIBS_native) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-posix32: $(OBJS_posix32) $(LIBS_posix32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix32"
	@$(CXX) $(OBJS_posix32) $(LIBS_START) $(LIBS_posix32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-posix64: $(OBJS_posix64) $(LIBS_posix64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix64"
	@$(CXX) $(OBJS_posix64) $(LIBS_START) $(LIBS_posix64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win32.exe: $(OBJS_win32) $(LIBS_win32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win32.exe"
	@$(CXX) $(OBJS_win32) $(LIBS_START) $(LIBS_win32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win64.exe: $(OBJS_win64) $(LIBS_win64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win64.exe"
	@$(CXX) $(OBJS_win64) $(LIBS_START) $(LIBS_win64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# native

$(OBJDIR)/CarlaBridgePlugin.cpp.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/CarlaEng%.cpp.o: $(CWD)/backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.o: $(CWD)/backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# posix32

$(OBJDIR)/CarlaBridgePlugin.cpp.posix32.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaEng%.cpp.posix32.o: $(CWD)/backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (posix32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.posix32.o: $(CWD)/backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (posix32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.posix32.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (posix32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# posix64

$(OBJDIR)/CarlaBridgePlugin.cpp.posix64.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaEng%.cpp.posix64.o: $(CWD)/backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (posix64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.posix64.o: $(CWD)/backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (posix64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.posix64.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (posix64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# win32

$(OBJDIR)/CarlaBridgePlugin.cpp.win32.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaEng%.cpp.win32.o: $(CWD)/backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (win32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.win32.o: $(CWD)/backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (win32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.win32.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (win32 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------
# win64

$(OBJDIR)/CarlaBridgePlugin.cpp.win64.o: CarlaBridgePlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaEng%.cpp.win64.o: $(CWD)/backend/engine/CarlaEng%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEng$*.cpp (win64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaPlug%.cpp.win64.o: $(CWD)/backend/plugin/CarlaPlug%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaPlug$*.cpp (win64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.win64.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaStandalone.cpp (win64 bridge)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS_native:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
