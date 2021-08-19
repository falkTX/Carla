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
# Internationalization

I18N_LANGUAGES :=

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect OS if not defined

TARGET_MACHINE := $(shell $(CC) -dumpmachine)

ifneq ($(BSD),true)
ifneq ($(HAIKU),true)
ifneq ($(HURD),true)
ifneq ($(LINUX),true)
ifneq ($(MACOS),true)
ifneq ($(WIN32),true)

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
ifneq (,$(findstring x86_64,$(TARGET_MACHINE)))
WIN64=true
endif
endif
ifneq (,$(findstring msys,$(TARGET_MACHINE)))
WIN32=true
endif

endif # WIN32
endif # MACOS
endif # LINUX
endif # HURD
endif # HAIKU
endif # BSD

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect the processor

TARGET_PROCESSOR := $(firstword $(subst -, ,$(TARGET_MACHINE)))

ifneq (,$(filter i%86,$(TARGET_PROCESSOR)))
CPU_I386=true
CPU_I386_OR_X86_64=true
endif
ifneq (,$(filter x86_64,$(TARGET_PROCESSOR)))
CPU_X86_64=true
CPU_I386_OR_X86_64=true
endif
ifneq (,$(filter arm%,$(TARGET_PROCESSOR)))
CPU_ARM=true
CPU_ARM_OR_AARCH64=true
endif
ifneq (,$(filter arm64%,$(TARGET_PROCESSOR)))
CPU_ARM64=true
CPU_ARM_OR_AARCH64=true
endif
ifneq (,$(filter aarch64%,$(TARGET_PROCESSOR)))
CPU_AARCH64=true
CPU_ARM_OR_AARCH64=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set wherever to build static binaries (Windows only)

ifeq ($(WIN32),true)
ifneq ($(MSYSTEM),MINGW32)
ifneq ($(MSYSTEM),MINGW64)
STATIC_BINARIES = true
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set PKG_CONFIG (can be overridden by environment variable)

PKG_CONFIG ?= pkg-config

ifeq ($(STATIC_BINARIES),true)
PKG_CONFIG_FLAGS = --static
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
# Set MACOS_OR_WIN32 and HAIKU_OR_MACOS_OR_WINDOWS

ifeq ($(HAIKU),true)
HAIKU_OR_MACOS_OR_WIN32=true
endif

ifeq ($(MACOS),true)
MACOS_OR_WIN32=true
HAIKU_OR_MACOS_OR_WIN32=true
endif

ifeq ($(WIN32),true)
MACOS_OR_WIN32=true
HAIKU_OR_MACOS_OR_WIN32=true
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

BASE_FLAGS = -Wall -Wextra -pipe -DBUILDING_CARLA -DREAL_BUILD -MD -MP -fno-common
BASE_OPTS  = -O3 -ffast-math -fdata-sections -ffunction-sections

ifeq ($(CPU_I386_OR_X86_64),true)
BASE_OPTS += -mtune=generic -msse -msse2 -mfpmath=sse
endif

ifeq ($(CPU_ARM),true)
ifneq ($(CPU_ARM64),true)
BASE_OPTS += -mfpu=neon-vfpv4 -mfloat-abi=hard
endif
endif

ifeq ($(MACOS),true)
# MacOS linker flags
BASE_FLAGS += -Wno-deprecated-declarations
LINK_OPTS   = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
ifneq ($(SKIP_STRIPPING),true)
LINK_OPTS += -Wl,-x
endif
else
# Common linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed
ifneq ($(SKIP_STRIPPING),true)
LINK_OPTS += -Wl,--strip-all
endif
endif

ifeq ($(NOOPT),true)
# No CPU-specific optimization flags
BASE_OPTS  = -O2 -ffast-math -fdata-sections -ffunction-sections -DBUILDING_CARLA_NOOPT
endif

ifeq ($(WIN32),true)
# mingw has issues with this specific optimization
# See https://github.com/falkTX/Carla/issues/696
BASE_OPTS  += -fno-rerun-cse-after-loop
# See https://github.com/falkTX/Carla/issues/855
BASE_OPTS  += -mstackrealign
ifeq ($(BUILDING_FOR_WINE),true)
BASE_FLAGS += -DBUILDING_CARLA_FOR_WINE
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
ARM32_FLAGS = -mcpu=cortex-a7 -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -mvectorize-with-neon-quad

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

