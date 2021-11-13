#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# Workaround GCC bug
ifeq ($(TESTBUILD),true)
ifeq ($(USING_JUCE),true)
BUILD_CXX_FLAGS += -Wno-undef
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin
DATADIR   := $(CWD)/../data

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/plugin/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/plugin/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

ifeq ($(NEEDS_WINE),true)
EXE_WRAPPER ?= wine
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
LIBS += $(MODULEDIR)/zita-resampler.a

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
LIBS += $(MODULEDIR)/juce_graphics.a
LIBS += $(MODULEDIR)/juce_gui_basics.a
ifeq ($(USING_JUCE_GUI_EXTRA),true)
LIBS += $(MODULEDIR)/juce_gui_extra.a
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Link flags

NATIVE_LINK_FLAGS  = $(HYLIA_LIBS)
NATIVE_LINK_FLAGS += $(LINK_FLAGS)
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
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(USING_JUCE_GUI_EXTRA),true)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

ifeq ($(MACOS),true)
SYMBOLS_NATIVE  = -Wl,-exported_symbol,_carla_get_native_rack_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay16_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay32_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay64_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_cv_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_create_native_plugin_host_handle
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_host_handle_free
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_plugin_engine
SYMBOLS_LV2     = -Wl,-exported_symbol,_lv2_descriptor
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2    += -Wl,-exported_symbol,_lv2ui_descriptor
SYMBOLS_LV2_UI  = -Wl,-exported_symbol,_lv2ui_descriptor
endif
SYMBOLS_VST     = -Wl,-exported_symbol,_VSTPluginMain
else ifeq ($(WIN32),true)
SYMBOLS_NATIVE  = symbols/carla-native-plugin.def
SYMBOLS_VST     = symbols/carla-vst.def
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2     = symbols/carla-lv2_dsp+ui.def
SYMBOLS_LV2_UI  = symbols/carla-lv2_ui.def
else
SYMBOLS_LV2     = symbols/carla-lv2_dsp.def
endif
else
SYMBOLS_NATIVE = -Wl,--version-script=symbols/carla-native-plugin.version
SYMBOLS_VST    = -Wl,--version-script=symbols/carla-vst.version
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2     = -Wl,--version-script=symbols/carla-lv2_dsp+ui.version
SYMBOLS_LV2_UI  = -Wl,--version-script=symbols/carla-lv2_ui.version
else
SYMBOLS_LV2     = -Wl,--version-script=symbols/carla-lv2_dsp.version
endif
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
	$(BINDIR)/libcarla_host-plugin$(LIB_EXT) \
	$(BINDIR)/libcarla_native-plugin$(LIB_EXT)

# sometimes needed for testing:
#	$(BINDIR)/carla-native-plugin$(APP_EXT)

TARGETS += \
	$(BINDIR)/carla.lv2/carla$(LIB_EXT)

ifeq ($(HAVE_PYQT),true)
TARGETS += \
	$(BINDIR)/carla.lv2/carla-ui$(LIB_EXT)
endif

ifeq ($(CAN_GENERATE_LV2_TTL),true)
TARGETS += \
	$(BINDIR)/carla.lv2/manifest.ttl
endif

ifneq ($(HAIKU),true)
ifeq ($(HAVE_DGL),true)
ifeq ($(HAVE_PYQT),true)
TARGETS += \
	$(BINDIR)/CarlaRack$(LIB_EXT) \
	$(BINDIR)/CarlaRackFX$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay16$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay32$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay64$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbayFX$(LIB_EXT) \
	$(BINDIR)/CarlaVstShell$(LIB_EXT) \
	$(BINDIR)/CarlaVstFxShell$(LIB_EXT)
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

ifeq ($(WIN32),true)
wine: $(BINDIR)/CarlaVstShellBridged.dll $(BINDIR)/CarlaVstFxShellBridged.dll
else
wine: $(BINDIR)/CarlaVstShellBridged.dll.so $(BINDIR)/CarlaVstFxShellBridged.dll.so
endif

static: $(OBJDIR)/carla-host-plugin.cpp.o

# ---------------------------------------------------------------------------------------------------------------------

BUNDLES = audiogain files miditools

bundles: \
	$(BINDIR)/carla-audiogain.lv2/manifest.ttl \
	$(BINDIR)/carla-audiogain.lv2/carla$(LIB_EXT) \
	$(BINDIR)/carla-files.lv2/manifest.ttl \
	$(BINDIR)/carla-files.lv2/carla$(LIB_EXT) \
	$(BINDIR)/carla-miditools.lv2/manifest.ttl \
	$(BINDIR)/carla-miditools.lv2/carla$(LIB_EXT)

BUNDLE_LIBS += $(MODULEDIR)/water.files.a
BUNDLE_LIBS += $(WATER_LIBS)

