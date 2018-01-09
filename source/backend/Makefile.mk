#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

ifeq ($(CWD),)
$(error CWD not defined)
endif

include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/backend/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/backend/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I.. -I$(CWD) -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
BUILD_CXX_FLAGS += $(LIBLO_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(NATIVE_PLUGINS_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------
