#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=carla-discovery
include $(CWD)/Makefile.mk

# Workaround GCC bug
ifeq ($(TESTBUILD),true)
ifeq ($(USING_JUCE),true)
BUILD_CXX_FLAGS += -Wno-undef
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/discovery/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/discovery/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
endif

ifeq ($(WINDOWS),true)
LINK_FLAGS += -lole32 -lws2_32 -mwindows
endif

ifeq ($(USING_JUCE),true)
BUILD_CXX_FLAGS += -std=gnu++14
endif

32BIT_FLAGS += -DBUILD_BRIDGE -DBUILD_BRIDGE_ALTERNATIVE_ARCH
64BIT_FLAGS += -DBUILD_BRIDGE -DBUILD_BRIDGE_ALTERNATIVE_ARCH
ARM32_FLAGS += -DBUILD_BRIDGE -DBUILD_BRIDGE_ALTERNATIVE_ARCH

# ---------------------------------------------------------------------------------------------------------------------

NATIVE_BUILD_FLAGS  =
NATIVE_LINK_FLAGS   =

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_BUILD_FLAGS += $(FLUIDSYNTH_FLAGS)
NATIVE_LINK_FLAGS  += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_YSFX),true)
NATIVE_BUILD_FLAGS += $(YSFX_FLAGS)
endif

# ---------------------------------------------------------------------------------------------------------------------

LIBS_native   = $(MODULEDIR)/lilv.a
LIBS_arm32    = $(MODULEDIR)/lilv.arm32.a
LIBS_posix32  = $(MODULEDIR)/lilv.posix32.a
LIBS_posix64  = $(MODULEDIR)/lilv.posix64.a
LIBS_win32    = $(MODULEDIR)/lilv.win32.a
LIBS_win64    = $(MODULEDIR)/lilv.win64.a
LINK_FLAGS   += $(LILV_LIBS)

LIBS_native  += $(MODULEDIR)/water.files.a
LIBS_arm32    = $(MODULEDIR)/water.files.arm32.a
LIBS_posix32  = $(MODULEDIR)/water.posix32.a
LIBS_posix64  = $(MODULEDIR)/water.posix64.a
LIBS_win32    = $(MODULEDIR)/water.win32.a
LIBS_win64    = $(MODULEDIR)/water.win64.a
LINK_FLAGS   += $(WATER_LIBS)
LINK_FLAGS   += $(LIBDL_LIBS)

ifeq ($(HAVE_YSFX),true)
LIBS_native  += $(MODULEDIR)/ysfx.a
LINK_FLAGS   += $(YSFX_GRAPHICS_LIBS)
endif

ifeq ($(USING_JUCE),true)
LIBS_native  += $(MODULEDIR)/carla_juce.a
LIBS_posix32 += $(MODULEDIR)/carla_juce.posix32.a
LIBS_posix64 += $(MODULEDIR)/carla_juce.posix64.a
LIBS_win32   += $(MODULEDIR)/carla_juce.win32.a
LIBS_win64   += $(MODULEDIR)/carla_juce.win64.a

LIBS_native  += $(MODULEDIR)/juce_audio_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_basics.win64.a
LINK_FLAGS   += $(JUCE_AUDIO_BASICS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_audio_processors.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_processors.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_processors.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_processors.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_processors.win64.a
LINK_FLAGS   += $(JUCE_AUDIO_PROCESSORS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_core.a
LIBS_posix32 += $(MODULEDIR)/juce_core.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_core.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_core.win32.a
LIBS_win64   += $(MODULEDIR)/juce_core.win64.a
LINK_FLAGS   += $(JUCE_CORE_LIBS)

LIBS_native  += $(MODULEDIR)/juce_data_structures.a
LIBS_posix32 += $(MODULEDIR)/juce_data_structures.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_data_structures.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_data_structures.win32.a
LIBS_win64   += $(MODULEDIR)/juce_data_structures.win64.a
LINK_FLAGS   += $(JUCE_DATA_STRUCTURES_LIBS)

LIBS_native  += $(MODULEDIR)/juce_events.a
LIBS_posix32 += $(MODULEDIR)/juce_events.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_events.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_events.win32.a
LIBS_win64   += $(MODULEDIR)/juce_events.win64.a
LINK_FLAGS   += $(JUCE_EVENTS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_graphics.a
LIBS_posix32 += $(MODULEDIR)/juce_graphics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_graphics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_graphics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_graphics.win64.a
LINK_FLAGS   += $(JUCE_GRAPHICS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_gui_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_gui_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_gui_basics.win64.a
LINK_FLAGS   += $(JUCE_GUI_BASICS_LIBS)

LIBS_native  += $(MODULEDIR)/juce_gui_extra.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_extra.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_extra.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_gui_extra.win32.a
LIBS_win64   += $(MODULEDIR)/juce_gui_extra.win64.a
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif # USING_JUCE

# ---------------------------------------------------------------------------------------------------------------------

OBJS_native  = $(OBJDIR)/$(MODULENAME).cpp.o
OBJS_arm32   = $(OBJDIR)/$(MODULENAME).cpp.arm32.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).cpp.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).cpp.posix64.o
OBJS_win32   = $(OBJDIR)/$(MODULENAME).cpp.win32.o
OBJS_win64   = $(OBJDIR)/$(MODULENAME).cpp.win64.o

# ---------------------------------------------------------------------------------------------------------------------

all: native

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(BINDIR)/$(MODULENAME)-*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

native:  $(BINDIR)/$(MODULENAME)-native$(APP_EXT)
arm32:   $(BINDIR)/$(MODULENAME)-arm32
posix32: $(BINDIR)/$(MODULENAME)-posix32
posix64: $(BINDIR)/$(MODULENAME)-posix64
win32:   $(BINDIR)/$(MODULENAME)-win32.exe
win64:   $(BINDIR)/$(MODULENAME)-win64.exe

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/$(MODULENAME)-native: $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native"
	$(SILENT)$(CXX) $(OBJS_native) $(LIBS_START) $(LIBS_native) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-native.exe: $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native.exe"
	$(SILENT)$(CXX) $(OBJS_native) $(LIBS_START) $(LIBS_native) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-arm32: $(OBJS_arm32) $(LIBS_arm32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix32"
	$(SILENT)$(CXX) $(OBJS_arm32) $(LIBS_START) $(LIBS_arm32) $(LIBS_END) $(LINK_FLAGS) $(ARM32_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-posix32: $(OBJS_posix32) $(LIBS_posix32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix32"
	$(SILENT)$(CXX) $(OBJS_posix32) $(LIBS_START) $(LIBS_posix32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-posix64: $(OBJS_posix64) $(LIBS_posix64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix64"
	$(SILENT)$(CXX) $(OBJS_posix64) $(LIBS_START) $(LIBS_posix64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win32.exe: $(OBJS_win32) $(LIBS_win32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win32.exe"
	$(SILENT)$(CXX) $(OBJS_win32) $(LIBS_START) $(LIBS_win32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win64.exe: $(OBJS_win64) $(LIBS_win64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win64.exe"
	$(SILENT)$(CXX) $(OBJS_win64) $(LIBS_START) $(LIBS_win64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/$(MODULENAME).cpp.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%32.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%64.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.arm32.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (ARM32)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(ARM32_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_native:%.o=%.d)
-include $(OBJS_arm32:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
