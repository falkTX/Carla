#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
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

BASE_FLAGS  = -Wall -Wextra

ifeq ($(DEBUG),true)
BASE_FLAGS += -O0 -g
BASE_FLAGS += -DDEBUG
STRIP       = true # FIXME
else
BASE_FLAGS += -O2 -ffast-math -mtune=generic -msse -mfpmath=sse
BASE_FLAGS += -DNDEBUG
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)

# --------------------------------------------------------------
# Modify to enable/disable specific features

# Support for LADSPA, DSSI, LV2 and VST plugins
CARLA_PLUGIN_SUPPORT = true

# Support for GIG, SF2 and SFZ sample banks (through fluidsynth and linuxsampler)
CARLA_SAMPLERS_SUPPORT = true

# Support for Native Audio (ALSA and/or PulseAudio in Linux)
CARLA_RTAUDIO_SUPPORT = false

# Comment this line to not use vestige header
BUILD_CXX_FLAGS += -DVESTIGE_HEADER

# --------------------------------------------------------------

HAVE_JACK         = $(shell pkg-config --exists jack && echo true)
HAVE_OPENGL       = $(shell pkg-config --exists gl && echo true)
HAVE_QT5          = $(shell pkg-config --atleast-version=5.0 QtCore && echo true)

HAVE_AF_DEPS      = $(shell pkg-config --exists libavcodec libavformat sndfile && echo true)
HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml zlib && echo true)
HAVE_ZYN_UI_DEPS  = $(shell pkg-config --exists ntk ntk_images && echo true)

ifeq ($(HAVE_JACK),true)
HAVE_JACK_LATENCY = $(shell pkg-config --atleast-version=0.121.0 jack && echo true)
HAVE_JACK2        = $(shell pkg-config --atleast-version=1.9.0 jack && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
endif
