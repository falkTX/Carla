#!/usr/bin/make -f
# Makefile for libjack #
# -------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/libjack/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/libjack/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
LINK_FLAGS      += $(MODULEDIR)/juce_core.a
LINK_FLAGS      += $(JUCE_CORE_LIBS)

# ----------------------------------------------------------------------------------------------------------------------

OBJS    =
TARGETS =

ifeq ($(LINUX),true)
OBJS    = $(OBJDIR)/libjack.cpp.o
TARGETS = $(BINDIR)/jack/libjack.so.0
endif

# ----------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------

$(BINDIR)/jack/libjack.so.0: $(OBJDIR)/libjack.cpp.o
	-@mkdir -p $(BINDIR)/jack
	@echo "Linking libjack.so.0"
	@$(CXX) $< $(SHARED) $(LINK_FLAGS) $(LIBJACK_LIBS) -o $@

# ----------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/libjack.cpp.o: libjack.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------