ifeq ($(STATIC_BINARIES),true)
LINK_FLAGS     += -static
endif

# ---------------------------------------------------------------------------------------------------------------------
# Strict test build

ifeq ($(TESTBUILD),true)
BASE_FLAGS += -Werror -Wcast-qual -Wconversion -Wdisabled-optimization
BASE_FLAGS += -Wdouble-promotion -Wfloat-equal -Wpointer-arith -Wsign-conversion
BASE_FLAGS += -Wformat=2 -Woverlength-strings
BASE_FLAGS += -Wmissing-declarations -Wredundant-decls
BASE_FLAGS += -Wshadow  -Wundef -Wuninitialized -Wunused
BASE_FLAGS += -Wstrict-aliasing -fstrict-aliasing
BASE_FLAGS += -Wstrict-overflow -fstrict-overflow
BASE_FLAGS += -Wnull-dereference
ifneq ($(CLANG),true)
BASE_FLAGS += -Wabi=98 -Wclobbered -Wlogical-op
BASE_FLAGS += -Wformat-truncation=2 -Wformat-overflow=2
BASE_FLAGS += -Wstringop-overflow=4 -Wstringop-truncation
BASE_FLAGS += -Wduplicated-branches -Wduplicated-cond
endif
CFLAGS     += -Winit-self -Wmissing-prototypes -Wnested-externs -Wstrict-prototypes -Wwrite-strings
ifneq ($(CLANG),true)
CFLAGS     += -Wjump-misses-init
endif
CXXFLAGS   += -Wc++0x-compat -Wc++11-compat
CXXFLAGS   += -Wnon-virtual-dtor -Woverloaded-virtual
# CXXFLAGS   += -Wold-style-cast -Wuseless-cast
CXXFLAGS   += -Wzero-as-null-pointer-constant
ifneq ($(DEBUG),true)
CXXFLAGS   += -Weffc++
endif
ifeq ($(LINUX),true)
BASE_FLAGS += -isystem /opt/kxstudio/include
endif
ifeq ($(MACOS),true)
CXXFLAGS   += -isystem /System/Library/Frameworks
endif
ifeq ($(WIN32),true)
BASE_FLAGS += -isystem /opt/mingw32/include
endif
ifeq ($(WIN64),true)
BASE_FLAGS += -isystem /opt/mingw64/include
endif
# TODO
ifeq ($(CLANG),true)
BASE_FLAGS += -Wno-double-promotion
BASE_FLAGS += -Wno-format-nonliteral
BASE_FLAGS += -Wno-tautological-pointer-compare
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

ifeq ($(LINUX),true)
HAVE_ALSA  = $(shell $(PKG_CONFIG) --exists alsa && echo true)
HAVE_HYLIA = true
endif

ifeq ($(MACOS),true)
ifneq ($(MACOS_OLD),true)
HAVE_HYLIA = true
endif
endif

ifeq ($(WIN32),true)
HAVE_HYLIA = true
endif

ifeq ($(MACOS_OR_WIN32),true)
HAVE_DGL = true
else
HAVE_DGL = $(shell $(PKG_CONFIG) --exists gl x11 && echo true)
HAVE_X11 = $(shell $(PKG_CONFIG) --exists x11 && echo true)
endif

ifeq ($(UNIX),true)
ifneq ($(MACOS),true)
HAVE_PULSEAUDIO = $(shell $(PKG_CONFIG) --exists libpulse-simple && echo true)
endif
endif

# ffmpeg values taken from https://ffmpeg.org/download.html (v2.8.15 maximum)
HAVE_FFMPEG     = $(shell $(PKG_CONFIG) --max-version=56.60.100 libavcodec && \
                          $(PKG_CONFIG) --max-version=56.40.101 libavformat && \
                          $(PKG_CONFIG) --max-version=54.31.100 libavutil && echo true)
