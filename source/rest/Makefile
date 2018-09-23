#!/usr/bin/make -f
# Makefile for carla-interposer #
# ----------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/interposer/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/interposer/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils

LINK_FLAGS += -Wl,-rpath=$(shell realpath $(CWD)/../bin)
LINK_FLAGS += -L$(BINDIR) -lcarla_standalone2 -lcarla_utils
LINK_FLAGS += -lrestbed -lssl -lcrypto
LINK_FLAGS += -lpthread

# ----------------------------------------------------------------------------------------------------------------------

OBJS    = $(OBJDIR)/rest-server.cpp.o $(OBJDIR)/buffers.cpp.o
TARGETS = $(BINDIR)/carla-rest-server

# ----------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-rest-server: $(OBJS)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla-rest-server"
	@$(CXX) $^ $(LINK_FLAGS) -o $@

# ----------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------
