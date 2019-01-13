#!/usr/bin/make -f
# Makefile for Carla C++ code #
# --------------------------- #
# Created by falkTX
#

# ---------------------------------------------------------------------------------------------------------------------
# Base environment vars

AR  ?= ar
CC  ?= gcc
CXX ?= g++

WINECC ?= winegcc

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect OS if not defined

ifneq ($(BSD),true)
ifneq ($(HAIKU),true)
ifneq ($(HURD),true)
ifneq ($(LINUX),true)
ifneq ($(MACOS),true)
ifneq ($(WIN32),true)

TARGET_MACHINE := $(shell $(CC) -dumpmachine)
ifneq (,$(findstring bsd,$(TARGET_MACHINE)))
BSD=true
endif
ifneq (,$(findstring haiku,$(TARGET_MACHINE)))
HAIKU=true
endif
ifneq (,$(findstring gnu,$(TARGET_MACHINE)))
HURD=true
endif
ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX=true
endif
ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS=true
endif
ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WIN32=true
endif

endif
endif
endif
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set LINUX_OR_MACOS

ifeq ($(LINUX),true)
LINUX_OR_MACOS=true
endif

ifeq ($(MACOS),true)
LINUX_OR_MACOS=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set MACOS_OR_WIN32

ifeq ($(MACOS),true)
MACOS_OR_WIN32=true
endif

ifeq ($(WIN32),true)
MACOS_OR_WIN32=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set UNIX

ifeq ($(BSD),true)
UNIX=true
endif

ifeq ($(HURD),true)
UNIX=true
endif

ifeq ($(LINUX),true)
UNIX=true
endif

ifeq ($(MACOS),true)
UNIX=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set build and link flags

BASE_FLAGS = -Wall -Wextra -pipe -DBUILDING_CARLA -DREAL_BUILD -MD -MP
BASE_OPTS  = -O3 -ffast-math -mtune=generic -msse -msse2 -mfpmath=sse -fdata-sections -ffunction-sections

ifeq ($(MACOS),true)
# MacOS linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
else
# Common linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed
ifneq ($(SKIP_STRIPPING),true)
LINK_OPTS += -Wl,--strip-all
endif
endif

ifeq ($(NOOPT),true)
# No CPU-specific optimization flags
BASE_OPTS  = -O2 -ffast-math -fdata-sections -ffunction-sections
endif

ifeq ($(WIN32),true)
# mingw has issues with this specific optimization
# See https://github.com/falkTX/Carla/issues/696
BASE_OPTS  += -fno-rerun-cse-after-loop
ifeq ($(BUILDING_FOR_WINDOWS),true)
BASE_FLAGS += -DBUILDING_CARLA_FOR_WINDOWS
endif
else
# Not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(CLANG),true)
BASE_FLAGS += -Wabsolute-value
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

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)

ifneq ($(MACOS),true)
# Not available on MacOS
LINK_FLAGS     += -Wl,--no-undefined
endif

ifeq ($(MACOS_OLD),true)
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(CXXFLAGS) -DHAVE_CPP11_SUPPORT=0
endif

ifeq ($(WIN32),true)
# Always build statically on windows
LINK_FLAGS     += -static
endif

# ---------------------------------------------------------------------------------------------------------------------
# Strict test build

ifeq ($(TESTBUILD),true)
BASE_FLAGS += -Werror -Wabi=98 -Wcast-qual -Wclobbered -Wconversion -Wdisabled-optimization -Wfloat-equal -Wformat=2 -Winit-self -Wmissing-declarations
BASE_FLAGS += -Woverlength-strings -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-conversion -Wundef -Wuninitialized -Wunused
BASE_FLAGS += -Wstrict-aliasing -fstrict-aliasing
BASE_FLAGS += -Wstrict-overflow -fstrict-overflow
CFLAGS     += -Wnested-externs -Wmissing-prototypes -Wstrict-prototypes -Wwrite-strings
CXXFLAGS   += -Wc++0x-compat -Wc++11-compat -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wzero-as-null-pointer-constant
ifeq ($(LINUX),true)
BASE_FLAGS += -isystem /opt/kxstudio/include
CXXFLAGS   += -isystem /usr/include/glib-2.0
CXXFLAGS   += -isystem /usr/include/glib-2.0/glib
CXXFLAGS   += -isystem /usr/include/gtk-2.0
CXXFLAGS   += -isystem /usr/include/gtk-2.0/gio
CXXFLAGS   += -isystem /usr/include/qt5
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

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

ifeq ($(LINUX),true)
HAVE_ALSA  = $(shell pkg-config --exists alsa && echo true)
HAVE_HYLIA = true
endif

ifeq ($(MACOS),true)
ifneq ($(MACOS_OLD),true)
HAVE_HYLIA = true
endif
endif

ifeq ($(MACOS_OR_WIN32),true)
HAVE_DGL   = true
else
HAVE_DGL   = $(shell pkg-config --exists gl x11 && echo true)
HAVE_GTK2  = $(shell pkg-config --exists gtk+-2.0 && echo true)
HAVE_GTK3  = $(shell pkg-config --exists gtk+-3.0 && echo true)
HAVE_X11   = $(shell pkg-config --exists x11 && echo true)
endif

