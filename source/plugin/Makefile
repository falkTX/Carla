#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/plugin/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/plugin/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

ifeq ($(HAVE_LIBLO),true)
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += $(LINUXSAMPLER_FLAGS)
endif

ifeq ($(HAVE_X11),true)
BUILD_CXX_FLAGS += $(X11_FLAGS)
endif

ifeq ($(CARLA_ZYN_COMPAT),true)
BUILD_CXX_FLAGS += -DCARLA_ZYN_LV2_EXPORTED
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Libs

LIBS  = $(MODULEDIR)/carla_engine_plugin.a
LIBS += $(MODULEDIR)/carla_plugin.a
LIBS += $(MODULEDIR)/jackbridge.a

LIBS += $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/rtmempool.a
LIBS += $(MODULEDIR)/water.a

LIBS += $(MODULEDIR)/audio_decoder.a
LIBS += $(MODULEDIR)/native-plugins.all.a

ifeq ($(HAVE_DGL),true)
LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
LIBS += $(MODULEDIR)/hylia.a
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Link flags

LINK_FLAGS += $(JACKBRIDGE_LIBS)
LINK_FLAGS += $(LILV_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)
LINK_FLAGS += $(WATER_LIBS)
LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)

LINK_FLAGS += $(FLUIDSYNTH_LIBS)
LINK_FLAGS += $(LIBLO_LIBS)
LINK_FLAGS += $(LINUXSAMPLER_LIBS)
LINK_FLAGS += $(MAGIC_LIBS)
LINK_FLAGS += $(X11_LIBS)

ifeq ($(MACOS),true)
# NOTE: this assumes only LV2 version will be built
SHARED += -Wl,-exported_symbol,_lv2_descriptor
SHARED += -Wl,-exported_symbol,_lv2ui_descriptor
endif

# ----------------------------------------------------------------------------------------------------------------------------

TARGETS = \
	$(BINDIR)/carla.lv2/carla$(LIB_EXT) \
	$(BINDIR)/carla.lv2/manifest.ttl

ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
ifeq ($(HAVE_PYQT),true)
TARGETS += \
	$(BINDIR)/CarlaRack$(LIB_EXT) \
	$(BINDIR)/CarlaRackFX$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbay16$(LIB_EXT) \
	$(BINDIR)/CarlaPatchbayFX$(LIB_EXT)
endif
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)
	rm -f $(BINDIR)/carla.lv2/*.*
	rm -f $(BINDIR)/carla-lv2-export$(APP_EXT)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/carla$(LIB_EXT): $(OBJDIR)/carla-lv2.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla.lv2/carla$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaRack$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-syn.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRack$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaRackFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.rack-fx.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaRackFX$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbay16$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-syn16.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbay16$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/CarlaPatchbayFX$(LIB_EXT): $(OBJDIR)/carla-vst.cpp.patchbay-fx.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking CarlaPatchbayFX$(LIB_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/carla-lv2.cpp.o: carla-lv2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

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

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla.lv2/manifest.ttl: $(OBJDIR)/carla-lv2-export.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)/carla.lv2
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $(BINDIR)/carla-lv2-export$(APP_EXT)

	@echo "Generating LV2 ttl data"
ifeq ($(BUILDING_FOR_WINDOWS),true)
	@cd $(BINDIR) && wine ./carla-lv2-export$(APP_EXT)
else
	@cd $(BINDIR) && ./carla-lv2-export$(APP_EXT)
endif
	@cd $(BINDIR)/carla.lv2 && ln -sf ../*bridge-* ../carla-discovery-* .

$(BINDIR)/carla-lv2-export$(APP_EXT): $(OBJDIR)/carla-lv2-export.cpp.o $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-lv2-export$(APP_EXT)"
	@$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

$(OBJDIR)/carla-lv2-export.cpp.o: carla-lv2-export.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJDIR)/carla-lv2.cpp.d
-include $(OBJDIR)/carla-lv2-export.cpp.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-fx.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-syn.d
-include $(OBJDIR)/carla-vst.cpp.patchbay-syn16.d
-include $(OBJDIR)/carla-vst.cpp.rack-fx.d
-include $(OBJDIR)/carla-vst.cpp.rack-syn.d

# ----------------------------------------------------------------------------------------------------------------------------
