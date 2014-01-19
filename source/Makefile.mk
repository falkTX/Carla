#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Modify to enable/disable specific features

# Support for LADSPA, DSSI, LV2, VST and AU plugins
CARLA_PLUGIN_SUPPORT = true

# Support for csound files (version 6)
CARLA_CSOUND_SUPPORT = false # not ready yet

# Support for GIG, SF2 and SFZ sample banks (through fluidsynth and linuxsampler)
CARLA_SAMPLERS_SUPPORT = true

# Use the free vestige header instead of the official VST SDK
CARLA_VESTIGE_HEADER = true

# --------------------------------------------------------------
# DO NOT MODIFY PAST THIS POINT!

AR  ?= ar
RM  ?= rm -f
CC  ?= gcc
CXX ?= g++

# --------------------------------------------------------------
# Fallback to Linux if no other OS defined

ifneq ($(HAIKU),true)
ifneq ($(MACOS),true)
ifneq ($(WIN32),true)
LINUX=true
endif
endif
endif

# --------------------------------------------------------------
# Common build and link flags

BASE_FLAGS = -Wall -Wextra -Wcast-qual -Wconversion -pipe -DREAL_BUILD
BASE_OPTS  = -O3 -ffast-math -mtune=generic -msse -msse2 -mfpmath=sse -fdata-sections -ffunction-sections
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,--gc-sections

# -Waggregate-return -Wsign-conversion

ifneq ($(MACOS),true)
BASE_FLAGS += -Wlogical-op -Werror
endif

ifneq ($(WIN32),true)
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(RASPPI),true)
# Raspberry-Pi optimization flags
BASE_OPTS  = -O3 -ffast-math -march=armv6 -mfpu=vfp -mfloat-abi=hard
LINK_OPTS  =
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
LINK_OPTS   =
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
LINK_OPTS  += -Wl,-O1 -Wl,--strip-all
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) -Wl,--no-undefined $(LDFLAGS)

ifeq ($(MACOS),true)
# No C++11 support; force 32bit per default
BUILD_C_FLAGS  += $(32BIT_FLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(32BIT_FLAGS) $(CXXFLAGS)
LINK_FLAGS      = $(32BIT_FLAGS) $(LDFLAGS)
endif

# --------------------------------------------------------------
# Check for required libs

ifneq ($(shell pkg-config --exists liblo && echo true),true)
$(error liblo missing, cannot continue)
endif

ifneq ($(shell pkg-config --exists QtCore && echo true),true)
$(error QtCore missing, cannot continue)
endif

ifneq ($(shell pkg-config --exists QtXml && echo true),true)
$(error QtXml missing, cannot continue)
endif

# --------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

HAVE_FFMPEG       = $(shell pkg-config --exists libavcodec libavformat libavutil && pkg-config --max-version=1.9 libavcodec && echo true)
HAVE_QT4          = $(shell pkg-config --exists QtCore QtGui && echo true)
HAVE_QT5          = $(shell pkg-config --exists Qt5Core Qt5Gui Qt5Widgets && echo true)

ifeq ($(LINUX),true)
HAVE_ALSA         = $(shell pkg-config --exists alsa && echo true)
HAVE_GTK2         = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3         = $(shell pkg-config --exists gtk+-3.0 && echo true)
HAVE_OPENGL       = $(shell pkg-config --exists gl && echo true)
HAVE_PULSEAUDIO   = $(shell pkg-config --exists libpulse-simple && echo true)
else
HAVE_OPENGL       = true
endif

ifeq ($(CARLA_CSOUND_SUPPORT),true)
# FIXME ?
HAVE_CSOUND       = $(shell pkg-config --exists sndfile && echo true)
endif

ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)
endif

# --------------------------------------------------------------
# Check for optional libs (needed by internal plugins)

HAVE_AF_DEPS      = $(shell pkg-config --exists sndfile && echo true)
HAVE_MF_DEPS      = $(shell pkg-config --exists smf && echo true)
HAVE_ZYN_DEPS     = $(shell pkg-config --exists fftw3 mxml zlib && echo true)
HAVE_ZYN_UI_DEPS  = $(shell pkg-config --exists ntk_images ntk && echo true)

# --------------------------------------------------------------
# Check for juce support

ifeq ($(HAIKU),true)
HAVE_JUCE = false
endif

ifeq ($(LINUX),true)
HAVE_JUCE = "false"
# $(shell pkg-config --exists x11 xinerama xext xcursor freetype2 && echo true)
endif

ifeq ($(MACOS),true)
HAVE_JUCE = true
endif

ifeq ($(WIN32),true)
HAVE_JUCE = true
endif

# --------------------------------------------------------------
# Set base stuff

ifeq ($(HAVE_FFMPEG),true)
BASE_FLAGS += -DHAVE_FFMPEG
endif

ifeq ($(HAVE_JUCE),true)
BASE_FLAGS += -DHAVE_JUCE
endif

ifeq ($(HAVE_OPENGL),true)
BASE_FLAGS += -DHAVE_OPENGL
endif

# --------------------------------------------------------------
# Set libs stuff (part 1)

LIBLO_FLAGS = $(shell pkg-config --cflags liblo)
LIBLO_LIBS  = $(shell pkg-config --libs liblo)

