#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I.. -I../../includes -I../../utils -isystem ../../modules
BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo)

BUILD_CXX_FLAGS += $(QTCORE_FLAGS)
BUILD_CXX_FLAGS += $(QTXML_FLAGS)

ifeq ($(HAVE_FFMPEG),true)
BUILD_CXX_FLAGS += -DHAVE_FFMPEG
endif

ifeq ($(HAVE_JUCE),true)
BUILD_CXX_FLAGS += -DHAVE_JUCE
endif

ifeq ($(HAVE_OPENGL),true)
BUILD_CXX_FLAGS += -DHAVE_OPENGL
endif

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA
# -DWANT_DSSI -DWANT_LV2 -DWANT_VST
# ifeq ($(CARLA_VESTIGE_HEADER),true)
# BUILD_CXX_FLAGS += -DVESTIGE_HEADER
# endif
endif

# --------------------------------------------------------------

# ifeq ($(HAVE_CSOUND),true)
# BUILD_CXX_FLAGS += -DWANT_CSOUND
# endif
#
# ifeq ($(HAVE_FLUIDSYNTH),true)
# BUILD_CXX_FLAGS += -DWANT_FLUIDSYNTH
# endif
#
# ifeq ($(HAVE_LINUXSAMPLER),true)
# BUILD_CXX_FLAGS += -DWANT_LINUXSAMPLER
# endif

# --------------------------------------------------------------

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

# --------------------------------------------------------------