HAVE_FLUIDSYNTH = $(shell $(PKG_CONFIG) --atleast-version=1.1.7 fluidsynth && echo true)
HAVE_JACK       = $(shell $(PKG_CONFIG) --exists jack && echo true)
HAVE_LIBLO      = $(shell $(PKG_CONFIG) --exists liblo && echo true)
HAVE_QT4        = $(shell $(PKG_CONFIG) --exists QtCore QtGui && echo true)
HAVE_QT5        = $(shell $(PKG_CONFIG) --exists Qt5Core Qt5Gui Qt5Widgets && \
                          $(PKG_CONFIG) --variable=qt_config Qt5Core | grep -q -v "static" && echo true)
HAVE_QT5PKG     = $(shell $(PKG_CONFIG) --exists Qt5OpenGLExtensions && echo true)
HAVE_SNDFILE    = $(shell $(PKG_CONFIG) --exists sndfile && echo true)

ifeq ($(HAVE_FLUIDSYNTH),true)
HAVE_FLUIDSYNTH_INSTPATCH = $(shell $(PKG_CONFIG) --atleast-version=2.1.0 fluidsynth && \
                                    $(PKG_CONFIG) --atleast-version=1.1.4 libinstpatch-1.0 && echo true)
endif

ifeq ($(LINUX),true)
# juce only supports the most common architectures
ifneq (,$(findstring arm,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifneq (,$(findstring aarch64,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifneq (,$(findstring i486,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifneq (,$(findstring i586,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifneq (,$(findstring i686,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifneq (,$(findstring x86_64,$(TARGET_MACHINE)))
HAVE_JUCE_SUPPORTED_ARCH = true
endif
ifeq ($(HAVE_JUCE_SUPPORTED_ARCH),true)
HAVE_JUCE_LINUX_DEPS = $(shell $(PKG_CONFIG) --exists x11 xcursor xext freetype2 && echo true)
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (special non-pkgconfig tests)

ifneq ($(WIN32),true)

ifeq ($(shell $(PKG_CONFIG) --exists libmagic && echo true),true)
HAVE_LIBMAGIC = true
else
# old libmagic versions don't have a pkg-config file, so we need to call the compiler to test it
CFLAGS_WITHOUT_ARCH = $(subst -arch arm64,,$(CFLAGS))
HAVE_LIBMAGIC = $(shell echo '\#include <magic.h>' | $(CC) $(CFLAGS_WITHOUT_ARCH) -x c -w -c - -o /dev/null 2>/dev/null && echo true)
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set Qt tools

ifeq ($(HAVE_QT4),true)
MOC_QT4 ?= $(shell $(PKG_CONFIG) --variable=moc_location QtCore)
RCC_QT4 ?= $(shell $(PKG_CONFIG) --variable=rcc_location QtCore)
UIC_QT4 ?= $(shell $(PKG_CONFIG) --variable=uic_location QtCore)
ifeq (,$(wildcard $(MOC_QT4)))
HAVE_QT4=false
endif
ifeq (,$(wildcard $(RCC_QT4)))
HAVE_QT4=false
endif
endif

ifeq ($(HAVE_QT5),true)
QT5_HOSTBINS = $(shell $(PKG_CONFIG) --variable=host_bins Qt5Core)
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
HAVE_PYQT = true
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set PyQt tools, part2

PYRCC ?= $(PYRCC5)
PYUIC ?= $(PYUIC5)

ifeq ($(HAVE_QT5),true)
HAVE_THEME = true
# else
# ifeq ($(MACOS),true)
# ifneq ($(MACOS_OLD),true)
# ifeq ($(HAVE_PYQT),true)
# HAVE_THEME = true
# MOC_QT5 ?= moc
# RCC_QT5 ?= rcc
# UIC_QT5 ?= uic
# endif
# endif
# endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set USING_JUCE

ifeq ($(MACOS_OR_WIN32),true)
ifneq ($(MACOS_OLD),true)
USING_JUCE = true
USING_JUCE_AUDIO_DEVICES = true
endif
endif

ifeq ($(HAVE_JUCE_LINUX_DEPS),true)
USING_JUCE = true
endif

ifeq ($(USING_JUCE),true)
ifeq ($(LINUX_OR_MACOS),true)
USING_JUCE_GUI_EXTRA = true
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set base defines

ifeq ($(JACKBRIDGE_DIRECT),true)
ifeq ($(HAVE_JACK),true)
BASE_FLAGS += -DJACKBRIDGE_DIRECT
else
$(error jackbridge direct mode requested, but jack not available)
endif
endif

ifeq ($(HAVE_DGL),true)
BASE_FLAGS += -DHAVE_DGL
BASE_FLAGS += -DDGL_NAMESPACE=CarlaDGL -DDGL_FILE_BROWSER_DISABLED -DDGL_NO_SHARED_RESOURCES
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
BASE_FLAGS += -DHAVE_FLUIDSYNTH
ifeq ($(HAVE_FLUIDSYNTH_INSTPATCH),true)
BASE_FLAGS += -DHAVE_FLUIDSYNTH_INSTPATCH
endif
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

ifeq ($(USING_JUCE),true)
BASE_FLAGS += -DUSING_JUCE
endif

ifeq ($(USING_JUCE_AUDIO_DEVICES),true)
BASE_FLAGS += -DUSING_JUCE_AUDIO_DEVICES
endif

ifeq ($(USING_JUCE_GUI_EXTRA),true)
BASE_FLAGS += -DUSING_JUCE_GUI_EXTRA
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (part 1)

ifeq ($(LINUX_OR_MACOS),true)
LIBDL_LIBS = -ldl
endif

ifeq ($(HAVE_DGL),true)
ifeq ($(MACOS),true)
DGL_LIBS  = -framework OpenGL -framework Cocoa
endif
ifeq ($(WIN32),true)
DGL_LIBS  = -lopengl32 -lgdi32
endif
ifneq ($(MACOS_OR_WIN32),true)
DGL_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags gl x11)
DGL_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs gl x11)
endif
endif

ifeq ($(HAVE_LIBLO),true)
LIBLO_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags liblo)
LIBLO_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs liblo)
endif

ifeq ($(HAVE_LIBMAGIC),true)
MAGIC_LIBS += -lmagic
ifeq ($(LINUX_OR_MACOS),true)
MAGIC_LIBS += -lz
endif
endif

ifeq ($(HAVE_FFMPEG),true)
FFMPEG_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags libavcodec libavformat libavutil)
FFMPEG_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs libavcodec libavformat libavutil)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs fluidsynth)
endif

ifeq ($(HAVE_JACK),true)
JACK_FLAGS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags jack)
JACK_LIBS   = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs jack)
JACK_LIBDIR = $(shell $(PKG_CONFIG) --variable=libdir jack)/jack
endif

ifeq ($(HAVE_QT5),true)
QT5_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs Qt5Core Qt5Gui Qt5Widgets)
endif

