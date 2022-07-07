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

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect OS if not defined

TARGET_MACHINE := $(shell $(CC) -dumpmachine)

ifneq ($(BSD),true)
ifneq ($(HAIKU),true)
ifneq ($(HURD),true)
ifneq ($(LINUX),true)
ifneq ($(MACOS),true)
ifneq ($(WASM),true)
ifneq ($(WINDOWS),true)

ifneq (,$(findstring bsd,$(TARGET_MACHINE)))
BSD = true
else ifneq (,$(findstring haiku,$(TARGET_MACHINE)))
HAIKU = true
else ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
else ifneq (,$(findstring gnu,$(TARGET_MACHINE)))
HURD = true
else ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
else ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
else ifneq (,$(findstring msys,$(TARGET_MACHINE)))
WINDOWS = true
else ifneq (,$(findstring wasm,$(TARGET_MACHINE)))
WASM = true
else ifneq (,$(findstring windows,$(TARGET_MACHINE)))
WINDOWS = true
endif

endif # WINDOWS
endif # WASM
endif # MACOS
endif # LINUX
endif # HURD
endif # HAIKU
endif # BSD

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect the processor

TARGET_PROCESSOR := $(firstword $(subst -, ,$(TARGET_MACHINE)))

ifneq (,$(filter i%86,$(TARGET_PROCESSOR)))
CPU_I386 = true
CPU_I386_OR_X86_64 = true
endif
ifneq (,$(filter wasm32,$(TARGET_PROCESSOR)))
CPU_I386 = true
CPU_I386_OR_X86_64 = true
endif
ifneq (,$(filter x86_64,$(TARGET_PROCESSOR)))
CPU_X86_64 = true
CPU_I386_OR_X86_64 = true
endif
ifneq (,$(filter arm%,$(TARGET_PROCESSOR)))
CPU_ARM = true
CPU_ARM_OR_AARCH64 = true
endif
ifneq (,$(filter arm64%,$(TARGET_PROCESSOR)))
CPU_ARM64 = true
CPU_ARM_OR_AARCH64 = true
endif
ifneq (,$(filter aarch64%,$(TARGET_PROCESSOR)))
CPU_AARCH64 = true
CPU_ARM_OR_AARCH64 = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set PKG_CONFIG (can be overridden by environment variable)

ifeq ($(WASM),true)
# Skip on wasm by default
PKG_CONFIG ?= false
else ifeq ($(WINDOWS),true)
# Build statically on Windows by default
PKG_CONFIG ?= pkg-config --static
else
PKG_CONFIG ?= pkg-config
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set cross compiling flag

ifeq ($(WASM),true)
CROSS_COMPILING = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set LINUX_OR_MACOS

ifeq ($(LINUX),true)
LINUX_OR_MACOS = true
endif

ifeq ($(MACOS),true)
LINUX_OR_MACOS = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS, HAIKU_OR_MACOS_OR_WINDOWS, MACOS_OR_WASM_OR_WINDOWS and MACOS_OR_WINDOWS

ifeq ($(HAIKU),true)
HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS = true
HAIKU_OR_MACOS_OR_WINDOWS = true
endif

ifeq ($(MACOS),true)
HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS = true
HAIKU_OR_MACOS_OR_WINDOWS = true
MACOS_OR_WASM_OR_WINDOWS = true
MACOS_OR_WINDOWS = true
endif

ifeq ($(WASM),true)
HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS = true
MACOS_OR_WASM_OR_WINDOWS = true
endif

ifeq ($(WINDOWS),true)
HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS = true
HAIKU_OR_MACOS_OR_WINDOWS = true
MACOS_OR_WASM_OR_WINDOWS = true
MACOS_OR_WINDOWS = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set UNIX

ifeq ($(BSD),true)
UNIX = true
endif

ifeq ($(HURD),true)
UNIX = true
endif

ifeq ($(LINUX),true)
UNIX = true
endif

ifeq ($(MACOS),true)
UNIX = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (required by backend or bridges)

ifeq ($(LINUX),true)
HAVE_ALSA  = $(shell $(PKG_CONFIG) --exists alsa && echo true)
HAVE_HYLIA = true
else ifeq ($(MACOS),true)
HAVE_HYLIA = true
else ifeq ($(WINDOWS),true)
HAVE_HYLIA = true
endif

ifeq ($(MACOS_OR_WASM_OR_WINDOWS),true)
HAVE_DGL     = true
HAVE_YSFXGUI = true
else
HAVE_DGL     = $(shell $(PKG_CONFIG) --exists gl x11 && echo true)
HAVE_YSFXGUI = $(shell $(PKG_CONFIG) --exists freetype2 fontconfig gl x11 && echo true)
HAVE_X11     = $(shell $(PKG_CONFIG) --exists x11 && echo true)
endif

