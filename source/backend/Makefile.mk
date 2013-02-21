#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += -pthread -fvisibility=hidden -fPIC -I. -I.. -I../../includes
BUILD_CXX_FLAGS += -pthread -fvisibility=hidden -fPIC -I. -I.. -I../../includes -I../../libs -I../../utils
LINK_FLAGS      += -lpthread

# --------------------------------------------------------------

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
# BUILD_C_FLAGS   += -DWANT_LV2
# BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_RTAUDIO
endif

# --------------------------------------------------------------

ifeq ($(HAVE_JACK),true)
BUILD_CXX_FLAGS += -DWANT_JACK
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
# BUILD_CXX_FLAGS += -DWANT_LINUXSAMPLER
endif

ifeq ($(HAVE_SUIL),true)
BUILD_CXX_FLAGS += -DWANT_SUIL
endif

ifeq ($(HAVE_AF_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_AUDIOFILE
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BUILD_CXX_FLAGS += -DWANT_ZYNADDSUBFX
endif