ifeq ($(UNIX),true)
ifneq ($(MACOS),true)
HAVE_PULSEAUDIO = $(shell pkg-config --exists libpulse-simple && echo true)
endif
endif

HAVE_FFMPEG     = $(shell pkg-config --exists libavcodec libavformat libavutil && echo true)
HAVE_FLUIDSYNTH = $(shell pkg-config --atleast-version=1.1.7 fluidsynth && echo true)
HAVE_LIBLO      = $(shell pkg-config --exists liblo && echo true)
HAVE_QT4        = $(shell pkg-config --exists QtCore QtGui && echo true)
HAVE_QT5        = $(shell pkg-config --exists Qt5Core Qt5Gui Qt5Widgets && echo true)
HAVE_SNDFILE    = $(shell pkg-config --exists sndfile && echo true)

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (special non-pkgconfig tests)

ifneq ($(WIN32),true)

# libmagic doesn't have a pkg-config file, so we need to call the compiler to test it
HAVE_LIBMAGIC = $(shell echo '\#include <magic.h>' | $(CC) $(CFLAGS) -x c -w -c - -o .libmagic-tmp 2>/dev/null && echo true)

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set Qt tools

ifeq ($(HAVE_QT4),true)
MOC_QT4 ?= $(shell pkg-config --variable=moc_location QtCore)
RCC_QT4 ?= $(shell pkg-config --variable=rcc_location QtCore)
UIC_QT4 ?= $(shell pkg-config --variable=uic_location QtCore)
ifeq (,$(wildcard $(MOC_QT4)))
HAVE_QT4=false
endif
ifeq (,$(wildcard $(RCC_QT4)))
HAVE_QT4=false
endif
endif

ifeq ($(HAVE_QT5),true)
QT5_HOSTBINS = $(shell pkg-config --variable=host_bins Qt5Core)
MOC_QT5 ?= $(QT5_HOSTBINS)/moc
RCC_QT5 ?= $(QT5_HOSTBINS)/rcc
UIC_QT5 ?= $(QT5_HOSTBINS)/uic
ifeq (,$(wildcard $(MOC_QT5)))
HAVE_QT5=false
endif
ifeq (,$(wildcard $(RCC_QT5)))
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

# ---------------------------------------------------------------------------------------------------------------------
# Set PyQt tools

PYRCC5 ?= $(shell which pyrcc5 2>/dev/null)
PYUIC5 ?= $(shell which pyuic5 2>/dev/null)

ifneq ($(PYUIC5),)
ifneq ($(PYRCC5),)
HAVE_PYQT=true
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set PyQt tools, part2

PYUIC ?= pyuic5
PYRCC ?= pyrcc5
ifeq ($(HAVE_QT5),true)
HAVE_THEME = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set base defines

ifeq ($(HAVE_DGL),true)
BASE_FLAGS += -DHAVE_DGL
BASE_FLAGS += -DDGL_NAMESPACE=CarlaDGL -DDGL_FILE_BROWSER_DISABLED -DDGL_NO_SHARED_RESOURCES
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BASE_FLAGS += -DHAVE_FLUIDSYNTH
endif

ifeq ($(HAVE_FFMPEG),true)
BASE_FLAGS += -DHAVE_FFMPEG
endif

ifeq ($(HAVE_HYLIA),true)
BASE_FLAGS += -DHAVE_HYLIA
endif

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

ifeq ($(HAVE_LIBMAGIC),true)
BASE_FLAGS += -DHAVE_LIBMAGIC
endif

ifeq ($(HAVE_PYQT),true)
BASE_FLAGS += -DHAVE_PYQT
endif

ifeq ($(HAVE_SNDFILE),true)
BASE_FLAGS += -DHAVE_SNDFILE
endif

ifeq ($(HAVE_X11),true)
BASE_FLAGS += -DHAVE_X11
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (part 1)

ifeq ($(LINUX_OR_MACOS),true)
LIBDL_LIBS = -ldl
endif

ifeq ($(WIN32),true)
PKG_CONFIG_FLAGS = --static
endif

ifeq ($(HAVE_DGL),true)
ifeq ($(MACOS),true)
DGL_LIBS  = -framework OpenGL -framework Cocoa
endif
ifeq ($(WIN32),true)
DGL_LIBS  = -lopengl32 -lgdi32
endif
ifneq ($(MACOS_OR_WIN32),true)
DGL_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags gl x11)
DGL_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs gl x11)
endif
endif

ifeq ($(HAVE_LIBLO),true)
LIBLO_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags liblo)
LIBLO_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs liblo)
endif

ifeq ($(HAVE_LIBMAGIC),true)
MAGIC_LIBS += -lmagic
ifeq ($(LINUX_OR_MACOS),true)
MAGIC_LIBS += -lz
endif
endif

ifeq ($(HAVE_FFMPEG),true)
FFMPEG_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags libavcodec libavformat libavutil)
FFMPEG_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs libavcodec libavformat libavutil)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs fluidsynth)
endif