ifeq ($(HAVE_SNDFILE),true)
SNDFILE_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags sndfile)
SNDFILE_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs sndfile)
endif

ifeq ($(HAVE_X11),true)
X11_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags x11)
X11_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs x11)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (part 2)

ifneq ($(USING_JUCE_AUDIO_DEVICES),true)

RTAUDIO_FLAGS    = -DHAVE_GETTIMEOFDAY
RTMIDI_FLAGS     =

ifeq ($(DEBUG),true)
RTAUDIO_FLAGS   += -D__RTAUDIO_DEBUG__
RTMIDI_FLAGS    += -D__RTMIDI_DEBUG__
endif

ifeq ($(LINUX),true)
ifeq ($(HAVE_ALSA),true)
RTAUDIO_FLAGS   += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags alsa) -D__LINUX_ALSA__
RTAUDIO_LIBS    += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs alsa) -lpthread
RTMIDI_FLAGS    += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags alsa) -D__LINUX_ALSA__
RTMIDI_LIBS     += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs alsa)
endif
endif

ifeq ($(MACOS),true)
RTAUDIO_FLAGS   += -D__MACOSX_CORE__
RTAUDIO_LIBS    += -framework CoreAudio
RTMIDI_FLAGS    += -D__MACOSX_CORE__
RTMIDI_LIBS     += -framework CoreMIDI
endif

