#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

CWD=..
include Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

OBJS_standalone = \
	$(OBJDIR)/CarlaStandalone.cpp.o \
	$(OBJDIR)/CarlaStandaloneNSM.cpp.o

ifneq ($(STATIC_PLUGIN_TARGET),true)
TARGETS = \
	$(BINDIR)/libcarla_standalone2$(LIB_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
BUILD_CXX_FLAGS += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

STANDALONE_LIBS  = $(MODULEDIR)/carla_engine.a
STANDALONE_LIBS += $(MODULEDIR)/carla_plugin.a
STANDALONE_LIBS += $(MODULEDIR)/jackbridge.a

STANDALONE_LIBS += $(MODULEDIR)/audio_decoder.a
STANDALONE_LIBS += $(MODULEDIR)/lilv.a
STANDALONE_LIBS += $(MODULEDIR)/native-plugins.a
STANDALONE_LIBS += $(MODULEDIR)/rtmempool.a
STANDALONE_LIBS += $(MODULEDIR)/sfzero.a
STANDALONE_LIBS += $(MODULEDIR)/water.a
STANDALONE_LIBS += $(MODULEDIR)/zita-resampler.a

ifeq ($(HAVE_DGL),true)
STANDALONE_LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
STANDALONE_LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(HAVE_YSFX),true)
STANDALONE_LIBS += $(MODULEDIR)/ysfx.a
endif

ifeq ($(USING_RTAUDIO),true)
STANDALONE_LIBS += $(MODULEDIR)/rtaudio.a
STANDALONE_LIBS += $(MODULEDIR)/rtmidi.a
endif

# ---------------------------------------------------------------------------------------------------------------------

STANDALONE_LINK_FLAGS  = $(HYLIA_LIBS)
STANDALONE_LINK_FLAGS += $(JACKBRIDGE_LIBS)
STANDALONE_LINK_FLAGS += $(LILV_LIBS)
STANDALONE_LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)
STANDALONE_LINK_FLAGS += $(RTMEMPOOL_LIBS)
STANDALONE_LINK_FLAGS += $(WATER_LIBS)

STANDALONE_LINK_FLAGS += $(LIBLO_LIBS)
STANDALONE_LINK_FLAGS += $(MAGIC_LIBS)
STANDALONE_LINK_FLAGS += $(FLUIDSYNTH_LIBS)
STANDALONE_LINK_FLAGS += $(SDL_LIBS)
STANDALONE_LINK_FLAGS += $(X11_LIBS)

ifeq ($(HAVE_YSFX),true)
STANDALONE_LINK_FLAGS += $(YSFX_GRAPHICS_LIBS)
endif

ifeq ($(USING_RTAUDIO),true)
STANDALONE_LINK_FLAGS += $(RTAUDIO_LIBS)
STANDALONE_LINK_FLAGS += $(RTMIDI_LIBS)
endif

ifeq ($(JACKBRIDGE_DIRECT),true)
STANDALONE_LINK_FLAGS += $(JACK_LIBS)
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(WASM),true)
STANDALONE_SHARED  = -sMAIN_MODULE=1 -O3
STANDALONE_SHARED += -sALLOW_MEMORY_GROWTH
STANDALONE_SHARED += -sINITIAL_MEMORY=32Mb
STANDALONE_SHARED += -sSTACK_SIZE=2MB
STANDALONE_SHARED += -lidbfs.js
STANDALONE_SHARED += -sEXPORTED_RUNTIME_METHODS=IDBFS,FS,cwrap
STANDALONE_SHARED += -sMODULARIZE=1 -sEXPORT_NAME=CarlaStandalone
STANDALONE_SHARED += -sENVIRONMENT=web
STANDALONE_SHARED += -sEXPORTED_FUNCTIONS="['_malloc','_free']"
STANDALONE_SHARED += -sEXPORTED_RUNTIME_METHODS="['lengthBytesUTF8','stringToUTF8','UTF8ToString']"
# TESTING
# STANDALONE_SHARED += --use-preload-cache
# STANDALONE_SHARED += --use-preload-plugins
# STANDALONE_SHARED += --preload-file=../frontend/web/resources@/resources
else
STANDALONE_SHARED = $(SHARED)
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
ifneq ($(WASM),true)
	$(MAKE) -C utils
endif

static: $(OBJS_standalone)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS_standalone) $(TARGETS)
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin
	$(MAKE) clean -C utils

debug:
	$(MAKE) DEBUG=true

doxygen: CarlaBackend.doxygen
	doxygen $^

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_standalone2$(LIB_EXT): $(OBJS_standalone) $(STANDALONE_LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_standalone2$(LIB_EXT)"
	$(SILENT)$(CXX) $(OBJS_standalone) $(BUILD_CXX_FLAGS) $(LIBS_START) $(STANDALONE_LIBS) $(LIBS_END) $(LINK_FLAGS) $(STANDALONE_LINK_FLAGS) $(STANDALONE_SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaStandalone.cpp.o: CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS_standalone:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
