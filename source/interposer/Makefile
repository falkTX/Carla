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

BUILD_CXX_FLAGS += -I$(CWD)/includes -I$(CWD)/utils

# ----------------------------------------------------------------------------------------------------------------------------

OBJS    =
TARGETS =

ifeq ($(LINUX),true)
OBJS    += $(OBJDIR)/interposer-safe.cpp.o
TARGETS += $(BINDIR)/libcarla_interposer-safe$(LIB_EXT)
ifeq ($(HAVE_X11),true)
OBJS    += $(OBJDIR)/interposer-x11.cpp.o
TARGETS += $(BINDIR)/libcarla_interposer-x11$(LIB_EXT)
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_interposer-safe$(LIB_EXT): $(OBJDIR)/interposer-safe.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_interposer-safe$(LIB_EXT)"
	@$(CXX) $< $(SHARED) $(LINK_FLAGS) -o $@

$(BINDIR)/libcarla_interposer-x11$(LIB_EXT): $(OBJDIR)/interposer-x11.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_interposer-x11$(LIB_EXT)"
	@$(CXX) $< $(SHARED) $(LINK_FLAGS) $(X11_LIBS) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/interposer-safe.cpp.o: interposer-safe.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/interposer-x11.cpp.o: interposer-x11.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(X11_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
