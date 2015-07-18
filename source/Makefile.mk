#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Modify to enable/disable specific features

# Use the free vestige header instead of the official VST SDK
CARLA_VESTIGE_HEADER = true

# Enable experimental plugins, don't complain if the build fails when using this!
EXPERIMENTAL_PLUGINS = false

# --------------------------------------------------------------
# DO NOT MODIFY PAST THIS POINT!

AR  ?= ar
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
# Set UNIX

ifeq ($(LINUX),true)
UNIX=true
endif

ifeq ($(MACOS),true)
UNIX=true
endif

# --------------------------------------------------------------
# Force some features on MacOS and Windows

ifeq ($(MACOS_OR_WIN32),true)
CARLA_VESTIGE_HEADER = false
EXPERIMENTAL_PLUGINS = false
endif

# --------------------------------------------------------------
# Set build and link flags

BASE_FLAGS = -Wall -Wextra -pipe -DBUILDING_CARLA -DREAL_BUILD -MD -MP
BASE_OPTS  = -O2 -ffast-math -mtune=generic -msse -msse2 -fdata-sections -ffunction-sections

ifneq ($(MACOS),true)
# MacOS doesn't support this
BASE_OPTS += -mfpmath=sse
endif

ifeq ($(MACOS),true)
# MacOS linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
else
# Common linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifeq ($(MODDUO),true)
# MOD Duo optimization flags
BASE_OPTS  = -O2 -ffast-math -march=armv7-a -mtune=cortex-a7 -mfpu=neon -mfloat-abi=hard
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifeq ($(RASPPI),true)
# Raspberry-Pi optimization flags
BASE_OPTS  = -O2 -ffast-math -march=armv6 -mfpu=vfp -mfloat-abi=hard
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifeq ($(PANDORA),true)
# OpenPandora optimization flags
BASE_OPTS  = -O2 -ffast-math -march=armv7-a -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifeq ($(NOOPT),true)
# No optimization flags
BASE_OPTS  = -O2 -ffast-math -fdata-sections -ffunction-sections
endif

ifneq ($(WIN32),true)
# not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(CLANG),true)
BASE_FLAGS += -Wabsolute-value
endif

ifeq ($(STOAT),true)
CC  = clang
CXX = clang++
BASE_FLAGS += -emit-llvm
BASE_OPTS  += -O0
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
ifeq ($(WIN32),true)
BASE_FLAGS += -msse -msse2
endif
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
BASE_FLAGS += -Werror -Wabi -Wcast-qual -Wclobbered -Wconversion -Wdisabled-optimization -Wfloat-equal -Wformat=2 -Winit-self -Wmissing-declarations
BASE_FLAGS += -Woverlength-strings -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-conversion -Wundef -Wuninitialized -Wunused
BASE_FLAGS += -Wstrict-aliasing -fstrict-aliasing
BASE_FLAGS += -Wstrict-overflow -fstrict-overflow
CFLAGS     += -Wnested-externs -Wmissing-prototypes -Wstrict-prototypes -Wwrite-strings
CXXFLAGS   += -Wc++0x-compat -Wc++11-compat -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wzero-as-null-pointer-constant
ifeq ($(LINUX),true)
BASE_FLAGS += -isystem /opt/kxstudio/include
CXXFLAGS   += -isystem /opt/kxstudio/include/ntk
CXXFLAGS   += -isystem /usr/include/qt4
endif
ifeq ($(MACOS),true)
BASE_FLAGS += -isystem /opt/kxstudio/include
CXXFLAGS   += -isystem /System/Library/Frameworks
endif
ifeq ($(WIN64),true)
BASE_FLAGS += -isystem /opt/mingw64/include
else
ifeq ($(WIN32),true)
BASE_FLAGS += -isystem /opt/mingw32/include
endif
endif
endif

# --------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

ifeq ($(MACOS_OR_WIN32),true)
HAVE_DGL        = true
else
HAVE_GTK2       = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3       = $(shell pkg-config --exists gtk+-3.0 && echo true)
ifeq ($(LINUX),true)
HAVE_ALSA       = $(shell pkg-config --exists alsa && echo true)
HAVE_DGL        = $(shell pkg-config --exists gl x11 && echo true)
HAVE_NTK        = $(shell pkg-config --exists ntk ntk_images && echo true)
HAVE_PULSEAUDIO = $(shell pkg-config --exists libpulse-simple && echo true)
HAVE_X11        = $(shell pkg-config --exists x11 && echo true)
endif
endif

HAVE_QT4          = $(shell pkg-config --exists QtCore QtGui && echo true)
HAVE_QT5          = $(shell pkg-config --exists Qt5Core Qt5Gui Qt5Widgets && echo true)

HAVE_LIBLO        = $(shell pkg-config --exists liblo && echo true)
HAVE_FLUIDSYNTH   = $(shell pkg-config --exists fluidsynth && echo true)
HAVE_LINUXSAMPLER = $(shell pkg-config --atleast-version=1.0.0.svn41 linuxsampler && echo true)
HAVE_PROJECTM     = $(shell pkg-config --exists libprojectM && echo true)

