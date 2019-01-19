#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/plugin/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/plugin/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

ifeq ($(HAVE_LIBLO),true)
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
endif

ifeq ($(HAVE_X11),true)
BUILD_CXX_FLAGS += $(X11_FLAGS)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Libs

LIBS  = $(MODULEDIR)/carla_engine_plugin.a
LIBS += $(MODULEDIR)/carla_plugin.a
LIBS += $(MODULEDIR)/jackbridge.a

LIBS += $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/rtmempool.a
LIBS += $(MODULEDIR)/water.a

LIBS += $(MODULEDIR)/audio_decoder.a
LIBS += $(MODULEDIR)/native-plugins.a
LIBS += $(MODULEDIR)/sfzero.a

ifeq ($(HAVE_DGL),true)
LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(USING_JUCE),true)
LIBS += $(MODULEDIR)/juce_audio_basics.a
LIBS += $(MODULEDIR)/juce_audio_processors.a
LIBS += $(MODULEDIR)/juce_core.a
LIBS += $(MODULEDIR)/juce_data_structures.a
LIBS += $(MODULEDIR)/juce_events.a
ifeq ($(MACOS_OR_WIN32),true)
LIBS += $(MODULEDIR)/juce_graphics.a
LIBS += $(MODULEDIR)/juce_gui_basics.a
endif
ifeq ($(MACOS),true)
LIBS += $(MODULEDIR)/juce_gui_extra.a
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Link flags

NATIVE_LINK_FLAGS  = $(LINK_FLAGS)
NATIVE_LINK_FLAGS += $(JACKBRIDGE_LIBS)
NATIVE_LINK_FLAGS += $(LILV_LIBS)
NATIVE_LINK_FLAGS += $(RTMEMPOOL_LIBS)
NATIVE_LINK_FLAGS += $(WATER_LIBS)
NATIVE_LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)

NATIVE_LINK_FLAGS += $(FLUIDSYNTH_LIBS)
NATIVE_LINK_FLAGS += $(LIBLO_LIBS)
NATIVE_LINK_FLAGS += $(MAGIC_LIBS)
NATIVE_LINK_FLAGS += $(X11_LIBS)

ifeq ($(USING_JUCE),true)
LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
ifeq ($(MACOS_OR_WIN32),true)
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
endif
ifeq ($(MACOS),true)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

ifeq ($(MACOS),true)
SYMBOLS_NATIVE  = -Wl,-exported_symbol,_carla_get_native_rack_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_library_filename
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_library_folder
SYMBOLS_LV2     = -Wl,-exported_symbol,_lv2_descriptor
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2    += -Wl,-exported_symbol,_lv2ui_descriptor
SYMBOLS_LV2_UI  = -Wl,-exported_symbol,_lv2ui_descriptor
endif
SYMBOLS_VST     = # TODO
endif

# ---------------------------------------------------------------------------------------------------------------------
# Other libs

LIBS_ui = $(MODULEDIR)/water.a

LIBS_wine  = $(MODULEDIR)/dgl.wine.a
LIBS_wine += $(MODULEDIR)/water.files.a

WINE_LINK_FLAGS  = $(LINK_FLAGS)
WINE_LINK_FLAGS += -L$(BINDIR) -lcarla_native-plugin
WINE_LINK_FLAGS += -lstdc++ -lopengl32 -lgdi32 -ldl
WINE_LINK_FLAGS += -Wl,-rpath,/usr/local/lib/carla
WINE_LINK_FLAGS += -Wl,-rpath,/usr/lib/carla

# ---------------------------------------------------------------------------------------------------------------------

TARGETS = \
	$(BINDIR)/carla-native-plugin$(APP_EXT) \
	$(BINDIR)/libcarla_native-plugin$(LIB_EXT)

TARGETS += \
	$(BINDIR)/carla.lv2/carla$(LIB_EXT)

ifeq ($(HAVE_PYQT),true)
TARGETS += \
	$(BINDIR)/carla.lv2/carla-ui$(LIB_EXT)
endif

ifneq ($(CROSS_COMPILING),true)
TARGETS += \
	$(BINDIR)/carla.lv2/manifest.ttl
endif

