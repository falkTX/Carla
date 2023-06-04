#!/usr/bin/make -f
# Makefile for juce_audio_basics #
# ------------------------------ #
# Created by falkTX
#

CWD=../..
MODULENAME=juce_audio_basics
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_BASICS_FLAGS) -I.. -std=gnu++14

# needed for older mingw, crashes on optimized float operations
ifeq ($(WINDOWS),true)
ifneq (,$(findstring i686,$(TARGET_MACHINE)))
BUILD_CXX_FLAGS += -mpreferred-stack-boundary=2
else
32BIT_FLAGS     += -mpreferred-stack-boundary=2
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS = $(OBJDIR)/$(MODULENAME).mm.o
else
OBJS = $(OBJDIR)/$(MODULENAME).cpp.o
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/$(MODULENAME).cpp.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/$(MODULENAME).mm.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