# FIXME update to DGL with wasm compat
ifeq ($(WASM),true)
HAVE_DGL = false
endif

# NOTE not yet implemented, disabled for now
HAVE_YSFXGUI = false

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
HAVE_LIBLO      = $(shell $(PKG_CONFIG) --exists liblo && echo true)
HAVE_QT4        = $(shell $(PKG_CONFIG) --exists QtCore QtGui && echo true)
HAVE_QT5        = $(shell $(PKG_CONFIG) --exists Qt5Core Qt5Gui Qt5Widgets && \
                          $(PKG_CONFIG) --variable=qt_config Qt5Core | grep -q -v "static" && echo true)
HAVE_QT5PKG     = $(shell $(PKG_CONFIG) --silence-errors --variable=prefix Qt5OpenGLExtensions 1>/dev/null && echo true)
HAVE_SNDFILE    = $(shell $(PKG_CONFIG) --exists sndfile && echo true)

ifeq ($(HAVE_FLUIDSYNTH),true)
HAVE_FLUIDSYNTH_INSTPATCH = $(shell $(PKG_CONFIG) --atleast-version=2.1.0 fluidsynth && \
                                    $(PKG_CONFIG) --atleast-version=1.1.4 libinstpatch-1.0 && echo true)
endif

ifeq ($(JACKBRIDGE_DIRECT),true)
HAVE_JACK    = $(shell $(PKG_CONFIG) --exists jack && echo true)
HAVE_JACKLIB = $(HAVE_JACK)
else ifeq ($(WASM),true)
HAVE_JACK    = false
HAVE_JACKLIB = false
else
HAVE_JACK    = true
HAVE_JACKLIB = false
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

ifeq ($(WASM),true)
HAVE_SDL1 = false
HAVE_SDL2 = true
else
HAVE_SDL1 = $(shell $(PKG_CONFIG) --exists sdl && echo true)
HAVE_SDL2 = $(shell $(PKG_CONFIG) --exists sdl2 && echo true)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (special non-pkgconfig tests)

ifneq ($(WASM),true)
ifneq ($(WINDOWS),true)

ifeq ($(shell $(PKG_CONFIG) --exists libmagic && echo true),true)
HAVE_LIBMAGIC = true
HAVE_LIBMAGICPKG = true
else
# old libmagic versions don't have a pkg-config file, so we need to call the compiler to test it
CFLAGS_WITHOUT_ARCH = $(subst -arch arm64,,$(CFLAGS))
HAVE_LIBMAGIC = $(shell echo '\#include <magic.h>' | $(CC) $(CFLAGS_WITHOUT_ARCH) -x c -w -c - -o /dev/null 2>/dev/null && echo true)
endif

endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set Qt tools

ifeq ($(HAVE_QT4),true)
MOC_QT4 ?= $(shell $(PKG_CONFIG) --variable=moc_location QtCore)
RCC_QT4 ?= $(shell $(PKG_CONFIG) --variable=rcc_location QtCore)
ifeq (,$(wildcard $(MOC_QT4)))
HAVE_QT4 = false
endif
ifeq (,$(wildcard $(RCC_QT4)))
HAVE_QT4 = false
endif
endif

ifeq ($(HAVE_QT5),true)
QT5_HOSTBINS = $(shell $(PKG_CONFIG) --variable=host_bins Qt5Core)
MOC_QT5 ?= $(QT5_HOSTBINS)/moc
RCC_QT5 ?= $(QT5_HOSTBINS)/rcc
ifeq (,$(wildcard $(MOC_QT5)))
HAVE_QT5 = false
endif
ifeq (,$(wildcard $(RCC_QT5)))
HAVE_QT5 = false
endif
endif

ifeq ($(HAVE_QT4),true)
HAVE_QT = true
endif
ifeq ($(HAVE_QT5),true)
HAVE_QT = true
endif
ifeq ($(WINDOWS),true)
HAVE_QT = true
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
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set USING_JUCE

ifeq ($(MACOS_OR_WINDOWS),true)
USING_JUCE = true
USING_JUCE_AUDIO_DEVICES = true
else ifeq ($(HAVE_JUCE_LINUX_DEPS),true)
USING_JUCE = true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set USING_RTAUDIO

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
ifneq ($(USING_JUCE_AUDIO_DEVICES),true)
USING_RTAUDIO = true
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Disable features for static plugin target

