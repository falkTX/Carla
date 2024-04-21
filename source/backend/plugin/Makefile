#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

BUILD_CXX_FLAGS += $(MAGIC_FLAGS)

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
BUILD_CXX_FLAGS += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CarlaPlugin.cpp.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.o \
	$(OBJDIR)/CarlaPluginNative.cpp.o \
	$(OBJDIR)/CarlaPluginCLAP.cpp.o \
	$(OBJDIR)/CarlaPluginLADSPADSSI.cpp.o \
	$(OBJDIR)/CarlaPluginLV2.cpp.o \
	$(OBJDIR)/CarlaPluginVST2.cpp.o \
	$(OBJDIR)/CarlaPluginVST3.cpp.o \
	$(OBJDIR)/CarlaPluginAU.cpp.o \
	$(OBJDIR)/CarlaPluginJSFX.cpp.o \
	$(OBJDIR)/CarlaPluginFluidSynth.cpp.o \
	$(OBJDIR)/CarlaPluginSFZero.cpp.o

ifeq ($(HAVE_JACK),true)
OBJS += $(OBJDIR)/CarlaPluginJack.cpp.o
endif

ifneq ($(WASM),true)
OBJS += $(OBJDIR)/CarlaPluginBridge.cpp.o
endif

TARGETS = \
	$(MODULEDIR)/carla_plugin.a

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/carla_plugin.a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_plugin.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/CarlaPluginFluidSynth.cpp.o: CarlaPluginFluidSynth.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) -c -o $@

$(OBJDIR)/CarlaPluginJSFX.cpp.o: CarlaPluginJSFX.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(YSFX_FLAGS) -c -o $@

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaPluginAU.cpp.o: BUILD_CXX_FLAGS += -ObjC++

$(OBJDIR)/CarlaPluginCLAP.cpp.o: BUILD_CXX_FLAGS += -ObjC++

$(OBJDIR)/CarlaPluginVST2.cpp.o: BUILD_CXX_FLAGS += -ObjC++

$(OBJDIR)/CarlaPluginVST3.cpp.o: BUILD_CXX_FLAGS += -ObjC++
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
