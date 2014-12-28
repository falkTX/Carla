#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CarlaPlugin.cpp.o \
	$(OBJDIR)/CarlaPluginInternal.cpp.o \
	$(OBJDIR)/CarlaPluginNative.cpp.o \
	$(OBJDIR)/CarlaPluginBridge.cpp.o \
	$(OBJDIR)/CarlaPluginLADSPA.cpp.o \
	$(OBJDIR)/CarlaPluginDSSI.cpp.o \
	$(OBJDIR)/CarlaPluginLV2.cpp.o \
	$(OBJDIR)/CarlaPluginVST2.cpp.o \
	$(OBJDIR)/CarlaPluginVST3.cpp.o \
	$(OBJDIR)/CarlaPluginAU.cpp.o \
	$(OBJDIR)/CarlaPluginJuce.cpp.o \
	$(OBJDIR)/CarlaPluginFluidSynth.cpp.o \
	$(OBJDIR)/CarlaPluginLinuxSampler.cpp.o

TARGETS = \
	$(MODULEDIR)/carla_plugin.a

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/carla_plugin.a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_plugin.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/CarlaPluginFluidSynth.cpp.o: CarlaPluginFluidSynth.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) -c -o $@

$(OBJDIR)/CarlaPluginLinuxSampler.cpp.o: CarlaPluginLinuxSampler.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
