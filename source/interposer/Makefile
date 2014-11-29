#!/usr/bin/make -f
# Makefile for carla-interposer #
# ----------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/interposer/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/interposer/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = $(OBJDIR)/interposer.cpp.o

ifeq ($(LINUX),true)
TARGETS = $(BINDIR)/libcarla_interposer$(LIB_EXT)
else
TARGETS =
endif

ifeq ($(UNIX),true)
LINK_FLAGS += -ldl
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_interposer$(LIB_EXT): $(OBJS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_interposer$(LIB_EXT)"
	@$(CXX) $< $(SHARED) $(LINK_FLAGS) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
