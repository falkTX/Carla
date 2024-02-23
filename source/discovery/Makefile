#!/usr/bin/make -f
# Makefile for carla-discovery #
# ---------------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=carla-discovery
include $(CWD)/Makefile.mk

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

ifeq ($(HAIKU),true)
LINK_FLAGS += -lpthread
else
BUILD_CXX_FLAGS += -pthread
LINK_FLAGS += -pthread
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
LIBS_arm32   += $(MODULEDIR)/water.files.arm32.a
LIBS_posix32 += $(MODULEDIR)/water.posix32.a
LIBS_posix64 += $(MODULEDIR)/water.posix64.a
LIBS_win32   += $(MODULEDIR)/water.win32.a
LIBS_win64   += $(MODULEDIR)/water.win64.a
LINK_FLAGS   += $(WATER_LIBS)
LINK_FLAGS   += $(LIBDL_LIBS)

ifeq ($(HAVE_YSFX),true)
LIBS_native  += $(MODULEDIR)/ysfx.a
LINK_FLAGS   += $(YSFX_GRAPHICS_LIBS)
endif

ifeq ($(BUILDING_FOR_WINE),true)
LIBS_win32   += $(MODULEDIR)/jackbridge.win32e.a
LIBS_win64   += $(MODULEDIR)/jackbridge.win64e.a
endif

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

$(BINDIR)/$(MODULENAME)-native$(APP_EXT): $(OBJS_native) $(LIBS_native)
	-@mkdir -p $(BINDIR)
	@echo "Linking $(MODULENAME)-native$(APP_EXT)"
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