QTCORE_FLAGS = $(shell pkg-config --cflags QtCore)
QTCORE_LIBS  = $(shell pkg-config --libs QtCore)
QTXML_FLAGS  = $(shell pkg-config --cflags QtXml)
QTXML_LIBS   = $(shell pkg-config --libs QtXml)

ifeq ($(HAVE_CSOUND),true)
CSOUND_FLAGS = $(shell pkg-config --cflags sndfile) -DUSE_DOUBLE=1
CSOUND_LIBS  = $(shell pkg-config --libs sndfile) -lcsound64
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell pkg-config --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINUXSAMPLER_FLAGS = $(shell pkg-config --cflags linuxsampler) -Wno-unused-parameter
LINUXSAMPLER_LIBS  = $(shell pkg-config --libs linuxsampler)
endif

RTMEMPOOL_LIBS = -lpthread

# --------------------------------------------------------------
# Set libs stuff (part 2)

RTAUDIO_FLAGS  = -DHAVE_GETTIMEOFDAY -D__UNIX_JACK__

ifeq ($(DEBUG),true)
RTAUDIO_FLAGS += -D__RTAUDIO_DEBUG__
RTMIDI_FLAGS  += -D__RTMIDI_DEBUG__
endif

ifeq ($(HAIKU),true)
endif

ifeq ($(LINUX),true)
ifeq ($(HAVE_OPENGL),true)
DGL_FLAGS                = $(shell pkg-config --cflags gl x11)
DGL_LIBS                 = $(shell pkg-config --libs gl x11)
endif
JACKBRIDGE_LIBS          = -ldl -lpthread -lrt
ifeq ($(HAVE_JUCE),true)
JUCE_CORE_LIBS           = -ldl -lpthread -lrt
JUCE_EVENTS_FLAGS        = $(shell pkg-config --cflags x11)
JUCE_EVENTS_LIBS         = $(shell pkg-config --libs x11)
JUCE_GRAPHICS_FLAGS      = $(shell pkg-config --cflags x11 xinerama xext freetype2)
JUCE_GRAPHICS_LIBS       = $(shell pkg-config --libs x11 xinerama xext freetype2)
JUCE_GUI_BASICS_FLAGS    = $(shell pkg-config --cflags x11 xinerama xext xcursor)
JUCE_GUI_BASICS_LIBS     = $(shell pkg-config --libs x11 xinerama xext xcursor) -ldl
endif
LILV_LIBS                = -ldl -lm -lrt
ifeq ($(HAVE_ALSA),true)
RTAUDIO_FLAGS           += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__
RTAUDIO_LIBS            += $(shell pkg-config --libs alsa) -lpthread
RTMIDI_FLAGS            += $(shell pkg-config --cflags alsa) -D__LINUX_ALSASEQ__
RTMIDI_LIBS             += $(shell pkg-config --libs alsa)
endif
ifeq ($(HAVE_PULSEAUDIO),true)
RTAUDIO_FLAGS           += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
RTAUDIO_LIBS            += $(shell pkg-config --libs libpulse-simple)
endif
endif

ifeq ($(MACOS),true)
DGL_LIBS                = -framework OpenGL -framework Cocoa
JACKBRIDGE_LIBS         = -ldl -lpthread
JUCE_AUDIO_BASICS_LIBS  = -framework Accelerate
JUCE_AUDIO_DEVICES_LIBS = -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework DiscRecording
JUCE_AUDIO_FORMATS_LIBS = -framework CoreAudio -framework CoreMIDI -framework QuartzCore -framework AudioToolbox
JUCE_CORE_LIBS          = -framework Cocoa -framework IOKit
JUCE_GRAPHICS_LIBS      = -framework Cocoa -framework QuartzCore
JUCE_GUI_BASICS_LIBS    = -framework Cocoa -framework Carbon -framework QuartzCore
LILV_LIBS               = -ldl -lm
RTAUDIO_FLAGS          += -D__MACOSX_CORE__
RTAUDIO_LIBS           += -lpthread
RTMIDI_FLAGS           += -D__MACOSX_CORE__
endif

ifeq ($(WIN32),true)
DGL_LIBS                = -lopengl32 -lgdi32
JACKBRIDGE_LIBS         = -lpthread
JUCE_AUDIO_DEVICES_LIBS = -lwinmm -lole32
JUCE_CORE_LIBS          = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
JUCE_EVENTS_LIBS        = -lole32
JUCE_GRAPHICS_LIBS      = -lgdi32
JUCE_GUI_BASICS_LIBS    = -lgdi32 -limm32 -lcomdlg32 -lole32
LILV_LIBS               = -lm
RTAUDIO_FLAGS          += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__
RTAUDIO_LIBS           += -ldsound -lpthread
RTMIDI_FLAGS           += -D__WINDOWS_MM__
endif

# --------------------------------------------------------------
# Set Qt tools

ifeq ($(HAVE_QT4),true)
MOC_QT4 ?= $(shell pkg-config --variable=moc_location QtCore)
RCC_QT4 ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC_QT4 ?= $(shell pkg-config --variable=uic_location QtCore)
endif

ifeq ($(HAVE_QT5),true)
MOC_QT5 ?= $(shell pkg-config --variable=libdir Qt5Core)/qt5/bin/moc
RCC_QT5 ?= $(shell pkg-config --variable=libdir Qt5Core)/qt5/bin/rcc
UIC_QT5 ?= $(shell pkg-config --variable=libdir Qt5Core)/qt5/bin/uic
endif

# --------------------------------------------------------------
