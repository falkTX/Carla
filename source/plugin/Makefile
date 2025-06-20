#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
BUILD_CXX_FLAGS += -pthread
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
BUILD_CXX_FLAGS += -I$(CWD)/modules/dgl/src/pugl-upstream/include
BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)
BUILD_CXX_FLAGS += $(MAGIC_FLAGS)
BUILD_CXX_FLAGS += $(X11_FLAGS)
BUILD_CXX_FLAGS += $(YSFX_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------
# Libs

LIBS  = $(MODULEDIR)/carla_engine_plugin.a
LIBS += $(MODULEDIR)/carla_plugin.a
ifeq ($(STATIC_PLUGIN_TARGET),true)
LIBS += $(MODULEDIR)/jackbridge.min.a
else
LIBS += $(MODULEDIR)/jackbridge.a
endif

LIBS += $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/rtmempool.a
LIBS += $(MODULEDIR)/water.a

LIBS += $(MODULEDIR)/audio_decoder.a
LIBS += $(MODULEDIR)/native-plugins.a
ifneq ($(STATIC_PLUGIN_TARGET),true)
LIBS += $(MODULEDIR)/sfzero.a
endif
LIBS += $(MODULEDIR)/zita-resampler.a

ifeq ($(HAVE_DGL),true)
ifeq ($(USING_CUSTOM_DPF),true)
LIBS += $(CUSTOM_DPF_PATH)/build/libdgl-opengl.a
else
LIBS += $(MODULEDIR)/dgl.a
endif
endif

ifeq ($(HAVE_HYLIA),true)
LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(HAVE_YSFX),true)
LIBS += $(MODULEDIR)/ysfx.a
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

LINK_FLAGS += $(YSFX_GRAPHICS_LIBS)

ifeq ($(MACOS),true)
SYMBOLS_NATIVE  = -Wl,-exported_symbol,_carla_get_native_rack_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay16_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay32_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay64_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_cv_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_cv8_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_cv32_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_patchbay_obs_plugin
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_create_native_plugin_host_handle
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_host_handle_free
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_native_plugin_engine
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_complete_license_text
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_supported_file_extensions
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_supported_features
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_library_filename
SYMBOLS_NATIVE += -Wl,-exported_symbol,_carla_get_library_folder
SYMBOLS_LV2     = -Wl,-exported_symbol,_lv2_descriptor
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2    += -Wl,-exported_symbol,_lv2ui_descriptor
SYMBOLS_LV2_UI  = -Wl,-exported_symbol,_lv2ui_descriptor
endif
SYMBOLS_VST     = -Wl,-exported_symbol,_VSTPluginMain
else ifeq ($(WASM),true)
SYMBOLS_NATIVE  = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS="[
SYMBOLS_NATIVE += 'carla_get_native_rack_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay16_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay32_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay64_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay_cv_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay_cv8_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay_cv32_plugin',
SYMBOLS_NATIVE += 'carla_get_native_patchbay_obs_plugin',
SYMBOLS_NATIVE += 'carla_create_native_plugin_host_handle',
SYMBOLS_NATIVE += 'carla_host_handle_free',
SYMBOLS_NATIVE += 'carla_get_native_plugin_engine',
SYMBOLS_NATIVE += 'carla_get_complete_license_text',
SYMBOLS_NATIVE += 'carla_get_supported_file_extensions',
SYMBOLS_NATIVE += 'carla_get_supported_features',
SYMBOLS_NATIVE += 'carla_get_library_filename',
SYMBOLS_NATIVE += 'carla_get_library_folder'
SYMBOLS_NATIVE += ]"
ifeq ($(HAVE_PYQT),true)
SYMBOLS_LV2     = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS="['lv2_descriptor','lv2ui_descriptor']"
SYMBOLS_LV2_UI  = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS="['lv2ui_descriptor']"
else
SYMBOLS_LV2     = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS="['lv2_descriptor']"
endif
SYMBOLS_VST     = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS="['VSTPluginMain']"
else ifeq ($(WINDOWS),true)
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
WINE_LINK_FLAGS += -lstdc++ -lopengl32 -lgdi32 -ldwmapi -ldl
WINE_LINK_FLAGS += -Wl,-rpath,/usr/local/lib/carla
WINE_LINK_FLAGS += -Wl,-rpath,/usr/lib/carla

# ---------------------------------------------------------------------------------------------------------------------

TARGETS = \
	$(BINDIR)/libcarla_host-plugin$(LIB_EXT) \
	$(BINDIR)/libcarla_native-plugin$(LIB_EXT)

# sometimes needed for testing:
#	$(BINDIR)/carla-native-plugin$(APP_EXT)

# ---------------------------------------------------------------------------------------------------------------------

ifneq ($(STATIC_PLUGIN_TARGET),true)

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

endif # !STATIC_PLUGIN_TARGET

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

