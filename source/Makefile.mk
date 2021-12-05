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
# Base definitions for dependencies and system type

include $(CWD)/Makefile.deps.mk

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
# Assume we want posix
BASE_FLAGS += -posix
# Needed for windows, see https://github.com/falkTX/Carla/issues/855
BASE_FLAGS += -mstackrealign
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

ifeq ($(WITH_LTO),true)
BASE_FLAGS += -fno-strict-aliasing -flto
endif

32BIT_FLAGS = -m32
64BIT_FLAGS = -m64
ARM32_FLAGS = -mcpu=cortex-a7 -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -mvectorize-with-neon-quad

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++11 $(CXXFLAGS)
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

ifeq ($(USING_RTAUDIO),true)
BASE_FLAGS += -DUSING_RTAUDIO
endif

ifeq ($(STATIC_PLUGIN_TARGET),true)
BASE_FLAGS += -DSTATIC_PLUGIN_TARGET
endif

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
