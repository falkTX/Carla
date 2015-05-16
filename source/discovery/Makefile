#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

ifeq ($(WIN64),true)
winv=64
else
winv=32
endif

CWD=..
MODULENAME=carla-discovery
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/discovery/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/discovery/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/utils -isystem $(CWD)/modules
BUILD_CXX_FLAGS += -I$(CWD)/includes/vst2 -I$(CWD)/includes/vst3

ifeq ($(CARLA_VESTIGE_HEADER),true)
BUILD_CXX_FLAGS += -DVESTIGE_HEADER
endif

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++
endif

# vstsdk requires this
ifeq ($(MACOS_OR_WIN32),true)
BUILD_CXX_FLAGS += -Wno-zero-as-null-pointer-constant
endif

32BIT_FLAGS += -DBUILD_BRIDGE
64BIT_FLAGS += -DBUILD_BRIDGE

# ----------------------------------------------------------------------------------------------------------------------------

NATIVE_BUILD_FLAGS  =
NATIVE_LINK_FLAGS   =

ifeq ($(HAVE_FLUIDSYNTH),true)
NATIVE_BUILD_FLAGS += $(FLUIDSYNTH_FLAGS)
NATIVE_LINK_FLAGS  += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
NATIVE_BUILD_FLAGS += $(LINUXSAMPLER_FLAGS)
NATIVE_LINK_FLAGS  += $(LINUXSAMPLER_LIBS)
endif

# ----------------------------------------------------------------------------------------------------------------------------

LIBS_native   = $(MODULEDIR)/juce_core.a
LIBS_posix32  = $(MODULEDIR)/juce_core.posix32.a
LIBS_posix64  = $(MODULEDIR)/juce_core.posix64.a
LIBS_win32    = $(MODULEDIR)/juce_core.win32.a
LIBS_win64    = $(MODULEDIR)/juce_core.win64.a
LINK_FLAGS   += $(JUCE_CORE_LIBS)

LIBS_native  += $(MODULEDIR)/lilv.a
LIBS_posix32 += $(MODULEDIR)/lilv.posix32.a
LIBS_posix64 += $(MODULEDIR)/lilv.posix64.a
LIBS_win32   += $(MODULEDIR)/lilv.win32.a
LIBS_win64   += $(MODULEDIR)/lilv.win64.a
LINK_FLAGS   += $(LILV_LIBS)

ifeq ($(MACOS_OR_WIN32),true)
LIBS_native  += $(MODULEDIR)/juce_audio_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_basics.win64.a
LIBS_native  += $(MODULEDIR)/juce_audio_processors.a
LIBS_posix32 += $(MODULEDIR)/juce_audio_processors.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_audio_processors.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_audio_processors.win32.a
LIBS_win64   += $(MODULEDIR)/juce_audio_processors.win64.a
LIBS_native  += $(MODULEDIR)/juce_data_structures.a
LIBS_posix32 += $(MODULEDIR)/juce_data_structures.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_data_structures.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_data_structures.win32.a
LIBS_win64   += $(MODULEDIR)/juce_data_structures.win64.a
LIBS_native  += $(MODULEDIR)/juce_events.a
LIBS_posix32 += $(MODULEDIR)/juce_events.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_events.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_events.win32.a
LIBS_win64   += $(MODULEDIR)/juce_events.win64.a
LIBS_native  += $(MODULEDIR)/juce_graphics.a
LIBS_posix32 += $(MODULEDIR)/juce_graphics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_graphics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_graphics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_graphics.win64.a
LIBS_native  += $(MODULEDIR)/juce_gui_basics.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_basics.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_basics.posix64.a
LIBS_win32   += $(MODULEDIR)/juce_gui_basics.win32.a
LIBS_win64   += $(MODULEDIR)/juce_gui_basics.win64.a
ifeq ($(MACOS),true)
LIBS_native  += $(MODULEDIR)/juce_gui_extra.a
LIBS_posix32 += $(MODULEDIR)/juce_gui_extra.posix32.a
LIBS_posix64 += $(MODULEDIR)/juce_gui_extra.posix64.a
endif
LINK_FLAGS   += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS   += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS   += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS   += $(JUCE_EVENTS_LIBS)
LINK_FLAGS   += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS   += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(MACOS),true)
LINK_FLAGS   += $(JUCE_GUI_EXTRA_LIBS)
endif
endif

ifeq ($(UNIX),true)
LINK_FLAGS += -ldl
endif

# ----------------------------------------------------------------------------------------------------------------------------

OBJS_native  = $(OBJDIR)/$(MODULENAME).cpp.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).cpp.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).cpp.posix64.o
OBJS_win32   = $(OBJDIR)/$(MODULENAME).cpp.win32.o
OBJS_win64   = $(OBJDIR)/$(MODULENAME).cpp.win64.o

# ----------------------------------------------------------------------------------------------------------------------------

all: native

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(BINDIR)/$(MODULENAME)-*

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

native:  $(BINDIR)/$(MODULENAME)-native$(APP_EXT)
posix32: $(BINDIR)/$(MODULENAME)-posix32
posix64: $(BINDIR)/$(MODULENAME)-posix64
win32:   $(BINDIR)/$(MODULENAME)-win32.exe
win64:   $(BINDIR)/$(MODULENAME)-win64.exe

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/$(MODULENAME)-native: $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native"
	@$(CXX) $< $(LIBS_START) $(LIBS_native) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-native.exe: $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native.exe"
	@$(CXX) $< $(LIBS_START) $(LIBS_native) $(LIBS_END) $(LINK_FLAGS) $(NATIVE_LINK_FLAGS) -o $@
	@echo "Symlinking $(MODULENAME)-win$(winv).exe"
	@rm -f $(BINDIR)/$(MODULENAME)-win$(winv).exe
	@ln -s $(MODULENAME)-native.exe $(BINDIR)/$(MODULENAME)-win$(winv).exe

$(BINDIR)/$(MODULENAME)-posix32: $(OBJS_posix32) $(LIBS_posix32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix32"
	@$(CXX) $< $(LIBS_START) $(LIBS_posix32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-posix64: $(OBJS_posix64) $(LIBS_posix64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-posix64"
	@$(CXX) $< $(LIBS_START) $(LIBS_posix64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win32.exe: $(OBJS_win32) $(LIBS_win32)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win32.exe"
	@$(CXX) $< $(LIBS_START) $(LIBS_win32) $(LIBS_END) $(LINK_FLAGS) $(32BIT_FLAGS) -o $@

$(BINDIR)/$(MODULENAME)-win64.exe: $(OBJS_win64) $(LIBS_win64)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-win64.exe"
	@$(CXX) $< $(LIBS_START) $(LIBS_win64) $(LIBS_END) $(LINK_FLAGS) $(64BIT_FLAGS) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/$(MODULENAME).cpp.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(NATIVE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%32.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%64.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS_native:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
