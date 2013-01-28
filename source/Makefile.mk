#!/usr/bin/make -f
# Makefile for Carla C++ code #
# -------------------------------------------- #
# Created by falkTX
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++
MOC ?= $(shell pkg-config --variable=moc_location QtCore)
RCC ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC ?= $(shell pkg-config --variable=uic_location QtCore)
STRIP ?= strip
WINDRES ?= windres

# --------------------------------------------------------------

DEBUG ?= false

ifeq ($(DEBUG),true)
BASE_FLAGS  = -O0 -g
BASE_FLAGS += -DDEBUG
STRIP       = true # FIXME
else
BASE_FLAGS  = -O2 -ffast-math -mtune=generic -msse -mfpmath=sse
BASE_FLAGS += -DNDEBUG
endif

BASE_FLAGS += -ansi -pedantic -pedantic-errors -Wall -Wextra -Wformat=2 -Wunused-parameter -Wuninitialized
BASE_FLAGS += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return
BASE_FLAGS += -fipa-pure-const -Wsuggest-attribute=noreturn #pure,const,noreturn
BASE_FLAGS += -Wno-vla -isystem /usr/include/qt4/
32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 -Wc++-compat -Wunsuffixed-float-constants -Wwrite-strings $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)

BUILD_CXX_FLAGS += -Wzero-as-null-pointer-constant

ifneq ($(DEBUG),true)
BUILD_CXX_FLAGS += -DQT_NO_DEBUG -DQT_NO_DEBUG_STREAM -DQT_NO_DEBUG_OUTPUT
endif

# --------------------------------------------------------------
# Modify to enable/disable specific features

# Support for LADSPA, DSSI, LV2 and VST plugins
CARLA_PLUGIN_SUPPORT = true

# Support for GIG, SF2 and SFZ sample banks (through fluidsynth and linuxsampler)
CARLA_SAMPLERS_SUPPORT = true

# Support for Native Audio (ALSA and/or PulseAudio in Linux)
CARLA_RTAUDIO_SUPPORT = true

# Comment this line to not use vestige header
BUILD_CXX_FLAGS += -DVESTIGE_HEADER

# --------------------------------------------------------------

HAVE_JACK         = $(shell pkg-config --exists jack && echo true)
HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml && echo true)

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
HAVE_SUIL         = $(shell pkg-config --exists suil-0 && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
endif