BUNDLE_FILES_LIBS += $(MODULEDIR)/audio_decoder.a
BUNDLE_FILES_LIBS += $(MODULEDIR)/water.a
BUNDLE_FILES_LIBS += $(MODULEDIR)/zita-resampler.a
BUNDLE_FILES_LIBS += $(AUDIO_DECODER_LIBS)
BUNDLE_FILES_LIBS += $(WATER_LIBS)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)
	rm -f $(BINDIR)/carla.lv2/*.*
	rm -f $(BINDIR)/carla-lv2-export$(APP_EXT)
	rm -rf $(BINDIR)/carla-*.lv2

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_host-plugin$(LIB_EXT): $(OBJDIR)/carla-host-plugin.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla-host-plugin$(LIB_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-native-plugin$(APP_EXT): $(OBJDIR)/carla-native-plugin.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-native-plugin$(APP_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/libcarla_native-plugin$(LIB_EXT): $(OBJDIR)/carla-native-plugin.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla-native-plugin$(LIB_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_NATIVE) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla$(LIB_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/carla.lv2/carla-ui$(LIB_EXT): $(OBJDIR)/carla-lv2-ui.cpp.o $(LIBS_ui)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla-ui$(LIB_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS_ui) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2_UI) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-%.lv2/manifest.ttl: $(DATADIR)/lv2-bundles/%/*.ttl
	-@mkdir -p $(shell dirname $@)
	@cp -v $^ $(shell dirname $@)

$(BINDIR)/carla-audiogain.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.audiogain.o $(MODULEDIR)/native-plugins_audiogain.a
	-@mkdir -p $(BINDIR)/carla-audiogain.lv2
	@echo "Linking carla-audiogain.lv2/carla$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_LIBS) $(LINK_FLAGS) -ldl -o $@

$(BINDIR)/carla-files.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.files.o $(MODULEDIR)/native-plugins_files.a
	-@mkdir -p $(BINDIR)/carla-files.lv2
	@echo "Linking carla-files.lv2/carla$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_FILES_LIBS) $(LINK_FLAGS) -o $@

$(BINDIR)/carla-miditools.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.miditools.o $(MODULEDIR)/native-plugins_miditools.a
	-@mkdir -p $(BINDIR)/carla-miditools.lv2
	@echo "Linking carla-miditools.lv2/carla$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_LIBS) $(LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaRack$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRack$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaRackFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRackFX$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay16$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn16.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay16$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay32$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn32.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay32$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay64$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn64.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay64$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbayFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbayFX$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaVstShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShell$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShell$(LIB_EXT)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

ifeq ($(WIN32),true)
$(BINDIR)/CarlaVstShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.synth.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_VST) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.fx.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_VST) $(LINK_FLAGS) -o $@
else
# wine things
$(BINDIR)/CarlaVstShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.dll.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll.so"
	@$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) symbols/carla-vst.def $(WINE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.dll.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll.so"
	@$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) symbols/carla-vst.def $(WINE_LINK_FLAGS) -o $@
endif

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-%-plugin.cpp.o: carla-%-plugin.cpp
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

$(OBJDIR)/carla-lv2-bundles.cpp.audiogain.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (audiogain)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=1 -c -o $@

$(OBJDIR)/carla-lv2-bundles.cpp.files.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (files)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=2 -c -o $@

$(OBJDIR)/carla-lv2-bundles.cpp.miditools.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (miditools)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=3 -c -o $@

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
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_16CH -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn32.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 32ch)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_32CH -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn64.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 64ch)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_64CH -c -o $@

$(OBJDIR)/carla-vst.cpp.shell.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.shellfx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.fx.o: carla-vst-export-bridged.cpp $(LIBS)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.synth.o: carla-vst-export-bridged.cpp $(LIBS)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst-export.cpp.o: carla-vst-export.cpp $(LIBS)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/carla-vst-export.cpp.dll.o: carla-vst-export.cpp $(LIBS)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Wine)"
	@$(WINECC) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/manifest.ttl: $(OBJDIR)/carla-lv2-export.cpp.o $(BINDIR)/carla.lv2/carla$(LIB_EXT)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $(BINDIR)/carla-lv2-export$(APP_EXT)

	@echo "Generating LV2 ttl data"
	@cd $(BINDIR) && $(EXE_WRAPPER) ./carla-lv2-export$(APP_EXT)
	@cd $(BINDIR)/carla.lv2 && ln -sf ../*bridge-* ../carla-discovery-* .

$(BINDIR)/carla-lv2-export$(APP_EXT): $(OBJDIR)/carla-lv2-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(OBJDIR)/carla-lv2-export.cpp.o: carla-lv2-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJDIR)/carla-host-plugin.cpp.d
-include $(OBJDIR)/carla-native-plugin.cpp.d
-include $(OBJDIR)/carla-lv2.cpp.d
-include $(OBJDIR)/carla-lv2-bundles.cpp.audiogain.d
-include $(OBJDIR)/carla-lv2-bundles.cpp.files.d
-include $(OBJDIR)/carla-lv2-bundles.cpp.miditools.d
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
