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
STANDALONE_LIBS += ../modules/juce_audio_formats.a
STANDALONE_LIBS += ../modules/juce_core.a
STANDALONE_LIBS += ../modules/lilv.a
STANDALONE_LIBS += ../modules/native-plugins.a
STANDALONE_LIBS += ../modules/rtmempool.a

ifeq ($(MACOS_OR_WIN32),true)
STANDALONE_LIBS += ../modules/juce_audio_devices.a
STANDALONE_LIBS += ../modules/juce_audio_processors.a
STANDALONE_LIBS += ../modules/juce_data_structures.a
STANDALONE_LIBS += ../modules/juce_events.a
STANDALONE_LIBS += ../modules/juce_graphics.a
STANDALONE_LIBS += ../modules/juce_gui_basics.a
ifeq ($(MACOS),true)
STANDALONE_LIBS += ../modules/juce_gui_extra.a
endif
else
STANDALONE_LIBS += ../modules/rtaudio.a
STANDALONE_LIBS += ../modules/rtmidi.a
endif

UTILS_LIBS = ../modules/juce_core.a

# --------------------------------------------------------------

STANDALONE_FLAGS  = $(JACKBRIDGE_LIBS)
STANDALONE_FLAGS += $(JUCE_CORE_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_FORMATS_LIBS)
STANDALONE_FLAGS += $(LIBLO_LIBS)
STANDALONE_FLAGS += $(LILV_LIBS)
STANDALONE_FLAGS += $(NATIVE_PLUGINS_LIBS)
STANDALONE_FLAGS += $(RTMEMPOOL_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
STANDALONE_FLAGS += $(JUCE_AUDIO_DEVICES_LIBS)
STANDALONE_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
STANDALONE_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
STANDALONE_FLAGS += $(JUCE_EVENTS_LIBS)
STANDALONE_FLAGS += $(JUCE_GRAPHICS_LIBS)
STANDALONE_FLAGS += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(MACOS),true)
STANDALONE_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif
else
STANDALONE_FLAGS += $(RTAUDIO_LIBS)
STANDALONE_FLAGS += $(RTMIDI_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
STANDALONE_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
STANDALONE_FLAGS += $(LINUXSAMPLER_LIBS)
endif

ifeq ($(HAVE_X11),true)
STANDALONE_FLAGS += $(X11_LIBS)
endif

ifeq ($(UNIX),true)
STANDALONE_FLAGS += -lmagic
endif

UTILS_FLAGS = $(JUCE_CORE_LIBS)

# --------------------------------------------------------------

OBJS = \
	CarlaStandalone.cpp.o \
	CarlaUtils.cpp.o

TARGETS = \
	../../bin/libcarla_standalone2$(LIB_EXT) \
	../../bin/libcarla_utils2$(LIB_EXT)

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

CarlaStandalone.cpp.o: $(CARLA_STANDALONE_CPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

CarlaUtils.cpp.o: $(CARLA_UTILS_CPP)
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# --------------------------------------------------------------

../../bin/libcarla_standalone2$(LIB_EXT): CarlaStandalone.cpp.o $(STANDALONE_LIBS)
	$(CXX) $< $(LIBS_START) $(STANDALONE_LIBS) $(LIBS_END) $(LINK_FLAGS) $(STANDALONE_FLAGS) $(SHARED) -o $@

../../bin/libcarla_utils2$(LIB_EXT): CarlaUtils.cpp.o $(UTILS_LIBS)
	$(CXX) $< $(LIBS_START) $(UTILS_LIBS) $(LIBS_END) $(LINK_FLAGS) $(UTILS_FLAGS) $(SHARED) -o $@

# --------------------------------------------------------------
