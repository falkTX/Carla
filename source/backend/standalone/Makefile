#!/usr/bin/make -f
# Makefile for carla-standalone #
# ----------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += $(shell pkg-config --cflags liblo) -I../../theme

ifeq ($(HAVE_QT5),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Widgets)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
endif

# --------------------------------------------------------------
# Common

LINK_FLAGS += $(shell pkg-config --libs liblo)

ifeq ($(HAVE_QT5),true)
LINK_FLAGS += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Xml Qt5Widgets)
else
LINK_FLAGS += $(shell pkg-config --libs QtCore QtGui QtXml)
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
DGL_LIBS  = -lX11
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

EXTRA_LIBS  = -ldl -lrt
LINK_FLAGS += $(EXTRA_LIBS)

# --------------------------------------------------------------

LIBS  = ../libcarla_engine.a
LIBS += ../libcarla_plugin.a
LIBS += ../libcarla_native.a
LIBS += ../../libs/rtmempool.a
LIBS += ../../libs/widgets.a
LIBS += ../../libs/theme.a

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
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -framework CoreAudio -framework CoreMidi -framework CoreFoundation -o $@

../libcarla_standalone.so: $(OBJS) $(LIBS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@ && $(STRIP) $@

# --------------------------------------------------------------

../libcarla_%.a:
	$(MAKE) -C ../$* $@

../../libs/%.a:
	$(MAKE) -C ../../libs/ $*.a