ifeq ($(UNIX),true)
RTAUDIO_FLAGS   += -D__UNIX_JACK__
ifeq ($(HAVE_PULSEAUDIO),true)
RTAUDIO_FLAGS   += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags libpulse-simple) -D__UNIX_PULSE__
RTAUDIO_LIBS    += $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs libpulse-simple)
endif
endif

ifeq ($(WIN32),true)
RTAUDIO_FLAGS   += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__ -D__WINDOWS_WASAPI__
RTAUDIO_LIBS    += -ldsound -luuid -lksuser -lwinmm
RTMIDI_FLAGS    += -D__WINDOWS_MM__
endif

endif # USING_JUCE_AUDIO_DEVICES

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
ifeq ($(USING_JUCE),true)
JUCE_AUDIO_DEVICES_LIBS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs alsa)
JUCE_CORE_LIBS          = -ldl -lpthread -lrt
JUCE_EVENTS_LIBS        = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs x11)
JUCE_GRAPHICS_LIBS      = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs freetype2)
JUCE_GUI_BASICS_LIBS    = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs x11 xext)
endif # USING_JUCE
endif # LINUX

ifeq ($(MACOS),true)
HYLIA_FLAGS      = -DLINK_PLATFORM_MACOSX=1
JACKBRIDGE_LIBS  = -ldl -lpthread
LILV_LIBS        = -ldl -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -framework AppKit
ifeq ($(USING_JUCE),true)
JUCE_AUDIO_BASICS_LIBS     = -framework Accelerate
JUCE_AUDIO_DEVICES_LIBS    = -framework AppKit -framework AudioToolbox -framework CoreAudio -framework CoreMIDI
JUCE_AUDIO_FORMATS_LIBS    = -framework AudioToolbox -framework CoreFoundation
JUCE_AUDIO_PROCESSORS_LIBS = -framework AudioToolbox -framework AudioUnit -framework CoreAudio -framework CoreAudioKit -framework Cocoa -framework Carbon
JUCE_CORE_LIBS             = -framework AppKit
JUCE_EVENTS_LIBS           = -framework AppKit
JUCE_GRAPHICS_LIBS         = -framework Cocoa -framework QuartzCore
JUCE_GUI_BASICS_LIBS       = -framework Cocoa
JUCE_GUI_EXTRA_LIBS        = -framework IOKit
endif # USING_JUCE
endif # MACOS

ifeq ($(WIN32),true)
HYLIA_FLAGS      = -DLINK_PLATFORM_WINDOWS=1
HYLIA_LIBS       = -liphlpapi
JACKBRIDGE_LIBS  = -lpthread
LILV_LIBS        = -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
ifeq ($(USING_JUCE),true)
JUCE_AUDIO_DEVICES_LIBS    = -lwinmm -lole32
JUCE_CORE_LIBS             = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
JUCE_GRAPHICS_LIBS         = -lgdi32
JUCE_GUI_BASICS_LIBS       = -lgdi32 -limm32 -lcomdlg32 -lole32
endif # USING_JUCE
endif # WIN32

# ---------------------------------------------------------------------------------------------------------------------

AUDIO_DECODER_LIBS  = $(FFMPEG_LIBS)
AUDIO_DECODER_LIBS += $(SNDFILE_LIBS)

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
LIBS_START = -Wl,--start-group -Wl,--whole-archive
LIBS_END   = -Wl,--no-whole-archive -Wl,--end-group
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
# Check if we can generate ttl files

ifneq ($(BUILDING_FOR_WINE),true)
ifeq ($(CROSS_COMPILING),true)
ifeq ($(WIN32),true)
NEEDS_WINE = true
endif
endif
endif

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_LV2_TTL = true
else ifeq ($(NEEDS_WINE),true)
CAN_GENERATE_LV2_TTL = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check if we should build the external plugins

ifeq ($(EXTERNAL_PLUGINS),true)
ifneq ($(DEBUG),true)
ifneq ($(TESTBUILD),true)
ifneq (,$(wildcard $(CWD)/native-plugins/external/Makefile.mk))
BASE_FLAGS += -DHAVE_EXTERNAL_PLUGINS
include $(CWD)/native-plugins/external/Makefile.mk
endif
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
