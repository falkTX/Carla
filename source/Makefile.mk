#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Modify to enable/disable specific features

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
# Set MACOS_OR_WIN32

ifeq ($(MACOS),true)
MACOS_OR_WIN32=true
endif

ifeq ($(WIN32),true)
MACOS_OR_WIN32=true
endif

# --------------------------------------------------------------
# Force some features on MacOS and Windows

ifeq ($(MACOS_OR_WIN32),true)
CARLA_VESTIGE_HEADER = false
endif

# --------------------------------------------------------------
# Common build and link flags

BASE_FLAGS = -Wall -Wextra -pipe -DREAL_BUILD
BASE_OPTS  = -O2 -ffast-math -mtune=generic -msse -msse2 -fdata-sections -ffunction-sections
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-O1 -Wl,--as-needed -Wl,--gc-sections
# LINK_OPTS += -Wl,--strip-all

ifneq ($(MACOS),true)
# MacOS doesn't support this
BASE_OPTS += -mfpmath=sse
else
# MacOS linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
endif

ifeq ($(RASPPI),true)
# Raspberry-Pi optimization flags
BASE_OPTS  = -O2 -ffast-math -march=armv6 -mfpu=vfp -mfloat-abi=hard
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifeq ($(PANDORA),true)
# OpenPandora flags
BASE_OPTS  = -O2 -ffast-math -march=armv7-a -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifneq ($(WIN32),true)
# not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
LINK_OPTS   =
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) -Wl,--no-undefined $(LDFLAGS)

ifeq ($(MACOS),true)
# No C++11 support
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)
endif

# --------------------------------------------------------------
# Strict test build

ifeq ($(TESTBUILD),true)
BASE_FLAGS += -Werror -Wcast-qual -Wconversion -Wformat -Wformat-security -Wredundant-decls -Wshadow -Wstrict-overflow -fstrict-overflow -Wundef -Wwrite-strings
ifneq ($(CC),clang)
BASE_FLAGS += -Wcast-align -Wunsafe-loop-optimizations
endif
ifneq ($(MACOS),true)
BASE_FLAGS += -Wmissing-declarations -Wsign-conversion
ifneq ($(CC),clang)
BASE_FLAGS += -Wlogical-op
endif
endif
CFLAGS     += -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes
CXXFLAGS   += -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual
ifeq ($(LINUX),true)
CFLAGS     += -isystem /opt/kxstudio/include
CXXFLAGS   += -isystem /opt/kxstudio/include
endif
ifeq ($(MACOS),true)
CFLAGS     += -isystem /opt/kxstudio/include
CXXFLAGS   += -isystem /opt/kxstudio/include
endif
ifeq ($(WIN32),true)
CFLAGS     += -isystem /opt/mingw32/include
CXXFLAGS   += -isystem /opt/mingw32/include
endif
endif

# --------------------------------------------------------------
# Check for required libs

ifneq ($(shell pkg-config --exists liblo && echo true),true)
$(error liblo missing, cannot continue)
endif

ifeq ($(LINUX),true)
ifeq (,$(wildcard /usr/include/magic.h))
$(error libmagic missing, cannot continue)
endif
endif

# --------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

ifneq ($(MACOS_OR_WIN32),true)
HAVE_FFMPEG     = $(shell pkg-config --exists libavcodec libavformat libavutil && echo true)
HAVE_GTK2       = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3       = $(shell pkg-config --exists gtk+-3.0 && echo true)
ifeq ($(LINUX),true)
HAVE_ALSA       = $(shell pkg-config --exists alsa && echo true)
HAVE_PULSEAUDIO = $(shell pkg-config --exists libpulse-simple && echo true)
HAVE_X11        = $(shell pkg-config --exists x11 && echo true)
endif
endif

HAVE_QT4          = $(shell pkg-config --exists QtCore QtGui && echo true)
HAVE_QT5          = $(shell pkg-config --exists Qt5Core Qt5Gui Qt5Widgets && echo true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --exists linuxsampler && echo true)

# --------------------------------------------------------------
# Set Qt tools

ifeq ($(HAVE_QT4),true)
MOC_QT4 ?= $(shell pkg-config --variable=moc_location QtCore)
RCC_QT4 ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC_QT4 ?= $(shell pkg-config --variable=uic_location QtCore)
ifeq (,$(wildcard $(MOC_QT4)))
HAVE_QT4=false
endif
endif

ifeq ($(HAVE_QT5),true)
QT5_LIBDIR = $(shell pkg-config --variable=libdir Qt5Core)
ifeq ($(MACOS),true)
MOC_QT5 ?= $(QT5_LIBDIR)/../bin/moc
RCC_QT5 ?= $(QT5_LIBDIR)/../bin/rcc
UIC_QT5 ?= $(QT5_LIBDIR)/../bin/uic
else # MACOS
ifneq (,$(wildcard $(QT5_LIBDIR)/qt5/bin/moc))
MOC_QT5 ?= $(QT5_LIBDIR)/qt5/bin/moc
RCC_QT5 ?= $(QT5_LIBDIR)/qt5/bin/rcc
UIC_QT5 ?= $(QT5_LIBDIR)/qt5/bin/uic
else
MOC_QT5 ?= $(QT5_LIBDIR)/qt/bin/moc
RCC_QT5 ?= $(QT5_LIBDIR)/qt/bin/rcc
UIC_QT5 ?= $(QT5_LIBDIR)/qt/bin/uic
endif
endif # MACOS
ifeq (,$(wildcard $(MOC_QT5)))
HAVE_QT5=false
endif
endif

# --------------------------------------------------------------
# Set default Qt used in frontend

ifeq ($(HAVE_QT4),true)
DEFAULT_QT ?= 4
else
DEFAULT_QT ?= 5
endif