ifneq ($(HAIKU),true)
ifeq ($(HAVE_PYQT),true)
TARGETS += \
	$(BINDIR)/CarlaRack$(LIB_EXT) \
	$(BINDIR)/CarlaRackFX$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay16$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay32$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbayFX$(LIB_EXT) \
	$(BINDIR)/CarlaVstShell$(LIB_EXT) \
	$(BINDIR)/CarlaVstFxShell$(LIB_EXT)
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

ifeq ($(WIN32),true)
wine: $(BINDIR)/CarlaVstShellBridged.dll $(BINDIR)/CarlaVstFxShellBridged.dll
else
wine: $(BINDIR)/CarlaVstShellBridged.dll.so $(BINDIR)/CarlaVstFxShellBridged.dll.so
endif

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)
	rm -f $(BINDIR)/carla.lv2/*.*
	rm -f $(BINDIR)/carla-lv2-export$(APP_EXT)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-native-plugin$(APP_EXT): $(OBJDIR)/carla-native-plugin.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-native-plugin$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/libcarla_native-plugin$(LIB_EXT): $(OBJDIR)/carla-native-plugin.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla-native-plugin$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_NATIVE) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2.cpp.o
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/carla.lv2/carla-ui$(LIB_EXT): $(OBJDIR)/carla-lv2-ui.cpp.o $(LIBS_ui)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla-ui$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS_ui) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2_UI) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaRack$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRack$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaRackFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRackFX$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay16$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn16.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay16$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay32$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn32.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay32$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbayFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbayFX$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaVstShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShell$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShell$(LIB_EXT)"
	@$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

ifeq ($(WIN32),true)
$(BINDIR)/CarlaVstShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.synth.o carla-vst-export.def
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll"
	@$(CXX) $^ $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.fx.o carla-vst-export.def
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll"
	@$(CXX) $^ $(SHARED) $(LINK_FLAGS) -o $@
else
$(BINDIR)/CarlaVstShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.dll.o carla-vst-export.def
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll.so"
	@$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) $(WINE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.dll.o carla-vst-export.def
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll.so"
	@$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) $(WINE_LINK_FLAGS) -o $@
endif

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-native-plugin.cpp.o: carla-native-plugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-lv2.cpp.o: carla-lv2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/carla-lv2-ui.cpp.o: carla-lv2-ui.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-vst.cpp.rack-fx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (RackFX)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=0 -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst.cpp.rack-syn.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (RackSynth)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=0 -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-fx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbayFX)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn16.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 16ch)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_16CH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn32.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 32ch)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_32CH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.shell.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL=1 -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.shellfx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL=1 -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.fx.o: carla-vst-export-bridged.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.synth.o: carla-vst-export-bridged.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst-export.cpp.o: carla-vst-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/carla-vst-export.cpp.dll.o: carla-vst-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Wine)"
	@$(WINECC) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/manifest.ttl: $(OBJDIR)/carla-lv2-export.cpp.o
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $(BINDIR)/carla-lv2-export$(APP_EXT)

	@echo "Generating LV2 ttl data"
ifeq ($(BUILDING_FOR_WINDOWS),true)
	@cd $(BINDIR) && wine ./carla-lv2-export$(APP_EXT)
else
	@cd $(BINDIR) && ./carla-lv2-export$(APP_EXT)
endif
	@cd $(BINDIR)/carla.lv2 && ln -sf ../*bridge-* ../carla-discovery-* .

$(BINDIR)/carla-lv2-export$(APP_EXT): $(OBJDIR)/carla-lv2-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(OBJDIR)/carla-lv2-export.cpp.o: carla-lv2-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJDIR)/carla-native-plugin.cpp.d
-include $(OBJDIR)/carla-lv2.cpp.d
-include $(OBJDIR)/carla-lv2-export.cpp.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-fx.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-syn.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-syn16.d
-include $(OBJDIR)/carla-vst.cpp.rack-fx.d
-include $(OBJDIR)/carla-vst.cpp.rack-syn.d
-include $(OBJDIR)/carla-vst.cpp.shell.d
-include $(OBJDIR)/carla-vst-export.cpp.d
-include $(OBJDIR)/carla-vst-export.cpp.dll.d
-include $(OBJDIR)/carla-vst-export-bridged.cpp.fx.d
-include $(OBJDIR)/carla-vst-export-bridged.cpp.synth.d

# ---------------------------------------------------------------------------------------------------------------------
