#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Modify to enable/disable specific features

# Support for LADSPA, DSSI, LV2 and VST plugins
CARLA_PLUGIN_SUPPORT = true

# Support for csound files (version 6)
CARLA_CSOUND_SUPPORT = true

# Support for GIG, SF2 and SFZ sample banks (through fluidsynth and linuxsampler)
CARLA_SAMPLERS_SUPPORT = true

# Support for Native Audio (ALSA and/or PulseAudio in Linux)
CARLA_RTAUDIO_SUPPORT = true

# Use the free vestige header instead of the official VST SDK
CARLA_VESTIGE_HEADER = true

# --------------------------------------------------------------
# DO NOT MODIFY PAST THIS POINT!

AR  ?= ar
CC  ?= gcc
CXX ?= g++

# --------------------------------------------------------------

BASE_FLAGS = -Wall -Wextra -fPIC -DPIC -pipe
BASE_OPTS  = -O3 -ffast-math -mtune=generic -msse -msse2 -mfpmath=sse -fdata-sections -ffunction-sections
LINK_OPTS  = -Wl,--gc-sections

ifeq ($(RASPPI),true)
# Raspberry-Pi optimization flags
BASE_OPTS  = -O3 -ffast-math -march=armv6 -mfpu=vfp -mfloat-abi=hard
LINK_OPTS  =
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
LINK_OPTS  += -Wl,--strip-all
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)

ifeq ($(MACOS),true)
# No C++11 support; force 32bit per default
BUILD_C_FLAGS   = $(BASE_FLAGS) $(32BIT_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(32BIT_FLAGS) $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)
endif

# --------------------------------------------------------------

HAVE_FFMPEG       = $(shell pkg-config --exists libavcodec libavformat libavutil && pkg-config --max-version=1.9 libavcodec && echo true)
HAVE_OPENGL       = $(shell pkg-config --exists gl && echo true)
HAVE_GTK2         = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3         = $(shell pkg-config --exists gtk+-3.0 && echo true)
HAVE_QT4          = $(shell pkg-config --exists QtCore && echo true)
HAVE_QT5          = $(shell pkg-config --exists Qt5Core && echo true)

HAVE_AF_DEPS      = $(shell pkg-config --exists sndfile && echo true)
HAVE_MF_DEPS      = $(shell pkg-config --exists smf && echo true)
HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml zlib && echo true)
HAVE_ZYN_UI_DEPS  = $(shell pkg-config --exists ntk ntk_images && echo true)

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
endif

# --------------------------------------------------------------

# TODO - move this to theme
ifeq ($(HAVE_QT4),true)
MOC ?= $(shell pkg-config --variable=moc_location QtCore)
RCC ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC ?= $(shell pkg-config --variable=uic_location QtCore)
else
MOC ?= moc
RCC ?= rcc
UIC ?= uic
endif

# --------------------------------------------------------------
