#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtGui)

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo QtCore QtGui QtXml)

# --------------------------------------------------------------
# Engine

ifeq ($(HAVE_ALSA),true)
LINK_FLAGS += $(shell pkg-config --libs alsa)
endif

ifeq ($(HAVE_JACK),true)
LINK_FLAGS += $(shell pkg-config --libs jack)
endif

ifeq ($(HAVE_PULSEAUDIO),true)
LINK_FLAGS += $(shell pkg-config --libs libpulse-simple)
endif

# --------------------------------------------------------------
# Plugin

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------
# Native

ifeq ($(WIN32),true)
DGL_LIBS  = -lgdi32
W32_LIBS  = -lcomdlg32 -ldsound -limm32 -lole32 -luuid -lwinmm -lwinspool -lws2_32
ifeq ($(HAVE_LINUXSAMPLER),true)
W32_LIBS += -lrpcrt4
endif
else
DGL_LIBS  = -lX11
endif

ifeq ($(HAVE_AF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs libavcodec libavformat libavutil sndfile)
endif

ifeq ($(HAVE_MF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs smf)
endif

ifeq ($(HAVE_OPENGL),true)
LINK_FLAGS += $(shell pkg-config --libs gl) $(DGL_LIBS)
endif

ifeq ($(HAVE_ZYN_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs fftw3 mxml zlib)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs ntk ntk_images)
endif
endif

LINK_FLAGS += $(EXTRA_LIBS)

# --------------------------------------------------------------

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a
LIBS += ../libcarla_native.a
LIBS += ../../libs/rtmempool.a
LIBS += ../../libs/widgets.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../../libs/lilv.a
endif

ifeq ($(HAVE_OPENGL),true)
LIBS += ../../libs/dgl.a
endif

OBJS  = \
     CarlaStandalone.cpp.o

STATIC = ../libcarla_standalone.a

ifeq ($(WIN32),true)
SHARED = ../libcarla_standalone.dll
else
SHARED = ../libcarla_standalone.so
endif

# --------------------------------------------------------------

all: $(SHARED)

clean:
	rm -f $(OBJS) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(STATIC): $(OBJS) $(LIBS)
	$(AR) rs $@ $^

../libcarla_standalone.dll: $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) $(W32_LIBS) -Wl,--output-def,$@.def -o $@ && $(STRIP) $@

../libcarla_standalone.so: $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

../libcarla_%.a:
	$(MAKE) -C ../$* $@

../../libs/%.a:
	$(MAKE) -C ../../libs/ $*.a
