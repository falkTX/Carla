#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CachedPlugins.cpp.o \
	$(OBJDIR)/Information.cpp.o \
	$(OBJDIR)/PipeClient.cpp.o \
	$(OBJDIR)/System.cpp.o \
	$(OBJDIR)/Windows.cpp.o

TARGETS = $(BINDIR)/libcarla_utils$(LIB_EXT)

# ----------------------------------------------------------------------------------------------------------------------------

LIBS  = $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/water.files.a

LINK_FLAGS += $(LILV_LIBS)
LINK_FLAGS += $(WATER_LIBS)

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

ifneq ($(HAIKU),true)
LINK_FLAGS += -lpthread
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_utils$(LIB_EXT): $(OBJS) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_utils$(LIB_EXT)"
	@$(CXX) $(OBJS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) $(LINK_FLAGS) $(SHARED) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
$(OBJDIR)/Windows.cpp.o: Windows.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
