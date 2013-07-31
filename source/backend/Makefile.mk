#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BACKEND_FLAGS    = -I. -I.. -I../../includes -I../../libs -I../../utils
BACKEND_FLAGS   += -pedantic -pedantic-errors -Wunused-parameter -Wuninitialized -Wno-vla
BACKEND_FLAGS   += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return

BUILD_C_FLAGS   += $(BACKEND_FLAGS)
BUILD_CXX_FLAGS += $(BACKEND_FLAGS)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_RTAUDIO
endif

# --------------------------------------------------------------

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
ifeq ($(HAVE_FFMPEG),true)
BUILD_CXX_FLAGS += -DHAVE_FFMPEG
endif
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
