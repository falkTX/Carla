#!/usr/bin/make -f
# Makefile for carla modules #
# -------------------------- #
# Created by falkTX
#

ifeq ($(CWD),)
$(error CWD not defined)
endif
ifeq ($(MODULENAME),)
$(error MODULENAME not defined)
endif

include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/$(MODULENAME)/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/$(MODULENAME)/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += -I. -I$(CWD)/includes
BUILD_CXX_FLAGS += -I. -I$(CWD)/includes -I$(CWD)/utils
BUILD_CXX_FLAGS += -DJUCE_APP_CONFIG_HEADER='<AppConfig.h>'

# ----------------------------------------------------------------------------------------------------------------------------