ifeq ($(HAVE_SNDFILE),true)
SNDFILE_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags sndfile)
SNDFILE_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs sndfile)
endif

ifeq ($(HAVE_X11),true)
X11_FLAGS = $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags x11)
X11_LIBS  = $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs x11)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (part 2)

RTAUDIO_FLAGS    = -DHAVE_GETTIMEOFDAY
RTMIDI_FLAGS     =

ifeq ($(DEBUG),true)
RTAUDIO_FLAGS   += -D__RTAUDIO_DEBUG__
RTMIDI_FLAGS    += -D__RTMIDI_DEBUG__
endif

ifeq ($(UNIX),true)
RTAUDIO_FLAGS   += -D__UNIX_JACK__
ifeq ($(HAVE_PULSEAUDIO),true)
RTAUDIO_FLAGS   += $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags libpulse-simple) -D__UNIX_PULSE__
RTAUDIO_LIBS    += $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs libpulse-simple)
endif
endif

ifeq ($(BSD),true)
JACKBRIDGE_LIBS  = -lpthread -lrt
LILV_LIBS        = -lm -lrt
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -lpthread -lrt
endif

ifeq ($(HAIKU),true)
JACKBRIDGE_LIBS  = -lpthread
LILV_LIBS        = -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -lpthread
endif

ifeq ($(HURD),true)
JACKBRIDGE_LIBS  = -ldl -lpthread -lrt
LILV_LIBS        = -ldl -lm -lrt
RTMEMPOOL_LIBS   = -lpthread -lrt
WATER_LIBS       = -ldl -lpthread -lrt
endif

ifeq ($(LINUX),true)
HYLIA_FLAGS      = -DLINK_PLATFORM_LINUX=1
JACKBRIDGE_LIBS  = -ldl -lpthread -lrt
LILV_LIBS        = -ldl -lm -lrt
RTMEMPOOL_LIBS   = -lpthread -lrt
WATER_LIBS       = -ldl -lpthread -lrt
ifeq ($(HAVE_ALSA),true)
RTAUDIO_FLAGS   += $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags alsa) -D__LINUX_ALSA__
RTAUDIO_LIBS    += $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs alsa) -lpthread
RTMIDI_FLAGS    += $(shell pkg-config $(PKG_CONFIG_FLAGS) --cflags alsa) -D__LINUX_ALSA__
RTMIDI_LIBS     += $(shell pkg-config $(PKG_CONFIG_FLAGS) --libs alsa)
endif
endif

ifeq ($(MACOS),true)
HYLIA_FLAGS      = -DLINK_PLATFORM_MACOSX=1
JACKBRIDGE_LIBS  = -ldl -lpthread
LILV_LIBS        = -ldl -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -framework AppKit
RTAUDIO_FLAGS   += -D__MACOSX_CORE__
RTAUDIO_LIBS    += -framework CoreAudio
RTMIDI_FLAGS    += -D__MACOSX_CORE__
RTMIDI_LIBS     += -framework CoreMIDI
endif

ifeq ($(WIN32),true)
HYLIA_FLAGS      = -DLINK_PLATFORM_WINDOWS=1
JACKBRIDGE_LIBS  = -lpthread
LILV_LIBS        = -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
RTAUDIO_FLAGS   += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__ -D__WINDOWS_WASAPI__
RTAUDIO_LIBS    += -ldsound -luuid -lksuser -lwinmm
RTMIDI_FLAGS    += -D__WINDOWS_MM__
endif

# ---------------------------------------------------------------------------------------------------------------------

NATIVE_PLUGINS_LIBS += $(DGL_LIBS)
NATIVE_PLUGINS_LIBS += $(FFMPEG_LIBS)
NATIVE_PLUGINS_LIBS += $(SNDFILE_LIBS)

# ---------------------------------------------------------------------------------------------------------------------
# Set app extension

ifeq ($(WIN32),true)
APP_EXT = .exe
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set shared lib extension

LIB_EXT = .so

ifeq ($(MACOS),true)
LIB_EXT = .dylib
endif

ifeq ($(WIN32),true)
LIB_EXT = .dll
endif

BASE_FLAGS += -DCARLA_LIB_EXT=\"$(LIB_EXT)\"

# ---------------------------------------------------------------------------------------------------------------------
# Set static libs start & end

ifneq ($(MACOS),true)
LIBS_START = -Wl,--start-group
LIBS_END   = -Wl,--end-group
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set shared library CLI arg

ifeq ($(MACOS),true)
SHARED = -dynamiclib
else
SHARED = -shared
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set arguments used for inline 'sed'

ifeq ($(BSD),true)
SED_ARGS=-i '' -e
else
SED_ARGS=-i -e
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set command used for file symlinking

LINK := ln -sf

# ---------------------------------------------------------------------------------------------------------------------

ifneq (,$(wildcard $(CWD)/native-plugins/external/Makefile.mk))
EXTERNAL_PLUGINS = true
BASE_FLAGS += -DHAVE_EXTERNAL_PLUGINS
include $(CWD)/native-plugins/external/Makefile.mk
endif

# ---------------------------------------------------------------------------------------------------------------------
