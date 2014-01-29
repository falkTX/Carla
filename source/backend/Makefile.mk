#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I.. -I../../includes -I../../utils -isystem ../../modules
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2
# -DWANT_VST
# ifeq ($(CARLA_VESTIGE_HEADER),true)
# BUILD_CXX_FLAGS += -DVESTIGE_HEADER
# endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

# ifeq ($(HAVE_CSOUND),true)
# BUILD_CXX_FLAGS += -DWANT_CSOUND
# endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += -DWANT_LINUXSAMPLER
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_AF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_AUDIOFILE
endif

ifeq ($(HAVE_MF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_MIDIFILE
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
ifeq ($(HAVE_ZYN_UI_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX_UI
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

CARLA_DEFINES_H = ../../includes/CarlaDefines.h
CARLA_MIDI_H    = ../../includes/CarlaMIDI.h

CARLA_BACKEND_H  = ../CarlaBackend.h $(CARLA_DEFINES_H)
CARLA_HOST_H     = ../CarlaHost.h $(CARLA_BACKEND_H)
CARLA_ENGINE_HPP = ../CarlaEngine.hpp $(CARLA_BACKEND_H)
CARLA_PLUGIN_HPP = ../CarlaPlugin.hpp $(CARLA_BACKEND_H)

CARLA_UTILS_HPP       = ../../utils/CarlaUtils.hpp $(CARLA_DEFINES_H)
CARLA_JUCE_UTILS_HPP  = ../../utils/CarlaJuceUtils.hpp $(CARLA_UTILS_HPP)
CARLA_LIB_UTILS_HPP   = ../../utils/CarlaLibUtils.hpp $(CARLA_UTILS_HPP)
CARLA_OSC_UTILS_HPP   = ../../utils/CarlaOscUtils.hpp $(CARLA_UTILS_HPP)

CARLA_MUTEX_HPP       = ../../utils/CarlaMutex.hpp $(CARLA_UTILS_HPP)
CARLA_RING_BUFFER_HPP = ../../utils/CarlaRingBuffer.hpp $(CARLA_UTILS_HPP)
CARLA_STRING_HPP      = ../../utils/CarlaString.hpp $(CARLA_JUCE_UTILS_HPP)
CARLA_THREAD_HPP      = ../../utils/CarlaThread.hpp $(CARLA_MUTEX_HPP) $(CARLA_STRING_HPP)

LV2_ATOM_QUEUE_HPP    = ../../utils/Lv2AtomQueue.hpp $(CARLA_MUTEX_HPP) $(CARLA_RING_BUFFER_HPP)
LINKED_LIST_HPP       = ../../utils/LinkedList.hpp $(CARLA_UTILS_HPP)
RT_LINKED_LIST_HPP    = ../../utils/RtLinkedList.hpp $(LINKED_LIST_HPP)

CARLA_BACKEND_UTILS_HPP = ../../utils/CarlaBackendUtils.hpp $(CARLA_BACKEND_H) $(CARLA_HOST_H) $(CARLA_STRING_HPP)
CARLA_BRIDGE_UTILS_HPP  = ../../utils/CarlaBridgeUtils.hpp $(CARLA_RING_BUFFER_HPP)
CARLA_ENGINE_UTILS_HPP  = ../../utils/CarlaEngineUtils.hpp $(CARLA_ENGINE_HPP) $(CARLA_UTILS_HPP)
CARLA_LIB_COUNTER_HPP   = ../../utils/CarlaLibCounter.hpp $(CARLA_LIB_UTILS_HPP) $(CARLA_MUTEX_HPP) $(LINKED_LIST_HPP)
CARLA_MATH_UTILS_HPP    = ../../utils/CarlaMathUtils.hpp $(CARLA_UTILS_HPP)
CARLA_PIPE_UTILS_HPP    = ../../utils/CarlaPipeUtils.hpp $(CARLA_STRING_HPP)
CARLA_SHM_UTILS_HPP     = ../../utils/CarlaShmUtils.hpp $(CARLA_UTILS_HPP)
CARLA_STATE_UTILS_HPP   = ../../utils/CarlaStateUtils.hpp $(LINKED_LIST_HPP)
CARLA_STATE_UTILS_CPP   = ../../utils/CarlaStateUtils.cpp $(CARLA_STATE_UTILS_HPP) $(CARLA_BACKEND_UTILS_HPP) $(CARLA_MATH_UTILS_HPP) $(CARLA_MIDI_H)

CARLA_LADSPA_UTILS_HPP  = ../../utils/CarlaLadspaUtils.hpp $(CARLA_UTILS_HPP)
CARLA_DSSI_UTILS_HPP    = ../../utils/CarlaDssiUtils.hpp $(CARLA_LADSPA_UTILS_HPP)
CARLA_LV2_UTILS_HPP     = ../../utils/CarlaLv2Utils.hpp $(CARLA_UTILS_HPP)
CARLA_VST_UTILS_HPP     = ../../utils/CarlaVstUtils.hpp $(CARLA_UTILS_HPP)

CARLA_NATIVE_H   = ../../modules/CarlaNative.h
CARLA_NATIVE_HPP = ../../modules/CarlaNative.hpp $(CARLA_NATIVE_H) $(CARLA_MIDI_H) $(CARLA_JUCE_UTILS_HPP)
JACK_BRIDGE_HPP  = ../../modules/jackbridge/JackBridge.hpp $(CARLA_DEFINES_H)
RTAUDIO_HPP      = ../../modules/rtaudio/RtAudio.h
RTMIDI_HPP       = ../../modules/rtmidi/RtMidi.h $(CARLA_DEFINES_H)

# ----------------------------------------------------------------------------------------------------------------------------
