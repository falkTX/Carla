#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

ifeq ($(CWD),)
CWD=../..
endif

include $(CWD)/Makefile.deps
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I.. -I$(CWD)/includes -I$(CWD)/utils -isystem $(CWD)/modules
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -DWANT_NATIVE

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
BUILD_CXX_FLAGS += -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST
ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -DWANT_AU
endif
ifeq ($(MACOS_OR_WIN32),true)
BUILD_CXX_FLAGS += -DWANT_VST3
endif
ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CSOUND),true)
BUILD_CXX_FLAGS += -DWANT_CSOUND
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BUILD_CXX_FLAGS += -DWANT_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BUILD_CXX_FLAGS += -DWANT_LINUXSAMPLER
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------
