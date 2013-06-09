#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo) -I../../theme

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Widgets)
endif

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo)

ifeq ($(HAVE_QT4),true)
LINK_FLAGS += $(shell pkg-config --libs QtCore QtGui QtXml)
else
LINK_FLAGS += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Xml Qt5Widgets)
endif

# --------------------------------------------------------------
# Engine

ifeq ($(HAVE_ALSA),true)
LINK_FLAGS += $(shell pkg-config --libs alsa)
endif

ifeq ($(HAVE_PULSEAUDIO),true)
LINK_FLAGS += $(shell pkg-config --libs libpulse-simple)
endif

# --------------------------------------------------------------
# Samplers

ifeq ($(HAVE_FLUIDSYNTH),true)
LINK_FLAGS += $(shell pkg-config --libs fluidsynth)
endif

ifeq ($(HAVE_LINUXSAMPLER),true)
LINK_FLAGS += $(shell pkg-config --libs linuxsampler)
endif

# --------------------------------------------------------------
# Native

ifeq ($(WIN32),true)
W32_LIBS  = -lcomdlg32 -ldsound -lgdi32 -limm32 -lole32 -luuid -lwinmm -lwinspool -lws2_32
ifeq ($(HAVE_LINUXSAMPLER),true)
W32_LIBS += -lrpcrt4
endif
else
ifeq ($(MACOS),true)
DGL_LIBS  = -framework Cocoa
else
DGL_LIBS  = -lX11
endif
endif

ifeq ($(HAVE_AF_DEPS),true)
LINK_FLAGS += $(shell pkg-config --libs sndfile)
ifeq ($(HAVE_FFMPEG),true)
LINK_FLAGS += $(shell pkg-config --libs libavcodec libavformat libavutil)
endif
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

EXTRA_LIBS  = -ldl
LINK_FLAGS += $(EXTRA_LIBS)

ifneq ($(MACOS),true)
EXTRA_LIBS += -lrt
endif

# --------------------------------------------------------------

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a
LIBS += ../libcarla_native.a
LIBS += ../../libs/rtmempool.a
LIBS += ../../libs/theme.a
LIBS += ../../libs/widgets.a

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
LIBS += ../../libs/lilv.a
endif

ifeq ($(HAVE_OPENGL),true)
LIBS += ../../libs/dgl.a
endif

OBJS  = \
     CarlaStandalone.cpp.o

ifeq ($(WIN32),true)
TARGET = ../libcarla_standalone.dll
else
ifeq ($(MACOS),true)
TARGET = ../libcarla_standalone.dylib
else
TARGET = ../libcarla_standalone.so
endif
endif

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.cpp.o: %.cpp ../CarlaBackend.hpp ../CarlaEngine.hpp ../CarlaPlugin.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../libcarla_standalone.dll: $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) $(W32_LIBS) -Wl,--output-def,$@.def -o $@ && $(STRIP) $@

../libcarla_standalone.dylib: $(OBJS) $(LIBS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -framework CoreAudio -framework CoreMIDI -framework CoreFoundation -o $@

../libcarla_standalone.so: $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

../libcarla_engine.a:
	$(MAKE) -C ../engine

../libcarla_plugin.a:
	$(MAKE) -C ../plugin

../libcarla_native.a:
	$(MAKE) -C ../native

../../libs/dgl.a:
	$(MAKE) -C ../../libs dgl

../../libs/lilv.a:
	$(MAKE) -C ../../libs lilv

../../libs/rtmempool.a:
	$(MAKE) -C ../../libs rtmempool

../../libs/theme.a:
	$(MAKE) -C ../../libs theme

../../libs/widgets.a:
	$(MAKE) -C ../../libs widgets

.PHONY: ../libcarla_engine.a \
	../libcarla_plugin.a \
	../libcarla_native.a \
	../../libs/dgl.a \
	../../libs/lilv.a \
	../../libs/rtmempool.a \
	../../libs/theme.a \
	../../libs/widgets.a
