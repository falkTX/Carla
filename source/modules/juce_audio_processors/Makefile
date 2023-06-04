#!/usr/bin/make -f
# Makefile for juce_audio_processors #
# ---------------------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=juce_audio_processors
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_AUDIO_PROCESSORS_FLAGS) -I$(CWD)/includes/vst3sdk -I.. -std=gnu++14

# needed by vst3
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -DDEVELOPMENT -D_DEBUG
else
BUILD_CXX_FLAGS += -DRELEASE
endif

# needed by vst3 on mingw
# ifeq ($(WINDOWS),true)
# BUILD_CXX_FLAGS += -D_NATIVE_WCHAR_T_DEFINED -D__wchar_t=wchar_t -fpermissive -Wno-unknown-pragmas
# # needed for bloated juce
# ifeq ($(DEBUG),true)
# BUILD_CXX_FLAGS += -Wa,-mbig-obj
# endif
# endif

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