ifeq ($(STATIC_PLUGIN_TARGET),true)
HAVE_ALSA = false
HAVE_JACK = false
HAVE_JACKLIB = false
HAVE_HYLIA = false
HAVE_LIBLO = false
HAVE_PULSEAUDIO = false
HAVE_PYQT = false
HAVE_QT4 = false
HAVE_QT5 = false
HAVE_QT5PKG = false
USING_JUCE = false
USING_JUCE_AUDIO_DEVICES = false
USING_RTAUDIO = false
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (part 1)

ifneq ($(HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS),true)
LIBDL_LIBS = -ldl
endif

ifeq ($(HAVE_LIBLO),true)
LIBLO_FLAGS = $(shell $(PKG_CONFIG) --cflags liblo)
LIBLO_LIBS  = $(shell $(PKG_CONFIG) --libs liblo)
endif

ifeq ($(HAVE_FFMPEG),true)
FFMPEG_FLAGS = $(shell $(PKG_CONFIG) --cflags libavcodec libavformat libavutil)
FFMPEG_LIBS  = $(shell $(PKG_CONFIG) --libs libavcodec libavformat libavutil)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
FLUIDSYNTH_FLAGS = $(shell $(PKG_CONFIG) --cflags fluidsynth)
FLUIDSYNTH_LIBS  = $(shell $(PKG_CONFIG) --libs fluidsynth)
endif

ifeq ($(HAVE_JACKLIB),true)
JACK_FLAGS  = $(shell $(PKG_CONFIG) --cflags jack)
JACK_LIBS   = $(shell $(PKG_CONFIG) --libs jack)
JACK_LIBDIR = $(shell $(PKG_CONFIG) --variable=libdir jack)/jack
endif

ifeq ($(HAVE_QT5),true)
QT5_FLAGS = $(shell $(PKG_CONFIG) --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LIBS  = $(shell $(PKG_CONFIG) --libs Qt5Core Qt5Gui Qt5Widgets)
endif

ifeq ($(WASM),true)
HAVE_SDL  = true
SDL_FLAGS = -sUSE_SDL=2
SDL_LIBS  = -sUSE_SDL=2
else ifeq ($(HAVE_SDL2),true)
HAVE_SDL  = true
SDL_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags sdl2)
SDL_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs sdl2)
else ifeq ($(HAVE_SDL1),true)
HAVE_SDL  = true
SDL_FLAGS = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --cflags sdl)
SDL_LIBS  = $(shell $(PKG_CONFIG) $(PKG_CONFIG_FLAGS) --libs sdl)
endif

ifeq ($(HAVE_SNDFILE),true)
SNDFILE_FLAGS = $(shell $(PKG_CONFIG) --cflags sndfile)
SNDFILE_LIBS  = $(shell $(PKG_CONFIG) --libs sndfile)
endif

# ifeq ($(HAVE_YSFXGUI),true)
# ifeq ($(MACOS),true)
# YSFX_LIBS  = -framework Cocoa -framework Carbon -framework Metal -framework Foundation
# else ifeq ($(WASM),true)
# # TODO
# YSFX_LIBS  =
# else
# YSFX_FLAGS = $(shell $(PKG_CONFIG) --cflags freetype2 fontconfig)
# YSFX_LIBS  = $(shell $(PKG_CONFIG) --libs freetype2 fontconfig gl x11)
# endif
# endif

ifeq ($(HAVE_X11),true)
X11_FLAGS = $(shell $(PKG_CONFIG) --cflags x11)
X11_LIBS  = $(shell $(PKG_CONFIG) --libs x11)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (dgl)

ifeq ($(HAVE_DGL),true)

ifeq ($(HAIKU),true)
DGL_FLAGS = $(shell $(PKG_CONFIG) --cflags gl)
DGL_LIBS  = $(shell $(PKG_CONFIG) --libs gl) -lbe
else ifeq ($(MACOS),true)
DGL_FLAGS = -DGL_SILENCE_DEPRECATION=1 -Wno-deprecated-declarations
DGL_LIBS  = -framework OpenGL -framework Cocoa -framework CoreVideo
else ifeq ($(WASM),true)
ifneq ($(USE_GLES2)$(USE_GLES3),true)
DGL_LIBS  = -sLEGACY_GL_EMULATION -sGL_UNSAFE_OPTS=0
endif
else ifeq ($(WINDOWS),true)
DGL_LIBS  = -lopengl32 -lgdi32
else
DGL_FLAGS = $(shell $(PKG_CONFIG) --cflags gl x11)
DGL_LIBS  = $(shell $(PKG_CONFIG) --libs gl x11)
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (libmagic)

