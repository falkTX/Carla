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
	$(OBJDIR)/CarlaPluginThread.cpp.o \
	$(OBJDIR)/NativePlugin.cpp.o \
	$(OBJDIR)/BridgePlugin.cpp.o \
	$(OBJDIR)/LadspaPlugin.cpp.o \
	$(OBJDIR)/DssiPlugin.cpp.o \
	$(OBJDIR)/Lv2Plugin.cpp.o \
	$(OBJDIR)/VstPlugin.cpp.o \
	$(OBJDIR)/Vst3Plugin.cpp.o \
	$(OBJDIR)/AuPlugin.cpp.o \
	$(OBJDIR)/JucePlugin.cpp.o \
	$(OBJDIR)/FluidSynthPlugin.cpp.o \
	$(OBJDIR)/LinuxSamplerPlugin.cpp.o

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

$(OBJDIR)/FluidSynthPlugin.cpp.o: FluidSynthPlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(FLUIDSYNTH_FLAGS) -MMD -c -o $@

$(OBJDIR)/LinuxSamplerPlugin.cpp.o: LinuxSamplerPlugin.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(LINUXSAMPLER_FLAGS) -MMD -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -MMD -c -o $@

-include $(OBJS:%.cpp.o=%.cpp.d)

# ----------------------------------------------------------------------------------------------------------------------------
