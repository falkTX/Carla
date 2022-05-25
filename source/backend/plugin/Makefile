#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# Workaround GCC bug
ifeq ($(TESTBUILD),true)
ifeq ($(USING_JUCE),true)
BUILD_CXX_FLAGS += -Wno-undef
endif
endif

ifeq ($(USING_JUCE),true)
ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -I$(CWD)/modules/mingw-std-threads
endif
endif

BUILD_CXX_FLAGS += $(MAGIC_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CarlaPlugin.cpp.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.o \
	$(OBJDIR)/CarlaPluginNative.cpp.o \
	$(OBJDIR)/CarlaPluginBridge.cpp.o \
	$(OBJDIR)/CarlaPluginLADSPADSSI.cpp.o \
	$(OBJDIR)/CarlaPluginLV2.cpp.o \
	$(OBJDIR)/CarlaPluginVST2.cpp.o \
	$(OBJDIR)/CarlaPluginVST3.cpp.o \
	$(OBJDIR)/CarlaPluginAU.cpp.o \
	$(OBJDIR)/CarlaPluginJSFX.cpp.o \
	$(OBJDIR)/CarlaPluginJuce.cpp.o \
	$(OBJDIR)/CarlaPluginFluidSynth.cpp.o \
	$(OBJDIR)/CarlaPluginSFZero.cpp.o

ifneq ($(STATIC_PLUGIN_TARGET),true)
OBJS += $(OBJDIR)/CarlaPluginJack.cpp.o
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
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(YSFX_FLAGS) -c -o $@

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaPluginVST2.cpp.o: CarlaPluginVST2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

endif

ifeq ($(USING_JUCE),true)
ifeq ($(MACOS),true)
$(OBJDIR)/CarlaPluginJuce.cpp.o: CarlaPluginJuce.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -std=gnu++14 -c -o $@
else
$(OBJDIR)/CarlaPluginJuce.cpp.o: CarlaPluginJuce.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -std=gnu++14 -c -o $@
endif
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