ifeq ($(HAVE_LIBMAGICPKG),true)

MAGIC_FLAGS  = $(shell $(PKG_CONFIG) --cflags libmagic)
# this is missing in upstream pkg-config
MAGIC_FLAGS += -I$(shell $(PKG_CONFIG) --variable=includedir libmagic)
MAGIC_LIBS   = $(shell $(PKG_CONFIG) --libs libmagic)

else ifeq ($(HAVE_LIBMAGIC),true)

MAGIC_LIBS += -lmagic
ifeq ($(LINUX_OR_MACOS),true)
MAGIC_LIBS += -lz
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (juce)

ifeq ($(USING_JUCE),true)

ifeq ($(LINUX),true)
JUCE_AUDIO_BASICS_LIBS     =
JUCE_AUDIO_DEVICES_LIBS    = $(shell $(PKG_CONFIG) --libs alsa)
JUCE_AUDIO_FORMATS_LIBS    =
JUCE_AUDIO_PROCESSORS_LIBS =
JUCE_CORE_LIBS             = -ldl -pthread -lrt
JUCE_EVENTS_LIBS           =
JUCE_GRAPHICS_LIBS         = $(shell $(PKG_CONFIG) --libs freetype2)
JUCE_GUI_BASICS_LIBS       =
JUCE_GUI_EXTRA_LIBS        =
else ifeq ($(MACOS),true)
JUCE_AUDIO_BASICS_LIBS     = -framework Accelerate
JUCE_AUDIO_DEVICES_LIBS    = -framework AppKit -framework AudioToolbox -framework CoreAudio -framework CoreMIDI
JUCE_AUDIO_FORMATS_LIBS    = -framework AudioToolbox -framework CoreFoundation
JUCE_AUDIO_PROCESSORS_LIBS = -framework AudioToolbox -framework AudioUnit -framework CoreAudio -framework CoreAudioKit -framework Cocoa
JUCE_CORE_LIBS             = -framework AppKit
JUCE_EVENTS_LIBS           = -framework AppKit
JUCE_GRAPHICS_LIBS         = -framework Cocoa -framework QuartzCore
JUCE_GUI_BASICS_LIBS       = -framework Cocoa
JUCE_GUI_EXTRA_LIBS        = -framework IOKit
else ifeq ($(WINDOWS),true)
JUCE_AUDIO_BASICS_LIBS     =
JUCE_AUDIO_DEVICES_LIBS    = -lwinmm -lole32
JUCE_AUDIO_FORMATS_LIBS    =
JUCE_AUDIO_PROCESSORS_LIBS =
JUCE_CORE_LIBS             = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm
JUCE_EVENTS_LIBS           = -lole32
JUCE_GRAPHICS_LIBS         = -lgdi32
JUCE_GUI_BASICS_LIBS       = -lgdi32 -limm32 -lcomdlg32 -lole32
JUCE_GUI_EXTRA_LIBS        =
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (rtaudio)

ifeq ($(USING_RTAUDIO),true)

RTAUDIO_FLAGS    = -DHAVE_GETTIMEOFDAY
RTMIDI_FLAGS     =

ifeq ($(DEBUG),true)
RTAUDIO_FLAGS   += -D__RTAUDIO_DEBUG__
RTMIDI_FLAGS    += -D__RTMIDI_DEBUG__
endif

ifeq ($(HAVE_ALSA),true)
RTAUDIO_FLAGS   += $(shell $(PKG_CONFIG) --cflags alsa) -D__LINUX_ALSA__
RTAUDIO_LIBS    += $(shell $(PKG_CONFIG) --libs alsa) -pthread
RTMIDI_FLAGS    += $(shell $(PKG_CONFIG) --cflags alsa) -D__LINUX_ALSA__
RTMIDI_LIBS     += $(shell $(PKG_CONFIG) --libs alsa)
endif

ifeq ($(HAVE_JACK),true)
ifeq ($(UNIX),true)
RTAUDIO_FLAGS   += -D__UNIX_JACK__
endif
endif

ifeq ($(HAVE_PULSEAUDIO),true)
RTAUDIO_FLAGS   += $(shell $(PKG_CONFIG) --cflags libpulse-simple) -D__UNIX_PULSE__
RTAUDIO_LIBS    += $(shell $(PKG_CONFIG) --libs libpulse-simple)
endif

