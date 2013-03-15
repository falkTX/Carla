#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BACKEND_FLAGS    = -fvisibility=hidden -fPIC
BACKEND_FLAGS   += -I. -I.. -I../../includes -I../../libs -I../../utils

BUILD_C_FLAGS   += $(BACKEND_FLAGS)
BUILD_CXX_FLAGS += $(BACKEND_FLAGS)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_C_FLAGS   += -DWANT_LV2
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_RTAUDIO
endif

# --------------------------------------------------------------

ifeq ($(HAVE_JACK),true)
BUILD_CXX_FLAGS += -DWANT_JACK
endif

ifeq ($(HAVE_JACK_LATENCY),true)
BUILD_CXX_FLAGS += -DWANT_JACK_LATENCY
endif

ifeq ($(HAVE_JACK2),true)
BUILD_CXX_FLAGS += -DWANT_JACK_PORT_RENAME
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += -DWANT_LINUXSAMPLER
endif

ifeq ($(HAVE_OPENGL),true)
BUILD_CXX_FLAGS += -DWANT_OPENGL
endif

ifeq ($(HAVE_AF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_AUDIOFILE
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
endif

ifeq ($(HAVE_ZYN_UI_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX_UI
endif
