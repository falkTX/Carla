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

ifeq ($(LINUX),true)
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
	../../bin/carla-native.lv2/carla-native$(LIB_EXT) \
	../../bin/carla-native-lv2-export$(APP_EXT)
endif

ifeq ($(LINUX),true)
ifeq ($(DEFAULT_QT),4)
ifeq ($(HAVE_X11),true)
TARGETS += \
	../../bin/carla-native-vstfx$(LIB_EXT) \
	../../bin/carla-native-vstsyn$(LIB_EXT)
endif
endif
endif

# --------------------------------------------------------------

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) *.o
	$(RM) carla-native.lv2/*.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

carla-native-base.cpp.o: carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-lv2.cpp.o: carla-native-lv2.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-lv2-export.cpp.o: carla-native-lv2-export.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-vst.cpp.fx.o: carla-native-vst.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

carla-native-vst.cpp.syn.o: carla-native-vst.cpp carla-native-base.cpp ../modules/CarlaNative.h
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_SYNTH -c -o $@

# --------------------------------------------------------------

../../bin/carla-native.lv2/carla-native$(LIB_EXT): carla-native-lv2.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/carla-native-lv2-export$(APP_EXT): carla-native-lv2-export.cpp.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

../../bin/carla-native-vstfx$(LIB_EXT): carla-native-vst.cpp.fx.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

../../bin/carla-native-vstsyn$(LIB_EXT): carla-native-vst.cpp.syn.o $(LIBS)
	$(CXX) $< $(LIBS_START) $(LIBS) $(LIBS_END) $(SHARED) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------
