#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

CWD=..
include Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

OBJS_standalone = \
	$(OBJDIR)/CarlaStandalone.cpp.o \
	$(OBJDIR)/CarlaStandaloneNSM.cpp.o

OBJS_utils = \
	$(OBJDIR)/CarlaUtils.cpp.o

TARGETS = \
	$(BINDIR)/libcarla_standalone2$(LIB_EXT) \
	$(BINDIR)/libcarla_utils$(LIB_EXT)

# ----------------------------------------------------------------------------------------------------------------------------

STANDALONE_LIBS  = $(MODULEDIR)/carla_engine.a
STANDALONE_LIBS += $(MODULEDIR)/carla_plugin.a

STANDALONE_LIBS += $(MODULEDIR)/jackbridge.a
STANDALONE_LIBS += $(MODULEDIR)/lilv.a
STANDALONE_LIBS += $(MODULEDIR)/native-plugins.a
STANDALONE_LIBS += $(MODULEDIR)/rtmempool.a
STANDALONE_LIBS += $(MODULEDIR)/water.a

ifeq ($(HAVE_DGL),true)
STANDALONE_LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
STANDALONE_LIBS += $(MODULEDIR)/hylia.a
endif

STANDALONE_LIBS += $(MODULEDIR)/rtaudio.a
STANDALONE_LIBS += $(MODULEDIR)/rtmidi.a

UTILS_LIBS += $(MODULEDIR)/lilv.a
UTILS_LIBS += $(MODULEDIR)/water.a

# ----------------------------------------------------------------------------------------------------------------------------

STANDALONE_LINK_FLAGS  = $(JACKBRIDGE_LIBS)
STANDALONE_LINK_FLAGS += $(LILV_LIBS)
STANDALONE_LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)
STANDALONE_LINK_FLAGS += $(RTMEMPOOL_LIBS)
STANDALONE_LINK_FLAGS += $(WATER_LIBS)

STANDALONE_LINK_FLAGS += $(RTAUDIO_LIBS)
STANDALONE_LINK_FLAGS += $(RTMIDI_LIBS)

ifeq ($(HAVE_LIBLO),true)
STANDALONE_LINK_FLAGS += $(LIBLO_LIBS)
endif

ifeq ($(HAVE_LIBMAGIC),true)
STANDALONE_LINK_FLAGS += $(MAGIC_LIBS)
endif

ifeq ($(HAVE_FLUIDSYNTH),true)
STANDALONE_LINK_FLAGS += $(FLUIDSYNTH_LIBS)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
STANDALONE_LINK_FLAGS += $(LINUXSAMPLER_LIBS)
endif

ifeq ($(HAVE_X11),true)
STANDALONE_LINK_FLAGS += $(X11_LIBS)
endif

UTILS_LINK_FLAGS += $(LILV_LIBS)
UTILS_LINK_FLAGS += $(WATER_LIBS)

ifeq ($(HAVE_X11),true)
UTILS_LINK_FLAGS += $(X11_LIBS)
endif

ifneq ($(HAIKU),true)
UTILS_LINK_FLAGS += -lpthread
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS_standalone) $(OBJS_utils) $(TARGETS)
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin

debug:
	$(MAKE) DEBUG=true

doxygen: CarlaBackend.doxygen
	doxygen $^

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_standalone2$(LIB_EXT): $(OBJS_standalone) $(STANDALONE_LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_standalone2$(LIB_EXT)"
	@$(CXX) $(OBJS_standalone) $(LIBS_START) $(STANDALONE_LIBS) $(LIBS_END) $(LINK_FLAGS) $(STANDALONE_LINK_FLAGS) $(SHARED) -o $@

$(BINDIR)/libcarla_utils$(LIB_EXT): $(OBJS_utils) $(UTILS_LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_utils$(LIB_EXT)"
	@$(CXX) $(OBJS_utils) $(LIBS_START) $(UTILS_LIBS) $(LIBS_END) $(LINK_FLAGS) $(UTILS_LINK_FLAGS) $(SHARED) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS_standalone:%.o=%.d)
-include $(OBJS_utils:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
