#!/usr/bin/make -f
# Makefile for carla plugin #
# ------------------------- #
# Created by falkTX
#

CWD=..
include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../utils -isystem ../modules
BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

# --------------------------------------------------------------
# Link flags

LINK_FLAGS += $(JACKBRIDGE_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_FORMATS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(LIBLO_LIBS)
LINK_FLAGS += $(LILV_LIBS)
LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(LINUXSAMPLER_LIBS)
endif

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

ifeq ($(UNIX),true)
LINK_FLAGS += -lmagic
endif

# --------------------------------------------------------------
# Libs

LIBS += ../backend/carla_engine_plugin.a
LIBS += ../backend/carla_plugin.a

LIBS += ../modules/native-plugins.a
LIBS += ../modules/jackbridge.a
LIBS += ../modules/juce_audio_basics.a
LIBS += ../modules/juce_audio_formats.a
LIBS += ../modules/juce_core.a
LIBS += ../modules/lilv.a
LIBS += ../modules/rtmempool.a

ifeq ($(MACOS_OR_WIN32),true)
LIBS += ../modules/juce_audio_processors.a
LIBS += ../modules/juce_data_structures.a
LIBS += ../modules/juce_events.a
LIBS += ../modules/juce_graphics.a
LIBS += ../modules/juce_gui_basics.a
LIBS += ../modules/juce_gui_extra.a
endif

# --------------------------------------------------------------

TARGETS =

ifneq ($(WIN32),true)
TARGETS += \
	../../bin/carla.lv2/carla$(LIB_EXT) \
	../../bin/carla-lv2-export$(APP_EXT)
endif

ifeq ($(LINUX),true)
ifeq ($(DEFAULT_QT),4)
ifeq ($(HAVE_X11),true)
TARGETS += \
	../../bin/CarlaRack$(LIB_EXT) \
	../../bin/CarlaRackFX$(LIB_EXT) \
	../../bin/CarlaPatchbay$(LIB_EXT) \
	../../bin/CarlaPatchbayFX$(LIB_EXT)
endif
endif
endif

# --------------------------------------------------------------

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o
	$(RM) carla.lv2/*.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

carla-base.cpp.o: carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-lv2.cpp.o: carla-lv2.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-lv2-export.cpp.o: carla-lv2-export.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-vst.cpp.rack-fx.o: carla-vst.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-vst.cpp.rack-syn.o: carla-vst.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH -c -o $@

carla-vst.cpp.patchbay-fx.o: carla-vst.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY -c -o $@

carla-vst.cpp.patchbay-syn.o: carla-vst.cpp carla-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_PATCHBAY -DCARLA_PLUGIN_SYNTH -c -o $@

# --------------------------------------------------------------

../../bin/carla.lv2/carla$(LIB_EXT): carla-lv2.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/carla-lv2-export$(APP_EXT): carla-lv2-export.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

../../bin/CarlaRack$(LIB_EXT): carla-vst.cpp.rack-syn.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/CarlaRackFX$(LIB_EXT): carla-vst.cpp.rack-fx.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/CarlaPatchbay$(LIB_EXT): carla-vst.cpp.patchbay-syn.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/CarlaPatchbayFX$(LIB_EXT): carla-vst.cpp.patchbay-fx.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------
