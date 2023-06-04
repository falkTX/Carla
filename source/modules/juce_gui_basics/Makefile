#!/usr/bin/make -f
# Makefile for juce_gui_basics #
# ---------------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=juce_gui_basics
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(JUCE_GUI_BASICS_FLAGS) -I.. -std=gnu++14

# needed for mingw juce
# ifeq ($(WINDOWS),true)
# BUILD_CXX_FLAGS += -Wno-missing-field-initializers -Wno-multichar
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