# --------------------------------------------------------------
# Check for optional libs (required by internal plugins)

HAVE_ZYN_DEPS    = $(shell pkg-config --exists fftw3 mxml zlib && echo true)
HAVE_ZYN_UI_DEPS = $(shell pkg-config --exists ntk_images ntk && echo true)

# --------------------------------------------------------------
# Set base defines

ifeq ($(HAVE_FLUIDSYNTH),true)
BASE_FLAGS += -DHAVE_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BASE_FLAGS += -DHAVE_LINUXSAMPLER
endif

ifeq ($(HAVE_X11),true)
BASE_FLAGS += -DHAVE_X11
endif

ifeq ($(CARLA_VESTIGE_HEADER),true)
BASE_FLAGS += -DVESTIGE_HEADER
endif

# --------------------------------------------------------------
# Set libs stuff (part 1)

LIBLO_FLAGS = $(shell pkg-config --cflags liblo)
LIBLO_LIBS  = $(shell pkg-config --libs liblo)

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell pkg-config --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINUXSAMPLER_FLAGS = $(shell pkg-config --cflags linuxsampler) -Wno-unused-parameter
LINUXSAMPLER_LIBS  = $(shell pkg-config --libs linuxsampler)
endif

ifeq ($(HAVE_X11),true)
X11_FLAGS = $(shell pkg-config --cflags x11)
X11_LIBS  = $(shell pkg-config --libs x11)
endif

# --------------------------------------------------------------
# Set libs stuff (part 2)

RTAUDIO_FLAGS  = -DHAVE_GETTIMEOFDAY -D__UNIX_JACK__

ifeq ($(DEBUG),true)
RTAUDIO_FLAGS += -D__RTAUDIO_DEBUG__
RTMIDI_FLAGS  += -D__RTMIDI_DEBUG__
endif

ifneq ($(HAIKU),true)
RTMEMPOOL_LIBS = -lpthread
endif

ifeq ($(LINUX),true)
JACKBRIDGE_LIBS       = -ldl -lpthread -lrt
JUCE_CORE_LIBS        = -ldl -lpthread -lrt
LILV_LIBS             = -ldl -lm -lrt
ifeq ($(HAVE_ALSA),true)
RTAUDIO_FLAGS        += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__
RTAUDIO_LIBS         += $(shell pkg-config --libs alsa) -lpthread
RTMIDI_FLAGS         += $(shell pkg-config --cflags alsa) -D__LINUX_ALSA__
RTMIDI_LIBS          += $(shell pkg-config --libs alsa)
endif
ifeq ($(HAVE_PULSEAUDIO),true)
RTAUDIO_FLAGS        += $(shell pkg-config --cflags libpulse-simple) -D__LINUX_PULSE__
RTAUDIO_LIBS         += $(shell pkg-config --libs libpulse-simple)
endif
endif

ifeq ($(MACOS),true)
JACKBRIDGE_LIBS            = -ldl -lpthread
JUCE_AUDIO_BASICS_LIBS     = -framework Accelerate
JUCE_AUDIO_DEVICES_LIBS    = -framework AppKit -framework AudioToolbox -framework CoreAudio -framework CoreMIDI
JUCE_AUDIO_FORMATS_LIBS    = -framework AudioToolbox -framework CoreFoundation
JUCE_AUDIO_PROCESSORS_LIBS = -framework AudioToolbox -framework AudioUnit -framework CoreAudio -framework CoreAudioKit -framework Cocoa -framework Carbon
JUCE_CORE_LIBS             = -framework AppKit
JUCE_EVENTS_LIBS           = -framework AppKit
JUCE_GRAPHICS_LIBS         = -framework Cocoa -framework QuartzCore
JUCE_GUI_BASICS_LIBS       = -framework Cocoa
JUCE_GUI_EXTRA_LIBS        = -framework Cocoa -framework IOKit
LILV_LIBS                  = -ldl -lm
endif

ifeq ($(WIN32),true)
JACKBRIDGE_LIBS         = -lpthread
JUCE_AUDIO_DEVICES_LIBS = -lwinmm -lole32
JUCE_CORE_LIBS          = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
# JUCE_EVENTS_LIBS        = -lole32
JUCE_GRAPHICS_LIBS      = -lgdi32
JUCE_GUI_BASICS_LIBS    = -lgdi32 -limm32 -lcomdlg32 -lole32
LILV_LIBS               = -lm
endif

# --------------------------------------------------------------
# Set libs stuff (part 3)

ifeq ($(HAVE_ZYN_DEPS),true)
NATIVE_PLUGINS_FLAGS += -DWANT_ZYNADDSUBFX
NATIVE_PLUGINS_LIBS  += $(shell pkg-config --libs fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
NATIVE_PLUGINS_FLAGS += -DWANT_ZYNADDSUBFX_UI
NATIVE_PLUGINS_LIBS  += $(shell pkg-config --libs ntk_images ntk)
endif
endif

# --------------------------------------------------------------
# Set app extension

ifeq ($(WIN32),true)
APP_EXT = .exe
endif

# --------------------------------------------------------------
# Set shared lib extension

LIB_EXT = .so

ifeq ($(MACOS),true)
LIB_EXT = .dylib
endif

ifeq ($(WIN32),true)
LIB_EXT = .dll
endif

# --------------------------------------------------------------
# Set static libs start & end

ifneq ($(MACOS),true)
LIBS_START = -Wl,--start-group
LIBS_END   = -Wl,--end-group
endif

# --------------------------------------------------------------
# Set shared library CLI arg

ifeq ($(MACOS),true)
SHARED = -dynamiclib
else
SHARED = -shared
endif

# --------------------------------------------------------------
