#!/usr/bin/make -f
# Makefile for carla-plugin #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
BUILD_CXX_FLAGS += $(MAGIC_FLAGS)
BUILD_CXX_FLAGS += $(YSFX_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CachedPlugins.cpp.o \
	$(OBJDIR)/CarlaUtils.cpp.o \
	$(OBJDIR)/Information.cpp.o \
	$(OBJDIR)/PipeClient.cpp.o \
	$(OBJDIR)/PluginDiscovery.cpp.o \
	$(OBJDIR)/System.cpp.o \
	$(OBJDIR)/Windows.cpp.o

TARGETS = $(BINDIR)/libcarla_utils$(LIB_EXT)

# ---------------------------------------------------------------------------------------------------------------------

LIBS  = $(MODULEDIR)/jackbridge.a
LIBS += $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/water.files.a

ifeq ($(HAVE_YSFX),true)
LIBS += $(MODULEDIR)/ysfx.a
endif

LINK_FLAGS += $(JACKBRIDGE_LIBS)
LINK_FLAGS += $(LILV_LIBS)
LINK_FLAGS += $(MAGIC_LIBS)
LINK_FLAGS += $(WATER_LIBS)

ifeq ($(HAVE_X11),true)
LINK_FLAGS += $(X11_LIBS)
endif

ifeq ($(HAVE_YSFX),true)
LINK_FLAGS += $(YSFX_GRAPHICS_LIBS)
endif

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
LINK_FLAGS += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_utils$(LIB_EXT): $(OBJS) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_utils$(LIB_EXT)"
	$(SILENT)$(CXX) $(OBJS) $(BUILD_CXX_FLAGS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) $(LINK_FLAGS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaUtils.cpp.o: BUILD_CXX_FLAGS += -ObjC++

$(OBJDIR)/Windows.cpp.o: BUILD_CXX_FLAGS += -ObjC++

endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