# --------------------------------------------------------------
# Check for optional libs (special non-pkgconfig unix tests)

ifeq ($(UNIX),true)

# libmagic doesn't have a pkg-config file, so we need to call the compiler to test it
HAVE_LIBMAGIC = $(shell echo '\#include <magic.h>' | $(CC) $(CFLAGS) -x c -w -c - -o .libmagic-tmp 2>/dev/null && echo true)

# fltk doesn't have a pkg-config file but has fltk-config instead.
# Also, don't try looking for it if we already have NTK.
ifneq ($(HAVE_NTK),true)
ifeq ($(shell which fltk-config 1>/dev/null 2>/dev/null && echo true),true)
ifeq ($(shell which fluid 1>/dev/null 2>/dev/null && echo true),true)
HAVE_FLTK = true
endif
endif
endif

endif

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

ifeq ($(HAVE_QT4),true)
HAVE_QT=true
endif
ifeq ($(HAVE_QT5),true)
HAVE_QT=true
endif
ifeq ($(WIN32),true)
HAVE_QT=true
endif

# --------------------------------------------------------------
# Set PyQt tools

PYUIC4 ?= /usr/bin/pyuic4
PYUIC5 ?= /usr/bin/pyuic5

ifneq (,$(wildcard $(PYUIC4)))
HAVE_PYQT=true
HAVE_PYQT4=true
else
HAVE_PYQT4=false
endif

ifneq (,$(wildcard $(PYUIC5)))
HAVE_PYQT=true
HAVE_PYQT5=true
else
HAVE_PYQT5=false
endif

# --------------------------------------------------------------
# Set default Qt used in frontend

ifeq ($(HAVE_PYQT4),true)
DEFAULT_QT ?= 4
else
DEFAULT_QT ?= 5
endif

# --------------------------------------------------------------
# Set base defines

ifeq ($(HAVE_DGL),true)
BASE_FLAGS += -DHAVE_DGL
endif

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

ifeq ($(HAVE_LIBMAGIC),true)
BASE_FLAGS += -DHAVE_LIBMAGIC
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BASE_FLAGS += -DHAVE_FLUIDSYNTH
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
BASE_FLAGS += -DHAVE_LINUXSAMPLER
endif

ifeq ($(HAVE_PROJECTM),true)
BASE_FLAGS += -DHAVE_PROJECTM
endif

ifeq ($(HAVE_X11),true)
BASE_FLAGS += -DHAVE_X11
endif

ifeq ($(CARLA_VESTIGE_HEADER),true)
BASE_FLAGS += -DVESTIGE_HEADER
endif

# --------------------------------------------------------------
# Set libs stuff (part 1)

ifeq ($(HAVE_LIBLO),true)
LIBLO_FLAGS = $(shell pkg-config --cflags liblo)
LIBLO_LIBS  = $(shell pkg-config --libs liblo)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell pkg-config --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINUXSAMPLER_FLAGS = $(shell pkg-config --cflags linuxsampler) -DIS_CPP11=1 -Wno-non-virtual-dtor -Wno-shadow -Wno-unused-parameter
LINUXSAMPLER_LIBS  = -Wl,-rpath=$(shell pkg-config --variable=libdir gig):$(shell pkg-config --variable=libdir linuxsampler)
LINUXSAMPLER_LIBS += $(shell pkg-config --libs linuxsampler)
endif

ifeq ($(HAVE_PROJECTM),true)
PROJECTM_FLAGS = $(shell pkg-config --cflags libprojectM)
PROJECTM_LIBS  = $(shell pkg-config --libs libprojectM)
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
ifeq ($(HAVE_DGL),true)
DGL_FLAGS             = $(shell pkg-config --cflags gl x11)
DGL_LIBS              = $(shell pkg-config --libs gl x11)
endif
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
DGL_LIBS                   = -framework OpenGL -framework Cocoa
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
DGL_LIBS                = -lopengl32 -lgdi32
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

HAVE_ZYN_DEPS    = $(shell pkg-config --exists liblo fftw3 mxml zlib && echo true)
ifeq ($(HAVE_FLTK),true)
HAVE_ZYN_UI_DEPS = true
endif
ifeq ($(HAVE_NTK),true)
HAVE_ZYN_UI_DEPS = true
endif

ifeq ($(HAVE_DGL),true)
NATIVE_PLUGINS_LIBS  += $(DGL_LIBS)
ifeq ($(HAVE_PROJECTM),true)
NATIVE_PLUGINS_LIBS  += $(PROJECTM_LIBS)
endif
endif

ifeq ($(EXPERIMENTAL_PLUGINS),true)
BASE_FLAGS           += -DHAVE_EXPERIMENTAL_PLUGINS
NATIVE_PLUGINS_LIBS  += -lclthreads -lzita-convolver -lzita-resampler
NATIVE_PLUGINS_LIBS  += $(shell pkg-config --libs fftw3f)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BASE_FLAGS           += -DHAVE_ZYN_DEPS
NATIVE_PLUGINS_LIBS  += $(shell pkg-config --libs liblo fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
BASE_FLAGS           += -DHAVE_ZYN_UI_DEPS
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
