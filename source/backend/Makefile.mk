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

BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------