ifeq ($(WINDOWS),true)
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
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-native-plugin$(APP_EXT): $(OBJDIR)/carla-native-plugin.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-native-plugin$(APP_EXT)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/libcarla_native-plugin$(LIB_EXT): $(OBJDIR)/carla-native-plugin.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla-native-plugin$(LIB_EXT)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_NATIVE) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla$(LIB_EXT)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/carla.lv2/carla-ui$(LIB_EXT): $(OBJDIR)/carla-lv2-ui.cpp.o $(LIBS_ui)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla-ui$(LIB_EXT)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS_ui) $(LIBS_END) $(SHARED) $(SYMBOLS_LV2_UI) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-%.lv2/manifest.ttl: $(DATADIR)/lv2-bundles/%/*.ttl
	-@mkdir -p $(shell dirname $@)
	$(SILENT)cp -v $^ $(shell dirname $@)

$(BINDIR)/carla-audiogain.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.audiogain.o $(MODULEDIR)/native-plugins_audiogain.a
	-@mkdir -p $(BINDIR)/carla-audiogain.lv2
	@echo "Linking carla-audiogain.lv2/carla$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_LIBS) $(LINK_FLAGS) -o $@

$(BINDIR)/carla-files.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.files.o $(MODULEDIR)/native-plugins_files.a
	-@mkdir -p $(BINDIR)/carla-files.lv2
	@echo "Linking carla-files.lv2/carla$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_FILES_LIBS) $(LINK_FLAGS) -o $@

$(BINDIR)/carla-miditools.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2-bundles.cpp.miditools.o $(MODULEDIR)/native-plugins_miditools.a
	-@mkdir -p $(BINDIR)/carla-miditools.lv2
	@echo "Linking carla-miditools.lv2/carla$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_LV2) $(BUNDLE_LIBS) $(LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaRack$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRack$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaRackFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRackFX$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay16$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn16.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay16$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay32$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn32.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay32$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay64$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn64.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay64$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbayFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-fx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbayFX$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/CarlaVstShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShell$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShell$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShell$(LIB_EXT)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(SYMBOLS_VST) $(NATIVE_LINK_FLAGS) -o $@

ifeq ($(WINDOWS),true)
$(BINDIR)/CarlaVstShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.synth.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_VST) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll: $(OBJDIR)/carla-vst-export-bridged.cpp.fx.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(SHARED) $(SYMBOLS_VST) $(LINK_FLAGS) -o $@
else
# wine things
$(BINDIR)/CarlaVstShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shell.o $(OBJDIR)/carla-vst-export.cpp.dll.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstShellBridged.dll.so"
	$(SILENT)$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) symbols/carla-vst.def $(WINE_LINK_FLAGS) -o $@

$(BINDIR)/CarlaVstFxShellBridged.dll.so: $(OBJDIR)/carla-vst.cpp.shellfx.o $(OBJDIR)/carla-vst-export.cpp.dll.o
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaVstFxShellBridged.dll.so"
	$(SILENT)$(WINECC) $^ $(LIBS_START) $(LIBS_wine) $(LIBS_END) $(SHARED) symbols/carla-vst.def $(WINE_LINK_FLAGS) -o $@
endif

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-%-plugin.cpp.o: carla-%-plugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-lv2.cpp.o: carla-lv2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/carla-lv2-ui.cpp.o: carla-lv2-ui.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-lv2-bundles.cpp.audiogain.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (audiogain)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=1 -c -o $@

$(OBJDIR)/carla-lv2-bundles.cpp.files.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (files)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=2 -c -o $@

$(OBJDIR)/carla-lv2-bundles.cpp.miditools.o: carla-lv2-bundles.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (miditools)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_BUNDLE_TYPE=3 -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-vst.cpp.rack-fx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (RackFX)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=0 -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst.cpp.rack-syn.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (RackSynth)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=0 -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-fx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbayFX)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn16.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 16ch)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_16CH -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn32.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 32ch)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_32CH -c -o $@

$(OBJDIR)/carla-vst.cpp.patchbay-syn64.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (PatchbaySynth 64ch)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY=1 -DCARLA_PLUGIN_SYNTH=1 -DCARLA_PLUGIN_64CH -c -o $@

$(OBJDIR)/carla-vst.cpp.shell.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst.cpp.shellfx.o: carla-vst.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (VstShell)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_VST_SHELL -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.fx.o: carla-vst-export-bridged.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=0 -c -o $@

$(OBJDIR)/carla-vst-export-bridged.cpp.synth.o: carla-vst-export-bridged.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH=1 -c -o $@

$(OBJDIR)/carla-vst-export.cpp.o: carla-vst-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/carla-vst-export.cpp.dll.o: carla-vst-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Wine)"
	$(SILENT)$(WINECC) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/manifest.ttl: $(OBJDIR)/carla-lv2-export.cpp.o $(BINDIR)/carla.lv2/carla$(LIB_EXT)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla-lv2-export$(APP_EXT)"
	$(SILENT)$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $(BINDIR)/carla-lv2-export$(APP_EXT)

	@echo "Generating LV2 ttl data"
	$(SILENT)cd $(BINDIR) && $(EXE_WRAPPER) ./carla-lv2-export$(APP_EXT)
	$(SILENT)cd $(BINDIR)/carla.lv2 && ln -sf ../*bridge-* ../carla-discovery-* .

$(BINDIR)/carla-lv2-export$(APP_EXT): $(OBJDIR)/carla-lv2-export.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-lv2-export$(APP_EXT)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(NATIVE_LINK_FLAGS) -o $@

$(OBJDIR)/carla-lv2-export.cpp.o: carla-lv2-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

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
