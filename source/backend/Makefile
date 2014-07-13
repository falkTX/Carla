#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

CWD=..
include Makefile.mk

# --------------------------------------------------------------

STANDALONE_LIBS  = carla_engine.a
STANDALONE_LIBS += carla_plugin.a

STANDALONE_LIBS += ../modules/jackbridge.a
STANDALONE_LIBS += ../modules/juce_audio_basics.a
STANDALONE_LIBS += ../modules/juce_audio_devices.a
STANDALONE_LIBS += ../modules/juce_audio_formats.a
STANDALONE_LIBS += ../modules/juce_audio_processors.a
STANDALONE_LIBS += ../modules/juce_core.a
STANDALONE_LIBS += ../modules/juce_events.a
STANDALONE_LIBS += ../modules/native-plugins.a
STANDALONE_LIBS += ../modules/rtaudio.a
STANDALONE_LIBS += ../modules/rtmempool.a
STANDALONE_LIBS += ../modules/rtmidi.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
STANDALONE_LIBS += ../modules/lilv.a
endif

ifeq ($(HAVE_AF_DEPS),true)
STANDALONE_LIBS += ../modules/audio_decoder.a
endif

ifeq ($(HAVE_DGL),true)
STANDALONE_LIBS += ../modules/dgl.a
endif

ifeq ($(HAVE_JUCE_UI),true)
STANDALONE_LIBS += ../modules/juce_data_structures.a
STANDALONE_LIBS += ../modules/juce_graphics.a
STANDALONE_LIBS += ../modules/juce_gui_basics.a
STANDALONE_LIBS += ../modules/juce_gui_extra.a
endif

# --------------------------------------------------------------

STANDALONE_FLAGS  = $(LIBLO_LIBS)
STANDALONE_FLAGS += $(QTCORE_LIBS)

STANDALONE_FLAGS += $(JACKBRIDGE_LIBS)
STANDALONE_FLAGS += $(JUCE_CORE_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_DEVICES_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_FORMATS_LIBS)
STANDALONE_FLAGS += $(JUCE_EVENTS_LIBS)
STANDALONE_FLAGS += $(NATIVE_PLUGINS_LIBS)
STANDALONE_FLAGS += $(RTAUDIO_LIBS)
STANDALONE_FLAGS += $(RTMEMPOOL_LIBS)
STANDALONE_FLAGS += $(RTMIDI_LIBS)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
STANDALONE_FLAGS += $(LILV_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
STANDALONE_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
STANDALONE_FLAGS += $(LINUXSAMPLER_LIBS)
endif

ifeq ($(HAVE_JUCE_UI),true)
STANDALONE_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
STANDALONE_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
STANDALONE_FLAGS += $(JUCE_GRAPHICS_LIBS)
STANDALONE_FLAGS += $(JUCE_GUI_BASICS_LIBS)
STANDALONE_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif

# --------------------------------------------------------------

OBJS = \
	CarlaStandalone.cpp.o

TARGETS = \
	../../bin/libcarla_standalone2$(LIB_EXT)

# --------------------------------------------------------------

all: $(TARGETS)

# --------------------------------------------------------------

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGETS)
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin

debug:
	$(MAKE) DEBUG=true

doxygen: CarlaBackend.doxygen
	doxygen $^

# --------------------------------------------------------------

CarlaStandalone.cpp.o: CarlaStandalone.cpp $(CARLA_STANDALONE_CPP_DEPS)
	$(CXX) $< $(BUILD_CXX_FLAGS) $(QTCORE_FLAGS) -c -o $@

# --------------------------------------------------------------

../../bin/libcarla_standalone2$(LIB_EXT): $(OBJS) $(STANDALONE_LIBS)
	$(CXX) $< $(LIBS_START) $(STANDALONE_LIBS) $(LIBS_END) $(LINK_FLAGS) $(STANDALONE_FLAGS) $(SHARED) -o $@

# --------------------------------------------------------------

carla_%.a:
	$(MAKE) -C $* ../carla_$*.a

../modules/%.a:
	$(MAKE) -C ../modules/$* ../$*.a

# --------------------------------------------------------------