ifeq ($(MACOS),true)
RTAUDIO_FLAGS   += -D__MACOSX_CORE__
RTAUDIO_LIBS    += -framework CoreAudio
RTMIDI_FLAGS    += -D__MACOSX_CORE__
RTMIDI_LIBS     += -framework CoreMIDI
else ifeq ($(WINDOWS),true)
RTAUDIO_FLAGS   += -D__WINDOWS_ASIO__ -D__WINDOWS_DS__ -D__WINDOWS_WASAPI__
RTAUDIO_LIBS    += -ldsound -luuid -lksuser
RTMIDI_FLAGS    += -D__WINDOWS_MM__
RTMIDI_LIBS     += -lwinmm
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set libs stuff (extra)

ifeq ($(BSD),true)

HYLIA_FLAGS      =
HYLIA_LIBS       =
JACKBRIDGE_LIBS  = -pthread -lrt
LILV_LIBS        = -lm -lrt
RTMEMPOOL_LIBS   = -pthread
WATER_LIBS       = -pthread -lrt

else ifeq ($(HAIKU),true)

HYLIA_FLAGS      =
HYLIA_LIBS       =
JACKBRIDGE_LIBS  = -lpthread
LILV_LIBS        = -lm
RTMEMPOOL_LIBS   = -lpthread
WATER_LIBS       = -lpthread

else ifeq ($(HURD),true)

HYLIA_FLAGS      =
HYLIA_LIBS       =
JACKBRIDGE_LIBS  = -ldl -pthread -lrt
LILV_LIBS        = -ldl -lm -lrt
RTMEMPOOL_LIBS   = -pthread -lrt
WATER_LIBS       = -ldl -pthread -lrt

else ifeq ($(LINUX),true)

HYLIA_FLAGS      = -DLINK_PLATFORM_LINUX=1
HYLIA_LIBS       =
JACKBRIDGE_LIBS  = -ldl -pthread -lrt
LILV_LIBS        = -ldl -lm -lrt
RTMEMPOOL_LIBS   = -pthread -lrt
WATER_LIBS       = -ldl -pthread -lrt

else ifeq ($(MACOS),true)

HYLIA_FLAGS      = -DLINK_PLATFORM_MACOSX=1
HYLIA_LIBS       = 
JACKBRIDGE_LIBS  = -ldl -pthread
LILV_LIBS        = -ldl -lm
RTMEMPOOL_LIBS   = -pthread
WATER_LIBS       = -framework AppKit

else ifeq ($(WASM),true)

HYLIA_FLAGS      =
HYLIA_LIBS       =
JACKBRIDGE_LIBS  =
LILV_LIBS        =
RTMEMPOOL_LIBS   =
WATER_LIBS       =

else ifeq ($(WINDOWS),true)

HYLIA_FLAGS      = -DLINK_PLATFORM_WINDOWS=1
HYLIA_LIBS       = -liphlpapi
JACKBRIDGE_LIBS  = -pthread
LILV_LIBS        = -lm
RTMEMPOOL_LIBS   = -pthread
WATER_LIBS       = -luuid -lwsock32 -lwininet -lversion -lole32 -lws2_32 -loleaut32 -limm32 -lcomdlg32 -lshlwapi -lrpcrt4 -lwinmm

endif

# ---------------------------------------------------------------------------------------------------------------------

AUDIO_DECODER_LIBS  = $(FFMPEG_LIBS)
AUDIO_DECODER_LIBS += $(SNDFILE_LIBS)

NATIVE_PLUGINS_LIBS += $(DGL_LIBS)
NATIVE_PLUGINS_LIBS += $(FFMPEG_LIBS)
NATIVE_PLUGINS_LIBS += $(SNDFILE_LIBS)

STATIC_CARLA_PLUGIN_LIBS  = $(AUDIO_DECODER_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(NATIVE_PLUGINS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(FLUIDSYNTH_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(HYLIA_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(LIBLO_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(LILV_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(MAGIC_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(RTMEMPOOL_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(WATER_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(YSFX_LIBS)

ifeq ($(USING_JUCE),true)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_AUDIO_BASICS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_AUDIO_FORMATS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_AUDIO_PROCESSORS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_CORE_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_EVENTS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_GRAPHICS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_GUI_BASICS_LIBS)
STATIC_CARLA_PLUGIN_LIBS += $(JUCE_GUI_EXTRA_LIBS)
endif

ifeq ($(EXTERNAL_PLUGINS),true)
ifneq ($(DEBUG),true)
ifneq ($(TESTBUILD),true)
ifeq ($(shell $(PKG_CONFIG) --exists liblo fftw3 mxml zlib && echo true),true)
STATIC_CARLA_PLUGIN_LIBS += $(shell $(PKG_CONFIG) --libs liblo fftw3 mxml zlib) -pthread
endif
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
